#pragma once

#include <assert.h>

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };

#include "ftxui/component/captured_mouse.hpp"  
#include "ftxui/component/component.hpp"  
#include "ftxui/component/component_base.hpp"      
#include "ftxui/component/component_options.hpp"   
#include "ftxui/component/screen_interactive.hpp"  
#include "ftxui/dom/elements.hpp"  
#include "ftxui/screen/color.hpp"  
#include "ftxui/component/loop.hpp"       

#include <SDL3/SDL_timer.h>

#include "eventpp/eventqueue.h"

using TNetworkIndex = uint8_t;
using TIndexInNetwork = uint8_t;

enum class VMEventType
{
	Dirty,
	InstructionExecuted,
};
class VM;
inline eventpp::EventQueue<VMEventType, void(VM* vm)> VMEventQueue;

enum class PuzzleEventType 
{
	Success,
};
struct PuzzleInstance;
inline eventpp::EventQueue<PuzzleEventType, void(PuzzleInstance* puzzle)> PuzzleEventQueue;

enum class GlobalEventType 
{
	LoadNewPuzzle,
};
inline eventpp::EventQueue<GlobalEventType, void()> GlobalEventQueue;