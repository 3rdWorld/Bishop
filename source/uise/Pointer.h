#pragma once

#include <3rdLib/lang/types/Primitives.h>
#include <3rdCGI/graphics/raster/RasterGraphicsSurface.h>
#include <3rdUIA/hid/Pointer.h>

namespace uise
{
	using namespace thirdCGI::graphics::raster;
	
	struct Pointer
	{
		static const RasterGraphicsSurface& defaultPointer;
		static const RasterGraphicsSurface& damagedSurface;
		static const RasterGraphicsSurface& blendedSurface;
		
		static thirdUIA::hid::Pointer::Message status;
	};
}


