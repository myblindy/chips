#include "stdafx.h"

import vm;
import vm_machines;
import hex_editor;
import registers_view;
import memory_details_view;
import puzzles;

using namespace std;
using namespace ftxui;

int main()
{
	auto&& puzzle = Puzzle0101;
	auto vm = puzzle.make_vm();

	string error_message;
	vm->OnError([&](const VM&, const string_view message) { error_message = message; });

	bool success = false;
	vm->OnSuccess([&](const VM&) { success = true; });

	// LDR 0x08
	vm->Memory(0x00, 0x00);
	vm->Memory(0x01, 0x08);

	// ADDI8 0x01
	vm->Memory(0x02, 0x03);
	vm->Memory(0x03, 0x01);

	// STR 0x0A
	vm->Memory(0x04, 0x02);
	vm->Memory(0x05, 0x0A);

	// JMP 0x02
	vm->Memory(0x06, 0x04);
	vm->Memory(0x07, 0x02);

	vm->Memory(0x08, 0xEF);

	auto screen = ScreenInteractive::Fullscreen();
	vm->OnDirty([&](const VM&) { screen.PostEvent(Event::Custom); });

	auto hex_editor = HexEditor(vm->Memory(), [&] { return vm->State() == VMState::Edit ? nullopt : make_optional(vm->IP()); },
		HexEditorOption::BytesPerLine(8));
	auto memory_details_view = MemoryDetailsView(vm, hex_editor, MemoryDetailsViewOption::Default());
	auto register_view = RegistersView(vm, RegistersViewOption::Default());

	auto hex_editor_window_contents = Container::Vertical({
		hex_editor | flex,
		Renderer([] { return separator(); }) | Maybe([&] { return !error_message.empty(); }),
		Container::Horizontal({
			Button("x", [&] { error_message.clear(); }, ButtonOption::Ascii()),
			Renderer([] { return separator(); }),
			Renderer([&] { return text(error_message) | color(Color::Red) | blink; }),
			}) | Maybe([&] { return !error_message.empty(); }),
		Renderer([] { return separator(); }),
		Container::Horizontal({
			memory_details_view,
			Renderer([] { return separatorLight(); }) | Maybe([&] { return vm->State() != VMState::Edit; }),
			register_view | Maybe([&] { return vm->State() != VMState::Edit; }),
			})
		}) | xflex;

	auto hex_editor_window = Renderer(hex_editor_window_contents, [&]
		{
			return window(text(format("Hex Editor ({} bytes)", vm->MemorySize())) | hcenter | bold,
				hex_editor_window_contents->Render());
		}) | flex;

	auto shell = Container::Vertical({
		hex_editor_window,
		Renderer([] { return separatorHeavy(); }),
		Container::Horizontal({
			Button("Run", [&] { vm->Run(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([&] { return vm->State() != VMState::Running; }),
			Button("Pause", [&] { vm->Pause(); }, ButtonOption::Animated(Color::LightGreen)) | Maybe([&] { return vm->State() == VMState::Running; }),
			Button("Step", [&] { vm->Step(); }, ButtonOption::Animated(Color::Aquamarine1)) | Maybe([&] { return vm->State() != VMState::Running; }),
			Button("Stop", [&] { vm->Stop();  }, ButtonOption::Animated(Color::Red)) | Maybe([&] { return vm->State() != VMState::Edit; }),
			Renderer([] { return separatorHeavy(); }),
			Renderer([&] { return puzzle.description_element | vcenter; }),
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
}