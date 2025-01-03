module;

#include "stdafx.h"

export module vm_machines;

import std;
import vm;

using namespace std;

vector instruction_set_01 = {
	MakeLoadRegister0AddressInstruction({ 0x00 }),
	MakeLoadRegister1AddressInstruction({ 0x01 }),
	MakeLoadRegister0Imm8Instruction({ 0x02 }),
	MakeLoadRegister1Imm8Instruction({ 0x03 }),
	MakeStoreRegister0AddressInstruction({ 0x04 }),
	MakeStoreRegister1AddressInstruction({ 0x05 }),
	MakeAddRegister0Imm8Instruction({ 0x06 }),
	MakeAddRegister0AddressInstruction({ 0x07 }),
	MakeSubRegister0Imm8Instruction({ 0x08 }),
	MakeSubRegister0AddressInstruction({ 0x09 }),
	MakeJmpImm8Instruction({ 0x0A }),
	MakeJmpNotZeroImm8Instruction({ 0x0B }),
	MakeOutImm8Instruction({ 0x0C }),
	MakeInInstruction({ 0x0D }),
	MakeTestZeroInstruction({ 0x0E }),
	MakeTestGreaterThanImm8Instruction({ 0x0F }),
	};

export auto MakeTest01Machine()
{
	return make_shared<VM>(2, 30, instruction_set_01);
}

export auto MakeTest02Machine()
{
	return make_shared<VM>(2, 128, instruction_set_01);
}

export auto MakeRAM128Machine()
{
	return make_shared<RAM>(128);
}

export auto MakeDisplay4x4Machine()
{
	return make_shared<Display>(4, 4);
}