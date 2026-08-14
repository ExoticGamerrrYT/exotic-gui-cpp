# Install rules and the CMake package config, so downstream projects can do
#
#   find_package(ExoticGui 0.1 REQUIRED)
#   target_link_libraries(app PRIVATE exotic::gui)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(EXOTIC_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ExoticGui")

install(TARGETS exotic_gui
    EXPORT ExoticGuiTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILE_SET generated_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT ExoticGuiTargets
    FILE ExoticGuiTargets.cmake
    NAMESPACE exotic::
    DESTINATION "${EXOTIC_CMAKE_DIR}")

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/ExoticGuiConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/ExoticGuiConfig.cmake"
    INSTALL_DESTINATION "${EXOTIC_CMAKE_DIR}")

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/ExoticGuiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${PROJECT_BINARY_DIR}/ExoticGuiConfig.cmake"
    "${PROJECT_BINARY_DIR}/ExoticGuiConfigVersion.cmake"
    DESTINATION "${EXOTIC_CMAKE_DIR}")

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/README.md"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/exotic-gui")
