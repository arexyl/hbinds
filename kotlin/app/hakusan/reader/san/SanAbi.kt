package app.hakusan.reader.san

import android.graphics.Bitmap

/**
 * The JNI surface of San's native ABI, version [ABI_VERSION].
 *
 * The C header `include/san.h` is the authority. Each constant mirrors the
 * header constant whose name adds the `SAN_` prefix. Each external function
 * maps one header function and takes JNI primitives and primitive arrays only,
 * except that [outputCopy] takes its destination as a [Bitmap]. An output value
 * travels in a one-element array, and a report in a [LongArray] whose length is
 * the report's `_LENGTH` constant, indexed by the report's constants. The job's
 * admission step, no later than its first publication, writes the image and
 * grid elements of the job report whatever its outcome, and the lease elements
 * only when it admits the lease; both are 0 before it. San's `JNI_OnLoad`
 * registers the functions. Nothing here loads the library.
 *
 * The object is internal to `:reader`, and its members are public, so the
 * compiler mangles no JVM name. The cross-check table `abi.tsv` lists every
 * constant and every JNI name and descriptor.
 *
 * Call [bindingEstablish] before any other function. The caller serializes
 * every call on one engine, except [jobCancel], which follows its own rules. A
 * call that takes a handle counts as a call on the live engine of the process,
 * whatever engine issued the handle. No function waits for input, budget, or a
 * consumer, calls back, or throws for an engine outcome; each returns a status.
 * [outputCopy] may wait briefly for a lock inside the Android bitmap functions.
 * No function carries `@FastNative` or `@CriticalNative`, because a step may
 * decode for milliseconds.
 */
internal object SanAbi {
  /** The ABI version. Hakusan and San must use the same value. */
  const val ABI_VERSION: Int = 0

  /** The call succeeded. */
  const val STATUS_OK: Int = 0

  /**
   * The call settled what it could, and the rest waits until a holder releases
   * a resource. Call again after such a release; never poll.
   */
  const val STATUS_PENDING: Int = 1

  /**
   * The job consumed every byte that its content holds, and the input has not
   * ended. Supply input, then step again.
   */
  const val STATUS_INPUT_NEEDED: Int = 2

  /**
   * The job made progress and stopped before it consumed every byte that its
   * content holds. Step again without supplying input.
   */
  const val STATUS_PROGRESS: Int = 3

  /** The profile cannot decode the format. */
  const val STATUS_UNSUPPORTED_FORMAT: Int = -1

  /** The profile cannot satisfy the requested access or output. */
  const val STATUS_UNSUPPORTED_OPERATION: Int = -2

  /** Metadata or encoded structure violates the format or the profile. */
  const val STATUS_INVALID_CONTENT: Int = -3

  /** The input completed, but the encoded image ended before its data. */
  const val STATUS_TRUNCATED_CONTENT: Int = -4

  /** The work cannot be admitted within the configured limits. */
  const val STATUS_BUDGET_EXCEEDED: Int = -5

  /** A required allocation failed despite admission. */
  const val STATUS_ALLOCATION_FAILED: Int = -6

  /** The owner canceled the work. */
  const val STATUS_CANCELED: Int = -7

  /** The handle is zero, stale, foreign, released, or of the wrong kind. */
  const val STATUS_INVALID_HANDLE: Int = -8

  /** The target is closing or closed and admits no new work. */
  const val STATUS_CLOSED: Int = -9

  /**
   * From [bindingEstablish], the library and the binding are incompatible. From
   * any other function, the call breaks the binding contract.
   */
  const val STATUS_BINDING_MISMATCH: Int = -10

  /** The library is unavailable. Only the Kotlin loader reports it. */
  const val STATUS_LOAD_FAILURE: Int = -11

  /** An unexpected recoverable failure ended the operation. */
  const val STATUS_ENGINE_FAILURE: Int = -12

  /** No handle. San never issues it. */
  const val NO_HANDLE: Long = 0L

  /** An output lease or a copy destination has premultiplied alpha. */
  const val ALPHA_PREMULTIPLIED: Int = 0

  /** An output lease or a copy destination has no alpha. */
  const val ALPHA_OPAQUE: Int = 1

  /** A job decoded supported SDR content as sRGB, under a warning. */
  const val WARNING_SRGB_FALLBACK: Int = 1

  /** Counters: the bytes of live blocks, block headers included. */
  const val ENGINE_COUNTERS_LIVE_BYTES: Int = 0

  /** Counters: the bytes that reservations hold and no block uses yet. */
  const val ENGINE_COUNTERS_RESERVED_BYTES: Int = 1

  /** Counters: the highest live bytes since creation or the last reset. */
  const val ENGINE_COUNTERS_PEAK_LIVE_BYTES: Int = 2

  /** Counters: the highest live plus reserved bytes since creation or reset. */
  const val ENGINE_COUNTERS_PEAK_COMMITTED_BYTES: Int = 3

  /** Counters: the live bytes of the input staging of contents. */
  const val ENGINE_COUNTERS_INPUT_BYTES: Int = 4

  /** Counters: the live bytes of native metadata and bookkeeping. */
  const val ENGINE_COUNTERS_METADATA_BYTES: Int = 5

  /** Counters: the live bytes of output leases and intermediates. */
  const val ENGINE_COUNTERS_OUTPUT_BYTES: Int = 6

  /** Counters: the live bytes of the block kept for the next output lease. */
  const val ENGINE_COUNTERS_KEPT_BYTES: Int = 7

  /** Counters: the live contents. */
  const val ENGINE_COUNTERS_CONTENT_COUNT: Int = 8

  /** Counters: the owned jobs. */
  const val ENGINE_COUNTERS_JOB_COUNT: Int = 9

  /** Counters: the live output leases. */
  const val ENGINE_COUNTERS_LEASE_COUNT: Int = 10

  /** The number of elements of the counters report. */
  const val ENGINE_COUNTERS_LENGTH: Int = 11

  /** Report: the output lease from the first publication on, or none. */
  const val JOB_REPORT_LEASE: Int = 0

  /** Report: 0, then one more in each call that extends the coverage. */
  const val JOB_REPORT_REVISION: Int = 1

  /** Report: the lease rows `[0, n)` are covered. */
  const val JOB_REPORT_COVERED_ROWS: Int = 2

  /** Report: the lease width in pixels, once the lease is admitted. */
  const val JOB_REPORT_LEASE_WIDTH: Int = 3

  /** Report: the lease height in pixels, once the lease is admitted. */
  const val JOB_REPORT_LEASE_HEIGHT: Int = 4

  /** Report: the lease's first column in the output grid. */
  const val JOB_REPORT_LEASE_LEFT: Int = 5

  /** Report: the lease's first row in the output grid. */
  const val JOB_REPORT_LEASE_TOP: Int = 6

  /** Report: the lease's alpha, once the lease is admitted. */
  const val JOB_REPORT_ALPHA: Int = 7

  /** Report: 1 when rows publish before the terminal outcome, else 0. */
  const val JOB_REPORT_EARLY_PUBLICATION: Int = 8

  /** Report: the image width in pixels, from the admission step on. */
  const val JOB_REPORT_IMAGE_WIDTH: Int = 9

  /** Report: the image height in pixels, from the admission step on. */
  const val JOB_REPORT_IMAGE_HEIGHT: Int = 10

  /** Report: the output grid's width, from the admission step on. */
  const val JOB_REPORT_OUTPUT_WIDTH: Int = 11

  /** Report: the output grid's height, from the admission step on. */
  const val JOB_REPORT_OUTPUT_HEIGHT: Int = 12

  /** Report: the image rows decoded so far, discarded rows included. */
  const val JOB_REPORT_DECODED_ROWS: Int = 13

  /** Report: the [WARNING_SRGB_FALLBACK] bits so far; a set bit stays set. */
  const val JOB_REPORT_WARNINGS: Int = 14

  /** The number of elements of the job report. */
  const val JOB_REPORT_LENGTH: Int = 15

  /**
   * Establishes the binding of this process. Returns [STATUS_OK] when
   * [abiVersion] equals the library's ABI version, which allows engine creation
   * from then on, and [STATUS_BINDING_MISMATCH] otherwise.
   *
   * @throws UnsatisfiedLinkError if the loaded library registered no function
   *   of this object, as after a missing class or a failed registration. That
   *   is a binding mismatch too.
   */
  @JvmStatic external fun bindingEstablish(abiVersion: Int): Int

  /**
   * Creates an engine under the named limits, each at least 1 and each count
   * limit at most 65535, in the field order of `san_engine_limits`. Writes the
   * new handle, or [NO_HANDLE] on failure, to `engineOut[0]`. [engineOut] holds
   * exactly one element.
   */
  @JvmStatic
  external fun engineCreate(
    aggregateBudgetBytes: Long,
    contentCount: Long,
    jobCount: Long,
    contentLengthBytes: Long,
    inputPortionBytes: Long,
    imageAxisPixels: Long,
    imageAreaPixels: Long,
    metadataBytes: Long,
    metadataExpandedBytes: Long,
    metadataPayloadBytes: Long,
    heldChunkBytes: Long,
    iccTagCount: Long,
    exifEntryCount: Long,
    outputBytes: Long,
    outputLeaseBytes: Long,
    outputLeaseCount: Long,
    engineOut: LongArray,
  ): Int

  /**
   * Advances the shutdown of [engine] without blocking. Returns
   * [STATUS_PENDING] while a job or a lease is unreleased, then [STATUS_OK],
   * after which [engine] is invalid.
   */
  @JvmStatic external fun engineShutdown(engine: Long): Int

  /**
   * Writes the counters of [engine] to [countersOut], which holds exactly
   * [ENGINE_COUNTERS_LENGTH] elements, and resets both peaks after the read
   * when [resetPeak] holds. Works while the engine shuts down.
   */
  @JvmStatic
  external fun engineCounters(
    engine: Long,
    resetPeak: Boolean,
    countersOut: LongArray,
  ): Int

  /**
   * Opens a content of [engine] for the bytes of one representation and writes
   * its handle to `contentOut[0]`. The content admits one job.
   */
  @JvmStatic external fun contentOpen(engine: Long, contentOut: LongArray): Int

  /**
   * Supplies `bytes[offset until offset + length]` as the representation's
   * bytes from [position], which equals the bytes received before. Feed only
   * after opening or after a step returned [STATUS_INPUT_NEEDED]. [endOfInput]
   * reports the producer's successful completion after these bytes; a producer
   * failure is a [jobCancel] or a [contentClose], never a feed.
   * [STATUS_ALLOCATION_FAILED] means that the platform could not provide the
   * array's elements, and nothing changed. It takes precedence over every check
   * of [content].
   */
  @JvmStatic
  external fun contentFeed(
    content: Long,
    position: Long,
    bytes: ByteArray,
    offset: Int,
    length: Int,
    endOfInput: Boolean,
  ): Int

  /**
   * Closes [content]. A live job ends as canceled and is quiescent when the
   * call returns; its handle stays owned until [jobRelease].
   */
  @JvmStatic external fun contentClose(content: Long): Int

  /**
   * Admits the one job of [content] for a region in image coordinates: the
   * half-open rectangle from [left] and [top] to [right] and [bottom], or four
   * zeros for the whole image. Any other region lies inside the image. The
   * whole image scales uniformly to at most [maxOutputWidth] columns and never
   * up; 0 requests a scale of 1:1. Writes the job's handle to `jobOut[0]`.
   */
  @JvmStatic
  external fun jobRequest(
    content: Long,
    left: Int,
    top: Int,
    right: Int,
    bottom: Int,
    maxOutputWidth: Int,
    jobOut: LongArray,
  ): Int

  /**
   * Advances [job] and writes its report to [reportOut], which holds exactly
   * [JOB_REPORT_LENGTH] elements. A rejecting status writes no element.
   */
  @JvmStatic external fun jobStep(job: Long, reportOut: LongArray): Int

  /**
   * Requests the cancellation of [job]. Safe from any thread while the caller
   * owns the job: every call returns before [jobRelease] of that job.
   */
  @JvmStatic external fun jobCancel(job: Long): Int

  /** Releases [job] after its terminal outcome; its handle is then invalid. */
  @JvmStatic external fun jobRelease(job: Long): Int

  /**
   * Copies the covered rows `[firstRow, firstRow + rowCount)` of [lease] into
   * [destination] from [destinationFirstRow] on. Only the output adapter calls
   * it, on its worker, with a mutable `ARGB_8888` sRGB [destination] that it
   * created and that no recorded frame references.
   */
  @JvmStatic
  external fun outputCopy(
    lease: Long,
    firstRow: Int,
    rowCount: Int,
    destination: Bitmap,
    destinationFirstRow: Int,
  ): Int

  /** Releases the caller's one claim on [lease]. */
  @JvmStatic external fun outputRelease(lease: Long): Int
}
