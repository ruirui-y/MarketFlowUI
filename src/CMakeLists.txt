# 行情终端主程序。
add_executable(marketflow_ui
    main.cpp
    chart_model.cpp
    feed_client.cpp
    log.cpp
    ${PROJECT_SOURCE_DIR}/resources.qrc
)
target_include_directories(marketflow_ui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(marketflow_ui PRIVATE market_feed_proto Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick Threads::Threads)

# ---------------------------------------------------------------------------
# 运行时部署：把 protobuf 和 Qt 的 DLL 拷到 exe 旁边，双击就能跑。
#
# windeployqt 解析依赖时按 DLL 名字在 PATH 里找，本机 PATH 第一个是 Qt 5.14.2，
# 不把 5.15.2 顶到前面就会部署成 5.14 的 DLL，启动报「无法定位程序输入点」。
# ---------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MARKETFLOW_DEPLOY_MODE --debug)
    set(MARKETFLOW_PROTOBUF_DLL "D:/YJJ/vcpkg/installed/x64-windows/debug/bin/libprotobufd.dll")
    set(MARKETFLOW_ABSEIL_DIR "D:/YJJ/vcpkg/installed/x64-windows/debug/bin")
else()
    set(MARKETFLOW_DEPLOY_MODE "")
    set(MARKETFLOW_PROTOBUF_DLL "D:/YJJ/vcpkg/installed/x64-windows/bin/libprotobuf.dll")
    set(MARKETFLOW_ABSEIL_DIR "D:/YJJ/vcpkg/installed/x64-windows/bin")
endif()

add_custom_command(TARGET marketflow_ui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MARKETFLOW_PROTOBUF_DLL}"
        "${MARKETFLOW_ABSEIL_DIR}/abseil_dll.dll"
        "$<TARGET_FILE_DIR:marketflow_ui>"
    COMMAND ${CMAKE_COMMAND} -E env "PATH=${MARKETFLOW_QT_ROOT}/bin;$ENV{PATH}"
        "${MARKETFLOW_QT_ROOT}/bin/windeployqt.exe"
        --no-translations ${MARKETFLOW_DEPLOY_MODE}
        --qmldir "${PROJECT_SOURCE_DIR}/qml"
        "$<TARGET_FILE:marketflow_ui>"
    COMMENT "部署运行时 DLL"
    VERBATIM
)