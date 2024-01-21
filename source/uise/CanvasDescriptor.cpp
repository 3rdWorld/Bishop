#include "CanvasDescriptor.h"

using namespace uise;


UBYTE CanvasDescriptor::
Sort(const CanvasDescriptor& lhs, const CanvasDescriptor& rhs)
{
	if(lhs.canvasIoHandle > rhs.canvasIoHandle)
	{
		return 0x7F;
		
	}else
	if(lhs.canvasIoHandle < rhs.canvasIoHandle)
	{
		return 0x80;
		
	}else
	{
		return 0;
	}
}

CanvasDescriptor* CanvasDescriptor::List::
GetCanvasDescriptor(ULONG canvasIoHandle)
{
	CanvasDescriptor* c = 0;
	
	ForEach([&](CanvasDescriptor& canvas)->UBYTE
	{
		if(canvas.canvasIoHandle > canvasIoHandle)
		{
			return ReturnCode_Break;
			
		}else
		if(canvas.canvasIoHandle < canvasIoHandle)
		{
			return ReturnCode_Success;
			
		}else
		{
			c = &canvas;
			
			return ReturnCode_Break;
		}
	});
	
	return c;
}

