#pragma once

#ifndef NEZHAIDE_DATA_VIEW_H
#define NEZHAIDE_DATA_VIEW_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <memory>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSplitter;
class QTableWidget;

namespace NezhaIDE::Services {
class DatabaseHelper;
}

namespace NezhaIDE::Views {

/**
 * 数据文件预览视图（.db / .csv）。
 *
 * .db 模式：左侧表选择 + SQL 查询面板，表内容以只读表格展示。
 * .csv 模式：解析后以表格展示。SQL 面板仅在 .db 模式显示。
 */
class DataView final : public QWidget {
    Q_OBJECT

public:
    enum class Mode { Database, Csv };

    explicit DataView(const QString &filePath, QWidget *parent = nullptr);
    ~DataView() override;

    bool load();
    void applyTheme();

    [[nodiscard]] QString filePath() const { return file_path_; }
    [[nodiscard]] Mode mode() const { return mode_; }

signals:
    void titleChanged(const QString &title);

private:
    void setupUI();
    void setupSqlPanel();

    bool loadDatabase();
    bool loadCsv();
    void loadTableData(const QString &table);
    int populateRows(const QString &sql);
    void onTableSelected(int index);
    void onRunQuery();
    void refreshCurrentTable();

    QString file_path_;
    Mode mode_{Mode::Csv};

    QLabel *path_label_{};
    QComboBox *table_combo_{};
    QPushButton *refresh_btn_{};
    QLabel *row_status_{};

    QSplitter *main_splitter_{};
    QTableWidget *table_{};
    QWidget *sql_panel_{};
    QPlainTextEdit *sql_edit_{};
    QPushButton *run_btn_{};

    std::unique_ptr<NezhaIDE::Services::DatabaseHelper> db_helper_;
    QStringList table_names_;
    int total_rows_{0};

    static constexpr int kMaxRows = 500;
};

} // namespace NezhaIDE::Views

#endif //NEZHAIDE_DATA_VIEW_H
