//
// Created by 钟智强 on 2026/8/10.
//
#pragma once

#ifndef NEZHAIDE_COLUMN_DEF_H
#define NEZHAIDE_COLUMN_DEF_H

#include <string_view>

namespace NezhaIDE::Model {

    struct ColumnDef {
        std::string_view name;
        std::string_view sql_type;
        bool is_primary{false};
        bool is_autoincrement{false};
    };
}

#endif //NEZHAIDE_COLUMN_DEF_H