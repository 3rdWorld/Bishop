#include "Bishop.h"
#include "MemoryPool.h"
#include "DeviceTree.h"
#include <hmis/pnp/PnpBusDriver.h>
#include <3rdLib/util/Alignment.h>
#include <3rdLib/util/Pointer.h>
#include <3rdHAL/thri/ios/SystemCall.h>

#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/hmis/HardwareDescriptor.h>
#include <3rdUSL/System.h>


using namespace hmis;
using namespace thirdLib::util;
using namespace thirdLib::stdio;
using namespace thri::ios;
using namespace thirdUSL::thri;
using namespace thirdUSL::hmis;
using namespace thirdUSL;


/**
 *
 *
 *
 */
__attribute__((constructor))
static void init()
{
	stdOut.SetOutputStream(&System::Console::GetOutputStream());
	stdDbg.SetOutputStream(&System::Console::GetOutputStream());
	stdLog.SetOutputStream(&System::Console::GetOutputStream());
}

/**
 *
 *
 *
 */
static UBYTE CreateDeviceTreeRoot()
{
	if(DeviceTree::root)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidState;
	}
	
	if(!(DeviceTree::root
		= MemoryPool::RequestHardwareDescriptor()))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
	
	DeviceTree::root->deviceName
		= 'TOOR';
	
	DeviceTree::root->descriptorType
		= HardwareDescriptor::Type_Hardware;
	
	DeviceTree::root->flags.bits.Enumerable
		= true;
	
	return ReturnCode_Success;
}

/**
 *
 *
 *
 */
UBYTE Bishop::
shareMemoryPoolView(
	const ::thri::tmx::Program::Handle& programHandle)
{
	const ULONG largePageSize = (0x01ull << thirdUSL::mm::
		LinearAddressSpace::pagingCapability.bitShiftCount.LargePageSize);
	
	UBYTE returnCode;
	
	returnCode = _$$_ShareMemory(
		ThreadInfo::GetProgramHandle(), {
		Alignment::Truncate(MemoryPool::base, largePageSize),
		Alignment::RoundOff(MemoryPool::size, largePageSize)},
		programHandle,
		Alignment::Truncate(MemoryPool::base, largePageSize))
		.DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_WARNING_
	}
	
	return returnCode;
}

/**
 *
 *
 *
 */
UBYTE Bishop::
ServiceLoop()
{
	const ULONG messageLength = 64;
	UBYTE returnCode;
	UBYTE messageBuffer[messageLength];
	
	
	for(;;)
	{
		returnCode = _$$_WaitMessage(
			messageBuffer, messageLength).DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			break;
		}
	}
	
	
	return returnCode;
}


/**
 *
 *
 *
 */
UBYTE Bishop::
init()
{
	UBYTE returnCode;
	
	MemoryPool::Initialize();
	
	returnCode = CreateDeviceTreeRoot();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	
	::hmis::pnp::PnpBusDriver::Enumerate();
	
	DeviceTree::root->DumpEnumTree();
	
	
	return ReturnCode_Success;
}

/**
 *
 *
 *
 */
UBYTE Bishop::
fini()
{
	_TRACE_TODO_UNIMPLIMENTED_
	
	return ReturnCode_FunctionNotImplemented;
}

/**
 *
 *
 *
 */
UBYTE Bishop::
Main()
{
	UBYTE returnCode;
	
	returnCode = shareMemoryPoolView(
		{ThreadInfo::GetProgramHandle().uniqueID, 'ESIU'});
	
	returnCode = ServiceLoop();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	return returnCode;
}







extern "C" UBYTE _start()
{
	UBYTE returnCode;
	
	returnCode = Bishop::init();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	returnCode = Bishop::Main();
	
	Bishop::fini();
	
	return returnCode;
}
