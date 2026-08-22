#include "gui/models/DiagnosticsModel.hpp"

namespace z3wb::gui {

DiagnosticsModel::DiagnosticsModel(QObject* pParent)
    : QAbstractListModel(pParent)
{
}

void DiagnosticsModel::resetWith(std::vector<Row> vecRows)
{
    beginResetModel();
    m_vecRows = std::move(vecRows);
    endResetModel();
}

int DiagnosticsModel::rowCount(const QModelIndex& oParent) const
{
    return oParent.isValid() ? 0 : static_cast<int>(m_vecRows.size());
}

QVariant DiagnosticsModel::data(const QModelIndex& oIndex, int iRole) const
{
    if (!hasIndex(oIndex.row(), oIndex.column(), {}))
    {
        return {};
    }

    const Row& oRow = m_vecRows[static_cast<std::size_t>(oIndex.row())];
    switch (iRole)
    {
        case k_iSeverityRole:
            return oRow.strSeverity;
        case k_iMessageRole:
            return oRow.strMessage;
        case k_iLineRole:
            return oRow.iLine;
        case k_iColumnRole:
            return oRow.iColumn;
        default:
            return {};
    }
}

QHash<int, QByteArray> DiagnosticsModel::roleNames() const
{
    auto oRoles = QAbstractListModel::roleNames();
    oRoles[k_iSeverityRole] = "dSeverity";
    oRoles[k_iMessageRole] = "dMessage";
    oRoles[k_iLineRole] = "dLine";
    oRoles[k_iColumnRole] = "dColumn";
    return oRoles;
}

} // namespace z3wb::gui
