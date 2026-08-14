#pragma once

#include "ImageProcessingTypes.h"
#include <windows.h>

// Signed rational number: numerator/denominator
class SignedRational {
public:
	SignedRational(int num, int denom) { Numerator = num; Denominator = denom; }
	int Numerator;
	int Denominator;
};

// Unsigned rational number: numerator/denominator
class Rational {
public:
	Rational(unsigned int num, unsigned int denom) { Numerator = num; Denominator = denom; }
	unsigned int Numerator;
	unsigned int Denominator;
};

class GPSCoordinate {
public:
	GPSCoordinate(LPCTSTR reference, double degrees, double minutes, double seconds) {
		m_sReference = CString(reference);
		if (minutes == 0.0 && seconds == 0.0) {
			minutes = 60 * abs(degrees - (int)degrees);
			degrees = (int)degrees;
		}
		if (seconds == 0.0) {
			seconds = 60 * abs(minutes - (int)minutes);
			minutes = (int)minutes;
		}
		Degrees = degrees;
		Minutes = minutes;
		Seconds = seconds;
	}
	LPCTSTR GetReference() { return m_sReference; }
	double Degrees;
	double Minutes;
	double Seconds;
private:
	CString m_sReference;
};

// Reads and parses the EXIF data of JPEG images (parsing is done by the exiv2 library)
class CEXIFReader {
public:
	// The pApp1Block must point to the APP1 block of the EXIF data, including the APP1 block marker
	// The class does not take ownership of the memory (no copy made), thus the APP1 block must not be deleted
	// while the EXIF reader class is deleted.
	CEXIFReader(void* pApp1Block, EImageFormat eImageFormat);
	~CEXIFReader(void);

	// Parse date string in the EXIF date/time format
	static bool ParseDateString(SYSTEMTIME & date, const CString& str);

public:
	// Camera model, image comment and description. The returned pointers are valid while the EXIF reader is not deleted.
	LPCTSTR GetCameraModel() { return m_sModel; }
	LPCTSTR GetUserComment() { return m_sUserComment; }
	LPCTSTR GetImageDescription() { return m_sImageDescription; }
	LPCTSTR GetSoftware() { return m_sSoftware; }
	bool GetCameraModelPresent() { return !m_sModel.IsEmpty(); }
	bool GetSoftwarePresent() { return !m_sSoftware.IsEmpty(); }
	// Date-time the picture was taken
	const SYSTEMTIME& GetAcquisitionTime() { return m_acqDate; }
	bool GetAcquisitionTimePresent() { return m_acqDate.wYear > 1600; }
	// Date-time the picture was saved/modified (used by editing software)
	const SYSTEMTIME& GetDateTime() { return m_dateTime; }
	bool GetDateTimePresent() { return m_dateTime.wYear > 1600; }
	// Exposure time
	const Rational& GetExposureTime() { return m_exposureTime; }
	bool GetExposureTimePresent() { return m_exposureTime.Denominator != 0; }
	// Exposure bias
	double GetExposureBias() { return m_dExposureBias; }
	bool GetExposureBiasPresent() { return m_dExposureBias != UNKNOWN_DOUBLE_VALUE; }
	// Flag if flash fired
	bool GetFlashFired() { return m_bFlashFired; }
	bool GetFlashFiredPresent() { return m_bFlashFlagPresent; }
	// Focal length (mm)
	double GetFocalLength() { return m_dFocalLength; }
	bool GetFocalLengthPresent() { return m_dFocalLength != UNKNOWN_DOUBLE_VALUE; }
	// F-Number
	double GetFNumber() { return m_dFNumber; }
	bool GetFNumberPresent() { return m_dFNumber != UNKNOWN_DOUBLE_VALUE; }
	// ISO speed value
	int GetISOSpeed() { return m_nISOSpeed; }
	bool GetISOSpeedPresent() { return m_nISOSpeed > 0; }
	// Image orientation as detected by sensor, coding according EXIF standard (thus no angle in degrees)
	int GetImageOrientation() { return m_nImageOrientation; }
	bool ImageOrientationPresent() { return m_nImageOrientation > 0; }
	// Thumbnail image information
	bool HasJPEGCompressedThumbnail() { return m_bHasJPEGCompressedThumbnail; }
	int GetJPEGThumbStreamLen() { return m_nJPEGThumbStreamLen; }
	int GetThumbnailWidth() { return m_nThumbWidth; }
	int GetThumbnailHeight() { return m_nThumbHeight; }
	// GPS information
	bool IsGPSInformationPresent() { return m_pLatitude != NULL && m_pLongitude != NULL; }
	bool IsGPSAltitudePresent() { return m_dAltitude != UNKNOWN_DOUBLE_VALUE; }
	GPSCoordinate* GetGPSLatitude() { return m_pLatitude; }
	GPSCoordinate* GetGPSLongitude() { return m_pLongitude; }
	double GetGPSAltitude() { return m_dAltitude; }

	// Sets the image orientation to given value (if tag was present in input stream).
	// Writes to the APP1 block passed in constructor.
	void WriteImageOrientation(int nOrientation);
	
	// Updates an existing JPEG compressed thumbnail image by given JPEG stream (SOI stripped)
	// Writes to the APP1 block passed in constructor. Make sure that enough memory is allocated for this APP1
	// block to hold the additional thumbnail data.
	void UpdateJPEGThumbnail(unsigned char* pJPEGStream, int nStreamLen, int nEXIFBlockLenCorrection, CSize sizeThumb);

	// Delete the thumbnail image
	// Writes to the APP1 block passed in constructor.
	void DeleteThumbnail();

	// Returns the current total length of the EXIF APP1 block (including marker and length bytes).
	// After a write operation (WriteImageOrientation/UpdateJPEGThumbnail/DeleteThumbnail) this
	// reflects the re-encoded block length and triggers the re-encode if it is still pending.
	int GetEXIFSize();

public:
	// unknown double value
	static double UNKNOWN_DOUBLE_VALUE;

private:
	CString m_sModel;
	CString m_sUserComment;
	CString m_sImageDescription;
	CString m_sSoftware;
	SYSTEMTIME m_acqDate;
	SYSTEMTIME m_dateTime;
	Rational m_exposureTime;
	double m_dExposureBias;
	bool m_bFlashFired;
	bool m_bFlashFlagPresent;
	double m_dFocalLength;
	double m_dFNumber;
	int m_nISOSpeed;
	int m_nImageOrientation;
	bool m_bHasJPEGCompressedThumbnail;
	int m_nThumbWidth;
	int m_nThumbHeight;
	int m_nJPEGThumbStreamLen;
	GPSCoordinate* m_pLatitude;
	GPSCoordinate* m_pLongitude;
	double m_dAltitude;

	// exiv2 driven state
	void* m_pExifData;   // Points to Exiv2::ExifData instance (NULL if not parsed / exiv2 unavailable)
	uint8* m_pApp1;      // Points to the APP1 block passed to the constructor
	int m_nApp1Size;     // Original total length of the APP1 block (including marker and length bytes)
	int m_nEXIFSize;     // Current total length of the APP1 block (may differ from m_nApp1Size after re-encode)
	int m_nThumbOffset;  // Offset of the compressed thumbnail SOI relative to the TIFF data start (Exif.Thumbnail.JPEGInterchangeFormat)
	int m_nByteOrder;    // Original TIFF byte order: 1 = little endian, 2 = big endian (Exiv2::ByteOrder), 0 = unknown
	bool m_bParsed;      // true if exiv2 successfully parsed the block
	bool m_bDirty;       // true if the parsed metadata was modified and needs a re-encode

	EImageFormat m_eImageFormat;

	void ParseWithExiv2();
	void Reencode();     // Serializes the modified metadata back to the APP1 block
};
