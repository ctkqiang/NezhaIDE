//
// Created by 钟智强 on 2026/8/11.
//

#pragma once

#ifndef NEZHAIDE_TOOLS_H
#define NEZHAIDE_TOOLS_H

#include <string>
#include <vector>

namespace NezhaIDE::Model {
    enum class ToolParameterType {
        Flag,
        String,
        Integer,
        Boolean,
        Enum,
        Path,
        File,
        Directory,
        Target,
        Port,
        PortRange
    };

    struct ToolParameter {
        int id{0};

        std::string parameter_name;
        std::string parameter_description;

        std::string parameter_short_name;
        std::string parameter_long_name;

        std::string parameter_default_value;

        ToolParameterType parameter_type{
            ToolParameterType::String
        };

        bool parameter_required{false};
        bool parameter_multiple{false};
        bool parameter_positional{false};
        bool parameter_enabled{true};

        std::vector<std::string> parameter_choices;
    };

    struct Tools {
        int id{0};

        std::string tool_name;
        std::string tool_description;

        std::string tool_path;
        std::string tool_version;

        std::string tool_author;
        std::string tool_homepage;

        std::vector<std::string> tool_tags;

        std::vector<ToolParameter> tool_parameters;
    };
}

#endif // NEZHAIDE_TOOLS_H
