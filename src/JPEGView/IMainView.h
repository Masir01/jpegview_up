#pragma once

// IMainView.h - Narrow view interface extracted from CMainDlg.
//
// Controllers (CropCtl, ZoomNavigatorCtl, the *PanelCtl classes, PanelController,
// HelpDlg, ...) depend on this interface instead of the concrete CMainDlg. This
// breaks the physical include cycle (MainDlg.h <-> CropCtl.h) and the bidirectional
// coupling between CMainDlg and its controllers.
//
// The interface methods are split into role interfaces in IMainViewRoles.h
// (Interface Segregation); IMainView aggregates them so existing IMainView* callers
// keep working unchanged. A controller may instead depend on a single role
// (INavigable, IZoomable, ICanvas, IViewState, IPanelHost, ICommandTarget).
//
// All 21 IsXxx() booleans were pure pass-throughs and are merged into the CViewFlags
// aggregate exposed via IViewState::ViewFlags(). (IsCropping / IsDoCropping delegate
// to CCropCtl but are exposed through CViewFlags::bCropping / bDoCropping, not as
// separate interface methods, so there is no name clash with CCropCtl::IsCropping.)

#include "stdafx.h"
#include "ProcessParams.h"
#include "Helpers.h"
#include "IMainViewRoles.h"

// Forward declarations (also provided by IMainViewRoles.h; repeated for clarity).
class CJPEGImage;
class CButtonCtrl;

// EImagePosition is defined in IMainViewRoles.h and reachable via the include above.

// Free helper functions used as button-callback targets (formerly CMainDlg statics).
void OnExecuteCommand(void* pContext, int nParameter, CButtonCtrl& sender);
bool IsCurrentImageFitToScreen(void* pContext);

// Aggregated view interface. Implemented by CMainDlg. Controllers may depend on the
// whole IMainView or on individual role interfaces (INavigable, IZoomable, ...).
class IMainView : public INavigable, public IZoomable, public ICanvas,
                  public IViewState, public IPanelHost, public ICommandTarget {
public:
	virtual ~IMainView() {}

};
