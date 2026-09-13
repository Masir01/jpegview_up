#pragma once

class CXMMImage;
struct AVXFilterKernelBlock;

// Used by BasicProcessing.cpp: Applies a filter using AVX. Own compilation unit to be able to compile this with AVX compiler flag.
CXMMImage* ApplyFilter_AVX(int nSourceHeight, int nTargetHeight, int nWidth,
	int nStartY_FP, int nStartX, int nIncrementY_FP,
	const AVXFilterKernelBlock& filter,
	int nFilterOffset, const CXMMImage* pSourceImg);

// Linear light variant: the source image holds float32 samples in the range [0..4095]. When
// bRoundResult is set the result is clamped to that range and rounded - this is used for the
// second filtering pass, whose output feeds the 12 bit lookup table converting back to sRGB.
CXMMImage* ApplyFilter_AVX_Linear(int nSourceHeight, int nTargetHeight, int nWidth,
	int nStartY_FP, int nStartX, int nIncrementY_FP,
	const AVXFilterKernelBlock& filter,
	int nFilterOffset, const CXMMImage* pSourceImg, bool bRoundResult);