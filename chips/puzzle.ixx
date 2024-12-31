module;

#include "stdafx.h"

export module puzzle;

import std.core;
import vm;
import vm_machines;

using namespace std;
using namespace ftxui;

export enum class PuzzleState
{
	Edit,
	Running,
	Paused
};

export struct PuzzleInstance;

export struct Puzzle
{
	using TNetwork = vector<shared_ptr<BaseMemory>>;
	using TMakeNetwork = vector<tuple<string, function<shared_ptr<BaseMemory>()>, bool, vector<uint8_t>>>;
	using TCheck = function<bool(PuzzleInstance& puzzle_instance)>;
	using TSetup = function<void(PuzzleInstance& puzzle_instance)>;

	function<shared_ptr<PuzzleInstance>()> make;
	string name;
	Element description_element;

	vector<TCheck> checks;
	TSetup setup;
	vector<TMakeNetwork> internal_make_networks;

	Puzzle(const vector<TMakeNetwork>& make_networks,
		const string& name_markup, const string& description_markup,
		const TSetup setup, const vector<TCheck>& checks);

	Element BuildMarkupElement(const std::string& description_markup);
};

export struct PuzzleInstance
{
	PuzzleInstance(Puzzle& puzzle, const vector<shared_ptr<BaseMemory>>& vms);

	shared_ptr<BaseMemory> VM(size_t index) { return vms[index]; }

	shared_ptr<BaseMemory> VM(TNetworkIndex network_index, TIndexInNetwork index_in_network)
	{
		auto result = vms
			| ranges::views::filter([=](auto&& vm) { return vm->NetworkIndex() == network_index && vm->IndexInNetwork() == index_in_network; });
		return *begin(result);
	}

	vector<shared_ptr<BaseMemory>>& VMs() { return vms; }
	vector<shared_ptr<BaseMemory>> VMs(TNetworkIndex network_index)
	{
		return vms
			| ranges::views::filter([=](auto&& vm) { return vm->NetworkIndex() == network_index; })
			| ranges::to<vector<shared_ptr<BaseMemory>>>();
	}

	PuzzleState State() const { return state; }

	Puzzle& PuzzleTemplate() const { return puzzle; }

	void SetupForRun();
	void Run();
	void Step();
	void Pause();
	void Stop();

private:
	Puzzle& puzzle;
	int check_index{};
	vector<shared_ptr<BaseMemory>> vms;

	atomic<PuzzleState> state = PuzzleState::Edit;
	SDL_TimerID timer{};

	bool RunChecks()
	{
		if (check_index < puzzle.checks.size() && puzzle.checks[check_index](*this))
			++check_index;
		return check_index == puzzle.checks.size();
	}
};

inline Puzzle::Puzzle(const vector<TMakeNetwork>& make_networks,
	const string& name, const string& description_markup,
	const TSetup setup, const vector<TCheck>& checks) : name(name)
{
	description_element = BuildMarkupElement(description_markup);

	internal_make_networks = make_networks;
	this->checks = checks;
	this->setup = setup;

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
									vm->Editable(get<2>(get<1>(make_network_w)));

									int memory_index = 0;
									for (auto&& b : get<3>(get<1>(make_network_w)))
										vm->Memory(memory_index++, b);

									return vm;
								});
					})
				| ranges::views::join
				| ranges::to<vector<shared_ptr<BaseMemory>>>();

			for (auto&& vm : vms)
				for (auto&& vm2 : vms)
					if (vm != vm2)
					{
						vm->AddNetworkedVM(vm2->NetworkIndex(), vm2->IndexInNetwork(), vm2);
						vm2->AddNetworkedVM(vm->NetworkIndex(), vm->IndexInNetwork(), vm);
					}
					else
						vm->AddNetworkedVM(vm->NetworkIndex(), vm->IndexInNetwork(), vm);

			return make_shared<PuzzleInstance>(*this, vms);
		};
}

inline Element Puzzle::BuildMarkupElement(const std::string& description_markup)
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

inline PuzzleInstance::PuzzleInstance(Puzzle& puzzle, const vector<shared_ptr<BaseMemory>>& vms)
	: puzzle(puzzle), vms(vms)
{
	GlobalEventQueue.appendListener(GlobalEventType::VMInstructionExecuted, [&](TGlobalEventSource)
		{
			if (RunChecks())
			{
				Stop();
				GlobalEventQueue.enqueue(GlobalEventType::PuzzleSuccess, this);
			}
		});
}

inline void PuzzleInstance::SetupForRun()
{
	for (auto& vm : vms)
		vm->SetupForRun();

	check_index = 0;
	puzzle.setup(*this);
}

inline void PuzzleInstance::Run()
{
	if (State() == PuzzleState::Edit)
		SetupForRun();

	state = PuzzleState::Running;
	timer = SDL_AddTimerNS(250ULL * 100000, [](auto userdata, auto id, auto interval) -> uint64_t
		{
			auto&& self = reinterpret_cast<PuzzleInstance*>(userdata);
			for (auto& vm : self->vms)
				vm->Step();
			return interval;
		}, this);
	assert(timer);
}

inline void PuzzleInstance::Step()
{
	if (State() == PuzzleState::Edit)
		SetupForRun();

	state = PuzzleState::Paused;
	if (timer)
	{
		SDL_RemoveTimer(timer);
		timer = 0;
	}

	for (auto& vm : vms)
		vm->Step();
}

inline void PuzzleInstance::Pause()
{
	state = PuzzleState::Paused;
	if (timer)
	{
		SDL_RemoveTimer(timer);
		timer = 0;
	}
}

inline void PuzzleInstance::Stop()
{
	state = PuzzleState::Edit;
	if (timer)
	{
		SDL_RemoveTimer(timer);
		timer = 0;
	}
	for (auto& vm : vms)
		vm->Stop();
}
