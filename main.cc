#include "src/configuration.h"
#include "src/model/credential_dataset.h"
#include "src/model/tool_registry.h"
#include "src/model/user_preference.h"
#include "src/repository/orm.h"
#include "src/services/database_helper.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/logger.h"
#include "views/main_window.h"
#include "views/editor/editor_tab_host.h"
#include "views/http_view_panel/http_view_panel.h"
#include "views/hydra/hydra_view_panel.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
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
#ifdef Q_OS_MACOS
    // macOS 26 的 BaseBoard 会对受保护进程(WindowServer/launchd)发起必然失败的
    // task_name_for_pid 查询(kern failure 0x5)却打成 Error 级 os_log，污染统一
    // 日志与控制台；禁用本进程的 os_log 发送以消除该噪音。Logger 走 iostream
    // 不受影响；WindowServer 进程侧打印的对应行不归本进程控制。
    if (!getenv("OS_ACTIVITY_MODE")) {
        setenv("OS_ACTIVITY_MODE", "disable", 1);
    }
#endif
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
    NezhaIDE::Repository::Repository<NezhaIDE::Model::CredentialDataset> dataset_repo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::CredentialEntry> entry_repo(db);

    const auto &tables = NezhaIDE::Constants::DatabaseTable;
    const auto init_all_schemas = [&]<typename... T>(T &...repos) {
        size_t i = 0;
        (init_schema(repos, tables[i++]), ...);
    };

    init_all_schemas(tool_repo, proc_repo, param_repo, pref_repo, dataset_repo, entry_repo);

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
        struct Selftest : QObject {
            NezhaIDE::Views::MainWindow &window;
            NezhaIDE::Views::HttpViewPanel *panel{};
            QLineEdit *urlInput{};
            QPushButton *sendBtn{};

            explicit Selftest(NezhaIDE::Views::MainWindow &w) : window(w) {}

            void dump(QWidget *w, const char *name) {
                if (!w) {
                    std::printf("SELFTEST: geo %s NULL\n", name);
                    return;
                }
                std::printf("SELFTEST: geo %s x=%d y=%d w=%d h=%d visible=%d\n",
                    name, w->x(), w->y(), w->width(), w->height(), w->isVisible() ? 1 : 0);
            }

            void pressReturn(QLineEdit *edit) {
                QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                QApplication::sendEvent(edit, &press);
                QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
                QApplication::sendEvent(edit, &release);
            }

            void writeFixture(const QString &path, const QString &content) {
                QFile f(path);
                if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    f.write(content.toUtf8());
                    f.close();
                }
            }

            void start() {
                auto *editorHost = window.findChild<NezhaIDE::Editor::EditorTabHost *>();
                if (!editorHost) {
                    std::printf("SELFTEST: no editor host\n");
                    QApplication::exit(2);
                    return;
                }
                editorHost->openHttpClient();
                panel = editorHost->findChild<NezhaIDE::Views::HttpViewPanel *>();
                if (!panel) {
                    std::printf("SELFTEST: no http panel\n");
                    QApplication::exit(2);
                    return;
                }
                urlInput = panel->findChild<QLineEdit *>(QStringLiteral("httpUrlInput"));
                sendBtn = panel->findChild<QPushButton *>(QStringLiteral("httpSendButton"));

                dump(panel, "panel");
                dump(panel->findChild<QPushButton *>(QStringLiteral("httpMethodButton")), "methodBtn");
                dump(urlInput, "urlInput");
                dump(sendBtn, "sendBtn");
                panel->grab().save(QStringLiteral("/tmp/nezha_01_initial.png"));

                const auto url = qEnvironmentVariable("NEZHA_SELFTEST_URL");
                urlInput->setText(url);
                std::printf("SELFTEST: url=%s\n", url.toUtf8().constData());
                sendBtn->click();

                QTimer::singleShot(1500, this, [this] {
                    panel->grab().save(QStringLiteral("/tmp/nezha_02_sending.png"));
                });

                QTimer::singleShot(10000, this, [this] {
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
                    dump(panel, "panel(resp)");
                    dump(pill, "statusPill");
                    dump(timeLabel, "timeLabel");
                    dump(body, "respBody");
                    panel->grab().save(QStringLiteral("/tmp/nezha_03_response.png"));

                    window.resize(760, 560);
                    QTimer::singleShot(400, this, [this] {
                        std::printf("SELFTEST: resized window to 760x560\n");
                        dump(panel, "panel(small)");
                        dump(panel->findChild<QPushButton *>(QStringLiteral("httpMethodButton")), "methodBtn(small)");
                        dump(urlInput, "urlInput(small)");
                        dump(sendBtn, "sendBtn(small)");
                        panel->grab().save(QStringLiteral("/tmp/nezha_04_small.png"));

                        urlInput->setText(QStringLiteral("not a url"));
                        sendBtn->click();
                        QTimer::singleShot(400, this, [this] {
                            panel->grab().save(QStringLiteral("/tmp/nezha_05_invalid.png"));
                        });

                        urlInput->setText(QStringLiteral("https://nonexistent-domain-zzz12345.com"));
                        sendBtn->click();
                        QTimer::singleShot(6000, this, [this] {
                            auto *pill = panel->findChild<QLabel *>(QStringLiteral("httpStatusPill"));
                            std::printf("SELFTEST: errorPill=%s\n", pill->text().toUtf8().constData());
                            panel->grab().save(QStringLiteral("/tmp/nezha_06_error.png"));
                            runHydraSelftest();
                        });
                    });
                });
            }
            void runHydraSelftest() {
                auto *editorHost = window.findChild<NezhaIDE::Editor::EditorTabHost *>();
                if (!editorHost) {
                    std::printf("SELFTEST: no editor host\n");
                    QApplication::exit(2);
                    return;
                }
                editorHost->openHydra();
                auto *hpanel = editorHost->findChild<NezhaIDE::Views::HydraViewPanel *>();
                if (!hpanel) {
                    std::printf("SELFTEST: no hydra panel\n");
                    QApplication::exit(2);
                    return;
                }
                auto *status = hpanel->findChild<QLabel *>(QStringLiteral("hydraStatusLabel"));
                auto *combo = hpanel->findChild<QComboBox *>(QStringLiteral("hydraServiceCombo"));
                auto *port = hpanel->findChild<QSpinBox *>(QStringLiteral("hydraPortSpin"));
                auto *host = hpanel->findChild<QLineEdit *>(QStringLiteral("hydraHostInput"));
                auto *users = hpanel->findChild<QLineEdit *>(QStringLiteral("hydraUsernamePath"));
                auto *customPath = hpanel->findChild<QLineEdit *>(QStringLiteral("hydraCustomPath"));
                auto *customRadio = hpanel->findChild<QRadioButton *>(QStringLiteral("hydraSourceCustom"));
                auto *randomRadio = hpanel->findChild<QRadioButton *>(QStringLiteral("hydraSourceRandom"));
                auto *generateBtn = hpanel->findChild<QPushButton *>(QStringLiteral("hydraGenerateButton"));
                auto *runBtn = hpanel->findChild<QPushButton *>(QStringLiteral("hydraRunButton"));
                const auto hints = hpanel->findChildren<QLabel *>(QStringLiteral("hydraHintLabel"));

                const auto findService = [combo](const QString &name) {
                    for (int i = 0; i < combo->count(); ++i) {
                        if (combo->itemData(i).toString() == name) return i;
                    }
                    return -1;
                };
                const auto selectService = [combo](const QString &name) {
                    for (int i = 0; i < combo->count(); ++i) {
                        if (combo->itemData(i).toString() == name) {
                            combo->setCurrentIndex(i);
                            return true;
                        }
                    }
                    return false;
                };
                const auto moduleArgsVisible = [hpanel] {
                    return hpanel->findChild<QLineEdit *>(
                        QStringLiteral("hydraModuleParam_module_args")) != nullptr;
                };

                // 等待模块探测（hydra -h 异步解析）完成后才断言
                QTimer::singleShot(1000, this, [=] {
                    std::printf("SELFTEST: hydra services=%d status=%s\n",
                                combo->count(), status->text().toUtf8().constData());
                    const auto ftpIdx = findService(QStringLiteral("ftp"));
                    const auto sshIdx = findService(QStringLiteral("ssh"));
                    std::printf("SELFTEST: hydra ftpIdx=%d sshIdx=%d sshEnabled=%d sshText=%s\n",
                                ftpIdx, sshIdx,
                                sshIdx >= 0 ? (combo->itemData(sshIdx, Qt::UserRole - 1).toBool() ? 1 : 0) : -1,
                                sshIdx >= 0 ? combo->itemText(sshIdx).toUtf8().constData() : "-");

                    selectService(QStringLiteral("ftp"));
                    std::printf("SELFTEST: hydra ftp port=%d moduleArgs=%s\n",
                                port->value(), moduleArgsVisible() ? "present" : "hidden");

                    selectService(QStringLiteral("mysql"));
                    std::printf("SELFTEST: hydra mysql port=%d moduleArgs=%s\n",
                                port->value(), moduleArgsVisible() ? "present" : "hidden");

                    selectService(QStringLiteral("http-get"));
                    std::printf("SELFTEST: hydra http-get port=%d moduleArgs=%s\n",
                                port->value(), moduleArgsVisible() ? "present" : "hidden");
                    if (auto *moduleArgs = hpanel->findChild<QLineEdit *>(
                            QStringLiteral("hydraModuleParam_module_args"))) {
                        moduleArgs->setText(QStringLiteral("SSL"));
                    }

                    selectService(QStringLiteral("mssql"));
                    std::printf("SELFTEST: hydra mssql port=%d\n", port->value());
                    hpanel->grab().save(QStringLiteral("/tmp/nezha_07_hydra_initial.png"));

                    selectService(QStringLiteral("ftp"));
                    host->setText(QStringLiteral("10.0.0.1"));
                    std::printf("SELFTEST: hydra status(host)=%s\n", status->text().toUtf8().constData());

                    writeFixture(QStringLiteral("/tmp/nezha_users.txt"), QStringLiteral("admin\nroot\ntest\n"));
                    users->setText(QStringLiteral("/tmp/nezha_users.txt"));
                    pressReturn(users);
                    QTimer::singleShot(400, this, [=] {
                        std::printf("SELFTEST: hydra status(users)=%s userHint=%s\n",
                                    status->text().toUtf8().constData(),
                                    hints.value(0)->text().toUtf8().constData());
                        customRadio->setChecked(true);
                        std::printf("SELFTEST: hydra status(custom)=%s\n", status->text().toUtf8().constData());

                        writeFixture(QStringLiteral("/tmp/nezha_passwords.txt"),
                                     QStringLiteral("123456\npassword\nadmin\nletmein\nqwerty\n"));
                        customPath->setText(QStringLiteral("/tmp/nezha_passwords.txt"));
                        pressReturn(customPath);
                        QTimer::singleShot(400, this, [=] {
                            std::printf("SELFTEST: hydra status(customLoaded)=%s runEnabled=%d passHint=%s\n",
                                        status->text().toUtf8().constData(), runBtn->isEnabled() ? 1 : 0,
                                        hints.value(3)->text().toUtf8().constData());
                            hpanel->grab().save(QStringLiteral("/tmp/nezha_08_hydra_ready.png"));

                            randomRadio->setChecked(true);
                            std::printf("SELFTEST: hydra status(randomSel)=%s\n", status->text().toUtf8().constData());
                            generateBtn->click();
                            QTimer::singleShot(500, this, [=] {
                                std::printf("SELFTEST: hydra status(randomLoaded)=%s passHint=%s\n",
                                            status->text().toUtf8().constData(),
                                            hints.value(3)->text().toUtf8().constData());

                                users->setText(QStringLiteral("/tmp/nezha_missing.txt"));
                                pressReturn(users);
                                QTimer::singleShot(400, this, [=] {
                                    std::printf("SELFTEST: hydra status(invalid)=%s\n", status->text().toUtf8().constData());
                                    hpanel->grab().save(QStringLiteral("/tmp/nezha_09_hydra_invalid.png"));

                                    users->setText(QStringLiteral("/tmp/nezha_users.txt"));
                                    pressReturn(users);
                                    QTimer::singleShot(400, this, [=] {
                                        std::printf("SELFTEST: hydra status(reloaded)=%s runEnabled=%d\n",
                                                    status->text().toUtf8().constData(), runBtn->isEnabled() ? 1 : 0);
                                        runBtn->click();
                                        QTimer::singleShot(600, this, [=] {
                                            const auto running = status->text() == QStringLiteral("运行中");
                                            std::printf("SELFTEST: hydra status(run)=%s\n", status->text().toUtf8().constData());
                                            hpanel->grab().save(QStringLiteral("/tmp/nezha_10_hydra_running.png"));
                                            if (running) {
                                                runBtn->click();
                                                QTimer::singleShot(3500, this, [=] {
                                                    std::printf("SELFTEST: hydra status(stopped)=%s\n", status->text().toUtf8().constData());
                                                    hpanel->grab().save(QStringLiteral("/tmp/nezha_11_hydra_stopped.png"));
                                                    std::printf("SELFTEST: DONE\n");
                                                    QApplication::exit(0);
                                                });
                                            } else {
                                                std::printf("SELFTEST: hydra finished before stop\n");
                                                std::printf("SELFTEST: DONE\n");
                                                QApplication::exit(0);
                                            }
                                        });
                                    });
                                });
                            });
                        });
                    });
                });
            }
        };
        auto *st = new Selftest(window);
        QTimer::singleShot(500, st, [st] { st->start(); });
    }

    return QApplication::exec();
}

int onStart() {

}