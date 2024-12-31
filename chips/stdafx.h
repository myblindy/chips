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

#include <variant>
#include <optional>

class not_implemented : public std::logic_error
{
public:
	not_implemented() : std::logic_error("Function not yet implemented") {}
};

using TNetworkIndex = uint8_t;
using TIndexInNetwork = uint8_t;

class BaseMemory;
struct PuzzleInstance;
enum class GlobalEventType
{
	VMDirty,
	VMInstructionExecuted,
	PuzzleSuccess,
	LoadNewPuzzle,
};
using TGlobalEventSource = std::optional<std::variant<BaseMemory*, PuzzleInstance*>>;
inline eventpp::EventQueue<GlobalEventType, void(TGlobalEventSource source)> GlobalEventQueue;