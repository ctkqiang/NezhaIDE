#pragma once

#include "binary_parser.h"
#include <expected>
#include <string>
#include <vector>

/**
 * 反汇编服务命名空间。
 */
namespace NezhaIDE::Services {

    /**
     * 单条反汇编指令的展示数据。
     */
    struct DisasmInstruction {
        uint64_t address{};
        uint64_t offset{};
        std::string bytes_hex;
        std::string mnemonic;
        std::string operands;
        int size{};
    };

    /**
     * Capstone 反汇编引擎单例。
     *
     * 支持 ARM64、ARM、Thumb、x86-64、x86 五种架构。
     * 启用 SKIPDATA 模式以容忍无效指令区域。
     */
    class Disassembler {
    public:
        /**
         * 获取全局反汇编器实例。
         *
         * @return 单例引用。
         */
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
