add_library(
  morpho_image_backend
  STATIC
    jpeg_to_png.cpp
    jpeg_to_png.hpp
)

target_include_directories(morpho_image_backend PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(
  morpho_image_backend
  PUBLIC morpho_core
  PRIVATE JPEG::JPEG PNG::PNG morpho_project_warnings
)
