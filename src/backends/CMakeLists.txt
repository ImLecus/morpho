add_subdirectory(image)
add_subdirectory(media)

add_library(
  morpho_converter
  STATIC
    morpho_converter.cpp
    morpho_converter.hpp
)
target_include_directories(morpho_converter PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(
  morpho_converter
  PUBLIC morpho_image_backend morpho_media_backend
  PRIVATE morpho_project_warnings
)

add_library(morpho_backends INTERFACE)
target_link_libraries(morpho_backends INTERFACE morpho_converter)
