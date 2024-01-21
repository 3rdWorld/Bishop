#include "ClientDescriptor.h"

using namespace uise;


static UBYTE Compare(
	const thri::tmx::Program::Handle& thisHandle,
	const thri::tmx::Program::Handle& thatHandle)
{
	if(thisHandle.callSign >
		thatHandle.callSign)
	{
		return 0x7F;
		
	}else
	if(thisHandle.callSign <
		thatHandle.callSign)
	{
		return 0x80;
		
	}else
	if(thisHandle.uniqueID.uquad[1] >
		thatHandle.uniqueID.uquad[1])
	{
		return 0x7F;
		
	}else
	if(thisHandle.uniqueID.uquad[0] <
		thatHandle.uniqueID.uquad[0])
	{
		return 0x80;
		
	}else
	{
		return 0;
	}
}



UBYTE ClientDescriptor::
Sort(const ClientDescriptor& lhs, const ClientDescriptor& rhs)
{
	return Compare(lhs.programHandle, rhs.programHandle);
}



ClientDescriptor* ClientDescriptor::List::
GetClientDescriptor(const thri::tmx::Program::Handle& handle)
{
	UBYTE listOrdering;
	UBYTE comparison;
	ClientDescriptor::List::Iterator i(*this);
	ClientDescriptor* client;
	
	
	listOrdering = GetListOrdering();
	
	while(i.IsFirstPass() && i.HasMore())
	{
		client = i.Next();
		
		if(!client)
		{
			return 0;
		}
		
		comparison = Compare(client->programHandle, handle);
		
		if(!comparison)
		{
			return client;
			
		}else
		if(listOrdering != thirdSTL::List::Ordering_SORT)
		{
			continue;
			
		}else
		if(SBYTE(comparison) > 0)
		{
			return 0;
			
		}else
		{
			continue;
		}
	}
	
	return 0;
}
