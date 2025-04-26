#include "DeviceTree.h"
#include "MemoryPool.h"

using namespace hmis;

thirdUSL::hmis::HardwareDescriptor*
DeviceTree::root = 0;






thirdUSL::hmis::HardwareDescriptor& DeviceTree::
GetRoot()
{
    const auto createDeviceTreeRoot =
        [&]()->thirdUSL::hmis::HardwareDescriptor&
    {
        if(auto hd = MemoryPool::RequestHardwareDescriptor())
        {
            hd->deviceName = 'TOOR';
            
            hd->descriptorType
                = HardwareDescriptor::Type_Hardware;
        
            hd->flags.bits.Enumerable = true;
        
            return hd;
            
        }else
        {
            _PANIC_CONDITION_BUGCHECK_
        }
    }

    static auto& root = createDeviceTreeRoot();

    return root;
}

