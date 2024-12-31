module;

#include "stdafx.h"

export module puzzles;

import std.core;
import puzzle;
import vm_machines;

using namespace std;

static default_random_engine random_engine;

export array Puzzles
{
	Puzzle {
		{ { { "CPU", MakeTest01Machine, true, { 0x00, 0x08, 0x03, 0x01, 0x02, 0x0A,0x04, 0x02, 0xEF } } } },
		"Shitty-Simple Test",
		"Load `0xDE` at `0x10` and `0xAD` at `0x11` in memory.\n",
		[](auto& puzzle_instance) {},
		{
			[](auto& puzzle_instance) {
				return puzzle_instance.VM(0)->Memory(0x10) == 0xDE && puzzle_instance.VM(0)->Memory(0x11) == 0xAD;
			}
		}
	},
	Puzzle {
		{
			{
				{ "CPU", MakeTest01Machine, true, {} },
				{ "ROM", MakeRAM128Machine, false, {} },
			},
		},
		"Simple Networked Test",
		"Read `0x00` from ROM to get the data length,\nthen sum that many bytes from ROM starting\nat `0x01` and write their sum in the CPU at `0x1D`.",
		[](auto& puzzle_instance) {
			auto rom = puzzle_instance.VM(1);

			uniform_int_distribution<uint16_t> dist_size(1, static_cast<uint16_t>(rom->MemorySize()));
			auto data_length = static_cast<TMemory>(dist_size(random_engine));
			rom->Memory(0, data_length);

			uniform_int_distribution<uint16_t> dist_byte(0, 255);
			for (size_t i = 0; i < data_length; ++i)
				rom->Memory(1 + i, static_cast<TMemory>(dist_byte(random_engine)));
		},
		{
			[](auto& puzzle_instance) {
				auto cpu = puzzle_instance.VM(0);
				auto rom = puzzle_instance.VM(1);

				auto data_length = rom->Memory(0);
				uint8_t sum = 0;
				for (size_t i = 0; i < data_length; ++i)
					sum += rom->Memory(1 + i);
				return cpu->Memory(0x1D) == sum;
			}
		}
	},
};