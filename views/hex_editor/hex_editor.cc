#include "hex_editor.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include <QFontDatabase>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTextBlock>
#include <QToolTip>
#include <QVBoxLayout>

namespace NezhaIDE::Views {

namespace {

struct DisasmBlockData : QTextBlockUserData {
    int insn_index{-1};
};

} // namespace

HexEditor::HexEditor(const QString &filePath, QWidget *parent)
    : QWidget(parent), file_path_(filePath)
{
    setupUI();
    setupGoBar();

    connect(disasm_view_, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        const auto *data = dynamic_cast<const DisasmBlockData *>(
            disasm_view_->textCursor().block().userData());
        if (!data || data->insn_index < 0) return;
        const auto &insn = disasm_insns_[static_cast<size_t>(data->insn_index)];
        hex_view_->setSelection(insn.offset, insn.size);
        syncHexSelectionToDisasm();
    });

    connect(&Services::ThemeService::instance(), &Services::ThemeService::themeChanged,
            this, [this] { applyTheme(); });
}

void HexEditor::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    main_splitter_ = new QSplitter(Qt::Vertical, this);
    main_splitter_->setHandleWidth(1);
    main_splitter_->setChildrenCollapsible(false);

    setupHexDataView();
    setupAnalysisTabs();

    main_splitter_->setStretchFactor(0, 3);
    main_splitter_->setStretchFactor(1, 2);
    main_splitter_->setSizes({360, 240});

    layout->addWidget(main_splitter_);
}

void HexEditor::setupGoBar() {
    auto *bar = new QWidget(this);
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(8, 4, 8, 4);
    h->setSpacing(6);

    pos_label_ = new QLabel(QStringLiteral("0x0"), bar);
    pos_label_->setObjectName(QStringLiteral("hexPosLabel"));
    pos_label_->setMinimumWidth(56);
    h->addWidget(pos_label_);
    h->addStretch();

    go_edit_ = new QLineEdit(bar);
    go_edit_->setObjectName(QStringLiteral("hexGoEdit"));
    go_edit_->setPlaceholderText(LOC("hex.offset_hint"));
    go_edit_->setFixedWidth(200);
    h->addWidget(go_edit_);

    auto *go_btn = new QPushButton(LOC("hex.go_to"), bar);
    go_btn->setObjectName(QStringLiteral("hexGoButton"));
    h->addWidget(go_btn);

    connect(go_btn, &QPushButton::clicked, this, [this] { goToOffset(); });
    connect(go_edit_, &QLineEdit::returnPressed, this, [this] { goToOffset(); });

    layout()->addWidget(bar);
}

void HexEditor::setupHexDataView() {
    hex_view_ = new HexView(main_splitter_);
    hex_view_->setObjectName(QStringLiteral("hexView"));
    main_splitter_->addWidget(hex_view_);
}

void HexEditor::setupAnalysisTabs() {
    analysis_tabs_ = new QTabWidget(main_splitter_);
    analysis_tabs_->setObjectName(QStringLiteral("hexAnalysisTabs"));

    disasm_view_ = new DisasmView(analysis_tabs_);
    disasm_view_->setObjectName(QStringLiteral("hexDisasmView"));
    disasm_view_->setReadOnly(true);
    disasm_view_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    analysis_tabs_->addTab(disasm_view_, LOC("hex.disassembly"));

    strings_table_ = new QTableWidget(0, 2, analysis_tabs_);
    strings_table_->setObjectName(QStringLiteral("hexStringsTable"));
    strings_table_->setHorizontalHeaderLabels(
        {QStringLiteral("Offset"), LOC("hex.strings")});
    strings_table_->horizontalHeader()->setStretchLastSection(true);
    strings_table_->verticalHeader()->setVisible(false);
    strings_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    strings_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    strings_table_->setShowGrid(false);
    analysis_tabs_->addTab(strings_table_, LOC("hex.strings"));

    sections_table_ = new QTableWidget(0, 5, analysis_tabs_);
    sections_table_->setObjectName(QStringLiteral("hexSectionsTable"));
    sections_table_->setHorizontalHeaderLabels(
        {LOC("hex.sections"), QStringLiteral("VA"), QStringLiteral("Offset"),
         QStringLiteral("Size"), QStringLiteral("Flags")});
    sections_table_->horizontalHeader()->setStretchLastSection(true);
    sections_table_->verticalHeader()->setVisible(false);
    sections_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sections_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    sections_table_->setShowGrid(false);
    analysis_tabs_->addTab(sections_table_, LOC("hex.sections"));

    symbols_table_ = new QTableWidget(0, 3, analysis_tabs_);
    symbols_table_->setObjectName(QStringLiteral("hexSymbolsTable"));
    symbols_table_->setHorizontalHeaderLabels(
        {LOC("hex.symbols"), QStringLiteral("Address"), QStringLiteral("Size")});
    symbols_table_->horizontalHeader()->setStretchLastSection(true);
    symbols_table_->verticalHeader()->setVisible(false);
    symbols_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    symbols_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    symbols_table_->setShowGrid(false);
    analysis_tabs_->addTab(symbols_table_, LOC("hex.symbols"));

    imports_table_ = new QTableWidget(0, 2, analysis_tabs_);
    imports_table_->setObjectName(QStringLiteral("hexImportsTable"));
    imports_table_->setHorizontalHeaderLabels(
        {LOC("hex.imports"), QStringLiteral("Library")});
    imports_table_->horizontalHeader()->setStretchLastSection(true);
    imports_table_->verticalHeader()->setVisible(false);
    imports_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    imports_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    imports_table_->setShowGrid(false);
    analysis_tabs_->addTab(imports_table_, LOC("hex.imports"));

    info_view_ = new QPlainTextEdit(analysis_tabs_);
    info_view_->setObjectName(QStringLiteral("hexInfoView"));
    info_view_->setReadOnly(true);
    info_view_->setMaximumBlockCount(100);
    analysis_tabs_->addTab(info_view_, LOC("hex.info"));

    main_splitter_->addWidget(analysis_tabs_);
}

bool HexEditor::load() {
    auto result = Model::BinaryParser::open(file_path_);
    if (!result.has_value()) return false;
    parser_ = std::make_unique<Model::BinaryParser>(std::move(result.value()));

    const auto &raw = parser_->raw_data();
    hex_view_->setData(raw.data(), raw.size());

    connect(hex_view_, &HexView::byteRangeSelected, this, [this](uint64_t, uint64_t) {
        pos_label_->setText(QStringLiteral("0x%1").arg(hex_view_->selectionStart(), 0, 16));
        syncHexSelectionToDisasm();
    });

    populateSections();
    populateSymbols();
    populateImports();
    populateStrings();
    populateInfo();
    runDisassembly();

    emit titleChanged(QStringLiteral("Hex: ") + QFileInfo(file_path_).fileName());
    return true;
}

void HexEditor::populateSections() {
    if (!parser_) return;
    const auto &secs = parser_->sections();
    if (secs.empty()) {
        applyEmptyState(sections_table_, LOC("hex.no_data"));
        return;
    }
    sections_table_->setRowCount(static_cast<int>(secs.size()));
    for (size_t i = 0; i < secs.size(); ++i) {
        const auto &s = secs[i];
        int row = static_cast<int>(i);
        sections_table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(s.name)));
        sections_table_->setItem(row, 1,
            new QTableWidgetItem(QStringLiteral("0x%1").arg(s.virtual_address, 0, 16)));
        sections_table_->setItem(row, 2,
            new QTableWidgetItem(QStringLiteral("0x%1").arg(s.file_offset, 0, 16)));
        sections_table_->setItem(row, 3,
            new QTableWidgetItem(QStringLiteral("0x%1").arg(s.size, 0, 16)));
        QString flags;
        if (s.is_readable) flags += QStringLiteral("R");
        if (s.is_writable) flags += QStringLiteral("W");
        if (s.is_executable) flags += QStringLiteral("X");
        sections_table_->setItem(row, 4, new QTableWidgetItem(flags));
    }
    sections_table_->resizeColumnsToContents();
}

void HexEditor::populateSymbols() {
    if (!parser_) return;
    const auto &syms = parser_->symbols();
    if (syms.empty()) {
        applyEmptyState(symbols_table_, LOC("hex.stripped"));
        return;
    }
    symbols_table_->setRowCount(static_cast<int>(syms.size()));
    for (size_t i = 0; i < syms.size(); ++i) {
        const auto &s = syms[i];
        int row = static_cast<int>(i);
        symbols_table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(s.name)));
        symbols_table_->setItem(row, 1,
            new QTableWidgetItem(QStringLiteral("0x%1").arg(s.address, 0, 16)));
        symbols_table_->setItem(row, 2,
            new QTableWidgetItem(QStringLiteral("0x%1").arg(s.size, 0, 16)));
    }
    symbols_table_->resizeColumnsToContents();
}

void HexEditor::populateImports() {
    if (!parser_) return;
    const auto &imps = parser_->imports();
    if (imps.empty()) {
        applyEmptyState(imports_table_, LOC("hex.no_data"));
        return;
    }
    imports_table_->setRowCount(static_cast<int>(imps.size()));
    for (size_t i = 0; i < imps.size(); ++i) {
        const auto &imp = imps[i];
        int row = static_cast<int>(i);
        imports_table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(imp.name)));
        imports_table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(imp.library)));
    }
    imports_table_->resizeColumnsToContents();
}

void HexEditor::populateStrings() {
    if (!parser_) return;
    const auto &strs = parser_->strings();
    if (strs.empty()) {
        applyEmptyState(strings_table_, LOC("hex.no_data"));
        return;
    }
    strings_table_->setRowCount(std::min(static_cast<int>(strs.size()), 5000));
    for (size_t i = 0; i < strs.size() && i < 5000; ++i) {
        const auto &s = strs[i];
        int row = static_cast<int>(i);
        strings_table_->setItem(row, 0, new QTableWidgetItem(
            QStringLiteral("0x%1").arg(i, 0, 16)));
        strings_table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(s)));
    }
    strings_table_->resizeColumnsToContents();
}

void HexEditor::populateInfo() {
    if (!parser_) return;
    QStringList lines;
    lines.append(QStringLiteral("File: ") + file_path_);
    lines.append(QStringLiteral("Format: ") + QString::fromStdString(parser_->format_name()));
    lines.append(QStringLiteral("Architecture: ") + QString::fromStdString(parser_->arch_name()));
    lines.append(QStringLiteral("File Size: %1 bytes").arg(parser_->raw_data().size()));
    lines.append(QStringLiteral("Entry Point: 0x%1").arg(parser_->entry_point(), 0, 16));
    lines.append(QStringLiteral("Image Base: 0x%1").arg(parser_->image_base(), 0, 16));
    lines.append(QStringLiteral("Sections: %1").arg(parser_->sections().size()));
    lines.append(QStringLiteral("Symbols: %1").arg(parser_->symbols().size()));
    lines.append(QStringLiteral("Imports: %1").arg(parser_->imports().size()));
    lines.append(QStringLiteral("Strings: %1").arg(parser_->strings().size()));
    lines.append(QStringLiteral("Disasm Targets: %1").arg(parser_->disasm_targets().size()));
    info_view_->setPlainText(lines.join(QStringLiteral("\n")));
}

void HexEditor::runDisassembly() {
    if (!parser_) return;
    const auto &targets = parser_->disasm_targets();
    const auto &raw = parser_->raw_data();
    auto arch = parser_->arch();

    disasm_insns_.clear();
    insn_block_numbers_.clear();
    disasm_view_->setCurrentLine(-1);
    disasm_view_->setInstructionHighlights({});
    disasm_view_->clear();

    if (targets.empty() || arch == Model::BinaryArch::Unknown) {
        disasm_view_->setPlainText(LOC("hex.no_data"));
        return;
    }

    auto &disasm = Services::Disassembler::instance();
    auto &ts = Services::ThemeService::instance();

    QTextCursor cursor(disasm_view_->document());
    cursor.beginEditBlock();

    const auto c_comment = ts.qcolor(QStringLiteral("text.tertiary"));
    const auto c_address = ts.qcolor(QStringLiteral("text.tertiary"));
    const auto c_bytes = ts.qcolor(QStringLiteral("text.secondary"));
    const auto c_mnemonic = ts.qcolor(QStringLiteral("accent"));
    const auto c_operands = ts.qcolor(QStringLiteral("text.primary"));

    const auto append = [&cursor](const QColor &color, const QString &text, const bool italic) {
        QTextCharFormat fmt;
        fmt.setForeground(color);
        fmt.setFontItalic(italic);
        cursor.setCharFormat(fmt);
        cursor.insertText(text);
    };

    for (const auto &tgt : targets) {
        if (tgt.offset >= raw.size() || tgt.size == 0) continue;
        auto actual_size = std::min(tgt.size, raw.size() - tgt.offset);

        auto result = disasm.disassemble(
            raw.data() + tgt.offset, actual_size, tgt.offset, tgt.address, arch);
        if (!result.has_value()) continue;

        append(c_comment, QStringLiteral("; section at 0x%1").arg(tgt.address, 0, 16), true);
        append(c_comment, QStringLiteral("\n"), false);

        for (const auto &insn : result.value()) {
            disasm_insns_.push_back(insn);
            insn_block_numbers_.push_back(cursor.blockNumber());

            auto *block_data = new DisasmBlockData;
            block_data->insn_index = static_cast<int>(disasm_insns_.size()) - 1;
            cursor.block().setUserData(block_data);

            append(c_address, QStringLiteral("0x%1").arg(insn.address, 16, 16, QLatin1Char('0')), false);
            append(c_comment, QStringLiteral("  "), false);
            append(c_bytes, QString::fromStdString(insn.bytes_hex), false);
            append(c_comment, QStringLiteral("  "), false);
            append(c_mnemonic, QString::fromStdString(insn.mnemonic).leftJustified(8, ' '), false);
            append(c_operands, QString::fromStdString(insn.operands), false);
            append(c_comment, QStringLiteral("\n"), false);
        }
        append(c_comment, QStringLiteral("\n"), false);
    }

    cursor.endEditBlock();
    disasm_view_->setTextCursor(QTextCursor(disasm_view_->document()));
}

void HexEditor::syncHexSelectionToDisasm() {
    if (!parser_ || disasm_insns_.empty()) return;

    const auto start = hex_view_->selectionStart();
    const auto end = hex_view_->selectionEnd();
    auto &ts = Services::ThemeService::instance();

    QList<QTextEdit::ExtraSelection> highlights;
    int first_match = -1;
    for (int i = 0; i < static_cast<int>(disasm_insns_.size()); ++i) {
        const auto &insn = disasm_insns_[i];
        if (insn.offset >= start && insn.offset < end) {
            if (first_match < 0) first_match = i;
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(ts.qcolor(QStringLiteral("overlay.selection")));
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor cursor(disasm_view_->document());
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                                insn_block_numbers_[i]);
            sel.cursor = cursor;
            highlights.append(sel);
        }
    }

    disasm_view_->setInstructionHighlights(highlights);
    if (first_match >= 0) {
        disasm_view_->setCurrentLine(insn_block_numbers_[first_match]);
    }
}

void HexEditor::goToOffset() {
    if (!parser_) return;
    const auto text = go_edit_->text().trimmed();
    bool ok = false;
    uint64_t offset = 0;
    if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
        offset = text.mid(2).toULongLong(&ok, 16);
    } else {
        offset = text.toULongLong(&ok, 10);
    }
    if (!ok || offset >= parser_->raw_data().size()) {
        go_edit_->setToolTip(LOC("hex.offset_invalid"));
        QToolTip::showText(go_edit_->mapToGlobal(QPoint(0, 0)), LOC("hex.offset_invalid"), go_edit_);
        return;
    }
    go_edit_->setToolTip({});
    hex_view_->setSelection(offset, 1);
    pos_label_->setText(QStringLiteral("0x%1").arg(offset, 0, 16));
    syncHexSelectionToDisasm();
}

void HexEditor::applyEmptyState(QTableWidget *table, const QString &message) {
    table->setRowCount(1);
    table->setItem(0, 0, new QTableWidgetItem(message));
    table->setSpan(0, 0, 1, table->columnCount());
}

QString HexEditor::filePath() const { return file_path_; }

void HexEditor::applyTheme() {
    auto &ts = Services::ThemeService::instance();
    setStyleSheet(ts.qss(QStringLiteral("style.http_panel")));
    hex_view_->setStyleSheet(
        QStringLiteral("QAbstractScrollArea { background: %1; }").arg(
            ts.color(QStringLiteral("bg.primary"))));
    analysis_tabs_->setStyleSheet(ts.qss(QStringLiteral("style.http_tabs")));
    disasm_view_->setStyleSheet(ts.qss(QStringLiteral("style.http_response_body")));
    info_view_->setStyleSheet(ts.qss(QStringLiteral("style.http_response_body")));
    sections_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    symbols_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    imports_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));
    strings_table_->setStyleSheet(ts.qss(QStringLiteral("style.http_kv_table")));

    // Go 定位栏
    if (auto *bar = pos_label_ ? pos_label_->parentWidget() : nullptr) {
        bar->setStyleSheet(ts.qss(QStringLiteral("style.hex_gobar")));
    }

    hex_view_->viewport()->update();
    disasm_view_->refresh();

    // 反汇编文本携带静态主题色，主题切换后需重建文档刷新颜色
    if (parser_) {
        const auto sel_start = hex_view_->selectionStart();
        runDisassembly();
        syncHexSelectionToDisasm();
        hex_view_->setSelection(sel_start, 1);
    }
}

} // namespace NezhaIDE::Views
