//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_USER_PREFERENCE_H
#define NEZHAIDE_USER_PREFERENCE_H

#include <string>
#include "src/configuration.h"

namespace NezhaIDE {
    enum class IDELanguage;
}

namespace NezhaIDE::Model {

    struct UserPreference {
        int id{0};

        [[ maybe_unused ]]
        std::string user_name;

        IDETheme theme;
        IDELanguage language;
        IDELLM llm;
        std::string llm_api_token;
    };
}

#endif //NEZHAIDE_USER_PREFERENCE_H
