#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace NezhaIDE::Model {

enum class BinaryFormat { Unknown, ELF, PE, MachO, MachO_Universal, APK };
enum class BinaryArch { Unknown, X86, X86_64, ARM, ARM64, Thumb };

struct BinarySection {
    std::string name;
    uint64_t virtual_address{};
    uint64_t file_offset{};
    uint64_t size{};
    bool is_executable{false};
    bool is_writable{false};
    bool is_readable{false};
};

struct BinarySymbol {
    std::string name;
    uint64_t address{};
    uint64_t size{};
    bool exported{false};
};

struct ImportEntry {
    std::string name;
    std::string library;
};

struct DisasmTarget {
    uint64_t address;
    uint64_t offset;
    uint64_t size;
};

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
