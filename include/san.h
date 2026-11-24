/*
 * The native ABI between Hakusan and San, version 0: lifecycle, content,
 * input, jobs, and output.
 *
 * This header is the authority. The Kotlin declarations in kotlin/ map each
 * function to one JNI method, and abi.tsv cross-checks both sides.
 *
 * Profile. The header is freestanding C11 and includes only <stdint.h>. Its
 * scalars are fixed-width integers, and its pointers point to such integers,
 * to arrays of them, or to structs of them.
 *
 * Linkage. San's library exports only JNI_OnLoad. JNI_OnLoad registers the
 * JNI methods, and each method converts its JNI arguments and calls one
 * function below. Every argument is a JNI primitive or a primitive array,
 * except the destination of san_output_copy, an android.graphics.Bitmap.
 * - An output value travels in a one-element primitive array. A report
 *   travels in a long array whose length is the report's _LENGTH macro, and
 *   its element i holds the value that the report's index macro i names. The
 *   method writes either only after the function returns.
 * - A byte range travels as a byte array, an offset, and a length. It is
 *   valid when the offset and the length are not negative and the length is
 *   at most the array's length minus the offset. The method checks the range
 *   before it requests the array's elements. It may pass the function a
 *   pointer into the array, which the function reads only during the call.
 *   When the platform cannot provide the array's elements, the method clears
 *   the pending exception and returns SAN_STATUS_ALLOCATION_FAILED, and
 *   nothing changes.
 * - A JNI boolean reaches the function as 0 or 1.
 * - A JNI argument that violates its declared shape, such as a null array, an
 *   output or report array of another length, an invalid byte range, or a
 *   destination Bitmap that fails the checks of san_output_copy, returns
 *   SAN_STATUS_BINDING_MISMATCH before any function below runs, so the call
 *   changes nothing and leaves no Java exception pending.
 * - The method's checks, its request for array elements, and its lock of
 *   Bitmap pixels run before the function, so their statuses take precedence
 *   over every status of the function, SAN_STATUS_INVALID_HANDLE included.
 *   Only the unlock of san_output_copy follows the function.
 * Every function uses the C calling convention of the target ABI: AAPCS64 on
 * arm64-v8a and the System V AMD64 ABI on x86_64.
 *
 * Compatibility. JNI_OnLoad fails only when GetEnv fails. When the class
 * app/hakusan/reader/san/SanAbi is missing or a registration fails, it clears
 * the pending exception, leaves no method of the class registered, and still
 * succeeds. The binding establishment call that follows the load then fails
 * with UnsatisfiedLinkError, and a different ABI version fails it with
 * SAN_STATUS_BINDING_MISMATCH. Hakusan reports both as a binding mismatch, and
 * only a failed load as a load failure.
 *
 * Calls. A call runs on the caller's thread, does bounded work, and returns
 * one status. It never waits for input, budget, or a consumer, and it never
 * calls back, raises a Java exception, or unwinds. San's own code takes no
 * lock; san_output_copy may wait briefly for a lock inside the Android bitmap
 * functions, which the render thread also takes. The caller serializes every
 * call on one engine, except san_job_cancel, which follows its own rules. A
 * call that takes a handle counts as a call on the live engine of the process,
 * whatever engine issued the handle: San finds the handle of a content, a job,
 * or a lease of a completed engine invalid only after it reads the state of
 * the live engine. An engine handle of a completed engine fails without that
 * read. Version 0 has one execution profile, the caller's thread with no codec
 * threads, and one pixel profile, which san_output_copy defines.
 *
 * Coordinates. Image coordinates are those of the image as displayed, after
 * any orientation that San applies. A decode path that applies no orientation
 * rejects a valid orientation other than the identity as
 * SAN_STATUS_UNSUPPORTED_OPERATION. A region is the half-open rectangle
 * [left, right) x [top, bottom) in image coordinates. The output grid is the
 * whole image scaled uniformly to a width that never exceeds the image width.
 * A lease is a rectangle of that grid, and its rows count from its own first
 * row.
 *
 * Order. Establish the binding first; no engine exists before that. A content
 * receives the bytes of one representation, its one job decodes them into one
 * output lease, and the caller copies the lease's covered rows. The caller
 * releases every job and every lease, and shutdown completes only after that.
 */
#ifndef SAN_H
#define SAN_H

#include <stdint.h>

/* The ABI version. Hakusan and San must use the same value. */
#define SAN_ABI_VERSION 0

/*
 * The result of a call. Zero is success. Positive values report work that
 * continues. Negative values are the failure categories, each a terminal
 * outcome of the call. A status never carries a warning.
 */
typedef int32_t san_status;

/* The call succeeded. */
#define SAN_STATUS_OK 0
/*
 * The call settled what it could, and the rest waits until a holder releases
 * a resource. Call again after such a release; never poll.
 */
#define SAN_STATUS_PENDING 1
/*
 * The job consumed every byte that its content holds, and the input has not
 * ended. Supply input, then step again.
 */
#define SAN_STATUS_INPUT_NEEDED 2
/*
 * The job made progress and stopped before it consumed every byte that its
 * content holds. Step again without supplying input.
 */
#define SAN_STATUS_PROGRESS 3

/* The profile cannot decode the format. */
#define SAN_STATUS_UNSUPPORTED_FORMAT ( -1 )
/* The profile cannot satisfy the requested access or output. */
#define SAN_STATUS_UNSUPPORTED_OPERATION ( -2 )
/* Metadata or encoded structure violates the format or the profile. */
#define SAN_STATUS_INVALID_CONTENT ( -3 )
/* The input completed, but the encoded image ended before its data. */
#define SAN_STATUS_TRUNCATED_CONTENT ( -4 )
/* The work cannot be admitted within the configured limits. */
#define SAN_STATUS_BUDGET_EXCEEDED ( -5 )
/* A required allocation failed despite admission. */
#define SAN_STATUS_ALLOCATION_FAILED ( -6 )
/* The owner canceled the work. */
#define SAN_STATUS_CANCELED ( -7 )
/* The handle is zero, stale, foreign, released, or of the wrong kind. */
#define SAN_STATUS_INVALID_HANDLE ( -8 )
/* The target is closing or closed and admits no new work. */
#define SAN_STATUS_CLOSED ( -9 )
/*
 * From san_binding_establish, the library and the binding are incompatible.
 * From any other function, the call breaks the binding contract: it runs
 * before the binding is established, passes an argument of the wrong shape or
 * outside its declared range, breaks the call order that this header states,
 * or copies into a destination or from rows that fail the checks of
 * san_output_copy.
 */
#define SAN_STATUS_BINDING_MISMATCH ( -10 )
/* The library is unavailable. Only the Kotlin loader reports it. */
#define SAN_STATUS_LOAD_FAILURE ( -11 )
/* An unexpected recoverable failure ended the operation. */
#define SAN_STATUS_ENGINE_FAILURE ( -12 )

/*
 * An opaque 64-bit handle that San issues and validates. It is never a
 * pointer, and zero is never a valid handle. San rejects a zero, stale,
 * foreign, released, or wrong-kind handle with SAN_STATUS_INVALID_HANDLE and
 * never dereferences it first.
 */
typedef int64_t san_handle;

/* No handle. San never issues it. */
#define SAN_NO_HANDLE 0

/* An output lease or a copy destination has premultiplied alpha. */
#define SAN_ALPHA_PREMULTIPLIED 0
/* An output lease or a copy destination has no alpha: every alpha is 255. */
#define SAN_ALPHA_OPAQUE 1

/*
 * A warning bit of a job: supported SDR content was decoded as sRGB, because
 * its color description was malformed or unsupported. A warning never
 * replaces a failure.
 */
#define SAN_WARNING_SRGB_FALLBACK 1

/*
 * Establishes the binding of this process. Returns SAN_STATUS_OK when
 * abi_version equals SAN_ABI_VERSION; the binding then stays established.
 * Otherwise returns SAN_STATUS_BINDING_MISMATCH and changes nothing. Callable
 * from any thread at any time, and repeatable.
 */
san_status san_binding_establish( int32_t abi_version );

/*
 * The limits of one engine, each named and in its unit. Hakusan selects every
 * value; San has no defaults. Each limit applies on its own and within the
 * aggregate budget, which counts a block once however many limits count it.
 * San checks no relation between two limits. Each count limit, which is
 * content_count, job_count, or output_lease_count, is at most 65535, because
 * a handle names no more contents, jobs, or leases than that.
 */
typedef struct san_engine_limits {
  /* The aggregate budget of every native resource of the engine, in bytes. */
  int64_t aggregate_budget_bytes;
  /* Live contents, each from its opening until its close. */
  int64_t content_count;
  /* Owned jobs, each from its request until its release. */
  int64_t job_count;
  /* The received length of one representation, in bytes. */
  int64_t content_length_bytes;
  /*
   * The bytes of one san_content_feed call, and the staging that opening a
   * content admits, in bytes.
   */
  int64_t input_portion_bytes;
  /* Each image axis, in pixels. */
  int64_t image_axis_pixels;
  /* Image width times height, in pixels. */
  int64_t image_area_pixels;
  /* Native metadata and bookkeeping, expanded metadata included, in bytes. */
  int64_t metadata_bytes;
  /* Expanded ICC and EXIF data live at once, in bytes. */
  int64_t metadata_expanded_bytes;
  /* One expanded ICC profile or EXIF payload, in bytes. */
  int64_t metadata_payload_bytes;
  /*
   * The declared length of a container chunk outside the image data that the
   * decoder holds whole before it handles it, in bytes.
   */
  int64_t held_chunk_bytes;
  /* ICC tag-table entries. */
  int64_t icc_tag_count;
  /* EXIF IFD0 entries that San examines. */
  int64_t exif_entry_count;
  /* Output leases and conversion intermediates together, in bytes. */
  int64_t output_bytes;
  /* One output lease, San's size header included, in bytes. */
  int64_t output_lease_bytes;
  /* Live output leases, each from its job's request until San reclaims it. */
  int64_t output_lease_count;
} san_engine_limits;

/*
 * Creates an engine under *limits, which the call only reads. On success,
 * writes a nonzero handle to *engine. On failure, writes SAN_NO_HANDLE there
 * and releases every partial resource. Both pointers are valid; San's JNI
 * layer passes its own objects.
 *
 * At most one engine is live in a process, from its creation until its
 * shutdown completes. Creation may run on any thread. Returns:
 * - SAN_STATUS_OK;
 * - SAN_STATUS_BINDING_MISMATCH before the binding is established;
 * - SAN_STATUS_BUDGET_EXCEEDED when a limit is below 1, when a count limit
 *   exceeds 65535, when the limits cannot admit the engine's own state, and
 *   while another engine is live;
 * - SAN_STATUS_ALLOCATION_FAILED or SAN_STATUS_ENGINE_FAILURE.
 */
san_status san_engine_create( const san_engine_limits *limits,
                              san_handle              *engine );

/*
 * Advances the shutdown of an engine without blocking. The first call stops
 * admission, ends every live job as canceled, and closes every content. From
 * then on until the shutdown completes, every other call on the engine returns
 * SAN_STATUS_CLOSED, except that san_engine_counters, san_job_cancel and
 * san_job_release of an owned job, and san_output_copy and san_output_release
 * of a held lease work as before. Each call settles what it can, in the
 * shutdown order.
 * Returns:
 * - SAN_STATUS_PENDING while the caller still has to release a job or an
 *   output lease; call again after such a release, and never poll;
 * - SAN_STATUS_OK once every resource is released, which reports the engine
 *   closed; the handle is invalid from then on;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including an engine
 *   whose shutdown completed.
 * The caller owns waiting and any timeout. A timeout never permits a forced
 * release; the engine stays closing.
 */
san_status san_engine_shutdown( san_handle engine );

/* The elements of the counters report of san_engine_counters. */
/* The bytes of live blocks, block headers included. */
#define SAN_ENGINE_COUNTERS_LIVE_BYTES 0
/* The bytes that reservations hold and no block uses yet. */
#define SAN_ENGINE_COUNTERS_RESERVED_BYTES 1
/* The highest live bytes since creation or the last peak reset. */
#define SAN_ENGINE_COUNTERS_PEAK_LIVE_BYTES 2
/* The highest live plus reserved bytes since creation or the last reset. */
#define SAN_ENGINE_COUNTERS_PEAK_COMMITTED_BYTES 3
/* The live bytes of the input staging that contents hold. */
#define SAN_ENGINE_COUNTERS_INPUT_BYTES 4
/* The live bytes of native metadata and bookkeeping. */
#define SAN_ENGINE_COUNTERS_METADATA_BYTES 5
/* The live bytes of output leases and conversion intermediates. */
#define SAN_ENGINE_COUNTERS_OUTPUT_BYTES 6
/*
 * The live bytes of the one block that the engine keeps, once the last claim
 * on an output lease has ended, for its next output lease. No lease holds
 * them. The rest of the live bytes is codec state and workspace.
 */
#define SAN_ENGINE_COUNTERS_KEPT_BYTES 7
/* The live contents. */
#define SAN_ENGINE_COUNTERS_CONTENT_COUNT 8
/* The owned jobs. */
#define SAN_ENGINE_COUNTERS_JOB_COUNT 9
/* The live output leases. */
#define SAN_ENGINE_COUNTERS_LEASE_COUNT 10
/* The number of elements of the counters report. */
#define SAN_ENGINE_COUNTERS_LENGTH 11

/*
 * Writes the counters of an engine to counters, which points to
 * SAN_ENGINE_COUNTERS_LENGTH elements. When reset_peak is 1, the call then
 * sets both peaks to their current values. The counters carry no image
 * content. The call works while the engine shuts down.
 * Returns:
 * - SAN_STATUS_OK, which alone writes the counters;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle;
 * - SAN_STATUS_BINDING_MISMATCH when reset_peak is neither 0 nor 1.
 */
san_status san_engine_counters( san_handle engine, int32_t reset_peak,
                                int64_t *counters );

/*
 * Opens a content for the bytes of one representation, which
 * san_content_feed supplies from position 0. Opening admits and allocates the
 * content's staging of input_portion_bytes, so that a feed only copies. A
 * content admits one job in its lifetime. On success, writes a nonzero handle
 * to *content; otherwise writes SAN_NO_HANDLE there and keeps nothing.
 * Returns:
 * - SAN_STATUS_OK;
 * - SAN_STATUS_INVALID_HANDLE for an invalid engine handle;
 * - SAN_STATUS_CLOSED while the engine shuts down;
 * - SAN_STATUS_BUDGET_EXCEEDED while content_count contents are live, or when
 *   the limits cannot admit the content's state and staging;
 * - SAN_STATUS_ALLOCATION_FAILED or SAN_STATUS_ENGINE_FAILURE.
 */
san_status san_content_open( san_handle engine, san_handle *content );

/*
 * Supplies the length bytes at bytes as the representation's bytes from
 * position on. When end_of_input is 1, the producer reported successful
 * completion after these bytes, and the input ends. A producer failure never
 * reaches San as a feed. position equals the bytes that the content received
 * before; length is 1 to input_portion_bytes, or 0 with end_of_input 1; bytes
 * is null only when length is 0. The call copies the bytes into the content's
 * staging, where the next step of the content's job consumes them. It reads
 * them only during the call, and it allocates nothing and waits for nothing
 * while it reads them.
 * The JNI method's checks under Linkage come first, whatever the handle:
 * SAN_STATUS_BINDING_MISMATCH for a null array or an invalid byte range, then
 * SAN_STATUS_ALLOCATION_FAILED when the platform cannot provide the array's
 * elements. The function's checks then run in this order, and a failed check
 * changes nothing:
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including a content that
 *   san_content_close closed;
 * - SAN_STATUS_CLOSED while the engine shuts down, and after the terminal
 *   outcome of the content's job;
 * - SAN_STATUS_BINDING_MISMATCH when end_of_input is neither 0 nor 1, when
 *   length is outside its range, when the input already ended, when the
 *   content holds bytes that no step has consumed, or when position differs
 *   from the bytes received.
 * Returns SAN_STATUS_OK once it kept the bytes and recorded the end.
 */
san_status san_content_feed( san_handle content, int64_t position,
                             const uint8_t *bytes, int32_t length,
                             int32_t end_of_input );

/*
 * Closes a content. A live job of the content ends as canceled in the call:
 * it is quiescent when the call returns, and its claim on its output lease
 * ends. Its handle stays owned until san_job_release. The content's staging
 * and its job's codec state are released, and output leases stay valid until
 * their holders release them.
 * Returns:
 * - SAN_STATUS_OK once the content is closed; its handle is invalid from then
 *   on;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including a content that
 *   san_content_close closed;
 * - SAN_STATUS_CLOSED while the engine shuts down, which closed it.
 */
san_status san_content_close( san_handle content );

/*
 * An output request for an image of w by h pixels. The region is
 * [left, right) x [top, bottom) in image coordinates, and four zeros request
 * the whole image. Any other region lies inside the image only when
 * right <= w and bottom <= h; a region that extends past the image leaves it,
 * also when it overlaps the image in part. A max_output_width of 0 requests
 * the image at a scale of 1:1. Another value caps the width of the whole
 * image, scaled uniformly, and never scales up: the output grid is
 * W = min(max_output_width, w) columns by
 * H = max(1, (2 * h * W + w) / (2 * w)) rows, so W = w gives a scale of 1:1.
 * The job's lease holds the grid rows [(top * H) / h, (bottom * H + h - 1) / h)
 * and the grid columns [(left * W) / w, (right * W + w - 1) / w), so that
 * adjacent regions share one grid. San computes every term in 64-bit integer
 * arithmetic.
 */
typedef struct san_output_request {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
  int32_t max_output_width;
} san_output_request;

/*
 * Admits one job that decodes the bytes of content into one output lease
 * under *request, which the call only reads. On success, writes a nonzero
 * handle to *job; otherwise writes SAN_NO_HANDLE there and keeps nothing. The
 * caller owns the job until san_job_release.
 *
 * The count limits are checked here. The step that learns the image's
 * dimensions checks the image limits, and a refusal there is the job's
 * terminal SAN_STATUS_BUDGET_EXCEEDED. The job's admission step checks the
 * rest before San writes the first row of the lease and no later than the
 * job's first publication. A refusal there is the job's terminal outcome, and
 * its checks run in this order:
 * - SAN_STATUS_UNSUPPORTED_OPERATION when the region leaves the image or when
 *   the build does not support the region or the resulting scale;
 * - SAN_STATUS_BUDGET_EXCEEDED when the lease exceeds output_lease_bytes,
 *   output_bytes, or the budget.
 * Returns:
 * - SAN_STATUS_OK;
 * - SAN_STATUS_INVALID_HANDLE for an invalid content handle;
 * - SAN_STATUS_CLOSED while the engine shuts down;
 * - SAN_STATUS_BINDING_MISMATCH when the content already admitted its job,
 *   when max_output_width is negative, or when the region is neither four
 *   zeros nor 0 <= left < right and 0 <= top < bottom;
 * - SAN_STATUS_UNSUPPORTED_OPERATION for a region or a scale that the build
 *   does not support, where the request's fields alone decide it;
 * - SAN_STATUS_BUDGET_EXCEEDED while job_count jobs are owned or
 *   output_lease_count leases are live;
 * - SAN_STATUS_ALLOCATION_FAILED or SAN_STATUS_ENGINE_FAILURE.
 */
san_status san_job_request( san_handle                content,
                            const san_output_request *request,
                            san_handle               *job );

/*
 * The elements of the job report of san_job_step. An unknown element is 0.
 * The job's admission step, which san_job_request defines, writes the image
 * and grid elements, from SAN_JOB_REPORT_IMAGE_WIDTH to
 * SAN_JOB_REPORT_OUTPUT_HEIGHT, whatever its outcome, so that a refused
 * request can be followed by one that fits. It writes the lease elements,
 * from SAN_JOB_REPORT_LEASE_WIDTH to SAN_JOB_REPORT_EARLY_PUBLICATION, only
 * when it admits the lease. Both groups are 0 before the admission step and
 * never change after it. A job that ends before its admission step, such as
 * for an image limit or for an orientation that its decode path does not
 * apply, reports neither group.
 */
/*
 * The output lease from the job's first publication on, and SAN_NO_HANDLE
 * before it. The caller's claim on the lease starts with the first report
 * that names it and ends with its one san_output_release.
 */
#define SAN_JOB_REPORT_LEASE 0
/* 0 before the first publication, then one more in each extending call. */
#define SAN_JOB_REPORT_REVISION 1
/* The lease rows [0, n) are covered. A covered row never changes. */
#define SAN_JOB_REPORT_COVERED_ROWS 2
/* The lease width in pixels. */
#define SAN_JOB_REPORT_LEASE_WIDTH 3
/* The lease height in pixels. */
#define SAN_JOB_REPORT_LEASE_HEIGHT 4
/* The lease's first column in the output grid. */
#define SAN_JOB_REPORT_LEASE_LEFT 5
/* The lease's first row in the output grid. */
#define SAN_JOB_REPORT_LEASE_TOP 6
/* The SAN_ALPHA_ value of the lease. */
#define SAN_JOB_REPORT_ALPHA 7
/*
 * 1 when the decode path publishes rows before the job's terminal outcome,
 * and 0 when it publishes once, at the end.
 */
#define SAN_JOB_REPORT_EARLY_PUBLICATION 8
/* The image width in pixels. */
#define SAN_JOB_REPORT_IMAGE_WIDTH 9
/* The image height in pixels. */
#define SAN_JOB_REPORT_IMAGE_HEIGHT 10
/* The output grid's width in pixels, W of san_output_request. */
#define SAN_JOB_REPORT_OUTPUT_WIDTH 11
/* The output grid's height in pixels, H of san_output_request. */
#define SAN_JOB_REPORT_OUTPUT_HEIGHT 12
/* The image rows decoded so far, over every pass, discarded rows included. */
#define SAN_JOB_REPORT_DECODED_ROWS 13
/*
 * The SAN_WARNING_ bits of the job so far. A set bit stays set, and a bit
 * that this header does not define stays 0. The report of the job's terminal
 * outcome holds every bit of the job.
 */
#define SAN_JOB_REPORT_WARNINGS 14
/* The number of elements of the job report. */
#define SAN_JOB_REPORT_LENGTH 15

/*
 * Advances a job. The call consumes the bytes that the job's content holds,
 * as far as they allow, and writes the job's report to report, which points
 * to SAN_JOB_REPORT_LENGTH elements. It observes the job's cancellation flag
 * when it starts, before it consumes a byte, also when the content holds no
 * byte, and at each observation point of its decode path.
 * When it returns a status other than SAN_STATUS_CANCELED, the coverage
 * includes every row that the bytes consumed so far make publishable; no step
 * holds such a row back for a later call.
 * Returns:
 * - SAN_STATUS_INPUT_NEEDED or SAN_STATUS_PROGRESS while the job continues;
 * - SAN_STATUS_OK when the job completed: every row of the lease is covered.
 *   A lease that holds the image's last row also needs the input to end after
 *   the whole image passed every check. A lease that ends above that row
 *   completes once its rows are covered and the bytes consumed so far passed
 *   their checks, and the job then consumes no more input;
 * - SAN_STATUS_UNSUPPORTED_FORMAT, SAN_STATUS_UNSUPPORTED_OPERATION,
 *   SAN_STATUS_INVALID_CONTENT, SAN_STATUS_TRUNCATED_CONTENT,
 *   SAN_STATUS_BUDGET_EXCEEDED, SAN_STATUS_ALLOCATION_FAILED,
 *   SAN_STATUS_CANCELED, or SAN_STATUS_ENGINE_FAILURE when the job failed or
 *   was canceled; a representation longer than content_length_bytes ends it
 *   as SAN_STATUS_BUDGET_EXCEEDED;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including a released
 *   job;
 * - SAN_STATUS_CLOSED after the job's terminal outcome, and while the engine
 *   shuts down.
 * SAN_STATUS_OK and the statuses of a failed or canceled job above are the
 * job's one terminal outcome. The job is quiescent when that call returns,
 * and its claim on the lease ends. The call accepts completion only while the
 * cancellation flag is clear.
 * Every status except SAN_STATUS_INVALID_HANDLE, SAN_STATUS_CLOSED, and
 * SAN_STATUS_BINDING_MISMATCH writes every element of the report. A call that
 * returns SAN_STATUS_CANCELED publishes nothing new: its lease, revision, and
 * covered rows repeat the last earlier publication, or name no lease. Any
 * other call, a terminal one included, may carry a new publication.
 */
san_status san_job_step( san_handle job, int64_t *report );

/*
 * Requests the cancellation of a job. It is the one call on an engine that
 * may run on a thread other than the one that serializes the engine, also
 * while another call on the engine runs, a step of the same job included. The
 * caller owns the job for the whole call: every call returns before the
 * caller releases the job. On a live job, the call sets the cancellation
 * flag, and the step that observes the flag returns SAN_STATUS_CANCELED
 * unless that step already accepted another terminal outcome. On a job after
 * its terminal outcome, the call changes nothing. It works while the engine
 * shuts down.
 * Returns SAN_STATUS_OK, or SAN_STATUS_INVALID_HANDLE for an invalid handle,
 * including a released job.
 */
san_status san_job_cancel( san_handle job );

/*
 * Releases an owned job after its terminal outcome, which a step reported or
 * which a content close or the engine's shutdown settled. The handle is
 * invalid from then on. The call leaves the job's output lease alone: the
 * caller's claim on it ends only with san_output_release. It works while the
 * engine shuts down; after the release of the last owned job and held lease
 * of a closing engine, call san_engine_shutdown again.
 * Returns:
 * - SAN_STATUS_OK;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including a released
 *   job;
 * - SAN_STATUS_BINDING_MISMATCH for a job before its terminal outcome, which
 *   the call leaves unchanged.
 */
san_status san_job_release( san_handle job );

/*
 * Copies the covered rows [first_row, first_row + row_count) of an output
 * lease into dst, from destination row dst_first_row on, in the pixel
 * profile: R, G, B, and A, one byte each, sRGB, with alpha premultiplied in
 * the sRGB-encoded values. dst holds dst_height rows of dst_stride bytes.
 * Each written row receives dst_width * 4 bytes at its start, and the rest of
 * its stride stays unchanged. dst_alpha is SAN_ALPHA_PREMULTIPLIED or
 * SAN_ALPHA_OPAQUE. The call keeps every claim on the lease, and it works
 * while the engine shuts down and after the lease's content closed.
 *
 * The JNI method takes the destination as an android.graphics.Bitmap. Before
 * it locks the pixels, it returns SAN_STATUS_BINDING_MISMATCH for a null
 * Bitmap, and for a Bitmap whose AndroidBitmap_getInfo fails, whose hardware
 * flag is set, whose format is not ANDROID_BITMAP_FORMAT_RGBA_8888, whose
 * alpha flag is neither ANDROID_BITMAP_FLAGS_ALPHA_PREMUL nor
 * ANDROID_BITMAP_FLAGS_ALPHA_OPAQUE, whose data space is not ADATASPACE_SRGB,
 * which rejects a recycled Bitmap, or whose width, height, or stride exceeds
 * INT32_MAX. A failed lock also returns SAN_STATUS_BINDING_MISMATCH. After the
 * lock, the method calls this function with the locked pixels and the
 * Bitmap's geometry and alpha, then unlocks the pixels on every path; the
 * unlock marks the Bitmap changed, also after a rejection.
 *
 * Before it writes, this function checks in this order, in 64-bit
 * arithmetic: the lease handle; a dst_width equal to the lease width;
 * dst_alpha, where SAN_ALPHA_OPAQUE admits only a lease without alpha; a
 * row_count of at least 1 and source rows inside the coverage; and a
 * dst_stride of at least dst_width * 4 with every destination row inside dst.
 * The first failed check decides the status. A failed call writes no pixel
 * and changes no San state.
 * Returns:
 * - SAN_STATUS_OK after it wrote every row;
 * - SAN_STATUS_INVALID_HANDLE for an invalid lease, including one whose
 *   claim its holder released;
 * - SAN_STATUS_BINDING_MISMATCH for every other failed check.
 * A failed unlock means that the Bitmap was recycled during the call. The
 * method then returns SAN_STATUS_BINDING_MISMATCH after a successful copy,
 * which wrote, and the rejection's status after a rejected copy.
 */
san_status san_output_copy( san_handle lease, int32_t first_row,
                            int32_t row_count, uint8_t *dst, int32_t dst_width,
                            int32_t dst_height, int32_t dst_stride,
                            int32_t dst_first_row, int32_t dst_alpha );

/*
 * Releases the caller's claim on an output lease, which started with the
 * first report that named the lease. San reclaims the lease after its last
 * claim: this release or the job's terminal outcome, whichever comes later.
 * The call works while the engine shuts down and after the lease's content
 * closed; after the release of the last owned job and held lease of a closing
 * engine, call san_engine_shutdown again.
 * Returns SAN_STATUS_OK, after which a copy from the lease returns
 * SAN_STATUS_INVALID_HANDLE, or SAN_STATUS_INVALID_HANDLE for an invalid
 * lease, including one whose claim the caller already released.
 */
san_status san_output_release( san_handle lease );

#endif /* SAN_H */
