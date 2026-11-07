#include "chart_model.h"
#include "feed_client.h"

#include <QCommandLineParser>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QVector>

namespace
{

// 造一批假的 K 线，只在 --demo 时用，用来验证界面能画出来。
// 正式链路是 FeedClient 从服务端收，不经过这里。
QVector<BarRow> MakeDemoBars(int count)
{
    QVector<BarRow> bars;
    bars.reserve(count);

    const quint64 minute_ns = 60ull * 1000ull * 1000ull * 1000ull;
    const quint64 base_ts = 1760000000ull * 1000000000ull;
    qint64 price = 10000;                                              // 100.00 元，单位分

    for (int i = 0; i < count; ++i)
    {
        const qint64 open = price;
        const qint64 close = open + QRandomGenerator::global()->bounded(-120, 120);
        const qint64 high = qMax(open, close) + QRandomGenerator::global()->bounded(0, 60);
        const qint64 low = qMin(open, close) - QRandomGenerator::global()->bounded(0, 60);

        BarRow bar;
        bar.symbol_id = 0;
        bar.window_start_ns = base_ts + static_cast<quint64>(i) * minute_ns;
        bar.open = open;
        bar.high = high;
        bar.low = low;
        bar.close = close;
        bar.volume = QRandomGenerator::global()->bounded(1000, 90000);
        bar.amount = static_cast<quint64>(close) * bar.volume;
        bars.push_back(bar);

        price = close;
    }

    return bars;
}

} // namespace

// 起 Qt 界面，把 ChartModel 和 FeedClient 交给 QML 用。
int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MarketFlowUI"));

    // 界面统一字体。QML 里没显式指定 family 的地方都继承这个。
    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));


    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("MarketFlow 行情终端"));
    parser.addHelpOption();
    const QCommandLineOption demo_option(QStringLiteral("demo"), QStringLiteral("用假数据预览界面，不连服务端"));
    parser.addOption(demo_option);
    parser.process(app);

    ChartModel chart_model;
    FeedClient feed_client;

    QObject::connect(&feed_client, &FeedClient::BarsReceived, &chart_model, &ChartModel::AppendBars);

    if (parser.isSet(demo_option))
    {
        chart_model.SetBars(MakeDemoBars(240));
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("chartModel"), &chart_model);
    engine.rootContext()->setContextProperty(QStringLiteral("feedClient"), &feed_client);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    return app.exec();
}