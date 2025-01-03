#include "stdafx.h"

import vm;

using namespace std;

void Display::Step()
{
	ExecuteNextInstruction();
}

bool Display::ExecuteNextInstruction()
{
	for (auto& [index, data] : incoming_data)
		if (data)
			if (get<1>(*data))
			{
				// write operation
				if(!Memory(get<0>(*data), *get<1>(*data)))
					return false;
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