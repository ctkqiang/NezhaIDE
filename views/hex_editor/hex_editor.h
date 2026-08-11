#pragma once

#include "binary_parser.h"
#include "hex_view.h"
#include "disassembler.h"
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QWidget>
#include <memory>
#include <vector>

namespace NezhaIDE::Views {

class HexEditor final : public QWidget {
    Q_OBJECT

public:
    explicit HexEditor(const QString &filePath, QWidget *parent = nullptr);

    bool load();
    void applyTheme();
    QString filePath() const;

signals:
    void titleChanged(const QString &title);

private:
    void setupUI();
    void setupHexDataView();
    void setupAnalysisTabs();
    void populateSections();
    void populateSymbols();
    void populateImports();
    void populateStrings();
    void populateInfo();
    void runDisassembly();
    void applyEmptyState(QTableWidget *table, const QString &message);
    void addTableRow(QTableWidget *table, const QStringList &columns);

    HexView *hex_view_{};
    QSplitter *main_splitter_{};
    QTabWidget *analysis_tabs_{};

    QTableWidget *sections_table_{};
    QTableWidget *symbols_table_{};
    QTableWidget *imports_table_{};
    QTableWidget *strings_table_{};
    QPlainTextEdit *disasm_view_{};
    QPlainTextEdit *info_view_{};

    std::unique_ptr<Model::BinaryParser> parser_;
    std::vector<Services::DisasmInstruction> disasm_insns_;
    QString file_path_;
};

} // namespace NezhaIDE::Views
