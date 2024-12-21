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

#include <SDL3/SDL_timer.h>