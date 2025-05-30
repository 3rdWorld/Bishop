#include "ArchBishop.h"

#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/System.h>
#include <3rdUSL/ns/NameSpace.h>


using namespace urls;
using namespace thri::ios;
using namespace thirdLib::stdio;
using namespace thirdUSL;
using namespace thirdUSL::ns;
using namespace thirdUSL::thri;



__attribute__((constructor))
static void init()
{
	if(auto outputStream = &System::Console::GetOutputStream())
	{	
		stdOut.SetOutputStream(outputStream);
		stdDbg.SetOutputStream(outputStream);
		stdLog.SetOutputStream(outputStream);
	}
}



UBYTE ArchBishop::
ServiceLoop()
{
	
	constexpr ULONG maximumMessageLength = 0x1000;
	
	static UBYTE messageBuffer[maximumMessageLength]
		__attribute__((aligned(0x1000))) = {};
	
	UBYTE returnCode;
	
	for(;;)
	{
		_$$_WaitMessage waitMessage(
			messageBuffer, maximumMessageLength);
		
		returnCode = waitMessage.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_WARNING_
			
			continue;
		}
		NameSpace::IoRequest::Header header = {.value = waitMessage.subject};
		
		if(header.bits.IoRequestCode == NameSpace::IoRequest::Code_Open)
		{
			LOG("IoRequest.Open(.Path:\"%.*s\", .Flags:%x)",
				waitMessage.size, messageBuffer, header.bits.IoRequestFlags);
		}else
		{
			LOG("IoRequest[IoRequestCode:%x, IoRequestFlags:%x",
				header.bits.IoRequestCode, header.bits.IoRequestFlags);
		}
		
		System::GetOutputStream().FlushOutputStreamBuffer();
	}
	
	return returnCode;
}

UBYTE ArchBishop::init(){return 0;}

UBYTE ArchBishop::fini(){return 0;}

UBYTE ArchBishop::
Main()
{
	ServiceLoop();
	
	return 0;
}

extern "C" void _start()
{
	ArchBishop::Main();
}
