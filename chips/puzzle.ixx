module;

#include "stdafx.h"

export module puzzle;

import std.core;
import vm;
import vm_machines;

using namespace std;
using namespace ftxui;

export struct Puzzle
{
	function<shared_ptr<VM>()> make_vm;
	Element description_element;

	Puzzle(const function<shared_ptr<VM>()>& make_vm, const string& description_markup,
		const vector<VM::TCheck>& checks)
	{
		SetupDescription(description_markup);

		internal_make_vm = make_vm;
		this->checks = checks;
		this->make_vm = [&]() -> shared_ptr<VM>
			{
				auto vm = internal_make_vm();
				for (auto&& check : this->checks)
					vm->AddCheck(check);
				return vm;
			};
	}

private:
	vector<VM::TCheck> checks;
	function<shared_ptr<VM>()> internal_make_vm;
	void SetupDescription(const std::string& description_markup)
	{
		Elements vbox_elements;
		Elements hbox_elements;

		string entry;
		bool entry_highlight = false;

		auto add_entry = [&](bool new_line)
			{
				if (entry.size() > 0)
				{
					auto element = text(entry);
					if (entry_highlight)
						element = element | color(Color::Aquamarine1);

					hbox_elements.push_back(element);

					if (new_line)
					{
						vbox_elements.push_back(hbox(hbox_elements));
						hbox_elements.clear();
					}

					entry.clear();
				}
			};

		for (auto c : description_markup)
			if (c == '`')
			{
				add_entry(false);
				entry_highlight = !entry_highlight;
			}
			else if (c == '\n')
			{
				add_entry(true);
				entry.clear();
			}
			else
			{
				entry += c;
			}
		add_entry(true);

		description_element = vbox(vbox_elements);
	}
};