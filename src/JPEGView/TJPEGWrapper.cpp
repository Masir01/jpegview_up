#include "stdafx.h"
#include "TJPEGWrapper.h"
#include "libjpeg-turbo\include\turbojpeg.h"
#include "MaxImageDef.h"
#include "SettingsProvider.h"

// Finds the smallest downsampling factor (1/2, 1/4, 1/8) that brings the image within
// MAX_IMAGE_PIXELS / MAX_IMAGE_DIMENSION limits. Returns TJUNSCALED if no downsampling is needed.
// nMaxDenom limits the largest allowed denominator (e.g. 2 = never downscale more than 1/2).
static tjscalingfactor FindDownscaleFactor(int nWidth, int nHeight, int nMaxDenom) {
	tjscalingfactor sf = TJUNSCALED;
	for (int nDenom = 2; nDenom <= nMaxDenom; nDenom *= 2) {
		tjscalingfactor candidate = { 1, nDenom };
		int nScaledW = TJSCALED(nWidth, candidate);
		int nScaledH = TJSCALED(nHeight, candidate);
		if (nScaledW <= MAX_IMAGE_DIMENSION && nScaledH <= MAX_IMAGE_DIMENSION &&
			(double)nScaledW * nScaledH <= MAX_IMAGE_PIXELS) {
			sf = candidate;
			break;
		}
	}
	return sf;
}

// Fast fit-to-screen mode: finds the largest downsampling factor (1/2, 1/4, 1/8) whose
// scaled dimensions both fit within the given screen bounds (aspect ratio preserved).
// Returns TJUNSCALED when the image already fits (or cannot be reduced enough).
static tjscalingfactor FindScreenFitFactor(int nWidth, int nHeight, int nScreenWidth, int nScreenHeight, int nMaxDenom) {
	tjscalingfactor sf = TJUNSCALED;
	for (int nDenom = 2; nDenom <= nMaxDenom; nDenom *= 2) {
		tjscalingfactor candidate = { 1, nDenom };
		int nScaledW = TJSCALED(nWidth, candidate);
		int nScaledH = TJSCALED(nHeight, candidate);
		if (nScaledW <= nScreenWidth && nScaledH <= nScreenHeight) {
			sf = candidate;
			break;
		}
	}
	return sf;
}

// Detects lossless JPEG streams (SOF3/SOF7/SOF11/SOF15) by scanning the marker segments.
// libjpeg-turbo ignores scaling factors for lossless JPEGs, so an oversized lossless image
// cannot be downscale-decoded. It must be classified as "too large" up front instead of
// letting the decoder fail with a generic "decode error" message.
static bool IsLosslessJPEG(const unsigned char* pBuffer, int sizebytes) {
	if (pBuffer == NULL || sizebytes < 4) {
		return false;
	}
	int nPos = 0;
	while (nPos + 1 < sizebytes) {
		if (pBuffer[nPos] != 0xFF) {
			break; // entropy-coded data reached (or corrupt marker)
		}
		unsigned char nMarker = pBuffer[nPos + 1];
		if (nMarker == 0xFF) { // fill byte
			nPos++;
			continue;
		}
		// Markers without a length field: TEM, RST0-7, SOI, EOI
		if (nMarker == 0x01 || nMarker == 0xD8 || nMarker == 0xD9 || (nMarker >= 0xD0 && nMarker <= 0xD7)) {
			if (nMarker == 0xD9) {
				break; // EOI reached without a SOF marker
			}
			nPos += 2;
			continue;
		}
		if (nMarker == 0xDA) {
			break; // SOS: entropy-coded data follows
		}
		// Lossless frame markers (SOF3, SOF7, SOF11, SOF15)
		if (nMarker == 0xC3 || nMarker == 0xC7 || nMarker == 0xCB || nMarker == 0xCF) {
			return true;
		}
		if (nPos + 3 >= sizebytes) {
			break;
		}
		int nSegLen = (pBuffer[nPos + 2] << 8) | pBuffer[nPos + 3];
		if (nSegLen < 2) {
			break; // corrupt marker length
		}
		nPos += 2 + nSegLen;
	}
	return false;
}

void * TurboJpeg::ReadImage(int &width,
					   int &height,
					   int &nchannels,
					   TJSAMP &chromoSubsampling,
					   bool &outOfMemory,
					   const void *buffer,
					   int sizebytes,
					   int *pScaleDenom,
					   int nScreenWidth,
					   int nScreenHeight)
{
	outOfMemory = false;
	width = height = 0;
	nchannels = 3;
	chromoSubsampling = TJSAMP_420;
	if (pScaleDenom != NULL) {
		*pScaleDenom = 1;
	}

	tjhandle hDecoder = tj3Init(TJINIT_DECOMPRESS);
	if (hDecoder == NULL) {
		return NULL;
	}
	if (CSettingsProvider::This().FastJPEGDecode()) {
		tj3Set(hDecoder, TJPARAM_FASTDCT, 1);
		// Fast upsampling (~1.25x faster, negligible quality loss at typical quality)
		tj3Set(hDecoder, TJPARAM_FASTUPSAMPLE, 1);
	}

	unsigned char* pPixelData = NULL;
	int nSubSampling;
	int nResult = tj3DecompressHeader(hDecoder, (unsigned char*)buffer, sizebytes);
	if (nResult == 0) {
		width = tj3Get(hDecoder, TJPARAM_JPEGWIDTH);
		height = tj3Get(hDecoder, TJPARAM_JPEGHEIGHT);
		nSubSampling = tj3Get(hDecoder, TJPARAM_SUBSAMP);
		chromoSubsampling = (TJSAMP)nSubSampling;
		if (chromoSubsampling == TJSAMP_UNKNOWN) {
			chromoSubsampling = TJSAMP_420;
		}

		int nScaledWidth = width;
		int nScaledHeight = height;
		tjscalingfactor scalingFactor = TJUNSCALED;
		bool bLossless = IsLosslessJPEG((const unsigned char*)buffer, sizebytes);
		bool bOversized = abs((double)width * height) > MAX_IMAGE_PIXELS ||
			width > MAX_IMAGE_DIMENSION || height > MAX_IMAGE_DIMENSION;
		// Lossless JPEGs ignore the scaling factor in libjpeg-turbo and would fail with a
		// generic decode error if scaled, so they are never downscale-decoded.
		// Fast fit-to-screen (extreme speed) mode: downscale images that exceed the screen
		// to a 1/2^n size fitting within the screen bounds (aspect ratio preserved).
		bool bFastFit = nScreenWidth > 0 && nScreenHeight > 0 &&
			CSettingsProvider::This().FastFitScreenDecode() && !bLossless &&
			(width > nScreenWidth || height > nScreenHeight);
		if (bFastFit) {
			int nMaxDenom = CSettingsProvider::This().OversizedDownscaleMaxFactor();
			scalingFactor = FindScreenFitFactor(width, height, nScreenWidth, nScreenHeight, nMaxDenom);
		} else if (bOversized && CSettingsProvider::This().OversizedDownscaleDecode() && !bLossless) {
			// Oversized image. Either downscale-decode it (when enabled) or refuse to load.
			int nMaxDenom = CSettingsProvider::This().OversizedDownscaleMaxFactor();
			scalingFactor = FindDownscaleFactor(width, height, nMaxDenom);
		}
		if (scalingFactor.num != scalingFactor.denom) {
			nScaledWidth = TJSCALED(width, scalingFactor);
			nScaledHeight = TJSCALED(height, scalingFactor);
			tj3SetScalingFactor(hDecoder, scalingFactor);
			if (pScaleDenom != NULL) {
				*pScaleDenom = scalingFactor.denom;
			}
		}

		if (abs((double)nScaledWidth * nScaledHeight) <= MAX_IMAGE_PIXELS &&
			nScaledWidth <= MAX_IMAGE_DIMENSION && nScaledHeight <= MAX_IMAGE_DIMENSION) {
			pPixelData = new(std::nothrow) unsigned char[TJPAD(nScaledWidth * 3) * nScaledHeight];
			if (pPixelData != NULL) {
				nResult = tj3Decompress8(hDecoder, (unsigned char*)buffer, sizebytes,
					pPixelData, TJPAD(nScaledWidth * 3), TJPF_BGR);
				if (nResult != 0) {
					delete[] pPixelData;
					pPixelData = NULL;
				} else {
					width = nScaledWidth;
					height = nScaledHeight;
				}
			} else {
				outOfMemory = true;
			}
		} else {
			// Could not downscale enough (practically never happens: 1/8 is always within limits).
			outOfMemory = true;
		}
	}

	tj3Destroy(hDecoder);

	return pPixelData;
}

void * TurboJpeg::Compress(const void *source,
					  int width,
					  int height,
					  int &len,
					  bool &outOfMemory,
					  int quality)
{
	outOfMemory = false;
	len = 0;
	tjhandle hEncoder = tj3Init(TJINIT_COMPRESS);
	if (hEncoder == NULL) {
		return NULL;
	}

	tj3Set(hEncoder, TJPARAM_SUBSAMP, TJSAMP_420);
	tj3Set(hEncoder, TJPARAM_QUALITY, quality);

	unsigned char* pJPEGCompressed = NULL;
	size_t nCompressedLen = 0;
	int nResult = tj3Compress8(hEncoder, (unsigned char*)source, width,
		TJPAD(width * 3), height, TJPF_BGR, &pJPEGCompressed, &nCompressedLen);
	if (nResult != 0) {
		if (pJPEGCompressed != NULL) {
			tj3Free(pJPEGCompressed);
			pJPEGCompressed = NULL;
		} else {
			outOfMemory = true;
		}
	}

	if (nCompressedLen > INT_MAX) {
		tj3Free(pJPEGCompressed);
		tj3Destroy(hEncoder);
		outOfMemory = true;
		return NULL;
	}

	len = (int)nCompressedLen;

	tj3Destroy(hEncoder);

	return pJPEGCompressed;
}
