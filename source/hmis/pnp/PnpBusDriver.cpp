#include "PnpBusDriver.h"
#include "AcpiSystemBus.h"
#include <hmis/DeviceTree.h>
#include <hmis/MemoryPool.h>
#include <hmis/pci/PCIBusDriver.h>

#include <3rdLib/util/Pointer.h>
#include <3rdLib/lang/types/NullTerminatedAsciiString.h>
#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/hmis/HardwareDescriptor.h>


using namespace hmis::pnp;
using namespace hmis;
using namespace thirdLib::util;
using namespace thirdUSL::hmis;
using namespace thirdUSL::thri;



inline void* operator new(size_t, void* p)
{
	_TRACE_FUNCTION_
	
	return p;
}



static UBYTE LoadPciBusDriver()
{
	//TODO: Do Actual loading
	static DriverDescriptor* pciBusDriverDescriptor = 0;
	
	if(pciBusDriverDescriptor)
	{
		return ReturnCode_Success;
		
	}else
	if((pciBusDriverDescriptor = (DriverDescriptor*)
		MemoryPool::RequestMemory(sizeof(DriverDescriptor))))
	{
		pciBusDriverDescriptor = new(pciBusDriverDescriptor)
			DriverDescriptor();
		
		pciBusDriverDescriptor->programHandle
			= ThreadInfo::GetProgramHandle();
		
		return hmis::pci::PCIBusDriver::DriverSetup(*pciBusDriverDescriptor);
		
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
}


static UBYTE LoadAcpiBusDriver()
{
	//TODO: Do Actual loading
	static DriverDescriptor* acpiBusDriverDescriptor = 0;
	
	if(acpiBusDriverDescriptor)
	{
		return ReturnCode_Success;
		
	}else
	if((acpiBusDriverDescriptor = (DriverDescriptor*)
		MemoryPool::RequestMemory(sizeof(DriverDescriptor))))
	{
		acpiBusDriverDescriptor = new(acpiBusDriverDescriptor)
			DriverDescriptor();
		
		acpiBusDriverDescriptor->programHandle
			= ThreadInfo::GetProgramHandle();
		
		return hmis::pnp::AcpiSystemBus::DriverSetup(*acpiBusDriverDescriptor);
		
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_Failure;
	}
}






static UBYTE ScanAcpiBusNode(HardwareDescriptor& deviceTreeNode)
{
	if(!deviceTreeNode.devicePath->IsExpandedAcpiDevicePath()
		&& deviceTreeNode.ioStack.IsEmpty())
	{
		//Only devices with an ACPI Device Path should be scanned
		//Therefore check for an ExpandedAcpiDevicePath or for
		//an IoStack which may have an IoFilter Device with an
		//AdressAcpiDevicePath
		
		return ReturnCode_Success;
	}
	
	UBYTE returnCode = AcpiSystemBus::Enumerate(deviceTreeNode);
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	return deviceTreeNode.childDeviceList.ForEach(
		[&](DeviceDescriptor& d)->UBYTE
	{
		HardwareDescriptor& hd = *(HardwareDescriptor*) &d;
		
		return ScanAcpiBusNode(hd);
	});
};








void PnpBusDriver::
Enumerate()
{
	if(LoadAcpiBusDriver() != ReturnCode_Success)
	{
		//TODO: Fail Here
		_TRACE_CONDITION_FAILPATH_
	}
	
	
	if(AcpiSystemBus::Enumerate() != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	
	DeviceTree::root->childDeviceList.ForEach(
		[](DeviceDescriptor& deviceDescriptor)->UBYTE
	{
		HardwareDescriptor& hardwareDescriptor
			= (HardwareDescriptor&) deviceDescriptor;

		
		if(!hardwareDescriptor.hid)
		{
			return 0;
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareDescriptor.hid, "PNP0C0A") == 0)
		{
			//LOG("ACPI ControlMethod Battery");
			
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareDescriptor.hid, "PNP0303") == 0)
		{
			//LOG("PS2 Compatible Controller");
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareDescriptor.hid, "PNP0C0F") == 0)
		{
			//LOG("PCI Interrupt Link");
			
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareDescriptor.hid, "PNP0A03") == 0)
		{
			LOG("PCI LocalBus");
			if(LoadPciBusDriver() != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
			}
			
			
			if(hardwareDescriptor.cid && thirdLib::lang::types::
				NullTerminatedAsciiString::Compare(
				hardwareDescriptor.cid, "PNP0A08") == 0)
			{
				if(const void* mcfg = AcpiSystemBus::GetPciMemoryConfigTable())
				{
					hmis::pci::PCIBusDriver::SetupPciExpressEnhancedConfigSpace(
						*(acpi::asdt::MCFG*) mcfg);
				}
			}
			
			
			hmis::pci::PCIBusDriver::AddDevice(hardwareDescriptor);
			hmis::pci::PCIBusDriver::Enumerate(hardwareDescriptor);
			
			ScanAcpiBusNode(hardwareDescriptor);
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareDescriptor.hid, "PNP0A08") == 0)
		{
			//LOG("PCI Express");
			
			
			if(LoadPciBusDriver() != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
			}
			
			if(const void* mcfg = AcpiSystemBus::GetPciMemoryConfigTable())
			{
				hmis::pci::PCIBusDriver::SetupPciExpressEnhancedConfigSpace(
					*(acpi::asdt::MCFG*) mcfg);
			}else
			{
				_TRACE_CONDITION_WARNING_
			}
			
			hmis::pci::PCIBusDriver::AddDevice(hardwareDescriptor);
			hmis::pci::PCIBusDriver::Enumerate(hardwareDescriptor);
			
			ScanAcpiBusNode(hardwareDescriptor);
			
		}else
		{
			ScanAcpiBusNode(hardwareDescriptor);
		}
		
		return 0;
	});
}

