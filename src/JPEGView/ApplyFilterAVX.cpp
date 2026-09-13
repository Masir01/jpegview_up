#include "StdAfx.h"
#include "XMMImage.h"
#include "ResizeFilter.h"
#include "ApplyFilterAVX.h"

// This macro allows for aligned definition of a 32 byte value with initialization of the 16 components
// to a single value
#define DECLARE_ALIGNED_QQWORD(name, initializer) \
	int16 _tempVal##name[32]; \
	int16* name = (int16*)((((PTR_INTEGRAL_TYPE)&(_tempVal##name) + 31) & ~31)); \
	name[0] = name[1] = name[2] = name[3] = name[4] = name[5] = name[6] = name[7] = name[8] = name[9] = name[10] = name[11] = name[12] = name[13] = name[14] = name[15] = initializer;

#ifdef _WIN64

CXMMImage* ApplyFilter_AVX(int nSourceHeight, int nTargetHeight, int nWidth,
	int nStartY_FP, int nStartX, int nIncrementY_FP,
	const AVXFilterKernelBlock& filter,
	int nFilterOffset, const CXMMImage* pSourceImg) {

	int nStartXAligned = nStartX & ~15;
	int nEndXAligned = (nStartX + nWidth + 15) & ~15;
	CXMMImage* tempImage = new CXMMImage(nEndXAligned - nStartXAligned, nTargetHeight, 16);
	if (tempImage->AlignedPtr() == NULL) {
		delete tempImage;
		return NULL;
	}

	int nCurY = nStartY_FP;
	int nChannelLenBytes = pSourceImg->GetPaddedWidth() * sizeof(short);
	int nRowLenBytes = nChannelLenBytes * 3;
	int nNumberOfBlocksX = (nEndXAligned - nStartXAligned) >> 4;
	const uint8* pSourceStart = (const uint8*)pSourceImg->AlignedPtr() + nStartXAligned * sizeof(short);
	AVXFilterKernel** pKernelIndexStart = filter.Indices;

	DECLARE_ALIGNED_QQWORD(ONE_XMM, 16383 - 42); // 1.0 in fixed point notation, minus rounding correction

	__m256i ymm0 = *((__m256i*)ONE_XMM);
	__m256i ymm1 = _mm256_setzero_si256();
	__m256i ymm2;
	__m256i ymm3;
	__m256i ymm4 = _mm256_setzero_si256();
	__m256i ymm5 = _mm256_setzero_si256();
	__m256i ymm6 = _mm256_setzero_si256();
	__m256i ymm7;

	__m256i* pDestination = (__m256i*)tempImage->AlignedPtr();

	for (int y = 0; y < nTargetHeight; y++) {
		uint32 nCurYInt = (uint32)nCurY >> 16; // integer part of Y
		int filterIndex = y + nFilterOffset;
		AVXFilterKernel* pKernel = pKernelIndexStart[filterIndex];
		int filterLen = pKernel->FilterLen;
		int filterOffset = pKernel->FilterOffset;
		const __m256i* pFilterStart = (__m256i*)&(pKernel->Kernel);
		const __m256i* pSourceRow = (const __m256i*)(pSourceStart + ((int)nCurYInt - filterOffset) * nRowLenBytes);

		for (int x = 0; x < nNumberOfBlocksX; x++) {
			const __m256i* pSource = pSourceRow;
			const __m256i* pFilter = pFilterStart;
			ymm4 = _mm256_setzero_si256();
			ymm5 = _mm256_setzero_si256();
			ymm6 = _mm256_setzero_si256();
			for (int i = 0; i < filterLen; i++) {
				ymm7 = *pFilter;

				// the pixel data RED channel
				ymm2 = *pSource;
				ymm2 = _mm256_add_epi16(ymm2, ymm2);
				ymm2 = _mm256_mulhi_epi16(ymm2, ymm7);
				ymm2 = _mm256_add_epi16(ymm2, ymm2);
				ymm4 = _mm256_adds_epi16(ymm4, ymm2);
				pSource = (__m256i*)((uint8*)pSource + nChannelLenBytes);

				// the pixel data GREEN channel
				ymm3 = *pSource;
				ymm3 = _mm256_add_epi16(ymm3, ymm3);
				ymm3 = _mm256_mulhi_epi16(ymm3, ymm7);
				ymm3 = _mm256_add_epi16(ymm3, ymm3);
				ymm5 = _mm256_adds_epi16(ymm5, ymm3);
				pSource = (__m256i*)((uint8*)pSource + nChannelLenBytes);

				// the pixel data BLUE channel
				ymm2 = *pSource;
				ymm2 = _mm256_add_epi16(ymm2, ymm2);
				ymm2 = _mm256_mulhi_epi16(ymm2, ymm7);
				ymm2 = _mm256_add_epi16(ymm2, ymm2);
				ymm6 = _mm256_adds_epi16(ymm6, ymm2);
				pSource = (__m256i*)((uint8*)pSource + nChannelLenBytes);

				pFilter++;
			}

			// limit to range 0 (in ymm1), 16383-42 (in ymm0)
			ymm4 = _mm256_min_epi16(ymm4, ymm0);
			ymm5 = _mm256_min_epi16(ymm5, ymm0);
			ymm6 = _mm256_min_epi16(ymm6, ymm0);

			ymm1 = _mm256_setzero_si256();

			ymm4 = _mm256_max_epi16(ymm4, ymm1);
			ymm5 = _mm256_max_epi16(ymm5, ymm1);
			ymm6 = _mm256_max_epi16(ymm6, ymm1);

			// store result in blocks
			*pDestination++ = ymm4;
			*pDestination++ = ymm5;
			*pDestination++ = ymm6;

			pSourceRow++;
		};

		nCurY += nIncrementY_FP;
	};

	return tempImage;
}

// Linear light variant: one float32 sample per channel, values in [0..4095] (12 bit linear).
// 8 floats fit into an AVX register, so a block covers 8 pixels instead of 16.
CXMMImage* ApplyFilter_AVX_Linear(int nSourceHeight, int nTargetHeight, int nWidth,
	int nStartY_FP, int nStartX, int nIncrementY_FP,
	const AVXFilterKernelBlock& filter,
	int nFilterOffset, const CXMMImage* pSourceImg, bool bRoundResult) {

	int nStartXAligned = nStartX & ~7;
	int nEndXAligned = (nStartX + nWidth + 7) & ~7;
	CXMMImage* tempImage = new CXMMImage(nEndXAligned - nStartXAligned, nTargetHeight, 8);
	if (tempImage->AlignedPtr() == NULL) {
		delete tempImage;
		return NULL;
	}

	int nCurY = nStartY_FP;
	int nChannelLenBytes = pSourceImg->GetPaddedWidth() * sizeof(float);
	int nRowLenBytes = nChannelLenBytes * 3;
	int nNumberOfBlocksX = (nEndXAligned - nStartXAligned) >> 3;
	const uint8* pSourceStart = (const uint8*)pSourceImg->AlignedPtr() + nStartXAligned * sizeof(float);
	AVXFilterKernel** pKernelIndexStart = filter.Indices;

	const __m256 ymmClampMax = _mm256_set1_ps(4095.0f);
	const __m256 ymmZero = _mm256_setzero_ps();

	__m256 ymm2;
	__m256 ymm3;
	__m256 ymm4;
	__m256 ymm5;
	__m256 ymm6;
	__m256 ymm7;
	__m256* pDestination = (__m256*)tempImage->AlignedPtr();

	for (int y = 0; y < nTargetHeight; y++) {
		uint32 nCurYInt = (uint32)nCurY >> 16; // integer part of Y
		int filterIndex = y + nFilterOffset;
		AVXFilterKernel* pKernel = pKernelIndexStart[filterIndex];
		int filterLen = pKernel->FilterLen;
		int filterOffset = pKernel->FilterOffset;
		const __m256* pFilterStart = (__m256*)&(pKernel->Kernel);
		const __m256* pSourceRow = (const __m256*)(pSourceStart + ((int)nCurYInt - filterOffset) * nRowLenBytes);

		for (int x = 0; x < nNumberOfBlocksX; x++) {
			const __m256* pSource = pSourceRow;
			const __m256* pFilter = pFilterStart;
			ymm4 = _mm256_setzero_ps();
			ymm5 = _mm256_setzero_ps();
			ymm6 = _mm256_setzero_ps();
			for (int i = 0; i < filterLen; i++) {
				ymm7 = *pFilter;

				// the pixel data RED channel
				ymm2 = *pSource;
				ymm2 = _mm256_mul_ps(ymm2, ymm7);
				ymm4 = _mm256_add_ps(ymm4, ymm2);
				pSource = (__m256*)((uint8*)pSource + nChannelLenBytes);

				// the pixel data GREEN channel
				ymm3 = *pSource;
				ymm3 = _mm256_mul_ps(ymm3, ymm7);
				ymm5 = _mm256_add_ps(ymm5, ymm3);
				pSource = (__m256*)((uint8*)pSource + nChannelLenBytes);

				// the pixel data BLUE channel
				ymm2 = *pSource;
				ymm2 = _mm256_mul_ps(ymm2, ymm7);
				ymm6 = _mm256_add_ps(ymm6, ymm2);
				pSource = (__m256*)((uint8*)pSource + nChannelLenBytes);

				pFilter++;
			}

			if (bRoundResult == true) {
				// limit to the 12 bit linear range and round to integer
				ymm4 = _mm256_min_ps(ymm4, ymmClampMax);
				ymm5 = _mm256_min_ps(ymm5, ymmClampMax);
				ymm6 = _mm256_min_ps(ymm6, ymmClampMax);

				ymm4 = _mm256_max_ps(ymm4, ymmZero);
				ymm5 = _mm256_max_ps(ymm5, ymmZero);
				ymm6 = _mm256_max_ps(ymm6, ymmZero);

				ymm4 = _mm256_round_ps(ymm4, _MM_FROUND_TO_NEAREST_INT);
				ymm5 = _mm256_round_ps(ymm5, _MM_FROUND_TO_NEAREST_INT);
				ymm6 = _mm256_round_ps(ymm6, _MM_FROUND_TO_NEAREST_INT);
			}

			// store result in blocks
			*pDestination++ = ymm4;
			*pDestination++ = ymm5;
			*pDestination++ = ymm6;

			pSourceRow++;
		};

		nCurY += nIncrementY_FP;
	};

	return tempImage;
}

#endif