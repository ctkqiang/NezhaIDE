#pragma once

#include <QAbstractScrollArea>
#include <cstdint>
#include <vector>

/**
 * 十六进制编辑与分析 UI 组件命名空间。
 */
namespace NezhaIDE::Views {

/**
 * 十六进制数据视图组件，基于 QAbstractScrollArea 实现虚拟滚动。
 *
 * 每行显示 16 字节，分为三列：文件偏移量、十六进制数据、ASCII 表示。
 * 支持鼠标拖拽选取字节范围，通过 ThemeService 获取主题颜色。
 * 选中范围变化时发出 byteRangeSelected 信号以实现跨视图联动。
 *
 * @note 仅渲染可见行，支持大文件浏览（>100MB）。
 */
class HexView final : public QAbstractScrollArea {
    Q_OBJECT

public:
    static constexpr int kBytesPerRow = 16;

    explicit HexView(QWidget *parent = nullptr);

    void setData(const uint8_t *data, size_t size);

    uint64_t selectionStart() const noexcept { return sel_start_; }
    uint64_t selectionEnd() const noexcept { return sel_end_; }
    bool hasSelection() const noexcept { return sel_start_ < sel_end_; }

public slots:
    void navigateToOffset(uint64_t offset);

signals:
    void byteRangeSelected(uint64_t offset, uint64_t size);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateScrollbars();
    int rowCount() const;
    int rowHeight() const;

    int offsetColumnWidth() const;
    int hexColumnWidth() const;
    int asciiColumnX() const;

    int byteAtPosition(const QPoint &pos) const;
    uint64_t byteOffsetAtPosition(const QPoint &pos) const;

    const uint8_t *data_{nullptr};
    size_t data_size_{0};
    uint64_t sel_start_{0};
    uint64_t sel_end_{0};
    bool selecting_{false};
    uint64_t drag_anchor_{0};
};

} // namespace NezhaIDE::Views
