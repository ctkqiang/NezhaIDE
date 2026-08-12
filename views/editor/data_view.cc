#include "data_view.h"
#include "simple_highlighter.h"
#include "src/services/database_helper.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

namespace {

QStringList parseCsvLine(const QString &line)
{
    QStringList fields;
    QString field;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.append(field.trimmed());
            field.clear();
        } else {
            field += c;
        }
    }
    fields.append(field.trimmed());
    return fields;
}

} // namespace

DataView::DataView(const QString &filePath, QWidget *parent)
    : QWidget(parent)
    , file_path_(filePath)
{
    setObjectName(QStringLiteral("dataViewRoot"));
    setupUI();
}

DataView::~DataView() = default;

bool DataView::load()
{
    const auto suffix = QFileInfo(file_path_).suffix().toLower();
    if (suffix == QStringLiteral("db") || suffix == QStringLiteral("sqlite")
        || suffix == QStringLiteral("sqlite3")) {
        mode_ = Mode::Database;
        return loadDatabase();
    }
    if (suffix == QStringLiteral("csv")) {
        mode_ = Mode::Csv;
        return loadCsv();
    }
    return false;
}

void DataView::setupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("dataViewToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(10, 8, 10, 8);
    toolbarLayout->setSpacing(8);

    path_label_ = new QLabel(QFileInfo(file_path_).fileName(), toolbar);
    path_label_->setObjectName(QStringLiteral("dataViewPath"));
    path_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    toolbarLayout->addWidget(path_label_);
    toolbarLayout->addStretch();

    table_combo_ = new QComboBox(toolbar);
    table_combo_->setObjectName(QStringLiteral("dataViewTableCombo"));
    toolbarLayout->addWidget(table_combo_);

    refresh_btn_ = new QPushButton(LOC("data_view.refresh"), toolbar);
    refresh_btn_->setObjectName(QStringLiteral("dataViewRefresh"));
    toolbarLayout->addWidget(refresh_btn_);

    row_status_ = new QLabel(toolbar);
    row_status_->setObjectName(QStringLiteral("dataViewStatus"));
    toolbarLayout->addWidget(row_status_);

    layout->addWidget(toolbar);

    main_splitter_ = new QSplitter(Qt::Vertical, this);
    main_splitter_->setHandleWidth(1);
    main_splitter_->setChildrenCollapsible(false);

    table_ = new QTableWidget(main_splitter_);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(false);
    table_->horizontalHeader()->setStretchLastSection(true);

    setupSqlPanel();
    sql_panel_->hide();

    main_splitter_->addWidget(table_);
    main_splitter_->addWidget(sql_panel_);
    main_splitter_->setSizes({400, 140});
    layout->addWidget(main_splitter_, 1);

    connect(refresh_btn_, &QPushButton::clicked, this, &DataView::refreshCurrentTable);
    applyTheme();
}

void DataView::setupSqlPanel()
{
    sql_panel_ = new QWidget(main_splitter_);
    sql_panel_->setObjectName(QStringLiteral("dataViewSqlPanel"));
    auto *panelLayout = new QVBoxLayout(sql_panel_);
    panelLayout->setContentsMargins(10, 8, 10, 10);
    panelLayout->setSpacing(6);

    sql_edit_ = new QPlainTextEdit(sql_panel_);
    sql_edit_->setPlaceholderText(LOC("data_view.sql_placeholder"));
    sql_edit_->setMaximumHeight(96);
    new NezhaIDE::Editor::SimpleHighlighter(
        NezhaIDE::Editor::languageSql(), sql_edit_->document(), sql_edit_);

    run_btn_ = new QPushButton(LOC("data_view.run_query"), sql_panel_);
    run_btn_->setObjectName(QStringLiteral("dataViewRunQuery"));
    connect(run_btn_, &QPushButton::clicked, this, &DataView::onRunQuery);

    panelLayout->addWidget(sql_edit_);
    panelLayout->addWidget(run_btn_, 0, Qt::AlignRight);
}

bool DataView::loadDatabase()
{
    db_helper_ = std::make_unique<NezhaIDE::Services::DatabaseHelper>(file_path_.toStdString());
    auto connectResult = db_helper_->initializeDatabase();
    if (!connectResult) {
        row_status_->setText(LOC("data_view.load_error")
                                 .arg(QString::fromStdString(connectResult.error().Message)));
        return false;
    }

    auto q = db_helper_->Prepare(
        "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
    if (!q) {
        row_status_->setText(LOC("data_view.load_error")
                                 .arg(QString::fromStdString(q.error().Message)));
        return false;
    }

    table_names_.clear();
    while (true) {
        auto step = q->Step();
        if (!step || !*step) break;
        table_names_.append(QString::fromUtf8(q->ColumnText(0).c_str()));
    }

    if (table_names_.isEmpty()) {
        row_status_->setText(LOC("data_view.no_tables"));
        return false;
    }

    table_combo_->addItems(table_names_);
    connect(table_combo_, &QComboBox::currentIndexChanged,
            this, &DataView::onTableSelected);

    sql_panel_->show();
    table_combo_->show();
    loadTableData(table_names_.first());
    emit titleChanged(QFileInfo(file_path_).fileName());
    return true;
}

bool DataView::loadCsv()
{
    QFile file(file_path_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        row_status_->setText(LOC("data_view.load_error").arg(file.errorString()));
        return false;
    }

    const auto lines = QString::fromUtf8(file.readAll())
                           .split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        row_status_->setText(LOC("data_view.load_error").arg(QStringLiteral("empty")));
        return false;
    }

    const auto headers = parseCsvLine(lines.first());
    table_->setColumnCount(headers.size());
    table_->setHorizontalHeaderLabels(headers);

    const int rows = static_cast<int>(
        std::min(static_cast<qsizetype>(kMaxRows), lines.size() - 1));
    table_->setRowCount(rows);
    for (int r = 0; r < rows; ++r) {
        const auto fields = parseCsvLine(lines[r + 1]);
        for (int c = 0; c < headers.size(); ++c) {
            table_->setItem(r, c,
                new QTableWidgetItem(c < fields.size() ? fields[c] : QString()));
        }
    }

    table_combo_->hide();
    row_status_->setText(LOC("data_view.csv_loaded").arg(rows));
    emit titleChanged(QFileInfo(file_path_).fileName());
    return true;
}

int DataView::populateRows(const QString &sql)
{
    auto q = db_helper_->Prepare(sql.toStdString());
    if (!q) {
        row_status_->setText(LOC("data_view.query_error")
                                 .arg(QString::fromStdString(q.error().Message)));
        return -1;
    }

    table_->clear();
    const int cols = q->ColumnCount();
    table_->setColumnCount(cols);
    QStringList headers;
    for (int c = 0; c < cols; ++c) {
        const auto *name = sqlite3_column_name(q->Handle(), c);
        headers.append(name ? QString::fromUtf8(name) : QStringLiteral("col%1").arg(c + 1));
    }
    table_->setHorizontalHeaderLabels(headers);
    table_->setRowCount(0);

    int shown = 0;
    while (shown < kMaxRows) {
        auto step = q->Step();
        if (!step || !*step) break;
        table_->insertRow(shown);
        for (int c = 0; c < cols; ++c) {
            table_->setItem(shown, c,
                new QTableWidgetItem(QString::fromStdString(q->ColumnText(c))));
        }
        ++shown;
    }
    return shown;
}

void DataView::loadTableData(const QString &table)
{
    const auto sql = QStringLiteral("SELECT * FROM \"%1\"").arg(table);
    sql_edit_->setPlainText(sql);
    const int shown = populateRows(sql);
    if (shown < 0) return;

    total_rows_ = shown;
    auto cq = db_helper_->Prepare(
        QStringLiteral("SELECT COUNT(*) FROM \"%1\"").arg(table).toStdString());
    if (cq) {
        auto step = cq->Step();
        if (step && *step) {
            total_rows_ = static_cast<int>(cq->ColumnInt64(0));
        }
    }
    row_status_->setText(LOC("data_view.rows_status").arg(shown).arg(total_rows_));
}

void DataView::onTableSelected(int index)
{
    if (index < 0 || index >= table_names_.size()) return;
    loadTableData(table_names_[index]);
}

void DataView::onRunQuery()
{
    const auto sql = sql_edit_->toPlainText().trimmed();
    if (sql.isEmpty()) return;
    const int shown = populateRows(sql);
    if (shown >= 0) {
        row_status_->setText(LOC("data_view.rows_status").arg(shown).arg(total_rows_));
    }
}

void DataView::refreshCurrentTable()
{
    if (mode_ == Mode::Database) {
        loadTableData(table_combo_->currentText());
    } else {
        loadCsv();
    }
}

void DataView::applyTheme()
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.data_view")));
    table_->setStyleSheet(ts.qss(QStringLiteral("style.data_view_table")));
    if (auto *hl = sql_edit_->findChild<NezhaIDE::Editor::SimpleHighlighter *>()) {
        hl->setTokenColors(ts.syntaxColors());
    }
}

} // namespace NezhaIDE::Views
