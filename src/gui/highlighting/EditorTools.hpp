#pragma once

#include <QObject>

namespace z3wb::gui {

// Small QML-facing helper that attaches the DSL highlighter to the editor's
// text document. Lives as the "editorTools" context property.
class EditorTools : public QObject
{
    Q_OBJECT

public:
    explicit EditorTools(QObject* pParent = nullptr);

    // Accepts a QQuickTextDocument from QML (TextArea.textDocument).
    Q_INVOKABLE void attachHighlighter(QObject* pTextDocument);
};

} // namespace z3wb::gui
