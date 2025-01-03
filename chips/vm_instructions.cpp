#include "stdafx.h"

import std.core;
import vm;

using namespace std;

VMInstruction MakeLoadRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR0", vector<uint8_t>{opcode}, {Addr{}},
		"Loads the value at `addr0` into `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Register(0, vm.Memory(address));
			return true;
		}
	};
}

VMInstruction MakeLoadRegister1AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR1", vector<uint8_t>{opcode}, {Addr{}},
		"Loads the value at `addr0` into `R1`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Register(1, vm.Memory(address));
			return true;
		}
	};
}

VMInstruction MakeLoadRegister0Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "LDR0I8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Loads the immediate value `i8val0` into `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

VMInstruction MakeLoadRegister1Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "LDR1I8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Loads the immediate value `i8val0` into `R1`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			vm.Register(1, static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

VMInstruction MakeStoreRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "STR0", vector<uint8_t>{opcode}, {Addr{}},
		"Stores the value in `R0` at `addr0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Memory(address, vm.Register(0));
			return true;
		}
	};
}

VMInstruction MakeStoreRegister1AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "STR1", vector<uint8_t>{opcode}, {Addr{}},
		"Stores the value in `R1` at `addr0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;

			vm.Memory(address, vm.Register(1));
			return true;
		}
	};
}

VMInstruction MakeAddRegister0Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "ADDI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Adds the immediate value `i8val0` to `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, vm.Register(0) + static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

VMInstruction MakeAddRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "ADD", vector<uint8_t>{opcode}, {Addr{}},
		"Adds the value at `addr0` to `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;
			vm.Register(0, vm.Register(0) + vm.Memory(address));
			return true;
		}
	};
}

VMInstruction MakeSubRegister0Imm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "SUBI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Subtracts the immediate value `i8val0` from `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.Register(0, vm.Register(0) - static_cast<TRegister>(operand_values[0]));
			return true;
		}
	};
}

VMInstruction MakeSubRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "SUB", vector<uint8_t>{opcode}, {Addr{}},
		"Subtracts the value at `addr0` from `R0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			const auto address = operand_values[0];
			if (address >= vm.MemorySize()) return false;
			vm.Register(0, vm.Register(0) - vm.Memory(address));
			return true;
		}
	};
}

VMInstruction MakeJmpImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "JMPI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Jumps to the immediate value `i8val0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			vm.IP(static_cast<TRegister>(operand_values[0]) - 2);
			return true;
		}
	};
}

VMInstruction MakeJmpNotZeroImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "JMPNZI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Jumps to the immediate value `i8val0` if the zero flag is not set.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (!vm.FlagZero())
				vm.IP(static_cast<TRegister>(operand_values[0]) - 2);
			return true;
		}
	};
}

VMInstruction MakeOutImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "OUTI8", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Send the value `i8val0` to network device at `R0` and address `R1`.\nSets the zero flag in case of error or buffer full.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;

			const auto dst_index = vm.Register(0);
			const auto address = vm.Register(1);
			const auto value = static_cast<TRegister>(operand_values[0]);

			auto dst_vm = vm.NetworkVM(dst_index);
			if (!dst_vm || !(*dst_vm)->IncomingData(vm.IndexInNetwork(), { { address, value } }))
				vm.FlagZero(true);
			else
				vm.FlagZero(false);
			return true;
		}
	};
}

VMInstruction MakeInInstruction(initializer_list<uint8_t> opcode)
{
	return { "IN", vector<uint8_t>{opcode}, {},
		"Requests a value from the network device at index `R0` and address `R1`.\nEither sets the zero flag if no data received,\nor data is received in `R0` and zero flag is cleared.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;

			const auto src_index = vm.Register(0);
			const auto value = vm.IncomingData(src_index);
			if (!value)
			{
				vm.FlagZero(true);

				// send the request
				const auto src_address = vm.Register(1);
				auto src_vm = vm.NetworkVM(src_index);
				if (src_vm)
					(*src_vm)->IncomingData(vm.IndexInNetwork(), { { src_address, {} } });
			}
			else
			{
				vm.FlagZero(false);
				vm.Register(0, *get<1>(*value));
				vm.IncomingData(src_index, nullopt, true);
			}

			return true;
		}
	};
}

VMInstruction MakeTestZeroInstruction(initializer_list<uint8_t> opcode)
{
	return { "TESTZ", vector<uint8_t>{opcode}, {},
		"Sets the zero flag if `R0` is zero.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.FlagZero(vm.Register(0) == 0);
			return true;
		}
	};
}

VMInstruction MakeTestGreaterThanImm8Instruction(initializer_list<uint8_t> opcode)
{
	return { "TESTGT", vector<uint8_t>{opcode}, {Imm<1>{}},
		"Sets the zero flag if `R0` is greater than `i8val0`.",
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.FlagZero(vm.Register(0) > operand_values[0]);
			return true;
		}
	};
}