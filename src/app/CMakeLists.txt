qt_add_executable(
  morpho
    main.cpp
    "${PROJECT_SOURCE_DIR}/assets/morpho.qrc"
)

target_link_libraries(morpho PRIVATE morpho_ui morpho_backends morpho_project_warnings)
target_compile_definitions(morpho PRIVATE MORPHO_VERSION="${PROJECT_VERSION}")

install(TARGETS morpho RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

foreach(icon_size IN ITEMS 16 24 32 48 64 128 256)
  install(
    FILES "${PROJECT_SOURCE_DIR}/assets/icons/${icon_size}/morpho.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}/apps"
    RENAME "morpho.png"
  )
endforeach()

install(
  FILES "${PROJECT_SOURCE_DIR}/morpho.png"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps"
  RENAME "morpho.png"
)
install(
  FILES "${PROJECT_SOURCE_DIR}/packaging/linux/morpho.desktop"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)
