include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

configure_package_config_file(
    cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
    NO_SET_AND_CHECK_MACRO
)
    
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake
    VERSION 0.0.1
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake"
    DESTINATION "share/${CMAKE_PROJECT_NAME}"
)

function(install_for_package TARGET)
    install(DIRECTORY publish/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(TARGETS ${TARGET} EXPORT ${CMAKE_PROJECT_NAME}-targets)
    install(
        EXPORT ${CMAKE_PROJECT_NAME}-targets
        NAMESPACE nil::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}
    )
endfunction()