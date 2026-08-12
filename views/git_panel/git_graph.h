#pragma once

#include <QAbstractScrollArea>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace NezhaIDE::Views {

struct GitGraphCommit {
    QString hash;
    QStringList parents;
    QString author;
    QString date;
    QString subject;
    QStringList refs;
    int column{-1};
};

/**
 * Git 提交图视图，虚拟滚动自绘。
 *
 * 每行一个 commit，左侧为分支泳道图（继承/新列分配 + 正交折线边），
 * 右侧为 refs 标签、主题行与作者/日期行。点击行发出 commitSelected。
 */
class GitGraphView : public QAbstractScrollArea {
    Q_OBJECT

public:
    explicit GitGraphView(QWidget *parent = nullptr);

    void setCommits(QList<GitGraphCommit> commits);
    void setRefs(const QHash<QString, QStringList> &refsByHash);
    void clear();
    void setEmptyText(const QString &text);
    void refresh();

    [[nodiscard]] QString selectedHash() const { return selected_hash_; }

signals:
    void commitSelected(const QString &hash, const QString &subject);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateScrollRange();
    void assignColumns();
    void updateRefs();
    void drawEdge(QPainter &p, const QPointF &from, const QPointF &to, const QColor &color) const;
    int rowAt(int y) const;
    qreal laneCenterX(int column) const;
    QColor branchColor(int column) const;

    static constexpr int kRowHeight = 46;
    static constexpr int kLaneWidth = 26;
    static constexpr int kNodeRadius = 6;

    QList<GitGraphCommit> commits_;
    QHash<QString, QStringList> refs_by_hash_;
    QHash<QString, int> row_of_hash_;
    QString empty_text_;
    QString selected_hash_;
    int selected_row_{-1};
    int hover_row_{-1};
    int lane_count_{0};
};

} // namespace NezhaIDE::Views
