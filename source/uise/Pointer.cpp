#include "Pointer.h"
#include <3rdLib/util/Pointer.h>

using namespace uise;

#define EDGE {0x00, 0x00, 0x00, 0xFF}
#define FORE {0xFF, 0xFF, 0xFF, 0xFF}
#define BACK {0x00, 0x00, 0x00, 0x00}

struct PIXEL_32BIT
{
	UBYTE r;
	UBYTE g;
	UBYTE b;
	UBYTE a;
};


thirdUIA::hid::Pointer::Message Pointer::status;


static const thirdCGI::graphics::raster::RasterGraphicsSurface&
GetDefaultPointer()
{
	static const PIXEL_32BIT pixelData[16][16] =
	{
		{ EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, FORE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, FORE, FORE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, FORE, FORE, FORE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, FORE, EDGE, EDGE, EDGE, EDGE, EDGE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, FORE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ EDGE, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
	};
	
	static thirdCGI::graphics::raster::RasterGraphicsSurface surface;
	
	surface.planes = 1;
	surface.stride = (sizeof(PIXEL_32BIT) * 16);
	surface.buffer = thirdLib::util::Pointer::ToUnSignedInteger(pixelData);
	surface.dimensions = {16, 16};
	
	surface.colorDescription.channelBitWidth = {
		.r = 0, .g = 8, .b = 16, .a = 24};
	
	surface.colorDescription.channelBitOffset = {
		.r = 8, .g = 8, .b = 8, .a = 8};
	
	surface.colorDepth = 32;
	
	
	return surface;
}




static const thirdCGI::graphics::raster::RasterGraphicsSurface&
GetDamagedSurface()
{
	static const PIXEL_32BIT pixelData[16][16] =
	{
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
	};
	
	static thirdCGI::graphics::raster::RasterGraphicsSurface surface;
	
	surface.planes = 1;
	surface.stride = (sizeof(PIXEL_32BIT) * 16);
	surface.buffer = thirdLib::util::Pointer::ToUnSignedInteger(pixelData);
	surface.dimensions = {16, 16};
	
	surface.colorDescription.channelBitWidth = {
		.r = 0, .g = 8, .b = 16, .a = 24};
	
	surface.colorDescription.channelBitOffset = {
		.r = 8, .g = 8, .b = 8, .a = 8};
	
	surface.colorDepth = 32;
	
	
	return surface;
}



static const thirdCGI::graphics::raster::RasterGraphicsSurface&
GetBlendedSurface()
{
	static const PIXEL_32BIT pixelData[16][16] =
	{
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
		{ BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK, BACK },
	};
	
	static thirdCGI::graphics::raster::RasterGraphicsSurface surface;
	
	surface.planes = 1;
	surface.stride = (sizeof(PIXEL_32BIT) * 16);
	surface.buffer = thirdLib::util::Pointer::ToUnSignedInteger(pixelData);
	surface.dimensions = {16, 16};
	
	surface.colorDescription.channelBitWidth = {
		.r = 0, .g = 8, .b = 16, .a = 24};
	
	surface.colorDescription.channelBitOffset = {
		.r = 8, .g = 8, .b = 8, .a = 8};
	
	surface.colorDepth = 32;
	
	
	return surface;
}


const RasterGraphicsSurface& Pointer::defaultPointer
	= GetDefaultPointer();

const RasterGraphicsSurface& Pointer::damagedSurface
	= GetDamagedSurface();

const RasterGraphicsSurface& Pointer::blendedSurface
	= GetBlendedSurface();