#include "git_graph.h"
#include "src/services/theme_service.h"
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

namespace NezhaIDE::Views {

namespace {
constexpr qreal kRefLabelHeight = 16;
constexpr qreal kRefLabelPad = 6;
}

GitGraphView::GitGraphView(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    viewport()->setCursor(Qt::PointingHandCursor);
}

void GitGraphView::setCommits(QList<GitGraphCommit> commits)
{
    commits_ = std::move(commits);
    assignColumns();
    updateRefs();
    row_of_hash_.clear();
    row_of_hash_.reserve(commits_.size());
    for (int i = 0; i < commits_.size(); ++i) {
        row_of_hash_.insert(commits_[i].hash, i);
    }
    if (selected_row_ >= commits_.size()) {
        selected_row_ = -1;
        selected_hash_.clear();
    }
    updateScrollRange();
    viewport()->update();
}

void GitGraphView::setRefs(const QHash<QString, QStringList> &refsByHash)
{
    refs_by_hash_ = refsByHash;
    updateRefs();
    viewport()->update();
}

void GitGraphView::clear()
{
    commits_.clear();
    refs_by_hash_.clear();
    row_of_hash_.clear();
    selected_hash_.clear();
    selected_row_ = -1;
    hover_row_ = -1;
    lane_count_ = 0;
    updateScrollRange();
    viewport()->update();
}

void GitGraphView::setEmptyText(const QString &text)
{
    empty_text_ = text;
    viewport()->update();
}

void GitGraphView::refresh()
{
    viewport()->update();
}

void GitGraphView::updateScrollRange()
{
    verticalScrollBar()->setRange(
        0, std::max(0, static_cast<int>(commits_.size()) * kRowHeight - viewport()->height()));
    verticalScrollBar()->setSingleStep(kRowHeight);
    verticalScrollBar()->setPageStep(viewport()->height());
}

void GitGraphView::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

int GitGraphView::rowAt(int y) const
{
    return (y + verticalScrollBar()->value()) / kRowHeight;
}

qreal GitGraphView::laneCenterX(int column) const
{
    return (column - (lane_count_ - 1) / 2.0) * kLaneWidth;
}

QColor GitGraphView::branchColor(int column) const
{
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    switch (column % 6) {
    case 0: return ts.qcolor(QStringLiteral("accent"));
    case 1: return ts.qcolor(QStringLiteral("git.added"));
    case 2: return ts.qcolor(QStringLiteral("git.modified"));
    case 3: return ts.qcolor(QStringLiteral("git.deleted"));
    case 4: return ts.qcolor(QStringLiteral("git.untracked"));
    default: return ts.qcolor(QStringLiteral("text.secondary"));
    }
}

/**
 * 从新到旧分配泳道：当前 commit 复用其在 lanes 中的既有列
 * （它是某个祖先 commit 的 parent 而被预留），否则取第一个空列；
 * 第一个 parent 继承当前列，其余 parent 预留新列。
 */
void GitGraphView::assignColumns()
{
    QList<QString> lanes;
    for (auto &c : commits_) {
        int col = lanes.indexOf(c.hash);
        if (col < 0) {
            col = lanes.indexOf(QString());
            if (col < 0) {
                lanes.append(QString());
                col = lanes.size() - 1;
            }
        }
        c.column = col;
        if (c.parents.isEmpty()) {
            lanes[col] = QString();
        } else {
            lanes[col] = c.parents[0];
            for (int p = 1; p < c.parents.size(); ++p) {
                const auto &parentHash = c.parents[p];
                int pcol = lanes.indexOf(parentHash);
                if (pcol < 0) {
                    pcol = lanes.indexOf(QString());
                    if (pcol < 0) {
                        lanes.append(QString());
                        pcol = lanes.size() - 1;
                    }
                }
                lanes[pcol] = parentHash;
            }
        }
    }
    lane_count_ = std::max(1, static_cast<int>(lanes.size()));
}

void GitGraphView::updateRefs()
{
    for (auto &c : commits_) {
        auto it = refs_by_hash_.constFind(c.hash);
        c.refs = it == refs_by_hash_.constEnd() ? QStringList{} : it.value();
    }
}

void GitGraphView::drawEdge(QPainter &p, const QPointF &from, const QPointF &to, const QColor &color) const
{
    auto c = color;
    c.setAlpha(150);
    QPen pen(c, 2);
    p.setPen(pen);
    if (from.x() == to.x()) {
        p.drawLine(from, to);
        return;
    }
    const qreal midY = (from.y() + to.y()) / 2.0;
    p.drawLine(from, QPointF(from.x(), midY));
    p.drawLine(QPointF(from.x(), midY), QPointF(to.x(), midY));
    p.drawLine(QPointF(to.x(), midY), to);
}

void GitGraphView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    auto &ts = NezhaIDE::Services::ThemeService::instance();
    QPainter p(viewport());
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(viewport()->rect(), ts.qcolor(QStringLiteral("bg.tertiary")));

    if (commits_.isEmpty()) {
        p.setPen(ts.qcolor(QStringLiteral("text.secondary")));
        p.drawText(viewport()->rect(), Qt::AlignCenter, empty_text_);
        return;
    }

    const int scrollY = verticalScrollBar()->value();
    const int first = std::clamp(scrollY / kRowHeight, 0, static_cast<int>(commits_.size()) - 1);
    const int last = std::clamp(scrollY / kRowHeight + viewport()->height() / kRowHeight + 1,
                                0, static_cast<int>(commits_.size()));

    const int graphW = std::max(kLaneWidth * 4, kLaneWidth * lane_count_ + kNodeRadius * 2 + 12);
    const int textX = graphW + 10;

    const auto highlightRow = [&](int row, const QColor &color) {
        const int y = row * kRowHeight - scrollY;
        p.fillRect(QRect(0, y, viewport()->width(), kRowHeight), color);
    };
    if (selected_row_ >= first && selected_row_ < last) {
        highlightRow(selected_row_, ts.qcolor(QStringLiteral("overlay.selection")));
    } else if (hover_row_ >= first && hover_row_ < last) {
        highlightRow(hover_row_, ts.qcolor(QStringLiteral("overlay.hover")));
    }

    for (int i = first; i < last; ++i) {
        const auto &c = commits_[i];
        const qreal y1 = i * kRowHeight + kRowHeight / 2.0;
        const qreal x1 = laneCenterX(c.column) + graphW / 2.0;
        for (const auto &parentHash : c.parents) {
            auto it = row_of_hash_.constFind(parentHash);
            if (it == row_of_hash_.constEnd()) continue;
            const int prow = it.value();
            const qreal y2 = prow * kRowHeight + kRowHeight / 2.0;
            const qreal x2 = laneCenterX(commits_[prow].column) + graphW / 2.0;
            drawEdge(p, {x1, y1}, {x2, y2}, branchColor(commits_[prow].column));
        }
    }

    QFont subjectFont(QStringLiteral("Menlo"), 12);
    subjectFont.setBold(true);
    QFont metaFont(QStringLiteral("Menlo"), 10);
    const QFontMetrics metaFm(metaFont);
    const QFontMetrics subjFm(subjectFont);

    for (int i = first; i < last; ++i) {
        const auto &c = commits_[i];
        const int rowY = i * kRowHeight - scrollY;
        const qreal cy = rowY + kRowHeight / 2.0;
        const qreal cx = laneCenterX(c.column) + graphW / 2.0;
        const QColor color = branchColor(c.column);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(QPointF(cx, cy), kNodeRadius, kNodeRadius);
        if (!c.refs.isEmpty()) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(color, 2));
            p.drawEllipse(QPointF(cx, cy), kNodeRadius + 2, kNodeRadius + 2);
        }

        qreal tx = textX;
        if (!c.refs.isEmpty()) {
            p.setFont(metaFont);
            p.setPen(Qt::white);
            for (const auto &ref : c.refs) {
                const qreal w = subjFm.horizontalAdvance(ref) + kRefLabelPad * 2;
                QRectF r(tx, rowY + 5, w, kRefLabelHeight);
                p.setPen(Qt::NoPen);
                p.setBrush(ref == QStringLiteral("HEAD") ? ts.qcolor(QStringLiteral("accent"))
                                                         : color);
                p.drawRoundedRect(r, 3, 3);
                p.setPen(Qt::white);
                p.drawText(r, Qt::AlignCenter, ref);
                tx += w + 6;
            }
        }

        p.setFont(subjectFont);
        p.setPen(ts.qcolor(QStringLiteral("text.primary")));
        const int subjectW = viewport()->width() - tx - 8;
        if (subjectW > 20) {
            p.drawText(QRect(tx, rowY + 4, subjectW, 20), Qt::AlignLeft | Qt::AlignVCenter,
                       subjFm.elidedText(c.subject, Qt::ElideRight, subjectW));
        }

        p.setFont(metaFont);
        p.setPen(ts.qcolor(QStringLiteral("text.secondary")));
        const auto meta = QStringLiteral("%1 · %2 · %3")
                              .arg(c.author, c.date.left(10), c.hash.left(7));
        const int metaW = viewport()->width() - textX - 8;
        if (metaW > 20) {
            p.drawText(QRect(textX, rowY + 26, metaW, 16), Qt::AlignLeft | Qt::AlignVCenter,
                       metaFm.elidedText(meta, Qt::ElideRight, metaW));
        }
    }
}

void GitGraphView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    const int row = rowAt(event->pos().y());
    if (row >= 0 && row < commits_.size()) {
        selected_row_ = row;
        selected_hash_ = commits_[row].hash;
        viewport()->update();
        emit commitSelected(selected_hash_, commits_[row].subject);
    }
}

void GitGraphView::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(event->pos().y());
    const int visible = (verticalScrollBar()->value() + viewport()->height()) / kRowHeight;
    const int clamped = row < commits_.size() && row <= visible ? row : -1;
    if (clamped != hover_row_) {
        hover_row_ = clamped;
        viewport()->update();
    }
}

void GitGraphView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (hover_row_ >= 0) {
        hover_row_ = -1;
        viewport()->update();
    }
}

} // namespace NezhaIDE::Views
