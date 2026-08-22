pragma Singleton
import QtQuick

// Dark IDE palette (VS Code-like). Single source of truth for colors and
// base font metrics; panels must not hardcode colors.
QtObject {
    readonly property color background: "#1e1f24"
    readonly property color panel: "#252630"
    readonly property color panelDark: "#212229"
    readonly property color editor: "#1b1c21"
    readonly property color border: "#3c3d45"
    readonly property color selection: "#2a2d3e"

    readonly property color text: "#d4d4d4"
    readonly property color textDim: "#8b8d97"

    readonly property color accent: "#3794ff"
    readonly property color button: "#0e639c"
    readonly property color buttonHover: "#1177bb"

    readonly property color success: "#89d185"
    readonly property color error: "#f48771"
    readonly property color warning: "#dcdcaa"
    readonly property color info: "#569cd6"

    readonly property int fontSize: 13
    readonly property int headerFontSize: 11
}
