#include "stdafx.h"

import std.core;
import vm;

using namespace std;

void VM::SetupForRun()
{
	BaseMemory::SetupForRun();
	ip = 0;
}

void VM::Step()
{
	ExecuteNextInstruction();
}

bool VM::ExecuteNextInstruction()
{
#define ERROR_RETURN(msg) do{ error_message = (msg); return false; }while(0)
	const auto ip = static_cast<size_t>(this->ip);
	if (ip >= memory.size())
		ERROR_RETURN(format("IP ({:#04x}) is out of bounds ({:#04x}).", ip, memory.size()));

	auto it = instructions.find((size_t)Memory(ip));
	if (it == instructions.end())
		ERROR_RETURN("Invalid instruction opcode.");

	const auto& instruction = it->second;
	if (!instruction.Execute(*this, ip))
		ERROR_RETURN("Internal instruction error.");
	GlobalEventQueue.enqueue(GlobalEventType::VMInstructionExecuted, this);

	this->ip += static_cast<TRegister>(instruction.OpcodeLength());
	return true;
#undef ERROR_RETURN
}