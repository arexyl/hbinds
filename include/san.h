/*
 * The native ABI between Hakusan and San, version 0: lifecycle.
 *
 * This header is the authority. The Kotlin declarations in kotlin/ map each
 * function to one JNI method, and abi.tsv cross-checks both sides.
 *
 * Profile. The header is freestanding C11 and includes only <stdint.h>. Its
 * scalars are fixed-width integers, and its pointers point to such integers or
 * to structs of them.
 *
 * Linkage. San's library exports only JNI_OnLoad. JNI_OnLoad registers the
 * JNI methods, and each method converts its JNI primitives and calls one
 * function below. An output value travels in a one-element primitive array,
 * which counts as a buffer. A JNI argument that violates its declared shape,
 * such as a null array or an output array whose length is not one, returns
 * SAN_STATUS_BINDING_MISMATCH before any function below runs, so the call
 * changes nothing and leaves no Java exception pending. Every function uses
 * the C calling convention of the target ABI: AAPCS64 on arm64-v8a and the
 * System V AMD64 ABI on x86_64.
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
 * one status. It never blocks, calls back, raises a Java exception, or
 * unwinds. The caller serializes every call on one engine. Version 0 has one
 * execution profile: the caller's thread and no codec threads.
 *
 * Order. Establish the binding first. No engine exists before that.
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
/* The handle is zero, stale, foreign, or of the wrong kind. */
#define SAN_STATUS_INVALID_HANDLE ( -8 )
/* The target is closing or closed and admits no new work. */
#define SAN_STATUS_CLOSED ( -9 )
/* The library and the binding are incompatible. */
#define SAN_STATUS_BINDING_MISMATCH ( -10 )
/* The library is unavailable. Only the Kotlin loader reports it. */
#define SAN_STATUS_LOAD_FAILURE ( -11 )
/* An unexpected recoverable failure ended the operation. */
#define SAN_STATUS_ENGINE_FAILURE ( -12 )

/*
 * An opaque 64-bit handle that San issues and validates. It is never a
 * pointer, and zero is never a valid handle. San rejects a zero, stale,
 * foreign, or wrong-kind handle with SAN_STATUS_INVALID_HANDLE and never
 * dereferences it first.
 */
typedef int64_t san_handle;

/* No handle. San never issues it. */
#define SAN_NO_HANDLE 0

/*
 * Establishes the binding of this process. Returns SAN_STATUS_OK when
 * abi_version equals SAN_ABI_VERSION; the binding then stays established.
 * Otherwise returns SAN_STATUS_BINDING_MISMATCH and changes nothing. Callable
 * from any thread at any time, and repeatable.
 */
san_status san_binding_establish( int32_t abi_version );

/*
 * The limits of one engine, each named and in its unit. Hakusan selects every
 * value; San has no defaults.
 */
typedef struct san_engine_limits {
  /* The aggregate budget of every native resource of the engine, in bytes. */
  int64_t aggregate_budget_bytes;
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
 * - SAN_STATUS_BUDGET_EXCEEDED when the limits cannot admit the engine's own
 *   state, including a budget of zero or less, and while another engine is
 *   live;
 * - SAN_STATUS_ALLOCATION_FAILED or SAN_STATUS_ENGINE_FAILURE.
 */
san_status san_engine_create( const san_engine_limits *limits,
                              san_handle              *engine );

/*
 * Advances the shutdown of an engine without blocking. The first call stops
 * admission; from then on until the shutdown completes, every other call on
 * the engine returns SAN_STATUS_CLOSED. Each call settles what it can, in the
 * shutdown order.
 * Returns:
 * - SAN_STATUS_PENDING while a holder still has to release a resource, such
 *   as an output lease; call again after a release, and never poll;
 * - SAN_STATUS_OK once every resource is released, which reports the engine
 *   closed; the handle is invalid from then on;
 * - SAN_STATUS_INVALID_HANDLE for an invalid handle, including an engine
 *   whose shutdown completed.
 * The caller owns waiting and any timeout. A timeout never permits a forced
 * release; the engine stays closing.
 */
san_status san_engine_shutdown( san_handle engine );

#endif /* SAN_H */
