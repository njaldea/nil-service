cmake_minimum_required(VERSION 3.27.1)

project(
    nil-service
    VERSION 1.0.7
    LANGUAGES CXX
    DESCRIPTION "boost net library wrapper"
)

find_package(nil-xalt CONFIG REQUIRED)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "nil-xalt-dev (>= ${nil-xalt_VERSION})")

find_package(nil-cmake CONFIG REQUIRED)
install_config("cmake/config.cmake.in")

add_subdirectory(src)      

set(ENABLE_SANDBOX  OFF CACHE BOOL "[0 | OFF - 1 | ON]: build sandbox?")
if(ENABLE_SANDBOX)
    add_subdirectory(sandbox)
endif()