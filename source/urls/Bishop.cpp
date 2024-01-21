#include "Bishop.h"

#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdUSL/thri/ThreadInfo.h>

using namespace urls;
using namespace thri::ios;
using namespace thirdUSL::thri;

UBYTE Bishop::
ServiceLoop()
{
	constexpr ULONG messageLength = 0x40;
	
	UBYTE returnCode;
	UBYTE messageBuffer[messageLength];
	
	::thri::tmx::Program::Handle source;
	
	for(;;)
	{
		returnCode = _$$_WaitMessage(
			source, messageBuffer, messageLength)
			.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			break;
		}
	}
	
	return returnCode;
}

UBYTE Bishop::init(){return 0;}

UBYTE Bishop::fini(){return 0;}

UBYTE Bishop::
Main()
{
	ServiceLoop();
	
	return 0;
}

extern "C" void _start()
{
	Bishop::Main();
}
