project(service)

find_package(Boost REQUIRED)

set(
    HEADERS
        publish/nil/service.hpp
        publish/nil/service/codec.hpp
        publish/nil/service/concat.hpp
        publish/nil/service/consume.hpp
        publish/nil/service/ID.hpp
        publish/nil/service/map.hpp
        publish/nil/service/detail/Callable.hpp
        publish/nil/service/detail/create_handler.hpp
        publish/nil/service/detail/create_message_handler.hpp
        publish/nil/service/detail/errors.hpp
        publish/nil/service/detail/Handlers.hpp
        publish/nil/service/structs.hpp
        publish/nil/service/self/create.hpp
        publish/nil/service/udp/server/create.hpp
        publish/nil/service/udp/client/create.hpp
        publish/nil/service/tcp/server/create.hpp
        publish/nil/service/tcp/client/create.hpp
)

set(
    SOURCES
        src/codec.cpp
        src/utils.hpp
        src/structs.cpp
        src/structs/HTTPService.hpp
        src/structs/MessagingService.hpp
        src/structs/ObservableService.hpp
        src/structs/RunnableService.hpp
        src/structs/Service.hpp
        src/structs/StandaloneService.hpp
        src/proxies/S.cpp
        src/proxies/A.cpp
        src/proxies/H.cpp
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
        src/http/server/Route.hpp
        src/http/server/WebSocket.cpp
        src/http/server/WebSocket.hpp
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(
    ${PROJECT_NAME}
    SYSTEM PRIVATE
    $<TARGET_PROPERTY:Boost::boost,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_NO_DEPRECATED)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/publish>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_test_subdirectory()

nil_install_headers()
nil_install_targets(${PROJECT_NAME})