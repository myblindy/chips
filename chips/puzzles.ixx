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
		"Shitty-Simple Test",
		"Load `0xDE` at `0x10` and `0xAD` at `0x11` in memory.\n",
		{
			[](PuzzleInstance& puzzle_instance) {
				return puzzle_instance.VM(0)->Memory(0x10) == 0xDE && puzzle_instance.VM(0)->Memory(0x11) == 0xAD;
			}
		}
	},
	Puzzle {
		{
			{ { "CPU", MakeTest01Machine, {} } },
			{ { "ROM", MakeROM128Machine, {} } }
		},
		"Simple Networked Test",
		"Read `0x00` from ROM to get the data length,\nthen sum that many bytes from ROM starting\nat `0x01` and write their sum in the CPU at `0x20`.",
		{
			[](PuzzleInstance& puzzle_instance) {
				auto cpu = puzzle_instance.VM(0);
				auto rom = puzzle_instance.VM(1);

				auto data_length = rom->Memory(0);
				uint8_t sum = 0;
				for (int i = 0; i < data_length; ++i)
					sum += rom->Memory(1 + i);
				return cpu->Memory(0x20) == sum;
			}
		}
	},
};