#pragma once

#include "bar_data.h"

#include <QObject>
#include <QTcpSocket>
#include <QVector>
#include <QByteArray>

// 连服务端，按「4 字节大端长度 + protobuf 载荷」的格式收 K 线推送。
class FeedClient final : public QObject
{
    Q_OBJECT
public:
    explicit FeedClient(QObject* parent = nullptr);

    Q_INVOKABLE void Connect(const QString& host, int port);
    Q_INVOKABLE void Disconnect();
    Q_INVOKABLE void Subscribe(const QVector<quint32>& symbol_ids);

    Q_INVOKABLE QString Status() const { return status_; }

signals:
    void StatusChanged();
    void BarsReceived(const QVector<BarRow>& bars);

private slots:
    void OnConnected();
    void OnDisconnected();
    void OnReadyRead();

private:
    void SetStatus(const QString& text);
    void HandleFrame(const QByteArray& payload);

    QTcpSocket socket_;
    QByteArray buffer_;                                            // 半包粘包都靠它攒
    QString status_{"未连接"};
};