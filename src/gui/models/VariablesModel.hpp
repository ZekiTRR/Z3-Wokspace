#pragma once

#include <QAbstractListModel>

#include <vector>

namespace z3wb::gui {

// Variables of the current problem. Before solving only name/type are filled;
// after a SAT run the value column carries the model value.
class VariablesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Row
    {
        QString strName;
        QString strType;
        QString strValue;
    };

    enum
    {
        k_iNameRole = Qt::UserRole + 1,
        k_iTypeRole,
        k_iValueRole,
        k_iHasValueRole,
    };

    explicit VariablesModel(QObject* pParent = nullptr);

    void resetWith(std::vector<Row> vecRows);

    [[nodiscard]] int rowCount(const QModelIndex& oParent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& oIndex, int iRole = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<Row> m_vecRows;
};

} // namespace z3wb::gui
