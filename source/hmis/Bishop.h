#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdHAL/thri/tmx/Program.h>

namespace hmis
{
	struct Bishop
	{
		static UBYTE ServiceLoop();
		
		static UBYTE shareMemoryPoolView(
			const thri::tmx::Program::Handle&);
		
		static UBYTE init();
		static UBYTE fini();
		static UBYTE Main();
	};
}

