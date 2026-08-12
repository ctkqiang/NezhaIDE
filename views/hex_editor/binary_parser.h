#pragma once

#include <QFileInfo>
#include <QString>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>

/**
 * 二进制分析数据模型命名空间。
 */
namespace NezhaIDE::Model {

/**
 * 可执行文件/容器格式。
 */
enum class BinaryFormat { Unknown, ELF, PE, MachO, MachO_Universal, APK };

/**
 * 目标 CPU 架构。
 */
enum class BinaryArch { Unknown, X86, X86_64, ARM, ARM64, Thumb };

/**
 * 二进制文件中的段/节信息。
 */
struct BinarySection {
    std::string name;
    uint64_t virtual_address{};
    uint64_t file_offset{};
    uint64_t size{};
    bool is_executable{false};
    bool is_writable{false};
    bool is_readable{false};
};

/**
 * 符号表条目（函数/对象符号）。
 */
struct BinarySymbol {
    std::string name;
    uint64_t address{};
    uint64_t size{};
    bool exported{false};
};

/**
 * 导入表条目。
 */
struct ImportEntry {
    std::string name;
    std::string library;
};

/**
 * 反汇编目标区域（代码段的地址范围）。
 */
struct DisasmTarget {
    uint64_t address;
    uint64_t offset;
    uint64_t size;
};

/**
 * 二进制文件解析器，基于 LIEF 库解析 ELF/PE/Mach-O/APK 格式。
 *
 * 提取段信息、符号表、导入表、可打印字符串和反汇编目标区域。
 * 使用 PIMPL 模式隐藏 LIEF 实现细节。
 * 反汇编通过独立的 Disassembler 服务执行。
 *
 * @see NezhaIDE::Services::Disassembler
 */
class BinaryParser {
public:
    /**
     * 打开并解析二进制文件。
     *
     * @param path 文件路径
     * @return 成功时返回 BinaryParser，失败时返回错误描述
     */
    static std::expected<BinaryParser, std::string> open(const QString &path);

    BinaryParser(BinaryParser &&) noexcept;
    BinaryParser &operator=(BinaryParser &&) noexcept;
    ~BinaryParser();

    BinaryFormat format() const noexcept;
    BinaryArch arch() const noexcept;

    uint64_t entry_point() const noexcept;
    uint64_t image_base() const noexcept;

    const std::vector<uint8_t> &raw_data() const noexcept;
    const std::vector<BinarySection> &sections() const noexcept;
    const std::vector<BinarySymbol> &symbols() const noexcept;
    const std::vector<ImportEntry> &imports() const noexcept;

    const std::vector<std::string> &strings() const noexcept;
    const std::vector<DisasmTarget> &disasm_targets() const noexcept;

    QString file_path() const;
    std::string format_name() const;
    std::string arch_name() const;

private:
    BinaryParser();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace NezhaIDE::Model
