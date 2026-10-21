# Third-party notices

Morpho's original code is licensed under the MIT License. Release artifacts must include the
full license texts for the components below. Development and test dependencies are listed in
`README.md` and `AGENTS.md`.

| Component | License | Use |
| --- | --- | --- |
| Qt 6 Core, Gui, Widgets | LGPL-3.0 | Dynamically linked GUI and document rendering. |
| FFmpeg libavcodec, libavformat, libavutil, libswscale, libswresample | LGPL-2.1-or-later | Dynamically linked audio and video conversion. Stable distribution builds use `--disable-gpl`, `--disable-nonfree`, and `--disable-network`; Linux preview artifacts currently use the Ubuntu package. |
| libpng | libpng-2.0 | PNG input and output. |
| libjpeg-turbo | BSD-3-Clause / IJG | JPEG input and output. |
| libwebp | BSD-3-Clause | WebP input and output. |
| libavif | BSD-2-Clause | AVIF input and output. |
| libtiff | libtiff license (BSD-like) | TIFF input and output. |
| giflib | MIT | GIF input and output. |
| LibreOfficeKit and host LibreOffice Writer | MPL-2.0 | Optional, dynamically loaded office document engine. LibreOffice is not bundled and Morpho does not start a `soffice` process. |
| Catch2 | BSL-1.0 | Test-only. |

The exact versions, notices, and full license texts for a release are collected from the pinned
build environment and shipped with that release. No third-party component is relicensed by
Morpho.

Linux preview artifacts built by the release workflow use the Ubuntu 24.04 FFmpeg package, which
may include GPL components. ADR 0002 records this temporary exception. Replace that package with
the project's LGPL FFmpeg build before publishing a stable release.
