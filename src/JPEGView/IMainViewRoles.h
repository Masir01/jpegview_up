#pragma once

// IMainViewRoles.h - Role interfaces extracted from IMainView (Interface Segregation).
//
// IMainView originally exposed ~90 methods; every controller depending on it was
// forced to depend on capabilities it never used. The methods are now split by
// responsibility into narrow role interfaces. IMainView aggregates them (see
// IMainView.h) so existing IMainView* callers keep working unchanged, while an
// individual controller may later depend on a single role (e.g. IZoomable) to
// reduce coupling further.
//
// The 21 IsXxx() booleans were pure pass-throughs and are all merged into the
// CViewFlags aggregate exposed via IViewState::ViewFlags(). IsCropping / IsDoCropping
// delegate to CCropCtl but are surfaced through CViewFlags::bCropping / bDoCropping
// (no separate interface method, so no clash with CCropCtl::IsCropping).

#include "stdafx.h"
#include "ProcessParams.h"
#include "Helpers.h"

// Forward declarations of types returned by pointer/reference from the interfaces.
class CJPEGImage;
class CJPEGProvider;
class CPanelMgr;
class CFileList;
class CKeyMap;
class CNavigationPanelCtl;
class CEXIFDisplayCtl;
class CUnsharpMaskPanelCtl;
class CImageProcPanelCtl;
class CRotationPanelCtl;
class CTiltCorrectionPanelCtl;
class CZoomNavigatorCtl;
class CWndButtonPanelCtl;
class CInfoButtonPanelCtl;
class CCropCtl;
class CPanel;
class CButtonCtrl;

// Previously nested in CMainDlg; moved here so it is available without CMainDlg.
enum EImagePosition {
	POS_First,
	POS_Last,
	POS_Next,
	POS_NextSlideShow,
	POS_NextAnimation,
	POS_Previous,
	POS_Current,
	POS_Clipboard,
	POS_Toggle,
	POS_AwayFromCurrent
};

// Read-only snapshot of the boolean view-state flags previously exposed as 21
// IsXxx() methods. Constructed once per query by CMainDlg::ViewFlags().
struct CViewFlags {
	bool bShowFileName;
	bool bInMovieMode;
	bool bInZoomMode;
	bool bPlayingAnimation;
	bool bFullScreenMode;
	bool bLandscapeMode;
	bool bHQResampling;
	bool bAutoContrast;
	bool bAutoContrastSection;
	bool bLDC;
	bool bKeepParams;
	bool bSpanVirtualDesktop;
	bool bDoDragging;
	bool bInZooming;
	bool bShowZoomFactor;
	bool bPanMouseCursorSet;
	bool bMouseOn;
	bool bWindowBorderless;
	bool bAlwaysOnTop;
	// bCropping / bDoCropping mirror CCropCtl state, filled by CMainDlg::ViewFlags()
	bool bCropping;
	bool bDoCropping;
};

// Navigation / file-list / slide-show control.
class INavigable {
public:
	virtual ~INavigable() {}
	virtual void SetStartupInfo(LPCTSTR sStartupFile, int nAutostartSlideShow, Helpers::ESorting eSorting, Helpers::ETransitionEffect eEffect,
		int nTransitionTime, bool bAutoExit, int nDisplayMonitor) = 0;
	virtual CJPEGImage* GetCurrentImage() = 0;
	virtual LPCTSTR CurrentFileName(bool bFileTitle) = 0;
	virtual CFileList* GetFileList() = 0;
	virtual CJPEGProvider* GetJPEGProvider() = 0;
	virtual CKeyMap* GetKeyMap() = 0;
	virtual void GotoImage(EImagePosition ePos) = 0;
	virtual void ReloadImage(bool keepParameters, bool updateWindow = true) = 0;
	virtual void AdjustWindowToImage(bool bAfterStartup) = 0;
	virtual bool IsAdjustWindowToImage() = 0;
	virtual bool IsImageExactlyFittingWindow() = 0;
	virtual Helpers::ETransitionEffect GetTransitionEffect() = 0;
	virtual int GetTransitionTime() = 0;
	virtual bool IsInSlideShowWithTransition() = 0;
};

// Zoom / rotation / pan / coordinate transforms.
class IZoomable {
public:
	virtual ~IZoomable() {}
	virtual double GetZoom() = 0;
	virtual int GetRotation() = 0;
	virtual CPoint GetDIBOffset() = 0;
	virtual double GetZoomMultiplier(CJPEGImage* pImage, const CRect& clientRect) = 0;
	virtual Helpers::EAutoZoomMode GetAutoZoomMode() = 0;
	virtual CPoint GetOffsets() = 0;
	virtual void ResetZoomTo100Percents(bool bZoomToMouse) = 0;
	virtual void ResetZoomToFitScreen(bool bFillWithCrop, bool bAllowEnlarge, bool bAdjustWindowSize) = 0;
	virtual bool PerformPan(int dx, int dy, bool bAbsolute) = 0;
	virtual void StartDragging(int nX, int nY, bool bDragWithZoomNavigator) = 0;
	virtual void DoDragging() = 0;
	virtual void EndDragging() = 0;
	virtual void SetCursorForMoveSection() = 0;
	virtual bool ScreenToImage(float & fX, float & fY) = 0;
	virtual bool ImageToScreen(float & fX, float & fY) = 0;
};

// Window canvas: CWindow pass-throughs + paint helpers.
class ICanvas {
public:
	virtual ~ICanvas() {}
	virtual HWND GetHWND() = 0;
	virtual const CRect& ClientRect() = 0;
	virtual const CRect& WindowRectOnClose() = 0;
	virtual const CRect& MonitorRect() = 0;
	virtual const CSize& VirtualImageSize() = 0;
	virtual void DisplayErrors(CJPEGImage* pCurrentImage, const CRect& clientRect, CDC& dc) = 0;
	virtual void DisplayFileName(const CRect& imageProcessingArea, CDC& dc, double realizedZoom) = 0;
	virtual void BlendBlackRect(CDC & targetDC, CPanel& panel, float fBlendFactor) = 0;
	virtual void Invalidate(BOOL bErase = TRUE) = 0;
	virtual void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE) = 0;
	virtual HDC GetDC() = 0;
	virtual int ReleaseDC(HDC hDC) = 0;
	virtual BOOL ScreenToClient(LPPOINT lpPoint) = 0;
	virtual BOOL ClientToScreen(LPPOINT lpPoint) = 0;
	virtual BOOL UpdateWindow() = 0;
};

// View state query: merged IsXxx flags + mouse position.
class IViewState {
public:
	virtual ~IViewState() {}
	virtual CPoint GetMousePos() = 0;
	virtual const CViewFlags& ViewFlags() const = 0;
};

// Panel host: access to owned panel controllers + processing params.
class IPanelHost {
public:
	virtual ~IPanelHost() {}
	virtual CPanelMgr* GetPanelMgr() = 0;
	virtual CNavigationPanelCtl* GetNavPanelCtl() = 0;
	virtual CEXIFDisplayCtl* GetEXIFDisplayCtl() = 0;
	virtual CUnsharpMaskPanelCtl* GetUnsharpMaskPanelCtl() = 0;
	virtual CImageProcPanelCtl* GetImageProcPanelCtl() = 0;
	virtual CRotationPanelCtl* GetRotationPanelCtl() = 0;
	virtual CTiltCorrectionPanelCtl* GetTiltCorrectionPanelCtl() = 0;
	virtual CZoomNavigatorCtl* GetZoomNavigatorCtl() = 0;
	virtual CWndButtonPanelCtl* GetWndButtonPanelCtl() = 0;
	virtual CInfoButtonPanelCtl* GetInfoButtonPanelCtl() = 0;
	virtual CCropCtl* GetCropCtl() = 0;
	virtual CImageProcessingParams* GetImageProcessingParams() = 0;
	virtual EProcessingFlags CreateDefaultProcessingFlags(bool bKeepParams = false) = 0;
	virtual bool PrepareForModalPanel() = 0;
};

// Command target: execute commands / popup / mouse on-off.
class ICommandTarget {
public:
	virtual ~ICommandTarget() {}
	virtual void UpdateWindowTitle() = 0;
	virtual void MouseOff() = 0;
	virtual void MouseOn() = 0;
	virtual void ExecuteCommand(int nCommand) = 0;
	virtual int TrackPopupMenu(CPoint pos, HMENU hMenu) = 0;
};
