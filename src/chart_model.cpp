#include "chart_model.h"

ChartModel::ChartModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ChartModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return bars_.size();
}

QVariant ChartModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= bars_.size())
    {
        return {};
    }

    const BarRow& bar = bars_.at(index.row());

    switch (role)
    {
    case WindowStartRole:
        return QVariant::fromValue(bar.window_start_ns);
    case OpenRole:
        return QVariant::fromValue(bar.open);
    case HighRole:
        return QVariant::fromValue(bar.high);
    case LowRole:
        return QVariant::fromValue(bar.low);
    case CloseRole:
        return QVariant::fromValue(bar.close);
    case VolumeRole:
        return QVariant::fromValue(bar.volume);
    default:
        return {};
    }
}

QHash<int, QByteArray> ChartModel::roleNames() const
{
    return {
        { WindowStartRole, "windowStart" },
        { OpenRole, "open" },
        { HighRole, "high" },
        { LowRole, "low" },
        { CloseRole, "close" },
        { VolumeRole, "volume" },
    };
}

// 取第 row 根，给 Canvas 一笔一笔画用。价格单位仍是「分」，QML 里除 100 显示。
QVariantMap ChartModel::At(int row) const
{
    if (row < 0 || row >= bars_.size())
    {
        return {};
    }

    const BarRow& bar = bars_.at(row);

    QVariantMap out;
    out.insert("windowStart", QVariant::fromValue(bar.window_start_ns));
    out.insert("open", QVariant::fromValue(bar.open));
    out.insert("high", QVariant::fromValue(bar.high));
    out.insert("low", QVariant::fromValue(bar.low));
    out.insert("close", QVariant::fromValue(bar.close));
    out.insert("volume", QVariant::fromValue(bar.volume));
    return out;
}

// 追加一批。
void ChartModel::AppendBars(const QVector<BarRow>& bars)
{
    if (bars.isEmpty())
    {
        return;
    }

    const int first = bars_.size();
    beginInsertRows(QModelIndex(), first, first + bars.size() - 1);
    bars_ += bars;
    endInsertRows();
    emit CountChanged();
}

// 整批替换。
void ChartModel::SetBars(const QVector<BarRow>& bars)
{
    beginResetModel();
    bars_ = bars;
    endResetModel();
    emit CountChanged();
}

// 清空。切股票之前调，不然新旧两根会拼在一起。
void ChartModel::Clear()
{
    beginResetModel();
    bars_.clear();
    endResetModel();
    emit CountChanged();
}