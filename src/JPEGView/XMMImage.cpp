#include "StdAfx.h"
#include "XMMImage.h"
#include "Helpers.h"
#include "SettingsProvider.h"
#include "LookupTables.h"	// sRGB8_LinRGB12[] / LinRGB12_sRGB8[] for linear light resampling

CXMMImage::CXMMImage(int nWidth, int nHeight, int padding) {
	Init(nWidth, nHeight, false, padding);
}

CXMMImage::CXMMImage(int nWidth, int nHeight, bool bPadHeight, int padding) {
	Init(nWidth, nHeight, bPadHeight, padding);
}

CXMMImage::CXMMImage(int nWidth, int nHeight, int nFirstX, int nLastX, int nFirstY, int nLastY, 
	const void* pDIB, int nChannels, int padding) {
	int nSectionWidth = nLastX - nFirstX + 1;
	int nSectionHeight = nLastY - nFirstY + 1;
	Init(nSectionWidth, nSectionHeight, false, padding);

	if (m_pMemory != NULL) {
		int nSrcLineWidthPadded = Helpers::DoPadding(nWidth * nChannels, 4);
		const uint8* pSrc = (uint8*)pDIB + (long long)nFirstY*(long long)nSrcLineWidthPadded + (long long)nFirstX*(long long)nChannels;
		if (m_bLinear) {
			// Linear light: convert 8 bit sRGB samples to 12 bit linear values stored as float
			float* pDst = (float*) m_pMemory;
			for (int j = 0; j < nSectionHeight; j++) {
				if (nChannels == 4) {
					for (int i = 0; i < nSectionWidth; i++) {
						uint32 sourcePixel = ((uint32*)pSrc)[i];
						int d = i;
						pDst[d] = (float)sRGB8_LinRGB12[sourcePixel & 0xFF];
						d += m_nPaddedWidth;
						pDst[d] = (float)sRGB8_LinRGB12[(sourcePixel >> 8) & 0xFF];
						d += m_nPaddedWidth;
						pDst[d] = (float)sRGB8_LinRGB12[(sourcePixel >> 16) & 0xFF];
					}
				} else {
					for (int i = 0; i < nSectionWidth; i++) {
						int s = i*3;
						int d = i;
						pDst[d] = (float)sRGB8_LinRGB12[pSrc[s]];
						d += m_nPaddedWidth;
						pDst[d] = (float)sRGB8_LinRGB12[pSrc[s+1]];
						d += m_nPaddedWidth;
						pDst[d] = (float)sRGB8_LinRGB12[pSrc[s+2]];
					}
				}
				pDst += 3*m_nPaddedWidth;
				pSrc += nSrcLineWidthPadded;
			}
		} else {
			uint16* pDst = (unsigned short*) m_pMemory;
			for (int j = 0; j < nSectionHeight; j++) {
				if (nChannels == 4) {
					for (int i = 0; i < nSectionWidth; i++) {
						uint32 sourcePixel = ((uint32*)pSrc)[i];
						int d = i;
						uint32 nBlue = sourcePixel & 0xFF;
						uint32 nGreen = (sourcePixel >> 8) & 0xFF;
						uint32 nRed = (sourcePixel >> 16) & 0xFF;
						// The commented out code actually is worse than the simple shift for precision of the fixed point arithmetic
						pDst[d] = ((uint16)nBlue << 6); // ((((uint16)nBlue) << 8) + nBlue) >> 2;
						d += m_nPaddedWidth;
						pDst[d] = ((uint16)nGreen << 6); //((((uint16) nGreen) << 8) + nGreen) >> 2;
						d += m_nPaddedWidth;
						pDst[d] = ((uint16)nRed << 6); //((((uint16) nRed) << 8) + nRed) >> 2;
					}
				} else {
					for (int i = 0; i < nSectionWidth; i++) {
						int s = i*3;
						int d = i;
						pDst[d] = ((uint16)pSrc[s] << 6);//((((uint16) pSrc[s]) << 8) + pSrc[s]) >> 2;
						d += m_nPaddedWidth;
						pDst[d] = ((uint16)pSrc[s+1] << 6);//((((uint16) pSrc[s+1]) << 8) + pSrc[s+1]) >> 2;
						d += m_nPaddedWidth;
						pDst[d] = ((uint16)pSrc[s+2] << 6);//((((uint16) pSrc[s+2]) << 8) + pSrc[s+2]) >> 2;
					}
				}
				pDst += 3*m_nPaddedWidth;
				pSrc += nSrcLineWidthPadded;
			}
		}
	}
}

CXMMImage::~CXMMImage(void) {
	if (m_pMemory != NULL) {
		// free the memory pages
		::VirtualFree(m_pMemory, 0, MEM_RELEASE);
		m_pMemory = NULL;
	}
}

void* CXMMImage::ConvertToDIBRGBA() const {
	if (m_pMemory == NULL) {
		return NULL;
	}
	uint8* pDIB = new uint8[m_nWidth*4 * m_nHeight];
	
	uint8* pDst = pDIB;
	if (m_bLinear) {
		// Convert 12 bit linear float samples back to 8 bit sRGB
		const float* pSrc = (const float*) m_pMemory;
		for (int j = 0; j < m_nHeight; j++) {
			for (int i = 0; i < m_nWidth; i++) {
				int d = i*4;
				int s = i;
				pDst[d] = LinRGB12_sRGB8[(int)(pSrc[s])];
				s += m_nPaddedWidth;
				pDst[d+1] = LinRGB12_sRGB8[(int)(pSrc[s])];
				s += m_nPaddedWidth;
				pDst[d+2] = LinRGB12_sRGB8[(int)(pSrc[s])];
				pDst[d+3] = 0xFF;
			}
			pSrc += 3*m_nPaddedWidth;
			pDst += m_nWidth*4;
		}
		return pDIB;
	}

	uint16* pSrc = (uint16*) m_pMemory;
	for (int j = 0; j < m_nHeight; j++) {
		for (int i = 0; i < m_nWidth; i++) {
			int d = i*4;
			int s = i;
			pDst[d] = (uint8)(pSrc[s] >> 6);
			s += m_nPaddedWidth;
			pDst[d+1] = (uint8)(pSrc[s] >> 6);
			s += m_nPaddedWidth;
			pDst[d+2] = (uint8)(pSrc[s] >> 6);
			pDst[d+3] = 0xFF;
		}
		pSrc += 3*m_nPaddedWidth;
		pDst += m_nWidth*4;
	}

	return pDIB;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Private
/////////////////////////////////////////////////////////////////////////////////////////

void CXMMImage::Init(int nWidth, int nHeight, bool bPadHeight, int padding) {
	// Must be known before GetMemSize() is called below
	m_bLinear = CSettingsProvider::This().LinearLightResampling();
	// pad scanlines
	m_nPaddedWidth = Helpers::DoPadding(nWidth, padding);
	if (bPadHeight) {
		m_nPaddedHeight = Helpers::DoPadding(nHeight, padding);
	} else {
		m_nPaddedHeight = nHeight;
	}
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	int nMemSize = GetMemSize();

	// Allocate memory aligned on page boundaries
	m_pMemory = ::VirtualAlloc(
						NULL,	  // let the call determine the start address
						nMemSize, // the size
						MEM_RESERVE | MEM_COMMIT,	// I want that memory, now
						PAGE_READWRITE);			// need both read and write
}
