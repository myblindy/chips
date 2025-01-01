#include "stdafx.h"

import std.core;
import vm;

using namespace std;

optional<shared_ptr<BaseMemory>> BaseMemory::NetworkVM(TNetworkIndex network_index, TIndexInNetwork index_in_network) const
{
	auto it = network_vms.find(network_index);
	if (it == network_vms.end())
		return nullopt;
	auto it2 = it->second.find(index_in_network);
	if (it2 == it->second.end())
		return nullopt;
	return it2->second;
}

optional<tuple<TMemory, optional<TRegister>>> BaseMemory::IncomingData(TIndexInNetwork index_in_network) const
{
	auto it = incoming_data.find(index_in_network);
	if (it == incoming_data.end())
		return nullopt;
	return it->second;
}

optional<string> BaseMemory::DecodeInstruction(size_t memory_index) const
{
	if (memory_index >= memory.size())
		return nullopt;
	auto it = instructions.find((size_t)Memory(memory_index));
	if (it == instructions.end())
		return nullopt;
	return it->second.Decode(this, memory_index);
}

size_t BaseMemory::IndexFromOpcode(const vector<uint8_t>& opcode) const
{
	switch (opcode.size())
	{
	case 1: return (size_t)opcode[0];
	case 2: return *reinterpret_cast<const uint16_t*>(opcode.data());
	case 3: case 4: return *reinterpret_cast<const uint32_t*>(opcode.data());
	default: assert(false); return 0;
	}
}
