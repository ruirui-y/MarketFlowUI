import QtQuick 2.15
import "../theme"

// 蜡烛图 + 成交量柱。数据从 chartModel 逐行取，每次整块重画。
Item
{
    id: chart

    readonly property real price_height: height * 0.72                                          // 上面 72% 画蜡烛
    readonly property real volume_height: height * 0.22                                         // 下面 22% 画成交量
    readonly property int axis_width: 64                                                        // 右边留 64 像素放价格刻度

    Canvas
    {
        id: canvas
        anchors.fill: parent
        antialiasing: true                                                                      // 抗锯齿，不然斜线是毛的

        // Qt 觉得这块画布该重画时会调到这里。
        onPaint:
        {
            const ctx = getContext("2d");                                                       // 拿到画布的画笔
            ctx.reset();                                                                        // 先擦掉上一帧，否则新旧图会叠在一起

            const total = chartModel.Count();                                                   // 现在有多少根 K 线
            ctx.font = Theme.fontNormal + "px " + Theme.fontNumber;
            ctx.textAlign = "center";

            if (total === 0)
            {
                ctx.fillStyle = Theme.textSecondary;
                ctx.fillText("还没有数据", width / 2, height / 2);
                return;
            }

            const plot_width = width - chart.axis_width;                                        // 去掉右边刻度占的宽度
            const slot = plot_width / total;                                                    // 一根 K 线占多少像素
            const body_width = Math.max(1, Math.min(slot * 0.68, 14));                          // 实体宽，最宽 14 像素

            // 先把最高价、最低价、最大成交量扫一遍。
            // 不扫就不知道价格区间，没法把价格换算成屏幕坐标。
            let min_price = Number.MAX_SAFE_INTEGER;
            let max_price = 0;
            let max_volume = 0;

            for (let i = 0; i < total; ++i)
            {
                const bar = chartModel.At(i);
                min_price = Math.min(min_price, bar.low);
                max_price = Math.max(max_price, bar.high);
                max_volume = Math.max(max_volume, bar.volume);
            }

            if (max_price === min_price)
            {
                max_price += 1;
                min_price -= 1;
            }

            const price_span = max_price - min_price;                                           // 价格跨度，后面按它映射到像素
            const top_pad = 16;                                                                 // 上下各留 16 像素，别贴着边框
            const usable_height = chart.price_height - top_pad * 2;

            // ---- 画横向网格和右边的价格数字 ----
            ctx.strokeStyle = Theme.grid;                                                       // 从这里开始设置线色，后面的线都用这个颜色
            ctx.lineWidth = 1;
            ctx.fillStyle = Theme.textSecondary;
            ctx.font = Theme.fontTiny + "px " + Theme.fontNumber;
            ctx.textAlign = "left";

            const grid_lines = 5;
            for (let g = 0; g <= grid_lines; ++g)
            {
                const ratio = g / grid_lines;
                const y = top_pad + ratio * usable_height;

                // 画线三步：开始路径、移到起点、连到终点，最后 stroke 落笔。
                ctx.beginPath();
                ctx.moveTo(0, y);
                ctx.lineTo(plot_width, y);
                ctx.stroke();

                const price = max_price - ratio * price_span;
                ctx.fillText((price / 100).toFixed(2), plot_width + 8, y + 4);                  // 分转成元，两位小数
            }

            // ---- 画蜡烛 ----
            for (let i = 0; i < total; ++i)
            {
                const bar = chartModel.At(i);
                const x = i * slot + slot / 2;                                                  // 这一根的中心横坐标
                const rising = bar.close >= bar.open;                                           // 收盘不低于开盘就算涨

                // 把价格翻译成屏幕行号：价格越高，算出来的 y 越小，画得越靠上。
                // 四个点用同一个公式，只是代入的价格字段不同。
                //
                //        │ ← 最高价那一行，影线上端
                //    ┌───┐ ← 实体上边，取下面两个里小的那个
                //    └───┘ ← 实体下边，取大的那个
                //        │ ← 最低价那一行，影线下端
                const y_high = top_pad + (max_price - bar.high) / price_span * usable_height;
                const y_low = top_pad + (max_price - bar.low) / price_span * usable_height;
                const y_open = top_pad + (max_price - bar.open) / price_span * usable_height;
                const y_close = top_pad + (max_price - bar.close) / price_span * usable_height;

                ctx.strokeStyle = rising ? Theme.up : Theme.down;
                ctx.fillStyle = rising ? Theme.up : Theme.down;
                ctx.lineWidth = 1;

                ctx.beginPath();
                ctx.moveTo(x, y_high);
                ctx.lineTo(x, y_low);
                ctx.stroke();

                // 实体：涨画空心框，跌画实心块，A 股的画法。
                const y_top = Math.min(y_open, y_close);
                const body_h = Math.max(1, Math.abs(y_close - y_open));                         // 开收一样高时给 1 像素兜底

                if (rising)
                {
                    ctx.strokeRect(x - body_width / 2, y_top, body_width, body_h);
                }
                else
                {
                    ctx.fillRect(x - body_width / 2, y_top, body_width, body_h);
                }
            }

            // ---- 画成交量柱 ----
            const volume_top = chart.price_height + 18;                                         // 成交量区域从蜡烛下面 18 像素开始
            const volume_usable = chart.volume_height - 8;

            for (let i = 0; i < total; ++i)
            {
                const bar = chartModel.At(i);
                const x = i * slot + slot / 2;

                // 按最大成交量等比缩放；全是 0 时高度直接给 0。
                const h = max_volume > 0 ? (bar.volume / max_volume) * volume_usable : 0;

                ctx.fillStyle = bar.close >= bar.open ? Theme.up : Theme.down;
                ctx.globalAlpha = 0.55;                                                         // 半透明，别抢上面蜡烛的视觉
                ctx.fillRect(x - body_width / 2, volume_top + (volume_usable - h), body_width, h);
                ctx.globalAlpha = 1.0;
            }
        }
    }

    // 条数变了（收到新推送）就重画。requestPaint 只登记，Qt 会在合适的时机调一次 onPaint。
    Connections
    {
        target: chartModel
        onCountChanged: canvas.requestPaint()
    }

    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
}