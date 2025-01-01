#include "stdafx.h"

#include <windows.h>
#undef min
#undef max

using namespace std;
using namespace ftxui;

void ResizeConsoleWindow(int width, int height)
{
	CONSOLE_SCREEN_BUFFER_INFOEX consolesize{
		.cbSize = sizeof(consolesize),
	};

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfoEx(hConsole, &consolesize);

	COORD c{
		.X = 80,
		.Y = 40,
	};
	consolesize.dwSize = c;

	consolesize.srWindow.Left = 0;
	consolesize.srWindow.Right = width;
	consolesize.srWindow.Top = 0;
	consolesize.srWindow.Bottom = height;

	SetConsoleScreenBufferInfoEx(hConsole, &consolesize);
}

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
			else if (new_line && !hbox_elements.empty())
			{
				vbox_elements.push_back(hbox(hbox_elements));
				hbox_elements.clear();
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
