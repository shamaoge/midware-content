
add_custom_command(
    OUTPUT "api_interface.cpp"
    COMMAND slice2cpp --underscore ${CMAKE_SOURCE_DIR}/src/api_interface.ice
    DEPENDS ${CMAKE_SOURCE_DIR}/src/api_interface.ice
)

include_directories(
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/common/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/third-party/
    ${CMAKE_SOURCE_DIR}/third-party/rapidjson
    ${CMAKE_SOURCE_DIR}/third-party/evhtp/include
    #${ICE_HOME}/include
    /home/apps/cpplibs/ice-3.6.3/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/common/
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/third-party/evhtp/lib
    #${ICE_HOME}/lib64/c++11
    /home/apps/cpplibs/ice-3.6.3/lib64/c++11
    /home/apps/cpplibs/glibc-2.14/lib
)

set(SERVER_SRC
    main.cpp
    conf.cpp
    http_handler.cpp
    api_interface.cpp
)

ADD_EXECUTABLE(midware-contents-service ${SERVER_SRC})
target_link_libraries(midware-contents-service
    pthread
    hiredis
    cryptopp crypto ssl
    evhtp event event_openssl event_core event_extra event_pthreads event_extra
    Ice IceUtil
)

