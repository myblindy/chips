module;

#include "stdafx.h"

export module puzzle;

import std.core;
import vm;
import vm_machines;

using namespace std;
using namespace ftxui;

export struct PuzzleInstance;

export struct Puzzle
{
	using TCheck = function<bool(PuzzleInstance &puzzle_instance)>;

	function<PuzzleInstance()> make;
	Element description_element;

	vector<TCheck> checks;
	vector<VM::TMakeNetwork> internal_make_networks;

	Puzzle(const vector<VM::TMakeNetwork>& make_networks,
		const string& description_markup, const vector<TCheck>& checks);

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

export struct PuzzleInstance
{
	PuzzleInstance(Puzzle& puzzle, const vector<shared_ptr<VM>>& vms)
		: puzzle(puzzle), vms(vms)
	{
	}

	shared_ptr<VM> VM(size_t index) { return vms[index]; }
	vector<shared_ptr<::VM>>& VMs() { return vms; }

	VMState State() const { return vms[0]->State(); }

	Puzzle& PuzzleTemplate() const { return puzzle; }

	void Run()
	{
		if (State() == VMState::Edit)
			check_index = 0;

		for (auto& vm : vms)
			vm->Run();
	}

	void Step()
	{
		if (State() == VMState::Edit)
			check_index = 0;

		for (auto& vm : vms)
			vm->Step();
	}

	void Pause()
	{
		for (auto& vm : vms)
			vm->Pause();
	}

	void Stop()
	{
		for (auto& vm : vms)
			vm->Stop();
	}

	void OnSuccess(function<void(PuzzleInstance&)> callback)
	{
		success_callbacks.push_back(callback);
	}

private:
	Puzzle& puzzle;
	int check_index{};
	vector<shared_ptr<::VM>> vms;

	vector<function<void(PuzzleInstance&)>> success_callbacks;
	void TriggerSuccessCallbacks()
	{
		for (auto& callback : success_callbacks)
			callback(*this);
	}

	bool RunChecks() 
	{
		if (check_index < puzzle.checks.size() && puzzle.checks[check_index](*this))
			++check_index;
		return check_index == puzzle.checks.size();
	}
};

inline Puzzle::Puzzle(const vector<VM::TMakeNetwork>& make_networks,
	const string& description_markup, const vector<TCheck>& checks)
{
	description_element = BuildMarkupElement(description_markup);

	internal_make_networks = make_networks;
	this->checks = checks;

	this->make = [&]()
		{
			auto vms = this->internal_make_networks
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

									int memory_index = 0;
									for (auto&& b : get<2>(get<1>(make_network_w)))
										vm->Memory(memory_index++, b);

									return vm;
								});
					})
				| ranges::views::join
				| ranges::to<vector<shared_ptr<VM>>>();

			return PuzzleInstance(*this, vms);
		};
}
