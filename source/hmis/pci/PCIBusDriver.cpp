#include "PCIBusDriver.h"
#include <hmis/MemoryPool.h>

#include <3rdLib/lang/types/NullTerminatedAsciiString.h>
#include <3rdLib/util/Trace.h>
#include <3rdLib/util/Pointer.h>
#include <3rdLib/util/MemOps.h>


#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdUSL/mm/LinearAddressSpace.h>

#include <3rdHAL/thri/ios/Interrupt.h>
#include <3rdHAL/thri/ios/SystemCall.h>
#include <3rdHAL/hmis/AcpiDeviceList.h>
#include <3rdHAL/acpi/asdt/MCFG.h>
#include <3rdHAL/acpi/hmis/ResourceDescriptor.h>

#include <hawai/system/bus/pci/PciCapability.h>
#include <hawai/system/bus/pci/PciInterconnect.h>
#include <hawai/system/bus/pci/PciBridge.h>
#include <hawai/system/bus/pci/PciExpress.h>
#include <hawai/system/bus/pci/PciConfigSpace.h>

#include <driver/system/bus/pci/PciBusEnumerator.h>


using namespace hmis::pci;
using namespace hmis;
using namespace driver::system::bus::pci;
using namespace hawai::system::bus::pci;
using namespace acpi::asmi;
using namespace thirdLib::util;
using namespace thirdUSL::hmis;
using namespace thirdUSL::thri;
using namespace thirdUSL::mm;








struct HostPciConfigSpace : PciConfigSpace::Interface
{
	private:
	
	HostPciConfigSpace(){}
	
	public:
	
	inline UQUAD BusAddressToAcpiAddress(
		const PciCoordinates& pciCoordinates,
		UBYTE configSpaceOffset)
	{
		return (configSpaceOffset | pciCoordinates.ToAcpiAddress());
	}


	ULONG ReadRegister(PciCoordinates pciCoordinates, UBYTE reg,
		UBYTE regSize) override;

	UBYTE LoadRegister(PciCoordinates pciCoordinates, UBYTE reg,
		UBYTE regSize, ULONG value) override;
	
	
	
	static HostPciConfigSpace instance;
};













HostPciConfigSpace HostPciConfigSpace::instance;

static DriverDescriptor* pciBusDriverDescriptor = 0;






ULONG HostPciConfigSpace::
ReadRegister(PciCoordinates pciCoordinates, UBYTE reg, UBYTE regSize)
{
	UQUAD value;
	_$$_OpRegionAccess::Flags ioFlags;
	
	UBYTE returnCode;
	
	value = -1ull;
	
	ioFlags.value = 0;
	ioFlags.bits.IoAccessSize = acpi::asmi::GenericAddress::
		BytesToAccessSize(regSize);
	
	returnCode = _$$_OpRegionAccess(
		ioFlags, acpi::asmi::AddressSpace::ID_PciConfigSpace,
		BusAddressToAcpiAddress(pciCoordinates, reg),
		Pointer::ToUnSignedInteger(&value), regSize).DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	return ULONG(value);
}










UBYTE HostPciConfigSpace::
LoadRegister(PciCoordinates pciCoordinates, UBYTE reg, UBYTE regSize,
	ULONG value)
{
	_$$_OpRegionAccess::Flags ioFlags;
	
	UBYTE returnCode;
	
	ioFlags.value = 0;
	
	ioFlags.bits.WriteAccess = 1;
	
	
	ioFlags.bits.IoAccessSize = acpi::asmi::GenericAddress::
		BytesToAccessSize(regSize);
	
	returnCode = _$$_OpRegionAccess(
		ioFlags, acpi::asmi::AddressSpace::ID_PciConfigSpace,
		BusAddressToAcpiAddress(pciCoordinates, reg),
		Pointer::ToUnSignedInteger(&value), regSize).DispatchSystemCall();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	return returnCode;
}










UBYTE PCIBusDriver::
SetupPciConfigSpace()
{
	if(PciConfigSpace::interface)
	{
		return ReturnCode_InvalidState;
	}
	
	PciConfigSpace::interface = &HostPciConfigSpace::instance;
	
	AddressSpace::RegisterGlobalAddressSpaceInterface(
		HostPciConfigSpace::instance);
	
	return ReturnCode_Success;
}







UBYTE PCIBusDriver::
SetupPciExpressEnhancedConfigSpace(const acpi::asdt::MCFG& mcfg)
{
	UBYTE returnCode;
	acpi::asmi::SystemMemory::Mapping mapping;
	acpi::asdt::MCFG::Entry* e;
	
	const ULONG entryCount = mcfg.GetEntryCount();
	
	
	
	if(PciInterconnect::mcfg)
	{
		if(PciInterconnect::mcfg != &mcfg)
		{
			_TRACE_CONDITION_FAILPATH_
			
			 return ReturnCode_InvalidState;
		}else
		{
			return ReturnCode_Success;
		}
	}
	
	
	
	if(!entryCount)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidArgument;
	}
	
	
	
	if(!(PciInterconnect::mcfg = (acpi::asdt::MCFG*)
		new UBYTE[mcfg.header.length]))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_NullPointer;
	}
	
	
	MemOps::Copy(&mcfg, PciInterconnect::mcfg, mcfg.header.length);
	
	
	
	for(ULONG i = 0; i < entryCount; i++)
	{
		e = &PciInterconnect::mcfg->entry[i];
		
		mapping.attribute.value = 0;
		
		mapping.region.base = e->baseMemory;
		
		mapping.region.size = (((e->busN - e->bus0) + 1) << 0x14);
		
		mapping.linearAddress = Pointer::ToUnSignedInteger(
			LinearAddressSpace::largeHeap.
			RequestLinearAddressSpace(mapping.region.size));
		
		if(!mapping.linearAddress)
		{
			_TRACE_CONDITION_FAILPATH_
			
			returnCode = ReturnCode_NullPointer;
			
			break;
		}
		
		mapping.attribute.bits.UnCacheable = 1;
		mapping.attribute.bits.WriteThrough	= 1;
		mapping.attribute.bits.ExecuteProtection = 1;
		
		returnCode = _$$_CreateMemoryMapping(
			mapping, ThreadInfo::GetProgramHandle())
			.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		e->baseMemory = mapping.linearAddress;
	}
		
	
	if(returnCode != ReturnCode_Success)
	{
		delete [] PciInterconnect::mcfg;
		
		PciInterconnect::mcfg = 0;
		
		return returnCode;
		
	}else
	{
		PciConfigSpace::interface
			= &PciExpress::EnhancedConfigSpace::instance;
		
		AddressSpace::RegisterGlobalAddressSpaceInterface(
			PciExpress::EnhancedConfigSpace::instance);
		
		return ReturnCode_Success;
	}
}







static uefi::DevicePath::Hardware::PCI*
CreatePciDevicePath()
{
	uefi::DevicePath::Hardware::PCI* devicePath;
	
	if(!(devicePath = (uefi::DevicePath::Hardware::PCI*)
		MemoryPool::RequestMemory(sizeof(uefi::DevicePath::Hardware::PCI))))
	{
		return 0;
	}
	
	
	devicePath->type
		= uefi::DevicePath::Type_Hardware;
	
	devicePath->subType
		= uefi::DevicePath::Hardware::SubType_PCI;
	
	devicePath->length
		= sizeof(uefi::DevicePath::Hardware::PCI);
	
	devicePath->device
		= 0xFF;
	
	devicePath->function
		= 0xFF;
	
	return devicePath;
}










UBYTE PCIBusDriver::
DriverSetup(thirdUSL::hmis::DriverDescriptor& driverDescriptor)
{
	UBYTE returnCode;
	
	returnCode = SetupPciConfigSpace();
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return returnCode;
	}
	
	pciBusDriverDescriptor = &driverDescriptor;
	
	return returnCode;
}


UBYTE PCIBusDriver::
DriverStart()
{
	_TRACE_TODO_UNIMPLIMENTED_
	
	return ReturnCode_FunctionNotImplemented;
}


UBYTE PCIBusDriver::
DriverPause()
{
	_TRACE_TODO_UNIMPLIMENTED_
	
	return ReturnCode_FunctionNotImplemented;
}








UBYTE PCIBusDriver::
AddDevice(HardwareDescriptor& deviceDescriptor)
{
	if(!pciBusDriverDescriptor)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidState;
		
	}else
	if(deviceDescriptor.descriptorType
		!= DeviceDescriptor::Type_Hardware)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidArgument;
		
	}else
	if(!deviceDescriptor.devicePath)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidArgument;
		
	}else
	if(deviceDescriptor.devicePath->IsExpandedAcpiDevicePath())
	{
		const ASCII* hardwareId;
		
		if(!(hardwareId = ((uefi::DevicePath::ACPI::Expanded*)
			deviceDescriptor.devicePath)->GetHidString()))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_InvalidArgument;
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareId, "PNP0A03") == 0)
		{
			//LOG("PCI LocalBus");
			deviceDescriptor.flags.bits.Enumerable = 1;
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			hardwareId, "PNP0A08") == 0)
		{
			//LOG("PCI Express");
			deviceDescriptor.flags.bits.Enumerable = 1;
			
		}else
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NotSupported;
			
		}
		
	}else
	if(!deviceDescriptor.devicePath->IsPciHardwareDevicePath())
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_NotSupported;
		
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		//TODO: Check if this is a PCIBridge
		return ReturnCode_FunctionNotImplemented;
	}
	
	
	
	return ReturnCode_FunctionNotImplemented;
}







UBYTE PCIBusDriver::
Enumerate(HardwareDescriptor& pciBusDescriptor)
{
	UWORD segNumber = 0;
	UBYTE busNumber = 0;
	UBYTE returnCode;
	
	//TODO:
	//Check that we are an FX or HW Driver to this Device
	//else return
	
	if(!pciBusDescriptor.devicePath)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidState;
		
	}else
	if(pciBusDescriptor.devicePath->IsExpandedAcpiDevicePath())
	{
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			pciBusDescriptor.hid, "PNP0A03") == 0)
		{
			//LOG("PCI LocalBus");
			busNumber = ((pciBusDescriptor.slotNumber >> 0x00) & 0x00FF);
			
			segNumber = ((pciBusDescriptor.slotNumber >> 0x08) & 0xFFFF);
			
		}else
		if(thirdLib::lang::types::NullTerminatedAsciiString::Compare(
			pciBusDescriptor.hid, "PNP0A08") == 0)
		{
			//LOG("PCI Express");
			busNumber = ((pciBusDescriptor.slotNumber >> 0x00) & 0x00FF);
			
			segNumber = ((pciBusDescriptor.slotNumber >> 0x08) & 0xFFFF);
			
		}else
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NotSupported;
			
		}
		
	}else
	if(!pciBusDescriptor.devicePath->IsPciHardwareDevicePath())
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_NotSupported;
		
	}else
	{
		PciBridge pciBridge;
		
		pciBridge.coordinates.bits.Function
			= ((uefi::DevicePath::Hardware::PCI*)
			pciBusDescriptor.devicePath)->function;
		
		pciBridge.coordinates.bits.DeviceSlot
			= ((uefi::DevicePath::Hardware::PCI*)
			pciBusDescriptor.devicePath)->device;
		
		pciBridge.coordinates.bits.BusNumber
			= (pciBusDescriptor.slotNumber & 0xFF);
		
		pciBridge.coordinates.bits.Segment
			= ((pciBusDescriptor.slotNumber >> 0x08) & 0xFFFF);
		
		
		if((pciBridge.GetDeviceBaseClass() != 0x06) || (
			pciBridge.GetDeviceTypeClass() != 0x04))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NotSupported;
		}
		
		busNumber = pciBridge.GetSecondaryBus();
		
		segNumber = ((pciBusDescriptor.slotNumber >> 0x08) & 0xFFFF);
		
		if(pciBridge.coordinates.bits.BusNumber >= busNumber)
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_InvalidArgument;
		}
	}
	
	
	
	
	
	
	
	
	
	
	
	
	return PciBusEnumerator(segNumber, busNumber).ForEachDeviceSlot(
		[&](const PciDeviceSlot& deviceSlot)->UBYTE
	{
		return deviceSlot.ForEachFunction(
			[&](PciBusTarget& busTarget)->UBYTE
		{
			uefi::DevicePath::Hardware::PCI* devicePath;
			HardwareDescriptor* deviceDescriptor;
			
			
			if(!(devicePath = CreatePciDevicePath()))
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_Failure;
			}
			
			if(!(deviceDescriptor = MemoryPool::RequestHardwareDescriptor()))
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_Failure;
			}
			
			deviceDescriptor->flags.bits.Enumerable
				= 0;
			
			deviceDescriptor->descriptorType
				= DeviceDescriptor::Type_Hardware;
			
			deviceDescriptor->deviceName = 0;
			
			deviceDescriptor->slotNumber
				= (busTarget.coordinates.bits.BusNumber | (
				busTarget.coordinates.bits.Segment << 8));
			
			devicePath->device
				= busTarget.coordinates.bits.DeviceSlot;
			
			devicePath->function
				= busTarget.coordinates.bits.Function;
			
			deviceDescriptor->devicePath
				= devicePath;
			
			if(pciBusDriverDescriptor)
			{
				deviceDescriptor->driverDescriptor
					= pciBusDriverDescriptor;

				returnCode = pciBusDriverDescriptor->deviceList.Insert(
					deviceDescriptor);
				
				if(returnCode != ReturnCode_Success)
				{
					_TRACE_CONDITION_FAILPATH_
					
					return returnCode;
				}
			}else
			{
				_TRACE_CONDITION_WARNING_
			}
			
			
			
			if(ASCII* hid = (ASCII*) MemoryPool::RequestMemory(
				sizeof(UQUAD) + 1))
			{
				const UQUAD value = busTarget.GetHID();
				
				MemOps::Copy(&value, hid, sizeof(UQUAD));
				
				hid[sizeof(UQUAD)] = 0;
				
				deviceDescriptor->hid = hid;
				
			}else
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_NullPointer;
			}
			
			
			
			if(ASCII* cid = (ASCII*) MemoryPool::RequestMemory(
				sizeof(UQUAD) + 1))
			{
				const UQUAD value = busTarget.GetCID();
				
				MemOps::Copy(&value, cid, sizeof(UQUAD));
				
				cid[sizeof(UQUAD)] = 0;
				
				deviceDescriptor->cid = cid;
				
			}else
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_NullPointer;
			}
			
			
			
			

			
			do
			{
				acpi::asmi::AddressSpace::Region crsBuffer(0, 0);
				
				
				if(busTarget.GetCurrentResourceSettings(crsBuffer)
					!= ReturnCode_Success)
				{
					_TRACE_CONDITION_WARNING_
					
					break;
				}
				
				if(!crsBuffer.size)
				{
					_TRACE_CONDITION_WARNING_
					
					break;
				}
				
				
				if(!(crsBuffer.base = Pointer::ToUnSignedInteger(
					MemoryPool::RequestMemory(crsBuffer.size))))
				{
					_TRACE_CONDITION_FAILPATH_
					
					return ReturnCode_NullPointer;
				}
				
				
				deviceDescriptor->crs
					= (acpi::hmis::ResourceDescriptor::List*) crsBuffer.base;
				
				if(busTarget.GetCurrentResourceSettings(crsBuffer)
					!= ReturnCode_Success)
				{
					_TRACE_CONDITION_WARNING_
					
					delete deviceDescriptor->crs;
					
					deviceDescriptor->crs = 0;
					
					return ReturnCode_NullPointer;
				}
				
				if(deviceDescriptor->crs)
				{
					//deviceDescriptor->crs->DumpResourceDescriptorList();
				}
				
			}while(0);
			
			
			
			
			
			LOG("Enumerated PCIDevice(%s, %s)",
				deviceDescriptor->cid, deviceDescriptor->hid);
			
			
			returnCode = pciBusDescriptor.AddChildDevice(
				*deviceDescriptor);
			
			if(returnCode != ReturnCode_Success)
			{
				return returnCode;
			}
			
			
			if((busTarget.GetDeviceBaseClass() != 0x06) || (
				busTarget.GetDeviceTypeClass() != 0x04))
			{
				return returnCode;
				
			}else
			if(busNumber < ((PciBridge*) &busTarget)->GetSecondaryBus())
			{
				deviceDescriptor->flags.bits.Enumerable = 1;
				
				
				returnCode = Enumerate(*deviceDescriptor);
				
				if(returnCode != ReturnCode_Success)
				{
					_TRACE_CONDITION_FAILPATH_
					
					return returnCode;
				}
				
				return returnCode;
				
			}else
			{
				_TRACE_CONDITION_WAYPOINT_
				
				return returnCode;
			}
		});
	});
	
}


