# morpho

<img src="./morpho.png" width="300" height="300">

Morpho is a free, offline, local-first file converter for the desktop. It converts images, audio,
video, documents, and archives without uploading files, creating accounts, or sending telemetry.

**Status:** pre-alpha (M0). The Qt window, build skeleton, and CI are in place. Conversion
backends are not implemented yet.

## Build

Requirements: Linux x86_64, CMake 3.28+, Ninja, Qt 6.4+ development packages, and GCC 13+ or
Clang 18+.

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
./build/dev/src/app/morpho
```

The GUI can be smoke-tested without a display:

```bash
QT_QPA_PLATFORM=offscreen ./build/dev/src/app/morpho --smoke-test
```

## License

MIT. See [LICENSE](LICENSE).
