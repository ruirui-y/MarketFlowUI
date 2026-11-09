#include "feed_client.h"

#include "log.h"
#include "market_feed.pb.h"

#include <QDebug>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QtEndian>

namespace
{
constexpr int kLengthFieldBytes = 4;                               // 长度前缀占 4 字节
constexpr quint32 kMaxFrameBytes = 8u << 20;                       // 单帧最大 8MB，防止被喂爆内存

// 把一帧 protobuf 拼上 4 字节大端长度后写出去。
void SendFramed(QTcpSocket& socket, const google::protobuf::MessageLite& message)
{
    QByteArray payload(static_cast<int>(message.ByteSizeLong()), Qt::Uninitialized);
    message.SerializeToArray(payload.data(), static_cast<int>(payload.size()));

    QByteArray header(kLengthFieldBytes, Qt::Uninitialized);
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()), header.data());

    socket.write(header);
    socket.write(payload);
}
}

FeedClient::FeedClient(QObject* parent)
    : QObject(parent)
{
    // 显式禁用代理：机器上配了系统代理时，Qt 会把它套到 QTcpSocket 上，
    // 连内网直连地址会直接报 "The proxy type is invalid for this operation"。
    socket_.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    connect(&socket_, &QTcpSocket::connected, this, &FeedClient::OnConnected);
    connect(&socket_, &QTcpSocket::disconnected, this, &FeedClient::OnDisconnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &FeedClient::OnReadyRead);
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError)
        {
            qWarning().noquote() << QStringLiteral("连接出错: %1 (状态=%2)").arg(socket_.errorString()).arg(static_cast<int>(socket_.state()));
            SetStatus(QStringLiteral("连接出错：") + socket_.errorString());
        });
}

// 把「服务端有哪些股票」转成 QML 能直接绑到下拉框的列表。
QVariantList FeedClient::SymbolIds() const
{
    QVariantList result;
    result.reserve(symbol_ids_.size());
    for (const quint32 symbol_id : symbol_ids_)
    {
        result.push_back(symbol_id);
    }
    return result;
}

// 发起连接。不做重连，断了就断了。
void FeedClient::Connect(const QString& host, int port)
{
    qInfo().noquote() << QStringLiteral("开始连接 %1:%2").arg(host).arg(port);

    SetStatus(QStringLiteral("连接中 ") + host + QLatin1Char(':') + QString::number(port));
    socket_.connectToHost(host, static_cast<quint16>(port));
}

void FeedClient::Disconnect()
{
    socket_.disconnectFromHost();
}

// 订阅指定标的。传空表示订阅全部。
void FeedClient::Subscribe(const QVector<quint32>& symbol_ids)
{
    if (socket_.state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    marketflow::feed::ClientFrame frame;
    frame.set_type(marketflow::feed::ClientFrame::SUBSCRIBE);
    for (const quint32 symbol_id : symbol_ids)
    {
        frame.mutable_subscribe()->add_symbol_ids(symbol_id);
    }

    qInfo().noquote() << QStringLiteral("发送订阅, 标的数=%1（0 表示全部）").arg(symbol_ids.size());

    QByteArray payload(static_cast<int>(frame.ByteSizeLong()), Qt::Uninitialized);
    frame.SerializeToArray(payload.data(), static_cast<int>(payload.size()));
    SendFrame(payload);
}

// 下拉框选中一只股票：先清图，再只订阅这一只。
void FeedClient::SubscribeSymbol(quint32 symbol_id)
{
    qInfo().noquote() << QStringLiteral("切换股票, symbol_id=%1").arg(symbol_id);

    emit ChartReset();
    Subscribe({symbol_id});
}

void FeedClient::SetStatus(const QString& text)
{
    status_ = text;
    emit StatusChanged();
}

// 给一段 protobuf 载荷加上长度头，写出去。
void FeedClient::SendFrame(const QByteArray& payload)
{
    QByteArray header(kLengthFieldBytes, Qt::Uninitialized);
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()), header.data());

    socket_.write(header);
    socket_.write(payload);
}

// 问服务端要股票列表。
void FeedClient::RequestSymbolList()
{
    if (socket_.state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    marketflow::feed::ClientFrame frame;
    frame.set_type(marketflow::feed::ClientFrame::SYMBOL_LIST);

    qInfo().noquote() << QStringLiteral("请求股票列表");

    QByteArray payload(static_cast<int>(frame.ByteSizeLong()), Qt::Uninitialized);
    frame.SerializeToArray(payload.data(), static_cast<int>(payload.size()));
    SendFrame(payload);
}

void FeedClient::OnConnected()
{
    qInfo().noquote() << QStringLiteral("已连接: 本地=%1 对端=%2")
                             .arg(socket_.localAddress().toString())
                             .arg(socket_.peerAddress().toString());

    SetStatus(QStringLiteral("已连接"));

    // 连上先要股票列表，拿到列表后自动订阅第一只。
    RequestSymbolList();
}

void FeedClient::OnDisconnected()
{
    qInfo().noquote() << QStringLiteral("连接已断开");

    SetStatus(QStringLiteral("已断开"));
}

// 收到的字节先攒进 buffer_，凑够一帧就切出来处理。
void FeedClient::OnReadyRead()
{
    buffer_ += socket_.readAll();

    qDebug().noquote() << QStringLiteral("收到数据, 本次累计缓冲 %1 字节").arg(buffer_.size());

    while (buffer_.size() >= kLengthFieldBytes)
    {
        const quint32 body_size = qFromBigEndian<quint32>(buffer_.constData());

        if (body_size > kMaxFrameBytes)
        {
            SetStatus(QStringLiteral("单帧过大，断开连接"));
            socket_.abort();
            buffer_.clear();
            return;
        }

        if (static_cast<quint32>(buffer_.size()) < kLengthFieldBytes + body_size)
        {
            return;                                                // 还没收全，等下一次 readyRead
        }

        const QByteArray payload = buffer_.mid(kLengthFieldBytes, static_cast<int>(body_size));
        buffer_.remove(0, kLengthFieldBytes + static_cast<int>(body_size));
        HandleFrame(payload);
    }
}

// 服务端发来的帧有两种：股票列表，或者一批 K 线。
void FeedClient::HandleFrame(const QByteArray& payload)
{
    marketflow::feed::FeedFrame frame;
    if (!frame.ParseFromArray(payload.constData(), payload.size()))
    {
        qWarning().noquote() << QStringLiteral("FeedFrame 解析失败, 载荷 %1 字节").arg(payload.size());
        return;
    }

    if (frame.type() == marketflow::feed::FeedFrame::SYMBOLS)
    {
        symbol_ids_.clear();
        for (const std::uint32_t symbol_id : frame.symbol_ids())
        {
            symbol_ids_.push_back(symbol_id);
        }

        qInfo().noquote() << QStringLiteral("收到股票列表, 数量=%1").arg(symbol_ids_.size());
        emit SymbolsChanged();

        // 默认选第一只，界面一打开就有图。
        if (!symbol_ids_.isEmpty())
        {
            SubscribeSymbol(symbol_ids_.first());
        }
        return;
    }

    if (frame.type() != marketflow::feed::FeedFrame::BARS)
    {
        return;
    }

    QVector<BarRow> bars;
    bars.reserve(frame.bars_size());

    for (int i = 0; i < frame.bars_size(); ++i)
    {
        const marketflow::feed::Bar& source = frame.bars(i);

        BarRow bar;
        bar.symbol_id = source.symbol_id();
        bar.window_start_ns = source.window_start_ns();
        bar.open = source.open();
        bar.high = source.high();
        bar.low = source.low();
        bar.close = source.close();
        bar.volume = source.volume();
        bar.amount = source.amount();
        bars.push_back(bar);
    }

    if (!bars.isEmpty())
    {
        qDebug().noquote() << QStringLiteral("解析出 %1 根 K 线").arg(bars.size());
        emit BarsReceived(bars);
    }
}