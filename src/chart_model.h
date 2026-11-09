#pragma once

#include "bar_data.h"

#include <QAbstractListModel>
#include <QVariantMap>
#include <QVector>

// 把收到的 K 线存成一个列表模型。
class ChartModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ Count NOTIFY CountChanged)            // QML 里写 chartModel.count，条数变了自动刷新

public:
    enum Role
    {
        WindowStartRole = Qt::UserRole + 1,
        OpenRole,
        HighRole,
        LowRole,
        CloseRole,
        VolumeRole
    };

    explicit ChartModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE int Count() const { return bars_.size(); }
    Q_INVOKABLE QVariantMap At(int row) const;                     // Canvas 画图时按行取，比走 index 方便

signals:
    void CountChanged();                                           // 条数变了，QML 收到就重画

public slots:
    void AppendBars(const QVector<BarRow>& bars);                  // 追加一批，界面跟着长
    void SetBars(const QVector<BarRow>& bars);                     // 换一整套，比如切标的
    void Clear();                                                 // 清空，切股票之前调

private:
    QVector<BarRow> bars_;
};