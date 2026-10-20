add_executable(morpho_image_conversion_test image_conversion_test.cpp)
target_link_libraries(
  morpho_image_conversion_test
  PRIVATE morpho_backends morpho_project_warnings
)

add_test(
  NAME image.roundtrip
  COMMAND $<TARGET_FILE:morpho_image_conversion_test> "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.jpg"
)
set_tests_properties(image.roundtrip PROPERTIES TIMEOUT 60)

add_executable(morpho_media_conversion_test media_conversion_test.cpp)
target_link_libraries(
  morpho_media_conversion_test
  PRIVATE morpho_backends morpho_project_warnings
)

add_test(
  NAME media.roundtrip
  COMMAND $<TARGET_FILE:morpho_media_conversion_test> "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.mp4"
)
set_tests_properties(media.roundtrip PROPERTIES TIMEOUT 120)

add_executable(
  morpho_ui_drop_test
  ui_drop_test.cpp
  "${PROJECT_SOURCE_DIR}/assets/morpho.qrc"
)
target_link_libraries(
  morpho_ui_drop_test
  PRIVATE morpho_ui morpho_backends morpho_project_warnings
)

add_test(
  NAME ui.drop_files
  COMMAND $<TARGET_FILE:morpho_ui_drop_test> "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.jpg" "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/tiny.mp4"
)
set_tests_properties(
  ui.drop_files
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
