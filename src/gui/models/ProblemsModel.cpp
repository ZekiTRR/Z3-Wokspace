#include "gui/models/ProblemsModel.hpp"

namespace z3wb::gui {

ProblemsModel::ProblemsModel(QObject* pParent)
    : QAbstractListModel(pParent)
{
}

void ProblemsModel::resetWith(std::vector<ProblemRow> vecRows)
{
    beginResetModel();
    m_vecRows = std::move(vecRows);
    endResetModel();
}

int ProblemsModel::rowCount(const QModelIndex& oParent) const
{
    return oParent.isValid() ? 0 : static_cast<int>(m_vecRows.size());
}

QVariant ProblemsModel::data(const QModelIndex& oIndex, int iRole) const
{
    if (!hasIndex(oIndex.row(), oIndex.column(), {}))
    {
        return {};
    }

    const ProblemRow& oRow = m_vecRows[static_cast<std::size_t>(oIndex.row())];
    switch (iRole)
    {
        case k_iNameRole:
            return oRow.strName;
        case k_iIdRole:
            return oRow.uId;
        default:
            return {};
    }
}

QHash<int, QByteArray> ProblemsModel::roleNames() const
{
    auto oRoles = QAbstractListModel::roleNames();
    oRoles[k_iNameRole] = "pName";
    oRoles[k_iIdRole] = "pId";
    return oRoles;
}

} // namespace z3wb::gui
