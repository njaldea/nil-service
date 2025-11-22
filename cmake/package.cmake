include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(NOT DEFINED NIL_INSTALL_GROUP)
    set(NIL_INSTALL_GROUP "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}" CACHE STRING "Install grouping directory" FORCE)
else()
    set(NIL_INSTALL_GROUP "${NIL_INSTALL_GROUP}" CACHE STRING "Install grouping directory" FORCE)
endif()

set(CMAKE_INSTALL_INCLUDEDIR "include/${NIL_INSTALL_GROUP}" CACHE PATH "" FORCE)
set(CMAKE_INSTALL_LIBDIR     "lib/${NIL_INSTALL_GROUP}"     CACHE PATH "" FORCE)
set(CMAKE_INSTALL_BINDIR     "bin/${NIL_INSTALL_GROUP}"     CACHE PATH "" FORCE)

configure_package_config_file(
    cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/${NIL_INSTALL_GROUP}
    NO_SET_AND_CHECK_MACRO
)
    
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${NIL_INSTALL_GROUP}
)

function(nil_install_targets TARGET)
    install(TARGETS ${TARGET} EXPORT nil-${TARGET}-targets)
    install(
        EXPORT nil-${TARGET}-targets
        NAMESPACE nil::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${NIL_INSTALL_GROUP}
    )
endfunction()

function(nil_install_headers TARGET TYPE)
    target_include_directories(
        ${TARGET}
        ${TYPE}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/publish>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    install(DIRECTORY publish/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endfunction()
