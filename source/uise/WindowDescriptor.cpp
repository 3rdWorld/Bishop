#include "WindowDescriptor.h"

using namespace uise;

WindowDescriptor* WindowDescriptor::List::
GetWindowDescriptor(ULONG windowIoHandle)
{
	WindowDescriptor* wd = 0;
	
	ForEach([&](WindowDescriptor& windowDescriptor)->UBYTE
	{
		if(windowDescriptor.windowIoHandle == windowIoHandle)
		{
			wd = &windowDescriptor;
			
			return ReturnCode_Break;
			
		}else
		{
			return ReturnCode_Success;
		}
	});
	
	return wd;
}

