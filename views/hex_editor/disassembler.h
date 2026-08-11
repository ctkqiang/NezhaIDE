#pragma once

#include "binary_parser.h"
#include <expected>
#include <string>
#include <vector>

namespace NezhaIDE::Services {
    struct DisasmInstruction {
        uint64_t address{};
        uint64_t offset{};
        std::string bytes_hex;
        std::string mnemonic;
        std::string operands;
        int size{};
    };

    class Disassembler {
    public:
        static Disassembler &instance();

        Disassembler(const Disassembler &) = delete;

        Disassembler &operator=(const Disassembler &) = delete;

        /**
         * 对二进制数据执行反汇编。
         *
         * @param data 指向原始字节的指针
         * @param size 数据大小（字节）
         * @param offset_in_file 数据在文件中的偏移量
         * @param base_address 起始虚拟地址
         * @param arch 目标架构
         * @return 反汇编指令列表，或错误描述
         */
        static std::expected<std::vector<DisasmInstruction>, std::string>
        disassemble(
            const uint8_t *data,
            size_t size,
            uint64_t offset_in_file,
            uint64_t base_address,
            Model::BinaryArch arch
        );

    private:
        Disassembler();

        ~Disassembler();
    };
} // namespace NezhaIDE::Services
