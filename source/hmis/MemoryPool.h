#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdUSL/mm/LinearAddressSpace.h>
#include <3rdUSL/hmis/HardwareDescriptor.h>
#include <3rdHAL/acpi/hmis/ResourceDescriptor.h>



namespace hmis
{
	struct MemoryPool
	{
		static const UQUAD base;
		static const UQUAD size;
		
		static thirdUSL::mm::LinearAddressSpace::Allocator&
			deviceDescriptorAllocator;
		
		static thirdUSL::mm::LinearAddressSpace::Allocator&
			generalPurposeAllocator;
		
		public: static void Initialize();
		
		public: static thirdUSL::hmis::HardwareDescriptor*
			RequestHardwareDescriptor();
		
		public: static void* RequestMemory(ULONG size);
		
		public: static acpi::hmis::ResourceDescriptor::List*
			RequestResourceDescriptorList(ULONG size);
		
		public: static uefi::DevicePath::Header*
			RequestDevicePath(ULONG size);
	};
}

