#include "theme_service.h"
#include "src/services/design_tokens.h"
#include "src/configuration.h"
#include <algorithm>
#include "src/utilities/logger.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QPalette>
#include <QScrollBar>
#include <QStyle>
#include <QStyleFactory>
#include <QToolTip>
#include <QXmlStreamReader>

namespace NezhaIDE::Services {

static const QHash<QString, QString> kFallbackColors = {
    {"bg.primary",         "#FFFFFF"},
    {"bg.secondary",       "#F2F2F7"},
    {"bg.tertiary",        "#FAFAFA"},
    {"border",             "#E5E5E5"},
    {"accent",             "#007AFF"},
    {"accent.hover",       "#0071E3"},
    {"accent.pressed",     "#005BB8"},
    {"accent.subtle",      "rgba(0,122,255,0.10)"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#1F2329"},
    {"text.secondary",     "#646A73"},
    {"text.tertiary",      "#999999"},
    {"overlay.hover",      "rgba(0,0,0,0.04)"},
    {"overlay.hover.subtle","rgba(0,0,0,0.02)"},
    {"overlay.selection",  "rgba(0,122,255,0.12)"},
    {"git.modified",       "#E67E22"},
    {"git.added",          "#27AE60"},
    {"git.deleted",        "#E74C3C"},
    {"git.untracked",      "#999999"},
    {"syntax.editor.background", "#FFFFFF"},
    {"syntax.editor.foreground", "#1A1A1A"},
    {"syntax.keyword",     "#CF1F8B"},
    {"syntax.type",        "#0E7A7A"},
    {"syntax.function",    "#2563EB"},
    {"syntax.string",      "#1A8C3A"},
    {"syntax.comment",     "#6B7280"},
    {"syntax.number",      "#C06014"},
    {"syntax.operator",    "#1A1A1A"},
    {"syntax.preprocessor","#7C3AED"},
    {"panel.bg",           "#FFFFFF"},
    {"panel.header",       "#F2F2F7"},
    {"panel.border",       "#E5E5E5"},
    {"badge.bg",           "#007AFF"},
    {"badge.text",         "#FFFFFF"},
    {"scrollbar.bg",       "transparent"},
    {"scrollbar.handle",   "rgba(0,0,0,0.18)"},
    {"scrollbar.handle.hover","rgba(0,0,0,0.32)"},
    {"input.bg",           "#FAFAFA"},
    {"input.border",       "#E5E5E5"},
};

static const QHash<QString, QString> kFallbackColorsDark = {
    {"bg.primary",         "#0D1117"},
    {"bg.secondary",       "#161B22"},
    {"bg.tertiary",        "#1C2128"},
    {"border",             "#30363D"},
    {"accent",             "#0A84FF"},
    {"accent.hover",       "#3395FF"},
    {"accent.pressed",     "#0071E3"},
    {"accent.subtle",      "rgba(10,132,255,0.18)"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#E6EDF3"},
    {"text.secondary",     "#8B949E"},
    {"text.tertiary",      "#6E7681"},
    {"overlay.hover",      "rgba(255,255,255,0.08)"},
    {"overlay.hover.subtle","rgba(255,255,255,0.03)"},
    {"overlay.selection",  "rgba(10,132,255,0.22)"},
    {"git.modified",       "#F0883E"},
    {"git.added",          "#3FB950"},
    {"git.deleted",        "#F85149"},
    {"git.untracked",      "#6E7681"},
    {"syntax.editor.background", "#0D1117"},
    {"syntax.editor.foreground", "#E6EDF3"},
    {"syntax.keyword",     "#FF7B72"},
    {"syntax.type",        "#FFA657"},
    {"syntax.function",    "#D2A8FF"},
    {"syntax.string",      "#A5D6FF"},
    {"syntax.comment",     "#8B949E"},
    {"syntax.number",      "#79C0FF"},
    {"syntax.operator",    "#E6EDF3"},
    {"syntax.preprocessor","#F0883E"},
    {"panel.bg",           "#0D1117"},
    {"panel.header",       "#161B22"},
    {"panel.border",       "#30363D"},
    {"badge.bg",           "#0A84FF"},
    {"badge.text",         "#FFFFFF"},
    {"scrollbar.bg",       "transparent"},
    {"scrollbar.handle",   "rgba(255,255,255,0.14)"},
    {"scrollbar.handle.hover","rgba(255,255,255,0.28)"},
    {"input.bg",           "#161B22"},
    {"input.border",       "#30363D"},
};

static const QHash<QString, QString> kStyleTemplates = {
    {"style.activity_bar",
     "QWidget#activityBar { background: $bg.secondary; }"},

    {"style.activity_bar_btn",
     "QPushButton#activityBarBtn { background: transparent;"
     "border: none; border-left: 2px solid transparent;"
     "border-radius: 0; padding: 0; outline: none; }"
     "QPushButton#activityBarBtn:hover { background: $overlay.hover; }"
     "QPushButton#activityBarBtn:pressed { background: $overlay.selection; }"
     "QPushButton#activityBarBtn:focus { outline: none; }"
     "QPushButton#activityBarBtn[active=\"true\"]"
     " { border-left: 2px solid $accent; }"},

    {"style.toolbar",
     "QToolBar { border: none; padding: %space-s% %space-m%; spacing: %space-xs%;"
     "background: transparent; }"
     "QToolBar QToolButton { border: none; border-radius: %radius%;"
     "padding: %space-s% %space-m%;"
     "color: $text.secondary; font-size: %font-sm%; }"
     "QToolBar QToolButton:hover { background: $overlay.hover; color: $text.primary; }"
     "QToolBar QToolButton:pressed { background: $overlay.selection; }"
     "QToolBar::separator { width: 1px; margin: %space-s% %space-m%;"
     "background: transparent; }"},

    {"style.tree_view",
     "QTreeView { border: none; background: $bg.secondary; outline: none;"
     "font-size: 13px; }"
     "QTreeView::item { padding: 5px 10; min-height: 26px; }"
     "QTreeView::item:hover { background: $overlay.hover; }"
     "QTreeView::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTreeView::branch { background: transparent; }"
     "QTreeView::branch:has-siblings:!adjoins-item { border-image: none; }"
     "QTreeView::branch:has-siblings:adjoins-item { border-image: none; }"
     "QTreeView::branch:!has-children:!has-siblings:adjoins-item { border-image: none; }"},

    {"style.splitter",
     "QSplitter::handle { background: transparent; }"
     "QSplitter::handle:hover { background: $border; }"},

    {"style.tab_widget",
     "QTabWidget::pane { border: none; background: $bg.primary; }"
     "QTabWidget::tab-bar { alignment: left; left: 0; }"
     "QTabBar { background: transparent; padding: %space-s% %space-s% 0 %space-s%; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: %space-s% %space-l%; margin: 0 %space-xs%;"
     "border: none; border-radius: %radius%;"
     "font-size: %font-sm%; height: 28px; }"
     "QTabBar::tab:hover { background: $overlay.hover; color: $text.primary; }"
     "QTabBar::tab:selected { background: $overlay.selection; color: $text.primary; }"
     "QTabBar::close-button { subcontrol-position: right;"
     "border-radius: %radius-sm%; padding: 0; margin: 0 0 0 %space-s%; }"
     "QTabBar::close-button:hover { background: $overlay.hover; }"},

    {"style.panel",
     "QWidget { background: $bg.secondary; }"},

    {"style.stack",
     "QStackedWidget { background: $bg.secondary; }"},

    {"style.header",
     "QWidget { background: $bg.secondary; }"
     "QLabel#sidebarTitle { font-size: %font-sm%; font-weight: bold;"
     " background: transparent;"
     " color: $text.secondary; }"
     "QPushButton#sidebarHeaderBtn { border: none; border-radius: %radius-sm%;"
     " background: transparent; }"
     "QPushButton#sidebarHeaderBtn:hover { background: $overlay.hover; }"
     "QPushButton#sidebarHeaderBtn:pressed { background: $overlay.selection; }"},

    {"style.main_window",
     "QMainWindow { background: $bg.primary; }"},

    {"style.explorer_root",
     "QWidget#ExplorerPanel { background: $bg.secondary; }"},

    {"style.menu",
     "QMenu { border: none; border-radius: %radius-lg%; padding: %space-s%;"
     "background: $bg.primary; }"
     "QMenu::item { padding: %space-s% 32px %space-s% %space-l%;"
     "border-radius: %radius-sm%; font-size: %font-ui%; }"
     "QMenu::item:selected { background: $accent; color: $button.text; }"
     "QMenu::item:disabled { color: $text.tertiary; }"
     "QMenu::separator { height: 1px; background: $border;"
     "margin: %space-s% %space-m%; }"},

    {"style.menubar",
     "QMenuBar { background: transparent;"
     "padding: %space-xs% 0; font-size: %font-ui%; }"
     "QMenuBar::item { padding: %space-s% %space-m%; border-radius: %radius%; }"
     "QMenuBar::item:selected { background: $overlay.hover; }"},

    {"style.statusbar",
     "QStatusBar { background: $bg.secondary; color: $text.tertiary;"
     "border: none; font-size: %font-sm%; padding: 0 %space-m%;"
     "min-height: 26px; }"
     "QStatusBar::item { border: none; margin: 0 %space-xs%; }"
     "QStatusBar QLabel { color: $text.tertiary; padding: 0 %space-m%;"
     "border-radius: %radius-sm%; }"
     "QStatusBar QLabel:hover { background: $overlay.hover; color: $text.primary; }"
     "QStatusBar QLabel#statusCursor { color: $text.secondary; font-family: %font-mono%; }"
     "QStatusBar QLabel#statusLang { color: $text.secondary; }"
     "QStatusBar QLabel#statusEncoding { color: $text.secondary; }"
     "QStatusBar QLabel#statusBranch { font-weight: bold; }"},

    {"style.primary_button",
     "QPushButton { background: $accent; color: $button.text; border: none; border-radius: 7px;"
     "padding: 6px 18px; font-size: 13px; font-weight: bold; }"
     "QPushButton:hover { background: $accent.hover; }"
     "QPushButton:pressed { background: $accent.pressed; }"},

    {"style.editor",
     "QPlainTextEdit { background: $syntax.editor.background; color: $syntax.editor.foreground;"
     "border: none; selection-background-color: $overlay.selection;"
     "font-family: %font-mono%; font-size: 13px;"
     "padding: 8px; }"
     "QWidget#lineNumberArea { background: $bg.secondary; color: $text.tertiary;"
     "font-family: %font-mono%; font-size: 12px; }"},

    {"style.http_panel",
     "QWidget#httpClientRoot { background: $bg.secondary; }"
     "QFrame#httpRequestBar { background: $bg.primary;"
     "border: 1px solid $border; border-radius: 8px; }"
     "QFrame#httpResponseBar { background: $bg.tertiary;"
     "border: 1px solid $border; border-radius: 8px; }"},

    {"style.http_url_input",
     "QLineEdit { border: 1px solid $border; border-radius: 8px; padding: 0px 12px;"
     "background: $bg.primary; color: $text.primary; font-size: 13px; }"
     "QLineEdit:focus { border-color: $accent; }"},

    {"style.http_kv_table",
     "QTableWidget { border: 1px solid $border; border-radius: 8px; background: $bg.primary;"
     "color: $text.primary; font-size: 13px; outline: none; }"
     "QTableWidget::item { padding: 0px 8px; border: none; }"
     "QTableWidget::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTableWidget::item:hover { background: $overlay.hover.subtle; }"
     "QHeaderView::section { background: $bg.secondary; color: $text.secondary;"
     "border: none; border-bottom: 1px solid $border; padding: 5px 8px;"
     "font-size: 11px; font-weight: bold; }"
     "QPushButton#httpRemoveRow { border: none; border-radius: 4px; background: transparent;"
     "color: $text.tertiary; font-size: 11px; }"
     "QPushButton#httpRemoveRow:hover { background: $overlay.hover; color: $git.deleted; }"
     "QPushButton#httpRemoveRow:pressed { background: $overlay.selection; }"},

    {"style.http_body_editor",
     "QPlainTextEdit { border: 1px solid $border; border-radius: 8px; background: $bg.primary;"
     "color: $text.primary; font-family: %font-mono%; font-size: 13px;"
     "selection-background-color: $overlay.selection; padding: 8px; }"
     "QPlainTextEdit:focus { border-color: $accent; }"},

    {"style.http_combo",
     "QComboBox { border: 1px solid $border; border-radius: 8px; padding: 0px 8px;"
     "background: $bg.primary; color: $text.primary; font-size: 12px; min-width: 130; }"
     "QComboBox:hover { border-color: $text.tertiary; }"
     "QComboBox:focus { border-color: $accent; }"
     "QComboBox::drop-down { border: none; width: 22px; }"
     "QComboBox::down-arrow { image: url(:/vectors/chevron_down.svg);"
     "width: 12px; height: 12px; }"
     "QComboBox QAbstractItemView { background: $bg.primary; color: $text.primary;"
     "selection-background-color: $overlay.selection; border: 1px solid $border;"
     "outline: none; }"},

    {"style.http_tabs",
     "QTabWidget::pane { border: none; background: transparent; }"
     "QTabBar { background: transparent; border-bottom: 1px solid $border; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: 7px 14px; border: none; border-bottom: 2px solid transparent;"
     "font-size: 13px; font-weight: bold; }"
     "QTabBar::tab:selected { color: $accent; border-bottom: 2px solid $accent; }"
     "QTabBar::tab:hover { color: $text.primary; }"},

    {"style.http_response_body",
     "QPlainTextEdit { border: 1px solid $border; border-radius: 8px; background: $bg.primary;"
     "color: $text.primary; font-family: %font-mono%; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 8px; }"},

    {"style.git_diff",
     "QPlainTextEdit { background: $bg.tertiary; border: none; border-radius: 8px;"
     "color: $text.primary; font-family: %font-mono%; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 10; }"},

    {"style.http_meta_label",
     "QLabel { font-size: 12px; color: $text.secondary; padding: 2px 0; }"},

    {"style.http_state_title",
     "QLabel { font-size: 16px; font-weight: bold; color: $text.secondary; }"},

    {"style.http_state_detail",
     "QLabel { font-size: 13px; color: $text.tertiary; }"},

    {"style.http_cancel_button",
     "QPushButton { background: transparent; color: $git.deleted;"
     "border: 1px solid $git.deleted; border-radius: 7px; padding: 6px 16px;"
     "font-size: 13px; font-weight: bold; }"
     "QPushButton:hover { background: $overlay.selection; }"
     "QPushButton:pressed { background: $overlay.hover; }"},

    {"style.http_ghost_button",
     "QPushButton { background: transparent; color: $text.secondary;"
     "border: 1px solid $border; border-radius: 7px; padding: 4px 12px;"
     "font-size: 12px; }"
     "QPushButton:hover { border-color: $accent; color: $accent; }"
     "QPushButton:pressed { background: $overlay.selection; }"},

    {"style.http_section_label",
     "QLabel { font-size: 11px; font-weight: bold;"
     " color: $text.secondary; }"},

    {"style.hydra_panel",
     "QWidget#hydraRoot { background: $bg.secondary; }"
     "QFrame#hydraTargetBar { background: $bg.primary;"
     "border: 1px solid $border; border-radius: 8px; }"
     "QFrame#hydraSection { background: $bg.primary;"
     "border: 1px solid $border; border-radius: 8px; }"
     "QLabel#hydraSectionLabel { font-size: 11px; font-weight: bold;"
     " color: $text.secondary; }"
     "QLabel#hydraHintLabel { font-size: 11px; color: $text.tertiary; }"
     "QRadioButton { color: $text.primary; font-size: 13px; spacing: 6px; }"
     "QRadioButton::indicator { width: 15px; height: 15px; }""QCheckBox { color: $text.primary; font-size: 13px; spacing: 6px; }"},

    {"style.hydra_status_chip",
     "QLabel#hydraStatusLabel {"
     "border-radius: 11px; padding: 4px 14px;"
     "font-size: 12px; font-weight: bold; }"
     "QLabel#hydraStatusLabel[state='pending']"
     " { background: $text.secondary; color: $button.text; }"
     "QLabel#hydraStatusLabel[state='active']"
     " { background: $accent; color: $button.text; }"
     "QLabel#hydraStatusLabel[state='error']"
     " { background: $git.deleted; color: $button.text; }"
     "QLabel#hydraStatusLabel[state='warn']"
     " { background: $git.modified; color: $button.text; }"
     "QLabel#hydraStatusLabel[state='success']"
     " { background: $git.added; color: $button.text; }"},

    {"style.hydra_spin",
     "QSpinBox { border: 1px solid $border; border-radius: 8px;"
     "background: $bg.primary; color: $text.primary; font-size: 12px;"
     "padding: 2px 6px; }"
     "QSpinBox:focus { border-color: $accent; }"
     "QSpinBox::up-button, QSpinBox::down-button { width: 18px;"
     "border: none; background: transparent; }"
     "QSpinBox::up-arrow { image: url(:/vectors/chevron_up.svg);"
     "width: 10; height: 10; }"
     "QSpinBox::down-arrow { image: url(:/vectors/chevron_down.svg);"
     "width: 10; height: 10; }"},

    {"style.data_view",
     "QWidget#dataViewRoot { background: $bg.secondary; }"
     "QWidget#dataViewToolbar { background: $bg.primary;"
     "border: 1px solid $border; border-radius: 8px; }"
     "QLabel#dataViewPath { font-size: 13px; font-weight: bold; color: $text.primary; }"
     "QLabel#dataViewStatus { font-size: 11px; color: $text.secondary; }"
     "QComboBox#dataViewTableCombo { border: 1px solid $border; border-radius: 8px;"
     "background: $bg.primary; color: $text.primary; font-size: 12px;"
     "padding: 2px 8px; }"
     "QComboBox#dataViewTableCombo::drop-down { border: none; width: 20; }"
     "QComboBox#dataViewTableCombo::down-arrow { image: url(:/vectors/chevron_down.svg);"
     "width: 10; height: 10; }"
     "QPushButton#dataViewRefresh { border: 1px solid $border; border-radius: 7px;"
     "background: transparent; color: $text.primary; font-size: 12px; padding: 4px 12px; }"
     "QPushButton#dataViewRefresh:hover { background: $overlay.hover; }"
     "QPushButton#dataViewRefresh:pressed { background: $overlay.selection; }"
     "QWidget#dataViewSqlPanel { background: $bg.primary;"
     "border-top: 1px solid $border; }"
     "QPlainTextEdit { border: 1px solid $border; border-radius: 8px;"
     "background: $bg.primary; color: $text.primary; font-size: 13px; padding: 6px; }"
     "QPushButton#dataViewRunQuery { border: 1px solid $accent; border-radius: 7px;"
     "background: $accent; color: white; font-size: 12px; padding: 4px 14px; }"
     "QPushButton#dataViewRunQuery:hover { background: $accent.hover; border-color: $accent.hover; }"
     "QPushButton#dataViewRunQuery:pressed { background: $accent.pressed; border-color: $accent.pressed; }"},

    {"style.data_view_table",
     "QTableWidget { border: none; background: $bg.primary;"
     "color: $text.primary; font-size: 13px; outline: none;"
     "alternate-background-color: $bg.tertiary; gridline-color: $border; }"
     "QTableWidget::item { padding: 0px 8px; border: none; }"
     "QTableWidget::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTableWidget::item:hover { background: $overlay.hover.subtle; }"
     "QHeaderView::section { background: $bg.secondary; color: $text.secondary;"
     "border: none; border-bottom: 1px solid $border; padding: 6px 8px;"
     "font-size: 11px; font-weight: bold; }"
     "QTableCornerButton::section { background: $bg.secondary; border: none; }"},

    {"style.panel_container",
     "QWidget#bottomPanelRoot { background: $panel.bg; border-top: 1px solid $panel.border; }"
     "QWidget#bottomPanelHeader { background: $panel.header; border-bottom: 1px solid $panel.border; }"
     "QWidget#bottomPanelTabBar { background: transparent; }"
     "QPushButton#panelTabBtn { background: transparent; color: $text.secondary;"
     "padding: %space-s% %space-l%; border: none; border-bottom: 2px solid transparent;"
     "font-size: %font-sm%; font-weight: bold; }"
     "QPushButton#panelTabBtn:checked { color: $text.primary; border-bottom: 2px solid $accent; }"
     "QPushButton#panelTabBtn:hover { color: $text.primary; background: $overlay.hover; }"
     "QPushButton#panelToolBtn { border: none; border-radius: %radius-sm%;"
     "background: transparent; padding: %space-xs% %space-s%;"
     "color: $text.secondary; font-size: 14px; }"
     "QPushButton#panelToolBtn:hover { background: $overlay.hover; color: $text.primary; }"
     "QWidget#bottomPanelStack { background: $panel.bg; }"},

    {"style.scrollbar",
     "QScrollBar:vertical { background: $scrollbar.bg; width: 8px; margin: 0; }"
     "QScrollBar::handle:vertical { background: $scrollbar.handle; border-radius: %radius-sm%; min-height: 30px; }"
     "QScrollBar::handle:vertical:hover { background: $scrollbar.handle.hover; }"
     "QScrollBar::add-line:vertical { height: 0; }"
     "QScrollBar::sub-line:vertical { height: 0; }"
     "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
     "QScrollBar:horizontal { background: $scrollbar.bg; height: 8px; margin: 0; }"
     "QScrollBar::handle:horizontal { background: $scrollbar.handle; border-radius: %radius-sm%; min-width: 30px; }"
     "QScrollBar::handle:horizontal:hover { background: $scrollbar.handle.hover; }"
     "QScrollBar::add-line:horizontal { width: 0; }"
     "QScrollBar::sub-line:horizontal { width: 0; }"
     "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"},

    {"style.badge",
     "QLabel#activityBadge { background: $badge.bg; color: $badge.text;"
     "border-radius: %radius-pill%; font-size: %font-tiny%; font-weight: bold;"
     "padding: 0 %space-s%; }"},

    {"style.input",
     "QLineEdit, QSpinBox, QDoubleSpinBox { border: 1px solid $input.border;"
     "border-radius: %radius%; padding: 0 %space-m%;"
     "background: $input.bg; color: $text.primary; font-size: %font-ui%;"
     "selection-background-color: $overlay.selection; }"
     "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {"
     "border-color: $accent; background: $bg.primary; }"
     "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {"
     "color: $text.tertiary; }"
     "QLineEdit:read-only { background: $bg.tertiary; }"},

    {"style.git_panel",
     "QWidget#gitPanelRoot { background: $bg.secondary; }"
     "QToolBar { background: $bg.tertiary; border: none; border-bottom: 1px solid $border;"
     "padding: 2px 4px; spacing: 2px; }"
     "QToolBar QToolButton { border: none; border-radius: 4px; padding: 4px;"
     "background: transparent; }"
     "QToolBar QToolButton:hover { background: $overlay.hover; }"
     "QToolBar QToolButton:pressed { background: $overlay.selection; }"
     "QToolBar::separator { width: 1px; margin: 4px 2px; background: $border; }"
     "QLabel#gitBranchLabel { padding: 6px 14px; font-size: 13px; font-weight: bold;"
     "color: $text.primary; background: transparent; border-bottom: 1px solid $border; }"
     "QLabel#gitStatusLabel { padding: 4px 14px; font-size: 11px; color: $text.tertiary;"
     "background: transparent; }"
     "QListWidget#gitFileList { border: none; background: transparent; font-size: 13px;"
     "outline: none; }"
     "QListWidget#gitFileList::item { padding: 4px 12px; min-height: 26px; }"
     "QListWidget#gitFileList::item:hover { background: $overlay.hover; }"
     "QListWidget#gitFileList::item:selected { background: $overlay.selection; color: $text.primary; }"
     "QTabWidget#gitFileTabs { background: transparent; }"
     "QTabWidget#gitFileTabs::pane { border: none; background: transparent; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: 5px 14px; border: none; border-bottom: 2px solid transparent;"
     "font-size: 11px; font-weight: bold; }"
     "QTabBar::tab:selected { color: $text.primary; border-bottom: 2px solid $accent; }"
     "QTabBar::tab:hover { color: $text.primary; }"
     "QPlainTextEdit#gitDiffView { background: $bg.tertiary; border: none;"
     "color: $text.primary; font-family: %font-mono%; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 10px; }"
     "QTextEdit#gitCommitMsg { border: 1px solid $border; border-radius: 8px; padding: 8px;"
     "font-size: 13px; background: $bg.tertiary; color: $text.primary; }"
     "QTextEdit#gitCommitMsg:focus { border-color: $accent; background: $bg.primary; }"
     "QPushButton#gitCommitBtn { background: $accent; color: $button.text;"
     "border: none; border-radius: 7px; padding: 6px 18px; font-size: 13px; font-weight: bold; }"
     "QPushButton#gitCommitBtn:hover { background: $accent.hover; }"
     "QPushButton#gitCommitBtn:pressed { background: $accent.pressed; }"
     "QPushButton#gitCommitBtn:disabled { background: $overlay.hover; color: $text.tertiary; }"
     "QWidget#gitCommitFrame { background: $bg.primary; border-top: 1px solid $border; }"},

    {"style.welcome_modern",
     "QWidget#welcomeModernRoot { background: $bg.primary; }"
     "QLabel#welcomeModernLogo { font-size: 48px; color: $accent;"
     "background: transparent; }"
     "QLabel#welcomeModernTitle { font-size: 28px; font-weight: bold;"
     "color: $text.primary; background: transparent;"
     " }"
     "QLabel#welcomeModernSubtitle { font-size: 14px; color: $text.secondary;"
     "background: transparent; }"
     "QWidget#welcomeCard { background: $bg.secondary;"
     "border: 1px solid $border; border-radius: %radius-lg%; }"
     "QWidget#welcomeCard:hover { border-color: $accent;"
     "background: $bg.tertiary; }"
     "QLabel#welcomeCardIcon { font-size: %font-xl%; background: transparent; }"
     "QLabel#welcomeCardTitle { font-size: %font-ui%; font-weight: bold;"
     "color: $text.primary; background: transparent; }"
     "QLabel#welcomeCardDesc { font-size: %font-sm%; color: $text.secondary;"
     "background: transparent; }"
     "QLabel#welcomeCardShortcut { font-size: %font-sm%; color: $text.tertiary;"
     "background: transparent; padding: 1px %space-s%;"
     "border: 1px solid $border; border-radius: %radius-sm%; }"},

    // ---- 全局底座模板（applyGlobalPalette() 拼进 app stylesheet，为裸奔控件兜底）----

    {"style.combo",
     "QComboBox { border: 1px solid $input.border; border-radius: %radius%;"
     "padding: 0 %space-m%; background: $input.bg; color: $text.primary;"
     "font-size: %font-ui%; min-width: 96px; }"
     "QComboBox:hover { border-color: $text.tertiary; }"
     "QComboBox:focus { border-color: $accent; background: $bg.primary; }"
     "QComboBox:disabled { color: $text.tertiary; }"
     "QComboBox::drop-down { border: none; width: 22px; }"
     "QComboBox::down-arrow { image: url(:/vectors/chevron_down.svg);"
     "width: 12px; height: 12px; }"
     "QComboBox QAbstractItemView { background: $bg.primary; color: $text.primary;"
     "selection-background-color: $overlay.selection; border: 1px solid $border;"
     "outline: none; padding: %space-s%; }"},

    {"style.button",
     "QPushButton { background: transparent; color: $text.primary;"
     "border: 1px solid transparent; border-radius: %radius%;"
     "padding: %space-s% %space-l%; font-size: %font-ui%; }"
     "QPushButton:hover { background: $overlay.hover; }"
     "QPushButton:pressed { background: $overlay.selection; }"
     "QPushButton:disabled { color: $text.tertiary; }"
     "QPushButton:default { background: $accent; color: $button.text;"
     "border-color: $accent; }"
     "QPushButton:default:hover { background: $accent.hover; border-color: $accent.hover; }"
     "QPushButton:default:pressed { background: $accent.pressed; border-color: $accent.pressed; }"
     "QPushButton:default:disabled { background: $overlay.hover; color: $text.tertiary;"
     "border-color: transparent; }"},

    {"style.groupbox",
     "QGroupBox { border: 1px solid $border; border-radius: %radius%;"
     "margin-top: %space-l%; background: transparent;"
     "padding: %space-m%; }"
     "QGroupBox::title { subcontrol-origin: margin; left: %space-m%;"
     "top: -%space-xs%; padding: 0 %space-s%;"
     "color: $text.secondary; font-size: %font-sm%; font-weight: bold;"
     "background: transparent; }"},

    {"style.checkbox",
     "QCheckBox, QRadioButton { color: $text.primary; font-size: %font-ui%;"
     "spacing: %space-s%; background: transparent; }"
     "QCheckBox:disabled, QRadioButton:disabled { color: $text.tertiary; }"},

    {"style.dialog",
     "QDialog { background: $bg.primary; }"
     "QDialog QLabel, QMessageBox QLabel { background: transparent;"
     "color: $text.primary; font-size: %font-ui%; }"},

    {"style.tooltip",
     "QToolTip { background: $bg.tertiary; color: $text.primary;"
     "border: 1px solid $border; border-radius: %radius%;"
     "padding: %space-s% %space-m%; font-size: %font-sm%; }"},

    {"style.editor_base",
     "QPlainTextEdit, QTextEdit { background: $bg.primary; color: $text.primary;"
     "selection-background-color: $overlay.selection;"
     "border: 1px solid $border; border-radius: %radius%; }"},

    // ---- 面板级模板（P4 归口：原内联 QSS 全部移入此处）----

    {"style.http_method_btn",
     "QPushButton#httpMethodButton { border: none; border-radius: %radius%;"
     "padding: 5px %space-l%; font-size: %font-sm%; font-weight: bold;"
     "color: $button.text; background: $text.tertiary; }"
     "QPushButton#httpMethodButton[method=\"GET\"] { background: $git.added; }"
     "QPushButton#httpMethodButton[method=\"POST\"] { background: $accent; }"
     "QPushButton#httpMethodButton[method=\"PUT\"],"
     "QPushButton#httpMethodButton[method=\"PATCH\"] { background: $git.modified; }"
     "QPushButton#httpMethodButton[method=\"DELETE\"] { background: $git.deleted; }"},

    {"style.http_status_pill",
     "QLabel#httpStatusPill { border-radius: %radius-pill%;"
     "padding: %space-s% %space-l%; font-size: %font-ui%; font-weight: bold;"
     "background: $text.secondary; color: $button.text; }"
     "QLabel#httpStatusPill[state=\"ok\"] { background: $git.added; }"
     "QLabel#httpStatusPill[state=\"redirect\"] { background: $accent; }"
     "QLabel#httpStatusPill[state=\"client_err\"] { background: $git.modified; }"
     "QLabel#httpStatusPill[state=\"server_err\"],"
     "QLabel#httpStatusPill[state=\"error\"] { background: $git.deleted; }"},

    {"style.terminal_panel",
     "QWidget#terminalPanelRoot { background: $bg.primary; }"},

    {"style.hex_view",
     "QAbstractScrollArea#hexView { background: $bg.primary; }"},

    {"style.hex_gobar",
     "QWidget#hexGoBar { background: $panel.header;"
     "border-top: 1px solid $panel.border; min-height: 32px; }"
     "QLineEdit#hexGoEdit { border: 1px solid $input.border;"
     "border-radius: %radius%; padding: 0 %space-m%;"
     "background: $input.bg; color: $text.primary; font-size: %font-sm%; }"
     "QLineEdit#hexGoEdit:focus { border-color: $accent; background: $bg.primary; }"
     "QPushButton#hexGoButton { background: transparent; color: $text.secondary;"
     "border: 1px solid $input.border; border-radius: %radius%;"
     "padding: %space-xs% %space-m%; font-size: %font-sm%; }"
     "QPushButton#hexGoButton:hover { border-color: $accent; color: $accent; }"
     "QLabel#hexPosLabel { color: $text.secondary; font-size: %font-sm%;"
     "background: transparent; }"},

    {"style.output_log",
     "QPlainTextEdit { background: $bg.primary; color: $text.primary;"
     "border: none; selection-background-color: $overlay.selection;"
     "font-family: %font-mono%; font-size: %font-ui%; padding: %space-m%; }"},
};

ThemeService &ThemeService::instance()
{
    static ThemeService svc;
    return svc;
}

void ThemeService::initialize(IDETheme theme)
{
    const auto resolved = resolveTheme(theme);
    current_theme_ = resolved;
    loadXml(xmlPath(resolved));
    rebuildStyles();
    applyGlobalPalette();
}

void ThemeService::applyTheme(IDETheme theme)
{
    const auto resolved = resolveTheme(theme);
    if (resolved == current_theme_) return;

    current_theme_ = resolved;
    loadXml(xmlPath(resolved));
    rebuildStyles();
    applyGlobalPalette();

    NezhaIDE::Configuration::instance().set_theme(resolved);
    NezhaIDE::Configuration::instance().save();

    emit themeChanged(resolved);
}

QString ThemeService::color(const QString &key) const
{
    return colors_.value(key);
}

QColor ThemeService::qcolor(const QString &key) const
{
    const auto val = colors_.value(key);
    if (val.isEmpty()) return {};
    return QColor(val);
}

QString ThemeService::qss(const QString &key) const
{
    return styles_.value(key);
}

QHash<QString, QColor> ThemeService::syntaxColors() const
{
    QHash<QString, QColor> result;
    for (auto it = colors_.cbegin(); it != colors_.cend(); ++it) {
        if (it.key().startsWith(QStringLiteral("syntax."))) {
            result[it.key()] = QColor(it.value());
        }
    }
    return result;
}

IDETheme ThemeService::currentTheme() const noexcept
{
    return current_theme_;
}

QString ThemeService::themeId(IDETheme theme)
{
    switch (theme) {
    case IDETheme::Light:
    case IDETheme::Auto:
        return QStringLiteral("default");
    case IDETheme::Dark:
        return QStringLiteral("nezha_cyber");
    case IDETheme::GitHub:
        return QStringLiteral("github");
    case IDETheme::Xcode:
        return QStringLiteral("xcode");
    }
    return QStringLiteral("default");
}

QString ThemeService::xmlPath(IDETheme theme)
{
#ifdef NEZHA_PROJECT_ROOT
    const auto dir = QStringLiteral(NEZHA_PROJECT_ROOT) + QStringLiteral("/resources/themes");
#else
    const auto dir = QCoreApplication::applicationDirPath() + QStringLiteral("/resources/themes");
#endif
    return dir + QStringLiteral("/") + themeId(theme) + QStringLiteral(".xml");
}

IDETheme ThemeService::resolveTheme(IDETheme theme)
{
    if (theme != IDETheme::Auto) return theme;

    const auto bg = QApplication::palette().color(QPalette::Window);
    return bg.lightness() < 128 ? IDETheme::Dark : IDETheme::Light;
}

void ThemeService::loadXml(const QString &path)
{
    colors_.clear();

    const auto &fallback = (current_theme_ == IDETheme::Dark) ? kFallbackColorsDark : kFallbackColors;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "无法打开主题文件: {}，使用回退调色板",
            path.toStdString()
        );
        for (auto it = fallback.cbegin(); it != fallback.cend(); ++it) {
            colors_[it.key()] = it.value();
        }
        return;
    }

    QXmlStreamReader xml(&file);
    bool in_syntax = false;
    while (!xml.atEnd() && !xml.hasError()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == QStringLiteral("color")) {
                const auto key = xml.attributes().value("key").toString();
                const auto value = xml.readElementText();
                if (!key.isEmpty()) {
                    colors_[key] = value;
                }
            } else if (xml.name() == QStringLiteral("syntax")) {
                in_syntax = true;
            } else if (in_syntax && xml.name() == QStringLiteral("token")) {
                const auto name = xml.attributes().value("name").toString();
                const auto value = xml.readElementText();
                if (!name.isEmpty()) {
                    colors_[QStringLiteral("syntax.") + name] = value;
                }
            }
        } else if (token == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("syntax")) {
            in_syntax = false;
        }
    }

    if (xml.hasError()) {
        Utilities::Logger::instance().log(
            Utilities::LogLevel::Warn,
            __FILE__, __LINE__, __func__,
            "主题 XML 解析错误: {}",
            xml.errorString().toStdString()
        );
    }

    // Merge fallback colors for any tokens not defined in the XML
    const auto &fb2 = (current_theme_ == IDETheme::Dark) ? kFallbackColorsDark : kFallbackColors;
    for (auto it = fb2.cbegin(); it != fb2.cend(); ++it) {
        if (!colors_.contains(it.key())) {
            colors_[it.key()] = it.value();
        }
    }
}

void ThemeService::rebuildStyles()
{
    styles_.clear();

    // key 按长度降序替换：避免 $accent 前缀吞噬 $accent.hover（留下 ".hover" 残渣）
    QStringList colorKeys;
    for (auto ci = colors_.cbegin(); ci != colors_.cend(); ++ci) colorKeys << ci.key();
    std::sort(colorKeys.begin(), colorKeys.end(),
              [](const QString &a, const QString &b) { return a.size() > b.size(); });

    for (auto it = kStyleTemplates.cbegin(); it != kStyleTemplates.cend(); ++it) {
        QString qss = it.value();
        for (const auto &key : colorKeys) {
            qss.replace(QStringLiteral("$") + key, colors_.value(key));
        }
        // 几何 token 二次替换（圆角/间距/字号/字体栈，与主题无关）
        for (auto ti = Tokens::qssTokenTable().cbegin(); ti != Tokens::qssTokenTable().cend(); ++ti) {
            qss.replace(QStringLiteral("%") + ti.key() + QStringLiteral("%"), ti.value());
        }
        styles_[it.key()] = qss;
    }
}

void ThemeService::applyGlobalPalette()
{
    auto *app = qApp;
    if (!app) return;

    const auto bgPrimary = qcolor("bg.primary");
    const auto bgSecondary = qcolor("bg.secondary");
    const auto bgTertiary = qcolor("bg.tertiary");
    const auto textPrimary = qcolor("text.primary");
    const auto textSecondary = qcolor("text.secondary");
    const auto textTertiary = qcolor("text.tertiary");
    const auto accent = qcolor("accent");
    const auto border = qcolor("border");
    const auto buttonText = qcolor("button.text");

    if (!bgPrimary.isValid()) return;

    QPalette p;
    p.setColor(QPalette::Window, bgPrimary);
    p.setColor(QPalette::WindowText, textPrimary);
    p.setColor(QPalette::Base, bgPrimary);
    p.setColor(QPalette::AlternateBase, bgSecondary);
    p.setColor(QPalette::ToolTipBase, bgTertiary);
    p.setColor(QPalette::ToolTipText, textPrimary);
    p.setColor(QPalette::Text, textPrimary);
    p.setColor(QPalette::Button, bgSecondary);
    p.setColor(QPalette::ButtonText, textPrimary);
    p.setColor(QPalette::BrightText, accent);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::LinkVisited, accent);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, buttonText.isValid() ? buttonText : QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, textTertiary);
    p.setColor(QPalette::Mid, border);
    p.setColor(QPalette::Dark, border);
    p.setColor(QPalette::Shadow, QColor(0, 0, 0, 60));

    p.setColor(QPalette::Disabled, QPalette::WindowText, textTertiary);
    p.setColor(QPalette::Disabled, QPalette::Text, textTertiary);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, textTertiary);

    app->setPalette(p);

    // 全局底座 QSS：为未按面板分发样式的原生控件（对话框/消息框/输入框/
    // 滚动条/按钮等）兜底。组件级 setStyleSheet 优先级更高，可局部覆盖。
    static const char *kGlobalTemplateKeys[] = {
        "style.menu", "style.menubar", "style.tooltip",
        "style.scrollbar", "style.input", "style.combo",
        "style.button", "style.groupbox", "style.checkbox",
        "style.dialog", "style.editor_base",
    };

    QString globalQss;
    for (const auto *key : kGlobalTemplateKeys) {
        const auto qss = styles_.value(QLatin1String(key));
        if (!qss.isEmpty()) globalQss += qss + QStringLiteral("\n");
    }

    app->setStyleSheet(globalQss);
}

} // namespace NezhaIDE::Services
