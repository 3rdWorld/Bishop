#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdUIA/hid/Display.h>
#include <3rdUIA/hid/Canvas.h>


namespace uise
{
	struct DisplayWriter
	{
		static thirdUIA::hid::Canvas doubleBuffer;
		static ULONG repaintRequestCount;
		static thirdUIA::hid::Display::Descriptor::List
			displayDescriptorList;
		
		static thirdUIA::hid::Window::Stack windowStack;
		
		static UBYTE AddDisplay(
			const thirdCGI::imaging::Surface&);
		
		static UBYTE UpdateLoop();
	};
}

