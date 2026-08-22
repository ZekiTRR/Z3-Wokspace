#pragma once

#include <QAbstractListModel>

#include <vector>

namespace z3wb::gui {

// Editor diagnostics (syntax + semantic errors) with source positions.
class DiagnosticsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Row
    {
        QString strSeverity; // "error" | "warning" | "info"
        QString strMessage;
        int iLine = 0;
        int iColumn = 0;
    };

    enum
    {
        k_iSeverityRole = Qt::UserRole + 1,
        k_iMessageRole,
        k_iLineRole,
        k_iColumnRole,
    };

    explicit DiagnosticsModel(QObject* pParent = nullptr);

    void resetWith(std::vector<Row> vecRows);

    [[nodiscard]] int rowCount(const QModelIndex& oParent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& oIndex, int iRole = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<Row> m_vecRows;
};

} // namespace z3wb::gui
