#include "log.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

namespace marketflowui::log
{

namespace
{

constexpr int kKeepDays = 15;                                     // 日志保留天数

QMutex g_mutex;                                                   // handler 可能在任何线程被调，必须锁
QFile g_file;
QDate g_opened_day;
QString g_log_dir;
int g_min_rank = 0;
QtMessageHandler g_previous_handler = nullptr;                    // 上一个 handler，用来继续输出到 VS

// QtMsgType 的枚举值不是按严重程度递增的（Info 是 4，Warning 是 1），所以自己排个序。
int Rank(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
        return 0;
    case QtInfoMsg:
        return 1;
    case QtWarningMsg:
        return 2;
    case QtCriticalMsg:
        return 3;
    case QtFatalMsg:
        return 4;
    }

    return 0;
}

// 日志里的级别名，跟服务端的 [info] / [warning] 保持一致。
const char* LevelName(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
        return "debug";
    case QtInfoMsg:
        return "info";
    case QtWarningMsg:
        return "warning";
    case QtCriticalMsg:
        return "critical";
    case QtFatalMsg:
        return "fatal";
    }

    return "debug";
}

QString FileNameFor(const QDate& day)
{
    return QStringLiteral("Log_%1_%2_%3.log")
        .arg(day.year(), 4, 10, QLatin1Char('0'))
        .arg(day.month(), 2, 10, QLatin1Char('0'))
        .arg(day.day(), 2, 10, QLatin1Char('0'));
}

// 换天了就关旧文件、开新文件。调用前必须已经持有 g_mutex。
void EnsureFileFor(const QDate& day)
{
    if (g_file.isOpen() && g_opened_day == day)
    {
        return;
    }

    if (g_file.isOpen())
    {
        g_file.close();
    }

    g_file.setFileName(g_log_dir + QLatin1Char('/') + FileNameFor(day));
    g_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    g_opened_day = day;
}

// 删掉 15 天以前的日志文件；名字认不出来的不动。
void PruneOld()
{
    const QDate cutoff = QDate::currentDate().addDays(-kKeepDays);
    const QDir dir(g_log_dir);

    const QFileInfoList entries = dir.entryInfoList(QStringList() << QStringLiteral("Log_*.log"), QDir::Files);
    for (const QFileInfo& entry : entries)
    {
        const QString name = entry.fileName();
        const QString datePart = name.mid(4, 10);                 // Log_2026_11_09.log → 2026_11_09
        const QDate day = QDate::fromString(datePart, QStringLiteral("yyyy_MM_dd"));

        if (day.isValid() && day < cutoff)
        {
            QFile::remove(entry.absoluteFilePath());
        }
    }
}

// 我们自己安装的 handler：写文件，再把消息交给原来的 handler（VS 输出窗口还能看到）。
void Handler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    {
        QMutexLocker locker(&g_mutex);

        if (Rank(type) >= g_min_rank)
        {
            const QDate today = QDate::currentDate();
            EnsureFileFor(today);

            if (g_file.isOpen())
            {
                const QString line = QStringLiteral("[%1] [%2] [%3] %4\n")
                    .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))
                    .arg(QLatin1String(LevelName(type)))
                    .arg(context.category != nullptr ? QLatin1String(context.category) : QLatin1String("client"))
                    .arg(message);

                g_file.write(line.toUtf8());

                // warning 以上立刻落盘，崩溃前的日志不会丢。
                if (Rank(type) >= Rank(QtWarningMsg))
                {
                    g_file.flush();
                }
            }
        }
    }

    if (g_previous_handler != nullptr)
    {
        g_previous_handler(type, context, message);
    }
}

}  // namespace

// 初始化：建目录、清旧文件、接管 Qt 的日志输出。
QString Init(QtMsgType minimum_level)
{
    g_log_dir = QCoreApplication::applicationDirPath() + QStringLiteral("/Log");
    QDir().mkpath(g_log_dir);
    g_min_rank = Rank(minimum_level);

    {
        QMutexLocker locker(&g_mutex);
        PruneOld();
        EnsureFileFor(QDate::currentDate());
    }

    g_previous_handler = qInstallMessageHandler(Handler);

    return g_log_dir;
}

}  // namespace marketflowui::log