module;

#include "stdafx.h"

export module interactive_display_component;

import std.core;
import vm;

using namespace std;
using namespace ftxui;

export struct InteractiveDisplayComponentOption
{
	static InteractiveDisplayComponentOption Default() { return {}; }

	shared_ptr<Display> display;
};

export class InteractiveDisplayComponentBase : public ComponentBase, public InteractiveDisplayComponentOption
{
public:
	InteractiveDisplayComponentBase(InteractiveDisplayComponentOption option)
		: InteractiveDisplayComponentOption(move(option))
	{
	}

	Element Render() override final
	{
		// display box
		Elements vbox_elements;
		vbox_elements.reserve(display->Height());

		for (size_t y = 0; y < display->Height(); ++y)
		{
			Elements hbox_elements;
			hbox_elements.reserve(display->Width());
			for (size_t x = 0; x < display->Width(); ++x)
			{
				const auto ch = (char)display->Memory(y * display->Width() * 3 + x * 3);
				const auto fg = (Color::Palette256)display->Memory(y * display->Width() * 3 + x * 3 + 1);
				const auto bg = (Color::Palette256)display->Memory(y * display->Width() * 3 + x * 3 + 2);

				hbox_elements.push_back(text(format("{}", ch ? ch : ' '))
					| color(fg) | bgcolor(bg));
			}
			vbox_elements.push_back(hbox(move(hbox_elements)));
		}

		return vbox({
			hbox({
				filler(),
				vbox(move(vbox_elements)) | borderRounded,
				filler(),
				}),
			text(format("{}/{} {}", display->IndexInNetwork(), display->NetworkIndex(), display->Name())) | color(Color::Aquamarine1) | hcenter,
			text("Display") | hcenter
			});
	}
};

export auto InteractiveDisplayComponent(shared_ptr<Display> vm, InteractiveDisplayComponentOption option = InteractiveDisplayComponentOption::Default())
{
	option.display = vm;
	return Make<InteractiveDisplayComponentBase>(move(option));
}