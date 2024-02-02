#include "UserInput.h"
#include "Pointer.h"

#include <3rdLib/util/Pointer.h>
#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/thri/HardwareIO.h>
#include <3rdUSL/thri/SystemIOSpace.h>
#include <3rdUSL/System.h>

#include <3rdHAL/thri/ios/SystemCall.h>

#include <3rdDev/device/input/keyboard/CharacterLayout.h>

#include <3rdDev/hawai/system/hid/ps2/Mouse.h>
#include <3rdDev/hawai/system/hid/ps2/Keyboard.h>

#include <3rdDev/driver/system/hid/ps2/Ps2HidBus.h>
#include <3rdDev/driver/system/hid/ps2/Ps2Mouse.h>
#include <3rdDev/driver/system/hid/ps2/Ps2Keyboard.h>

#include <3rdUIA/hid/KeyStroke.h>

using namespace uise;
using namespace thirdLib::util;
using namespace thirdUSL::thri;
using namespace thirdUSL;

using namespace thirdUIA::hid;
const WindowDescriptor* UserInput::focus = 0;

static UBYTE InitI8042portService()
{
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//Declare Variables
	//TODO:
	//Our code assumes the System has only One I8042port Controller
	//which is a bad design
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	#define _BUFFER_SIZE_		32
	using namespace hawai::system::hid::ps2;
	using namespace driver::system::hid::ps2;
	
	static UBYTE auxBuffer[_BUFFER_SIZE_];
	static UBYTE kbdBuffer[_BUFFER_SIZE_];
	
	
	//TODO: Get IO & GSI/IRQ Resource Info from _CRS
	
	static Ps2HidBus ps2HidBus(
		{
			.addressSpaceId = AddressSpace::ID_SystemIO,
			.bitWidth = UBYTE_BIT_WIDTH,
			.bitIndex = 0x00,
			.accessSize = GenericAddress::AccessSize_Byte,
			.address = I8042port::DefaultIoReg_IoData
		},
		
		{
			.addressSpaceId = AddressSpace::ID_SystemIO,
			.bitWidth = UBYTE_BIT_WIDTH,
			.bitIndex = 0x00,
			.accessSize = GenericAddress::AccessSize_Byte,
			.address = I8042port::DefaultIoReg_Status
		});
	//--------------------------------------------------------------------------
	
	
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//Interrupt Service Routine
	//TODO: Make the kernel provide a timestamp for each IRQ
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	void (*interruptHandler)(::thri::ios::Interrupt::Source)
		= [](::thri::ios::Interrupt::Source interruptSource)->void
	{
		//......................................................................
		//Helper Function
		//......................................................................
		auto getSystemTimeStamp = []()->UQUAD
		{
			_$$_GetSystemTime getSystemTime;
			
			if(getSystemTime.DispatchSystemCall() == ReturnCode_Success)
			{
				return getSystemTime.value;
			}else
			{
				_TRACE_CONDITION_FAILPATH_
				
				return -1uLL;
			}
		};
		//......................................................................
		
		
		
		
		//TODO: We could store a handle in InterruptSource so we can
		//quickly resolve the device from which the IRQ came from....
		//or have seperate IRQ handlers for each device:
		//The InterruptSource::bus could be used to store the bus-address
		
		
		if(ps2HidBus.deviceList.ForEach(
			[&](Ps2Device& device)->UBYTE
		{
			if(device.gsi != interruptSource.gsi)
			{
				return ReturnCode_Success;
			}
			
			
			if(!device.inputStream)
			{
				device.ioChannel.ReadData();
				
			}else
			if(device.deviceID <= 0x04)
			{
				Ps2Mouse::InputStream& inputStream
					= *(Ps2Mouse::InputStream*) device.inputStream;
				
				inputStream.Add(device.ioChannel.ReadData());
				
				if(inputStream.HasInputPacket())
				{
					auto inputPacket = inputStream.GetInputPacket();
					
					auto& mouseData = uise::Pointer::status.mouse.data;
					
					uise::Pointer::status.mouse.timeStamp
						= getSystemTimeStamp();
					
					mouseData.buttonState
						= inputPacket.buttonState;
					
					 mouseData.coordinates.x
						+= inputPacket.coordinates.x;
					
					 mouseData.coordinates.y
						-= inputPacket.coordinates.y;
					
					if( mouseData.coordinates.x < 0)
						 mouseData.coordinates.x = 0;
					
					if(mouseData.coordinates.y < 0)
						 mouseData.coordinates.y = 0;
					
					_$$_PostMessage(
						ThreadInfo::GetProgramHandle(),'PSID', 0, 0)
						.DispatchSystemCall();
				}
			}else
			if((device.deviceID & 0xFF) == 0xAB)
			{
				Ps2Keyboard::InputStream& inputStream
					= *(Ps2Keyboard::InputStream*) device.inputStream;
				
				inputStream.Add(device.ioChannel.ReadData());
				
				if(inputStream.HasInputPacket())
				{
					auto inputPacket = inputStream.GetInputPacket();
					
					if(UserInput::focus && !UserInput::focus->flags.bits.NonInterractive)
					{
						thirdUIA::hid::KeyStroke::Message::Keyboard
							keyboardMessage(getSystemTimeStamp(), inputPacket);
						
						_$$_PostMessage(
							UserInput::focus->clientDescriptor.programHandle,
							UserInput::focus->threadID,
							&keyboardMessage,
							keyboardMessage.size).DispatchSystemCall();
					}
				}
			}else
			{
				_TRACE_CONDITION_FAILPATH_
			}
			
			return ReturnCode_Break;
			
		}) != ReturnCode_Break){_TRACE_CONDITION_WARNING_}
		
		System::GetOutputStream().FlushOutputStreamBuffer();
		
		interruptSource.value = 0;
	};
	//--------------------------------------------------------------------------
	
	
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//Helper function
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	auto registerISR = [&](const Ps2Device& device)->UBYTE
	{
		if(!device.gsi)
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_InvalidArgument;
		}
		::thri::ios::Interrupt::Source s;
		::thri::ios::Interrupt::Target t;
		UBYTE returnCode;
		
		s.value = 0;
		
		s.irq = device.gsi;
		s.gsi = device.gsi;
		t.vector = (device.gsi + 0x20);
		t.cpu = -1ul;
		
		returnCode = _$$_AttachInterruptListener(
			s, t, (void*)interruptHandler).DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
		}
		
		return returnCode;
	};
	//--------------------------------------------------------------------------
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	UBYTE returnCode;
	
	//__________________________________________________________________________
	//
	//Put it all together
	//__________________________________________________________________________
	do
	{
		//TODO: This assumes all IO Registers of the PS2 are in
		//System IO Space, yet by using GADDR we intend to provide
		//generic AddressSpace access
		
		returnCode = thirdUSL::thri::HardwareIO::RequestOpRegion(
			AddressSpace::ID_SystemIO,
			{I8042port::DefaultIoReg_Status, sizeof(UBYTE)});
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		
		returnCode = thirdUSL::thri::HardwareIO::RequestOpRegion(
			AddressSpace::ID_SystemIO,
			{I8042port::DefaultIoReg_IoData, sizeof(UBYTE)});
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		
		returnCode = ps2HidBus.InitializePs2HidBus();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		returnCode = ps2HidBus.Enumerate();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		
		
		returnCode = ps2HidBus.deviceList.ForEach(
			[&](Ps2Device& device)->UBYTE
		{
			acpi::asmi::AddressSpace::Region buffer;
			
			const UBYTE address = device.GetDeviceAddress();		
			
			if(address == 0)
			{
				device.gsi = 0x01;
				
				buffer.SetRegion(
					thirdLib::util::Pointer::ToUnSignedInteger(kbdBuffer),
					_BUFFER_SIZE_);
			}else
			if(address == 1)
			{
				device.gsi = 0x0C;
				
				buffer.SetRegion(
					thirdLib::util::Pointer::ToUnSignedInteger(auxBuffer),
					_BUFFER_SIZE_);
			}else
			{
				buffer.SetRegion(0, 0);
			}
			
			
			
			
			
			if(device.deviceID <= 0x04)
			{
				device.inputStream = new Ps2Mouse::InputStream(
					buffer, device.deviceID);
				
				if(registerISR(device) == ReturnCode_Success)
				{
					//Let RestoreInterruptAssertion be the last
					
					device.ioChannel.RestoreIoChannel();
					BusTarget(device.ioChannel).RestoreFunction();
					device.ioChannel.RestoreInterruptAssertion();
				}
			}else
			if((device.deviceID & 0xFF) == 0xAB)
			{
				device.inputStream = new Ps2Keyboard::InputStream(
					buffer, 0x01);
				
				if(registerISR(device) == ReturnCode_Success)
				{
					device.ioChannel.RestoreIoChannel();
					device.ioChannel.RestoreInterruptAssertion();
				}
			}
			else
			{
				_TRACE_CONDITION_WARNING_
			}
			
			
			
			return ReturnCode_Success;
		});
		
	}while(0);
	
	System::GetOutputStream().FlushOutputStreamBuffer();
	
	
	return returnCode;;
}





UBYTE UserInput::
ServiceLoop()
{
	UBYTE returnCode;
	
	returnCode = InitI8042portService();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_WARNING_
	}
	
	for(;;)
	{
		_$$_WaitInterrupt().DispatchSystemCall();
	}
	
	return ReturnCode_Success;
}
