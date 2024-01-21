#ifndef _hmis_pci_PCIBusDriver_H
#define _hmis_pci_PCIBusDriver_H

#include <3rdLib/lang/types/Primitives.h>
#include <3rdUSL/hmis/HardwareDescriptor.h>
#include <3rdUSL/hmis/DriverDescriptor.h>
#include <3rdHAL/acpi/asdt/MCFG.h>

namespace hmis { namespace pci
{
	struct PCIBusDriver
	{
		static UBYTE SetupPciConfigSpace();
		
		static UBYTE SetupPciExpressEnhancedConfigSpace(
			const acpi::asdt::MCFG&);
		
		static UBYTE DriverSetup(thirdUSL::hmis::DriverDescriptor&);
		
		static UBYTE DriverStart();
		
		static UBYTE DriverPause();
		
		static UBYTE AddDevice(thirdUSL::hmis::HardwareDescriptor&);
		static UBYTE Enumerate(thirdUSL::hmis::HardwareDescriptor&);
	};
}}

#endif

