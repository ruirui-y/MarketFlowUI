#pragma once

#include <QtGlobal>

// 一根分钟 K 线。网络收下来和给 QML 显示用的是同一份，不再转来转去。
struct BarRow
{
    quint32 symbol_id{0};
    quint64 window_start_ns{0};
    qint64 open{0};
    qint64 high{0};
    qint64 low{0};
    qint64 close{0};
    quint64 volume{0};
    quint64 amount{0};
};