
#pragma once

enum TJSAMP;

class TurboJpeg
{
public:
	// Returns data in the form BGRBGR**********BGR000 where the zeros are padding to 4 byte boundary
	// pScaleDenom: optional output. If the image exceeds MAX_IMAGE_PIXELS / MAX_IMAGE_DIMENSION,
	// it is automatically decoded downscaled with the returned denominator (2, 4, 8...).
	// 1 means no downscaling was required. Pass NULL to ignore (default, backward compatible).
	static void * ReadImage(int &width,   // width of the image loaded (scaled down if downsampled).
						 int &height,  // height of the image loaded (scaled down if downsampled).
						 int &bpp,     // BYTES (not bits) PER PIXEL.
						 TJSAMP &chromoSubsampling, // chromo subsampling of image
						 bool &outOfMemory, // set to true when no memory to read image
						 const void *buffer, // memory address containing jpeg compressed data.
						 int sizebytes, // size of jpeg compressed data.
						 int *pScaleDenom = NULL); // optional output: downsampling denominator used (1 = none).

	// Compress image data into JPEG stream, returns compressed data.
	// The returned buffer must be freed with tj3Free()!
	static void * Compress(const void *buffer, // address of image in memory, format must be 3 bytes per pixel BRGBGR with padding to 4 byte boundary
						 int width, // width of image in pixels
						 int height, // height of image in pixels.
						 int &len, // returns length of compressed data
						 bool &outOfMemory, // returns if out of memory
						 int quality=75); // image quality as a percentage

};
