qt_add_executable(morpho main.cpp)

target_link_libraries(morpho PRIVATE morpho_ui morpho_project_warnings)
target_compile_definitions(morpho PRIVATE MORPHO_VERSION="${PROJECT_VERSION}")

install(TARGETS morpho RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
