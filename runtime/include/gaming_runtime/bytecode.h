#pragma once

#include "gaming_runtime/game_module.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gaming_runtime {

enum class BytecodeOp : std::uint8_t {
    Nop,
    Push,
    Add,
    Subtract,
    Multiply,
    Set,
    Get,
    CompareEqual,
    Jump,
    JumpIfZero,
    Halt
};

struct BytecodeInstruction {
    BytecodeOp op = BytecodeOp::Nop;
    std::int64_t operand = 0;
};

class BytecodeGameModule final : public GameModule {
public:
    explicit BytecodeGameModule(std::string program_path);

    bool initialize() override;
    void update(const GameFrameContext& context) override;
    void shutdown() override;

    std::int64_t register_value(std::size_t index) const;
    bool halted() const noexcept;

private:
    bool load_program();

    std::string program_path_;
    std::vector<BytecodeInstruction> program_;
    std::int64_t registers_[8]{};
    std::vector<std::int64_t> stack_;
    std::size_t instruction_pointer_ = 0;
    bool initialized_ = false;
    bool halted_ = false;
};

} // namespace gaming_runtime
