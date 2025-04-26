#pragma once

#include <3rdUSL/hmis/HardwareDescriptor.h>


namespace hmis
{
	struct DeviceTree
	{
		static thirdUSL::hmis::HardwareDescriptor* root;

		static thirdUSL::hmis::HardwareDescriptor& GetRoot();
	};
}

