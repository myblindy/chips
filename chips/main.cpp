#include "stdafx.h"

import std.core;
import vm;
import vm_machines;
import hex_editor;
import registers_view;
import memory_details_view;
import puzzles;

using namespace std;
using namespace ftxui;

static Component MakeVmContainer(shared_ptr<VM> vm, bool& success)
{
	auto hex_editor = HexEditor(vm->Memory(), [vm] { return vm->State() == VMState::Edit ? nullopt : make_optional(vm->IP()); },
		HexEditorOption::BytesPerLine(8));
	auto memory_details_view = MemoryDetailsView(vm, hex_editor, MemoryDetailsViewOption::Default());
	auto register_view = RegistersView(vm, RegistersViewOption::Default());

	auto hex_editor_window_contents = Container::Vertical({
		hex_editor | flex,
		Renderer([] { return separator(); }) | Maybe([vm] { return !vm->ErrorMessage().empty(); }),
		Container::Horizontal({
			Button("x", [vm] { vm->ErrorMessage().clear(); }, ButtonOption::Ascii()),
			Renderer([] { return separator(); }),
			Renderer([vm] { return text(vm->ErrorMessage()) | color(Color::Red) | blink; }),
			}) | Maybe([vm] { return !vm->ErrorMessage().empty(); }),
		Renderer([] { return separator(); }),
		Container::Horizontal({
			memory_details_view | size(WIDTH, GREATER_THAN, 30),
			Renderer([] { return separatorLight(); }) | Maybe([vm] { return vm->State() != VMState::Edit; }),
			register_view | Maybe([vm] { return vm->State() != VMState::Edit; }),
			})
		}) | xflex;

	auto hex_editor_window = Renderer(hex_editor_window_contents, [hex_editor_window_contents, vm]
		{
			auto title_element = hbox({
				text("Hex Editor for "),
				text(format("{}/{} {}", vm->NetworkIndex(), vm->IndexInNetwork(), vm->Name())) | color(Color::Aquamarine1),
				text(" ("),
				text(format("{}b", vm->MemorySize())) | color(Color::Aquamarine1),
				text(")"),
				});
			return window(title_element | hcenter | bold,
				hex_editor_window_contents->Render());
		});

	return hex_editor_window;
}

static Component MakeSuccessModal(bool& success, bool &show_puzzle_selection)
{
	auto sucess_modal_window_actions = Container::Horizontal({
		Button("OK", [&] { success = false; show_puzzle_selection = true; }, ButtonOption::Animated(Color::LightGreen)),
		});
	auto success_modal_window = Renderer(sucess_modal_window_actions, [=] { return window(
		text("Level completed!") | hcenter | bold,
		vbox({
			text("Details NYI"),
			text("Details NYI"),
			text("Details NYI"),
			text("Details NYI"),
			separator(),
			sucess_modal_window_actions->Render() | center,
			}));
		}) | size(WIDTH, GREATER_THAN, 30);
	return success_modal_window;
}

static Component MakePuzzleSelectionModal(const vector<string>& puzzle_names, int& selected_puzzle)
{
	auto puzzle_selection = Menu(&puzzle_names, &selected_puzzle);

	Components puzzle_tab_components;
	for (auto& puzzle : Puzzles)
		puzzle_tab_components.push_back(Renderer([&] { return puzzle.description_element; }));
	auto puzzle_tab_contents = Container::Tab(puzzle_tab_components, &selected_puzzle);

	return Window(WindowOptions{
		.inner = Container::Vertical({
			Container::Horizontal({
				puzzle_selection,
				Renderer([] { return separatorHeavy(); }),
				puzzle_tab_contents,
				}) | flex,
			Renderer([] { return separatorHeavy(); }),
			Button("Start", [] { GlobalEventQueue.enqueue(GlobalEventType::LoadNewPuzzle); }, ButtonOption::Animated(Color::LightGreen)),
			}),
		.title = "Select a puzzle",
		.width = 100,
		.height = 15,
		.resize_left = false,
		.resize_right = false,
		.resize_top = false,
		.resize_down = false,
		});
}

static Component MakeShell(int& selected_vm, bool& success, shared_ptr<PuzzleInstance> puzzle, int& selected_puzzle, bool& show_puzzle_selection,
	const vector<string>& puzzle_names, vector<string>& vm_tab_names)
{
	Component shell;
	if (puzzle)
	{
		Components vm_tab_components;
		for (auto& vm : puzzle->VMs())
			vm_tab_components.push_back(MakeVmContainer(vm, success));
		auto vm_tab_contents = Container::Tab(vm_tab_components, &selected_vm) | flex;

		vm_tab_names = puzzle->VMs() | ranges::views::transform([](const auto& vm) {
			return format("{}/{} {}", vm->NetworkIndex(), vm->IndexInNetwork(), vm->Name());
			}) | ranges::to<vector<string>>();
		auto vm_tab = Menu(&vm_tab_names, &selected_vm);

		auto main_content = Container::Horizontal({
			vm_tab_contents,
			Renderer([] { return separatorHeavy(); }),
			Container::Vertical({
				Renderer([] { return text("Devices:") | dim; }),
				vm_tab,
				Renderer([] { return filler(); }),
				}),
			});

		shell = Container::Vertical({
			main_content | flex,
			Renderer([] { return separatorHeavy(); }),
			Container::Horizontal({
				Button("Run", [puzzle] { puzzle->Run(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([puzzle] { return puzzle->State() != VMState::Running; }),
				Button("Pause", [puzzle] { puzzle->Pause(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([puzzle] { return puzzle->State() == VMState::Running; }),
				Button("Step", [puzzle] { puzzle->Step(); }, ButtonOption::Animated(Color::Aquamarine1)) | Maybe([puzzle] { return puzzle->State() != VMState::Running; }),
				Button("Stop", [puzzle] { puzzle->Stop(); }, ButtonOption::Animated(Color::Red)) | Maybe([puzzle] { return puzzle->State() != VMState::Edit; }),
				Renderer([] { return separatorHeavy(); }),
				Renderer([puzzle] { return puzzle->PuzzleTemplate().description_element | vcenter; }),
				}),
			Renderer([] { return separatorHeavy(); }),
			});

		shell |= Modal(MakeSuccessModal(success, show_puzzle_selection), &success);
	}
	else
		shell = Renderer([] { return text(""); });

	shell |= Modal(MakePuzzleSelectionModal(puzzle_names, selected_puzzle), &show_puzzle_selection);

	return shell;
}

int main()
{
	bool success = false;
	shared_ptr<PuzzleInstance> puzzle;

	auto screen = ScreenInteractive::Fullscreen();

	int selected_vm = 0;

	int selected_puzzle = -1;
	bool show_puzzle_selection = true;
	auto puzzle_names = Puzzles | ranges::views::transform([](const auto& puzzle) { return puzzle.name; }) | ranges::to<vector<string>>();
	vector<string> vm_tab_names;

	Component shell;

	unique_ptr<Loop> loop;

	auto load_puzzle = [&] {
		if (selected_puzzle >= 0)
			puzzle = Puzzles[selected_puzzle].make();
		else
			puzzle = nullptr;

		show_puzzle_selection = !puzzle;
		selected_vm = 0;

		shell = MakeShell(selected_vm, success, puzzle, selected_puzzle, show_puzzle_selection, puzzle_names, vm_tab_names);
		loop = make_unique<Loop>(&screen, shell);
		};
	load_puzzle();

	// append listeners to global events of interest to the UI
	VMEventQueue.appendListener(VMEventType::Dirty, [&](VM*) { screen.PostEvent(Event::Custom); });
	PuzzleEventQueue.appendListener(PuzzleEventType::Success, [&](PuzzleInstance*) { success = true; });
	GlobalEventQueue.appendListener(GlobalEventType::LoadNewPuzzle, [&] { load_puzzle(); });

	while (!loop->HasQuitted())
	{
		loop->RunOnce();

		// process event queues
		VMEventQueue.process();
		PuzzleEventQueue.process();
		GlobalEventQueue.process();
	}

	return 0;
}