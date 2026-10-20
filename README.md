# morpho

<img src="./morpho.png" width="300" height="300">

Morpho is a free, offline, local-first file converter for the desktop. It converts images, audio,
video, documents, and archives without uploading files, creating accounts, or sending telemetry.

**Status:** pre-alpha. The Qt window, build skeleton, and CI are in place. Drag PNG, JPEG,
WEBP, AVIF, TIFF, BMP, or GIF files into the window, choose an output format, and the converted
images stay in memory in the right sidebar as entries such as `foto (JPEG → PNG)`, with the old
and new file sizes and a **Download** button.

## Build

Requirements: Linux x86_64, CMake 3.28+, Ninja, Qt 6.4+ development packages, libjpeg-turbo,
libpng, libwebp, libavif, libtiff, and giflib development packages, and GCC 13+ or Clang 18+.

On Ubuntu 24.04:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools \
  libjpeg-turbo8-dev libpng-dev libwebp-dev libavif-dev libtiff-dev libgif-dev
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

### Troubleshooting

If CMake reports that `Qt6Config.cmake` was not found, install the Qt 6 development packages:

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools
```

If you use a Qt installation outside the system paths, pass its prefix explicitly:

```bash
cmake --preset dev -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

If CMake reports that `libgif` was not found:

```bash
sudo apt install libgif-dev
```

If the application reports a missing `libgav1.so.1` or `libyuv.so.0`, the AVIF runtime
dependencies are not installed:

```bash
sudo apt install libavif-dev
# or, without the development package:
sudo apt install libavif16 libgav1-1 libyuv0
```

Then reconfigure and rebuild. Do not rely on temporary local library prefixes for a normal
development build; they are only used in isolated test environments.

## License

MIT. See [LICENSE](LICENSE).
