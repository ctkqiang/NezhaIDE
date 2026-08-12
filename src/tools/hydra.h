//
// Created by 钟智强 on 2026/8/12.
//
#pragma once

#ifndef NEZHAIDE_HYDRA_H
#define NEZHAIDE_HYDRA_H

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <memory>

class QTemporaryFile;

namespace NezhaIDE::Tools {
    /**
     * 密码数据集来源，UI 单选互斥。
     *
     * GitHub 为不可信远程列表，仅用户显式点击导入后才下载；
     * Random 由内置生成器本地产生；Custom 为本地自定义密码文件。
     */
    enum class PasswordSource {
        None,
        GitHub,
        Random,
        Custom
    };

    /**
     * 面板状态机，驱动状态标签显示与 Run 按钮可用性。
     *
     * NoTargetConfigured/SelectService 先于凭据类状态求值，
     * 因为服务与目标决定其余配置的合法性。
     */
    enum class HydraState {
        NoTargetConfigured,
        NoUsernameFile,
        UsernameLoaded,
        InvalidUsernameFile,
        SelectService,
        SelectPasswordSource,
        CustomPasswordRequired,
        PasswordLoaded,
        Ready,
        Running,
        Stopped,
        Completed,
        Error
    };

    /**
     * 工具数据库中一个 Hydra 模块参数的 UI 描述。
     */
    struct ModuleParam {
        QString name;
        QString display;
        QString type;
        QString default_value;
        QString help;
    };

    /**
     * Hydra 工具运行配置，独立于通用 Tools 模型。
     *
     * 仅持有路径与参数，不含运行时凭据内容；凭据数据
     * 经 HydraService 导入数据库后按 dataset 引用。
     * module_params 保存动态参数区收集的模块专用值，
     * 其中 module_args 映射为 hydra 的 -m 选项。
     */
    struct HydraConfig {
        QString target_host;
        int target_port{22};
        QString service{QStringLiteral("ssh")};
        QString username_file;
        PasswordSource password_source{PasswordSource::None};
        QString github_url;
        int random_count{100};
        int random_min_length{8};
        int random_max_length{16};
        QString custom_password_file;
        int threads{4};
        int timeout_seconds{30};
        QString try_mode;
        bool exit_on_first{false};
        bool verbose{false};
        QString extra_args;
        QHash<QString, QString> module_params;
    };

    /**
     * Hydra 凭据测试工具服务单例。
     *
     * 负责模块元数据的数据库播种与已安装模块探测（以本机 hydra -h
     * 输出为真值来源）、用户名/密码数据集的加载验证与 SQL 导入（含来源
     * 溯源）、GitHub 显式导入、随机密码生成，以及 hydra 进程的启动/停止。
     * 运行日志经 sanitize 处理，不暴露明文密码。
     */
    class HydraService final : public QObject {
        Q_OBJECT

    public:
        static HydraService &instance();

        HydraService(const HydraService &) = delete;

        HydraService &operator=(const HydraService &) = delete;

        HydraService(HydraService &&) = delete;

        HydraService &operator=(HydraService &&) = delete;

        /**
         * 数据集导入结果，ok() 为 false 时 error 携带原因。
         */
        struct LoadResult {
            std::int64_t dataset_id{0};
            int entry_count{0};
            QString error;

            [[nodiscard]] bool ok() const { return error.isEmpty(); }
        };

        /**
         * 已知 Hydra 模块的静态元数据（名称/显示名/默认端口）。
         * 与已安装状态无关，探测结果见 availableModules()。
         */
        struct ModuleInfo {
            QString name;
            QString display;
            int port{0};
        };

        /**
         * 全部已知模块（含未编译模块），用于填充服务选择器。
         */
        [[nodiscard]] QList<ModuleInfo> knownModules() const;

        /**
         * 探测得到的已安装模块名列表；探测完成前为空。
         */
        [[nodiscard]] QStringList availableModules() const;

        /**
         * 本机 hydra 明确标记为未编译的模块名列表。
         */
        [[nodiscard]] QStringList unavailableModules() const;

        /**
         * 模块是否被本机 hydra 编译支持（探测完成后才可信）。
         */
        [[nodiscard]] bool isModuleInstalled(const QString &service) const;

        /**
         * 探测后是否至少有一个可用的服务模块（hydra 是否已安装）。
         */
        [[nodiscard]] bool hasInstalledModules() const noexcept;

        /**
         * 从工具数据库读取指定模块的动态参数定义。
         *
         * @param service 模块名（tool_procedures.name）。
         * @return 参数列表；模块无参数时为空。
         */
        [[nodiscard]] QList<ModuleParam> moduleParams(const QString &service) const;

        /**
         * 验证并导入用户名文件，返回数据集条目数。
         *
         * @param path 本地 .txt 文件路径。
         * @return 导入结果；文件不存在/不可读/为空时 error 非空。
         */
        LoadResult loadUsernameFile(const QString &path);

        /**
         * 以单个用户名（如 "root"）创建单条用户名数据集。
         *
         * @param username 用户名，去首尾空白后必须非空。
         * @return 导入结果。
         */
        LoadResult loadSingleUsername(const QString &username);

        /**
         * 验证并导入自定义密码文件。
         *
         * @param path 本地 .txt 文件路径。
         * @return 导入结果。
         */
        LoadResult loadPasswordFile(const QString &path);

        /**
         * 生成随机密码数据集并导入。
         *
         * @param count 生成条数。
         * @param min_length 最小长度。
         * @param max_length 最大长度。
         * @return 导入结果。
         */
        LoadResult generateRandomPasswords(int count, int min_length, int max_length);

        /**
         * 从用户显式指定的 URL 下载密码列表并导入（异步）。
         *
         * 仅允许 https，视为不可信输入：逐行清洗、限制单条长度，
         * 来源记录为 GitHub 并保存 URL 溯源。结果经 githubImportFinished 返回。
         *
         * @param url 密码列表的 https URL。
         */
        void importGithubPasswords(const QString &url);

        /**
         * 校验配置并启动 hydra 进程（异步，经 logLine 输出日志）。
         *
         * 缺少 hydra 二进制或配置无效时发出 runStateChanged(Error)。
         *
         * @param config 完整运行配置。
         */
        void run(const HydraConfig &config);

        /**
         * 停止正在运行的 hydra 进程，完成后状态为 Stopped。
         */
        void stop();

        [[nodiscard]] bool isRunning() const noexcept;

        /**
         * 最近一次成功导入的用户名/密码数据集 ID，供运行期引用。
         */
        [[nodiscard]] std::int64_t lastUsernameDatasetId() const noexcept;

        [[nodiscard]] std::int64_t lastPasswordDatasetId() const noexcept;

    signals:
        /**
         * 已安装模块探测完成（异步解析 hydra -h 输出后发出）。
         */
        void modulesProbed();

        void usernameDatasetChanged(int entryCount);

        void passwordDatasetChanged(int entryCount, const QString &provenance);

        void githubImportFinished(int entryCount, const QString &error);

        void runStateChanged(NezhaIDE::Tools::HydraState state);

        void logLine(const QString &line);

        void runFinished(bool success, int attemptCount, int foundCount, const QString &summary);

    private:
        HydraService();

        ~HydraService() override;

        void seedToolsDatabase();

        void probeInstalledModules();

        void onProbeFinished();

        [[nodiscard]] bool readLines(const QString &path, QStringList *out) const;

        void onProcessOutput();

        void onProcessFinished(int exitCode, QProcess::ExitStatus status);

        void onGithubReplyFinished(QNetworkReply *reply);

        void appendLog(const QString &line);

        LoadResult importLines(const QString &name, const QString &type, const QString &source,
                               const QString &filePath, const QStringList &lines);

        QProcess *process_{};
        QProcess *probe_{};
        QNetworkAccessManager *nam_{};

        std::unique_ptr<QTemporaryFile> user_file_{};
        std::unique_ptr<QTemporaryFile> pass_file_{};
        std::int64_t username_dataset_id_{0};
        std::int64_t password_dataset_id_{0};

        QString last_password_provenance_;
        QString pending_github_url_;

        int attempt_count_{0};
        int found_count_{0};
        bool stopped_{false};
        bool probe_started_{false};
        QStringList installed_modules_;
        QStringList unavailable_modules_;
    };
}

#endif // NEZHAIDE_HYDRA_H
