cmake_minimum_required(VERSION 3.27.1)

project(nil-service CXX)

include(cmake/package.cmake)
include(cmake/toolchain.cmake)
include(cmake/quality.cmake)
include(cmake/test.cmake)
include(cmake/coverage.cmake)

add_subdirectory(src)

set(ENABLE_SANDBOX  OFF CACHE BOOL "[0 | OFF - 1 | ON]: build sandbox?")
if(ENABLE_SANDBOX)
    add_subdirectory(sandbox)
endif()