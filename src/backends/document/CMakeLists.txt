add_library(
  morpho_document_backend
  STATIC
    document_converter.cpp
    document_converter.hpp
)

target_include_directories(morpho_document_backend PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(
  morpho_document_backend
  PUBLIC morpho_core
  PRIVATE Qt6::Gui ${CMAKE_DL_LIBS} morpho_project_warnings
)

if(LIBREOFFICEKIT_INCLUDE_DIR)
  target_include_directories(morpho_document_backend PRIVATE "${LIBREOFFICEKIT_INCLUDE_DIR}")
  target_compile_definitions(
    morpho_document_backend
    PRIVATE
      MORPHO_HAS_LIBREOFFICEKIT=1
      MORPHO_LIBREOFFICE_PROGRAM_PATH="${MORPHO_LIBREOFFICE_PROGRAM_PATH}"
  )
endif()
