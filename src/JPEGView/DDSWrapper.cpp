#include "stdafx.h"
#include "DDSWrapper.h"
#include "Helpers.h"
#include "SettingsProvider.h"
#include <new>

#ifdef _WIN64

#include "DirectXTex.h"

CJPEGImage* DDSReader::ReadImage(LPCTSTR strFileName, bool& bOutOfMemory) {
	CJPEGImage* Image = NULL;
	try {
		// Load DDS file; handles all DXTC/BCn, legacy and uncompressed formats.
		DirectX::ScratchImage ddsImage;
		HRESULT hr = DirectX::LoadFromDDSFile(reinterpret_cast<const wchar_t*>(strFileName),
			DirectX::DDS_FLAGS_NONE, nullptr, ddsImage);
		if (FAILED(hr)) {
			return NULL;
		}

		const DirectX::Image* base = ddsImage.GetImage(0, 0, 0);
		if (base == NULL) {
			return NULL;
		}

		// Decompress block-compressed formats (BC1-BC7) into uncompressed RGBA.
		const DirectX::Image* srcImage = base;
		DirectX::ScratchImage decompressed;
		if (DirectX::IsCompressed(base->format)) {
			hr = DirectX::Decompress(*base, DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
			if (FAILED(hr)) {
				return NULL;
			}
			srcImage = decompressed.GetImage(0, 0, 0);
			if (srcImage == NULL) {
				return NULL;
			}
		}

		// Convert to 32 bpp BGRA, the internal pixel layout used by JPEGView.
		DirectX::ScratchImage bgra;
		hr = DirectX::Convert(*srcImage, DXGI_FORMAT_B8G8R8A8_UNORM,
			DirectX::TEX_FILTER_DEFAULT, 0.5f, bgra);
		if (FAILED(hr)) {
			return NULL;
		}

		const DirectX::Image* finalImage = bgra.GetImage(0, 0, 0);
		if (finalImage == NULL) {
			return NULL;
		}

		const size_t nWidth = finalImage->width;
		const size_t nHeight = finalImage->height;
		const size_t nRowSize = nWidth * 4;
		unsigned char* pPixelData = new (std::nothrow) unsigned char[nHeight * nRowSize];
		if (pPixelData == NULL) {
			bOutOfMemory = true;
			return NULL;
		}

		const unsigned char* pSrc = finalImage->pixels;
		const size_t rowPitch = finalImage->rowPitch;
		for (size_t row = 0; row < nHeight; row++) {
			memcpy(pPixelData + row * nRowSize, pSrc + row * rowPitch, nRowSize);
		}

		// JPEGView display has no per-pixel alpha: composite alpha over the
		// configured transparency background (consistent with PSD handling).
		if (!bgra.IsAlphaAllOpaque()) {
			uint32* pImage32 = reinterpret_cast<uint32*>(pPixelData);
			COLORREF backgroundColor = CSettingsProvider::This().ColorTransparency();
			for (size_t i = 0; i < nWidth * nHeight; i++) {
				*pImage32++ = Helpers::AlphaBlendBackground(*pImage32, backgroundColor);
			}
		}

		Image = new CJPEGImage(static_cast<int>(nWidth), static_cast<int>(nHeight),
			pPixelData, NULL, 4, 0, IF_DDS, false, 0, 1, 0);
	} catch (std::bad_alloc&) {
		bOutOfMemory = true;
		if (Image != NULL) { delete Image; Image = NULL; }
	} catch (...) {
		if (Image != NULL) { delete Image; Image = NULL; }
	}
	return Image;
}

#endif // _WIN64
