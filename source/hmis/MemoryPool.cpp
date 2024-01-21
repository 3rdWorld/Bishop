#include "MemoryPool.h"
#include <3rdUSL/mm/LinearAddressSpace.h>
#include <3rdUSL/thri/ThreadInfo.h>
#include <3rdLib/util/Pointer.h>
#include <3rdLib/util/Alignment.h>
#include <3rdHAL/thri/ios/SystemCall.h>

using namespace hmis;
using namespace thirdLib::util;
using namespace thirdUSL::mm;
using namespace thirdUSL::thri;
using namespace thirdUSL::hmis;



#define _DeviceDescriptorPool_SIZE_		0x10000
#define _DeviceDescriptorPool_BASE_		((_HMIS_OBJECT_POOL_BASE_ + _HMIS_OBJECT_POOL_SIZE_) - _DeviceDescriptorPool_SIZE_)

#define _GeneralPurposePool_SIZE_		0x10000
#define _GeneralPurposePool_BASE_		(_DeviceDescriptorPool_BASE_ - _GeneralPurposePool_SIZE_)

#define _MEMORY_POOL_BASE_				_GeneralPurposePool_BASE_
#define _MEMORY_POOL_SIZE_				((_HMIS_OBJECT_POOL_BASE_ + _HMIS_OBJECT_POOL_SIZE_) - _MEMORY_POOL_BASE_)


const UQUAD MemoryPool::base = _MEMORY_POOL_BASE_;

const UQUAD MemoryPool::size = _MEMORY_POOL_SIZE_;


thirdUSL::mm::LinearAddressSpace::Allocator MemoryPool::
deviceDescriptorAllocator(
	{_DeviceDescriptorPool_BASE_, _DeviceDescriptorPool_SIZE_},
	sizeof(void*));

thirdUSL::mm::LinearAddressSpace::Allocator MemoryPool::
generalPurposeAllocator(
	{_GeneralPurposePool_BASE_, _GeneralPurposePool_SIZE_},
	sizeof(void*));

inline void* operator new(size_t, void* p)
{
	_TRACE_FUNCTION_
	
	return p;
}


void MemoryPool::
Initialize()
{
	if(_$$_CreateZeroPageMapping({
		MemoryPool::deviceDescriptorAllocator.base,
		MemoryPool::deviceDescriptorAllocator.size},
		ThreadInfo::GetProgramHandle()).DispatchSystemCall()
		!= ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
		LOG("%qx %qx", MemoryPool::deviceDescriptorAllocator.base,
			MemoryPool::deviceDescriptorAllocator.size);
	}
	
	if(_$$_CreateZeroPageMapping({
		MemoryPool::generalPurposeAllocator.base,
		MemoryPool::generalPurposeAllocator.size},
		ThreadInfo::GetProgramHandle()).DispatchSystemCall()
		!= ReturnCode_Success)
	{
		_TRACE_CONDITION_FAILPATH_
	}
}


HardwareDescriptor* MemoryPool::
RequestHardwareDescriptor()
{
	HardwareDescriptor* descriptor;
	
	if((descriptor =  (HardwareDescriptor*)
		deviceDescriptorAllocator.RequestLinearAddressSpace(
		Alignment::RoundOff(sizeof(HardwareDescriptor),
		deviceDescriptorAllocator.alignment))))
	{
		descriptor = new(descriptor) HardwareDescriptor();
	}
	
	return descriptor;
}



acpi::hmis::ResourceDescriptor::List* MemoryPool::
RequestResourceDescriptorList(ULONG size)
{
	return (acpi::hmis::ResourceDescriptor::List*)
		generalPurposeAllocator.RequestLinearAddressSpace(
		Alignment::RoundOff(size, generalPurposeAllocator.alignment));
}

void* MemoryPool::
RequestMemory(ULONG size)
{
	return generalPurposeAllocator.RequestLinearAddressSpace(
		Alignment::RoundOff(size, generalPurposeAllocator.alignment));
}

uefi::DevicePath::Header* MemoryPool::
RequestDevicePath(ULONG size)
{
	return (uefi::DevicePath::Header*) generalPurposeAllocator
		.RequestLinearAddressSpace(Alignment::RoundOff(
		size, generalPurposeAllocator.alignment));
}
