add_library(
  morpho_ui
  STATIC
    main_window.cpp
    main_window.hpp
)

target_include_directories(morpho_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(morpho_ui PUBLIC Qt6::Widgets PRIVATE morpho_project_warnings)
