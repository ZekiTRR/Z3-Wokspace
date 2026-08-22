#include "gui/highlighting/DslHighlighter.hpp"

#include <QTextCharFormat>

namespace z3wb::gui {

namespace {

// Keep in sync with qml/Theme.qml.
constexpr auto k_strKeyword = "#569cd6"; // blue
constexpr auto k_strType = "#4ec9b0";    // teal
constexpr auto k_strBool = "#dcdcaa";    // yellow
constexpr auto k_strNumber = "#b5cea8";  // light green
constexpr auto k_strString = "#ce9178";  // orange
constexpr auto k_strComment = "#6a9955"; // gray green

} // namespace

DslHighlighter::DslHighlighter(QTextDocument* pParent)
    : QSyntaxHighlighter(pParent)
{
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "\\b(?:var|constraint)\\b")), QColor(k_strKeyword), true});
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "\\b(?:Bool|Int|Real|String|BitVec)\\b")), QColor(k_strType), false});
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "\\b(?:true|false)\\b")), QColor(k_strBool), false});
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "\\b(?:0[xX][0-9a-fA-F]+|0[bB][01]+|\\d+(?:\\.\\d+)?)\\b")),
        QColor(k_strNumber), false});
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "\"[^\"]*\"")), QColor(k_strString), false});
    m_vecRules.append({QRegularExpression(QStringLiteral(
        "//[^\n]*")), QColor(k_strComment), false});
}

void DslHighlighter::highlightBlock(const QString& strText)
{
    for (const Rule& oRule : std::as_const(m_vecRules))
    {
        QRegularExpressionMatchIterator itMatches =
            oRule.oPattern.globalMatch(strText);
        while (itMatches.hasNext())
        {
            const QRegularExpressionMatch oMatch = itMatches.next();
            QTextCharFormat oFormat;
            oFormat.setForeground(oRule.oColor);
            if (oRule.bBold)
            {
                oFormat.setFontWeight(QFont::Bold);
            }
            setFormat(oMatch.capturedStart(), oMatch.capturedLength(), oFormat);
        }
    }

    // Block comments: state 1 = inside /* ... */, spanning blocks.
    setCurrentBlockState(0);
    int iIndex = (previousBlockState() == 1) ? 0 : strText.indexOf(QStringLiteral("/*"));
    while (iIndex >= 0)
    {
        const int iEnd = strText.indexOf(QStringLiteral("*/"), iIndex + 2);
        int iLength;
        if (iEnd == -1)
        {
            setCurrentBlockState(1);
            iLength = strText.length() - iIndex;
        }
        else
        {
            iLength = iEnd - iIndex + 2;
        }

        QTextCharFormat oFormat;
        oFormat.setForeground(QColor(k_strComment));
        setFormat(iIndex, iLength, oFormat);

        iIndex = strText.indexOf(QStringLiteral("/*"), iIndex + iLength);
    }
}

} // namespace z3wb::gui
