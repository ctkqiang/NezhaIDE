#include "theme_service.h"
#include "src/configuration.h"
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
    {"bg.secondary",       "#F5F6F7"},
    {"bg.tertiary",        "#FAFBFC"},
    {"border",             "#E5E6EB"},
    {"accent",             "#3370FF"},
    {"accent.hover",       "#245BDB"},
    {"accent.pressed",     "#1A4AB3"},
    {"accent.subtle",      "#E8F0FE"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#1F2329"},
    {"text.secondary",     "#646A73"},
    {"text.tertiary",      "#8F959E"},
    {"overlay.hover",      "rgba(31,35,41,0.06)"},
    {"overlay.hover.subtle","rgba(31,35,41,0.03)"},
    {"overlay.selection",  "rgba(51,112,255,0.10)"},
    {"git.modified",       "#FF8800"},
    {"git.added",          "#00B42A"},
    {"git.deleted",        "#F53F3F"},
    {"git.untracked",      "#86909C"},
    {"syntax.editor.background", "#FFFFFF"},
    {"syntax.editor.foreground", "#1F2329"},
    {"syntax.keyword",     "#D92C7B"},
    {"syntax.type",        "#00A47C"},
    {"syntax.function",    "#3370FF"},
    {"syntax.string",      "#00A870"},
    {"syntax.comment",     "#86909C"},
    {"syntax.number",      "#D9720A"},
    {"syntax.operator",    "#1F2329"},
    {"syntax.preprocessor","#7B3BED"},
    {"panel.bg",           "#FFFFFF"},
    {"panel.header",       "#F5F6F7"},
    {"panel.border",       "#E5E6EB"},
    {"badge.bg",           "#3370FF"},
    {"badge.text",         "#FFFFFF"},
    {"scrollbar.bg",       "transparent"},
    {"scrollbar.handle",   "rgba(0,0,0,0.18)"},
    {"scrollbar.handle.hover","rgba(0,0,0,0.32)"},
    {"input.bg",           "#F5F6F7"},
    {"input.border",       "#E5E6EB"},
};

static const QHash<QString, QString> kFallbackColorsDark = {
    {"bg.primary",         "#1A1A2E"},
    {"bg.secondary",       "#16163A"},
    {"bg.tertiary",        "#1E1E3A"},
    {"border",             "#2D2D4A"},
    {"accent",             "#3370FF"},
    {"accent.hover",       "#4C84FF"},
    {"accent.pressed",     "#245BDB"},
    {"accent.subtle",      "rgba(51,112,255,0.18)"},
    {"button.text",        "#FFFFFF"},
    {"text.primary",       "#E8EAED"},
    {"text.secondary",     "#9AA0A6"},
    {"text.tertiary",      "#6B7280"},
    {"overlay.hover",      "rgba(255,255,255,0.06)"},
    {"overlay.hover.subtle","rgba(255,255,255,0.03)"},
    {"overlay.selection",  "rgba(51,112,255,0.20)"},
    {"git.modified",       "#FF8800"},
    {"git.added",          "#00B42A"},
    {"git.deleted",        "#F53F3F"},
    {"git.untracked",      "#6B7280"},
    {"syntax.editor.background", "#1A1A2E"},
    {"syntax.editor.foreground", "#E8EAED"},
    {"syntax.keyword",     "#FF6B9D"},
    {"syntax.type",        "#4EC9B0"},
    {"syntax.function",    "#6C9FFF"},
    {"syntax.string",      "#98C379"},
    {"syntax.comment",     "#6B7280"},
    {"syntax.number",      "#FFB86C"},
    {"syntax.operator",    "#E8EAED"},
    {"syntax.preprocessor","#C084FC"},
    {"panel.bg",           "#16163A"},
    {"panel.header",       "#1A1A2E"},
    {"panel.border",       "#2D2D4A"},
    {"badge.bg",           "#3370FF"},
    {"badge.text",         "#FFFFFF"},
    {"scrollbar.bg",       "transparent"},
    {"scrollbar.handle",   "rgba(255,255,255,0.14)"},
    {"scrollbar.handle.hover","rgba(255,255,255,0.28)"},
    {"input.bg",           "#1E1E3A"},
    {"input.border",       "#2D2D4A"},
};

static const QHash<QString, QString> kStyleTemplates = {
    {"style.activity_bar",
     "QWidget#activityBar { background: $bg.secondary; }"},

    {"style.activity_bar_btn",
     "QPushButton#activityBarBtn { background: transparent;"
     "border: none; border-left: 2px solid transparent;"
     "border-radius: 0; padding: 0; }"
     "QPushButton#activityBarBtn:hover { background: $overlay.hover; }"
     "QPushButton#activityBarBtn[active=\"true\"]"
     " { border-left: 2px solid $accent; }"},

    {"style.toolbar",
     "QToolBar { border: none; padding: 4px 6px; spacing: 2px;"
     "background: transparent; }"
     "QToolBar QToolButton { border: none; border-radius: 6px; padding: 4px 8px;"
     "color: $text.secondary; font-size: 12px; }"
     "QToolBar QToolButton:hover { background: $overlay.hover; color: $text.primary; }"
     "QToolBar QToolButton:pressed { background: $overlay.selection; }"
     "QToolBar::separator { width: 1px; margin: 4px 6px; background: transparent; }"},

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

    {"style.list_widget",
     "QListWidget { border: none; background: transparent; font-size: 13px; }"
     "QListWidget::item { padding: 5px 14px; min-height: 26px; }"
     "QListWidget::item:hover { background: $overlay.hover; }"
     "QListWidget::item:selected { background: $overlay.selection; color: $text.primary; }"},

    {"style.splitter",
     "QSplitter::handle { background: transparent; }"
     "QSplitter::handle:hover { background: $border; }"},

    {"style.tab_widget",
     "QTabWidget::pane { border: none; background: $bg.primary; }"
     "QTabWidget::tab-bar { alignment: left; left: 0; }"
     "QTabBar { background: transparent; padding: 4px 4px 0 4px; }"
     "QTabBar::tab { background: transparent; color: $text.secondary;"
     "padding: 6px 14px; margin: 0 2px;"
     "border: none; border-radius: 6px;"
     "font-size: 12px; height: 24px; }"
     "QTabBar::tab:hover { background: $overlay.hover; color: $text.primary; }"
     "QTabBar::tab:selected { background: $overlay.selection; color: $text.primary; }"
     "QTabBar::close-button { subcontrol-position: right;"
     "border-radius: 8px; padding: 0; margin: 0 0 0 4px; }"
     "QTabBar::close-button:hover { background: $overlay.hover; }"},

    {"style.panel",
     "QWidget { background: $bg.secondary; }"},

    {"style.stack",
     "QStackedWidget { background: $bg.secondary; }"},

    {"style.header",
     "QWidget { background: $bg.secondary; }"
     "QLabel#sidebarTitle { font-size: 11px; font-weight: bold;"
     " background: transparent;"
     " color: $text.secondary; }"
     "QPushButton#sidebarHeaderBtn { border: none; border-radius: 5px;"
     " background: transparent; }"
     "QPushButton#sidebarHeaderBtn:hover { background: $overlay.hover; }"},

    {"style.main_window",
     "QMainWindow { background: $bg.primary; }"},

    {"style.explorer_root",
     "QWidget#ExplorerPanel { background: $bg.secondary; }"},

    {"style.tab_active",
     "QPushButton { background: transparent; color: $text.primary;"
     "border: none; border-bottom: 2px solid $accent;"
     "padding: 8px 12px; font-size: 11px; font-weight: bold;"
     " }"},

    {"style.tab_inactive",
     "QPushButton { background: transparent; color: $text.tertiary;"
     "border: none; border-bottom: 2px solid transparent;"
     "padding: 8px 12px; font-size: 11px;"
     " }"
     "QPushButton:hover { color: $text.secondary; }"},

    {"style.menu",
     "QMenu { border: none; border-radius: 8px; padding: 4px;"
     "background: $bg.primary; }"
     "QMenu::item { padding: 6px 32px 6px 12px; border-radius: 4px;"
     "font-size: 13px; }"
     "QMenu::item:selected { background: $accent; color: $button.text; }"
     "QMenu::separator { height: 1px; background: $border; margin: 4px 8px; }"},

    {"style.menubar",
     "QMenuBar { background: transparent;"
     "padding: 2px 0; font-size: 13px; }"
     "QMenuBar::item { padding: 4px 10; border-radius: 6px; }"
     "QMenuBar::item:selected { background: $overlay.hover; }"},

    {"style.statusbar",
     "QStatusBar { background: $bg.secondary; color: $text.tertiary;"
     "border: none; font-size: 11px; padding: 0px 8px; min-height: 26px; }"
     "QStatusBar::item { border: none; }"
     "QStatusBar QLabel { color: $text.tertiary; padding: 0px 8px; border-radius: 3px; }"
     "QStatusBar QLabel:hover { background: $overlay.hover; color: $text.primary; }"
     "QStatusBar QLabel#statusCursor { color: $text.secondary; font-family: Menlo; }"
     "QStatusBar QLabel#statusLang { color: $text.secondary; }"
     "QStatusBar QLabel#statusEncoding { color: $text.secondary; }"
     "QStatusBar QLabel#statusBranch { font-weight: bold; }"},

    {"style.branch_label",
     "QLabel { padding: 6px 12px; font-size: 12px; font-weight: bold;"
     "color: $text.primary; background: transparent; }"},

    {"style.commit_frame",
     "QWidget { background: transparent; padding: 8px; }"},

    {"style.commit_input",
     "QTextEdit { border: 1px solid $border; border-radius: 8px; padding: 8px;"
     "font-size: 13px; background: $bg.tertiary; }"
     "QTextEdit:focus { border-color: $accent; background: $bg.primary; }"},

    {"style.primary_button",
     "QPushButton { background: $accent; color: $button.text; border: none; border-radius: 7px;"
     "padding: 6px 18px; font-size: 13px; font-weight: bold; }"
     "QPushButton:hover { background: $accent.hover; }"
     "QPushButton:pressed { background: $accent.pressed; }"},

    {"style.status_label",
     "QLabel { padding: 4px 12px; font-size: 11px; color: $text.tertiary;"
     "background: transparent; }"},

    {"style.editor",
     "QPlainTextEdit { background: $syntax.editor.background; color: $syntax.editor.foreground;"
     "border: none; selection-background-color: $overlay.selection;"
     "font-family: Menlo; font-size: 13px;"
     "padding: 8px; }"
     "QWidget#lineNumberArea { background: $bg.secondary; color: $text.tertiary;"
     "font-family: Menlo; font-size: 12px; }"},

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
     "color: $text.primary; font-family: Menlo; font-size: 13px;"
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
     "color: $text.primary; font-family: Menlo; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 8px; }"},

    {"style.git_diff",
     "QPlainTextEdit { background: $bg.tertiary; border: none; border-radius: 8px;"
     "color: $text.primary; font-family: Menlo; font-size: 12px;"
     "selection-background-color: $overlay.selection; padding: 10; }"},

    {"style.dock_widget",
     "QDockWidget { background: $bg.secondary; }"
     "QDockWidget::title { background: $bg.secondary; color: $text.secondary;"
     "border: none; border-bottom: 1px solid $border; padding: 5px 12px;"
     "font-size: 12px; font-weight: bold; }"},

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

    {"style.welcome_root",
     "QWidget#welcomeRoot { background: transparent; }"
     "QLabel#welcomeTitle { font-size: 26px; font-weight: bold;"
     " color: $text.primary; background: transparent; }"
     "QLabel#welcomeVersion { font-size: 13px; color: $text.tertiary;"
     " background: transparent; }"
     "QLabel#welcomeStartLabel { font-size: 11px; font-weight: bold;"
     ""
     " color: $text.secondary; background: transparent; }"
     "QWidget#welcomeActionRow { background: transparent;"
     " border-radius: 6px; }"
     "QWidget#welcomeActionRow:hover { background: $overlay.hover; }"
     "QLabel#welcomeActionLabel { font-size: 13px; color: $text.primary;"
     " background: transparent; }"
     "QLabel#welcomeActionKey { font-size: 11px; color: $text.tertiary;"
     " background: transparent; padding: 1px 6px;"
     " border: 1px solid $border; border-radius: 3px; }"},

    {"style.panel_container",
     "QWidget#bottomPanelRoot { background: $panel.bg; border-top: 1px solid $panel.border; }"
     "QWidget#bottomPanelHeader { background: $panel.header; border-bottom: 1px solid $panel.border; }"
     "QWidget#bottomPanelTabBar { background: transparent; }"
     "QPushButton#panelTabBtn { background: transparent; color: $text.secondary;"
     "padding: 4px 14px; border: none; border-bottom: 2px solid transparent;"
     "font-size: 11px; font-weight: bold;  }"
     "QPushButton#panelTabBtn:checked { color: $text.primary; border-bottom: 2px solid $accent; }"
     "QPushButton#panelTabBtn:hover { color: $text.primary; }"
     "QPushButton#panelToolBtn { border: none; border-radius: 4px;"
     "background: transparent; padding: 2px 6px; color: $text.secondary; font-size: 14px; }"
     "QPushButton#panelToolBtn:hover { background: $overlay.hover; color: $text.primary; }"
     "QWidget#bottomPanelStack { background: $panel.bg; }"},

    {"style.scrollbar",
     "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }"
     "QScrollBar::handle:vertical { background: $scrollbar.handle; border-radius: 4px; min-height: 30; }"
     "QScrollBar::handle:vertical:hover { background: $scrollbar.handle.hover; }"
     "QScrollBar::add-line:vertical { height: 0; }"
     "QScrollBar::sub-line:vertical { height: 0; }"
     "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
     "QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }"
     "QScrollBar::handle:horizontal { background: $scrollbar.handle; border-radius: 4px; min-width: 30; }"
     "QScrollBar::handle:horizontal:hover { background: $scrollbar.handle.hover; }"
     "QScrollBar::add-line:horizontal { width: 0; }"
     "QScrollBar::sub-line:horizontal { width: 0; }"
     "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"},

    {"style.badge",
     "QLabel#activityBadge { background: $badge.bg; color: $badge.text; border-radius: 8px; font-size: 10; font-weight: bold; padding: 0px 5px; }"},

    {"style.breadcrumb",
     "QWidget#breadcrumbBar { background: $panel.header;"
     "border-bottom: 1px solid $panel.border; min-height: 26px; }"
     "QLabel#breadcrumbItem { font-size: 12px; color: $text.secondary;"
     "padding: 2px 4px; background: transparent; border-radius: 3px; }"
     "QLabel#breadcrumbItem:hover { background: $overlay.hover; color: $text.primary; }"
     "QLabel#breadcrumbSep { color: $text.tertiary; font-size: 11px;"
     "padding: 0 2px; background: transparent; }"},

    {"style.input",
     "QLineEdit { border: 1px solid $input.border; border-radius: 6px;"
     "padding: 0 10; background: $input.bg; color: $text.primary;"
     "font-size: 13px; }"
     "QLineEdit:focus { border-color: $accent; background: $bg.primary; }"},

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
     "color: $text.primary; font-family: Menlo; font-size: 12px;"
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
     "border: 1px solid $border; border-radius: 10; }"
     "QWidget#welcomeCard:hover { border-color: $accent;"
     "background: $bg.tertiary; }"
     "QLabel#welcomeCardTitle { font-size: 13px; font-weight: bold;"
     "color: $text.primary; background: transparent; }"
     "QLabel#welcomeCardDesc { font-size: 12px; color: $text.secondary;"
     "background: transparent; }"
     "QLabel#welcomeCardShortcut { font-size: 11px; color: $text.tertiary;"
     "background: transparent; padding: 1px 6px;"
     "border: 1px solid $border; border-radius: 3px; }"},
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

    for (auto it = kStyleTemplates.cbegin(); it != kStyleTemplates.cend(); ++it) {
        QString qss = it.value();
        for (auto ci = colors_.cbegin(); ci != colors_.cend(); ++ci) {
            qss.replace(QStringLiteral("$") + ci.key(), ci.value());
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

    const auto menuQss = styles_.value(QStringLiteral("style.menu"));

    QString globalQss;
    if (!menuQss.isEmpty()) globalQss += menuQss + QStringLiteral("\n");

    globalQss += QStringLiteral(
        "QToolTip {"
        "  background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 4px; padding: 4px 8px; font-size: 12px; }"
    ).arg(bgTertiary.name(), textPrimary.name(), border.name());

    app->setStyleSheet(globalQss);
}

} // namespace NezhaIDE::Services
