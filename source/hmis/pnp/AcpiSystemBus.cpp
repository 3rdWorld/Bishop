#include "AcpiSystemBus.h"
#include <hmis/MemoryPool.h>
#include <hmis/DeviceTree.h>

#include <3rdLib/util/Pointer.h>
#include <3rdLib/util/MemOps.h>
#include <3rdLib/lang/types/NullTerminatedAsciiString.h>

#include <3rdHAL/uefi/DevicePath.h>
#include <3rdHAL/acpi/lang/aml/DataObject.h>
#include <3rdHAL/acpi/asdt/MCFG.h>
#include <3rdHAL/thri/ios/SystemCall.h>

#include <3rdUSL/hmis/HardwareDescriptor.h>




using namespace hmis::pnp;
using namespace hmis;
using namespace uefi;
using namespace thirdLib::util;
using namespace thirdUSL::hmis;


struct DeviceData
{
	UWORD deviceHandle;
	UWORD parentHandle;
};

thirdUSL::hmis::DriverDescriptor* acpiDriverDescriptor = 0;


const void* AcpiSystemBus::
GetPciMemoryConfigTable()
{
	auto getAcpiMCFG = []()->void*
	{
		_$$_GetAcpiSystemDescriptorTable mcfg('GFCM', {0, 0});
		
		void* ioBuffer;
		UBYTE returnCode;
		
		
		returnCode = mcfg.DispatchSystemCall();
		
		if(returnCode != ReturnCode_OutOfBounds)
		{
			_TRACE_CONDITION_FAILPATH_
			
			return 0;
		}
		
		if(!(ioBuffer = MemoryPool::RequestMemory(mcfg.asdtLength)))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return 0;
		}
		
		mcfg = _$$_GetAcpiSystemDescriptorTable('GFCM', {
			Pointer::ToUnSignedInteger(ioBuffer), mcfg.asdtLength});
	
		returnCode = mcfg.DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			//FYI: We shall leak memory here
			_TRACE_CONDITION_FAILPATH_
			
			return 0;
			
		}else
		{
			
			return ioBuffer;
		}
	};
	
	
	static const void* mcfg = getAcpiMCFG();
	
	return mcfg;
}













UBYTE AcpiSystemBus::
Enumerate()
{
	if(DeviceTree::root)
	{
		return Enumerate(*DeviceTree::root, "\\_SB_");
		
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_InvalidState;
	}
}




UBYTE AcpiSystemBus::
Enumerate(thirdUSL::hmis::HardwareDescriptor& hardwareDescriptor)
{
	const DeviceData* deviceData = 0;
	
	
	
	
	
	
	
	if(hardwareDescriptor.flags.bits.IsIoFilter)
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_NotSupported;
		
	}else
	if(hardwareDescriptor.devicePath->IsExpandedAcpiDevicePath())
	{
		deviceData = (DeviceData*)
			hardwareDescriptor.driverSpecificDeviceData;
		
	}else
	{
		hardwareDescriptor.ioStack.ForEach(
			[&](DeviceDescriptor& dd)->UBYTE
		{
			if(!dd.flags.bits.IsIoFilter)
			{
				return ReturnCode_Success;
			}
			
			if(dd.descriptorType != DeviceDescriptor::Type_Hardware)
			{
				return ReturnCode_Success;
			};
			
			HardwareDescriptor& hd = *(HardwareDescriptor*) &dd;
			
			
			if((hd.devicePath->type == DevicePath::Type_ACPI) && (
				hd.devicePath->subType == DevicePath::ACPI::SubType_Address))
			{
				if(hd.deviceName != hardwareDescriptor.deviceName)
				{
					//We should probably fail
					
					_TRACE_CONDITION_WARNING_
				}
				
				deviceData = (DeviceData*) hd.driverSpecificDeviceData;
				
				return ReturnCode_Break;
				
			}else
			{
				return ReturnCode_Success;
			}
		});
	}
	
	
	
	
	
	
	
	
	
	
	if(deviceData)
	{
		return Enumerate(hardwareDescriptor,
			(const ASCII*)(ULONG(deviceData->deviceHandle)));
		
	}else
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_NotSupported;
	}
}








UBYTE AcpiSystemBus::
Enumerate(
	HardwareDescriptor& deviceTreeNode,
	const ASCII* acpiBusPath)
{
	UBYTE* devicePnpInfoBuffer;
	UBYTE returnCode;
	
	
	
	
	
	
	
	
	if(!(devicePnpInfoBuffer = new UBYTE[0x1000]))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_OutOfMemory;
	}
	
	
	
	
	
	
	
	
	
	do
	{
		returnCode = _$$_EnumerateAcpiBusNode(acpiBusPath, {
			Pointer::ToUnSignedInteger(devicePnpInfoBuffer),
			0x1000}).DispatchSystemCall();
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
		
		returnCode = populate(deviceTreeNode,
			*(::hmis::AcpiDeviceList*) devicePnpInfoBuffer);
		
		if(returnCode != ReturnCode_Success)
		{
			_TRACE_CONDITION_FAILPATH_
			
			break;
		}
		
	}while(0);
	
	
	
	
	
	
	delete [] devicePnpInfoBuffer;
	
	return returnCode;
}







UBYTE AcpiSystemBus::
populate(
	HardwareDescriptor& deviceTreeNode,
	::hmis::AcpiDeviceList& deviceList)
{
	UBYTE* deviceCRSListBuffer = 0;
	
	
	
	
	
	
	
	auto createHardwareDescriptor = [&](
		HardwareDescriptor& treeNode,
		const ::hmis::AcpiDeviceList::Entry& entry)->UBYTE
	{
		DeviceData* deviceData;
		HardwareDescriptor* hardwareDescriptor;
		DevicePath::Header* devicePath;
		UBYTE returnCode;
		
		
		
		
		if(!(hardwareDescriptor = MemoryPool::RequestHardwareDescriptor()))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NullPointer;
		}
		
		hardwareDescriptor->descriptorType
			= HardwareDescriptor::Type_Hardware;
		
		hardwareDescriptor->deviceName = entry.objectName;
		
		hardwareDescriptor->slotNumber = 0;
		
		hardwareDescriptor->driverDescriptor
			= (void*) acpiDriverDescriptor;
		
		if(acpiDriverDescriptor)
		{
			//TODO: Leave this to a Driver Management;
			
			returnCode = acpiDriverDescriptor->deviceList.Insert(
				hardwareDescriptor);
			
			if(returnCode != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_Success;
			}
		}else
		{
			_TRACE_CONDITION_WARNING_
		}
		
		
		
		if(!(deviceData = (DeviceData*) MemoryPool::RequestMemory(
			sizeof(DeviceData))))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NullPointer;
		}
		
		deviceData->deviceHandle = entry.deviceAcpiHandle;
		
		deviceData->parentHandle = entry.parentAcpiHandle;
		
		hardwareDescriptor->driverSpecificDeviceData
			= (void*) deviceData;
		
		
		
		
		
		
		
		
		devicePath = (uefi::DevicePath::Header*) entry.pathData;
		
		if(!(hardwareDescriptor->devicePath
			= MemoryPool::RequestDevicePath(devicePath->length)))
		{
			_TRACE_CONDITION_FAILPATH_
			
			return ReturnCode_NullPointer;
		}
		
		MemOps::Copy(devicePath, hardwareDescriptor->devicePath,
			devicePath->length);
		
		hardwareDescriptor->deviceName = entry.objectName;
		
		
		
		
		
		
		if(devicePath->IsExpandedAcpiDevicePath())
		{
			hardwareDescriptor->hid = ((DevicePath::ACPI::Expanded*)
				hardwareDescriptor->devicePath)->GetHidString();
			
			hardwareDescriptor->cid = ((DevicePath::ACPI::Expanded*)
				hardwareDescriptor->devicePath)->GetCidString();
			
			
			returnCode = treeNode.AddChildDevice(*hardwareDescriptor);
			
			
			if(returnCode != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
				
				return returnCode;
			}
			
			
			if((thirdLib::lang::types::NullTerminatedAsciiString::Compare(
				hardwareDescriptor->hid, "PNP0A03") == 0) || (
				thirdLib::lang::types::NullTerminatedAsciiString::Compare(
				hardwareDescriptor->hid, "PNP0A08") == 0))
			{
				UBYTE resultsBuffer[0x10] = {};
				
				acpi::lang::aml::DataObject::Integer* dataObject
					= (acpi::lang::aml::DataObject::Integer*) resultsBuffer;
				
				returnCode = _$$_EvaluateAcpiDeviceProperty(
					entry.deviceAcpiHandle, "_BBN", {
					Pointer::ToUnSignedInteger(resultsBuffer), 0x10}, 0)
					.DispatchSystemCall();
				
				
				
				if(returnCode == ReturnCode_InvalidArgument)
				{
					//If we are confident about our Arguments,
					//this means _BBN is not implemented, thus defaulting to 0
					
					hardwareDescriptor->slotNumber = 0;
					
					returnCode = ReturnCode_Success;
					
				}else
				if(returnCode != ReturnCode_Success)
				{
					_TRACE_CONDITION_FAILPATH_
					
					return returnCode;
					
				}else
				if(!dataObject->IsInteger())
				{
					_TRACE_CONDITION_FAILPATH_
					
					return returnCode;
				}else
				{
					hardwareDescriptor->slotNumber
						= dataObject->GetValue();
				}
				
				
				
				
				returnCode = _$$_EvaluateAcpiDeviceProperty(
					entry.deviceAcpiHandle, "_SEG", {
					Pointer::ToUnSignedInteger(resultsBuffer), 0x10}, 0)
					.DispatchSystemCall();
				
				
				if(returnCode != ReturnCode_Success)
				{
					//Assume since we were successful with _BBN, _SEG
					//is just not implemented so default to 0
					
					returnCode = ReturnCode_Success;
					
				}else
				if(!dataObject->IsInteger())
				{
					_TRACE_CONDITION_FAILPATH_
					
					return returnCode;
				}else
				{
					hardwareDescriptor->slotNumber
						|= (dataObject->GetValue() << 0x08);
				}
			}
		}else
		{
			hardwareDescriptor->hid = treeNode.hid;
			
			hardwareDescriptor->cid = treeNode.cid;
			
			returnCode = treeNode.ioStack.Insert(hardwareDescriptor);
			
			if(returnCode != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
			}
			
			hardwareDescriptor->flags.bits.IsIoFilter = 1;
			
			
			//DeviceName of the IO Anchor to the ACPI Namespace name 
			treeNode.deviceName = entry.objectName;
			
			//Exit here, _ADR objects that implement _CRS will present
			//_HID which will follow the ExpandedAcpiDevicePath branch
			//We therefore assume their is no _CRS object implemented,
			//_CRS can be dynamically determined and allocated by the
			//parent bus
			return returnCode;
		}
		
		
		
		
		
		returnCode = _$$_EvaluateAcpiDeviceProperty(
			entry.deviceAcpiHandle, "_CRS", {thirdLib::util::Pointer::
			ToUnSignedInteger(deviceCRSListBuffer), 0x1000}, 0)
			.DispatchSystemCall();
		
		if(returnCode == ReturnCode_Success)
		{
			ULONG crsInitListLength;
			
			acpi::lang::aml::DataObject::Buffer* crsBuffer;
			
			crsBuffer = (acpi::lang::aml::DataObject::Buffer*)
				deviceCRSListBuffer;
			
			crsInitListLength = crsBuffer->GetInitListLength();
			
			if(!(hardwareDescriptor->crs = MemoryPool::
				RequestResourceDescriptorList(crsInitListLength)))
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_NullPointer;
			}
			
			crsBuffer->CopyByteList(hardwareDescriptor->crs,
				crsInitListLength);
		}else
		if(returnCode == ReturnCode_InvalidArgument)
		{
			//If we are confident about our Arguments,
			//this means _CRS is not implemented
			
			returnCode = ReturnCode_Success;
		}else
		{
			_TRACE_CONDITION_WARNING_
			
			returnCode = ReturnCode_Success;
		}
		
		
		return ReturnCode_Success;
	};
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	if(!(deviceCRSListBuffer = new UBYTE[0x1000]))
	{
		_TRACE_CONDITION_FAILPATH_
		
		return ReturnCode_OutOfMemory;
	}
	
	
	
	
	
	UBYTE returnCode = deviceList.ForEach(
		[&](const ::hmis::AcpiDeviceList::Entry& entry)->UBYTE
	{
		DevicePath::Header* devicePath;
		
		
		
		
		
		
		devicePath = (uefi::DevicePath::Header*) entry.pathData;
		
		if(devicePath->type != DevicePath::Type_ACPI)
		{
			_TRACE_CONDITION_WARNING_
			
			return ReturnCode_Success;
			
		}else
		if(devicePath->subType == DevicePath::ACPI::SubType_Address)
		{
			DevicePath::ACPI::Address* address
				= (DevicePath::ACPI::Address*) devicePath;
			
			returnCode = deviceTreeNode.childDeviceList.ForEach(
				[&](DeviceDescriptor& dd)->UBYTE
			{
				HardwareDescriptor& hd = *(HardwareDescriptor*) &dd;
				
				if(hd.devicePath->IsPciHardwareDevicePath())
				{
					DevicePath::Hardware::PCI* pci
						= (DevicePath::Hardware::PCI*) hd.devicePath;
					
					if((((address->_ADR >> 0x10) & 0xFF) == pci->device) && (
						((address->_ADR >> 0x00) & 0xFF) == pci->function))
					{
						returnCode = createHardwareDescriptor(hd, entry);
						
						if(returnCode != ReturnCode_Success)
						{
							_TRACE_CONDITION_FAILPATH_
							
							return returnCode;
							
						}else
						{
							return ReturnCode_Break;
						}
					}
				}
				
				
				return ReturnCode_Success;
			});
			
			if(returnCode == ReturnCode_Break)
			{
				return ReturnCode_Success;
			}else
			{
				return returnCode;
			}
			
		}else
		if(devicePath->subType != DevicePath::ACPI::SubType_Expanded)
		{
			_TRACE_CONDITION_WARNING_
			
			return ReturnCode_Success;
			
		}else
		{
			returnCode = createHardwareDescriptor(deviceTreeNode, entry);
			
			if(returnCode != ReturnCode_Success)
			{
				_TRACE_CONDITION_FAILPATH_
				
				return ReturnCode_Success;
				
			}else
			{
				return returnCode;
			}
		}
	});
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	delete [] deviceCRSListBuffer;
	
	
	
	
	if(returnCode != ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
	
	
	return returnCode;
}





UBYTE AcpiSystemBus::
DriverSetup(thirdUSL::hmis::DriverDescriptor& driverDescriptor)
{
	acpiDriverDescriptor = &driverDescriptor;
	
	return ReturnCode_Success;
}

UBYTE AcpiSystemBus::
DriverPause()
{
	_TRACE_TODO_UNIMPLIMENTED_
	
	return ReturnCode_FunctionNotImplemented;
}

UBYTE AcpiSystemBus::
DriverStart()
{
	_TRACE_TODO_UNIMPLIMENTED_
	
	return ReturnCode_FunctionNotImplemented;
}