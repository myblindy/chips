module;

#include "stdafx.h"

export module vm;

import std;

using namespace std;

export using TMemory = uint8_t;
export using TRegister = uint8_t;
export using TAddress = uint8_t;

class VM;

export template<int Bytes> 
struct Imm {};
export struct Addr {};
export struct Reg {};

export struct VMInstruction
{
	using TOperand = variant<Imm<1>, Imm<2>, Imm<4>, Addr, Reg>;

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

	optional<string> Decode(const BaseMemory* memory, size_t memory_index) const;
};

export class BaseMemory
{
public:
	BaseMemory(size_t memory_size)
		: memory(memory_size), saved_memory(memory_size)
	{
	}

private:
	string name;
	bool editable{};
	TNetworkIndex network_index{};
	TIndexInNetwork index_in_network{};

	virtual bool ExecuteNextInstruction() = 0;

protected:
	vector<TMemory> memory, saved_memory;
	string error_message;
	vector<TRegister> registers;
	unordered_map<size_t, const VMInstruction> instructions;
	unordered_map<TNetworkIndex, unordered_map<TIndexInNetwork, shared_ptr<BaseMemory>>> network_vms;
	unordered_map<TIndexInNetwork, optional<tuple<TMemory, optional<TRegister>>>> incoming_data;

public:
	auto Name() const { return name; }
	void Name(const string_view value) { name = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	auto Editable() const { return editable; }
	void Editable(bool value) { editable = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	auto NetworkIndex() const { return network_index; }
	void NetworkIndex(TNetworkIndex value) { network_index = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	auto IndexInNetwork() const { return index_in_network; }
	void IndexInNetwork(TIndexInNetwork value) { index_in_network = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	void AddNetworkedVM(TNetworkIndex network_index, TIndexInNetwork index_in_network, shared_ptr<BaseMemory> vm)
	{
		network_vms[network_index][index_in_network] = vm;
	}
	optional<shared_ptr<BaseMemory>> NetworkVM(TNetworkIndex network_index, TIndexInNetwork index_in_network) const;
	optional<shared_ptr<BaseMemory>> NetworkVM(TIndexInNetwork index_in_network) const
	{
		return NetworkVM(network_index, index_in_network);
	}

	bool IncomingData(TIndexInNetwork index_in_network, optional<tuple<TMemory, optional<TRegister>>> data, bool force = false)
	{
		if (!force && incoming_data[index_in_network].has_value())
			return false;
		incoming_data[index_in_network] = data;
		return true;
	}
	optional<tuple<TMemory, optional<TRegister>>> IncomingData(TIndexInNetwork index_in_network) const;

	string ErrorMessage() const { return error_message; }

	void Memory(size_t index, const TMemory value) { memory[index] = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	auto Memory() { return span{ memory }; }
	const auto Memory(size_t index) const { return memory[index]; }
	const auto Memory(const pair<size_t, size_t> range) const { return span{ memory }.subspan(range.first, range.second); }

	const auto MemorySize() const { return memory.size(); }

	size_t IndexFromOpcode(const vector<TMemory>& opcode) const;
	optional<string> DecodeInstruction(size_t memory_index) const;

	auto RegisterName(int index) const { return format("R{}", index); }

	virtual void SetupForRun() = 0;
	virtual void Step() = 0;
	virtual void Stop() = 0;
};

export class VM : public BaseMemory
{
	TRegister ip{};

	struct {
		bool zero : 1;
	} flags;

	bool ExecuteNextInstruction() override;

public:
	VM(int registers, size_t memory_size, const vector<VMInstruction>& instructions = {})
		: BaseMemory(memory_size)
	{
		this->registers = vector<TRegister>(registers);
		for (auto&& instruction : instructions)
			this->instructions.insert({ IndexFromOpcode(instruction.base_opcode), instruction });
	}

	const auto Register(int index) const { return registers[index]; }
	void Register(int index, const TRegister value) { registers[index] = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	const auto RegisterCount() const { return registers.size(); }

	const auto FlagZero() const { return flags.zero; }
	void FlagZero(bool value) { flags.zero = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	const auto IP() const { return ip; }
	void IP(const TRegister value) { ip = value; GlobalEventQueue.enqueue(GlobalEventType::VMDirty, this); }

	void SetupForRun() override;
	void Step() override;
	void Stop() override;
};

export class RAM : public BaseMemory
{
	bool ExecuteNextInstruction() override;

public:
	RAM(size_t memory_size)
		: BaseMemory(memory_size)
	{
	}

	void SetupForRun() override;
	void Step() override;
	void Stop() override;
};

export VMInstruction MakeLoadRegister0AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeLoadRegister1AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeLoadRegister0Imm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeLoadRegister1Imm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeStoreRegister0AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeStoreRegister1AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeAddRegister0Imm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeAddRegister0AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeSubRegister0Imm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeSubRegister0AddressInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeJmpImm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeJmpNotZeroImm8Instruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeOutInstruction(initializer_list<uint8_t> opcode);
export VMInstruction MakeInInstruction(initializer_list<uint8_t> opcode);