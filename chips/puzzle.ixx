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
	function<vector<shared_ptr<VM>>()> make;
	Element description_element;

	Puzzle(const vector<VM::TMakeNetwork>& make_networks,
		const string& description_markup,
		const vector<VM::TCheck>& checks)
	{
		description_element = BuildMarkupElement(description_markup);

		internal_make_networks = make_networks;
		this->checks = checks;
		this->make = [&]()
			{
				vector<shared_ptr<VM>> result;

				auto elements_count_range = this->internal_make_networks
					| ranges::views::transform([](auto&& elem) { return elem.size(); });
				result.reserve(ranges::fold_left_first(elements_count_range, plus{}).value_or(0));

				return this->internal_make_networks
					| ranges::views::enumerate
					| ranges::views::transform([](auto&& make_networks_w)
						{
							auto network_index = static_cast<uint8_t>(get<0>(make_networks_w));
							return get<1>(make_networks_w)
								| ranges::views::enumerate
								| ranges::views::transform([=](auto&& make_network_w)
									{
										auto vm = get<1>(get<1>(make_network_w))();
										vm->Name(get<0>(get<1>(make_network_w)));
										vm->IndexInNetwork(static_cast<uint8_t>(get<0>(make_network_w)));
										vm->NetworkIndex(network_index);
										return vm;
									});
						})
					| ranges::views::join
					| ranges::to<vector<shared_ptr<VM>>>();
			};
	}

private:
	vector<VM::TCheck> checks;
	vector<VM::TMakeNetwork> internal_make_networks;
	Element BuildMarkupElement(const std::string& description_markup)
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

		return vbox(vbox_elements);
	}
};