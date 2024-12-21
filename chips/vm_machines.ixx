module;

#include "stdafx.h"

export module vm_machines;

import std;
import vm;

using namespace std;

export shared_ptr<VM> MakeTest01Machine()
{
	return make_shared<VM>(1, 30, initializer_list<const VMInstruction>{
		MakeLoadRegisterSingleAddressInstruction({ 0x00 }),
		MakeLoadRegisterSingleImm8Instruction({ 0x01 }),
		MakeStoreRegisterSingleAddressInstruction({ 0x02 }),
		MakeAddRegisterImm8Instruction({ 0x03 }),
		MakeJmpImm8Instruction({ 0x04 }),
	});
}