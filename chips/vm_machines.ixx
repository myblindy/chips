module;

#include "stdafx.h"

export module vm_machines;

import std;
import vm;

using namespace std;

export shared_ptr<VM> MakeTest01Machine()
{
	return make_shared<VM>(2, 30,
		vector {
			MakeLoadRegister0AddressInstruction({ 0x00 }),
			MakeLoadRegister1AddressInstruction({ 0x01 }),
			MakeLoadRegister0Imm8Instruction({ 0x02 }),
			MakeLoadRegister1Imm8Instruction({ 0x03 }),
			MakeStoreRegister0AddressInstruction({ 0x04 }),
			MakeStoreRegister1AddressInstruction({ 0x05 }),
			MakeAddRegister0Imm8Instruction({ 0x06 }),
			MakeJmpImm8Instruction({ 0x07 }),
			MakeJmpNotZeroImm8Instruction({ 0x08 }),
			MakeOutInstruction({ 0x09 }),
			MakeInInstruction({ 0x0A }),
		});
}

export shared_ptr<VM> MakeROM128Machine()
{
	return make_shared<VM>(1, 128);
}