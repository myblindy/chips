#include "stdafx.h"

import std.core;
import vm;

using namespace std;

size_t VMInstruction::OpcodeLength() const
{
	size_t length = base_opcode.size();
	for (auto&& operand : operands)
	{
		visit(overload{
			[&](const Imm<1>&) { length += 1; },
			[&](const Imm<2>&) { length += 2; },
			[&](const Imm<4>&) { length += 4; },
			[&](const Addr&) { length += sizeof(TAddress); },
			[&](const Reg&) { length += 1; },
			}, operand);
	}
	return length;
}

bool VMInstruction::Execute(VM& vm, size_t memory_index) const
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
			[&](const Addr&) { operand_values.push_back(*reinterpret_cast<const TAddress*>(instruction_stream.subspan(0, sizeof(TAddress)).data())); instruction_stream = instruction_stream.subspan(sizeof(TAddress)); },
			[&](const Reg&) { operand_values.push_back(instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			}, operand);
	}

	if (!execute_internal || !execute_internal(*this, vm, memory_index, operand_values))
		return false;
	return true;
}

optional<string> VMInstruction::Decode(const BaseMemory* memory, size_t memory_index) const
{
	if (memory_index + OpcodeLength() > memory->MemorySize())
		return nullopt;

	auto instruction_stream = memory->Memory({ memory_index, OpcodeLength() });
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
			},
			[&](const Reg&) { result += memory->RegisterName(instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			}, operand);
	}
	return result;
}
