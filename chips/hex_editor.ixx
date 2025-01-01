module;

#include "stdafx.h"

export module hex_editor;

import std.core;

using namespace std;
using namespace ftxui;

export struct HexEditorState
{
	Element element;
	bool focused;
};

export struct HexEditorOption
{
	static HexEditorOption Default();
	static HexEditorOption BytesPerLine(int bytes_per_line);

	function<Element(HexEditorState)> transform{};

	Ref<int> bytes_per_line = 16;

	Ref<int> cursor_half_byte_position{};
	Ref<span<uint8_t>> content{};
	Ref<function<optional<size_t>()>> ip{};
};

export class HexEditorBase : public ComponentBase, public HexEditorOption
{
public:
	HexEditorBase(HexEditorOption option)
		: HexEditorOption(move(option))
	{
	}

	Element Render() override final
	{
		const auto is_focused = Focused();
		const auto focused = !is_focused ? select : focusCursorUnderlineBlinking;

		*cursor_half_byte_position = clamp(*cursor_half_byte_position, 0, (int)content->size() * 2);
		auto cursor_line = *cursor_half_byte_position / (*bytes_per_line * 2);
		auto cursor_half_column = *cursor_half_byte_position % (*bytes_per_line * 2);

		auto ip_line = *ip ? (*ip)().value_or(-1) / *bytes_per_line : -1;
		auto ip_column = *ip ? (*ip)().value_or(0) % *bytes_per_line : -1;

		Elements elements;
		elements.reserve((size_t)ceil(content->size() * 2.f / *bytes_per_line) + 2);

		int line_length = 0;
		string line;

		// header
		for (auto i = 0; i < *bytes_per_line; ++i)
			line += format("{:02X} ", i);
		elements.push_back(text(line) | dim);
		elements.push_back(separator());

		// data
		line.clear();
		auto add_line = [&]
			{
				if (elements.size() - 2 != cursor_line && elements.size() - 2 != ip_line)
					elements.push_back(text(line));
				else if (elements.size() - 2 != ip_line)
					elements.push_back(hbox(
						text(line.substr(0, uint64_t(cursor_half_column / 2.0 * 3.0))),
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0), 1)) | focused,
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0 + 1)))
					));
				else if (elements.size() - 2 != cursor_line)
					elements.push_back(hbox(
						text(line.substr(0, ip_column * 3)),
						text(line.substr(ip_column * 3, 2)) | inverted,
						text(line.substr(ip_column * 3 + 2))
					));
				else if (cursor_half_column / 2 >= ip_column + 1)
					elements.push_back(hbox(
						text(line.substr(0, ip_column * 3)),
						text(line.substr(ip_column * 3, 2)) | inverted,
						text(line.substr(ip_column * 3 + 2, uint64_t(cursor_half_column / 2.0 * 3.0) - ip_column * 3 - 2)),
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0), 1)) | focused,
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0 + 1)))
					));
				else if (cursor_half_column / 2 < ip_column)
					elements.push_back(hbox(
						text(line.substr(0, uint64_t(cursor_half_column / 2.0 * 3.0))),
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0), 1)) | focused,
						text(line.substr(uint64_t(cursor_half_column / 2.0 * 3.0 + 1), ip_column * 3 - uint64_t(cursor_half_column / 2.0 * 3.0) - 1)),
						text(line.substr(ip_column * 3, 2)) | inverted,
						text(line.substr(ip_column * 3 + 2))
					));
				else
					elements.push_back(hbox(
						text(line.substr(0, ip_column * 3)),
						cursor_half_column == ip_column * 2 + 1
						? text(line.substr(ip_column * 3, 1)) | inverted
						: text(line.substr(ip_column * 3, 1)) | inverted | focused,
						cursor_half_column == ip_column * 2
						? text(line.substr(ip_column * 3 + 1, 1)) | inverted
						: text(line.substr(ip_column * 3 + 1, 1)) | inverted | focused,
						text(line.substr(ip_column * 3 + 2))
					));

				line.clear();
				line_length = 1;
			};

		for (auto&& uint8_t : *content)
		{
			if (line_length++ == *bytes_per_line)
				add_line();

			if (line.size() > 0)
				line += " ";
			line += format("{:02X}", (unsigned char)uint8_t);
		}

		if (line.size() > 0)
			add_line();

		// line numbers
		Elements line_number_elements;
		line_number_elements.reserve(elements.size());
		line_number_elements.push_back(text("  "));
		line_number_elements.push_back(separator());
		for (auto i = 2; i < (int)elements.size(); ++i)
			line_number_elements.push_back(text(format("{:02X}", (i - 2) * *bytes_per_line)) | dim);

		auto element = hbox({
			vbox(move(line_number_elements)) | yflex,
			separator(),
			vbox(move(elements)) | reflect(box_) | flex,
			}) | frame ;

		auto transform_func = transform ? transform : HexEditorOption::Default().transform;
		return transform_func({ move(element), is_focused });
	}

private:
	bool HandleArrowLeft()
	{
		if (*cursor_half_byte_position > 0)
		{
			--(*cursor_half_byte_position);
			return true;
		}
		return false;
	}

	bool HandleArrowRight()
	{
		if (*cursor_half_byte_position < (int)content->size() * 2 - 1)
		{
			++(*cursor_half_byte_position);
			return true;
		}
		return false;
	}

	bool HandleHome()
	{
		*cursor_half_byte_position = 0;
		return true;
	}

	bool HandleEnd()
	{
		*cursor_half_byte_position = (int)content->size() * 2 - 1;
		return true;
	}

	bool HandleArrowUp()
	{
		auto cursor_line = *cursor_half_byte_position / (*bytes_per_line * 2);
		if (cursor_line > 0)
			*cursor_half_byte_position -= *bytes_per_line * 2;
		else
			*cursor_half_byte_position = 0;
		return true;
	}

	bool HandleArrowDown()
	{
		auto cursor_line = *cursor_half_byte_position / (*bytes_per_line * 2);
		if (cursor_line < (int)content->size() / *bytes_per_line)
			*cursor_half_byte_position = min(*cursor_half_byte_position + *bytes_per_line * 2, (int)content->size() * 2 - 1);
		else
			*cursor_half_byte_position = (int)content->size() * 2 - 1;
		return true;
	}

	bool HandleMouse(Event event)
	{
		hovered_ = box_.Contain(event.mouse().x, event.mouse().y)
			&& CaptureMouse(event);

		if (!hovered_)
			return false;

		if (event.mouse().button == Mouse::Left)
		{
			TakeFocus();

			auto x = clamp(event.mouse().x - box_.x_min, 0, *bytes_per_line * 3 - 1);
			auto y = event.mouse().y - box_.y_min - 2;

			// last line?
			if (y >= content->size() / *bytes_per_line)
			{
				y = (int)(content->size() / *bytes_per_line);

				// are we beyond the last line's width?
				if (x >= (content->size() % *bytes_per_line) * 3)
					x = (content->size() % *bytes_per_line) * 3 - 1;
			}

			auto column = x % 3 == 0 ? x / 3 : x / 3 + 0.5f;

			auto new_cursor_half_byte_position = clamp((int)(y * *bytes_per_line * 2 + column * 2), 0, (int)content->size() * 2);
			if (new_cursor_half_byte_position != *cursor_half_byte_position)
			{
				*cursor_half_byte_position = new_cursor_half_byte_position;
				return true;
			}
		}

		return false;
	}

	bool HandleCharacter(const string& characters)
	{
		for (auto&& ch : characters)
		{
			if (!isxdigit(ch))
				continue;

			auto value = (uint8_t)(ch >= 'a' ? ch - 'a' + 10 : ch >= 'A' ? ch - 'A' + 10 : ch - '0');

			if (*cursor_half_byte_position % 2 == 0)
				(*content)[*cursor_half_byte_position / 2] = value << 4 | (*content)[*cursor_half_byte_position / 2] & (uint8_t)0x0F;
			else
				(*content)[*cursor_half_byte_position / 2] = (*content)[*cursor_half_byte_position / 2] & (uint8_t)0xF0 | value;
			HandleArrowRight();
		}

		return true;
	}

	bool OnEvent(Event event) override final
	{
		if (event == Event::ArrowLeft)
			return HandleArrowLeft();
		if (event == Event::ArrowRight)
			return HandleArrowRight();
		if (event == Event::Home)
			return HandleHome();
		if (event == Event::End)
			return HandleEnd();
		if (event == Event::ArrowUp)
			return HandleArrowUp();
		if (event == Event::ArrowDown)
			return HandleArrowDown();
		if (event.is_mouse())
			return HandleMouse(event);
		if (event.is_character())
			return HandleCharacter(event.character());
		return false;
	}

	bool Focusable() const override final { return true; }

	bool hovered_ = false;
	Box box_, cursor_box_;
};

export auto HexEditor(span<uint8_t> content, function<optional<size_t>()> ip, HexEditorOption option)
{
	option.content = move(content);
	option.ip = move(ip);
	return Make<HexEditorBase>(move(option));
}