#include "localization_service.h"
#include "src/configuration.h"
#include "src/utilities/logger.h"
#include <QCoreApplication>
#include <QFile>
#include <QXmlStreamReader>

namespace NezhaIDE::Services {

LocalizationService &LocalizationService::instance()
{
    static LocalizationService svc;
    return svc;
}

void LocalizationService::initialize(IDELanguage language)
{
    auto& lgr = NezhaIDE::Utilities::Logger::instance();
    lgr.log(NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "语言初始化: {}", static_cast<int>(language));
    current_language_ = language;
    loadXml(xmlPath(language));
}

void LocalizationService::switchLanguage(IDELanguage language)
{
    if (language == current_language_) return;

    auto& lgr = NezhaIDE::Utilities::Logger::instance();
    lgr.log(NezhaIDE::Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "语言切换: {}", static_cast<int>(language));
    current_language_ = language;
    loadXml(xmlPath(language));

    NezhaIDE::Configuration::instance().set_language(language);
    NezhaIDE::Configuration::instance().save();

    emit languageChanged(language);
}

QString LocalizationService::translate(const QString &key) const
{
    return strings_.value(key, key);
}

IDELanguage LocalizationService::currentLanguage() const noexcept
{
    return current_language_;
}

void LocalizationService::loadXml(const QString &path)
{
    strings_.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        NezhaIDE::Utilities::Logger::instance().log(
            NezhaIDE::Utilities::LogLevel::Warn, __FILE__, __LINE__, __func__,
            "无法加载语言文件: {}", path.toStdString());
        return;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("string")) {
            const auto key = xml.attributes().value("key").toString();
            const auto value = xml.readElementText();
            if (!key.isEmpty()) {
                strings_[key] = value;
            }
        }
    }
}

QString LocalizationService::xmlPath(IDELanguage language)
{
#ifdef NEZHA_PROJECT_ROOT
    const auto dir = QStringLiteral(NEZHA_PROJECT_ROOT) + QStringLiteral("/resources/localisation");
#else
    const auto dir = QCoreApplication::applicationDirPath() + QStringLiteral("/resources/localisation");
#endif
    switch (language) {
    case NezhaIDE::IDELanguage::Chinese:
        return dir + QStringLiteral("/chinese.xml");
    case NezhaIDE::IDELanguage::English:
        return dir + QStringLiteral("/english.xml");
    case NezhaIDE::IDELanguage::German:
        return dir + QStringLiteral("/german.xml");
    }
    return {};
}

} // namespace NezhaIDE::Services
