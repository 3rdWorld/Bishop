#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdLib/util/MacroLib.h>
#include <3rdSTL/SingleHardLinkedCircularList.h>
#include <3rdUIA/hid/Display.h>


namespace uise
{	
	struct ClientDescriptor;
	
	
	struct WindowDescriptor : thirdUIA::hid::Window::Context
	{
		struct List;
		
		typedef thirdUIA::hid::Display::Request::Parameters::
			AttachWindow::Flags Flags;
		
		const ClientDescriptor& clientDescriptor;
		const ULONG threadID;
		Flags flags;
		
		thirdSTL::SingleHardLinkedCircularList::Node windowListNode;
		
		WindowDescriptor(
			const Region& region,
			const ULONG windowIoHandle,
			const thirdUIA::hid::Canvas& canvas,
			const Region& fieldOfView,
			ULONG backgroundColor,
			const ClientDescriptor& clientDescriptor,
			ULONG threadID,
			const Flags& flags)
			
			: thirdUIA::hid::Window::Context(
				region, windowIoHandle, canvas, fieldOfView, backgroundColor)
			
			, clientDescriptor(clientDescriptor)
			, threadID(threadID)
			, flags(flags)
			, windowListNode(){}
	};
	
	
	
	
	struct WindowDescriptor::List
		: thirdSTL::SingleHardLinkedCircularList::Template<
		WindowDescriptor, thirdSTL::List::Ordering_FIFO, nullptr,
		_MACRO_GetFieldOffset(WindowDescriptor, windowListNode)>
	{
		WindowDescriptor* GetWindowDescriptor(ULONG windowIoHandle);
	};
}

