#pragma once

#include <QTabWidget>
#include <QHash>

namespace NezhaIDE::Views {
    class HttpClientPanel;
}

namespace NezhaIDE::Editor {

class CodeEditor;

class EditorTabHost final : public QTabWidget {
    Q_OBJECT

public:
    explicit EditorTabHost(QWidget *parent = nullptr);

public slots:
    void openFile(const QString &path);
    void openHttpClient();

private:
    void onTabCloseRequested(int index);
    void ensureWelcomeTab();
    void applyStyles();

    QHash<QString, CodeEditor *> editors_;
    NezhaIDE::Views::HttpClientPanel *http_panel_{};
};

} // namespace NezhaIDE::Editor
