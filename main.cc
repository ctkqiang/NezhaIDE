#include "src/configuration.h"
#include "src/model/tool_registry.h"
#include "src/model/user_preference.h"
#include "src/repository/orm.h"
#include "src/services/database_helper.h"
#include "src/services/language_manager.h"
#include "src/utilities/logger.h"
#include "src/views/main_window.h"
#include <QApplication>
#include <QDir>

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

    init_schema(tool_repo, "tools");
    init_schema(proc_repo, "tool_procedures");
    init_schema(param_repo, "tool_parameters");
    init_schema(pref_repo, "user_preferences");

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__, __LINE__, __func__,
        "数据库就绪: {}",
        db_path.toStdString()
    );

    auto &lang_mgr = NezhaIDE::Services::LanguageManager::instance();
    lang_mgr.initialize(app, NezhaIDE::Configuration::instance().language());

    NezhaIDE::Views::MainWindow window;
    window.show();

    return QApplication::exec();
}