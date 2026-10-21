# morpho

<img src="./morpho.png" width="300" height="300">

Morpho is a free, offline, local-first file converter for the desktop. It converts images, audio,
video, documents, and archives without uploading files, creating accounts, or sending telemetry.

**Status:** pre-alpha. The Qt window, build skeleton, and CI are in place. Drag image, audio,
video, or document files into the window, choose an output format, and the results stay in
memory in the right sidebar as entries such as `foto (JPEG → PNG)`, with the old and new file
sizes and a **Download** button.

Supported outputs: PNG, JPEG, WEBP, AVIF, TIFF, BMP, GIF, MP3, M4A, OPUS, OGG, FLAC, WAV, MP4,
WEBM, MKV, MOV, PDF, DOCX, ODT. Input formats are detected automatically.

Markdown converts to PDF natively. DOCX, DOC, ODT, RTF, and TXT convert to PDF, DOCX, or ODT
through LibreOfficeKit when it is available; PDF input is not supported yet.

## Build

Requirements: Linux x86_64, CMake 3.28+, Ninja, Qt 6.4+ development packages, libjpeg-turbo,
libpng, libwebp, libavif, libtiff, giflib, and FFmpeg development libraries, plus GCC 13+ or
Clang 18+. Office document conversion additionally needs `libreofficekit-dev` at build time and
a LibreOffice Writer runtime.

On Ubuntu 24.04:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools \
  libjpeg-turbo8-dev libpng-dev libwebp-dev libavif-dev libtiff-dev libgif-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev \
  libreofficekit-dev libreoffice-writer
```

The distribution build must use an LGPL FFmpeg configuration (`--disable-gpl
--disable-nonfree --disable-network`). The Ubuntu development packages are used for development
and testing only. MP4 and MOV currently use MPEG-4 Part 2 + AAC; WEBM and MKV use VP9 + Opus.

LibreOfficeKit is optional. It is loaded in-process from the system LibreOffice installation;
Morpho never starts a `soffice` process, bundles LibreOffice, or uses the network. LibreOfficeKit
is MPL-2.0. Without its headers, the build still works and Markdown-to-PDF remains available, but
office inputs are rejected. On distributions that install LibreOffice elsewhere, configure
`MORPHO_LIBREOFFICE_PROGRAM_PATH` to the directory containing `libmergedlo.so`:

```bash
cmake --preset dev -DMORPHO_LIBREOFFICE_PROGRAM_PATH=/usr/lib64/libreoffice/program
```

The document engine decision is recorded in [ADR 0001](docs/adr/0001-document-engine.md).

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

## Releases

Push a stable SemVer tag such as `v0.1.0` to a commit on `main`:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The `Release` workflow builds and tests the Linux application, creates an AppImage and a `.deb`,
smoke-tests the AppImage, writes `SHA256SUMS`, and publishes a GitHub Release named after the
tag.

Release assets:

- `Morpho-<tag>-linux-x86_64.AppImage`
- `Morpho-<tag>-linux-amd64.deb`
- `SHA256SUMS`, `LICENSE`, and `THIRD_PARTY_NOTICES.md`

The packages are unsigned previews built on Ubuntu 24.04 with the distribution Qt and FFmpeg
packages; the distribution FFmpeg may include GPL components. Windows and macOS releases are not
produced yet. See [ADR 0002](docs/adr/0002-linux-release-artifacts.md).

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

If the application reports that office documents are unsupported, install the LibreOfficeKit
headers and Writer runtime, then reconfigure:

```bash
sudo apt install libreofficekit-dev libreoffice-writer
cmake --preset dev
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

MIT. See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
