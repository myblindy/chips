#include "stdafx.h"

import std.core;
import vm;

using namespace std;

void RAM::SetupForRun()
{
	saved_memory = memory;
}

void RAM::Stop()
{
	memory = saved_memory;
	error_message.clear();
}

void RAM::Step()
{
	ExecuteNextInstruction();
}

bool RAM::ExecuteNextInstruction()
{
	// memories only respond to IN and OUT instructions
	for (auto& [index, data] : incoming_data)
		if (data)
			if (get<1>(*data))
			{
				// write operation
				Memory(get<0>(*data), *get<1>(*data));
				incoming_data[index] = nullopt;
			}
			else
			{
				// read operation
				auto src_vm = NetworkVM(index);
				if (src_vm)
					(*src_vm)->IncomingData(IndexInNetwork(), { { get<0>(*data), Memory(get<0>(*data)) } });
				incoming_data[index] = nullopt;
			}

	return true;
}