project(sandbox)

find_package(nil-clix CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE nil::clix)

target_link_libraries(${PROJECT_NAME} PRIVATE service)

if(ENABLE_C_API)
    add_executable(${PROJECT_NAME}-c main.c)
    target_link_libraries(${PROJECT_NAME}-c PRIVATE service-c-api)
    target_link_libraries(${PROJECT_NAME}-c PRIVATE pthread)
endif()