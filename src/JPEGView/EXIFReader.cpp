#include "StdAfx.h"
#include "EXIFReader.h"
#include "ImageProcessingTypes.h"
#include "Helpers.h"

// ---------------------------------------------------------------------------
// exiv2 based EXIF parsing. The JPEGView release builds define JPEGVIEW_EXIV2
// (x64 Release). In configurations where the static exiv2 library is not
// linked, the reader degrades to an empty implementation (all fields return
// their default values and write operations become no-ops).
// ---------------------------------------------------------------------------
#ifdef JPEGVIEW_EXIV2
// The Windows headers define min/max macros which break exiv2's use of
// std::numeric_limits<T>::min()/max() in value.hpp. EXIFReader.cpp itself
// does not rely on the min/max macros.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "exiv2/exif.hpp"
#include "exiv2/tags.hpp" // defines ExifKey
#include "exiv2/types.hpp"
#include "exiv2/metadatum.hpp"
#include "exiv2/value.hpp"
#endif

double CEXIFReader::UNKNOWN_DOUBLE_VALUE = 283740261.192864;

// The caller (SaveImage.cpp) guarantees that at least this many bytes are
// available right behind the APP1 block passed to the constructor for a
// re-encode (see cnAdditionalThumbBytes in SaveImage.cpp).
#define cnMaxEXIFGrowth 32000

#ifdef JPEGVIEW_EXIV2

namespace {
	// Returns a pointer to the Exifdatum with the given key, or NULL if absent / empty.
	const Exiv2::Exifdatum* Exiv2GetDatum(const Exiv2::ExifData* pData, const char* sKey) {
		if (pData == NULL) return NULL;
		try {
			Exiv2::ExifData::const_iterator pos = pData->findKey(Exiv2::ExifKey(sKey));
			if (pos == pData->end() || pos->count() == 0) return NULL;
			return &(*pos);
		} catch (...) {
			return NULL;
		}
	}

	// Reads the UserComment value (raw bytes, first 8 bytes are the charset marker).
	void ReadExifUserComment(const Exiv2::Exifdatum& datum, CString& sOut) {
		std::unique_ptr<Exiv2::Value> pOwned = datum.getValue();
		if (pOwned == NULL) return;
		const Exiv2::Value* pVal = pOwned.get();
		size_t nSize = pVal->size();
		if (nSize <= 8) return;
		std::vector<Exiv2::byte> buf(nSize);
		pVal->copy(&buf[0], Exiv2::invalidByteOrder);
		const Exiv2::byte* pData = &buf[0];
		if (memcmp(pData, "ASCII\0\0\0", 8) == 0) {
			sOut = CString((LPCSTR)(pData + 8));
		} else if (memcmp(pData, "UNICODE\0\0", 8) == 0 || memcmp(pData, "Unicode\0\0", 8) == 0) {
			int nChars = (int)((nSize - 8) / 2);
			sOut = CString((LPCWSTR)(pData + 8), nChars);
		}
	}

	// Reads a double value from a rational tag, returns UNKNOWN_DOUBLE_VALUE if absent.
	double Exiv2RationalToDouble(const Exiv2::Exifdatum* pDatum) {
		if (pDatum == NULL) return CEXIFReader::UNKNOWN_DOUBLE_VALUE;
		Exiv2::Rational r = pDatum->toRational();
		if (r.second == 0) return CEXIFReader::UNKNOWN_DOUBLE_VALUE;
		return (double)r.first / (double)r.second;
	}

	// Reads a GPSCoordinate from a 3-rational coordinate tag plus reference tag.
	GPSCoordinate* Exiv2ReadCoordinate(const Exiv2::ExifData* pData, const char* sCoordKey, const char* sRefKey) {
		const Exiv2::Exifdatum* pCoord = Exiv2GetDatum(pData, sCoordKey);
		const Exiv2::Exifdatum* pRef = Exiv2GetDatum(pData, sRefKey);
		if (pCoord == NULL || pRef == NULL) return NULL;
		std::unique_ptr<Exiv2::Value> pCoordValue = pCoord->getValue();
		if (pCoordValue == NULL || pCoordValue->count() < 3) return NULL;
		CString sReference = CString(pRef->toString().c_str());
		Exiv2::Rational rDeg = pCoord->toRational(0);
		Exiv2::Rational rMin = pCoord->toRational(1);
		Exiv2::Rational rSec = pCoord->toRational(2);
		if (rDeg.second == 0 || rMin.second == 0 || rSec.second == 0) return NULL;
		double dDeg = (double)rDeg.first / (double)rDeg.second;
		double dMin = (double)rMin.first / (double)rMin.second;
		double dSec = (double)rSec.first / (double)rSec.second;
		return new GPSCoordinate(sReference, dDeg, dMin, dSec);
	}
}

// ---------------------------------------------------------------------------
// Parses the APP1 block with exiv2 and fills all member fields.
// ---------------------------------------------------------------------------
void CEXIFReader::ParseWithExiv2() {
	if (m_pApp1 == NULL) return;
	// Validate APP1 segment: FF E1 marker + "Exif\0\0" identifier
	if (m_pApp1[0] != 0xFF || m_pApp1[1] != 0xE1) return;
	uint32 nSegLen = ((uint32)m_pApp1[2] << 8) | m_pApp1[3];
	if (nSegLen < 16) return;
	if (memcmp(m_pApp1 + 4, "Exif\0\0", 6) != 0) return;
	m_nApp1Size = (int)nSegLen + 2;
	m_nEXIFSize = m_nApp1Size;

	uint8* pTIFF = m_pApp1 + 10;
	size_t nTIFFSize = (size_t)(m_nApp1Size - 10);

	try {
		Exiv2::ExifData* pData = new Exiv2::ExifData();
		Exiv2::ByteOrder byteOrder = Exiv2::ExifParser::decode(*pData, pTIFF, nTIFFSize);
		if (byteOrder == Exiv2::invalidByteOrder) {
			delete pData;
			return;
		}
		m_pExifData = pData;
		m_nByteOrder = (int)byteOrder;
	} catch (...) {
		return;
	}
	m_bParsed = true;
	const Exiv2::ExifData* pData = (const Exiv2::ExifData*)m_pExifData;

	// Camera model = Make (first word, if not already leading the model) + " " + Model
	{
		CString sMake, sModel;
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Image.Make");
		if (pDatum != NULL) sMake = CString(pDatum->toString().c_str());
		pDatum = Exiv2GetDatum(pData, "Exif.Image.Model");
		if (pDatum != NULL) sModel = CString(pDatum->toString().c_str());
		if (!sMake.IsEmpty() && !sModel.IsEmpty()) {
			if (sModel.Left(1) != sMake.Left(1)) {
				int nPos = sMake.Find(_T(' '));
				if (nPos >= 0) sMake = sMake.Left(nPos);
			}
			m_sModel = sMake + _T(" ") + sModel;
		} else if (!sModel.IsEmpty()) {
			m_sModel = sModel;
		}
	}
	// Image description (usually UTF-8 encoded)
	{
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Image.ImageDescription");
		if (pDatum != NULL) {
			std::unique_ptr<Exiv2::Value> pOwned = pDatum->getValue();
			if (pOwned != NULL) {
				const Exiv2::Value* pVal = pOwned.get();
				size_t nSize = pVal->size();
				if (nSize > 0) {
					std::vector<Exiv2::byte> buf(nSize);
					pVal->copy(&buf[0], Exiv2::invalidByteOrder);
					m_sImageDescription = Helpers::TryConvertFromUTF8(&buf[0], (int)nSize);
				}
			}
		}
	}
	// Software
	{
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Image.Software");
		if (pDatum != NULL) m_sSoftware = CString(pDatum->toString().c_str());
	}
	// Date-time the picture was saved/modified
	{
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Image.DateTime");
		if (pDatum != NULL) ParseDateString(m_dateTime, CString(pDatum->toString().c_str()));
	}
	// Date-time the picture was taken
	{
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Photo.DateTimeOriginal");
		if (pDatum != NULL) ParseDateString(m_acqDate, CString(pDatum->toString().c_str()));
	}
	// Exposure / flash / focal length / f-number / ISO / user comment
	{
		m_exposureTime = Rational(0, 0);
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Photo.ExposureTime");
		if (pDatum != NULL) {
			Exiv2::Rational r = pDatum->toRational();
			m_exposureTime = Rational(r.first, r.second);
		}
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.ExposureBiasValue");
		if (pDatum != NULL) {
			double d = Exiv2RationalToDouble(pDatum);
			if (d != UNKNOWN_DOUBLE_VALUE) m_dExposureBias = d;
		}
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.Flash");
		if (pDatum != NULL) {
			m_bFlashFlagPresent = true;
			m_bFlashFired = (pDatum->toInt64() & 1) != 0;
		}
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.FocalLength");
		if (pDatum != NULL) {
			double d = Exiv2RationalToDouble(pDatum);
			if (d != UNKNOWN_DOUBLE_VALUE) m_dFocalLength = d;
		}
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.FNumber");
		if (pDatum != NULL) {
			double d = Exiv2RationalToDouble(pDatum);
			if (d != UNKNOWN_DOUBLE_VALUE) m_dFNumber = d;
		}
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.ISOSpeedRatings");
		if (pDatum == NULL) pDatum = Exiv2GetDatum(pData, "Exif.Photo.ISOSpeed");
		if (pDatum != NULL) m_nISOSpeed = (int)pDatum->toInt64();
		// User comment (Samsung Galaxy writes a useless "User comments" into each JPEG - ignore it)
		pDatum = Exiv2GetDatum(pData, "Exif.Photo.UserComment");
		if (pDatum != NULL) ReadExifUserComment(*pDatum, m_sUserComment);
		if (m_sUserComment == _T("User comments")) m_sUserComment = _T("");
	}
	// Image orientation (the JXL/HEIF/AVIF decoders apply orientation themselves)
	if (m_eImageFormat != IF_JXL && m_eImageFormat != IF_HEIF && m_eImageFormat != IF_AVIF) {
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Image.Orientation");
		if (pDatum != NULL) m_nImageOrientation = (int)pDatum->toInt64();
	}
	// GPS information
	{
		m_pLatitude = Exiv2ReadCoordinate(pData, "Exif.GPSInfo.GPSLatitude", "Exif.GPSInfo.GPSLatitudeRef");
		m_pLongitude = Exiv2ReadCoordinate(pData, "Exif.GPSInfo.GPSLongitude", "Exif.GPSInfo.GPSLongitudeRef");
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.GPSInfo.GPSAltitude");
		if (pDatum != NULL) {
			double d = Exiv2RationalToDouble(pDatum);
			if (d != UNKNOWN_DOUBLE_VALUE) {
				const Exiv2::Exifdatum* pAltRef = Exiv2GetDatum(pData, "Exif.GPSInfo.GPSAltitudeRef");
				if (pAltRef != NULL && pAltRef->toInt64() == 1) d = -d;
				m_dAltitude = d;
			}
		}
	}
	// Thumbnail
	{
		const Exiv2::Exifdatum* pDatum = Exiv2GetDatum(pData, "Exif.Thumbnail.Compression");
		if (pDatum != NULL && pDatum->toInt64() == 6) {
			// JPEG compressed thumbnail
			const Exiv2::Exifdatum* pOff = Exiv2GetDatum(pData, "Exif.Thumbnail.JPEGInterchangeFormat");
			const Exiv2::Exifdatum* pLen = Exiv2GetDatum(pData, "Exif.Thumbnail.JPEGInterchangeFormatLength");
			if (pOff != NULL && pLen != NULL) {
				uint32 nOffset = (uint32)pOff->toInt64();
				uint32 nBytes = (uint32)pLen->toInt64();
				if (nOffset + nBytes <= (uint32)(m_nApp1Size - 10)) {
					uint8* pSOI = m_pApp1 + 10 + nOffset;
					if (pSOI[0] == 0xFF && pSOI[1] == 0xD8) {
						m_bHasJPEGCompressedThumbnail = true;
						m_nJPEGThumbStreamLen = (int)nBytes;
						m_nThumbOffset = (int)nOffset;
						uint8* pSOF = (uint8*)Helpers::FindJPEGMarker(pSOI, (int)nBytes, 0xC0);
						if (pSOF != NULL) {
							m_nThumbHeight = (pSOF[5] << 8) + pSOF[6];
							m_nThumbWidth = (pSOF[7] << 8) + pSOF[8];
						}
					}
				}
			}
		} else {
			// Uncompressed thumbnail - read width/height tags
			const Exiv2::Exifdatum* pW = Exiv2GetDatum(pData, "Exif.Thumbnail.ImageWidth");
			const Exiv2::Exifdatum* pH = Exiv2GetDatum(pData, "Exif.Thumbnail.ImageLength");
			if (pW != NULL) m_nThumbWidth = (int)pW->toInt64();
			if (pH != NULL) m_nThumbHeight = (int)pH->toInt64();
		}
	}
}

// ---------------------------------------------------------------------------
// Serializes the (modified) ExifData back into the APP1 block.
// Prefers a non-intrusive in-place update (block size unchanged); falls back to
// a full re-encode into the growth budget the caller reserved.
// ---------------------------------------------------------------------------
void CEXIFReader::Reencode() {
	if (!m_bParsed || m_pExifData == NULL || m_pApp1 == NULL) return;
	m_bDirty = false;
	try {
		Exiv2::ExifData* pData = (Exiv2::ExifData*)m_pExifData;
		uint8* pTIFF = m_pApp1 + 10;
		size_t nTIFFSize = (size_t)(m_nApp1Size - 10);
		Exiv2::ByteOrder byteOrder = (m_nByteOrder == 2) ? Exiv2::bigEndian : Exiv2::littleEndian;

		Exiv2::Blob blob;
		Exiv2::WriteMethod wm = Exiv2::ExifParser::encode(blob, pTIFF, nTIFFSize, byteOrder, *pData);
		if (wm == Exiv2::wmNonIntrusive) {
			// Updated in place, block size unchanged
			m_nEXIFSize = m_nApp1Size;
			return;
		}
		// Intrusive: full re-encode. The blob is a plain TIFF structure (no "Exif\0\0" header).
		int nNewLen = 10 + (int)blob.size();
		if (nNewLen <= m_nApp1Size + cnMaxEXIFGrowth && nNewLen < 65536) {
			memcpy(m_pApp1 + 10, &blob[0], blob.size());
			m_pApp1[2] = (uint8)((nNewLen - 2) >> 8);
			m_pApp1[3] = (uint8)((nNewLen - 2) & 0xFF);
			m_nEXIFSize = nNewLen;
		} else {
			// The block grew beyond the reserved budget. Try a full re-encode
			// into the original block size; if that does not fit either, leave
			// the original block untouched (safe fallback).
			Exiv2::Blob blob2;
			Exiv2::ExifParser::encode(blob2, byteOrder, *pData);
			int nLen2 = 10 + (int)blob2.size();
			if (nLen2 <= m_nApp1Size && nLen2 < 65536) {
				memcpy(m_pApp1 + 10, &blob2[0], blob2.size());
				m_pApp1[2] = (uint8)((nLen2 - 2) >> 8);
				m_pApp1[3] = (uint8)((nLen2 - 2) & 0xFF);
				m_nEXIFSize = nLen2;
			} else {
				m_nEXIFSize = m_nApp1Size;
			}
		}
	} catch (...) {
		m_nEXIFSize = m_nApp1Size;
	}
}

#endif // JPEGVIEW_EXIV2

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
CEXIFReader::CEXIFReader(void* pApp1Block, EImageFormat eImageFormat)
	: m_acqDate{}, m_dateTime{}, m_exposureTime(0, 0),
	  m_dExposureBias(UNKNOWN_DOUBLE_VALUE),
	  m_bFlashFired(false), m_bFlashFlagPresent(false),
	  m_dFocalLength(UNKNOWN_DOUBLE_VALUE), m_dFNumber(UNKNOWN_DOUBLE_VALUE),
	  m_nISOSpeed(0), m_nImageOrientation(0),
	  m_bHasJPEGCompressedThumbnail(false),
	  m_nThumbWidth(-1), m_nThumbHeight(-1), m_nJPEGThumbStreamLen(0),
	  m_pLatitude(NULL), m_pLongitude(NULL), m_dAltitude(UNKNOWN_DOUBLE_VALUE),
	  m_pExifData(NULL), m_pApp1((uint8*)pApp1Block),
	  m_nApp1Size(0), m_nEXIFSize(0), m_nThumbOffset(0), m_nByteOrder(0),
	  m_bParsed(false), m_bDirty(false), m_eImageFormat(eImageFormat)
{
	// Always determine the block length from the APP1 segment header, so that
	// GetEXIFSize() returns the original block size even in builds without exiv2.
	if (m_pApp1 != NULL && m_pApp1[0] == 0xFF && m_pApp1[1] == 0xE1) {
		uint32 nSegLen = ((uint32)m_pApp1[2] << 8) | m_pApp1[3];
		if (nSegLen >= 16) {
			m_nApp1Size = (int)nSegLen + 2;
			m_nEXIFSize = m_nApp1Size;
		}
	}
#ifdef JPEGVIEW_EXIV2
	ParseWithExiv2();
#endif
}

CEXIFReader::~CEXIFReader(void) {
	delete m_pLatitude;
	delete m_pLongitude;
#ifdef JPEGVIEW_EXIV2
	delete (Exiv2::ExifData*)m_pExifData;
#endif
}

bool CEXIFReader::ParseDateString(SYSTEMTIME & date, const CString& str) {
	int nYear, nMonth, nDay, nHour, nMin, nSec;
	if (_stscanf(str, _T("%d:%d:%d %d:%d:%d"), &nYear, &nMonth, &nDay, &nHour, &nMin, &nSec) == 6) {
		date.wYear = nYear;
		date.wMonth = nMonth;
		date.wDay = nDay;
		date.wHour = nHour;
		date.wMinute = nMin;
		date.wSecond = nSec;
		date.wMilliseconds = 0;
		date.wDayOfWeek = 0;
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Write operations - modify the parsed ExifData and re-encode it into the APP1 block
// ---------------------------------------------------------------------------
void CEXIFReader::WriteImageOrientation(int nOrientation) {
#ifdef JPEGVIEW_EXIV2
	if (!m_bParsed || m_pExifData == NULL) return;
	// Only meaningful for formats where JPEGView applies the EXIF orientation itself
	if (m_eImageFormat == IF_JXL || m_eImageFormat == IF_HEIF || m_eImageFormat == IF_AVIF) return;
	// Only write if the tag was present in the input stream
	if (m_nImageOrientation <= 0) return;
	Exiv2::ExifData* pData = (Exiv2::ExifData*)m_pExifData;
	Exiv2::ExifData::iterator pos = pData->findKey(Exiv2::ExifKey("Exif.Image.Orientation"));
	if (pos != pData->end()) {
		*pos = (uint16_t)nOrientation; // keep SHORT type so the re-encode stays non-intrusive
		m_nImageOrientation = nOrientation;
		m_bDirty = true;
	}
#endif
}

void CEXIFReader::UpdateJPEGThumbnail(unsigned char* pJPEGStream, int nStreamLen, int nEXIFBlockLenCorrection, CSize sizeThumb) {
#ifdef JPEGVIEW_EXIV2
	if (!m_bParsed || !m_bHasJPEGCompressedThumbnail || m_pExifData == NULL) return;
	Exiv2::ExifData* pData = (Exiv2::ExifData*)m_pExifData;
	Exiv2::ExifThumb exifThumb(*pData);
	exifThumb.setJpegThumbnail(pJPEGStream, nStreamLen);
	m_bDirty = true;
#endif
}

void CEXIFReader::DeleteThumbnail() {
#ifdef JPEGVIEW_EXIV2
	if (!m_bParsed || m_pExifData == NULL) return;
	Exiv2::ExifData* pData = (Exiv2::ExifData*)m_pExifData;
	Exiv2::ExifThumb exifThumb(*pData);
	exifThumb.erase();
	m_bHasJPEGCompressedThumbnail = false;
	m_nThumbWidth = -1;
	m_nThumbHeight = -1;
	m_nJPEGThumbStreamLen = 0;
	m_bDirty = true;
#endif
}

int CEXIFReader::GetEXIFSize() {
#ifdef JPEGVIEW_EXIV2
	if (m_bDirty) Reencode();
#endif
	return m_nEXIFSize;
}
