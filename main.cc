#include "src/utilities/logger.h"

static auto& logger = NezhaIDE::Utilities::Logger::instance();

namespace NezhaIDE {

    int static run() {
        logger.log(
            Utilities::LogLevel::Info,
            __FILE__,
            __LINE__,
            __func__,
            "{}!",
            "IDE 启动中..."
        );

        return 0;
    }
}


int main() {
    return NezhaIDE::run();
}
