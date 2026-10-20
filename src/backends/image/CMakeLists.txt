add_library(
  morpho_image_backend
  STATIC
    avif_codec.cpp
    bmp_codec.cpp
    codecs.hpp
    gif_codec.cpp
    image.cpp
    image.hpp
    image_converter.cpp
    image_converter.hpp
    jpeg_codec.cpp
    png_codec.cpp
    tiff_codec.cpp
    webp_codec.cpp
)

target_include_directories(morpho_image_backend PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(
  morpho_image_backend
  PUBLIC morpho_core
  PRIVATE
    JPEG::JPEG
    PNG::PNG
    TIFF::TIFF
    PkgConfig::WEBP
    PkgConfig::AVIF
    GIF::GIF
    morpho_project_warnings
)
