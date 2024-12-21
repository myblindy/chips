#include "stdafx.h"

import hex_editor;

using namespace ftxui;

HexEditorOption HexEditorOption::Default()
{
	HexEditorOption option;
	option.transform = [](HexEditorState state) -> Element
		{
			state.element |= color(Color::White);

			return state.element;
		};
	return option;
}

HexEditorOption HexEditorOption::BytesPerLine(int bytes_per_line)
{
	HexEditorOption option = Default();
	option.bytes_per_line = bytes_per_line;
	return option;
}