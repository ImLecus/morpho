add_library(morpho_core INTERFACE)

target_compile_features(morpho_core INTERFACE cxx_std_20)
target_include_directories(morpho_core INTERFACE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(morpho_core INTERFACE morpho_project_warnings)
