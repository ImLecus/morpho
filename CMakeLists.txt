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
      -Wold-style-cast
      -Wnon-virtual-dtor
      -Woverloaded-virtual
      -Wformat=2
      -Wundef
      -Wcast-qual
  )

  if(MORPHO_WARNINGS_AS_ERRORS)
    target_compile_options(morpho_project_warnings INTERFACE -Werror)
  endif()
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
