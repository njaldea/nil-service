project(sandbox)

find_package(nil-clix CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE nil::clix)


if(ENABLE_SSL)
    target_link_libraries(${PROJECT_NAME} PRIVATE service_ssl)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE service)
endif()