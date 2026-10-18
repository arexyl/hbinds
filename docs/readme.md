# hbinds

Native ABI bindings between [Hakusan](https://github.com/arexyl/hakusan), the Android application, and [San](https://github.com/arexyl/san), its native image engine.

This repository holds the C header that San implements and the Kotlin declarations that Hakusan calls. Hakusan owns the ABI and develops it here. San pins a version and implements it.

## Usage

Both projects check out this repository as a Git submodule at `share/bindings/` and track the `nightly` branch:

```sh
git submodule update --init share/bindings
```

The submodule pointer selects the ABI version that a project builds against. Move to a new version by updating that pointer together with the code that depends on it.

## Layout

- `include/san.h` is the C header. It defines the ABI and is the authority.
- `kotlin/` holds the Kotlin declarations of the JNI surface, which Hakusan compiles.
- `abi.tsv` is the cross-check table that both projects test against. Its constants and layouts come from the header, and its JNI method names and descriptors come from the Kotlin declarations.

Change the header, the declarations, and the table together. `SAN_ABI_VERSION` names the ABI version, and Hakusan and San must agree on it exactly. A mismatch is reported before any engine exists.

### Table format

`abi.tsv` holds only printable ASCII (0x20 to 0x7E), tabs, and LF line endings, with a final newline. It has no blank lines, comments, or quoting, and the order of its rows carries no meaning. The first line is exactly `kind`, `name`, `abi`, and `value`, separated by tabs. Every other line is one row of four nonempty fields, separated by tabs. The seven kinds below are the only kinds.

| Kind | Name | Value |
|---|---|---|
| `i32` | A header macro `SAN_...` that the Kotlin declarations hold as an `Int` | A decimal integer in the `Int` range |
| `i64` | A header macro `SAN_...` that the Kotlin declarations hold as a `Long` | A decimal integer in the `Long` range |
| `type` | An integer typedef, or a field written `struct.field` | Its fixed-width type: one of `int8_t`, `int16_t`, `int32_t`, `int64_t`, `uint8_t`, `uint16_t`, `uint32_t`, and `uint64_t` |
| `size` | A type | Its size in bytes |
| `align` | A type | Its alignment in bytes |
| `offset` | A field written `struct.field` | Its offset in bytes |
| `jni` | A JNI method written `binary/class/Name.method` | Its JNI descriptor, such as `(J[J)I` |

- Integers are canonical decimals that match `0|-?[1-9][0-9]*`: no leading zero, no plus sign, and no `-0`.
- The `abi` column is `*`, `arm64-v8a`, or `x86_64`. `*` marks a row that holds on every ABI. A row that differs by ABI is written once for `arm64-v8a` and once for `x86_64`, and never together with a `*` row. `i32`, `i64`, and `jni` rows always use `*`.
- A `kind`, `name`, and `abi` combination appears at most once.
- An `i32` or `i64` macro is a decimal integer constant `N` in the header, or `( -N )` for a negative value. The kind names the Kotlin type and the value range. Compare a constant by its value and range, not by the width of its translated C type: `SAN_NO_HANDLE` is an `i64` row although its macro has the C type `int`.
- The table is complete in both directions:
  - every macro of the header that starts with `SAN_`, except the include guard `SAN_H`, has an `i32` or `i64` row, and every such row names one;
  - every integer typedef has `type`, `size`, and `align` rows, every struct has `size` and `align` rows, and each of its fields has `type` and `offset` rows;
  - every constant of the Kotlin declarations has an `i32` or `i64` row, and every such row names one;
  - every native method of the Kotlin declarations has a `jni` row, and every `jni` row names one.
- Function prototypes have no row kind. Each project checks them against the header directly.

## License

MIT. See [license](../license).
