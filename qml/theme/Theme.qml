pragma Singleton

import QtQuick 2.15

// 全项目唯一的配色和尺寸来源。改这里，所有界面跟着变。
QtObject
{
    readonly property color bg: "#0d1117"                                                       // 窗口底色
    readonly property color panel: "#161b22"                                                    // 面板、工具栏
    readonly property color panelHover: "#1f2630"
    readonly property color border: "#30363d"
    readonly property color grid: "#21262d"                                                     // 图表网格线

    readonly property color textPrimary: "#e6edf3"
    readonly property color textSecondary: "#8b949e"

    readonly property color up: "#f85149"                                                       // 涨：红
    readonly property color down: "#3fb950"                                                     // 跌：绿
    readonly property color accent: "#58a6ff"
    readonly property color warn: "#d29922"

    readonly property int radius: 6
    readonly property int gap: 12
    readonly property int barHeight: 44

    readonly property string fontUi: "Microsoft YaHei UI"                                       // 界面文字
    readonly property string fontNumber: "Consolas"                                             // 价格和数字用等宽，列才对得齐

    readonly property int fontTiny: 11
    readonly property int fontSmall: 12
    readonly property int fontNormal: 14
    readonly property int fontLarge: 18
    readonly property int fontHuge: 22
}