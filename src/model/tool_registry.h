//
// Created by 钟智强 on 2026/8/10.
//

#ifndef NEZHAIDE_TOOL_REGISTRY_H
#define NEZHAIDE_TOOL_REGISTRY_H

#include "src/model/column_def.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace NezhaIDE::Model {

    struct ToolParameter final {
        std::int64_t id{0};
        std::int64_t procedure_id{0};
        std::string name;
        std::string type;
        std::string description;
        std::string default_value;
        bool required{false};
        std::int32_t position{0};

        static consteval std::string_view table_name() {
            return "tool_parameters";
        }

        static consteval auto columns() {
            return std::array{
                ColumnDef{"id", "INTEGER", true, true},
                ColumnDef{"procedure_id", "INTEGER", false, false},
                ColumnDef{"name", "TEXT", false, false},
                ColumnDef{"type", "TEXT", false, false},
                ColumnDef{"description", "TEXT", false, false},
                ColumnDef{"default_value", "TEXT", false, false},
                ColumnDef{"required", "INTEGER", false, false},
                ColumnDef{"position", "INTEGER", false, false},
            };
        }

        template<typename Query>
        void bind_values(Query& query, const bool for_insert) const {
            if (for_insert) {
                query.Bind(1, static_cast<std::int64_t>(procedure_id));
                query.Bind(2, name);
                query.Bind(3, type);
                query.Bind(4, description);
                query.Bind(5, default_value);
                query.Bind(6, required);
                query.Bind(7, static_cast<std::int64_t>(position));
            } else {
                query.Bind(1, static_cast<std::int64_t>(procedure_id));
                query.Bind(2, name);
                query.Bind(3, type);
                query.Bind(4, description);
                query.Bind(5, default_value);
                query.Bind(6, required);
                query.Bind(7, static_cast<std::int64_t>(position));
                query.Bind(8, static_cast<std::int64_t>(id));
            }
        }

        template<typename Query>
        static ToolParameter from_query(Query& query) {
            ToolParameter p;
            p.id = static_cast<std::int64_t>(query.ColumnInt64(0));
            p.procedure_id = static_cast<std::int64_t>(query.ColumnInt64(1));
            p.name = query.ColumnText(2);
            p.type = query.ColumnText(3);
            p.description = query.ColumnText(4);
            p.default_value = query.ColumnText(5);
            p.required = query.ColumnBool(6);
            p.position = static_cast<std::int32_t>(query.ColumnInt64(7));
            return p;
        }
    };

    struct ToolProcedure final {
        std::int64_t id{0};
        std::int64_t tool_id{0};
        std::string name;
        std::string description;
        std::string input_type;
        std::string output_type;
        std::vector<ToolParameter> parameters;

        static consteval std::string_view table_name() {
            return "tool_procedures";
        }

        static consteval auto columns() {
            return std::array{
                ColumnDef{"id", "INTEGER", true, true},
                ColumnDef{"tool_id", "INTEGER", false, false},
                ColumnDef{"name", "TEXT", false, false},
                ColumnDef{"description", "TEXT", false, false},
                ColumnDef{"input_type", "TEXT", false, false},
                ColumnDef{"output_type", "TEXT", false, false},
            };
        }

        template<typename Query>
        void bind_values(Query& query, const bool for_insert) const {
            if (for_insert) {
                query.Bind(1, static_cast<std::int64_t>(tool_id));
                query.Bind(2, name);
                query.Bind(3, description);
                query.Bind(4, input_type);
                query.Bind(5, output_type);
            } else {
                query.Bind(1, static_cast<std::int64_t>(tool_id));
                query.Bind(2, name);
                query.Bind(3, description);
                query.Bind(4, input_type);
                query.Bind(5, output_type);
                query.Bind(6, static_cast<std::int64_t>(id));
            }
        }

        template<typename Query>
        static ToolProcedure from_query(Query& query) {
            ToolProcedure p;
            p.id = static_cast<std::int64_t>(query.ColumnInt64(0));
            p.tool_id = static_cast<std::int64_t>(query.ColumnInt64(1));
            p.name = query.ColumnText(2);
            p.description = query.ColumnText(3);
            p.input_type = query.ColumnText(4);
            p.output_type = query.ColumnText(5);
            return p;
        }
    };

    struct Tool final {
        std::int64_t id{0};
        std::string uuid;
        std::string name;
        std::string display_name;
        std::string executable_path;
        std::string manifest_path;
        std::string version;
        std::string configuration_json;
        bool enabled{true};
        std::vector<ToolProcedure> procedures;

        static consteval std::string_view table_name() {
            return "tools";
        }

        static consteval auto columns() {
            return std::array{
                ColumnDef{"id", "INTEGER", true, true},
                ColumnDef{"uuid", "TEXT", false, false},
                ColumnDef{"name", "TEXT", false, false},
                ColumnDef{"display_name", "TEXT", false, false},
                ColumnDef{"executable_path", "TEXT", false, false},
                ColumnDef{"manifest_path", "TEXT", false, false},
                ColumnDef{"version", "TEXT", false, false},
                ColumnDef{"configuration_json", "TEXT", false, false},
                ColumnDef{"enabled", "INTEGER", false, false},
            };
        }

        template<typename Query>
        void bind_values(Query& query, const bool for_insert) const {
            if (for_insert) {
                query.Bind(1, uuid);
                query.Bind(2, name);
                query.Bind(3, display_name);
                query.Bind(4, executable_path);
                query.Bind(5, manifest_path);
                query.Bind(6, version);
                query.Bind(7, configuration_json);
                query.Bind(8, enabled);
            } else {
                query.Bind(1, uuid);
                query.Bind(2, name);
                query.Bind(3, display_name);
                query.Bind(4, executable_path);
                query.Bind(5, manifest_path);
                query.Bind(6, version);
                query.Bind(7, configuration_json);
                query.Bind(8, enabled);
                query.Bind(9, static_cast<std::int64_t>(id));
            }
        }

        template<typename Query>
        static Tool from_query(Query& query) {
            Tool t;
            t.id = static_cast<std::int64_t>(query.ColumnInt64(0));
            t.uuid = query.ColumnText(1);
            t.name = query.ColumnText(2);
            t.display_name = query.ColumnText(3);
            t.executable_path = query.ColumnText(4);
            t.manifest_path = query.ColumnText(5);
            t.version = query.ColumnText(6);
            t.configuration_json = query.ColumnText(7);
            t.enabled = query.ColumnBool(8);
            return t;
        }
    };

    struct ToolRegistry final {
        std::vector<Tool> tools;
    };
}

#endif //NEZHAIDE_TOOL_REGISTRY_H