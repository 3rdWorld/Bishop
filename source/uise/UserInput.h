#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include "WindowDescriptor.h"
#include "ClientDescriptor.h"

namespace uise
{
	struct UserInput
	{
		static const WindowDescriptor* focus;
		static UBYTE ServiceLoop();
	};
}

