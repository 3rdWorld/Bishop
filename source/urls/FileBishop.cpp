#include "FileBishop.h"

#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdUSL/thri/ThreadInfo.h>

using namespace urls;
using namespace thri::ios;
using namespace thirdUSL::thri;



const ULONG FileBishop::threadID = 'ELIF';


UBYTE FileBishop::
ServiceLoop()
{
	constexpr ULONG maximumMessageLength = 0x100;
	
	UBYTE returnCode;
	UBYTE messageBuffer[maximumMessageLength];
	
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
	}
	
	return returnCode;
}
