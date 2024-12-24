module;

#include "stdafx.h"

export module puzzles;

import std.core;
import puzzle;
import vm_machines;

using namespace std;

export array Puzzles
{
	Puzzle {
		{ { { "CPU", MakeTest01Machine } } },
		"Load `0xDE` at `0x10` and `0xAD` at `0x11` in memory.\n",
		{
			[](const VM& vm) { return vm.Memory(0x10) == 0xDE && vm.Memory(0x11) == 0xAD; }
		}
	}
};