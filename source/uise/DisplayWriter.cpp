#include "DisplayWriter.h"
#include "Pointer.h"
#include "WindowDescriptor.h"
#include "ClientDescriptor.h"
#include "Bishop.h"

#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdHAL/acpi/asmi/SystemMemory.h>

#include <3rdLib/util/Pointer.h>
#include <3rdLib/util/Alignment.h>
#include <3rdLib/util/MemOps.h>

#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/mm/LinearAddressSpace.h>
#include <3rdUSL/System.h>

#include <3rdCGI/glyphics/GlyphicsLibrary.h>
#include <3rdCGI/graphics/raster/RasterGraphicsLibrary.h>

#include <3rdUIA/UIRequest.h>

using namespace uise;
using namespace thirdLib::util;
using namespace thirdUSL::thri;
using namespace thirdUSL::mm;
using namespace thirdUSL;

using namespace thirdCGI::graphics::raster;
using namespace thirdCGI::glyphics;

using namespace thirdUIA::hid;
using namespace thirdUIA;

#define _SIZE_PostedMessageBuffer_	0x1000

constexpr static const UBYTE maxPacketSize = 0x40;

static UBYTE postedMessageBuffer[_SIZE_PostedMessageBuffer_];

thirdUIA::hid::Canvas DisplayWriter::
	doubleBuffer;

ULONG DisplayWriter::repaintRequestCount = 0x01;

thirdUIA::hid::Display::Descriptor::List
	DisplayWriter::displayDescriptorList;

thirdUIA::hid::Window::Stack
	DisplayWriter::windowStack;


struct PostedMessage
{
	::thri::ios::Program::Handle clientProgramHandle;
	ULONG clientThreadId;
	UIRequest uiRequest;
};


/**
 *
 *
 */
UBYTE DisplayWriter::
AddDisplay(const thirdCGI::imaging::Surface& hardSurface)
{
	acpi::asmi::SystemMemory::Mapping mapping;
	
	thirdUIA::hid::Display::Descriptor* displayDescriptor;
	UBYTE returnCode;
	
	if(hardSurface.IsGlyphicsSurface())
	{
		if(!(displayDescriptor = new thirdUIA::hid::Display::Descriptor()))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_Error;
		}
		
		displayDescriptor->canvas.glyphics = (
			thirdCGI::glyphics::GlyphicsSurface&) hardSurface;
		
	}else
	if(hardSurface.IsGraphicsSurface())
	{
		if(!(displayDescriptor = new thirdUIA::hid::Display::Descriptor()))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_Error;
		}
		
		displayDescriptor->canvas.raster = (
			thirdCGI::graphics::raster::RasterGraphicsSurface&)
			hardSurface;
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Error;
	}
	
	
	if(!(mapping.region.base = hardSurface.buffer))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Error;
	}
	
	
	
	if(!(mapping.region.size = Alignment::RoundOffPage(
		hardSurface.stride * hardSurface.dimensions.height)))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Error;
	}
	
	
	if(!(mapping.linearAddress = thirdLib::util::Pointer::ToUnSignedInteger(
		LinearAddressSpace::Allocator::graphicsHeap.RequestLinearAddressSpace(
		mapping.region.size))))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Error;
	}
	
	mapping.attribute = 0b1111;
	
	returnCode = _$$_CreateMemoryMapping(
		mapping, ThreadInfo::GetProgramHandle()).DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	displayDescriptor->canvas.surface.buffer
		= mapping.linearAddress;
	
	returnCode = DisplayWriter::displayDescriptorList.Insert(
		displayDescriptor);
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	if(doubleBuffer.surface.buffer)
	{
		return returnCode;
	}
	
	if(!(mapping.linearAddress = thirdLib::util::Pointer::ToUnSignedInteger(
		LinearAddressSpace::Allocator::graphicsHeap.RequestLinearAddressSpace(
		mapping.region.size))))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Error;
	}
	
	mapping.attribute = 0;
	
	returnCode = _$$_CreateZeroPageMapping(
		{mapping.linearAddress, mapping.region.size},
		ThreadInfo::GetProgramHandle()).DispatchSystemCall();
	
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	
	doubleBuffer = displayDescriptor->canvas;
	
	doubleBuffer.surface.buffer = mapping.linearAddress;
	
	return returnCode;
}







/**
 *
 *
 */
UBYTE DisplayWriter::
UpdateLoop()
{
	ULONG color = 0xFF000000;
	
	Coordinates lastPointerCoordinates = {-1, -1};
	
	UQUAD lastPointerTimeStamp = 0ull;
	
	thirdUIA::hid::Display::Descriptor* primaryDisplay;
	
	UBYTE returnCode;
	
	
	
	
	
	
	
	
	
	auto waitNextUIRepaintRequest = [&](void)->void
	{
		ClientDescriptor* clientDescriptor;
		
		
		
		
		
		for(;;)
		{
			_$$_NextMessage nextMessage;
			
			returnCode = nextMessage.DispatchSystemCall();
			
			if(returnCode != ReturnCode_Success)
			{
				_PANIC_CONDITION_BUGCHECK_
				return;
			}
			
			
			if(!(primaryDisplay = thirdUIA::hid::Display::
				Descriptor::List::Iterator(displayDescriptorList).Head()))
			{
				_TRACE_CONDITION_WAYPOINT_
				continue;
			}
			
			if(nextMessage.nextIndex >= (
				_SIZE_PostedMessageBuffer_ / maxPacketSize))
			{
				_TRACE_CONDITION_FAILPATH_
				
				return;
			}
			
			const PostedMessage& postedMessage = *(
				PostedMessage*) (postedMessageBuffer + (
				nextMessage.nextIndex * maxPacketSize));
			
			const Display::Request::Parameters::Repaint& params
				= *(Display::Request::Parameters::Repaint*)
				postedMessage.uiRequest.parameters;
			
			
			
			
			if(!(clientDescriptor = Bishop::clientDescriptorList
				.GetClientDescriptor(postedMessage.clientProgramHandle)))
			{
				//assume pointer repaint
				return;
				
			}else
			if((postedMessage.uiRequest.header.bits.UIRequestType
					!= UIRequest::Type_Display) || (
				postedMessage.uiRequest.header.bits.UIRequestCode
					!= Display::Request::Code_Repaint))
			{
				continue;
				
			}else
			if(postedMessage.uiRequest.header.bits.UIRequestSize
				< sizeof(Display::Request::Template<
					Display::Request::Code_Repaint,
					Display::Request::Parameters::Repaint>))
			{
				repaintRequestCount++;
				
				return;
				
			}else
			if(WindowDescriptor* wd = clientDescriptor->windowDescriptorList
				.GetWindowDescriptor(params.windowIoHandle))
			{
				//Minus 1 pixel(s) width each(for the right and left borders)
				const SLONG lengthContentArea = (
					wd->dimensions.length - 2);
				
				//Minus 1 pixel(s) width each(for the top and bottom borders)
				//Minus 8 pixel(s) width each(for the header and footer)
				const SLONG heightContentArea = (
					wd->dimensions.height - 18);	
				
				Region regionToRepaint = {};
				
				
				
				if((lengthContentArea <= 0) || (heightContentArea <= 0))
				{
					_TRACE_CONDITION_WAYPOINT_
					continue;
				}
				
				
				regionToRepaint.dimensions = params.windowRegion.dimensions;
				
				
				
				if(params.windowRegion.dimensions.length < 0)
				{
					regionToRepaint.dimensions.length = lengthContentArea;
					
					regionToRepaint.coordinates.x = 0;
					
				}else
				{
					if(params.windowRegion.coordinates.x >= 0)
					{
						regionToRepaint.coordinates.x
							= params.windowRegion.coordinates.x;
					}else
					if(_MACRO_ABS(params.windowRegion.coordinates.x)
						> wd->dimensions.length)
					{
						regionToRepaint.coordinates.x = 0;
						
					}else
					{
						regionToRepaint.coordinates.x = (
							params.windowRegion.coordinates.x +
							wd->dimensions.length);
					}
				}
				
				
				
				
				
				
				if(params.windowRegion.dimensions.height < 0)
				{
					regionToRepaint.dimensions.height = heightContentArea;
					
					regionToRepaint.coordinates.y = 0;
					
				}else
				{
					if(params.windowRegion.coordinates.y >= 0)
					{
						regionToRepaint.coordinates.y
							= params.windowRegion.coordinates.y;
						
					}else
					if(_MACRO_ABS(params.windowRegion.coordinates.y)
						> wd->dimensions.height)
					{
						regionToRepaint.coordinates.y = 0;
						
					}else
					{
						regionToRepaint.coordinates.y = (
							params.windowRegion.coordinates.y +
							wd->dimensions.height);
					}
				}
				
				
				
				
				
				
				if(regionToRepaint.Intersects(
						{{0, 0}, {lengthContentArea, heightContentArea}}))
				{
					Region intersection;
					
					regionToRepaint.GetIntersection(
						{{0, 0}, {lengthContentArea, heightContentArea}},
						intersection);
					
					regionToRepaint = intersection;
					
					regionToRepaint.coordinates.x += (1 + wd->coordinates.x);
					regionToRepaint.coordinates.y += (9 + wd->coordinates.y);
				}else
				{
					_TRACE_CONDITION_WAYPOINT_
					continue;
				}
				
				
				
				
				thirdUIA::hid::Display::Writer(
					doubleBuffer.surface, windowStack)
					.InvalidateRegion(regionToRepaint, wd);
				
				
				
				
				
				
				if(regionToRepaint.Intersects({
					lastPointerCoordinates,
					Pointer::damagedSurface.dimensions}))
				{
					Region intersection;
					
					regionToRepaint.GetIntersection({
						lastPointerCoordinates,
						Pointer::damagedSurface.dimensions},
						intersection);
					
					
					
					RasterGraphicsLibrary(Pointer::damagedSurface).BlitSurface(
						doubleBuffer.raster,
						intersection.coordinates,
						{
							(intersection.coordinates.x
								- lastPointerCoordinates.x),
							
							(intersection.coordinates.y
								- lastPointerCoordinates.y),
						},
						
						intersection.dimensions);
					
					
					
					RasterGraphicsLibrary(doubleBuffer.raster).BlitSurface(
						Pointer::defaultPointer,
						{
							(intersection.coordinates.x
								- lastPointerCoordinates.x),
							
							(intersection.coordinates.y
								- lastPointerCoordinates.y),
						},
						intersection.coordinates,
						intersection.dimensions,
						RasterOperation_AlphaBlend);
				}
				
				
				
				
				
				
				
				
				
				
				RasterGraphicsLibrary(primaryDisplay->canvas.raster)
					.BlitSurface(
						doubleBuffer.raster,
						regionToRepaint.coordinates,
						regionToRepaint.coordinates,
						doubleBuffer.surface.dimensions);
				
			}else
			{
				continue;
			}
		}
	};

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	thirdUIA::hid::Display::Writer::memoryPool.size
		= 0x4000;
	
	
	if(!(thirdUIA::hid::Display::Writer::memoryPool.base
		= thirdLib::util::Pointer::ToUnSignedInteger(
		LinearAddressSpace::Allocator::graphicsHeap.RequestLinearAddressSpace(
		thirdUIA::hid::Display::Writer::memoryPool.size))))
	{
		_PANIC_CONDITION_BUGCHECK_
	}
	
	returnCode =_$$_CreateZeroPageMapping(
		thirdUIA::hid::Display::Writer::memoryPool,
		ThreadInfo::GetProgramHandle()).DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_PANIC_CONDITION_BUGCHECK_
		
	}
	
	
	
	
	
	{
		_$$_InitMessageQueue::Flags flags{};
		
		flags.bits.InjectSourceAddress = 1;
		
		returnCode =  _$$_InitMessageQueue(
			postedMessageBuffer,
			_SIZE_PostedMessageBuffer_,
			maxPacketSize,
			flags.value).DispatchSystemCall();
		
		
		if(returnCode != ReturnCode_Success)
		{
			_PANIC_CONDITION_BUGCHECK_
		}
	}
	
	
	
	
	do
	{
		const ULONG repaintRequests = repaintRequestCount;
		
		const auto pointerDamagedCoordinates
			= lastPointerCoordinates;
		
		auto pointerData = Pointer::status.mouse;
		
		auto& currentPointerCoordinates
			= pointerData.data.coordinates;
		
		LOGIC pointerHasMoved = false;
		
		
		
		
		

		if(!(primaryDisplay = thirdUIA::hid::Display::
			Descriptor::List::Iterator(displayDescriptorList).Head()))
		{
			_TRACE_CONDITION_WAYPOINT_
			
		}else
		if(!repaintRequests)
		{
		}else
		if(thirdUIA::hid::Window::Stack::Iterator(windowStack).Next())
		{
			thirdUIA::hid::Display::Writer(doubleBuffer.surface, windowStack)
				.InvalidateDisplay();
			
		}else
		if(primaryDisplay->canvas.surface.IsGlyphicsSurface())
		{
			GlyphicsLibrary(primaryDisplay->canvas.glyphics).SolidFill(
				{0, 0}, primaryDisplay->canvas.surface.dimensions, color++);
			
		}else
		if(primaryDisplay->canvas.surface.IsGraphicsSurface())
		{
			RasterGraphicsLibrary(primaryDisplay->canvas.raster).FillColor(
				{0, 0}, primaryDisplay->canvas.surface.dimensions, color++);
			
		}else
		{
			_TRACE_CONDITION_WAYPOINT_
		}
		
		__sync_sub_and_fetch(&repaintRequestCount, repaintRequests);
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		do
		{
			if(primaryDisplay->canvas.raster.dimensions.length
				< currentPointerCoordinates.x)
			{
				Pointer::status.mouse.data.coordinates.x
					= primaryDisplay->canvas.raster.dimensions.length;
				
				currentPointerCoordinates.x
					= primaryDisplay->canvas.raster.dimensions.length;
			}
			
			if(primaryDisplay->canvas.raster.dimensions.height
				< currentPointerCoordinates.y)
			{
				Pointer::status.mouse.data.coordinates.y
					= primaryDisplay->canvas.raster.dimensions.height;
				
				currentPointerCoordinates.y
					= primaryDisplay->canvas.raster.dimensions.height;
			}
			
			
			pointerHasMoved = ((
				lastPointerCoordinates.x != currentPointerCoordinates.x) || (
				lastPointerCoordinates.y != currentPointerCoordinates.y));
			
			
			
			if(!repaintRequests && !pointerHasMoved)
			{
				break;
			}
			
			
			if(!repaintRequests && pointerHasMoved &&
				(lastPointerCoordinates.x >= 0) &&
				(lastPointerCoordinates.y >= 0))
			{
				RasterGraphicsLibrary(doubleBuffer.raster).BlitSurface(
					Pointer::damagedSurface, {0, 0}, {
					pointerDamagedCoordinates.x, pointerDamagedCoordinates.y},
					Pointer::blendedSurface.dimensions);
			}
			
			
			if((primaryDisplay->canvas.raster.dimensions.height
					== currentPointerCoordinates.y) || (
				primaryDisplay->canvas.raster.dimensions.length
					== currentPointerCoordinates.x))
			{
				lastPointerCoordinates = {
					currentPointerCoordinates.x,
					currentPointerCoordinates.y};
				
				break;
			}
			
			

			
			RasterGraphicsLibrary(Pointer::damagedSurface).BlitSurface(
				doubleBuffer.raster, {
				currentPointerCoordinates.x, currentPointerCoordinates.y},
				{0, 0}, Pointer::damagedSurface.dimensions);
			
			RasterGraphicsLibrary(doubleBuffer.raster).BlitSurface(
				Pointer::defaultPointer, {0, 0}, {
				currentPointerCoordinates.x, currentPointerCoordinates.y},
				Pointer::defaultPointer.dimensions,
				RasterOperation_AlphaBlend);
			
			lastPointerCoordinates = {
				currentPointerCoordinates.x, currentPointerCoordinates.y};
			
			
			
		}while(0);
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		
		
		
		if(repaintRequests)
		{
			RasterGraphicsLibrary(primaryDisplay->canvas.raster).BlitSurface(
				doubleBuffer.raster, {0, 0}, {0, 0},
				doubleBuffer.surface.dimensions);
			
		}else
		if(pointerHasMoved)
		{
			SLONG x;
			SLONG y;
			SLONG l;
			SLONG h;
			
			
			if(pointerDamagedCoordinates.x < currentPointerCoordinates.x)
			{
				x = pointerDamagedCoordinates.x;
				
				l = Pointer::defaultPointer.dimensions.length + (
					currentPointerCoordinates.x - pointerDamagedCoordinates.x);
			}else
			{
				x = currentPointerCoordinates.x;
				
				l = Pointer::defaultPointer.dimensions.length + (
					pointerDamagedCoordinates.x - currentPointerCoordinates.x);
			}
			
			
			
			if(pointerDamagedCoordinates.y < currentPointerCoordinates.y)
			{
				y = pointerDamagedCoordinates.y;
				
				h = Pointer::defaultPointer.dimensions.height + (
					currentPointerCoordinates.y - pointerDamagedCoordinates.y);
			}else
			{
				y = currentPointerCoordinates.y;
				
				h = Pointer::defaultPointer.dimensions.height + (
					pointerDamagedCoordinates.y - currentPointerCoordinates.y);
			}
			
			RasterGraphicsLibrary(primaryDisplay->canvas.raster).BlitSurface(
				doubleBuffer.raster, {x, y}, {x, y}, {l, h});
		
			
			
			if((_MACRO_MAX(pointerData.timeStamp, lastPointerTimeStamp) -
				_MACRO_MIN(pointerData.timeStamp, lastPointerTimeStamp))
				< 250'000'000)
			{
				//Don't overwhelm IPC with mouse motion data traffic
				//This limits mouse IPC traffic to atmost 4 Hz
				//TODO:... 
				
			}else
			if(WindowDescriptor* windowDescriptor = (WindowDescriptor*)
				windowStack.GetWindowContext({
				currentPointerCoordinates.x, currentPointerCoordinates.y}))
			{
				if(!windowDescriptor->flags.bits.NonInterractive)
				{
					thirdUIA::hid::Pointer::Message::Mouse mouseMessage;
					
					mouseMessage.pointerShape = 0;
					
					mouseMessage.timeStamp = pointerData.timeStamp;
					
					mouseMessage.data = pointerData.data;
					//1 pixel(s) for border thickness
					//8 pixel(s) for header thickness
					//TODO:
					//Make these params configurable per window
					
					mouseMessage.data.coordinates.x
						-= (windowDescriptor->coordinates.x + 1);
					
					mouseMessage.data.coordinates.y
						-= (windowDescriptor->coordinates.y + 1 + 8);
					
					
					
					
					
					_$$_PostMessage(
						windowDescriptor->clientDescriptor.programHandle,
						windowDescriptor->threadID,
						&mouseMessage,
						mouseMessage.size).DispatchSystemCall();
					
					lastPointerTimeStamp = pointerData.timeStamp;
				}
			}
		}

		
		waitNextUIRepaintRequest();
		
		
		
		
	}while(true);	
	
	
	return ReturnCode_Success;
}



