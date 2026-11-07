import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "theme"
import "components"

// 行情终端主窗口：上面一条工具栏，中间一张 K 线图。
ApplicationWindow
{
    id: window

    visible: true
    width: 1280
    height: 760
    minimumWidth: 900
    minimumHeight: 560
    color: Theme.bg
    title: qsTr("MarketFlow 行情终端")

    // header 是 ApplicationWindow 自带的槽位，放进去的东西自动贴在窗口最上面。
    header: Rectangle
    {
        height: Theme.barHeight
        color: Theme.panel

        Rectangle
        {
            anchors.bottom: parent.bottom                                                       // 把这条边钉到父控件的底边
            width: parent.width
            height: 1
            color: Theme.border                                                                 // 边框线的颜色
        }

        // RowLayout 把子控件横着排，间距由 spacing 统一管。
        RowLayout
        {
            anchors.fill: parent                                                                // 四条边都贴住父控件，等于铺满；anchors 就是干这个的
            anchors.leftMargin: Theme.gap                                                       // 左右各留 12 像素，不贴着窗口边
            anchors.rightMargin: Theme.gap
            spacing: Theme.gap

            Text
            {
                text: qsTr("MarketFlow")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontLarge
                font.bold: true
            }

            Rectangle { width: 1; height: 20; color: Theme.border }                             // 一条竖线当分隔符

            // TextField 就是单行输入框。
            TextField
            {
                id: host_field
                text: "127.0.0.1"
                Layout.preferredWidth: 180                                                      // 在 RowLayout 里希望占多宽
                placeholderText: qsTr("服务端地址")                                              // 没输入内容时显示的灰字
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSmall

                // 输入框的外观自己画，不用系统默认样式。
                background: Rectangle
                {
                    color: Theme.bg
                    radius: Theme.radius
                    border.color: host_field.activeFocus ? Theme.accent : Theme.border
                }
            }

            TextField
            {
                id: port_field
                text: "9100"
                Layout.preferredWidth: 90
                placeholderText: qsTr("端口")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSmall

                background: Rectangle
                {
                    color: Theme.bg
                    radius: Theme.radius
                    border.color: port_field.activeFocus ? Theme.accent : Theme.border
                }
            }

            // 连接按钮。已经连上时禁用，防止重复点。
            Button
            {
                text: qsTr("连接")
                enabled: feedClient.Status() !== "已连接"

                // 点一下就去连服务端，端口从输入框的字符串转成数字。
                onClicked: feedClient.Connect(host_field.text, parseInt(port_field.text))

                // 按钮里的文字也自己画，这样才能跟着主题配色走。
                contentItem: Text
                {
                    text: parent.text
                    color: parent.enabled ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle
                {
                    color: parent.hovered ? Theme.panelHover : Theme.bg                         // 鼠标悬停时提亮
                    radius: Theme.radius
                    border.color: Theme.border
                }
            }

            Rectangle { width: 1; height: 20; color: Theme.border }

            // 连接状态：一个圆点加一句话。
            RowLayout
            {
                spacing: 6

                Rectangle
                {
                    width: 8
                    height: 8
                    radius: 4
                    color: feedClient.Status() === "已连接" ? Theme.down : Theme.textSecondary
                }

                Text
                {
                    text: feedClient.Status()
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSmall
                }
            }

            // 空控件把左边撑开，右边的内容就被挤到最右边。
            Item { Layout.fillWidth: true }

            Text
            {
                // count 是 C++ 那边声明的 Q_PROPERTY，条数一变这里自动刷新。
                text: chartModel.count + qsTr(" 根 K 线")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSmall
            }
        }
    }

    // 外面套一层圆角矩形当卡片，K 线图铺在它的内边距里。
    Rectangle
    {
        anchors.fill: parent
        anchors.margins: Theme.gap
        color: Theme.panel
        radius: Theme.radius
        border.color: Theme.border

        KLineChart
        {
            anchors.fill: parent
            anchors.margins: Theme.gap
        }
    }
}