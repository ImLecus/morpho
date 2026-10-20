add_executable(morpho_jpeg_to_png_test jpeg_to_png_test.cpp)
target_link_libraries(
  morpho_jpeg_to_png_test
  PRIVATE morpho_backends morpho_project_warnings
)

add_test(
  NAME jpeg_to_png.converts
  COMMAND $<TARGET_FILE:morpho_jpeg_to_png_test> "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.jpg"
)
set_tests_properties(jpeg_to_png.converts PROPERTIES TIMEOUT 30)

add_executable(morpho_ui_drop_test ui_drop_test.cpp)
target_link_libraries(
  morpho_ui_drop_test
  PRIVATE morpho_ui morpho_backends morpho_project_warnings
)

add_test(
  NAME ui.drop_jpeg
  COMMAND $<TARGET_FILE:morpho_ui_drop_test> "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.jpg"
)
set_tests_properties(
  ui.drop_jpeg
  PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

add_test(NAME morpho.version COMMAND $<TARGET_FILE:morpho> --version)
set_tests_properties(morpho.version PROPERTIES PASS_REGULAR_EXPRESSION "Morpho ${PROJECT_VERSION}")

add_test(NAME morpho.help COMMAND $<TARGET_FILE:morpho> --help)
set_tests_properties(morpho.help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: morpho")

add_test(NAME morpho.smoke COMMAND $<TARGET_FILE:morpho> --smoke-test)
set_tests_properties(
  morpho.smoke
  PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)
