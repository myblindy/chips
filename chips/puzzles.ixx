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
		{ { { "CPU", MakeTest01Machine, { 0x00, 0x08, 0x03, 0x01, 0x02, 0x0A,0x04, 0x02, 0xEF } } } },
		"Load `0xDE` at `0x10` and `0xAD` at `0x11` in memory.\n",
		{
			[](PuzzleInstance& puzzle_instance) {
				return puzzle_instance.VM(0)->Memory(0x10) == 0xDE && puzzle_instance.VM(0)->Memory(0x11) == 0xAD;
			}
		}
	}
};