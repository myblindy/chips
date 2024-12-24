module;

#include "stdafx.h"

export module vm;

import std;

using namespace std;

using TMemory = uint8_t;
using TRegister = uint8_t;
using TAddress = uint8_t;

export enum class VMState
{
	Edit,
	Running,
	Paused
};

class VM;

template<int Bytes>
struct Imm {};

struct Addr {};

export struct VMInstruction
{
	using TOperand = variant<Imm<1>, Imm<2>, Imm<4>, Addr>;

	const char* name;
	const vector<TMemory> base_opcode;
	const vector<TOperand> operands;
	function<bool(const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values)> execute_internal;

	size_t OpcodeLength() const;

	bool OpcodeValid(const span<const uint8_t> stream) const
	{
		return stream.size() >= OpcodeLength() && ranges::equal(stream.subspan(0, base_opcode.size()), base_opcode);
	}

	bool Execute(VM& vm, size_t memory_index) const;

	optional<string> Decode(const VM& vm, size_t memory_index) const;
};

export class VM
{
public:
	using TNetwork = vector<shared_ptr<VM>>;
	using TMakeNetwork = vector<tuple<string, function<shared_ptr<VM>()>, vector<uint8_t>>>;

private:
	string name;
	uint8_t network_index{};
	uint8_t index_in_network{};
	vector<TRegister> registers;
	vector<TMemory> memory, saved_memory;
	string error_message;
	unordered_map<size_t, const VMInstruction> instructions;

	size_t IndexFromOpcode(const vector<TMemory>& opcode) const;

	TRegister ip{};
	atomic<VMState> state = VMState::Edit;

	SDL_TimerID timer{};

	bool ExecuteNextInstruction();

	vector<function<void(const VM&)>> dirty_callbacks;
	void TriggerDirtyCallbacks() const { for (auto&& callback : dirty_callbacks) callback(*this); }

	vector<function<void(const VM&)>> success_callbacks;
	void TriggerSuccessCallbacks() const { for (auto&& callback : success_callbacks) callback(*this); }

public:
	VM(int registers, size_t memory_size, initializer_list<const VMInstruction> instructions)
		: registers(registers), memory(memory_size), saved_memory(memory_size)
	{
		for (auto&& instruction : instructions)
			this->instructions.insert({ IndexFromOpcode(instruction.base_opcode), instruction });
	}

	auto Name() const { return name; }
	void Name(const string_view value) { name = value; TriggerDirtyCallbacks(); }

	auto NetworkIndex() const { return network_index; }
	void NetworkIndex(uint8_t value) { network_index = value; TriggerDirtyCallbacks(); }

	auto IndexInNetwork() const { return index_in_network; }
	void IndexInNetwork(uint8_t value) { index_in_network = value; TriggerDirtyCallbacks(); }

	string ErrorMessage() const { return error_message; }

	void Memory(size_t index, const TMemory value) { memory[index] = value; TriggerDirtyCallbacks(); }

	auto Memory() { return span{ memory }; }
	const auto Memory(size_t index) const { return memory[index]; }
	const auto Memory(const pair<size_t, size_t> range) const { return span{ memory }.subspan(range.first, range.second); }

	const auto MemorySize() const { return memory.size(); }

	const auto Register(int index) const { return registers[index]; }
	void Register(int index, const TRegister value) { registers[index] = value; TriggerDirtyCallbacks(); }

	auto RegisterName(int index) const { return format("R{}", index); }

	const auto RegisterCount() const { return registers.size(); }

	const VMState State() const { return state; }

	const auto IP() const { return ip; }
	void IP(const TRegister value) { ip = value; TriggerDirtyCallbacks(); }

	void Run();
	void Step();
	void Pause();
	void Stop();

	optional<string> DecodeInstruction(size_t memory_index) const;

	void OnDirty(function<void(const VM&)> callback) { dirty_callbacks.push_back(callback); }
	void OnSuccess(function<void(const VM&)> callback) { success_callbacks.push_back(callback); }
};

inline size_t VM::IndexFromOpcode(const vector<uint8_t>& opcode) const
{
	switch (opcode.size())
	{
	case 1: return (size_t)opcode[0];
	case 2: return *reinterpret_cast<const uint16_t*>(opcode.data());
	case 3: case 4: return *reinterpret_cast<const uint32_t*>(opcode.data());
	default: assert(false); return 0;
	}
}

void VM::Run()
{
	if (state == VMState::Edit)
	{
		saved_memory = memory;
		ip = 0;
	}

	state = VMState::Running;

	timer = SDL_AddTimerNS(250ULL * 100000, [](auto userdata, auto id, auto interval) -> uint64_t
		{
			auto&& self = reinterpret_cast<VM*>(userdata);
			self->ExecuteNextInstruction();
			return interval;
		}, this);
	assert(timer);
}

void VM::Step()
{
	if (state == VMState::Edit)
	{
		saved_memory = memory;
		ip = 0;
	}

	state = VMState::Paused;
	ExecuteNextInstruction();
}

void VM::Pause()
{
	state = VMState::Paused;

	if (timer)
	{
		SDL_RemoveTimer(timer);
		timer = 0;
	}
}

void VM::Stop()
{
	memory = saved_memory;
	state = VMState::Edit;
	error_message.clear();

	if (timer)
	{
		SDL_RemoveTimer(timer);
		timer = 0;
	}
}

inline bool VM::ExecuteNextInstruction()
{
#define ERROR_RETURN(msg) do{ error_message = (msg); return false; }while(0)
	const auto ip = static_cast<size_t>(this->ip);
	if (ip >= memory.size())
		ERROR_RETURN(format("IP ({:#04x}) is out of bounds ({:#04x}).", ip, memory.size()));
	auto it = instructions.find((size_t)Memory(ip));
	if (it == instructions.end())
		ERROR_RETURN("Invalid instruction opcode.");
	const auto& instruction = it->second;
	if (!instruction.Execute(*this, ip))
		ERROR_RETURN("Internal instruction error.");

	this->ip += static_cast<TRegister>(instruction.OpcodeLength());
	return true;
#undef ERROR_RETURN
}

inline optional<string> VM::DecodeInstruction(size_t memory_index) const
{
	auto it = instructions.find((size_t)Memory(memory_index));
	if (it == instructions.end())
		return nullopt;
	return it->second.Decode(*this, memory_index);
}

inline size_t VMInstruction::OpcodeLength() const
{
	size_t length = base_opcode.size();
	for (auto&& operand : operands)
	{
		visit(overload{
			[&](const Imm<1>&) { length += 1; },
			[&](const Imm<2>&) { length += 2; },
			[&](const Imm<4>&) { length += 4; },
			[&](const Addr&) { length += sizeof(TAddress); }
			}, operand);
	}
	return length;
}

inline bool VMInstruction::Execute(VM& vm, size_t memory_index) const
{
	auto instruction_stream = vm.Memory({ memory_index, OpcodeLength() });
	if (!OpcodeValid(instruction_stream))
		return false;

	instruction_stream = instruction_stream.subspan(base_opcode.size());
	vector<size_t> operand_values;
	operand_values.reserve(operands.size());
	for (auto&& operand : operands)
	{
		visit(overload{
			[&](const Imm<1>&) { operand_values.push_back(instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			[&](const Imm<2>&) { operand_values.push_back(*reinterpret_cast<const uint16_t*>(instruction_stream.subspan(0, 2).data())); instruction_stream = instruction_stream.subspan(2); },
			[&](const Imm<4>&) { operand_values.push_back(*reinterpret_cast<const uint32_t*>(instruction_stream.subspan(0, 4).data())); instruction_stream = instruction_stream.subspan(4); },
			[&](const Addr&) { operand_values.push_back(*reinterpret_cast<const TAddress*>(instruction_stream.subspan(0, sizeof(TAddress)).data())); instruction_stream = instruction_stream.subspan(sizeof(TAddress)); }
			}, operand);
	}

	if (!execute_internal || !execute_internal(*this, vm, memory_index, operand_values))
		return false;
	return true;
}

inline optional<string> VMInstruction::Decode(const VM& vm, size_t memory_index) const
{
	if (memory_index + OpcodeLength() > vm.MemorySize())
		return nullopt;

	auto instruction_stream = vm.Memory({ memory_index, OpcodeLength() });
	if (!OpcodeValid(instruction_stream))
		return nullopt;
	instruction_stream = instruction_stream.subspan(base_opcode.size());

	string result = name;
	result += " ";
	bool first = true;
	for (auto&& operand : operands)
	{
		if (first)
			first = false;
		else
			result += ", ";

		visit(overload{
			[&](const Imm<1>&) { result += to_string(instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			[&](const Imm<2>&) { result += to_string(*reinterpret_cast<const uint16_t*>(instruction_stream.subspan(0, 2).data())); instruction_stream = instruction_stream.subspan(2); },
			[&](const Imm<4>&) { result += to_string(*reinterpret_cast<const uint32_t*>(instruction_stream.subspan(0, 4).data())); instruction_stream = instruction_stream.subspan(4); },
			[&](const Addr&) {
				auto address = *reinterpret_cast<const TAddress*>(instruction_stream.subspan(0, sizeof(TAddress)).data());
				if (sizeof(TAddress) == 4)
					result += format("{:#010x}", static_cast<intptr_t>(address));
				else if (sizeof(TAddress) == 2)
					result += format("{:#06x}", static_cast<intptr_t>(address));
				else if (sizeof(TAddress) == 1)
					result += format("{:#04x}", static_cast<intptr_t>(address));
				else
					assert(false);
				instruction_stream = instruction_stream.subspan(sizeof(TAddress));
			}
			}, operand);
	}
	return result;
}

export VMInstruction MakeLoadRegisterSingleAddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR", vector<uint8_t>{opcode}, {Addr{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Register(0, vm.Memory(address));
			return true;
		}
	};
}

export VMInstruction MakeLoadRegisterSingleImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "LDRI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

export VMInstruction MakeStoreRegisterSingleAddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "STR", vector<uint8_t>{opcode}, {Addr{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Memory(address, vm.Register(0));
			return true;
		}
	};
}

export VMInstruction MakeAddRegisterImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "ADDI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, vm.Register(0) + static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

export VMInstruction MakeJmpImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "JMPI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			vm.IP(static_cast<TRegister>(operand_values[0]) - 2);
			return true;
		}
	};
}