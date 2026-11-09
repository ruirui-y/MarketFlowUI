#pragma once

#include <QString>
#include <QtGlobal>

namespace marketflowui::log
{

// 初始化客户端日志：把 qDebug / qInfo / qWarning 等全部写进「可执行文件旁边的 Log/」目录，
// 文件名 Log_年_月_日.log，保留 15 天。同时仍然输出到 VS 的输出窗口。
// 必须在 QApplication 建好之后再调用，因为要用 applicationDirPath()。
QString Init(QtMsgType minimum_level = QtDebugMsg);

}  // namespace marketflowui::log