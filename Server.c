/* Server.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Gestalt.h>
#include <Memory.h>
#include <SegLoad.h>
#include <DiskInit.h>
#include <Files.h>
#include <Script.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <EPPC.h>
#include <AppleEvents.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Devices.h>

#include "IPC.h"
#include "Misc.h"
#include "Server.h"

/*————————————————————————————————————————————————————————————*/

QDGlobals qd;

Boolean hasColorQD;

Boolean inBackGnd = false;
Boolean quitting = false;
Boolean finished = false;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustFileMenu(void)
{
	MenuHandle theMenu;
	WindowPtr theWindow;
	
	if ((theMenu = GetMenuHandle(FileID)) != nil) {
		if ((theWindow = FrontWindow()) == nil)
			DisableItem(theMenu, CloseItem);
		else
			EnableItem(theMenu, CloseItem);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustEditMenu(void)
{
	void AdjEditConf(WindowPtr, MenuHandle);
	
	short theKind;
	MenuHandle theMenu;
	WindowPtr theWindow;
	
	if ((theMenu = GetMenuHandle(EditID)) != nil) {
		if ((theWindow = FrontWindow()) == nil) {
			DisableItem(theMenu, UndoItem);
			DisableItem(theMenu, CutItem);
			DisableItem(theMenu, CopyItem);
			DisableItem(theMenu, PasteItem);
			DisableItem(theMenu, ClearItem);
			DisableItem(theMenu, SelectAllItem);
		} else if ((theKind = ((WindowPeek) theWindow)->windowKind) < 0) {
			EnableItem(theMenu, UndoItem);
			EnableItem(theMenu, CutItem);
			EnableItem(theMenu, CopyItem);
			EnableItem(theMenu, PasteItem);
			EnableItem(theMenu, ClearItem);
			DisableItem(theMenu, SelectAllItem);
		} else {
			SetPort(theWindow);
			switch (theKind) {
			case confKind:
				AdjEditConf(theWindow, theMenu);
				break;
			default:
				DisableItem(theMenu, UndoItem);
				DisableItem(theMenu, CutItem);
				DisableItem(theMenu, CopyItem);
				DisableItem(theMenu, PasteItem);
				DisableItem(theMenu, ClearItem);
				DisableItem(theMenu, SelectAllItem);
				break;
			}
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustMenus(void)
{
	AdjustFileMenu();
	AdjustEditMenu();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void NCloseWindow(WindowPtr theWindow)
{
	void CloseSplash(WindowPtr);
	void CloseConf(WindowPtr);
	
	short theKind;
	
	if ((theKind = ((WindowPeek) theWindow)->windowKind) < 0)
		CloseDeskAcc(theKind);
	else {
		SetPort(theWindow);
		switch (theKind) {
		case splashKind:
			CloseSplash(theWindow);
			break;
		case confKind:
			CloseConf(theWindow);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void Quit(void)
{
	quitting = true;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ActivateWindow(WindowPtr theWindow, Boolean theFlag)
{
	void ActivateConf(WindowPtr, Boolean);
	
	SetPort(theWindow);
	switch (((WindowPeek) theWindow)->windowKind) {
	case confKind:
		ActivateConf(theWindow, theFlag);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

pascal OSErr OpenAppAE(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr error;
	Size actSize;
	DescType typeCode;
	
	error = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode,
	 nil, 0, &actSize);
	if (error == errAEDescNotFound)
		error = noErr;
	else if (error == noErr)
		error = errAEEventNotHandled;
	if (error == noErr)
		NOP();
	return error;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

pascal OSErr OpenDocsAE(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr error;
	long numItems, index;
	Size actSize;
	DescType typeCode;
	AEKeyword keyword;
	AEDescList docList;
	FSSpec theFile;
	
	error = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	if (error == noErr) {
		error = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode,
		 nil, 0, &actSize);
		if (error == errAEDescNotFound)
			error = noErr;
		else if (error == noErr)
			error = errAEEventNotHandled;
		if (error == noErr) {
			error = AECountItems(&docList, &numItems);
			for (index = 1; error == noErr && index <= numItems; ++index) {
				error = AEGetNthPtr(&docList, index, typeFSS, &keyword, &typeCode,
				 (Ptr) &theFile, sizeof theFile, &actSize);
				if (error == noErr)
					NOP();
			}
			if (error == noErr) {
				error = AEDisposeDesc(&docList);
				return error;
			}
		}
		(void) AEDisposeDesc(&docList);
	}
	return error;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

pascal OSErr PrintDocsAE(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr error;
	long numItems, index;
	Size actSize;
	DescType typeCode;
	AEKeyword keyword;
	AEDescList docList;
	FSSpec theFile;
	
	error = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	if (error == noErr) {
		error = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode,
		 nil, 0, &actSize);
		if (error == errAEDescNotFound)
			error = noErr;
		else if (error == noErr)
			error = errAEEventNotHandled;
		if (error == noErr) {
			error = AECountItems(&docList, &numItems);
			for (index = 1; error == noErr && index <= numItems; ++index) {
				error = AEGetNthPtr(&docList, index, typeFSS, &keyword, &typeCode,
				 (Ptr) &theFile, sizeof theFile, &actSize);
				if (error == noErr)
					NOP();
			}
			if (error == noErr) {
				error = AEDisposeDesc(&docList);
				return error;
			}
		}
		(void) AEDisposeDesc(&docList);
	}
	return error;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

pascal OSErr QuitAppAE(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	OSErr error;
	Size actSize;
	DescType typeCode;
	
	error = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode,
	 nil, 0, &actSize);
	if (error == errAEDescNotFound)
		error = noErr;
	else if (error == noErr)
		error = errAEEventNotHandled;
	if (error == noErr)
		Quit();
	return error;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoClose(void)
{
	WindowPtr theWindow;
	
	if ((theWindow = FrontWindow()) != nil)
		NCloseWindow(theWindow);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoAppleMenu(short menuItem)
{
	WindowPtr NewSplash(void);
	
	Str255 deskAccName;
	
	switch (menuItem) {
	case AboutItem:
		(void) NewSplash();
		break;
	default:
		GetMenuItemText(GetMenuHandle(AppleID), menuItem, deskAccName);
		(void) OpenDeskAcc(deskAccName);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoFileMenu(short menuItem)
{
	switch (menuItem) {
	case CloseItem:
		DoClose();
		break;
	case QuitItem:
		Quit();
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoEditMenu(short menuItem)
{
	void EditConf(WindowPtr, short);
	
	short theKind;
	WindowPtr theWindow;
	
	if (!SystemEdit(menuItem-1) && (theWindow = FrontWindow()) != nil
	 && (theKind = ((WindowPeek) theWindow)->windowKind) >= 0) {
		SetPort(theWindow);
		switch (theKind) {
		case confKind:
			EditConf(theWindow, menuItem);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoMenu(long theMenu)
{
	short menuID, menuItem;
	
	menuID = HiWord(theMenu);
	menuItem = LoWord(theMenu);
	switch (menuID) {
	case AppleID:
		DoAppleMenu(menuItem);
		break;
	case FileID:
		DoFileMenu(menuItem);
		break;
	case EditID:
		DoEditMenu(menuItem);
		break;
	}
	HiliteMenu(0);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustCursor(void)
{
	void AdjCurConf(WindowPtr, Point);
	
	short theKind;
	WindowPtr theWindow;
	Point mouseLoc;
	
	if (!inBackGnd && ((theWindow = FrontWindow()) == nil
	 || (theKind = ((WindowPeek) theWindow)->windowKind) >= 0))
		if (!quitting)
			if (theWindow != nil) {
				SetPort(theWindow);
				GetMouse(&mouseLoc);
				switch (theKind) {
				case confKind:
					AdjCurConf(theWindow, mouseLoc);
					break;
				default:
					SetCursorID(arrowCursor);
					break;
				}
			} else
				SetCursorID(arrowCursor);
		else
			SetCursorID(watchCursor);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void MenuClick(const EventRecord *theEvent)
{
	AdjustMenus();
	DoMenu(MenuSelect(theEvent->where));
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ContentClick(const EventRecord *theEvent, WindowPtr theWindow)
{
	void CloseSplash(WindowPtr);
	void ClickConf(WindowPtr, Point, short);
	
	if (theWindow != FrontWindow())
		SelectWindow(theWindow);
	else {
		SetPort(theWindow);
		switch (((WindowPeek) theWindow)->windowKind) {
		case splashKind:
			CloseSplash(theWindow);
			break;
		case confKind:
			ClickConf(theWindow, theEvent->where, theEvent->modifiers);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DragClick(const EventRecord *theEvent, WindowPtr theWindow)
{
	Rect theRect;
	
	theRect = (*GetGrayRgn())->rgnBBox;
	InsetRect(&theRect, 4, 4);
	DragWindow(theWindow, theEvent->where, &theRect);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GrowClick(const EventRecord *theEvent, WindowPtr theWindow)
{
	void GrowConf(WindowPtr, Point);
	
	SetPort(theWindow);
	switch (((WindowPeek) theWindow)->windowKind) {
	case confKind:
		GrowConf(theWindow, theEvent->where);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GoAwayClick(const EventRecord *theEvent, WindowPtr theWindow)
{
	void GoAwayConf(WindowPtr, Point);
	
	SetPort(theWindow);
	switch (((WindowPeek) theWindow)->windowKind) {
	case confKind:
		GoAwayConf(theWindow, theEvent->where);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ZoomClick(const EventRecord *theEvent, WindowPtr theWindow, short partCode)
{
	void ZoomConf(WindowPtr, Point, short);
	
	SetPort(theWindow);
	switch (((WindowPeek) theWindow)->windowKind) {
	case confKind:
		ZoomConf(theWindow, theEvent->where, partCode);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoMouseDown(const EventRecord *theEvent)
{
	short partCode;
	WindowPtr whichWindow;
	
	switch (partCode = FindWindow(theEvent->where, &whichWindow)) {
	case inMenuBar:
		MenuClick(theEvent);
		break;
	case inSysWindow:
		SystemClick(theEvent, whichWindow);
		break;
	case inContent:
		ContentClick(theEvent, whichWindow);
		break;
	case inDrag:
		DragClick(theEvent, whichWindow);
		break;
	case inGrow:
		GrowClick(theEvent, whichWindow);
		break;
	case inGoAway:
		GoAwayClick(theEvent, whichWindow);
		break;
	case inZoomIn:
	case inZoomOut:
		ZoomClick(theEvent, whichWindow, partCode);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoKeyDown(const EventRecord *theEvent)
{
	void KeyConf(WindowPtr, long);
	
	WindowPtr theWindow;
	
	if (theEvent->modifiers & cmdKey) {
		AdjustMenus();
		DoMenu(MenuKey(theEvent->message & charCodeMask));
	} else if ((theWindow = FrontWindow()) != nil) {
		SetPort(theWindow);
		switch (((WindowPeek) theWindow)->windowKind) {
		case confKind:
			KeyConf(theWindow, theEvent->message);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoUpdate(const EventRecord *theEvent)
{
	void UpdateSplash(WindowPtr);
	void UpdateConf(WindowPtr);
	
	WindowPtr theWindow;
	
	theWindow = (WindowPtr) theEvent->message;
	SetPort(theWindow);
	switch (((WindowPeek) theWindow)->windowKind) {
	case splashKind:
		UpdateSplash(theWindow);
		break;
	case confKind:
		UpdateConf(theWindow);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoDisk(const EventRecord *theEvent)
{
	Point where;
	
	if (HiWord(theEvent->message) != noErr) {
		SetPt(&where, 0, 0);
		DIBadMount(where, theEvent->message);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoActivate(const EventRecord *theEvent)
{
	ActivateWindow((WindowPtr) theEvent->message, (theEvent->modifiers & activeFlag) != 0);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoOSEvent(const EventRecord *theEvent)
{
	WindowPtr theWindow;
	
	switch (theEvent->message >> 24 & 0xFF) {
	case suspendResumeMessage:
		inBackGnd = (theEvent->message & resumeFlag) == 0;
		if ((theWindow = FrontWindow()) != nil)
			ActivateWindow(theWindow, !inBackGnd);
		if (theEvent->message & convertClipboardFlag)
			NOP();
		break;
	case mouseMovedMessage:
		NOP();
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoHLEvent(const EventRecord *theEvent)
{
	(void) AEProcessAppleEvent(theEvent);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoEvent(const EventRecord *theEvent)
{
	switch (theEvent->what) {
	case mouseDown:
		if (!quitting)
			DoMouseDown(theEvent);
		break;
	case keyDown:
	case autoKey:
		if (!quitting)
			DoKeyDown(theEvent);
		break;
	case updateEvt:
		DoUpdate(theEvent);
		break;
	case diskEvt:
		DoDisk(theEvent);
		break;
	case activateEvt:
		DoActivate(theEvent);
		break;
	case osEvt:
		DoOSEvent(theEvent);
		break;
	case kHighLevelEvent:
		DoHLEvent(theEvent);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoIdle(void)
{
	extern void IdleConf(WindowRef);
	
	WindowRef theWindow;
	GrafPtr savePort;
	
	CheckTimeout();
	theWindow = FrontWindow();
	if (!quitting) {
		if (theWindow != nil) {
			GetPort(&savePort);
			SetPortWindowPort(theWindow);
			switch (GetWindowKind(theWindow)) {
			case confKind:
				IdleConf(theWindow);
				break;
			}
			SetPort(savePort);
		}
	} else
		if (theWindow != nil)
			NCloseWindow(theWindow);
		else
			finished = true;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void EventLoop(void)
{
	Boolean gotEvent;
	EventRecord theEvent;
	
	do {
		AdjustCursor();
		gotEvent = WaitNextEvent(everyEvent, &theEvent, 0, nil);
		AdjustCursor();
		if (gotEvent)
			DoEvent(&theEvent);
		else
			DoIdle();
	} while (!finished);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

void SetUpMenus(void)
{
	Handle menuList;
	
	menuList = GetNewMBar(MenuBarID);
	SetMenuBar(menuList);
	DisposeHandle(menuList);
	AppendResMenu(GetMenuHandle(AppleID), 'DRVR');
	DrawMenuBar();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

void SetUpAE(void)
{
	OSErr error;
	
	error = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
	 NewAEEventHandlerProc((ProcPtr) OpenAppAE), 0, false);
	error = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
	 NewAEEventHandlerProc((ProcPtr) OpenDocsAE), 0, false);
	error = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
	 NewAEEventHandlerProc((ProcPtr) PrintDocsAE), 0, false);
	error = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
	 NewAEEventHandlerProc((ProcPtr) QuitAppAE), 0, false);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

void Initialize(void)
{
	extern OSErr MCPOpen(void);
	extern WindowPtr NewSplash(void);
	extern void UpdateSplash(WindowPtr);
	
	EventRecord theEvent;
	WindowPtr theSplash;
	long response;
	OSErr error;
	short i;
	
	InitGraf(&qd.thePort);
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	DILoad();
	InitCursor();
	
	SetCursorID(watchCursor);
	
	i = 3;
	do
		EventAvail(everyEvent, &theEvent);
	while (--i);
	
	if (!CheckEnvirons())
		ExitToShell();
	
	if (IPCInit() != noErr)
		ExitToShell();
	
	error = Gestalt(gestaltQuickdrawVersion, &response);
	hasColorQD = ((error == noErr) && (response >= gestalt8BitQD));
	
	if ((theSplash = NewSplash()) != nil) {
		SetPort(theSplash);
		UpdateSplash(theSplash);
	}
	
	if (MCPOpen() != noErr)
		ExitToShell();
	
	SetUpAE();
	SetUpMenus();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

void CleanUp(void)
{
	extern OSErr MCPClose(void);
	
	(void) MCPClose();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

main()
{
	extern int _DataInit(void);

	UnloadSeg(_DataInit);
	
	MaxApplZone();

	Initialize();
	UnloadSeg(Initialize);
	
	EventLoop();
	
	CleanUp();
}

/*————————————————————————————————————————————————————————————*/
