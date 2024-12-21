module;

#include "stdafx.h"

export module registers_view;

import std.core;
import vm;

using namespace std;
using namespace ftxui;

export struct RegistersViewOption
{
	static RegistersViewOption Default();

	shared_ptr<VM> vm;
};

export class RegistersViewBase : public ComponentBase, public RegistersViewOption
{
public:
	RegistersViewBase(RegistersViewOption option)
		: RegistersViewOption(move(option))
	{
	}

	Element Render() override final
	{
		Elements elements;
		elements.reserve((vm->RegisterCount() + 1) * 3);

		elements.push_back(text("IP: ") | bold | dim);
		elements.push_back(text(format("{:#04x}", vm->IP())));

		for (int reg = 0; reg < vm->RegisterCount(); ++reg)
		{
			elements.push_back(separatorLight());
			elements.push_back(text(vm->RegisterName(reg) + ": ") | bold | dim);
			elements.push_back(text(format("{:#04x}", vm->Register(reg))));
		}
		return hbox(elements);
	}
};

export auto RegistersView(shared_ptr<VM> vm, RegistersViewOption option)
{
	option.vm = vm;
	return Make<RegistersViewBase>(move(option));
}

RegistersViewOption RegistersViewOption::Default()
{
	return RegistersViewOption();
}
