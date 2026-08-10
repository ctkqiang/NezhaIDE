#include "src/utilities/logger.h"
#include "src/views/main_window.h"
#include <QApplication>

#include "src/configuration.h"

static auto& logger = NezhaIDE::Utilities::Logger::instance();

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName(NezhaIDE::Constants::ApplicationName.data());
    QApplication::setApplicationVersion(NezhaIDE::Constants::ApplicationVersion.data());
    QApplication::setOrganizationName(NezhaIDE::Constants::OrganisationName.data());

    logger.log(
        NezhaIDE::Utilities::LogLevel::Info,
        __FILE__,
        __LINE__,
        __func__,
        "{}!",
        "IDE 启动中..."
    );

    NezhaIDE::Views::MainWindow window;
    window.show();

    return QApplication::exec();
}
