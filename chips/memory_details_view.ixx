module;

#include "stdafx.h"

export module memory_details_view;

import std.core;
import hex_editor;
import puzzle;
import vm;

using namespace std;
using namespace ftxui;

export struct MemoryDetailsViewOption
{
	static MemoryDetailsViewOption Default();

	shared_ptr<PuzzleInstance> puzzle;
	shared_ptr<VM> vm;
	shared_ptr<HexEditorBase> hex_editor;
};

export class MemoryDetailsViewBase : public ComponentBase, public MemoryDetailsViewOption
{
public:
	MemoryDetailsViewBase(MemoryDetailsViewOption option)
		: MemoryDetailsViewOption(move(option))
	{
	}

	Element Render() override final
	{
		auto selected_address = *hex_editor->cursor_half_byte_position / 2;
		if (puzzle->State() == PuzzleState::Edit)
			return hbox(
				text("SL@") | dim,
				text(format("{:#04x}", selected_address)),
				separatorLight(),
				text(vm->DecodeInstruction(selected_address).value_or("???"))
			);

		return hbox(
			vbox(
				hbox(
					text("IP@") | dim,
					text(format("{:#04x}", vm->IP()))
				),
				hbox(
					text("SL@") | dim,
					text(format("{:#04x}", selected_address))
				)
			),
			separatorLight(),
			vbox(
				text(vm->DecodeInstruction(vm->IP()).value_or("???")),
				text(vm->DecodeInstruction(selected_address).value_or("???"))
			)
		);
	}
};

export auto MemoryDetailsView(shared_ptr<PuzzleInstance> puzzle, shared_ptr<VM> vm,
	shared_ptr<HexEditorBase> hex_editor, MemoryDetailsViewOption option)
{
	option.puzzle = puzzle;
	option.vm = vm;
	option.hex_editor = hex_editor;
	return Make<MemoryDetailsViewBase>(move(option));
}

MemoryDetailsViewOption MemoryDetailsViewOption::Default()
{
	return MemoryDetailsViewOption();
}
