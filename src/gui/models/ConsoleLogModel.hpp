#pragma once

#include <QAbstractListModel>

#include <vector>

namespace z3wb::gui {

// Append-only console log. Rows are never removed or reordered, so appends
// use beginInsertRows/endInsertRows instead of full resets.
class ConsoleLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Row
    {
        QString strTime;
        QString strLevel; // "INFO" | "WARNING" | "ERROR"
        QString strText;
    };

    enum
    {
        k_iTimeRole = Qt::UserRole + 1,
        k_iLevelRole,
        k_iTextRole,
    };

    explicit ConsoleLogModel(QObject* pParent = nullptr);

    void append(Row oRow);

    [[nodiscard]] int rowCount(const QModelIndex& oParent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& oIndex, int iRole = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<Row> m_vecRows;
};

} // namespace z3wb::gui
