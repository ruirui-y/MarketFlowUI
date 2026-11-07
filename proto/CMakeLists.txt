# 推送协议。客户端和服务端共用这一份定义。
set(FEED_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/market_feed.proto)
set(FEED_PB_CC ${CMAKE_CURRENT_BINARY_DIR}/market_feed.pb.cc)
set(FEED_PB_H ${CMAKE_CURRENT_BINARY_DIR}/market_feed.pb.h)

add_custom_command(
    OUTPUT ${FEED_PB_CC} ${FEED_PB_H}
    COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} ${FEED_PROTO}
    DEPENDS ${FEED_PROTO}
    COMMENT "protoc 生成 market_feed.pb.cc"
)

add_library(market_feed_proto STATIC
    ${FEED_PB_CC}
    ${FEED_PB_H}
)
target_include_directories(market_feed_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(market_feed_proto PUBLIC protobuf::libprotobuf marketflowui_build_options)

# protoc 生成的代码里有「未引用形参」的告警，不是我们写的。
if(MSVC)
    target_compile_options(market_feed_proto PRIVATE /wd4100)
endif()