#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdUSL/hmis/HardwareDescriptor.h>
#include <3rdUSL/hmis/DriverDescriptor.h>
#include <hmis/AcpiDeviceList.h>

namespace hmis { namespace pnp
{
	struct AcpiSystemBus
	{
		static const void* GetPciMemoryConfigTable();
		static UBYTE Enumerate();
		static UBYTE Enumerate(thirdUSL::hmis::HardwareDescriptor&);
		static UBYTE Enumerate(thirdUSL::hmis::HardwareDescriptor&,
			const ASCII* acpiBusPath);
		
		static UBYTE populate(thirdUSL::hmis::HardwareDescriptor&,
			::hmis::AcpiDeviceList&);
		
		static UBYTE DriverSetup(thirdUSL::hmis::DriverDescriptor&);
		
		static UBYTE DriverPause();
		
		static UBYTE DriverStart();
	};
}}