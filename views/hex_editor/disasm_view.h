#pragma once

#ifndef NEZHAIDE_DISASM_VIEW_H
#define NEZHAIDE_DISASM_VIEW_H

#include <QPlainTextEdit>
#include <QTextEdit>

class QPaintEvent;

namespace NezhaIDE::Views {

/**
 * 反汇编展示视图：QPlainTextEdit + 左侧行号区 + 当前指令行高亮。
 *
 * 指令行高亮（批量背景）与当前行高亮（光标所在行）分开维护，
 * 避免互斥覆盖；行号区随滚动/宽度变化自动重绘。
 */
class DisasmView final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit DisasmView(QWidget *parent = nullptr);

    /**
     * 设置指令命中高亮（hex 选中联动时传入）。
     */
    void setInstructionHighlights(const QList<QTextEdit::ExtraSelection> &selections);

    /**
     * 移动光标到指定行并居中，同时更新当前行高亮。
     * 与当前行相同时不做任何事（避免 setTextCursor 触发的联动回环）。
     */
    void setCurrentLine(int line);

    /**
     * 主题切换后重绘行号区。
     */
    void refresh();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    class LineNumberArea;
    void updateLineNumberAreaWidth();
    void updateExtraSelections();

    LineNumberArea *line_number_area_{};
    QList<QTextEdit::ExtraSelection> insn_highlights_;
    int current_line_{-1};
};

} // namespace NezhaIDE::Views

#endif //NEZHAIDE_DISASM_VIEW_H
