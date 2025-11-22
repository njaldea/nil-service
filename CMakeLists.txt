cmake_minimum_required(VERSION 3.27.1)

project(nil-service VERSION 1.0.4 LANGUAGES CXX)

include(cmake/package.cmake)
include(cmake/toolchain.cmake)
include(cmake/quality.cmake)
include(cmake/test.cmake)
include(cmake/coverage.cmake)
include(cmake/cpack.cmake)

add_subdirectory(src)

set(ENABLE_SANDBOX  OFF CACHE BOOL "[0 | OFF - 1 | ON]: build sandbox?")
if(ENABLE_SANDBOX)
    add_subdirectory(sandbox)
endif()