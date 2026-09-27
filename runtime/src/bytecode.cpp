#include "gaming_runtime/bytecode.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

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

    if (op == "EQ") {
        out = {BytecodeOp::CompareEqual, 0};
        return true;
    }

    if (op == "ADD") { out = {BytecodeOp::Add, 0}; return true; }
    if (op == "SUB") { out = {BytecodeOp::Subtract, 0}; return true; }
    if (op == "MUL") { out = {BytecodeOp::Multiply, 0}; return true; }

    std::int64_t value = 0;
    if (!(input >> value)) {
        return false;
    }

    if (op == "PUSH") {
        out = {BytecodeOp::Push, value};
    } else if (op == "SET") {
        out = {BytecodeOp::Set, value};
    } else if (op == "GET") {
        out = {BytecodeOp::Get, value};
    } else if (op == "JMP") {
        out = {BytecodeOp::Jump, value};
    } else if (op == "JZ") {
        out = {BytecodeOp::JumpIfZero, value};
    } else if (op == "LOAD") {
        out = {BytecodeOp::LoadMemory, value};
    } else if (op == "STORE") {
        out = {BytecodeOp::StoreMemory, value};
    } else if (op == "CREATE_ENTITY") {
        out = {BytecodeOp::CreateEntity, value};
    } else if (op == "DESTROY_ENTITY") {
        out = {BytecodeOp::DestroyEntity, value};
    } else if (op == "SET_POSITION") {
        out = {BytecodeOp::SetPosition, value};
    } else if (op == "SET_SCALE") {
        out = {BytecodeOp::SetScale, value};
    } else if (op == "SET_RESOURCE") {
        out = {BytecodeOp::SetResource, value};
    } else if (op == "SET_VISIBLE") {
        out = {BytecodeOp::SetVisible, value};
    } else if (op == "ATTACH_CAMERA") {
        out = {BytecodeOp::AttachCamera, value};
    } else if (op == "DETACH_CAMERA") {
        out = {BytecodeOp::DetachCamera, value};
    } else if (op == "SET_CAMERA_ZOOM") {
        out = {BytecodeOp::SetCameraZoom, value};
    } else if (op == "SET_CAMERA_VIEWPORT") {
        out = {BytecodeOp::SetCameraViewport, value};
    } else if (op == "SET_CAMERA_ACTIVE") {
        out = {BytecodeOp::SetCameraActive, value};
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
    std::fill(std::begin(memory_), std::end(memory_), 0);
    std::fill(std::begin(entity_registers_), std::end(entity_registers_), kInvalidEntity);
    stack_.clear();
    instruction_pointer_ = 0;
    halted_ = false;
    initialized_ = true;
    return true;
}

void BytecodeGameModule::update(const GameFrameContext& context) {
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

        case BytecodeOp::Get:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                stack_.size() >= 1024) {
                halted_ = true;
                break;
            }
            stack_.push_back(registers_[instruction.operand]);
            break;

        case BytecodeOp::CompareEqual:
            if (stack_.size() < 2) {
                halted_ = true;
                break;
            }
            {
                const auto rhs = stack_.back();
                stack_.pop_back();
                const auto lhs = stack_.back();
                stack_.pop_back();
                stack_.push_back(lhs == rhs ? 1 : 0);
            }
            break;

        case BytecodeOp::Jump:
            if (instruction.operand < 0 ||
                static_cast<std::size_t>(instruction.operand) >= program_.size()) {
                halted_ = true;
                break;
            }
            instruction_pointer_ = static_cast<std::size_t>(instruction.operand);
            break;

        case BytecodeOp::JumpIfZero:
            if (stack_.empty() || instruction.operand < 0 ||
                static_cast<std::size_t>(instruction.operand) >= program_.size()) {
                halted_ = true;
                break;
            }
            {
                const auto condition = stack_.back();
                stack_.pop_back();
                if (condition == 0) {
                    instruction_pointer_ = static_cast<std::size_t>(instruction.operand);
                }
            }
            break;

        case BytecodeOp::LoadMemory:
            if (instruction.operand < 0 || instruction.operand >= 256 ||
                stack_.size() >= 1024) {
                halted_ = true;
                break;
            }
            stack_.push_back(memory_[instruction.operand]);
            break;

        case BytecodeOp::StoreMemory:
            if (instruction.operand < 0 || instruction.operand >= 256 ||
                stack_.empty()) {
                halted_ = true;
                break;
            }
            memory_[instruction.operand] = stack_.back();
            stack_.pop_back();
            break;

        case BytecodeOp::CreateEntity:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr) {
                halted_ = true;
                break;
            }
            {
                const EntityId entity =
                    context.world->create_entity();
                if (entity == kInvalidEntity) {
                    halted_ = true;
                    break;
                }
                entity_registers_[instruction.operand] = entity;
            }
            break;

        case BytecodeOp::DestroyEntity:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity) {
                halted_ = true;
                break;
            }
            if (!context.world->destroy_entity(
                    entity_registers_[instruction.operand])) {
                halted_ = true;
                break;
            }
            entity_registers_[instruction.operand] = kInvalidEntity;
            break;

        case BytecodeOp::SetPosition:
        case BytecodeOp::SetScale:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity ||
                stack_.size() < 3) {
                halted_ = true;
                break;
            }
            {
                auto* transform =
                    context.world->transform(entity_registers_[instruction.operand]);
                if (transform == nullptr) {
                    halted_ = true;
                    break;
                }
                const float x = static_cast<float>(stack_[stack_.size() - 3]) / 1000.0f;
                const float y = static_cast<float>(stack_[stack_.size() - 2]) / 1000.0f;
                const float z = static_cast<float>(stack_[stack_.size() - 1]) / 1000.0f;
                stack_.resize(stack_.size() - 3);

                if (instruction.op == BytecodeOp::SetPosition) {
                    transform->position = {x, y, z};
                } else {
                    transform->scale = {x, y, z};
                }
            }
            break;

        case BytecodeOp::SetResource:
        case BytecodeOp::SetVisible:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity ||
                stack_.empty()) {
                halted_ = true;
                break;
            }
            {
                auto* renderable =
                    context.world->renderable(entity_registers_[instruction.operand]);
                if (renderable == nullptr) {
                    halted_ = true;
                    break;
                }

                const auto value = stack_.back();
                stack_.pop_back();

                if (instruction.op == BytecodeOp::SetResource) {
                    if (value < 0 ||
                        static_cast<std::uint64_t>(value) > UINT32_MAX) {
                        halted_ = true;
                        break;
                    }
                    renderable->resource_id =
                        static_cast<std::uint32_t>(value);
                } else {
                    renderable->visible = value != 0;
                }
            }
            break;

        case BytecodeOp::AttachCamera:
        case BytecodeOp::DetachCamera:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity) {
                halted_ = true;
                break;
            }
            if (instruction.op == BytecodeOp::AttachCamera) {
                if (!context.world->attach_camera(
                        entity_registers_[instruction.operand])) {
                    halted_ = true;
                }
            } else {
                if (!context.world->detach_camera(
                        entity_registers_[instruction.operand])) {
                    halted_ = true;
                }
            }
            break;

        case BytecodeOp::SetCameraZoom:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity ||
                stack_.empty()) {
                halted_ = true;
                break;
            }
            {
                auto* camera = context.world->camera(
                    entity_registers_[instruction.operand]);
                if (camera == nullptr) {
                    halted_ = true;
                    break;
                }
                const auto value = stack_.back();
                stack_.pop_back();
                if (value <= 0) {
                    halted_ = true;
                    break;
                }
                camera->zoom = static_cast<float>(value) / 1000.0f;
            }
            break;

        case BytecodeOp::SetCameraViewport:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity ||
                stack_.size() < 2) {
                halted_ = true;
                break;
            }
            {
                auto* camera = context.world->camera(
                    entity_registers_[instruction.operand]);
                if (camera == nullptr) {
                    halted_ = true;
                    break;
                }
                const auto width = stack_[stack_.size() - 2];
                const auto height = stack_[stack_.size() - 1];
                stack_.resize(stack_.size() - 2);
                if (width <= 0 || height <= 0 ||
                    static_cast<std::uint64_t>(width) > UINT32_MAX ||
                    static_cast<std::uint64_t>(height) > UINT32_MAX) {
                    halted_ = true;
                    break;
                }
                camera->viewport_width = static_cast<std::uint32_t>(width);
                camera->viewport_height = static_cast<std::uint32_t>(height);
            }
            break;

        case BytecodeOp::SetCameraActive:
            if (instruction.operand < 0 || instruction.operand >= 8 ||
                context.world == nullptr ||
                entity_registers_[instruction.operand] == kInvalidEntity ||
                stack_.empty()) {
                halted_ = true;
                break;
            }
            {
                auto* camera = context.world->camera(
                    entity_registers_[instruction.operand]);
                if (camera == nullptr) {
                    halted_ = true;
                    break;
                }
                camera->active = stack_.back() != 0;
                stack_.pop_back();
            }
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
    std::fill(std::begin(entity_registers_), std::end(entity_registers_), kInvalidEntity);
    program_.clear();
}

std::int64_t BytecodeGameModule::register_value(std::size_t index) const {
    if (index >= 8) {
        return 0;
    }

    return registers_[index];
}

std::int64_t BytecodeGameModule::memory_value(std::size_t index) const {
    if (index >= 256) {
        return 0;
    }

    return memory_[index];
}

bool BytecodeGameModule::halted() const noexcept {
    return halted_;
}

} // namespace gaming_runtime
