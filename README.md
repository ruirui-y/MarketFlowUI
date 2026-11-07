# MarketFlowUI

MarketFlow 的 Qt 行情终端：连服务端订阅推送，绘制分时与 K 线。

Qt 5.14.2 + C++20 + QML（不引第三方 UI 库，主题和图表都是自己写的）。

---

## 当前进度

| 部分 | 状态 |
|---|---|
| 协议定义（protobuf） | 完成 |
| 连接 + 拆帧 + 解析 | 完成 |
| 数据模型（给 QML 用） | 完成 |
| 深色主题 + K 线自绘 | 完成 |
| 服务端推送 | 在 `MarketFlow` 仓库里做 |

---

## 协议

线上格式固定：

```text
+-------------------+-------------------------+
| 长度 4 字节 大端   | protobuf 载荷 N 字节     |
+-------------------+-------------------------+
```

长度字段表示**载荷**有多少字节，不包含它自己那 4 字节。

- 客户端 → 服务端：`SubscribeRequest{ repeated symbol_ids, from_ts_ns }`
- 服务端 → 客户端：`FeedFrame{ type = BARS, repeated Bar bars }`

价格单位是**分**（`int64`），界面显示时除以 100。

---

## 构建

Linux：

```bash
cmake --preset linux-debug
cmake --build --preset build-linux-debug
```

Windows：用 Visual Studio 打开本目录，在 CMake 设置里指定 `CMAKE_PREFIX_PATH` 指向 Qt 5.14.2。

---

## 先看界面

服务端还没写的时候，用假数据预览：

```bash
./build/linux-debug/bin/marketflow_ui --demo
```

`--demo` 只造 240 根假 K 线给界面用，正式链路是连服务端收推送。

---

## 目录

```text
MarketFlowUI/
├── proto/market_feed.proto   推送协议
├── src/
│   ├── bar_data.h            一根 K 线的内存结构
│   ├── chart_model.*         给 QML 的列表模型
│   ├── feed_client.*         连接、拆帧、解析
│   └── main.cpp              入口
├── qml/
│   ├── main.qml              主窗口
│   ├── theme/Theme.qml       全部配色和尺寸
│   └── components/KLineChart.qml   蜡烛图 + 成交量
└── resources.qrc
```