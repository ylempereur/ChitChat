/* Client.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>			// ConditionalMacros.h
#include <PLStringFuncs.h>	// Types.h
#include <Gestalt.h>		// Types.h, MixedMode.h
#include <Memory.h>			// Types.h
#include <Resources.h>		// Types.h, Files.h
#include <SegLoad.h>		// Types.h
#include <DiskInit.h>		// Types.h
#include <Script.h>			// Types.h, OSUtils.h
#include <Quickdraw.h>		// Types.h
#include <Fonts.h>			// Types.h
#include <Icons.h>			// Types.h, Quickdraw.h
#include <Events.h>			// Types.h, Quickdraw.h
#include <EPPC.h>			// PPCToolbox.h, Processes.h, Events.h
#include <AppleEvents.h>	// Types.h, Memory.h, OSUtils.h, Events.h, EPPC.h, Notification.h
#include <Windows.h>		// Quickdraw.h, Events.h, Controls.h
#include <Menus.h>			// Quickdraw.h
#include <TextEdit.h>		// Quickdraw.h
#include <Dialogs.h>		// Windows.h, TextEdit.h
#include <ToolUtils.h>		// Quickdraw.h
#include <Devices.h>
#include <Packages.h>		// Types.h, StandardFile.h, Script.h
#include <Notification.h>	// Types.h, OSUtils.h
#include <Printing.h>

#include "SmartScrollAPI.h"

#include "IPC.h"
#include "Protocol.h"
#include "Misc.h"
#include "Client.h"

/*————————————————————————————————————————————————————————————*/

typedef struct {
	QElemPtr qLink;
	long cid;
	Str32 title;
	Str32 name;
} NoteRec, *NotePtr;

/*————————————————————————————————————————————————————————————*/

QDGlobals qd;

Str32 gObject, gZone, gUserName;
Str8 gPassword;

QHdr gNoteQueue;
THPrint prRecHdl = nil;
Handle gIconSuite;
Handle gSound;
NMRec gNote;

MLHandle macroList;
MenuRef macroMenu = nil;

short gRate;

Boolean hasColorQD, hasAliasMgr, hasFindFolder, hasDragMgr, hasGetHiliteRgn;
Boolean hasSoundInput, hasAaron;

Boolean gSplash, gAutoLogon, gTracking;

Boolean inBackGnd = false;
Boolean quitting = false;
Boolean finished = false;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

/*————————————————————————————————————————————————————————————*/

void InitQueue(QHdrPtr q)
{
	q->qFlags = 0x0000;
	q->qHead = nil;
	q->qTail = nil;
}

/*————————————————————————————————————————————————————————————*/

void InitNotify(void)
{
	OSErr error;
	
	InitQueue(&gNoteQueue);
	error = GetIconSuite(&gIconSuite, 128, svAllSmallData);
	if (error != noErr)
		gIconSuite = nil;
	gSound = GetResource('snd ', 128);
}

/*————————————————————————————————————————————————————————————*/

OSErr NotifyDisc(void)
{
	NotePtr n;
	
	n = (NotePtr) NewPtr(sizeof(NoteRec));
	if (n == nil)
		return memFullErr;
	n->cid = 0;
	Enqueue((QElemPtr) n, &gNoteQueue);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr NotifyUser(long cid, Str32 title, Str32 name)
{
	NotePtr n;
	
	n = (NotePtr) NewPtr(sizeof(NoteRec));
	if (n == nil)
		return memFullErr;
	n->cid = cid;
	PLstrcpy(n->title, title);
	PLstrcpy(n->name, name);
	Enqueue((QElemPtr) n, &gNoteQueue);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

void NotifyIdle(void)
{
	extern long GetConfWindowCID(WindowRef);
	extern long GetGrafWindowCID(WindowRef);
	extern OSErr Notify(long, Str32, Str32);
	extern void Error(short);
	
	NotePtr n;
	NMRecPtr p;
	WindowRef window;
	long cid;
	OSErr error;
	Boolean found;
	
	n = (NotePtr) gNoteQueue.qHead;
	if (n == nil)
		return;
	cid = n->cid;
	if (cid != 0) {
		window = FrontWindow(), found = false;
		while (window != nil) {
			switch (GetWindowKind(window)) {
			case confKind:
				if (GetConfWindowCID(window) == cid)
					found = true;
				break;
			case grafKind:
				if (GetGrafWindowCID(window) == cid)
					found = true;
				break;
			}
			if (found) {
				(void) Dequeue((QElemPtr) n, &gNoteQueue);
				DisposePtr((Ptr) n);
				return;
			}
			window = GetNextWindow(window);
		}
	}
	if (inBackGnd) {
		if (gNoteQueue.qFlags & 0x8000)
			return;
		p = &gNote;
		p->qType = nmType;
		p->nmMark = 1;
		p->nmIcon = gIconSuite;
		p->nmSound = gSound;
		p->nmStr = nil;
		p->nmResp = nil;
		p->nmRefCon = 0;
		error = NMInstall(p);
		if (error != noErr)
			return;
		gNoteQueue.qFlags |= 0x8000;
	} else {
		if (gNoteQueue.qFlags & 0x8000) {
			(void) NMRemove(&gNote);
			gNoteQueue.qFlags &= ~0x8000;
		}
		if (cid == 0)
			Error(3);
		else {
			error = Notify(n->cid, n->title, n->name);
			if (error == ERR_NOSUCHCONF)
				Error(1);
		}
		(void) Dequeue((QElemPtr) n, &gNoteQueue);
		DisposePtr((Ptr) n);
	}
}

/*————————————————————————————————————————————————————————————*/

void NotifyCleanUp(void)
{
	if (gNoteQueue.qFlags & 0x8000)
		(void) NMRemove(&gNote);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustFileMenu(void)
{
	extern void AdjFileConf(WindowRef, MenuRef);
	extern void AdjFileGraf(WindowRef, MenuRef);
	
	MenuRef theMenu;
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if ((theMenu = GetMenuHandle(FileID)) != nil) {
		if ((theWindow = FrontWindow()) == nil) {
			DisableItem(theMenu, CloseItem);
			DisableItem(theMenu, SaveAsItem);
			DisableItem(theMenu, PrintItem);
		} else if ((theKind = GetWindowKind(theWindow)) < 0) {
			EnableItem(theMenu, CloseItem);
			DisableItem(theMenu, SaveAsItem);
			DisableItem(theMenu, PrintItem);
		} else {
			GetPort(&savePort);
			SetPortWindowPort(theWindow);
			switch (theKind) {
			case confKind:
				AdjFileConf(theWindow, theMenu);
				break;
			case grafKind:
				AdjFileGraf(theWindow, theMenu);
				break;
			default:
				EnableItem(theMenu, CloseItem);
				DisableItem(theMenu, SaveAsItem);
				DisableItem(theMenu, PrintItem);
				break;
			}
			SetPort(savePort);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustEditMenu(void)
{
	extern void AdjEditConf(WindowRef, MenuRef);
	extern void AdjEditGraf(WindowRef, MenuRef);
	
	MenuRef theMenu;
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if ((theMenu = GetMenuHandle(EditID)) != nil) {
		if ((theWindow = FrontWindow()) == nil) {
			DisableItem(theMenu, UndoItem);
			DisableItem(theMenu, CutItem);
			DisableItem(theMenu, CopyItem);
			DisableItem(theMenu, PasteItem);
			DisableItem(theMenu, ClearItem);
			DisableItem(theMenu, SelectAllItem);
		} else if ((theKind = GetWindowKind(theWindow)) < 0) {
			EnableItem(theMenu, UndoItem);
			EnableItem(theMenu, CutItem);
			EnableItem(theMenu, CopyItem);
			EnableItem(theMenu, PasteItem);
			EnableItem(theMenu, ClearItem);
			DisableItem(theMenu, SelectAllItem);
		} else {
			GetPort(&savePort);
			SetPortWindowPort(theWindow);
			switch (theKind) {
			case confKind:
				AdjEditConf(theWindow, theMenu);
				break;
			case grafKind:
				AdjEditGraf(theWindow, theMenu);
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
			SetPort(savePort);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustSpecialMenu(void)
{
	Boolean MCPOnLine(void);
	
	MenuRef theMenu;
	
	if ((theMenu = GetMenuHandle(SpecialID)) != nil) {
		if (!MCPOnLine()) {
			EnableItem(theMenu, ConnectItem);
			DisableItem(theMenu, InviteItem);
			DisableItem(theMenu, JoinItem);
			DisableItem(theMenu, ChPasswdItem);
		} else {
			DisableItem(theMenu, ConnectItem);
			EnableItem(theMenu, InviteItem);
			EnableItem(theMenu, JoinItem);
			EnableItem(theMenu, ChPasswdItem);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustVoiceMenu(void)
{
	MenuRef theMenu;
	short i;
	
	if ((theMenu = GetMenuHandle(VoiceID)) != nil) {
		for (i = 1; i <= 4; i++) {
			SetItemMark(theMenu, i, i == gRate ? checkMark : noMark);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustMenus(void)
{
	AdjustFileMenu();
	AdjustEditMenu();
	AdjustSpecialMenu();
	AdjustVoiceMenu();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void NCloseWindow(WindowRef theWindow)
{
	extern void CloseSplash(WindowRef);
	extern void CloseTrack(WindowRef);
	extern long CloseConf(WindowRef);
	extern long CloseGraf(WindowRef);
	extern OSErr MCPLeave(long);
	
	GrafPtr savePort;
	long cid;
	short theKind;
	
	if ((theKind = GetWindowKind(theWindow)) < 0)
		CloseDeskAcc(theKind);
	else {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (theKind) {
		case splashKind:
			CloseSplash(theWindow);
			break;
		case tracKind:
			CloseTrack(theWindow);
			break;
		case confKind:
			cid = CloseConf(theWindow);
			(void) MCPLeave(cid);
			break;
		case grafKind:
			cid = CloseGraf(theWindow);
			(void) MCPLeave(cid);
			break;
		}
		SetPort(savePort);
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

void ActivateWindow(WindowRef theWindow, Boolean theFlag)
{
	extern void ActivateConf(WindowRef, Boolean);
	extern void ActivateGraf(WindowRef, Boolean);
	
	GrafPtr savePort;
	
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	switch (GetWindowKind(theWindow)) {
	case confKind:
		ActivateConf(theWindow, theFlag);
		break;
	case grafKind:
		ActivateGraf(theWindow, theFlag);
		break;
	}
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

pascal OSErr OpenAppAE(const AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefcon)
{
	extern void KillSplash(void);
	void DoConnect(void);
	
	DescType typeCode;
	Size actSize;
	OSErr error;
	
	error = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard, &typeCode,
	 nil, 0, &actSize);
	if (error == errAEDescNotFound)
		error = noErr;
	else if (error == noErr)
		error = errAEEventNotHandled;
	if (error == noErr) {
		KillSplash();
		DoConnect();
	}
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
	WindowRef theWindow;
	
	if ((theWindow = FrontWindow()) != nil)
		NCloseWindow(theWindow);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoSaveAs(void)
{
	extern void SaveConf(WindowRef);
	extern void SaveGraf(WindowRef);
	
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if ((theWindow = FrontWindow()) != nil && (theKind = GetWindowKind(theWindow)) >= 0) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (theKind) {
		case confKind:
			SaveConf(theWindow);
			break;
		case grafKind:
			SaveGraf(theWindow);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoPageSetup(void)
{
	SetCursorID(watchCursor);
	PrOpen();
	if (PrError() != noErr) {
		PrClose();
		SysBeep(30);
		return;
	}
	if (prRecHdl == nil) {
		prRecHdl = (THPrint) NewHandleClear(sizeof(TPrint));
		if (prRecHdl == nil) {
			PrClose();
			SysBeep(30);
			return;
		}
		PrintDefault(prRecHdl);
		if (PrError() != noErr) {
			DisposeHandle((Handle) prRecHdl);
			prRecHdl = nil;
			PrClose();
			SysBeep(30);
			return;
		}
	}
	SetCursorID(arrowCursor);
	(void) PrStlDialog(prRecHdl);
	PrClose();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoPrint(void)
{
	extern void PrintConf(WindowRef);
	
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if ((theWindow = FrontWindow()) != nil && (theKind = GetWindowKind(theWindow)) >= 0) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (theKind) {
		case confKind:
			PrintConf(theWindow);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoConnect(void)
{
	extern WindowRef NewTrack(void);
	extern OSErr Logon(ConstStr32Param, Str32, Str8);
	extern OSErr MCPOpen(EntityPtr);
	extern OSErr MCPClose(void);
	extern OSErr MCPLogin(Str32, Str8);
	extern void Denied(short);
	
	EntityName nbpEntity;
	OSErr error;
	short i;
	
	if (gAutoLogon) {
		PLstrcpy(nbpEntity.objStr, gObject);
		PLstrcpy(nbpEntity.typeStr, "\pMCPServer");
		PLstrcpy(nbpEntity.zoneStr, gZone);
		error = MCPOpen(&nbpEntity);
		if (error == noErr) {
			error = MCPLogin(gUserName, gPassword);
			if (error == noErr) {
				if (gTracking)
					(void) NewTrack();
				return;
			}
			(void) MCPClose();
		}
	}
	PLstrcpy(nbpEntity.objStr, gObject);
	PLstrcpy(nbpEntity.zoneStr, gZone);
	error = IPCBrowser(true, &nbpEntity, "\pMCPServer");
	if (error != noErr)
		return;
	error = MCPOpen(&nbpEntity);
	if (error != noErr) {
		if (error == errOpening)
			Denied(5);
		else
			SysBeep(30);
		return;
	}
	PLstrcpy(gObject, nbpEntity.objStr);
	PLstrcpy(gZone, nbpEntity.zoneStr);
	for (i = 3; i != 0; i--) {
		error = Logon(nbpEntity.objStr, gUserName, gPassword);
		if (error != noErr) {
			(void) MCPClose();
			return;
		}
		error = MCPLogin(gUserName, gPassword);
		switch (error) {
		case noErr:
			if (gTracking)
				(void) NewTrack();
			return;
		case ERR_BADVERSION:
			Denied(1);
			break;
		case ERR_NOSUCHNAME:
			Denied(2);
			break;
		case ERR_PASSWDMISMATCH:
			Denied(3);
			break;
		case ERR_NAMEINUSE:
			Denied(4);
			break;
		}
	}
	(void) MCPClose();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoAppleMenu(short menuItem)
{
	WindowRef NewSplash(void);
	
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
	extern void Preferences(void);
	extern Boolean MCPOnLine(void);
	extern OSErr DiscAlert(void);
	
	switch (menuItem) {
	case CloseItem:
		DoClose();
		break;
	case SaveAsItem:
		DoSaveAs();
		break;
	case PageSetupItem:
		DoPageSetup();
		break;
	case PrintItem:
		DoPrint();
		break;
	case PreferencesItem:
		Preferences();
		break;
	case QuitItem:
		if (!MCPOnLine() || DiscAlert() != userCanceledErr)
			Quit();
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoEditMenu(short menuItem)
{
	extern void EditConf(WindowRef, short);
	extern void EditGraf(WindowRef, short);
	
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if (!SystemEdit(menuItem-1) && (theWindow = FrontWindow()) != nil
	 && (theKind = GetWindowKind(theWindow)) >= 0) {
	 	GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (theKind) {
		case confKind:
			EditConf(theWindow, menuItem);
			break;
		case grafKind:
			EditGraf(theWindow, menuItem);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoSpecialMenu(short menuItem)
{
	extern OSErr Invite(void);
	extern OSErr Join(void);
	extern OSErr ChangePass(void);
	extern void Error(short);
	
	OSErr error;
	
	switch (menuItem) {
	case ConnectItem:
		DoConnect();
		break;
	case InviteItem:
		(void) Invite();
		break;
	case JoinItem:
		error = Join();
		if (error == ERR_NOSUCHCONF)
			Error(1);
		break;
	case ChPasswdItem:
		error = ChangePass();
		if (error == ERR_PASSWDMISMATCH)
			Error(2);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoVoiceMenu(short menuItem)
{
	gRate = menuItem;
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
	case SpecialID:
		DoSpecialMenu(menuItem);
		break;
	case VoiceID:
		DoVoiceMenu(menuItem);
		break;
	}
	HiliteMenu(0);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjustCursor(void)
{
	extern void AdjCurConf(WindowRef, Point);
	
	Point mouseLoc;
	WindowRef theWindow;
	GrafPtr savePort;
	short theKind;
	
	if (!inBackGnd && ((theWindow = FrontWindow()) == nil
	 || (theKind = GetWindowKind(theWindow)) >= 0))
		if (!quitting)
			if (theWindow != nil) {
				GetPort(&savePort);
				SetPortWindowPort(theWindow);
				GetMouse(&mouseLoc);
				switch (theKind) {
				case confKind:
					AdjCurConf(theWindow, mouseLoc);
					break;
				default:
					SetCursorID(arrowCursor);
					break;
				}
				SetPort(savePort);
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

void ContentClick(const EventRecord *theEvent, WindowRef theWindow)
{
	extern void CloseSplash(WindowRef);
	extern void ClickConf(WindowRef, const EventRecord *, Point);
	extern void ClickGraf(WindowRef, const EventRecord *);
	extern void BackClickGraf(WindowRef, const EventRecord *);
	
	Point thePoint;
	GrafPtr savePort;
	
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	thePoint = theEvent->where;
	GlobalToLocal(&thePoint);
	if (theWindow == FrontWindow())
		switch (GetWindowKind(theWindow)) {
		case splashKind:
			CloseSplash(theWindow);
			break;
		case confKind:
			ClickConf(theWindow, theEvent, thePoint);
			break;
		case grafKind:
			ClickGraf(theWindow, theEvent);
			break;
		}
	else
		switch (GetWindowKind(theWindow)) {
		case grafKind:
			BackClickGraf(theWindow, theEvent);
			break;
		default:
			SelectWindow(theWindow);
			break;
		}
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DragClick(const EventRecord *theEvent, WindowRef theWindow)
{
	Rect theRect;
	
	theRect = (*GetGrayRgn())->rgnBBox;
	InsetRect(&theRect, 4, 4);
	DragWindow(theWindow, theEvent->where, &theRect);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GrowClick(const EventRecord *theEvent, WindowRef theWindow)
{
	void GrowConf(WindowRef, Point);
	void GrowGraf(WindowRef, Point);
	
	GrafPtr savePort;
	
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	switch (GetWindowKind(theWindow)) {
	case confKind:
		GrowConf(theWindow, theEvent->where);
		break;
	case grafKind:
		GrowGraf(theWindow, theEvent->where);
		break;
	}
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GoAwayClick(const EventRecord *theEvent, WindowRef theWindow)
{
	extern void CloseTrack(WindowRef);
	extern long CloseConf(WindowRef);
	extern long CloseGraf(WindowRef);
	extern OSErr MCPLeave(long);
	
	GrafPtr savePort;
	long cid;
	
	if (TrackGoAway(theWindow, theEvent->where)) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (GetWindowKind(theWindow)) {
		case tracKind:
			CloseTrack(theWindow);
			break;
		case confKind:
			cid = CloseConf(theWindow);
			(void) MCPLeave(cid);
			break;
		case grafKind:
			cid = CloseGraf(theWindow);
			(void) MCPLeave(cid);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ZoomClick(const EventRecord *theEvent, WindowRef theWindow, short partCode)
{
	void ZoomConf(WindowRef, short);
	void ZoomGraf(WindowRef, short);
	
	GrafPtr savePort;
	
	if (TrackBox(theWindow, theEvent->where, partCode)) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (GetWindowKind(theWindow)) {
		case confKind:
			ZoomConf(theWindow, partCode);
			break;
		case grafKind:
			ZoomGraf(theWindow, partCode);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoMouseDown(const EventRecord *theEvent)
{
	WindowRef whichWindow;
	short partCode;
	
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
	extern void KeyConf(WindowRef, long);
	extern void KeyGraf(WindowRef, long);
	
	WindowRef theWindow;
	GrafPtr savePort;
	
	if (theEvent->modifiers & cmdKey) {
		AdjustMenus();
		DoMenu(MenuKey(theEvent->message & charCodeMask));
	} else if ((theWindow = FrontWindow()) != nil) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		switch (GetWindowKind(theWindow)) {
		case confKind:
			KeyConf(theWindow, theEvent->message);
			break;
		case grafKind:
			KeyGraf(theWindow, theEvent->message);
			break;
		}
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoUpdate(const EventRecord *theEvent)
{
	void UpdateSplash(WindowRef);
	void UpdateTrack(WindowRef);
	void UpdateConf(WindowRef);
	void UpdateGraf(WindowRef);
	
	WindowRef theWindow;
	GrafPtr savePort;
	
	theWindow = (WindowRef) theEvent->message;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	switch (GetWindowKind(theWindow)) {
	case splashKind:
		UpdateSplash(theWindow);
		break;
	case tracKind:
		UpdateTrack(theWindow);
		break;
	case confKind:
		UpdateConf(theWindow);
		break;
	case grafKind:
		UpdateGraf(theWindow);
		break;
	}
	SetPort(savePort);
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
	ActivateWindow((WindowRef) theEvent->message, (theEvent->modifiers & activeFlag) != 0);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void DoOSEvent(const EventRecord *theEvent)
{
	WindowRef theWindow;
	
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
	extern void MCPIdle(void);
	extern void IdleConf(WindowRef);
	
	WindowRef theWindow;
	GrafPtr savePort;
	
	MCPIdle();
	NotifyIdle();
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
	EventRecord theEvent;
	Boolean gotEvent;
	
	do {
		AdjustCursor();
		gotEvent = WaitNextEvent(everyEvent, &theEvent, 6, nil);
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
	if (menuList == nil)
		return;
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
	extern void GetPrefs(void);
	extern void BuildMenu(void);
	extern void MCPInit(void);
	extern void InitConf(void);
	extern void InitGrafW(void);
	extern WindowRef NewSplash(void);
	extern void UpdateSplash(WindowRef);
	
	EventRecord theEvent;
	WindowRef theSplash;
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
	
	error = Gestalt(gestaltAliasMgrAttr, &response);
	hasAliasMgr = ((error == noErr) && (response & (1 << gestaltAliasMgrPresent)));
	
	error = Gestalt(gestaltFindFolderAttr, &response);
	hasFindFolder = ((error == noErr) && (response & (1 << gestaltFindFolderPresent)));
	
	error = Gestalt(gestaltDragMgrAttr, &response);
	hasDragMgr = ((error == noErr) && (response & (1 << gestaltDragMgrPresent)));
	
	error = Gestalt(gestaltTEAttr, &response);
	hasGetHiliteRgn = ((error == noErr) && (response & (1 << gestaltTEHasGetHiliteRgn)));
	
	error = Gestalt(gestaltSoundAttr, &response);
	hasSoundInput = ((error == noErr) && (response & (1 << gestaltHasSoundInputDevice)));
	
	error = Gestalt('Aarn', &response);
	hasAaron = (error == noErr);
	
	GetPrefs();
	BuildMenu();
	
	MCPInit();
	
	InitConf();
	InitGrafW();
	
	if (gSplash && (theSplash = NewSplash()) != nil) {
		SetPortWindowPort(theSplash);
		UpdateSplash(theSplash);
	}
	
	SetUpAE();
	SetUpMenus();
	InitNotify();
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

void CleanUp(void)
{
	extern OSErr MCPClose(void);
	extern void PutPrefs(void);
	
	NotifyCleanUp();
	(void) MCPClose();
	PutPrefs();
	DisposeAllSmartScrolls();
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
