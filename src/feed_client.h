#pragma once

#include "bar_data.h"

#include <QObject>
#include <QTcpSocket>
#include <QVariantList>
#include <QVector>

// 连服务端，按「4 字节大端长度 + protobuf 载荷」的格式收发。
class FeedClient final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ Status NOTIFY StatusChanged)              // 状态文字，变了界面自动刷新
    Q_PROPERTY(QVariantList symbolIds READ SymbolIds NOTIFY SymbolsChanged)  // 服务端有哪些股票，给下拉框用

public:
    explicit FeedClient(QObject* parent = nullptr);

    [[nodiscard]] QString Status() const { return status_; }
    [[nodiscard]] QVariantList SymbolIds() const;

    Q_INVOKABLE void Connect(const QString& host, int port);
    Q_INVOKABLE void Disconnect();
    Q_INVOKABLE void Subscribe(const QVector<quint32>& symbol_ids);
    Q_INVOKABLE void SubscribeSymbol(quint32 symbol_id);                     // 下拉框选中一只时调

signals:
    void StatusChanged();
    void SymbolsChanged();
    void ChartReset();                                                       // 换股票了，图表先清空
    void BarsReceived(const QVector<BarRow>& bars);

private slots:
    void OnConnected();
    void OnDisconnected();
    void OnReadyRead();

private:
    void SetStatus(const QString& text);
    void SendFrame(const QByteArray& payload);
    void RequestSymbolList();
    void HandleFrame(const QByteArray& payload);

    QTcpSocket socket_;
    QByteArray buffer_;                                                      // 半包粘包都靠它攒
    QString status_{"未连接"};
    QVector<quint32> symbol_ids_;
};