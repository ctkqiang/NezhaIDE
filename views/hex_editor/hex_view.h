#pragma once

#include <QAbstractScrollArea>
#include <cstdint>
#include <vector>

namespace NezhaIDE::Views {

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
