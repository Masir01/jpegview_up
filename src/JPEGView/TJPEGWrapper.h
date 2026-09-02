
#pragma once

enum TJSAMP;

class TurboJpeg
{
public:
	// Returns data in the form BGRBGR**********BGR000 where the zeros are padding to 4 byte boundary
	// pScaleDenom: optional output. If the image is downscale-decoded (oversized handling or
	// fast fit-to-screen mode), the used denominator (2, 4, 8...) is returned. 1 means no
	// downscaling was required. Pass NULL to ignore (default, backward compatible).
	// nScreenWidth/nScreenHeight: optional monitor size in pixels. When > 0 and the
	// "FastFitScreenDecode" (extreme speed mode) setting is enabled, images larger than the
	// screen are decoded at the largest 1/2^n size that still fits within the screen bounds,
	// preserving the aspect ratio. Pass 0, 0 to disable (default, backward compatible).
	static void * ReadImage(int &width,   // width of the image loaded (scaled down if downsampled).
						 int &height,  // height of the image loaded (scaled down if downsampled).
						 int &bpp,     // BYTES (not bits) PER PIXEL.
						 TJSAMP &chromoSubsampling, // chromo subsampling of image
						 bool &outOfMemory, // set to true when no memory to read image
						 const void *buffer, // memory address containing jpeg compressed data.
						 int sizebytes, // size of jpeg compressed data.
						 int *pScaleDenom = NULL, // optional output: downsampling denominator used (1 = none).
						 int nScreenWidth = 0,   // optional monitor width for fast fit-to-screen decode.
						 int nScreenHeight = 0); // optional monitor height for fast fit-to-screen decode.

	// Compress image data into JPEG stream, returns compressed data.
	// The returned buffer must be freed with tj3Free()!
	static void * Compress(const void *buffer, // address of image in memory, format must be 3 bytes per pixel BRGBGR with padding to 4 byte boundary
						 int width, // width of image in pixels
						 int height, // height of image in pixels.
						 int &len, // returns length of compressed data
						 bool &outOfMemory, // returns if out of memory
						 int quality=75); // image quality as a percentage

};
