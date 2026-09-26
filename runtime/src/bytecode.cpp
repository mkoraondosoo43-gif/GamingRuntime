#include "gaming_runtime/bytecode.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace gaming_runtime {
namespace {

bool parse_instruction(const std::string& line, BytecodeInstruction& out) {
    std::istringstream input(line);
    std::string op;
    input >> op;

    if (op.empty() || op[0] == '#') {
        return false;
    }

    if (op == "NOP") {
        out = {BytecodeOp::Nop, 0};
        return true;
    }

    if (op == "HALT") {
        out = {BytecodeOp::Halt, 0};
        return true;
    }

    std::int64_t value = 0;
    if (!(input >> value)) {
        return false;
    }

    if (op == "PUSH") {
        out = {BytecodeOp::Push, value};
    } else if (op == "ADD") {
        out = {BytecodeOp::Add, 0};
    } else if (op == "SUB") {
        out = {BytecodeOp::Subtract, 0};
    } else if (op == "MUL") {
        out = {BytecodeOp::Multiply, 0};
    } else if (op == "SET") {
        out = {BytecodeOp::Set, value};
    } else {
        return false;
    }

    return true;
}

} // namespace

BytecodeGameModule::BytecodeGameModule(std::string program_path)
    : program_path_(std::move(program_path)) {}

bool BytecodeGameModule::load_program() {
    std::ifstream file(program_path_);
    if (!file) {
        return false;
    }

    program_.clear();

    std::string line;
    while (std::getline(file, line)) {
        BytecodeInstruction instruction;

        if (line.empty()) {
            continue;
        }

        if (!parse_instruction(line, instruction)) {
            if (line[0] == '#') {
                continue;
            }
            return false;
        }

        program_.push_back(instruction);
    }

    return !program_.empty();
}

bool BytecodeGameModule::initialize() {
    if (!load_program()) {
        return false;
    }

    std::fill(std::begin(registers_), std::end(registers_), 0);
    stack_.clear();
    instruction_pointer_ = 0;
    halted_ = false;
    initialized_ = true;
    return true;
}

void BytecodeGameModule::update(const GameFrameContext&) {
    if (!initialized_ || halted_) {
        return;
    }

    constexpr std::size_t max_instructions_per_frame = 64;
    std::size_t executed = 0;

    while (instruction_pointer_ < program_.size() &&
           executed < max_instructions_per_frame &&
           !halted_) {
        const auto instruction = program_[instruction_pointer_++];

        switch (instruction.op) {
        case BytecodeOp::Nop:
            break;

        case BytecodeOp::Push:
            if (stack_.size() >= 1024) {
                halted_ = true;
                break;
            }
            stack_.push_back(instruction.operand);
            break;

        case BytecodeOp::Add:
        case BytecodeOp::Subtract:
        case BytecodeOp::Multiply:
            if (stack_.size() < 2) {
                halted_ = true;
                break;
            }
            {
                const auto rhs = stack_.back();
                stack_.pop_back();
                const auto lhs = stack_.back();
                stack_.pop_back();

                if (instruction.op == BytecodeOp::Add) {
                    stack_.push_back(lhs + rhs);
                } else if (instruction.op == BytecodeOp::Subtract) {
                    stack_.push_back(lhs - rhs);
                } else {
                    stack_.push_back(lhs * rhs);
                }
            }
            break;

        case BytecodeOp::Set:
            if (instruction.operand < 0 ||
                instruction.operand >= 8 ||
                stack_.empty()) {
                halted_ = true;
                break;
            }

            registers_[instruction.operand] = stack_.back();
            stack_.pop_back();
            break;

        case BytecodeOp::Halt:
            halted_ = true;
            break;
        }

        ++executed;
    }
}

void BytecodeGameModule::shutdown() {
    initialized_ = false;
    halted_ = true;
    stack_.clear();
    program_.clear();
}

std::int64_t BytecodeGameModule::register_value(std::size_t index) const {
    if (index >= 8) {
        return 0;
    }

    return registers_[index];
}

bool BytecodeGameModule::halted() const noexcept {
    return halted_;
}

} // namespace gaming_runtime
