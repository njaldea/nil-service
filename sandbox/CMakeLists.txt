project(sandbox)

find_package(nil-clix CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE nil::clix)

target_link_libraries(${PROJECT_NAME} PRIVATE service)

add_executable(${PROJECT_NAME}-pipe pipe_stdout.cpp)
target_link_libraries(${PROJECT_NAME}-pipe PRIVATE service)

if(ENABLE_C_API)
    find_package(nil-clix CONFIG REQUIRED c-api)

    add_executable(${PROJECT_NAME}-c main.c)
    target_link_libraries(${PROJECT_NAME}-c PRIVATE service-c-api)
    target_link_libraries(${PROJECT_NAME}-c PRIVATE nil::clix-c-api)
    target_link_libraries(${PROJECT_NAME}-c PRIVATE pthread)
endif()

if(ENABLE_TEST)
    add_test(
        NAME sandbox_integration_test
        COMMAND bash ${CMAKE_SOURCE_DIR}/sandbox/test_sandbox.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(sandbox_integration_test PROPERTIES
        ENVIRONMENT "SANDBOX=$<TARGET_FILE:sandbox>"
    )
endif()