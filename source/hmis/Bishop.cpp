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
	constexpr ULONG messageLength = 64;
	UBYTE messageBuffer[messageLength];
	UBYTE returnCode;
	
	
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
	
	::hmis::pnp::PnpBusDriver::Enumerate();

	DeviceTree::GetRoot().DumpEnumTree();

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
	
	// returnCode = shareMemoryPoolView(
	// 	{ThreadInfo::GetProgramHandle().uniqueID, 'ESIU'});
	
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
	
	{
		const auto& threadHandle = ThreadInfo::GetThreadHandle();

		LOG("Thread[%g:%.4s.%.4s] Hello-World..!",
			&threadHandle.uniqueID,
			&threadHandle.callSign,
			&threadHandle.threadID);
	}
	

	returnCode = Bishop::init();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	returnCode = Bishop::Main();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}   
	
	Bishop::fini();
	
	return returnCode;
}
