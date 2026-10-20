cmake_minimum_required(VERSION 3.28)

project(
  Morpho
  VERSION 0.1.0
  DESCRIPTION "Offline, local-first file converter"
  LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Morpho does not use C++20 modules. Disable dependency scanning so Clang builds
# do not require clang-scan-deps.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(GNUInstallDirs)
include(CTest)

option(MORPHO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(morpho_project_warnings INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(
    morpho_project_warnings
    INTERFACE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wconversion
      -Wsign-conversion
      -Wformat=2
      -Wundef
      -Wcast-qual
      $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
      $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
      $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
  )

  if(MORPHO_WARNINGS_AS_ERRORS)
    target_compile_options(morpho_project_warnings INTERFACE -Werror)
  endif()
endif()

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(GIF REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBP REQUIRED IMPORTED_TARGET libwebp)
pkg_check_modules(AVIF REQUIRED IMPORTED_TARGET libavif)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
