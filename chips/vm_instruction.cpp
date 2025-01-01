#include "stdafx.h"

import std.core;
import vm;

using namespace std;

VMInstruction::VMInstruction(const char* name, const vector<TMemory> base_opcode, const vector<TOperand> operands, const char* base_description_markup, function<bool(const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values)> execute_internal)
	: name(name), base_opcode(base_opcode), operands(operands), execute_internal(execute_internal)
{
	// convert the base opcode to a string
	string opcode_string;
	for (auto&& opcode : base_opcode)
		opcode_string += format("{:02x} ", opcode);

	// add the instruction definition to the description markup
	string description_markup = format("`{}{}` ", opcode_string, name);

	int idx = 0;
	for (auto&& operand : operands)
	{
		if (idx)
			description_markup += ", ";

		visit(overload{
			[&](const Imm<1>&) { description_markup += format("`i8val{}`", idx); },
			[&](const Imm<2>&) { description_markup += format("`i16val{}`", idx); },
			[&](const Imm<4>&) { description_markup += format("`i32val{}`", idx); },
			[&](const Addr&) { description_markup += format("`addr{}`", idx); },
			[&](const Reg&) { description_markup += format("`reg{}`", idx); },
			}, operand);

		++idx;
	}

	description_element = BuildMarkupElement(description_markup + "\n" + base_description_markup);
}

size_t VMInstruction::OpcodeLength() const
{
	size_t length = base_opcode.size();
	for (auto&& operand : operands)
		visit([&](auto&& v) { length += remove_cvref_t<decltype(v)>::ByteSize; }, operand);

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
			[&](const Imm<1>&) { result += format("{:#04x}", instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			[&](const Imm<2>&) { result += format("{:#06x}", *reinterpret_cast<const uint16_t*>(instruction_stream.subspan(0, 2).data())); instruction_stream = instruction_stream.subspan(2); },
			[&](const Imm<4>&) { result += format("{:#010x}", *reinterpret_cast<const uint32_t*>(instruction_stream.subspan(0, 4).data())); instruction_stream = instruction_stream.subspan(4); },
			[&](const Addr&) {
				auto address = *reinterpret_cast<const TAddress*>(instruction_stream.subspan(0, sizeof(TAddress)).data());
				if (sizeof(TAddress) == 4)
					result += format("[{:#010x}]", static_cast<intptr_t>(address));
				else if (sizeof(TAddress) == 2)
					result += format("[{:#06x}]", static_cast<intptr_t>(address));
				else if (sizeof(TAddress) == 1)
					result += format("[{:#04x}]", static_cast<intptr_t>(address));
				else
					assert(false);
				instruction_stream = instruction_stream.subspan(sizeof(TAddress));
			},
			[&](const Reg&) { result += memory->RegisterName(instruction_stream[0]); instruction_stream = instruction_stream.subspan(1); },
			}, operand);
	}
	return result;
}
