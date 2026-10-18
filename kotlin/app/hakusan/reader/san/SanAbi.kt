package app.hakusan.reader.san

/**
 * The JNI surface of San's native ABI, version [ABI_VERSION].
 *
 * The C header `include/san.h` is the authority. Each constant mirrors the
 * header constant whose name adds the `SAN_` prefix. Each external function
 * maps one header function and uses JNI primitives only; an output value
 * travels in a one-element array. San's `JNI_OnLoad` registers the functions.
 * Nothing here loads the library.
 *
 * The object is internal to `:reader`, and its members are public, so the
 * compiler mangles no JVM name. The cross-check table `abi.tsv` lists every
 * constant and every JNI name and descriptor.
 *
 * Call [bindingEstablish] before any other function. The caller serializes
 * every call on one engine. No function blocks, calls back, or throws for an
 * engine outcome; each returns a status.
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

  /** The handle is zero, stale, foreign, or of the wrong kind. */
  const val STATUS_INVALID_HANDLE: Int = -8

  /** The target is closing or closed and admits no new work. */
  const val STATUS_CLOSED: Int = -9

  /** The library and the binding are incompatible. */
  const val STATUS_BINDING_MISMATCH: Int = -10

  /** The library is unavailable. Only the Kotlin loader reports it. */
  const val STATUS_LOAD_FAILURE: Int = -11

  /** An unexpected recoverable failure ended the operation. */
  const val STATUS_ENGINE_FAILURE: Int = -12

  /** No handle. San never issues it. */
  const val NO_HANDLE: Long = 0L

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
   * Creates an engine whose aggregate budget is [aggregateBudgetBytes]. Writes
   * the new handle, or [NO_HANDLE] on failure, to `engineOut[0]` and returns a
   * status. [engineOut] holds exactly one element. Any other length returns
   * [STATUS_BINDING_MISMATCH] before any engine exists and leaves the array
   * unchanged.
   */
  @JvmStatic
  external fun engineCreate(
    aggregateBudgetBytes: Long,
    engineOut: LongArray,
  ): Int

  /**
   * Advances the shutdown of [engine] without blocking. Returns
   * [STATUS_PENDING] while a holder still has to release a resource, then
   * [STATUS_OK], after which [engine] is invalid. Returns
   * [STATUS_INVALID_HANDLE] for an invalid engine or one already shut down.
   */
  @JvmStatic external fun engineShutdown(engine: Long): Int
}
