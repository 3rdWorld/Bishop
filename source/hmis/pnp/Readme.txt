TODO:

FYI

DeviceDescriptors are of two types, i.e
	
	HardwareDescriptors ==> HW
	FunctionDescriptors ==> FX

which are based on ideas from the Microsoft(TM) Window(R)
device driver model. However, in our case, an IoFilter is
not a seperate DeviceDescriptor class but subtypes of
HW and FX descriptors (which are either IoFilters or IoAnchors).

In a device IoStack there are a maximum of two IoAnchors i.e.,
one of each type (HW IoAnchor and FX IoAnchor). You may however
several IoFilters of either type.


DeviceObjectModel (IoTree) consists of two concepts:

	1)	DeviceTree, made up of HW anchors with child-parent relations

	2)	IoStack, made up IoFilters and one FX Anchor
	

:::::::::::	
AddDevice Method Is An offer by the Driver Framework to a Driver
to manage the Function of a Device or provide IoFiltering.
The hardware management of a device is always claimed by the Driver
that enumerated it.
