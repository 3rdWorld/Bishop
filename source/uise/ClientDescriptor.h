#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdLib/util/MacroLib.h>
#include <3rdSTL/SingleHardLinkedCircularList.h>
#include <3rdUIA/hid/Display.h>
#include <3rdHAL/thri/tmx/Program.h>

#include "CanvasDescriptor.h"
#include "WindowDescriptor.h"

namespace uise
{
	struct ClientDescriptor
	{
		struct List;
		
		thri::tmx::Program::Handle programHandle;
		thirdSTL::DoubleHardLinkedCircularList::Node clientListNode;
		CanvasDescriptor::List canvasDescriptorList;
		WindowDescriptor::List windowDescriptorList;
		acpi::asmi::SystemMemory::Mapping graphicsHeap;
		
		static UBYTE Sort(
			const ClientDescriptor&,
			const ClientDescriptor&);
		
		ClientDescriptor(const thri::tmx::Program::Handle& programHandle)
			: programHandle(programHandle)
			, clientListNode()
			, canvasDescriptorList()
			, windowDescriptorList()
			, graphicsHeap() {}
	};
	
	
	
	
	
	
	struct ClientDescriptor::List
		: thirdSTL::DoubleHardLinkedCircularList::Template<
		ClientDescriptor,
		thirdSTL::List::Ordering_SORT,
		ClientDescriptor::Sort,
		_MACRO_GetFieldOffset(ClientDescriptor, clientListNode)>
	{
		ClientDescriptor* GetClientDescriptor(
			const thri::tmx::Program::Handle&);
	};
}

