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
	TNetworkIndex network_index{};
	TIndexInNetwork index_in_network{};
	vector<TRegister> registers;
	vector<TMemory> memory, saved_memory;
	string error_message;
	unordered_map<TNetworkIndex, unordered_map<TIndexInNetwork, shared_ptr<VM>>> network_vms;
	unordered_map<size_t, const VMInstruction> instructions;
	unordered_map<TIndexInNetwork, optional<TRegister>> incoming_data;

	size_t IndexFromOpcode(const vector<TMemory>& opcode) const;

	TRegister ip{};
	atomic<VMState> state = VMState::Edit;

	struct {
		bool zero : 1;
	} flags;

	SDL_TimerID timer{};

	bool ExecuteNextInstruction();

public:
	VM(int registers, size_t memory_size, const vector<VMInstruction>& instructions = {})
		: registers(registers), memory(memory_size), saved_memory(memory_size)
	{
		for (auto&& instruction : instructions)
			this->instructions.insert({ IndexFromOpcode(instruction.base_opcode), instruction });
	}

	auto Name() const { return name; }
	void Name(const string_view value) { name = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	auto NetworkIndex() const { return network_index; }
	void NetworkIndex(TNetworkIndex value) { network_index = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	auto IndexInNetwork() const { return index_in_network; }
	void IndexInNetwork(TIndexInNetwork value) { index_in_network = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	void AddNetworkedVM(TNetworkIndex network_index, TIndexInNetwork index_in_network, shared_ptr<VM> vm)
	{
		network_vms[network_index][index_in_network] = vm;
	}
	optional<shared_ptr<VM>> NetworkVM(TNetworkIndex network_index, TIndexInNetwork index_in_network) const
	{
		auto it = network_vms.find(network_index);
		if (it == network_vms.end())
			return nullopt;
		auto it2 = it->second.find(index_in_network);
		if (it2 == it->second.end())
			return nullopt;
		return it2->second;
	}
	optional<shared_ptr<VM>> NetworkVM(TIndexInNetwork index_in_network) const
	{
		return NetworkVM(network_index, index_in_network);
	}

	bool IncomingData(TIndexInNetwork index_in_network, optional<TRegister> value, bool force = false)
	{
		if (!force && incoming_data[index_in_network].has_value())
			return false;
		incoming_data[index_in_network] = value;
		return true;
	}
	optional<TRegister> IncomingData(TIndexInNetwork index_in_network) const
	{
		auto it = incoming_data.find(index_in_network);
		if (it == incoming_data.end())
			return nullopt;
		return it->second;
	}

	string ErrorMessage() const { return error_message; }

	void Memory(size_t index, const TMemory value) { memory[index] = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	auto Memory() { return span{ memory }; }
	const auto Memory(size_t index) const { return memory[index]; }
	const auto Memory(const pair<size_t, size_t> range) const { return span{ memory }.subspan(range.first, range.second); }

	const auto MemorySize() const { return memory.size(); }

	const auto Register(int index) const { return registers[index]; }
	void Register(int index, const TRegister value) { registers[index] = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	auto RegisterName(int index) const { return format("R{}", index); }

	const auto RegisterCount() const { return registers.size(); }

	const auto FlagZero() const { return flags.zero; }
	void FlagZero(bool value) { flags.zero = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	const VMState State() const { return state; }

	const auto IP() const { return ip; }
	void IP(const TRegister value) { ip = value; VMEventQueue.enqueue(VMEventType::Dirty, this); }

	void Run();
	void Step();
	void Pause();
	void Stop();

	optional<string> DecodeInstruction(size_t memory_index) const;
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
	VMEventQueue.enqueue(VMEventType::InstructionExecuted, this);

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

export VMInstruction MakeLoadRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR0", vector<uint8_t>{opcode}, {Addr{}},
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

export VMInstruction MakeLoadRegister1AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR1", vector<uint8_t>{opcode}, {Addr{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Register(1, vm.Memory(address));
			return true;
		}
	};
}

export VMInstruction MakeLoadRegister0Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "LDR0I8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

export VMInstruction MakeLoadRegister1Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "LDR1I8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			vm.Register(1, static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

export VMInstruction MakeStoreRegister0AddressInstruction(initializer_list<uint8_t> opcode)
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

export VMInstruction MakeStoreRegister1AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "STR", vector<uint8_t>{opcode}, {Addr{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Memory(address, vm.Register(1));
			return true;
		}
	};
}

export VMInstruction MakeAddRegister0Imm8Instruction(initializer_list<uint8_t> opcode)
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

export VMInstruction MakeJmpNotZeroImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "JMPNZI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (!vm.FlagZero())
				vm.IP(static_cast<TRegister>(operand_values[0]) - 2);
			return true;
		}
	};
}

export VMInstruction MakeOutInstruction(initializer_list<uint8_t> opcode)
{
	return { "OUT", vector<uint8_t>{opcode}, {},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;

			const auto dst_index = vm.Register(0);
			const auto value = vm.Register(1);

			auto dst_vm = vm.NetworkVM(dst_index);
			if (!dst_vm || !(*dst_vm)->IncomingData(vm.IndexInNetwork(), value))
				vm.FlagZero(true);
			else
				vm.FlagZero(false);
			return true;
		}
	};
}

export VMInstruction MakeInInstruction(initializer_list<uint8_t> opcode)
{
	return { "IN", vector<uint8_t>{opcode}, {},
		[&](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;

			const auto src_index = vm.Register(0);
			auto value = vm.IncomingData(vm.IndexInNetwork());
			if (!value)
				vm.FlagZero(true);
			else
			{
				vm.FlagZero(false);
				vm.Register(0, *value);
				vm.IncomingData(vm.IndexInNetwork(), nullopt, true);
			}

			return true;
		}
	};
}