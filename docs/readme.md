# hbinds

Native ABI bindings between [Hakusan](https://github.com/arexyl/hakusan), the Android application, and [San](https://github.com/arexyl/san), its native image engine.

This repository holds the C header that San implements and the Kotlin declarations that Hakusan calls. Hakusan owns the ABI and develops it here. San pins a version and implements it.

## Usage

Both projects check out this repository as a Git submodule at `share/bindings/` and track the `nightly` branch:

```sh
git submodule update --init share/bindings
```

The submodule pointer selects the ABI version that a project builds against. Move to a new version by updating that pointer together with the code that depends on it.

## License

MIT. See [license](../license).
