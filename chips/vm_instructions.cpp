#include "stdafx.h"

import std.core;
import vm;

using namespace std;

VMInstruction MakeLoadRegister0AddressInstruction(initializer_list<uint8_t> opcode)
{
	return { "LDR0", vector<uint8_t>{opcode}, {Addr{}},
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
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (!vm.FlagZero())
				vm.IP(static_cast<TRegister>(operand_values[0]) - 2);
			return true;
		}
	};
}

VMInstruction MakeOutInstruction(initializer_list<uint8_t> opcode)
{
	return { "OUT", vector<uint8_t>{opcode}, {Reg{}},
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 2) return false;

			const auto dst_index = vm.Register(static_cast<TIndexInNetwork>(operand_values[0]));
			const auto address = vm.Register(0);
			const auto value = vm.Register(1);

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
		[](const VMInstruction& self, VM& vm, size_t memory_index, const vector<size_t>& operand_values) -> bool
		{
			if (vm.RegisterCount() < 1) return false;
			vm.FlagZero(vm.Register(0) == 0);
			return true;
		}
	};
}