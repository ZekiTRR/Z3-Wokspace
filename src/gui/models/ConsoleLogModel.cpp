#include "gui/models/ConsoleLogModel.hpp"

namespace z3wb::gui {

ConsoleLogModel::ConsoleLogModel(QObject* pParent)
    : QAbstractListModel(pParent)
{
}

void ConsoleLogModel::append(Row oRow)
{
    const int iNewRow = static_cast<int>(m_vecRows.size());
    beginInsertRows({}, iNewRow, iNewRow);
    m_vecRows.push_back(std::move(oRow));
    endInsertRows();
}

int ConsoleLogModel::rowCount(const QModelIndex& oParent) const
{
    return oParent.isValid() ? 0 : static_cast<int>(m_vecRows.size());
}

QVariant ConsoleLogModel::data(const QModelIndex& oIndex, int iRole) const
{
    if (!hasIndex(oIndex.row(), oIndex.column(), {}))
    {
        return {};
    }

    const Row& oRow = m_vecRows[static_cast<std::size_t>(oIndex.row())];
    switch (iRole)
    {
        case k_iTimeRole:
            return oRow.strTime;
        case k_iLevelRole:
            return oRow.strLevel;
        case k_iTextRole:
            return oRow.strText;
        default:
            return {};
    }
}

QHash<int, QByteArray> ConsoleLogModel::roleNames() const
{
    auto oRoles = QAbstractListModel::roleNames();
    oRoles[k_iTimeRole] = "cTime";
    oRoles[k_iLevelRole] = "cLevel";
    oRoles[k_iTextRole] = "cText";
    return oRoles;
}

} // namespace z3wb::gui
