project(service)

find_package(Boost REQUIRED COMPONENTS asio beast)
find_package(nil-xalt REQUIRED)

set(
    HEADERS
        publish/nil/service.hpp
        publish/nil/service/codec.hpp
        publish/nil/service/concat.hpp
        publish/nil/service/consume.hpp
        publish/nil/service/ID.hpp
        publish/nil/service/map.hpp
        publish/nil/service/detail/create_handler.hpp
        publish/nil/service/detail/create_message_handler.hpp
        publish/nil/service/structs.hpp
        publish/nil/service/self/create.hpp
        publish/nil/service/udp/server/create.hpp
        publish/nil/service/udp/client/create.hpp
        publish/nil/service/tcp/server/create.hpp
        publish/nil/service/tcp/client/create.hpp
        publish/nil/service/ws/server/create.hpp
        publish/nil/service/ws/client/create.hpp
)

set(
    SOURCES
        src/codec.cpp
        src/utils.hpp
        src/structs/WebTransaction.cpp
        src/structs/WebTransaction.hpp
        src/self/create.cpp
        src/udp/client/create.cpp
        src/udp/server/create.cpp
        src/tcp/client/create.cpp
        src/tcp/server/create.cpp
        src/tcp/Connection.cpp
        src/tcp/Connection.hpp
        src/ws/client/create.cpp
        src/ws/server/create.cpp
        src/ws/Connection.cpp
        src/ws/Connection.hpp
        src/http/server/create.cpp
        src/http/server/WebSocket.cpp
        src/http/server/WebSocket.hpp
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    nil::xalt
)

target_include_directories(
    ${PROJECT_NAME}
    SYSTEM PRIVATE
    $<TARGET_PROPERTY:Boost::asio,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Boost::beast,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_NO_DEPRECATED)

add_test_subdirectory()

nil_install_headers(${PROJECT_NAME} PUBLIC)
nil_install_targets(${PROJECT_NAME})

set(ENABLE_SSL  OFF CACHE BOOL "[0 | OFF - 1 | ON]: build with ssl?")
if(ENABLE_SSL)
    find_package(OpenSSL REQUIRED)

    add_library(
        ${PROJECT_NAME}-ssl
        publish/nil/service/wss/server/create.hpp
        publish/nil/service/wss/client/create.hpp
        src/wss/client/create.cpp
        src/wss/server/create.cpp
        src/wss/Connection.cpp
        src/wss/Connection.hpp
        src/https/server/create.cpp
        src/https/server/WebSocket.cpp
        src/https/server/WebSocket.hpp
    )

    target_link_libraries(
        ${PROJECT_NAME}-ssl PRIVATE
        OpenSSL::SSL
    )
    target_link_libraries(
        ${PROJECT_NAME}-ssl PUBLIC
        ${PROJECT_NAME}
    )
    target_compile_definitions(
        ${PROJECT_NAME}-ssl
        PUBLIC
        NIL_SERVICE_SSL
    )
    target_include_directories(
        ${PROJECT_NAME}-ssl
        SYSTEM PRIVATE
        $<TARGET_PROPERTY:Boost::asio,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:Boost::beast,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(${PROJECT_NAME}-ssl PRIVATE BOOST_ASIO_NO_DEPRECATED)
    
    nil_install_targets(${PROJECT_NAME}-ssl)
endif()
