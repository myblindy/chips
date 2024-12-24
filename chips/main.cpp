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

static Component MakeVmContainer(shared_ptr<VM> vm, bool& success, ScreenInteractive& screen)
{
	vm->OnSuccess([&](const VM&) { success = true; });
	vm->OnDirty([&](const VM&) { screen.PostEvent(Event::Custom); });

	auto hex_editor = HexEditor(vm->Memory(), [&] { return vm->State() == VMState::Edit ? nullopt : make_optional(vm->IP()); },
		HexEditorOption::BytesPerLine(8));
	auto memory_details_view = MemoryDetailsView(vm, hex_editor, MemoryDetailsViewOption::Default());
	auto register_view = RegistersView(vm, RegistersViewOption::Default());

	auto hex_editor_window_contents = Container::Vertical({
		hex_editor | flex,
		Renderer([] { return separator(); }) | Maybe([&] { return !vm->ErrorMessage().empty(); }),
		Container::Horizontal({
			Button("x", [&] { vm->ErrorMessage().clear(); }, ButtonOption::Ascii()),
			Renderer([] { return separator(); }),
			Renderer([&] { return text(vm->ErrorMessage()) | color(Color::Red) | blink; }),
			}) | Maybe([&] { return !vm->ErrorMessage().empty(); }),
		Renderer([] { return separator(); }),
		Container::Horizontal({
			memory_details_view | size(WIDTH, GREATER_THAN, 30),
			Renderer([] { return separatorLight(); }) | Maybe([&] { return vm->State() != VMState::Edit; }),
			register_view | Maybe([&] { return vm->State() != VMState::Edit; }),
			})
		}) | xflex;

	auto hex_editor_window = Renderer(hex_editor_window_contents, [hex_editor_window_contents, &vm]
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

int main()
{
	bool success = false;
	auto puzzle = Puzzles[0].make();

	auto screen = ScreenInteractive::Fullscreen();

	int selected_vm = 0;
	Components vm_tab_components;
	for (auto& vm : puzzle.VMs())
		vm_tab_components.push_back(MakeVmContainer(vm, success, screen));
	auto vm_tab_contents = Container::Tab(vm_tab_components, &selected_vm) | flex;

	auto vm_tab_names = puzzle.VMs() | ranges::views::transform([](const auto& vm) {
		return format("{}/{} {}", vm->NetworkIndex(), vm->IndexInNetwork(), vm->Name());
		}) | ranges::to<vector<string>>();
	auto vm_tab = Toggle(&vm_tab_names, &selected_vm);

	auto shell = Container::Vertical({
		Container::Horizontal({
			vm_tab_contents,
			Renderer([] { return separatorHeavy(); }),
			Container::Vertical({
				vm_tab,
				Renderer([] { return filler(); }),
				}),
			}) | flex,
		Renderer([] { return separatorHeavy(); }),
		Container::Horizontal({
			Button("Run", [&] { puzzle.Run(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([&] { return puzzle.State() != VMState::Running; }),
			Button("Pause", [&] { puzzle.Pause(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([&] { return puzzle.State() == VMState::Running; }),
			Button("Step", [&] { puzzle.Step(); }, ButtonOption::Animated(Color::Aquamarine1)) | Maybe([&] { return puzzle.State() != VMState::Running; }),
			Button("Stop", [&] { puzzle.Stop(); }, ButtonOption::Animated(Color::Red)) | Maybe([&] { return puzzle.State() != VMState::Edit; }),
			Renderer([] { return separatorHeavy(); }),
			Renderer([&] { return puzzle.PuzzleTemplate().description_element | vcenter; }),
			}),
		});

	auto sucess_modal_window_actions = Container::Horizontal({
		Button("OK", [&] { success = false; }, ButtonOption::Animated(Color::LightGreen)),
		});

	auto success_modal_window = Renderer(sucess_modal_window_actions, [&] { return window(
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

	shell |= Modal(success_modal_window, &success);

	screen.Loop(shell);

	return 0;
}