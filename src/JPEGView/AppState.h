#pragma once

// AppState.h - UI / view / parameter state extracted from CMainDlg.
//
// CMainDlg kept ~90 mixed state members (zoom, rotation, offsets, processing
// parameters, transition effect, animation state, ...). They are moved into this
// dedicated struct so the main dialog no longer is a god-class holding all mutable
// view state inline. CMainDlg owns a single CAppState m_state instance and accesses
// the members as m_state.m_X. This makes the state a clearly delimited unit that
// could later be observed / snapshotted / unit-tested in isolation.
//
// The members are declared without initializers on purpose: initialization is done
// by CMainDlg's constructor (unchanged behaviour). Plain POD members (int/double/
// bool/POINT/DWORD) are left uninitialized, exactly as before.

#include "stdafx.h"
#include "ProcessParams.h"
#include "Helpers.h"

class CAppState {
public:
	// Current parameter set
	int m_nRotation; // this can only be 0, 90, 180 or 270
	int m_nUserRotation; // Rotation delta from user, can only be 0, 90, 180 or 270
	bool m_bUserZoom;
	bool m_bUserPan; // user has zoomed and panned away from default values
	bool m_bResizeForNewImage;
	double m_dZoom, m_dRealizedZoom;
	double m_dStartZoom; // zoom when start zoomin in zoom mode
	double m_dZoomAtResizeStart; // zoom factor when user started resizing JPEGView main window
	double m_dZoomMult;
	bool m_bZoomMode;
	bool m_bZoomModeOnLeftMouse;
	Helpers::EAutoZoomMode m_eAutoZoomModeWindowed;
	Helpers::EAutoZoomMode m_eAutoZoomModeFullscreen;
	Helpers::EAutoZoomMode m_autoZoomFitToScreen;
	bool m_isUserFitToScreen;

	CImageProcessingParams* m_pImageProcParams;
	bool m_bHQResampling;
	bool m_bAutoContrast;
	bool m_bAutoContrastSection;
	bool m_bLDC;
	bool m_bLandscapeMode;
	bool m_bKeepParams;

	// used to enable switch between two sets of parameters with CTRL-A
	CImageProcessingParams* m_pImageProcParams2;
	EProcessingFlags m_eProcessingFlags2;

	// set of parameters used when m_bKeepParams is true
	CImageProcessingParams* m_pImageProcParamsKept;
	EProcessingFlags m_eProcessingFlagsKept;
	double m_dZoomKept;
	CPoint m_offsetKept;
	bool m_bCurrentImageInParamDB;
	bool m_bCurrentImageIsSpecialProcessing;
	double m_dCurrentInitialLightenShadows;

	bool m_bDragging;
	bool m_bDoDragging;
	bool m_bMovieMode;
	double m_dMovieFPS;
	bool m_bProcFlagsTouched;
	EProcessingFlags m_eProcFlagsBeforeMovie;
	bool m_bInTrackPopupMenu;
	CPoint m_offsets; // Note: These offsets are center of image based
	CPoint m_DIBOffsets;
	int m_nCapturedX, m_nCapturedY;
	int m_nMouseX, m_nMouseY;
	bool m_bDefaultSelectionMode;
	bool m_bShowFileName;
	bool m_bFullScreenMode;
	bool m_bAutoFitWndToImage;
	bool m_bLockPaint;
	int m_nCurrentTimeout;
	POINT m_startMouse;
	CSize m_virtualImageSize;
	bool m_bInZooming;
	bool m_bTemporaryLowQ;
	bool m_bShowZoomFactor;
	bool m_bSpanVirtualDesktop;
	bool m_bPanMouseCursorSet;
	bool m_bMouseOn;
	bool m_bKeepParametersBeforeAnimation;
	bool m_bIsAnimationPlaying;
	int m_nLastAnimationOffset;
	int m_nExpectedNextAnimationTickCount;
	int m_nMonitor;
	WINDOWPLACEMENT m_storedWindowPlacement;
	CRect m_monitorRect;
	CRect m_clientRect;
	CRect m_windowRectOnClose;
	CString m_sSaveDirectory;
	CString m_sSaveExtension;

	Helpers::ETransitionEffect m_eTransitionEffect;
	int m_nTransitionTime;
	DWORD m_nLastSlideShowImageTickCount;
	bool m_bUseLosslessWEBP;
	bool m_isBeforeFileSelected;
	double m_dLastImageDisplayTime;
	bool m_bWindowBorderless;
	bool m_bAlwaysOnTop;
	bool m_bSelectZoom;  // keeps track of select-to-zoom mode when CTRL+SHIFT+LMouse
};
