#pragma once

#include <QTabWidget>
#include <QHash>

namespace NezhaIDE::Views {
    class HttpViewPanel;
    class HydraViewPanel;
    class HexEditor;
}

namespace NezhaIDE::Editor {

class CodeEditor;

/**
 * 编辑器标签页托管容器，管理所有打开的编辑器实例。
 *
 * 统一管理 CodeEditor（文本）、HexEditor（二进制分析）和 HttpViewPanel（HTTP 客户端）
 * 三种编辑器类型。openFile() 自动检测文件类型并创建对应的编辑器。
 */
class EditorTabHost final : public QTabWidget {
    Q_OBJECT

public:
    explicit EditorTabHost(QWidget *parent = nullptr);

    [[nodiscard]] CodeEditor *currentEditor() const;

public slots:
    /**
     * 打开文件：自动检测是否为二进制，分别创建 CodeEditor 或 HexEditor。
     *
     * @param path 文件完整路径。
     */
    void openFile(const QString &path);

    /**
     * 以二进制分析模式打开文件。
     *
     * @param path 二进制文件路径。
     */
    void openBinaryFile(const QString &path);

    /**
     * 创建或切换到 HTTP 客户端面板。
     */
    void openHttpClient();

    /**
     * 创建或切换到 Hydra 凭据测试面板。
     */
    void openHydra();

signals:
    void editActionsChanged();

private:
    void onTabCloseRequested(int index);
    void ensureWelcomeTab();
    void removeWelcomeTab();
    void applyStyles();

    QHash<QString, CodeEditor *> editors_;
    QHash<QString, NezhaIDE::Views::HexEditor *> hex_editors_;
    NezhaIDE::Views::HttpViewPanel *http_panel_{};
    NezhaIDE::Views::HydraViewPanel *hydra_panel_{};
    QWidget *welcome_tab_{};
};

} // namespace NezhaIDE::Editor
