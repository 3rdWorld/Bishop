#include "Bishop.h"
#include "UserInput.h"

#include <3rdLib/util/Pointer.h>
#include <3rdLib/util/Alignment.h>
#include <3rdLib/util/MacroLib.h>

#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdHAL/acpi/asmi/AddressSpace.h>

#include <3rdUSL/mm/ProgramHeap.h>
#include <3rdUSL/mm/LinearAddressSpace.h>
#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/thri/SystemIOSpace.h>
#include <3rdUSL/hmis/DeviceDescriptor.h>
#include <3rdUSL/System.h>

#include <3rdART/BootStrap.h>




using namespace uise;
using namespace thirdLib::util;
using namespace thirdLib::stdio;
using namespace thirdUSL::mm;
using namespace thirdUSL::thri;
using namespace thirdUSL::hmis;
using namespace thirdUSL;
using namespace thirdUIA::hid;
using namespace thirdUIA;
using namespace thirdART;


ClientDescriptor::List Bishop::clientDescriptorList;


typedef UBYTE (*UIRequestServiceRoutine)(
	ClientDescriptor&, ULONG threadID, const UIRequest&);


static UIRequestServiceRoutine uiRequestServiceRoutine[0x10][0x10] = {};
/**
 *
 *
 *
 */
__attribute__((constructor))
static void init()
{
	{
		auto outputStream = &System::Console::GetOutputStream();
			
		stdOut.SetOutputStream(outputStream);
		stdDbg.SetOutputStream(outputStream);
		stdLog.SetOutputStream(outputStream);
	}
	
	
	acpi::asmi::AddressSpace::RegisterGlobalAddressSpaceInterface(
		acpi::asmi::SystemMemory::AddressSpaceInterface::instance);
	
	acpi::asmi::AddressSpace::RegisterGlobalAddressSpaceInterface(
		SystemIOSpace::instance);
}



//TODO: This is Ugly
static UBYTE handleUIRequestInsertTile(
	ClientDescriptor& clientDescriptor,
	ULONG threadID,
	const UIRequest& uiRequest)
{
	Display::Request::Parameters::InsertTile& params
		= *(Display::Request::Parameters::InsertTile*)
		uiRequest.parameters;
	
	if(WindowDescriptor* wd = clientDescriptor.windowDescriptorList
		.GetWindowDescriptor(params.windowIoHandle))
	{
		return wd->tileList.Insert(new Window::Tile(
			params.windowRegion, params.tileIoHandle,
			params.fieldOfView, params.backgroundColor));
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
}


//TODO: This is Ugly
static UBYTE handleUIRequestDeclareCanvas(
	ClientDescriptor& clientDescriptor,
	ULONG threadID,
	const UIRequest& uiRequest)
{
	Display::Request::Parameters::DeclareCanvas& params
		= *(Display::Request::Parameters::DeclareCanvas*)
		uiRequest.parameters;
	
	CanvasDescriptor* canvasDescriptor;
	
	
	UBYTE returnCode;
	
	
	if((canvasDescriptor = clientDescriptor.canvasDescriptorList
		.GetCanvasDescriptor(params.canvasIoHandle)))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidState;
	}
	
	
	
	if(!clientDescriptor.graphicsHeap.linearAddress)
	{
		if(!LinearAddressSpace::largeHeap.GetAlignment())
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_InvalidState;
		}
		
		
		
		clientDescriptor.graphicsHeap.region.base = Alignment::Truncate(
			params.canvas.surface.buffer,
			LinearAddressSpace::largeHeap.GetAlignment());
		
		
		clientDescriptor.graphicsHeap.region.size = (Alignment::RoundOff(
			acpi::asmi::AddressSpace::Region(params.canvas.surface.buffer, (
				params.canvas.surface.stride *
				params.canvas.surface.dimensions.height)).GetUpperBound(),
			
			LinearAddressSpace::largeHeap.GetAlignment())
			
			- clientDescriptor.graphicsHeap.region.base);
		
		
		if(!(clientDescriptor.graphicsHeap.linearAddress
			= Pointer::ToUnSignedInteger(LinearAddressSpace::largeHeap
			.RequestLinearAddressSpace(
			clientDescriptor.graphicsHeap.region.size))))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_Error;
		}
		
		returnCode = _$$_ShareMemory(
			clientDescriptor.programHandle,
			clientDescriptor.graphicsHeap.region,
			ThreadInfo::GetProgramHandle(),
			clientDescriptor.graphicsHeap.linearAddress)
			.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_Failure;
		}
	}else
	{
		if(!clientDescriptor.graphicsHeap.region.Contains({
			params.canvas.surface.buffer, (
			params.canvas.surface.stride *
			params.canvas.surface.dimensions.height)}))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_InvalidState;
		}
	}
	
	
	
	if(!(canvasDescriptor = new CanvasDescriptor(
		params.canvas, params.canvasIoHandle)))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidArgument;
		
	}
	
	canvasDescriptor->canvas.surface.buffer = ((
		params.canvas.surface.buffer & (
		LinearAddressSpace::largeHeap.GetAlignment() - 1))
		| clientDescriptor.graphicsHeap.linearAddress);
	
	
	returnCode = clientDescriptor.canvasDescriptorList.Insert(
		canvasDescriptor);
	
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	return ReturnCode_Success;	
}




//TODO: This is Ugly
static UBYTE handleUIRequestAttachWindow(
	ClientDescriptor& clientDescriptor,
	ULONG threadID,
	const UIRequest& uiRequest)
{
	CanvasDescriptor* canvasDescriptor;
	WindowDescriptor* windowDescriptor;
	Coordinates coordinates;
	
	Display::Request::Parameters::AttachWindow& params
		= *(Display::Request::Parameters::AttachWindow*)
		uiRequest.parameters;
	
	UBYTE returnCode;

	
	if((windowDescriptor = clientDescriptor.windowDescriptorList
		.GetWindowDescriptor(params.windowIoHandle)))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
	
	
	if(!(canvasDescriptor = clientDescriptor.canvasDescriptorList
		.GetCanvasDescriptor(params.canvasIoHandle)))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
	
	if(const thirdUIA::hid::Display::Display::Descriptor* d
		= thirdUIA::hid::Display::Descriptor::List::Iterator(
		DisplayWriter::displayDescriptorList).Head())
	{
		coordinates = {
			(_MACRO_ABS(d->canvas.surface.dimensions.length
				- params.dimensions.length) >> 1),
			(_MACRO_ABS(d->canvas.surface.dimensions.height
				- params.dimensions.height) >> 1)
		};
	}else
	{
		coordinates = {0, 0};
	}
	
	
	
		
	if(!(windowDescriptor = new WindowDescriptor(
			{coordinates, params.dimensions},
			params.windowIoHandle,
			canvasDescriptor->canvas,
			params.fieldOfView,
			params.backgroundColor,
			clientDescriptor,
			threadID,
			{.value = UBYTE(uiRequest.header.bits.UIRequestFlags)})))
		
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
	
	
	
	returnCode = clientDescriptor.windowDescriptorList.Insert(
		windowDescriptor);
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	
	returnCode = DisplayWriter::windowStack.Insert(windowDescriptor);
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
		
	}else
	{
		UserInput::focus = windowDescriptor;
		__sync_add_and_fetch(&DisplayWriter::repaintRequestCount, 1);
		return returnCode;
	}
}



//TODO: This is Ugly
static UBYTE handleUIRequestRepaint(
	ClientDescriptor& clientDescriptor,
	ULONG threadID,
	const UIRequest& uiRequest)
{
	__sync_add_and_fetch(&DisplayWriter::repaintRequestCount, 1);
	
	
	{
		_$$_PostMessage(ThreadInfo::GetProgramHandle(), 'PSID', 0, 0)
			.DispatchSystemCall();
	}
	
	return ReturnCode_Success;
}




ClientDescriptor* Bishop::
RegisterClient(::thri::tmx::Program::Handle& programHandle)
{
	ClientDescriptor* clientDescriptor;
	
	
	if((clientDescriptor = clientDescriptorList.GetClientDescriptor(
		programHandle)))
	{
		return clientDescriptor;
		
	}else
	if((clientDescriptor = new ClientDescriptor(programHandle)))
	{
		if(Bishop::clientDescriptorList.Insert(clientDescriptor)
			!= ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			delete clientDescriptor;
			
			return 0;
		}else
		{
			return clientDescriptor;
		}
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return 0;
		
	}
}



/**
 *
 *
 *
 */
UBYTE Bishop::
ServiceLoop()
{
	constexpr const ULONG messageLength = 0x80;
	
	UBYTE messageBuffer[messageLength];
	
	const UIRequest& uiRequest = *(UIRequest*) messageBuffer;
	
	ClientDescriptor* clientDescriptor;
	
	
	
	for(;;)
	{
		UBYTE returnCode;
		
		_$$_WaitMessage waitMessage(messageBuffer, messageLength);
		
		returnCode = waitMessage.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_WARNING_
			
			continue;
		}
		
		
		if(!(clientDescriptor = clientDescriptorList.GetClientDescriptor(
			waitMessage.programHandle)))
		{
			//FIX Me!!!!!
			//We should probably fail for UIRequests from unregistered
			//parties instead of auto registering them.
			if(!(clientDescriptor = RegisterClient(waitMessage.programHandle)))
			{
				_TRACE_CONDITION_FAILPATH_
				
				continue;
			}
		}
		
		
		if(UIRequestServiceRoutine serviceRoutine
			= uiRequestServiceRoutine
			[uiRequest.header.bits.UIRequestType]
			[uiRequest.header.bits.UIRequestCode])
		{
			returnCode = serviceRoutine(
				*clientDescriptor, waitMessage.threadID, uiRequest);
			
			if(returnCode != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
			}
		}else
		{
			_TRACE_CONDITION_WARNING_
		}
	}
	
	return ReturnCode_Success;
}














UBYTE Bishop::
CreateThread(ULONG threadID, UBYTE priority, UBYTE(*entry)(void*), void* args,
	ULONG argSize)
{
	UBYTE returnCode;
	
	ULONG(*prologue)() = []()->ULONG
	{
		UBYTE returnCode;
		
		returnCode = BootStrap::InstantiateThreadLocalStorage();
		
		if(returnCode == ReturnCode_Success)
		{
			auto outputStream = &System::Console::GetOutputStream();
				
			stdOut.SetOutputStream(outputStream);
			stdDbg.SetOutputStream(outputStream);
			stdLog.SetOutputStream(outputStream);
		}
		
		return returnCode;
	};
	
	returnCode = _$$_CreateNewThread(
		ThreadInfo::GetProgramHandle(),
		threadID,
		Pointer::ToUnSignedInteger((const void*) entry),
		args, argSize, priority, false,
		Pointer::ToUnSignedInteger((void*) prologue))
		.DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	return returnCode;
}






void populateUIServiceRoutines()
{
	{
		auto displayRoutines = uiRequestServiceRoutine[
			UIRequest::Type_Display];
			
		displayRoutines[Display::Request::Code_DeclareCanvas]
			= handleUIRequestDeclareCanvas;
			
		displayRoutines[Display::Request::Code_AttachWindow]
			= handleUIRequestAttachWindow;
		
		displayRoutines[Display::Request::Code_Repaint]
			= handleUIRequestRepaint;
		
		displayRoutines[Display::Request::Code_InsertTile]
			= handleUIRequestInsertTile;
	};
}





UBYTE Bishop::
Main(const thirdCGI::imaging::Surface& displaySurface)
{
	populateUIServiceRoutines();
	
	
	if(!displaySurface.buffer)
	{
		_TRACE_CONDITION_FAILPATH_
		
	}else
	if(DisplayWriter::AddDisplay(displaySurface)
		!= ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
	}else
	if(CreateThread('PSID', 0x00, (UBYTE(*)(void*)) &DisplayWriter::UpdateLoop, 0, 0)
		!= ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
	}else
	if(CreateThread('DNIU', 0x0F, (UBYTE(*)(void*)) &UserInput::ServiceLoop, 0, 0)
		!= ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
	}
	
	
	return Bishop::ServiceLoop();
}





/**
 *
 *
 *
 */
extern "C" UBYTE _start(int argc, char** argv)
{
	return Bishop::Main(*(thirdCGI::imaging::Surface*) argv);
}
