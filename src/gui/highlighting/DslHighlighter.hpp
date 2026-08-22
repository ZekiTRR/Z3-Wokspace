#pragma once

#include <QSyntaxHighlighter>

#include <QColor>
#include <QRegularExpression>
#include <QVector>

namespace z3wb::gui {

// Lightweight regex-based highlighter for the workbench DSL. Block comments
// are tracked through block states so multi-line /* */ works.
//
// Colors mirror Theme.qml (the QML palette is not reachable from C++ without
// an extra bridge; keep the two lists in sync).
class DslHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DslHighlighter(QTextDocument* pParent);

protected:
    void highlightBlock(const QString& strText) override;

private:
    struct Rule
    {
        QRegularExpression oPattern;
        QColor oColor;
        bool bBold = false;
    };

    QVector<Rule> m_vecRules;
};

} // namespace z3wb::gui
