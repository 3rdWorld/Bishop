#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include "DisplayWriter.h"
#include "ClientDescriptor.h"


namespace uise
{
	struct Bishop
	{
		static ClientDescriptor::List clientDescriptorList;
		
		static UBYTE ServiceLoop();
		
		static ClientDescriptor* RegisterClient(
			::thri::tmx::Program::Handle&);
		
		static UBYTE Main(
			const thirdCGI::imaging::Surface& displaySurface);
		
		static UBYTE CreateThread(
			ULONG threadID,
			UBYTE priorityLevel,
			UBYTE(*ThreadMain)(void*),
			void* args,
			ULONG argSize);
	};
}

