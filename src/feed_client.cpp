#include "feed_client.h"

#include "market_feed.pb.h"

#include <QtEndian>

namespace
{
constexpr int kLengthFieldBytes = 4;                               // 长度前缀占 4 字节
constexpr quint32 kMaxFrameBytes = 8u << 20;                       // 单帧最大 8MB，防止被喂爆内存
}

FeedClient::FeedClient(QObject* parent)
    : QObject(parent)
{
    connect(&socket_, &QTcpSocket::connected, this, &FeedClient::OnConnected);
    connect(&socket_, &QTcpSocket::disconnected, this, &FeedClient::OnDisconnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &FeedClient::OnReadyRead);
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError)
        {
            SetStatus(QStringLiteral("连接出错：") + socket_.errorString());
        });
}

// 发起连接。不做重连，断了就断了。
void FeedClient::Connect(const QString& host, int port)
{
    SetStatus(QStringLiteral("连接中 ") + host + QLatin1Char(':') + QString::number(port));
    socket_.connectToHost(host, static_cast<quint16>(port));
}

void FeedClient::Disconnect()
{
    socket_.disconnectFromHost();
}

// 告诉服务端我只要这几个标的。
void FeedClient::Subscribe(const QVector<quint32>& symbol_ids)
{
    if (socket_.state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    marketflow::feed::SubscribeRequest request;
    for (const quint32 symbol_id : symbol_ids)
    {
        request.add_symbol_ids(symbol_id);
    }

    QByteArray payload(static_cast<int>(request.ByteSizeLong()), Qt::Uninitialized);
    request.SerializeToArray(payload.data(), static_cast<int>(payload.size()));

    // 前面的 4 字节写成大端长度，后面的 payload 原样跟。
    QByteArray frame(kLengthFieldBytes, Qt::Uninitialized);
    qToBigEndian<quint32>(static_cast<quint32>(payload.size()), frame.data());
    socket_.write(frame);
    socket_.write(payload);
}

void FeedClient::SetStatus(const QString& text)
{
    status_ = text;
    emit StatusChanged();
}

void FeedClient::OnConnected()
{
    SetStatus(QStringLiteral("已连接"));
}

void FeedClient::OnDisconnected()
{
    SetStatus(QStringLiteral("已断开"));
}

// 收到的字节先攒进 buffer_，凑够一帧就切出来处理。
void FeedClient::OnReadyRead()
{
    buffer_ += socket_.readAll();

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

// 把一帧 protobuf 拆成 K 线，转成 BarRow 发出去。
void FeedClient::HandleFrame(const QByteArray& payload)
{
    marketflow::feed::FeedFrame frame;
    if (!frame.ParseFromArray(payload.constData(), payload.size()))
    {
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
        emit BarsReceived(bars);
    }
}