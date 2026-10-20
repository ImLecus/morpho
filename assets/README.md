# Assets

- `../morpho.png` — Morpho project logo, 512x512. Used in the README and for the 512x512
  desktop icon. License: MIT, copyright Morpho contributors.
- `icons/<size>/morpho.png` — Downscaled icons generated from `../morpho.png` with ImageMagick
  and stripped of metadata. Used by the Qt resource and the hicolor icon theme.
- `icons/chevron-down.png` — White dropdown chevron used by the format selectors.
- `morpho.qrc` — Qt resource manifest that embeds the icon set and UI glyphs in the application
  binary.

Application icons use PNG. WebP is not used for the app icon because desktop icon themes and
minimal Qt installations do not universally provide a WebP image plugin, and Windows/macOS
require ICO/ICNS formats for native executable icons.

The window icon is a multi-size set rather than the 512x512 source. X11 stores window icons in
`_NET_WM_ICON`, and a single 512x512 image can exceed the maximum XCB request size; Qt then
ignores the icon. Sizes 16 through 256 keep the request bounded while remaining crisp on HiDPI
displays.

