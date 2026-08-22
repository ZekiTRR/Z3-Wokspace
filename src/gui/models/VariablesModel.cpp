#include "gui/models/VariablesModel.hpp"

namespace z3wb::gui {

VariablesModel::VariablesModel(QObject* pParent)
    : QAbstractListModel(pParent)
{
}

void VariablesModel::resetWith(std::vector<Row> vecRows)
{
    beginResetModel();
    m_vecRows = std::move(vecRows);
    endResetModel();
}

int VariablesModel::rowCount(const QModelIndex& oParent) const
{
    return oParent.isValid() ? 0 : static_cast<int>(m_vecRows.size());
}

QVariant VariablesModel::data(const QModelIndex& oIndex, int iRole) const
{
    if (!hasIndex(oIndex.row(), oIndex.column(), {}))
    {
        return {};
    }

    const Row& oRow = m_vecRows[static_cast<std::size_t>(oIndex.row())];
    switch (iRole)
    {
        case k_iNameRole:
            return oRow.strName;
        case k_iTypeRole:
            return oRow.strType;
        case k_iValueRole:
            return oRow.strValue;
        case k_iHasValueRole:
            return !oRow.strValue.isEmpty();
        default:
            return {};
    }
}

QHash<int, QByteArray> VariablesModel::roleNames() const
{
    auto oRoles = QAbstractListModel::roleNames();
    oRoles[k_iNameRole] = "vName";
    oRoles[k_iTypeRole] = "vType";
    oRoles[k_iValueRole] = "vValue";
    oRoles[k_iHasValueRole] = "vHasValue";
    return oRoles;
}

} // namespace z3wb::gui
