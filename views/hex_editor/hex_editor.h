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

/**
 * 二进制分析编辑器主控件。
 *
 * 上部为 HexData 视图，下部为 QTabWidget 包含 6 个分析面板：
 * Disassembly、Strings、Sections、Symbols、Imports、Info。
 * 通过 BinaryParser 和 Disassembler 获取真实数据。
 * HexData 与 Disassembly 之间支持双向选中联动。
 */
class HexEditor final : public QWidget {
    Q_OBJECT

public:
    explicit HexEditor(const QString &filePath, QWidget *parent = nullptr);

    /**
     * 加载并解析二进制文件。
     *
     * @return true 表示解析成功，false 表示加载失败。
     */
    bool load();

    /**
     * 根据当前主题重新应用颜色和样式。
     */
    void applyTheme();

    /**
     * 获取当前打开文件的路径。
     *
     * @return 文件完整路径。
     */
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
