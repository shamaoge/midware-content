
include_directories(
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/common/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/third-party/
    ${CMAKE_SOURCE_DIR}/third-party/rapidjson
    ${CMAKE_SOURCE_DIR}/third-party/evhtp/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/common/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/third-party/evhtp/lib
)

set(SERVER_SRC
    main.cpp
    conf.cpp
    http_handler.cpp
)

ADD_EXECUTABLE(midware-contents-service ${SERVER_SRC})
target_link_libraries(midware-contents-service
    pthread
    hiredis
    cryptopp crypto ssl
    evhtp event event_openssl event_core event_extra event_pthreads event_extra
)

