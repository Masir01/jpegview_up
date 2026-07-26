#pragma once

// IMainView.h - Narrow view interface extracted from CMainDlg.
//
// Controllers (CropCtl, ZoomNavigatorCtl, the *PanelCtl classes, PanelController,
// HelpDlg, ...) depend on this interface instead of the concrete CMainDlg. This
// breaks the physical include cycle (MainDlg.h <-> CropCtl.h) and the bidirectional
// coupling between CMainDlg and its controllers: a controller can be compiled and
// tested against IMainView alone, without pulling in the whole main dialog header.
//
// The interface mirrors the public methods of CMainDlg that controllers actually
// call. CMainDlg implements it (its existing methods override the pure virtuals
// automatically). A few CWindow methods (Invalidate, GetDC, ...) are exposed as
// pass-throughs so controllers can call them via the interface.

#include "stdafx.h"
#include "ProcessParams.h"
#include "Helpers.h"

// Forward declarations of types returned by pointer/reference from the interface.
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

// Free helper functions used as button-callback targets (formerly CMainDlg statics).
void OnExecuteCommand(void* pContext, int nParameter, CButtonCtrl& sender);
bool IsCurrentImageFitToScreen(void* pContext);

// Pure virtual view interface implemented by CMainDlg.
class IMainView {
public:
	virtual ~IMainView() {}

	// Called by main()
	virtual void SetStartupInfo(LPCTSTR sStartupFile, int nAutostartSlideShow, Helpers::ESorting eSorting, Helpers::ETransitionEffect eEffect,
		int nTransitionTime, bool bAutoExit, int nDisplayMonitor) = 0;

	// Called by the different controller classes
	virtual HWND GetHWND() = 0;
	virtual bool IsShowFileName() = 0;
	virtual bool IsInMovieMode() = 0;
	virtual bool IsInZoomMode() = 0;
	virtual bool IsPlayingAnimation() = 0;
	virtual bool IsFullScreenMode() = 0;
	virtual bool IsLandscapeMode() = 0;
	virtual bool IsHQResampling() = 0;
	virtual bool IsAutoContrast() = 0;
	virtual bool IsAutoContrastSection() = 0;
	virtual bool IsLDC() = 0;
	virtual bool IsKeepParams() = 0;
	virtual bool IsSpanVirtualDesktop() = 0;
	virtual bool IsCropping() = 0;
	virtual bool IsDoCropping() = 0;
	virtual bool IsDoDragging() = 0;
	virtual bool IsInZooming() = 0;
	virtual bool IsShowZoomFactor() = 0;
	virtual bool IsPanMouseCursorSet() = 0;
	virtual bool IsMouseOn() = 0;
	virtual bool IsWindowBorderless() = 0;
	virtual bool IsAlwaysOnTop() = 0;

	virtual CPoint GetMousePos() = 0;
	virtual double GetZoom() = 0;
	virtual int GetRotation() = 0;
	virtual CJPEGImage* GetCurrentImage() = 0;
	virtual CPanelMgr* GetPanelMgr() = 0;
	virtual LPCTSTR CurrentFileName(bool bFileTitle) = 0;
	virtual CFileList* GetFileList() = 0;
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
	virtual const CRect& ClientRect() = 0;
	virtual const CRect& WindowRectOnClose() = 0;
	virtual const CRect& MonitorRect() = 0;
	virtual const CSize& VirtualImageSize() = 0;
	virtual CJPEGProvider* GetJPEGProvider() = 0;
	virtual CKeyMap* GetKeyMap() = 0;
	virtual CPoint GetDIBOffset() = 0;
	virtual double GetZoomMultiplier(CJPEGImage* pImage, const CRect& clientRect) = 0;
	virtual Helpers::EAutoZoomMode GetAutoZoomMode() = 0;
	virtual CPoint GetOffsets() = 0;
	virtual CImageProcessingParams* GetImageProcessingParams() = 0;
	virtual EProcessingFlags CreateDefaultProcessingFlags(bool bKeepParams = false) = 0;
	virtual void DisplayErrors(CJPEGImage* pCurrentImage, const CRect& clientRect, CDC& dc) = 0;
	virtual void DisplayFileName(const CRect& imageProcessingArea, CDC& dc, double realizedZoom) = 0;
	virtual void BlendBlackRect(CDC & targetDC, CPanel& panel, float fBlendFactor) = 0;

	virtual void UpdateWindowTitle() = 0;
	virtual void MouseOff() = 0;
	virtual void MouseOn() = 0;
	virtual void GotoImage(EImagePosition ePos) = 0;
	virtual void ReloadImage(bool keepParameters, bool updateWindow = true) = 0;
	virtual void ResetZoomTo100Percents(bool bZoomToMouse) = 0;
	virtual void ResetZoomToFitScreen(bool bFillWithCrop, bool bAllowEnlarge, bool bAdjustWindowSize) = 0;
	virtual bool PerformPan(int dx, int dy, bool bAbsolute) = 0;
	virtual void StartDragging(int nX, int nY, bool bDragWithZoomNavigator) = 0;
	virtual void DoDragging() = 0;
	virtual void EndDragging() = 0;
	virtual void SetCursorForMoveSection() = 0;
	virtual bool ScreenToImage(float & fX, float & fY) = 0;
	virtual bool ImageToScreen(float & fX, float & fY) = 0;
	virtual void ExecuteCommand(int nCommand) = 0;
	virtual bool PrepareForModalPanel() = 0;
	virtual int TrackPopupMenu(CPoint pos, HMENU hMenu) = 0;
	virtual void AdjustWindowToImage(bool bAfterStartup) = 0;
	virtual bool IsAdjustWindowToImage() = 0;
	virtual bool IsImageExactlyFittingWindow() = 0;
	virtual Helpers::ETransitionEffect GetTransitionEffect() = 0;
	virtual int GetTransitionTime() = 0;
	virtual bool IsInSlideShowWithTransition() = 0;

	// CWindow pass-throughs so controllers can call window methods via the interface
	virtual void Invalidate(BOOL bErase = TRUE) = 0;
	virtual void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE) = 0;
	virtual HDC GetDC() = 0;
	virtual int ReleaseDC(HDC hDC) = 0;
	virtual BOOL ScreenToClient(LPPOINT lpPoint) = 0;
	virtual BOOL ClientToScreen(LPPOINT lpPoint) = 0;
	virtual BOOL UpdateWindow() = 0;
};
