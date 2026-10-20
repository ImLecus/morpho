add_library(
  morpho_media_backend
  STATIC
    media_converter.cpp
    media_converter.hpp
)

target_include_directories(morpho_media_backend PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(
  morpho_media_backend
  PUBLIC morpho_core
  PRIVATE
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    PkgConfig::SWRESAMPLE
    morpho_project_warnings
)
