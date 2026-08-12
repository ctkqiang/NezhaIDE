#include "binary_parser.h"
#include "src/utilities/logger.h"

#include <LIEF/LIEF.hpp>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <cstring>
#include <set>

namespace NezhaIDE::Model {

static bool is_printable_ascii(uint8_t b) { return b >= 0x20 && b <= 0x7E; }

static void extract_strings(const std::vector<uint8_t> &data, std::vector<std::string> &out) {
    const size_t min_len = 4;
    std::set<std::string> seen;
    size_t start = 0;
    bool in_string = false;

    for (size_t i = 0; i < data.size(); ++i) {
        bool printable = is_printable_ascii(data[i]);
        if (printable && !in_string) {
            start = i;
            in_string = true;
        } else if (!printable && in_string) {
            if (i - start >= min_len) {
                std::string s(reinterpret_cast<const char *>(&data[start]), i - start);
                if (seen.insert(s).second && out.size() < 10000) {
                    out.push_back(std::move(s));
                }
            }
            in_string = false;
        }
    }
}

static BinaryArch map_arch(const std::string &arch_str) {
    if (arch_str.find("ARM64") != std::string::npos || arch_str.find("AARCH64") != std::string::npos)
        return BinaryArch::ARM64;
    if (arch_str.find("ARM") != std::string::npos || arch_str.find("AARCH32") != std::string::npos)
        return BinaryArch::ARM;
    if (arch_str.find("x86_64") != std::string::npos || arch_str.find("AMD64") != std::string::npos)
        return BinaryArch::X86_64;
    if (arch_str.find("x86") != std::string::npos || arch_str.find("i386") != std::string::npos)
        return BinaryArch::X86;
    return BinaryArch::Unknown;
}

static std::vector<uint8_t> read_raw_file(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    auto bytes = file.readAll();
    return {reinterpret_cast<const uint8_t *>(bytes.constData()),
            reinterpret_cast<const uint8_t *>(bytes.constData()) + bytes.size()};
}

struct BinaryParser::Impl {
    std::vector<uint8_t> raw_data;
    BinaryFormat format{};
    BinaryArch arch{};
    uint64_t entry_point{};
    uint64_t image_base{};
    std::vector<BinarySection> sections;
    std::vector<BinarySymbol> symbols;
    std::vector<ImportEntry> imports;
    std::vector<std::string> strings;
    std::vector<DisasmTarget> disasm_targets;
    QString file_path;
};

BinaryParser::BinaryParser() : impl_(std::make_unique<Impl>()) {}
BinaryParser::~BinaryParser() = default;
BinaryParser::BinaryParser(BinaryParser &&) noexcept = default;
BinaryParser &BinaryParser::operator=(BinaryParser &&) noexcept = default;

std::expected<BinaryParser, std::string> BinaryParser::open(const QString &path) {
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        return std::unexpected("file not found: " + path.toStdString());
    }

    auto p = BinaryParser();
    auto &impl = *p.impl_;
    impl.file_path = path;

    auto raw = read_raw_file(path);
    if (raw.empty()) {
        return std::unexpected("empty file: " + path.toStdString());
    }
    impl.raw_data = std::move(raw);
    extract_strings(impl.raw_data, impl.strings);

    auto ext = fi.suffix().toLower();
    if (ext == QStringLiteral("apk") || ext == QStringLiteral("jar") || ext == QStringLiteral("zip")) {
        impl.format = (ext == QStringLiteral("apk")) ? BinaryFormat::APK : BinaryFormat::Unknown;
        return p;
    }

    auto &logger = Utilities::Logger::instance();

    try {
        auto binary = LIEF::Parser::parse(path.toStdString());
        if (!binary) {
            return std::unexpected("not a supported binary format: " + path.toStdString());
        }

        impl.entry_point = binary->entrypoint();
        impl.image_base = binary->imagebase();

        auto fmt = binary->format();
        switch (fmt) {
        case LIEF::Binary::FORMATS::ELF:   impl.format = BinaryFormat::ELF; break;
        case LIEF::Binary::FORMATS::PE:    impl.format = BinaryFormat::PE; break;
        case LIEF::Binary::FORMATS::MACHO: impl.format = BinaryFormat::MachO; break;
        default: break;
        }

        auto arch_str = LIEF::to_string(binary->header().architecture());
        impl.arch = map_arch(arch_str);

        for (auto &sec : binary->sections()) {
            BinarySection bs;
            bs.name = sec.name();
            bs.virtual_address = sec.virtual_address();
            bs.file_offset = sec.offset();
            bs.size = sec.size();
            impl.sections.push_back(std::move(bs));

            if (!sec.name().empty() && sec.virtual_address() > 0 && sec.size() > 0) {
                auto content = sec.content();
                if (!content.empty()) {
                    DisasmTarget dt;
                    dt.address = sec.virtual_address();
                    dt.offset = sec.offset();
                    dt.size = sec.size();
                    impl.disasm_targets.push_back(dt);
                }
            }
        }

        for (auto &sym : binary->symbols()) {
            if (sym.name().empty()) continue;
            BinarySymbol bs;
            bs.name = sym.name();
            bs.address = sym.value();
            bs.size = sym.size();
            impl.symbols.push_back(std::move(bs));
        }

        for (auto &imp : binary->imported_functions()) {
            ImportEntry ie;
            ie.name = imp.name();
            impl.imports.push_back(std::move(ie));
        }
    } catch (const std::exception &e) {
        logger.log(Utilities::LogLevel::Debug, __FILE__, __LINE__, __func__,
            "LIEF 解析异常: {}", e.what());
    }

    logger.log(Utilities::LogLevel::Info, __FILE__, __LINE__, __func__,
        "二进制: {} fmt={} arch={} sections={} syms={} imports={} strings={} disasm={}",
        path.toStdString(), p.format_name(), p.arch_name(),
        impl.sections.size(), impl.symbols.size(), impl.imports.size(),
        impl.strings.size(), impl.disasm_targets.size());

    return p;
}

BinaryFormat BinaryParser::format() const noexcept { return impl_->format; }
BinaryArch BinaryParser::arch() const noexcept { return impl_->arch; }
uint64_t BinaryParser::entry_point() const noexcept { return impl_->entry_point; }
uint64_t BinaryParser::image_base() const noexcept { return impl_->image_base; }

const std::vector<uint8_t> &BinaryParser::raw_data() const noexcept { return impl_->raw_data; }
const std::vector<BinarySection> &BinaryParser::sections() const noexcept { return impl_->sections; }
const std::vector<BinarySymbol> &BinaryParser::symbols() const noexcept { return impl_->symbols; }
const std::vector<ImportEntry> &BinaryParser::imports() const noexcept { return impl_->imports; }
const std::vector<std::string> &BinaryParser::strings() const noexcept { return impl_->strings; }
const std::vector<DisasmTarget> &BinaryParser::disasm_targets() const noexcept { return impl_->disasm_targets; }

QString BinaryParser::file_path() const { return impl_->file_path; }

std::string BinaryParser::format_name() const {
    switch (impl_->format) {
    case BinaryFormat::ELF:   return "ELF";
    case BinaryFormat::PE:    return "PE";
    case BinaryFormat::MachO: return "Mach-O";
    case BinaryFormat::APK:   return "APK";
    default:                  return "Unknown";
    }
}

std::string BinaryParser::arch_name() const {
    switch (impl_->arch) {
    case BinaryArch::X86_64: return "x86-64";
    case BinaryArch::X86:    return "x86";
    case BinaryArch::ARM64:  return "ARM64";
    case BinaryArch::ARM:    return "ARM";
    case BinaryArch::Thumb:  return "Thumb";
    default:                 return "Unknown";
    }
}

} // namespace NezhaIDE::Model
