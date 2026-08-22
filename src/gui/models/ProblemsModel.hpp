#pragma once

#include <QAbstractListModel>

#include <vector>

namespace z3wb::gui {

// Flat list of problems for the Project Explorer panel. The view model
// pushes full snapshots; the panel never mutates core state directly.
class ProblemsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct ProblemRow
    {
        QString strName;
        std::uint64_t uId = 0;
    };

    enum
    {
        k_iNameRole = Qt::UserRole + 1,
        k_iIdRole,
    };

    explicit ProblemsModel(QObject* pParent = nullptr);

    void resetWith(std::vector<ProblemRow> vecRows);

    [[nodiscard]] int rowCount(const QModelIndex& oParent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& oIndex, int iRole = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<ProblemRow> m_vecRows;
};

} // namespace z3wb::gui
