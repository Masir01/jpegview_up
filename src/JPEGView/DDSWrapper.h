#pragma once

#include "JPEGImage.h"

class DDSReader
{
public:
	// Returns image from DDS file (x64 build only; uses DirectXTex)
	static CJPEGImage* ReadImage(LPCTSTR strFileName, bool& bOutOfMemory);
};
