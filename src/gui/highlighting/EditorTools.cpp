#include "gui/highlighting/EditorTools.hpp"

#include "gui/highlighting/DslHighlighter.hpp"

#include <QQuickTextDocument>

namespace z3wb::gui {

EditorTools::EditorTools(QObject* pParent)
    : QObject(pParent)
{
}

void EditorTools::attachHighlighter(QObject* pTextDocument)
{
    auto* pQuickDocument = qobject_cast<QQuickTextDocument*>(pTextDocument);
    if (pQuickDocument == nullptr)
    {
        return;
    }

    QTextDocument* pDocument = pQuickDocument->textDocument();
    // One highlighter per document: re-attachment replaces the old instance
    // by parenting to the same document.
    new DslHighlighter(pDocument);
}

} // namespace z3wb::gui
