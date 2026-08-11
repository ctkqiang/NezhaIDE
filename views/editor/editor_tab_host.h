#pragma once

#include <QTabWidget>
#include <QHash>

namespace NezhaIDE::Editor {

class CodeEditor;

class EditorTabHost final : public QTabWidget {
    Q_OBJECT

public:
    explicit EditorTabHost(QWidget *parent = nullptr);

public slots:
    void openFile(const QString &path);

private:
    void onTabCloseRequested(int index);
    void ensureWelcomeTab();
    void applyStyles();

    QHash<QString, CodeEditor *> editors_;
};

} // namespace NezhaIDE::Editor
