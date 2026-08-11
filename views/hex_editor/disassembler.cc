#include "disassembler.h"
#include "src/utilities/logger.h"
#include <capstone/capstone.h>
#include <iomanip>
#include <sstream>

namespace NezhaIDE::Services {

static cs_arch map_cs_arch(Model::BinaryArch arch, cs_mode &mode) {
    mode = CS_MODE_LITTLE_ENDIAN;
    switch (arch) {
    case Model::BinaryArch::X86_64:
        mode = CS_MODE_64;
        return CS_ARCH_X86;
    case Model::BinaryArch::X86:
        mode = CS_MODE_32;
        return CS_ARCH_X86;
    case Model::BinaryArch::ARM64:
        mode = CS_MODE_ARM;
        return CS_ARCH_ARM64;
    case Model::BinaryArch::ARM:
        return CS_ARCH_ARM;
    case Model::BinaryArch::Thumb:
        mode = CS_MODE_THUMB;
        return CS_ARCH_ARM;
    default:
        return CS_ARCH_MAX;
    }
}

static std::string bytes_to_hex(const uint8_t *bytes, size_t count) {
    std::ostringstream oss;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) oss << ' ';
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

Disassembler &Disassembler::instance() {
    static Disassembler d;
    return d;
}

Disassembler::Disassembler() = default;
Disassembler::~Disassembler() = default;

std::expected<std::vector<DisasmInstruction>, std::string>
Disassembler::disassemble(const uint8_t *data, size_t size, uint64_t offset_in_file,
                          uint64_t base_address, Model::BinaryArch arch) {
    if (!data || size == 0) {
        return std::unexpected("empty data");
    }

    cs_mode mode;
    auto cs_arch = map_cs_arch(arch, mode);
    if (cs_arch == CS_ARCH_MAX) {
        return std::unexpected("unsupported architecture");
    }

    csh handle;
    auto err = cs_open(cs_arch, mode, &handle);
    if (err != CS_ERR_OK) {
        return std::unexpected(std::string("capstone open error: ") + cs_strerror(err));
    }

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(handle, CS_OPT_SKIPDATA, CS_OPT_ON);

    auto count = cs_disasm(handle, data, size, base_address, 0, nullptr);
    if (count == 0) {
        cs_close(&handle);
        return std::unexpected("no instructions disassembled");
    }

    auto *insns = cs_disasm(handle, data, size, base_address, count, nullptr);
    if (!insns) {
        cs_close(&handle);
        return std::unexpected(std::string("capstone disasm error: ") + cs_strerror(cs_errno(handle)));
    }

    std::vector<DisasmInstruction> result;
    result.reserve(count);

    auto &dlog = Utilities::Logger::instance();

    for (size_t i = 0; i < count; ++i) {
        const auto &insn = insns[i];
        DisasmInstruction di;
        di.address = insn.address;
        di.offset = offset_in_file + (insn.address - base_address);
        di.mnemonic = insn.mnemonic;
        di.operands = insn.op_str ? insn.op_str : "";
        di.size = insn.size;
        di.bytes_hex = bytes_to_hex(insn.bytes, insn.size);
        result.push_back(std::move(di));
    }

    cs_free(insns, count);
    cs_close(&handle);

    dlog.log(Utilities::LogLevel::Debug, __FILE__, __LINE__, __func__,
        "反汇编完成: {} 指令, arch={}", result.size(),
        static_cast<int>(arch));

    return result;
}

} // namespace NezhaIDE::Services
