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
