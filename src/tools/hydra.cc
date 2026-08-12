//
// Created by 钟智强 on 2026/8/12.
//

#include "hydra.h"
#include "src/configuration.h"
#include "src/model/credential_dataset.h"
#include "src/model/tool_registry.h"
#include "src/repository/orm.h"
#include "src/services/database_helper.h"
#include "src/utilities/logger.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <QUuid>

namespace NezhaIDE::Tools {

namespace {

const QRegularExpression &passwordPattern()
{
    static const QRegularExpression rx(QStringLiteral(R"(password:\s*\S+)"));
    return rx;
}

const QRegularExpression &quotedPassPattern()
{
    static const QRegularExpression rx(QStringLiteral(R"(pass\s+"[^"]*")"));
    return rx;
}

const QRegularExpression &attemptPattern()
{
    static const QRegularExpression rx(QStringLiteral(R"(\[ATTEMPT\])"));
    return rx;
}

const QRegularExpression &loginPattern()
{
    static const QRegularExpression rx(QStringLiteral(R"(login:\s*\S+)"));
    return rx;
}

/**
 * 已知 Hydra 模块的静态元数据。
 *
 * 覆盖本机 hydra -h 输出的全部服务（含未编译项），默认端口取自
 * hydra 各模块的 DEFAULT_PORT。module_args 仅对 -m 有实际意义的
 * 模块置 true，避免向用户暴露无效选项。
 */
struct ModuleDef {
    const char *name;
    const char *display;
    int port;
    bool module_args;
};

constexpr std::array kModules = {
    ModuleDef{"ssh", "SSH", 22, false},
    ModuleDef{"ftp", "FTP", 21, false},
    ModuleDef{"ftps", "FTPS", 990, false},
    ModuleDef{"telnet", "Telnet", 23, false},
    ModuleDef{"http-get", "HTTP GET", 80, true},
    ModuleDef{"http-post", "HTTP POST", 80, true},
    ModuleDef{"http-get-form", "HTTP GET Form", 80, true},
    ModuleDef{"http-post-form", "HTTP POST Form", 80, true},
    ModuleDef{"http-proxy", "HTTP Proxy", 8080, true},
    ModuleDef{"http-proxy-urlenum", "HTTP Proxy URL Enum", 8080, false},
    ModuleDef{"http-head", "HTTP HEAD", 80, false},
    ModuleDef{"smb", "SMB", 445, false},
    ModuleDef{"rdp", "RDP", 3389, false},
    ModuleDef{"mysql", "MySQL", 3306, false},
    ModuleDef{"postgres", "PostgreSQL", 5432, true},
    ModuleDef{"mssql", "MSSQL", 1433, false},
    ModuleDef{"oracle", "Oracle", 1521, false},
    ModuleDef{"ldap2", "LDAP v2", 389, true},
    ModuleDef{"ldap3", "LDAP v3", 389, true},
    ModuleDef{"ldap3s", "LDAP v3 over SSL", 636, true},
    ModuleDef{"imap", "IMAP", 143, false},
    ModuleDef{"pop3", "POP3", 110, false},
    ModuleDef{"smtp", "SMTP", 25, true},
    ModuleDef{"smtp-enum", "SMTP User Enum", 25, true},
    ModuleDef{"vnc", "VNC", 5900, false},
    ModuleDef{"sip", "SIP", 5060, true},
    ModuleDef{"snmp", "SNMP", 161, false},
    ModuleDef{"redis", "Redis", 6379, false},
    ModuleDef{"adam6500", "Adam6500", 10000, false},
    ModuleDef{"afp", "Apple Filing Protocol", 548, false},
    ModuleDef{"asterisk", "Asterisk Manager", 5038, false},
    ModuleDef{"cisco", "Cisco Telnet", 23, false},
    ModuleDef{"cisco-enable", "Cisco Enable", 23, false},
    ModuleDef{"cobaltstrike", "Cobalt Strike", 50050, false},
    ModuleDef{"cvs", "CVS pserver", 2401, false},
    ModuleDef{"firebird", "Firebird", 3050, false},
    ModuleDef{"icq", "ICQ", 5190, false},
    ModuleDef{"irc", "IRC", 6667, false},
    ModuleDef{"memcached", "Memcached", 11211, false},
    ModuleDef{"mongodb", "MongoDB", 27017, false},
    ModuleDef{"mysql5", "MySQL v5", 3306, false},
    ModuleDef{"ncp", "NCP", 2038, false},
    ModuleDef{"nntp", "NNTP", 119, false},
    ModuleDef{"pcanywhere", "pcAnywhere", 5631, false},
    ModuleDef{"pcnfs", "PCNFS", 4045, false},
    ModuleDef{"radmin2", "Radmin2", 4899, false},
    ModuleDef{"rexec", "rexec", 512, false},
    ModuleDef{"rlogin", "rlogin", 513, false},
    ModuleDef{"rpcap", "rpcap", 2002, false},
    ModuleDef{"rsh", "rsh", 514, false},
    ModuleDef{"rtsp", "RTSP", 554, false},
    ModuleDef{"s7-300", "Siemens S7-300", 102, false},
    ModuleDef{"sapr3", "SAP R/3", 3200, false},
    ModuleDef{"smb2", "SMBv2", 445, false},
    ModuleDef{"socks5", "SOCKS5", 1080, false},
    ModuleDef{"sshkey", "SSH Key", 22, false},
    ModuleDef{"svn", "SVN", 3690, false},
    ModuleDef{"teamspeak", "TeamSpeak", 9987, true},
    ModuleDef{"vmauthd", "VMware Authd", 902, false},
    ModuleDef{"xmpp", "XMPP", 5222, false},
};

QString databasePath()
{
#ifdef NEZHA_PROJECT_ROOT
    const auto dataDir = QStringLiteral(NEZHA_PROJECT_ROOT) + QStringLiteral("/data");
#else
    const auto dataDir = QDir::currentPath() + QStringLiteral("/data");
#endif
    QDir dir;
    dir.mkpath(dataDir);
    return dataDir + QStringLiteral("/") + QString::fromUtf8(
        NezhaIDE::Constants::DatabaseName.data(),
        static_cast<int>(NezhaIDE::Constants::DatabaseName.size()));
}

/**
 * 递归展开 hydra 帮助输出中的服务 token。
 *
 * 处理 {a|b} 多选组（http-{head|get|post} → 三个服务）、尾部
 * [s]（ldap3[s] → ldap3/ldap3s）与 (v4) 等括号批注（仅剥离）。
 */
void expandToken(const QString &token, QStringList *out)
{
    static const QRegularExpression braces(QStringLiteral(R"(\{([^{}]+)\})"));
    auto m = braces.match(token);
    if (m.hasMatch()) {
        for (const auto &alt : m.captured(1).split(QLatin1Char('|'), Qt::SkipEmptyParts)) {
            QString next = token;
            next.replace(m.capturedStart(), m.capturedLength(), alt);
            expandToken(next, out);
        }
        return;
    }

    static const QRegularExpression bracket(QStringLiteral(R"(^(.*)\[([^\]]+)\]$)"));
    m = bracket.match(token);
    if (m.hasMatch()) {
        expandToken(m.captured(1), out);
        expandToken(m.captured(1) + m.captured(2), out);
        return;
    }

    static const QRegularExpression parens(QStringLiteral(R"(\([^)]*\))"));
    QString cleaned = token;
    cleaned.remove(parens);
    cleaned = cleaned.trimmed();
    if (!cleaned.isEmpty()) {
        out->append(cleaned);
    }
}

/**
 * 解析 "Supported services:" / "These services were not compiled in:" 行。
 *
 * expand_paren_contents 为 true 时，括号内逗号分隔的内容（如
 * "SSL-services (ftps, sip, rdp, ...)" 中的 ftps/sip/rdp）也作为
 * 独立 token 输出。
 */
QStringList parseServiceLine(const QString &line, bool expandParenContents)
{
    QStringList out;
    if (expandParenContents) {
        static const QRegularExpression parenContents(QStringLiteral(R"(\(([^)]*)\))"));
        auto it = parenContents.globalMatch(line);
        while (it.hasNext()) {
            const auto m = it.next();
            for (auto &part : m.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
                const auto word = part.trimmed();
                if (!word.isEmpty() && word != QStringLiteral("...")) {
                    out.append(word);
                }
            }
        }
    }
    const auto tokens = line.split(QRegularExpression(QStringLiteral(R"(\s+)")), Qt::SkipEmptyParts);
    for (const auto &token : tokens) {
        expandToken(token, &out);
    }
    out.removeDuplicates();
    return out;
}

} // namespace

HydraService &HydraService::instance()
{
    static HydraService svc;
    return svc;
}

HydraService::HydraService()
    : process_(new QProcess(this)), probe_(new QProcess(this)),
      nam_(new QNetworkAccessManager(this))
{
    // 独立连接同一数据库文件（WAL 支持多连接），确保数据集表存在
    NezhaIDE::Services::DatabaseHelper db(databasePath().toStdString());
    if (const auto result = db.initializeDatabase(); !result.has_value()) {
        NezhaIDE::Utilities::Logger::instance().log(
            NezhaIDE::Utilities::LogLevel::Error, __FILE__, __LINE__, __func__,
            "Hydra 数据库连接失败: {}", result.error().Message);
    } else {
        NezhaIDE::Repository::Repository<NezhaIDE::Model::CredentialDataset> datasetRepo(db);
        NezhaIDE::Repository::Repository<NezhaIDE::Model::CredentialEntry> entryRepo(db);
        if (const auto result = datasetRepo.initialize_schema(); !result.has_value()) {
            NezhaIDE::Utilities::Logger::instance().log(
                NezhaIDE::Utilities::LogLevel::Error, __FILE__, __LINE__, __func__,
                "credential_datasets 建表失败: {}", result.error().Message);
        }
        if (const auto result = entryRepo.initialize_schema(); !result.has_value()) {
            NezhaIDE::Utilities::Logger::instance().log(
                NezhaIDE::Utilities::LogLevel::Error, __FILE__, __LINE__, __func__,
                "credential_entries 建表失败: {}", result.error().Message);
        }
    }

    seedToolsDatabase();
    probeInstalledModules();

    connect(process_, &QProcess::readyReadStandardOutput,
            this, &HydraService::onProcessOutput);
    connect(process_, &QProcess::readyReadStandardError,
            this, &HydraService::onProcessOutput);
    connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &HydraService::onProcessFinished);
    connect(probe_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &HydraService::onProbeFinished);
}

HydraService::~HydraService() = default;

QList<HydraService::ModuleInfo> HydraService::knownModules() const
{
    QList<ModuleInfo> modules;
    modules.reserve(kModules.size());
    for (const auto &mod : kModules) {
        modules.append({QString::fromUtf8(mod.name), QString::fromUtf8(mod.display), mod.port});
    }
    return modules;
}

QStringList HydraService::availableModules() const
{
    return installed_modules_;
}

QStringList HydraService::unavailableModules() const
{
    return unavailable_modules_;
}

bool HydraService::isModuleInstalled(const QString &service) const
{
    return installed_modules_.contains(service);
}

bool HydraService::hasInstalledModules() const noexcept
{
    return !installed_modules_.isEmpty();
}

QList<ModuleParam> HydraService::moduleParams(const QString &service) const
{
    QList<ModuleParam> params;
    if (service.isEmpty()) {
        return params;
    }

    NezhaIDE::Services::DatabaseHelper db(databasePath().toStdString());
    if (const auto result = db.initializeDatabase(); !result.has_value()) {
        return params;
    }

    auto procQr = db.Prepare(
        "SELECT id FROM tool_procedures WHERE tool_id ="
        " (SELECT id FROM tools WHERE name = 'hydra') AND name = ?;");
    if (!procQr.has_value()) {
        return params;
    }
    auto &procQuery = procQr.value();
    if (!procQuery.Bind(1, service.toStdString()).has_value()) {
        return params;
    }
    const auto procStep = procQuery.Step();
    if (!procStep.has_value() || !procStep.value()) {
        return params;
    }
    const auto procedureId = procQuery.ColumnInt64(0);

    auto paramQr = db.Prepare(
        "SELECT name, type, description, default_value FROM tool_parameters"
        " WHERE procedure_id = ? ORDER BY position;");
    if (!paramQr.has_value()) {
        return params;
    }
    auto &paramQuery = paramQr.value();
    if (!paramQuery.Bind(1, procedureId).has_value()) {
        return params;
    }
    while (true) {
        const auto step = paramQuery.Step();
        if (!step.has_value() || !step.value()) {
            break;
        }
        ModuleParam param;
        param.name = QString::fromStdString(paramQuery.ColumnText(0));
        param.type = QString::fromStdString(paramQuery.ColumnText(1));
        param.help = QString::fromStdString(paramQuery.ColumnText(2));
        param.default_value = QString::fromStdString(paramQuery.ColumnText(3));
        param.display = param.name;
        params.append(param);
    }
    return params;
}

HydraService::LoadResult HydraService::loadUsernameFile(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        return {0, 0, QStringLiteral("unreadable")};
    }

    QStringList lines;
    if (!readLines(path, &lines)) {
        return {0, 0, QStringLiteral("unreadable")};
    }
    if (lines.isEmpty()) {
        return {0, 0, QStringLiteral("empty")};
    }

    const auto result = importLines(info.fileName(), QStringLiteral("username"),
                                    QStringLiteral("file"), path, lines);
    if (result.ok()) {
        username_dataset_id_ = result.dataset_id;
        emit usernameDatasetChanged(result.entry_count);
    }
    return result;
}

HydraService::LoadResult HydraService::loadSingleUsername(const QString &username)
{
    const auto name = username.trimmed();
    if (name.isEmpty()) {
        return {0, 0, QStringLiteral("empty")};
    }

    const auto result = importLines(name, QStringLiteral("username"),
                                    QStringLiteral("custom"), QStringLiteral("single-user"),
                                    {name});
    if (result.ok()) {
        username_dataset_id_ = result.dataset_id;
        emit usernameDatasetChanged(result.entry_count);
    }
    return result;
}

HydraService::LoadResult HydraService::loadPasswordFile(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        return {0, 0, QStringLiteral("unreadable")};
    }

    QStringList lines;
    if (!readLines(path, &lines)) {
        return {0, 0, QStringLiteral("unreadable")};
    }
    if (lines.isEmpty()) {
        return {0, 0, QStringLiteral("empty")};
    }

    const auto result = importLines(info.fileName(), QStringLiteral("password"),
                                    QStringLiteral("custom"), path, lines);
    if (result.ok()) {
        password_dataset_id_ = result.dataset_id;
        last_password_provenance_ = path;
        emit passwordDatasetChanged(result.entry_count, last_password_provenance_);
    }
    return result;
}

bool HydraService::readLines(const QString &path, QStringList *out) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    while (!file.atEnd()) {
        auto line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty()) continue;
        if (line.size() > 4096) line = line.left(4096);
        out->append(line);
    }
    return true;
}

HydraService::LoadResult HydraService::generateRandomPasswords(int count, int min_length, int max_length)
{
    if (count <= 0 || count > 1'000'000) {
        return {0, 0, QStringLiteral("invalid_count")};
    }
    if (min_length <= 0 || max_length < min_length || max_length > 512) {
        return {0, 0, QStringLiteral("invalid_length")};
    }

    static constexpr const char kCharset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QStringList lines;
    lines.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int len = QRandomGenerator::global()->bounded(min_length, max_length + 1);
        QString pass;
        pass.reserve(len);
        for (int j = 0; j < len; ++j) {
            pass.append(QLatin1Char(kCharset[QRandomGenerator::global()->bounded(62)]));
        }
        lines.append(pass);
    }

    const auto name = QStringLiteral("random-%1").arg(count);
    const auto result = importLines(name, QStringLiteral("password"),
                                    QStringLiteral("generator"), {}, lines);
    if (result.ok()) {
        password_dataset_id_ = result.dataset_id;
        last_password_provenance_ = name;
        emit passwordDatasetChanged(result.entry_count, last_password_provenance_);
    }
    return result;
}

void HydraService::importGithubPasswords(const QString &url)
{
    const QUrl parsed(url);
    if (parsed.scheme() != QStringLiteral("https") || parsed.host().isEmpty()) {
        emit githubImportFinished(0, QStringLiteral("invalid_url"));
        return;
    }

    pending_github_url_ = url;
    auto *reply = nam_->get(QNetworkRequest(parsed));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        onGithubReplyFinished(reply);
    });
}

void HydraService::onGithubReplyFinished(QNetworkReply *reply)
{
    const auto url = pending_github_url_;
    const auto ok = reply->error() == QNetworkReply::NoError;
    const auto errorCode = reply->error();
    const auto data = reply->readAll();
    reply->deleteLater();

    if (!ok) {
        emit githubImportFinished(0, QStringLiteral("http_%1").arg(errorCode));
        return;
    }

    QStringList lines;
    for (auto &line : QString::fromUtf8(data).split(QLatin1Char('\n'))) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        if (line.size() > 4096) line = line.left(4096);
        lines.append(line);
    }
    if (lines.isEmpty()) {
        emit githubImportFinished(0, QStringLiteral("empty"));
        return;
    }

    const QUrl parsed(url);
    const auto name = QStringLiteral("github:%1").arg(parsed.host());
    const auto result = importLines(name, QStringLiteral("password"),
                                    QStringLiteral("github"), url, lines);
    if (result.ok()) {
        password_dataset_id_ = result.dataset_id;
        last_password_provenance_ = url;
        emit passwordDatasetChanged(result.entry_count, last_password_provenance_);
    }
    emit githubImportFinished(result.entry_count, result.error);
}

HydraService::LoadResult HydraService::importLines(
    const QString &name, const QString &type, const QString &source,
    const QString &filePath, const QStringList &lines)
{
    NezhaIDE::Services::DatabaseHelper db(databasePath().toStdString());
    if (const auto result = db.initializeDatabase(); !result.has_value()) {
        return {0, 0, QString::fromStdString(result.error().Message)};
    }

    // BEGIN/COMMIT 包裹批量导入，避免逐条提交
    if (const auto result = db.Execute("BEGIN;"); !result.has_value()) {
        return {0, 0, QString::fromStdString(result.error().Message)};
    }

    const auto now = QDateTime::currentDateTime().toString(Qt::ISODate);

    auto insertDataset = [&]() -> std::expected<std::int64_t, QString> {
        auto qr = db.Prepare(
            "INSERT INTO credential_datasets"
            " (name, dataset_type, source, file_path, imported_at, entry_count)"
            " VALUES (?, ?, ?, ?, ?, ?);");
        if (!qr.has_value()) return std::unexpected(QStringLiteral("db_prepare"));
        auto &query = qr.value();
        if (!query.Bind(1, name.toStdString()).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Bind(2, type.toStdString()).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Bind(3, source.toStdString()).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Bind(4, filePath.toStdString()).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Bind(5, now.toStdString()).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Bind(6, static_cast<std::int64_t>(lines.size())).has_value()) return std::unexpected(QStringLiteral("db_bind"));
        if (!query.Execute().has_value()) return std::unexpected(QStringLiteral("db_insert"));
        return db.last_insert_rowid();
    };

    const auto datasetResult = insertDataset();
    if (!datasetResult.has_value()) {
        [[maybe_unused]] const auto rollback = db.Execute("ROLLBACK;");
        return {0, 0, datasetResult.error()};
    }
    const auto datasetId = datasetResult.value();

    // SQLQuery 无 Reset，每条记录需独立 prepare；逐条插入
    for (int i = 0; i < lines.size(); ++i) {
        auto qr = db.Prepare(
            "INSERT INTO credential_entries (dataset_id, value, line_number)"
            " VALUES (?, ?, ?);");
        if (!qr.has_value()) {
            [[maybe_unused]] const auto rollback = db.Execute("ROLLBACK;");
            return {0, 0, QStringLiteral("db_prepare")};
        }
        auto &query = qr.value();
        if (!query.Bind(1, static_cast<std::int64_t>(datasetId)).has_value()
            || !query.Bind(2, lines[i].toStdString()).has_value()
            || !query.Bind(3, static_cast<std::int64_t>(i + 1)).has_value()) {
            [[maybe_unused]] const auto rollback = db.Execute("ROLLBACK;");
            return {0, 0, QStringLiteral("db_bind")};
        }
        if (!query.Execute().has_value()) {
            [[maybe_unused]] const auto rollback = db.Execute("ROLLBACK;");
            return {0, 0, QStringLiteral("db_insert")};
        }
    }

    if (const auto result = db.Execute("COMMIT;"); !result.has_value()) {
        return {0, 0, QString::fromStdString(result.error().Message)};
    }

    return {datasetId, static_cast<int>(lines.size()), {}};
}

void HydraService::seedToolsDatabase()
{
    NezhaIDE::Services::DatabaseHelper db(databasePath().toStdString());
    if (const auto result = db.initializeDatabase(); !result.has_value()) {
        return;
    }

    // 已播种则跳过，不覆盖用户对工具定义的修改
    auto countQr = db.Prepare("SELECT COUNT(*) FROM tools WHERE name = 'hydra';");
    if (!countQr.has_value()) {
        return;
    }
    auto &countQuery = countQr.value();
    const auto countStep = countQuery.Step();
    if (!countStep.has_value() || !countStep.value() || countQuery.ColumnInt64(0) > 0) {
        return;
    }

    NezhaIDE::Repository::Repository<NezhaIDE::Model::Tool> toolRepo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::ToolProcedure> procRepo(db);
    NezhaIDE::Repository::Repository<NezhaIDE::Model::ToolParameter> paramRepo(db);

    NezhaIDE::Model::Tool tool;
    tool.uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    tool.name = "hydra";
    tool.display_name = "THC-Hydra";
    tool.executable_path = "hydra";
    tool.version = "9.6dev";
    tool.enabled = true;
    if (!toolRepo.save(tool).has_value()) {
        return;
    }

    for (const auto &mod : kModules) {
        NezhaIDE::Model::ToolProcedure proc;
        proc.tool_id = tool.id;
        proc.name = mod.name;
        proc.description = mod.display;
        proc.input_type = "username:password";
        proc.output_type = "credential";
        if (!procRepo.save(proc).has_value()) {
            continue;
        }
        if (!mod.module_args) {
            continue;
        }
        NezhaIDE::Model::ToolParameter param;
        param.procedure_id = proc.id;
        param.name = "module_args";
        param.type = "string";
        param.description = "Additional module-specific arguments passed via -m";
        param.required = false;
        param.position = 1;
        paramRepo.save(param);
    }
}

void HydraService::probeInstalledModules()
{
    if (probe_started_) {
        return;
    }
    probe_started_ = true;

    const auto binary = QStandardPaths::findExecutable(QStringLiteral("hydra"));
    if (binary.isEmpty()) {
        // 无法探测：全部模块视为不可用，UI 据此禁用并提示安装
        installed_modules_.clear();
        unavailable_modules_.clear();
        for (const auto &mod : kModules) {
            unavailable_modules_.append(QString::fromUtf8(mod.name));
        }
        probe_started_ = false;
        emit modulesProbed();
        return;
    }

    probe_->start(binary, {QStringLiteral("-h")});
}

void HydraService::onProbeFinished()
{
    probe_started_ = false;
    installed_modules_.clear();
    unavailable_modules_.clear();

    const auto output = probe_->readAllStandardOutput();
    for (const auto &line : QString::fromUtf8(output).split(QLatin1Char('\n'))) {
        const auto trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("Supported services:"))) {
            installed_modules_ = parseServiceLine(
                trimmed.mid(trimmed.indexOf(QLatin1Char(':')) + 1), false);
        } else if (trimmed.startsWith(QStringLiteral("These services were not compiled in:"))) {
            unavailable_modules_ = parseServiceLine(
                trimmed.mid(trimmed.indexOf(QLatin1Char(':')) + 1), true);
        }
    }

    emit modulesProbed();
}

void HydraService::run(const HydraConfig &config)
{
    if (isRunning()) {
        emit runStateChanged(HydraState::Error);
        appendLog(QStringLiteral("Hydra 已在运行"));
        return;
    }
    if (config.service.trimmed().isEmpty()) {
        emit runStateChanged(HydraState::SelectService);
        return;
    }
    if (username_dataset_id_ == 0) {
        emit runStateChanged(HydraState::NoUsernameFile);
        return;
    }
    if (password_dataset_id_ == 0) {
        emit runStateChanged(HydraState::SelectPasswordSource);
        return;
    }
    if (config.target_host.trimmed().isEmpty()) {
        emit runStateChanged(HydraState::Error);
        appendLog(QStringLiteral("目标主机不能为空"));
        return;
    }

    const auto binary = QStandardPaths::findExecutable(QStringLiteral("hydra"));
    if (binary.isEmpty()) {
        emit runStateChanged(HydraState::Error);
        appendLog(QStringLiteral("未找到 hydra 二进制，请先安装 THC-Hydra"));
        return;
    }

    // 从 SQL 数据集导出临时凭据文件，hydra 只接触临时文件
    const auto exportDataset = [this](std::int64_t datasetId) -> std::unique_ptr<QTemporaryFile> {
        auto *file = new QTemporaryFile(QDir::temp().filePath(QStringLiteral("nezha_hydra_XXXXXX")), this);
        if (!file->open()) {
            file->deleteLater();
            return nullptr;
        }

        NezhaIDE::Services::DatabaseHelper db(databasePath().toStdString());
        if (const auto result = db.initializeDatabase(); !result.has_value()) {
            file->deleteLater();
            return nullptr;
        }
        auto qr = db.Prepare(
            "SELECT value FROM credential_entries WHERE dataset_id = ? ORDER BY line_number;");
        if (!qr.has_value()) {
            file->deleteLater();
            return nullptr;
        }
        auto &query = qr.value();
        if (!query.Bind(1, static_cast<std::int64_t>(datasetId)).has_value()) {
            file->deleteLater();
            return nullptr;
        }
        while (true) {
            const auto step = query.Step();
            if (!step.has_value() || !step.value()) break;
            const auto value = query.ColumnText(0);
            file->write(value.data(), static_cast<qint64>(value.size()));
            file->write("\n", 1);
        }
        file->flush();
        return std::unique_ptr<QTemporaryFile>(file);
    };

    user_file_ = exportDataset(username_dataset_id_);
    pass_file_ = exportDataset(password_dataset_id_);
    if (!user_file_ || !pass_file_) {
        user_file_.reset();
        pass_file_.reset();
        emit runStateChanged(HydraState::Error);
        appendLog(QStringLiteral("凭据数据集导出失败"));
        return;
    }

    QStringList args;
    args << QStringLiteral("-L") << user_file_->fileName();
    args << QStringLiteral("-P") << pass_file_->fileName();
    args << QStringLiteral("-t") << QString::number(config.threads);
    args << QStringLiteral("-w") << QString::number(config.timeout_seconds);
    args << QStringLiteral("-s") << QString::number(config.target_port);
    if (!config.try_mode.trimmed().isEmpty()) {
        args << QStringLiteral("-e") << config.try_mode.trimmed();
    }
    if (config.exit_on_first) {
        args << QStringLiteral("-f");
    }
    if (config.verbose) {
        args << QStringLiteral("-v") << QStringLiteral("-V");
    }
    const auto moduleArgs = config.module_params.value(QStringLiteral("module_args"));
    if (!moduleArgs.trimmed().isEmpty()) {
        args << QStringLiteral("-m") << moduleArgs.trimmed();
    }
    if (!config.extra_args.trimmed().isEmpty()) {
        args << config.extra_args.trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    }
    args << QStringLiteral("%1://%2:%3")
                .arg(config.service.trimmed(), config.target_host.trimmed())
                .arg(config.target_port);

    attempt_count_ = 0;
    found_count_ = 0;
    stopped_ = false;
    appendLog(QStringLiteral("启动 hydra: %1 %2").arg(binary, args.join(QLatin1Char(' '))));
    emit runStateChanged(HydraState::Running);
    process_->start(binary, args);
}

void HydraService::stop()
{
    if (!isRunning()) return;
    stopped_ = true;
    process_->terminate();
    QTimer::singleShot(1500, this, [this] {
        if (isRunning()) {
            process_->kill();
        }
    });
    appendLog(QStringLiteral("正在停止 hydra…"));
}

bool HydraService::isRunning() const noexcept
{
    return process_->state() != QProcess::NotRunning;
}

void HydraService::onProcessOutput()
{
    const auto raw = process_->readAllStandardOutput() + process_->readAllStandardError();
    for (auto &line : QString::fromUtf8(raw).split(QLatin1Char('\n'))) {
        if (line.trimmed().isEmpty()) continue;

        if (attemptPattern().match(line).hasMatch()) {
            ++attempt_count_;
        }
        // 成功凭据行形如 "login: root  password: 12345"，先计数再打码
        if (loginPattern().match(line).hasMatch() && line.contains(QStringLiteral("password:"))) {
            ++found_count_;
        }
        line.replace(passwordPattern(), QStringLiteral("password: ****"));
        line.replace(quotedPassPattern(), QStringLiteral("pass \"****\""));
        appendLog(line);
    }
}

void HydraService::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    user_file_.reset();
    pass_file_.reset();

    if (stopped_) {
        stopped_ = false;
        appendLog(QStringLiteral("hydra 已停止"));
        emit runStateChanged(HydraState::Stopped);
        emit runFinished(false, attempt_count_, found_count_, QStringLiteral("stopped"));
        return;
    }

    if (status == QProcess::CrashExit) {
        emit runStateChanged(HydraState::Error);
        appendLog(QStringLiteral("hydra 进程异常退出"));
        emit runFinished(false, attempt_count_, found_count_, QStringLiteral("crash"));
        return;
    }

    // hydra 找到凭据退出码为 0，未找到为 1；两者均为正常完成
    const auto summary = QStringLiteral("退出码 %1，尝试 %2 次，发现 %3 组凭据")
                             .arg(exitCode).arg(attempt_count_).arg(found_count_);
    appendLog(summary);
    emit runStateChanged(HydraState::Completed);
    emit runFinished(exitCode == 0, attempt_count_, found_count_, summary);
}

void HydraService::appendLog(const QString &line)
{
    NezhaIDE::Utilities::Logger::instance().log(
        NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "Hydra: {}", line.toStdString());
    emit logLine(line);
}

std::int64_t HydraService::lastUsernameDatasetId() const noexcept
{
    return username_dataset_id_;
}

std::int64_t HydraService::lastPasswordDatasetId() const noexcept
{
    return password_dataset_id_;
}

} // namespace NezhaIDE::Tools
