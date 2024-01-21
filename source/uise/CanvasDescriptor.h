#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdLib/util/MacroLib.h>
#include <3rdSTL/SingleHardLinkedCircularList.h>
#include <3rdUIA/hid/Display.h>


namespace uise
{
	struct CanvasDescriptor
	{
		struct List;
		
		
		thirdUIA::hid::Canvas canvas;
		ULONG canvasIoHandle;
		thirdSTL::SingleHardLinkedCircularList::Node canvasListNode;
		
		CanvasDescriptor(
			const thirdUIA::hid::Canvas& canvas,
			ULONG canvasIoHandle)
			
			: canvas(canvas)
			, canvasIoHandle(canvasIoHandle) {}
			
		static UBYTE Sort(const CanvasDescriptor&, const CanvasDescriptor&);
	};
	
	
	
	
	
	
	struct CanvasDescriptor::List
		: thirdSTL::SingleHardLinkedCircularList::Template<
		CanvasDescriptor, thirdSTL::List::Ordering_SORT, CanvasDescriptor::Sort,
		_MACRO_GetFieldOffset(CanvasDescriptor, canvasListNode)>
	{
		CanvasDescriptor* GetCanvasDescriptor(ULONG canvasIoHandle);
	};
}
