#include "src/configuration.h"
#include "src/model/tool_registry.h"
#include "src/model/user_preference.h"
#include "src/repository/orm.h"
#include "src/services/database_helper.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include "views/main_window.h"
#include "views/editor/editor_tab_host.h"
#include "views/http_client_panel.h"
#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

static auto& logger = NezhaIDE::Utilities::Logger::instance();

static QString ensure_data_directory() {
#ifdef NEZHA_PROJECT_ROOT
    const auto data_path = QStringLiteral(NEZHA_PROJECT_ROOT) + QStringLiteral("/data");
#else
    const auto data_path = QDir::currentPath() + QStringLiteral("/data");
#endif
    QDir dir;
    if (!dir.mkpath(data_path)) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "无法创建数据目录: {}",
            data_path.toStdString()
        );
    }
    return data_path;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName(NezhaIDE::Constants::ApplicationName.data());
    QApplication::setApplicationVersion(NezhaIDE::Constants::ApplicationVersion.data());
    QApplication::setOrganizationName(NezhaIDE::Constants::OrganisationName.data());

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__, __LINE__, __func__,
        "{}!",
        "IDE 启动中..."
    );

    const auto data_dir = ensure_data_directory();
    const auto db_path = data_dir + QStringLiteral("/")
                       + QString::fromUtf8(NezhaIDE::Constants::DatabaseName.data(),
                                           static_cast<int>(NezhaIDE::Constants::DatabaseName.size()));

    NezhaIDE::Services::DatabaseHelper db(db_path.toStdString());
    if (auto result = db.initializeDatabase(); !result.has_value()) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "数据库初始化失败: {}",
            result.error().Message
        );
        return 1;
    }

    const auto init_schema = [&]<typename T>(NezhaIDE::Repository::Repository<T>& repo,
                                               std::string_view table_name) -> bool {
        if (auto result = repo.initialize_schema(); !result.has_value()) {
            logger.log(
                NezhaIDE::Utilities::LogLevel::Error,
                __FILE__, __LINE__, __func__,
                "建表失败 {}: {}",
                table_name, result.error().Message
            );
            return false;
        }
        return true;
    };

    NezhaIDE::Repository::Repository<NezhaIDE::Model::Tool> tool_repo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::ToolProcedure> proc_repo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::ToolParameter> param_repo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::UserPreference> pref_repo(db);

    const auto &tables = NezhaIDE::Constants::DatabaseTable;
    const auto init_all_schemas = [&]<typename... T>(T &...repos) {
        size_t i = 0;
        (init_schema(repos, tables[i++]), ...);
    };

    init_all_schemas(tool_repo, proc_repo, param_repo, pref_repo);

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__, __LINE__, __func__,
        "数据库就绪: {}",
        db_path.toStdString()
    );

    auto &lang_mgr = NezhaIDE::Services::LocalizationService::instance();
    lang_mgr.initialize(NezhaIDE::Configuration::instance().language());

    NezhaIDE::Services::ThemeService::instance().initialize(
        NezhaIDE::Configuration::instance().theme());

    NezhaIDE::Views::MainWindow window;
    window.show();

    if (qEnvironmentVariableIsSet("NEZHA_SELFTEST")) {
        QTimer::singleShot(500, [&window] {
            auto *editorHost = window.findChild<NezhaIDE::Editor::EditorTabHost *>();
            if (!editorHost) {
                std::printf("SELFTEST: no editor host\n");
                QApplication::exit(2);
                return;
            }
            editorHost->openHttpClient();

            auto *panel = editorHost->findChild<NezhaIDE::Views::HttpClientPanel *>();
            if (!panel) {
                std::printf("SELFTEST: no http panel\n");
                QApplication::exit(2);
                return;
            }
            panel->grab().save(QStringLiteral("/tmp/nezha_01_initial.png"));

            auto *urlInput = panel->findChild<QLineEdit *>(QStringLiteral("httpUrlInput"));
            auto *sendBtn = panel->findChild<QPushButton *>(QStringLiteral("httpSendButton"));

            const auto url = qEnvironmentVariable("NEZHA_SELFTEST_URL");
            urlInput->setText(url);
            std::printf("SELFTEST: url=%s\n", url.toUtf8().constData());
            sendBtn->click();

            QTimer::singleShot(1200, [panel] {
                panel->grab().save(QStringLiteral("/tmp/nezha_02_sending.png"));
            });

            QTimer::singleShot(10000, [&window, panel, urlInput, sendBtn] {
                auto *pill = panel->findChild<QLabel *>(QStringLiteral("httpStatusPill"));
                auto *timeLabel = panel->findChild<QLabel *>(QStringLiteral("httpTimeLabel"));
                auto *sizeLabel = panel->findChild<QLabel *>(QStringLiteral("httpSizeLabel"));
                auto *body = panel->findChild<QPlainTextEdit *>(QStringLiteral("httpResponseBody"));
                auto *headers = panel->findChild<QTableWidget *>(QStringLiteral("httpHeadersTable"));

                std::printf("SELFTEST: status=%s\n", pill->text().toUtf8().constData());
                std::printf("SELFTEST: time=%s\n", timeLabel->text().toUtf8().constData());
                std::printf("SELFTEST: size=%s\n", sizeLabel->text().toUtf8().constData());
                std::printf("SELFTEST: bodyLen=%d\n", static_cast<int>(body->toPlainText().size()));
                std::printf("SELFTEST: headers=%d\n", headers->rowCount());
                panel->grab().save(QStringLiteral("/tmp/nezha_03_response.png"));

                urlInput->setText(QStringLiteral("not a url"));
                sendBtn->click();
                QTimer::singleShot(500, [panel] {
                    panel->grab().save(QStringLiteral("/tmp/nezha_04_invalid.png"));
                });

                urlInput->setText(QStringLiteral("https://nonexistent-domain-zzz12345.com"));
                sendBtn->click();
                QTimer::singleShot(6000, [&window, panel] {
                    auto *pill = panel->findChild<QLabel *>(QStringLiteral("httpStatusPill"));
                    std::printf("SELFTEST: errorPill=%s\n", pill->text().toUtf8().constData());
                    panel->grab().save(QStringLiteral("/tmp/nezha_05_error.png"));
                    std::printf("SELFTEST: DONE\n");
                    QApplication::exit(0);
                });
            });
        });
    }

    return QApplication::exec();
}