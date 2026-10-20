# morpho

<img src="./morpho.png" width="300" height="300">

Morpho is a free, offline, local-first file converter for the desktop. It converts images, audio,
video, documents, and archives without uploading files, creating accounts, or sending telemetry.

**Status:** pre-alpha (M0 plus the first conversion). The Qt window, build skeleton, and CI are
in place. Drag JPG/JPEG files into the window, or use **File > Open JPG files**. Converted PNGs
stay in memory and appear in the right sidebar as an entry such as `foto (JPG → PNG)`, with the
file size and a **Download** button. This is the only conversion implemented so far.

## Build

Requirements: Linux x86_64, CMake 3.28+, Ninja, Qt 6.4+ development packages,
libjpeg-turbo and libpng development packages, and GCC 13+ or Clang 18+.

On Ubuntu 24.04:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools \
  libjpeg-turbo8-dev libpng-dev
```

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
