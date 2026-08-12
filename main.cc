#include "src/configuration.h"
#include "src/model/credential_dataset.h"
#include "src/model/password.h"
#include "src/model/tool_registry.h"
#include "src/model/user_preference.h"
#include "src/repository/orm.h"
#include "src/services/database_helper.h"
#include "src/services/localization_service.h"
#include "src/services/theme_service.h"
#include "src/utilities/downloader.h"
#include "src/utilities/logger.h"
#include "views/main_window.h"
#include "views/editor/code_editor.h"
#include "views/editor/data_view.h"
#include "views/editor/editor_tab_host.h"
#include "views/editor/simple_highlighter.h"
#include "views/hex_editor/disasm_view.h"
#include "views/hex_editor/hex_editor.h"
#include "views/hex_editor/hex_view.h"
#include "views/http_view_panel/http_view_panel.h"
#include "views/hydra/hydra_view_panel.h"
#include "views/git_panel/git_panel.h"
#include "views/git_panel/git_graph.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextStream>
#include <QTimer>

#include <vector>

static auto& logger = NezhaIDE::Utilities::Logger::instance();

int onStart();

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
    app.setStyle(QStringLiteral("Fusion"));

    auto font = app.font();
    font.setPointSize(13);
    app.setFont(font);

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

    onStart();

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

                                    auto *singleRadio = hpanel->findChild<QRadioButton *>(
                                        QStringLiteral("hydraUsernameSourceSingle"));
                                    auto *singleInput = hpanel->findChild<QLineEdit *>(
                                        QStringLiteral("hydraUsernameSingle"));
                                    singleRadio->setChecked(true);
                                    std::printf("SELFTEST: hydra status(singleSel)=%s\n",
                                                status->text().toUtf8().constData());
                                    singleInput->setText(QStringLiteral("root"));
                                    pressReturn(singleInput);
                                    QTimer::singleShot(400, this, [=] {
                                        std::printf("SELFTEST: hydra status(singleLoaded)=%s userHint=%s runEnabled=%d\n",
                                                    status->text().toUtf8().constData(),
                                                    hints.value(0)->text().toUtf8().constData(),
                                                    runBtn->isEnabled() ? 1 : 0);
                                        hpanel->grab().save(QStringLiteral("/tmp/nezha_09b_hydra_single.png"));

                                        hpanel->findChild<QRadioButton *>(
                                            QStringLiteral("hydraUsernameSourceFile"))->setChecked(true);
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

                                                    auto *gitPanel = window.findChild<NezhaIDE::Views::GitPanel *>();
                                                    if (!gitPanel) {
                                                        std::printf("SELFTEST: no git panel\n");
                                                        QApplication::exit(2);
                                                        return;
                                                    }
                                                    gitPanel->setWorkingDirectory(QStringLiteral(NEZHA_PROJECT_ROOT));
                                                    auto *gitTabs = gitPanel->findChild<QTabWidget *>(QStringLiteral("gitFileTabs"));
                                                    auto *graph = gitPanel->findChild<NezhaIDE::Views::GitGraphView *>(QStringLiteral("gitGraphView"));
                                                    gitTabs->setCurrentIndex(1);
                                                    QTimer::singleShot(1500, this, [=] {
                                                        QMouseEvent press(QEvent::MouseButtonPress, QPointF(20, 23),
                                                                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                                                        QApplication::sendEvent(graph->viewport(), &press);
                                                        std::printf("SELFTEST: git graph selected=%s\n",
                                                                    graph->selectedHash().toUtf8().constData());
                                                        graph->grab().save(QStringLiteral("/tmp/nezha_12_git_graph.png"));
                                                        QTimer::singleShot(600, this, [=] {
                                                            auto *gitDiff = gitPanel->findChild<QPlainTextEdit *>();
                                                            std::printf("SELFTEST: git tab0=%s diffLen=%d\n",
                                                                        gitTabs->tabText(0).toUtf8().constData(),
                                                                        static_cast<int>(gitDiff->toPlainText().size()));

                                                            // DataView selftest：.db 表浏览+SQL / .csv 表格 / .json beautify
                                                            {
                                                                QFile csvFile(QStringLiteral("/tmp/nz_fixture.csv"));
                                                                if (csvFile.open(QIODevice::WriteOnly)) {
                                                                    csvFile.write(QStringLiteral(
                                                                        "name,age,city\n"
                                                                        "\"Alice, Jr\",25,\"New York\"\n"
                                                                        "Bob,30,Paris\n").toUtf8());
                                                                    csvFile.close();
                                                                }
                                                                QFile jsonFile(QStringLiteral("/tmp/nz_fixture.json"));
                                                                if (jsonFile.open(QIODevice::WriteOnly)) {
                                                                    jsonFile.write(QStringLiteral(
                                                                        "{\"a\":\"你好\",\"n\":42}").toUtf8());
                                                                    jsonFile.close();
                                                                }
                                                                QProcess::execute(QStringLiteral("sqlite3"),
                                                                    {QStringLiteral("/tmp/nz_fixture.db"),
                                                                     QStringLiteral("CREATE TABLE t(a INTEGER,b TEXT); "
                                                                                    "INSERT INTO t VALUES(1,'x'),(2,'y');")});

                                                                auto *editorHost = window.findChild<NezhaIDE::Editor::EditorTabHost *>();
                                                                if (!editorHost) {
                                                                    std::printf("SELFTEST: no editor host\n");
                                                                    QApplication::exit(2);
                                                                    return;
                                                                }
                                                                editorHost->openFile(QStringLiteral("/tmp/nz_fixture.db"));
                                                                editorHost->openFile(QStringLiteral("/tmp/nz_fixture.csv"));
                                                                editorHost->openFile(QStringLiteral("/tmp/nz_fixture.json"));

                                                                int dbOk = 0, csvOk = 0, jsonOk = 0;
                                                                const auto views = editorHost->findChildren<NezhaIDE::Views::DataView *>();
                                                                for (auto *dv : views) {
                                                                    auto *table = dv->findChild<QTableWidget *>();
                                                                    if (dv->filePath().endsWith(QStringLiteral("nz_fixture.db"))) {
                                                                        auto *combo = dv->findChild<QComboBox *>(QStringLiteral("dataViewTableCombo"));
                                                                        auto *sqlEdit = dv->findChild<QPlainTextEdit *>();
                                                                        dbOk = (combo && combo->count() == 1
                                                                                && combo->currentText() == QStringLiteral("t")
                                                                                && table && table->rowCount() >= 2
                                                                                && table->columnCount() == 2
                                                                                && table->item(0, 0)
                                                                                && table->item(0, 0)->text() == QStringLiteral("1")
                                                                                && sqlEdit
                                                                                && sqlEdit->findChild<NezhaIDE::Editor::SimpleHighlighter *>())
                                                                                   ? 1 : 0;
                                                                    } else if (dv->filePath().endsWith(QStringLiteral("nz_fixture.csv"))) {
                                                                        auto *combo = dv->findChild<QComboBox *>(QStringLiteral("dataViewTableCombo"));
                                                                        csvOk = (table && table->rowCount() == 2
                                                                                && table->columnCount() == 3
                                                                                && table->item(0, 0)
                                                                                && table->item(0, 0)->text() == QStringLiteral("Alice, Jr")
                                                                                && combo && combo->isHidden()) ? 1 : 0;
                                                                    }
                                                                }
                                                                auto *jsonEditor = qobject_cast<NezhaIDE::Editor::CodeEditor *>(editorHost->currentWidget());
                                                                if (jsonEditor) {
                                                                    const auto text = jsonEditor->document()->toPlainText();
                                                                    jsonOk = (text.contains(QStringLiteral("你好"))
                                                                              && text.contains(QStringLiteral("    "))
                                                                              && jsonEditor->findChild<NezhaIDE::Editor::SimpleHighlighter *>())
                                                                                 ? 1 : 0;
                                                                }
                                                                std::printf("SELFTEST: data db=%d csv=%d json=%d\n", dbOk, csvOk, jsonOk);
                                                                if (!dbOk || !csvOk || !jsonOk) {
                                                                    QApplication::exit(2);
                                                                    return;
                                                                }
                                                                if (!views.isEmpty()) {
                                                                    views.first()->grab().save(QStringLiteral("/tmp/nezha_13_data_view.png"));
                                                                }
                                                            }
                                                            std::printf("SELFTEST: DONE\n");
                                                            QApplication::exit(0);
                                                        });
                                                    });
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
                });
            }
        };
        auto *st = new Selftest(window);
        QTimer::singleShot(500, st, [st] { st->start(); });
    }

    if (qEnvironmentVariableIsSet("NEZHA_HEX_SELFTEST")) {
        struct HexSelftest : QObject {
            NezhaIDE::Views::MainWindow &window;

            explicit HexSelftest(NezhaIDE::Views::MainWindow &w) : window(w) {}

            void start() {
                auto *editorHost = window.findChild<NezhaIDE::Editor::EditorTabHost *>();
                if (!editorHost) {
                    std::printf("HEXST: no editor host\n");
                    QApplication::exit(2);
                    return;
                }
                editorHost->openBinaryFile(QStringLiteral("/bin/ls"));
                auto *hex = editorHost->findChild<NezhaIDE::Views::HexEditor *>();
                if (!hex) {
                    std::printf("HEXST: no hex editor\n");
                    QApplication::exit(2);
                    return;
                }
                auto *disasm = hex->findChild<NezhaIDE::Views::DisasmView *>(
                    QStringLiteral("hexDisasmView"));
                auto *hexView = hex->findChild<NezhaIDE::Views::HexView *>(
                    QStringLiteral("hexView"));
                auto *goEdit = hex->findChild<QLineEdit *>(QStringLiteral("hexGoEdit"));
                auto *posLabel = hex->findChild<QLabel *>(QStringLiteral("hexPosLabel"));

                const auto docLines = disasm->document()->blockCount();
                std::printf("HEXST: disasm lines=%d\n", docLines);
                hex->grab().save(QStringLiteral("/tmp/hex_01_loaded.png"));

                QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                goEdit->setText(QStringLiteral("0x1000"));
                QApplication::sendEvent(goEdit, &press);
                std::printf("HEXST: goto sel=%llx pos=%s\n", hexView->selectionStart(),
                            posLabel->text().toUtf8().constData());
                hex->grab().save(QStringLiteral("/tmp/hex_02_goto.png"));

                goEdit->setText(QStringLiteral("999999999999"));
                QApplication::sendEvent(goEdit, &press);
                std::printf("HEXST: goto-invalid sel=%llx\n", hexView->selectionStart());
                hex->grab().save(QStringLiteral("/tmp/hex_03_invalid.png"));

                hexView->setSelection(0x2000, 2);
                QApplication::processEvents();
                std::printf("HEXST: disasm highlights=%d\n", disasm->extraSelections().size());
                hex->grab().save(QStringLiteral("/tmp/hex_04_link.png"));

                auto block = disasm->document()->firstBlock();
                while (block.isValid() && !block.userData()) {
                    block = block.next();
                }
                if (block.isValid()) {
                    disasm->setTextCursor(QTextCursor(block));
                    QApplication::processEvents();
                    std::printf("HEXST: click-disasm sel=%llx size=%llu\n",
                                hexView->selectionStart(),
                                hexView->selectionEnd() - hexView->selectionStart());
                    hex->grab().save(QStringLiteral("/tmp/hex_05_clickdisasm.png"));
                } else {
                    std::printf("HEXST: no insn block\n");
                }

                std::printf("HEXST: DONE\n");
                QApplication::exit(0);
            }
        };
        auto *hst = new HexSelftest(window);
        QTimer::singleShot(500, hst, [hst] { hst->start(); });
    }

    return QApplication::exec();
}

namespace {

constexpr std::string_view PasswordList =
    "https://github.com/ctkqiang/NezhaIDE/releases/download/password-list/rockyou.txt";
constexpr std::string_view RockYouFileName = "rockyou.txt";
constexpr std::string_view RockYouInsertSql = "INSERT INTO rockyou (password) VALUES (?1);";

bool import_rockyou_file(NezhaIDE::Services::DatabaseHelper &db, const QString &file_path) {
    using NezhaIDE::Services::DatabaseHelper;

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "无法打开 RockYou 文件: {}",
            file.errorString().toStdString()
        );
        return false;
    }

    QTextStream stream(&file);
    std::int64_t imported = 0;
    constexpr std::int64_t kBatchSize = 50000;

    while (!stream.atEnd()) {
        std::vector<std::string> batch;
        batch.reserve(kBatchSize);
        while (!stream.atEnd() && static_cast<std::int64_t>(batch.size()) < kBatchSize) {
            const auto line = stream.readLine();
            if (!line.isEmpty()) {
                batch.emplace_back(line.toStdString());
            }
        }
        if (batch.empty()) {
            continue;
        }

        // SQLQuery 每次执行后无 Reset，逐行 Prepare 对百万行太慢；
        // 借 Transaction 暴露的原生句柄做 prepare/bind/step/reset 批量插入
        auto result = db.Transaction([&](sqlite3 *raw) -> DatabaseHelper::Result {
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(raw, RockYouInsertSql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
                return std::unexpected(DatabaseHelper::DatabaseError{
                    sqlite3_errcode(raw), sqlite3_errmsg(raw), std::string(RockYouInsertSql)});
            }
            for (const auto &password : batch) {
                sqlite3_bind_text(stmt, 1, password.data(),
                                  static_cast<int>(password.size()), SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) != SQLITE_DONE) {
                    const auto code = sqlite3_errcode(raw);
                    const auto msg = sqlite3_errmsg(raw);
                    sqlite3_finalize(stmt);
                    return std::unexpected(DatabaseHelper::DatabaseError{code, msg, std::string(RockYouInsertSql)});
                }
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
            }
            sqlite3_finalize(stmt);
            return {};
        });
        if (!result.has_value()) {
            logger.log(
                NezhaIDE::Utilities::LogLevel::Error,
                __FILE__, __LINE__, __func__,
                "RockYou 批量导入失败: {}",
                result.error().Message
            );
            return false;
        }

        imported += static_cast<std::int64_t>(batch.size());
        QCoreApplication::processEvents();
        logger.log(
            NezhaIDE::Utilities::LogLevel::Info,
            __FILE__, __LINE__, __func__,
            "RockYou 已导入 {} 条",
            imported
        );
    }

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__, __LINE__, __func__,
        "RockYou 密码库导入完成，共 {} 条",
        imported
    );
    return true;
}

} // namespace

int onStart() {
    const auto data_dir = ensure_data_directory();
    const auto db_path = data_dir + QStringLiteral("/")
                       + QString::fromUtf8(NezhaIDE::Constants::DatabaseName.data(),
                                           static_cast<int>(NezhaIDE::Constants::DatabaseName.size()));

    NezhaIDE::Services::DatabaseHelper db(db_path.toStdString());
    if (auto result = db.initializeDatabase(); !result.has_value()) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "onStart 数据库初始化失败: {}",
            result.error().Message
        );
        return 1;
    }

    NezhaIDE::Repository::Repository<NezhaIDE::Model::RockYou> rockyou_repo(db);
    if (auto result = rockyou_repo.initialize_schema(); !result.has_value()) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "建表失败 rockyou: {}",
            result.error().Message
        );
        return 1;
    }

    auto count_qr = db.Prepare("SELECT COUNT(*) FROM rockyou;");
    if (!count_qr.has_value()) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "RockYou 计数查询失败: {}",
            count_qr.error().Message
        );
        return 1;
    }
    auto count_step = count_qr->Step();
    if (!count_step.has_value() || !count_step.value()) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Error,
            __FILE__, __LINE__, __func__,
            "RockYou 计数查询失败"
        );
        return 1;
    }
    const auto existing = count_qr->ColumnInt64(0);
    if (existing > 0) {
        logger.log(
            NezhaIDE::Utilities::LogLevel::Info,
            __FILE__, __LINE__, __func__,
            "RockYou 密码库已就绪: {} 条",
            existing
        );
        return 0;
    }

    const auto rockyou_path = data_dir + QStringLiteral("/")
                            + QString::fromUtf8(RockYouFileName.data(),
                                                static_cast<int>(RockYouFileName.size()));

    NezhaIDE::Utilities::DownloadConfig config;
    config.id = 1;
    config.url = std::string(PasswordList);
    config.fileName = std::string(RockYouFileName);
    config.outputfilePath = rockyou_path.toStdString();

    auto &downloader = NezhaIDE::Utilities::Downloader::instance();
    QObject::connect(&downloader, &NezhaIDE::Utilities::Downloader::downloadFinished,
                     &downloader,
                     [=](const int id, const bool success, const QString &error,
                         const QString &file_path) {
                         Q_UNUSED(id);
                         if (!success) {
                             logger.log(
                                 NezhaIDE::Utilities::LogLevel::Error,
                                 __FILE__, __LINE__, __func__,
                                 "RockYou 下载失败: {}",
                                 error.toStdString()
                             );
                             return;
                         }
                         logger.log(
                             NezhaIDE::Utilities::LogLevel::Info,
                             __FILE__, __LINE__, __func__,
                             "RockYou 下载完成: {}",
                             file_path.toStdString()
                         );
                         // onStart 的局部 db 在下载回调前已析构，需重建连接
                         NezhaIDE::Services::DatabaseHelper import_db(db_path.toStdString());
                         if (auto result = import_db.initializeDatabase(); !result.has_value()) {
                             logger.log(
                                 NezhaIDE::Utilities::LogLevel::Error,
                                 __FILE__, __LINE__, __func__,
                                 "RockYou 导入连接失败: {}",
                                 result.error().Message
                             );
                             return;
                         }
                         import_rockyou_file(import_db, file_path);
                     });

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__, __LINE__, __func__,
        "开始下载 RockYou 密码库: {}",
        PasswordList
    );
    downloader.download(config);
    return 0;
}