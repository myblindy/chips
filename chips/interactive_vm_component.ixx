module;

#include "stdafx.h"

export module interactive_vm_component;

import std.core;
import vm;

using namespace std;
using namespace ftxui;

export struct InteractiveVMComponentOption
{
	static InteractiveVMComponentOption Default() { return {}; }

	shared_ptr<VM> vm;
};

export class InteractiveVMComponentBase : public ComponentBase, public InteractiveVMComponentOption
{
public:
	InteractiveVMComponentBase(InteractiveVMComponentOption option)
		: InteractiveVMComponentOption(move(option))
	{
	}

	Element Render() override final
	{
		// chip render
		auto c = Canvas(12, 12);
		c.DrawBlockLine(0, 2, 10, 2);
		c.DrawBlockLine(10, 0, 10, 10);
		c.DrawBlockLine(10, 8, 0, 8);
		c.DrawBlockLine(0, 0, 0, 10);
		c.DrawBlockLine(5, 0, 5, 2);
		c.DrawBlockLine(5, 8, 5, 10);

		return vbox({
			filler(),
			canvas(move(c)) | center,
			filler(),
			text(format("{}/{} {}", vm->IndexInNetwork(), vm->NetworkIndex(), vm->Name())) | color(Color::Aquamarine1),
			text("VM") | hcenter
			});
	}
};

export auto InteractiveVMComponent(shared_ptr<VM> vm, InteractiveVMComponentOption option = InteractiveVMComponentOption::Default())
{
	option.vm = vm;
	return Make<InteractiveVMComponentBase>(move(option));
}