/* Graphic.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <Resources.h>
#include <Folders.h>
#include <Aliases.h>
#include <Scrap.h>
#include <Quickdraw.h>
#include <Menus.h>
#include <Windows.h>
#include <AppleEvents.h>
#include <Drag.h>
#include <ToolUtils.h>
#include <TextUtils.h>
#include <StandardFile.h>

#include "Misc.h"
#include "Client.h"

/*————————————————————————————————————————————————————————————*/

typedef struct {
	Rect boundsRect;
	short procID;
	Boolean visible;
	Boolean filler1;
	Boolean goAwayFlag;
	Boolean filler2;
	long refCon;
	Str255 title;
} WindowTemplate, *WindowTPtr, **WindowTHndl;

typedef struct {
	WindowRef window, textWin;
	long cid;
	Rect destRect;
	PicHandle picture;
	long picSize;
	Rect picFrame;
	long count;
	short state;
} GFRecord, *GFPtr;

/*————————————————————————————————————————————————————————————*/

extern Str32 gUserName;
extern Boolean hasAliasMgr, hasDragMgr, hasAaron;

static DragTrackingHandlerUPP trackingHandler;
static DragReceiveHandlerUPP receiveHandler;
static Boolean canAcceptItems, cursorInContent;

/*————————————————————————————————————————————————————————————*/

#pragma segment Windows

/*————————————————————————————————————————————————————————————*/

static void CenterWindow(short windowID)
{
	WindowTHndl window;
	WindowTPtr w;
	short dh, dv;
	
	window = (WindowTHndl) GetResource('WIND', windowID);
	if (window == nil)
		return;
	HNoPurge((Handle) window);
	w = *window;
	if (hasAaron) {
		dv = qd.screenBits.bounds.top - w->boundsRect.top + GetMBarHeight() + 24;
		dh = qd.screenBits.bounds.right - w->boundsRect.right - 8;
	} else {
		dv = qd.screenBits.bounds.top - w->boundsRect.top + GetMBarHeight() + 21;
		dh = qd.screenBits.bounds.right - w->boundsRect.right - 3;
	}
	OffsetRect(&w->boundsRect, dh, dv);
}

/*————————————————————————————————————————————————————————————*/

static void MyDrawGrowIcon(GFPtr vars)
{
	Rect theRect;
	RgnHandle saveClip;
	
	saveClip = NewRgn();
	GetClip(saveClip);
	theRect = GetWindowPort(vars->window)->portRect;
	theRect.top = theRect.bottom - 15;
	theRect.left = theRect.right - 15;
	ClipRect(&theRect);
	DrawGrowIcon(vars->window);
	SetClip(saveClip);
	DisposeRgn(saveClip);
	ValidRect(&theRect);
}

/*————————————————————————————————————————————————————————————*/

static void ResizeWindow(GFPtr vars)
{
	Rect destRect, viewRect;
	long x, y;
	short vw, vh, dw, dh, dv;
	
	if (vars->state == 3) {
		viewRect = GetWindowPort(vars->window)->portRect;
		destRect = vars->picFrame;
		vw = viewRect.right - viewRect.left;
		vh = viewRect.bottom - viewRect.top;
		dw = destRect.right - destRect.left;
		dh = destRect.bottom - destRect.top;
		if (dw > vw || dh > vh) {
			x = dw * vh;
			y = dh * vw;
			if (x < y) {
				dw = (x + dh - 1) / dh;
				dh = vh;
			} else {
				dh = (y + dw - 1) / dw;
				dw = vw;
			}
			destRect.right = destRect.left + dw;
			destRect.bottom = destRect.top + dh;
		}
		dh = (viewRect.left + viewRect.right - destRect.left - destRect.right) / 2;
		dv = (viewRect.top + viewRect.bottom - destRect.top - destRect.bottom) / 2;
		OffsetRect(&destRect, dh, dv);
		vars->destRect = destRect;
	}
}

/*————————————————————————————————————————————————————————————*/

static void AdjustZoom(GFPtr vars)
{
	Rect destRect, viewRect;
	long x, y;
	short vw, vh, dw, dh;
	
	viewRect = qd.screenBits.bounds;
	if (hasAaron) {
		viewRect.top += GetMBarHeight() + 24;
		viewRect.left += 8;
		viewRect.bottom -= 8;
		viewRect.right -= 8;
	} else {
		viewRect.top += GetMBarHeight() + 21;
		viewRect.left += 3;
		viewRect.bottom -= 3;
		viewRect.right -= 3;
	}
	if (vars->state == 0) {
		destRect = viewRect;
	} else {
		dw = vars->picFrame.right - vars->picFrame.left;
		dh = vars->picFrame.bottom - vars->picFrame.top;
		if (dw < 200) dw = 200;
		if (dh < 100) dh = 100;
		*(Point *) &destRect.top = *(Point *) &GetWindowPort(vars->window)->portRect.top;
		LocalToGlobal((Point *) &destRect.top);
		destRect.right = destRect.left + dw;
		destRect.bottom = destRect.top + dh;
		if (destRect.top < viewRect.top || destRect.left < viewRect.left ||
			destRect.bottom > viewRect.bottom || destRect.right > viewRect.right) {
			vw = viewRect.right - viewRect.left;
			vh = viewRect.bottom - viewRect.top;
			if (dw > vw || dh > vh) {
				x = dw * vh;
				y = dh * vw;
				if (x < y) {
					dw = (x + dh - 1) / dh;
					dh = vh;
				} else {
					dh = (y + dw - 1) / dw;
					dw = vw;
				}
			}
			*(Point *) &destRect.top = *(Point *) &viewRect.top;
			destRect.right = destRect.left + dw;
			destRect.bottom = destRect.top + dh;
		}
	}
	SetWindowStandardState(vars->window, &destRect);
}

/*————————————————————————————————————————————————————————————*/

long GetGrafWindowCID(WindowRef theWindow)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	return vars->cid;
}

/*————————————————————————————————————————————————————————————*/

void TextWinClosed(WindowRef theWindow)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	vars->textWin = nil;
}

/*————————————————————————————————————————————————————————————*/

static void NotifyGraf(GFPtr vars, Str32 name)
{
	extern void AddToConf(WindowRef, short, const void *, long);
	
	Str63 string;
	
	if (vars->textWin == nil)
		return;
	PLstrcpy(string, "\p• ");
	PLstrcat(string, name);
	PLstrcat(string, "\p is sending a graphic...\n");
	AddToConf(vars->textWin, 2, &string[1], string[0]);
}

/*————————————————————————————————————————————————————————————*/

static pascal OSErr TrackingHandler(DragTrackingMessage message, WindowRef theWindow,
									void *refCon, DragReference theDrag)
{
	register GFPtr vars;
	HFSFlavor theHFSFlavor;
	Rect r;
	Point mouse;
	DragAttributes attributes;
	ItemReference theItem;
	FlavorFlags flavorFlags;
	RgnHandle theRgn;
	GrafPtr savePort;
	Size dataSize;
	OSErr error;
	unsigned short count;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	switch (message) {
	case dragTrackingEnterHandler:
		canAcceptItems = false;
		if (vars->state != 0 && vars->state != 3)
			break;
		error = CountDragItems(theDrag, &count);
		if (error != noErr || count != 1)
			break;
		error = GetDragItemReferenceNumber(theDrag, 1, &theItem);
		if (error != noErr)
			break;
		error = GetFlavorFlags(theDrag, theItem, 'PICT', &flavorFlags);
		if (error == noErr) {
			canAcceptItems = true;
			break;
		}
		error = GetFlavorDataSize(theDrag, theItem, flavorTypeHFS, &dataSize);
		if (error != noErr || dataSize > sizeof(HFSFlavor))
			break;
		error = GetFlavorData(theDrag, theItem, flavorTypeHFS, &theHFSFlavor, &dataSize, 0);
		if (error == noErr && theHFSFlavor.fileType == 'PICT')
			canAcceptItems = true;
		break;
	case dragTrackingEnterWindow:
		if (!canAcceptItems)
			break;
		cursorInContent = false;
		break;
	case dragTrackingInWindow:
		if (!canAcceptItems)
			break;
		GetDragAttributes(theDrag, &attributes);
		if (attributes & dragInsideSenderWindow)
			break;
		GetDragMouse(theDrag, &mouse, nil);
		GlobalToLocal(&mouse);
		r = GetWindowPort(theWindow)->portRect;
		if (PtInRect(mouse, &r)) {
			if (!cursorInContent) {
				theRgn = NewRgn();
				RectRgn(theRgn, &r);
				ShowDragHilite(theDrag, theRgn, true);
				DisposeRgn(theRgn);
				cursorInContent = true;
			}
		} else {
			if (cursorInContent) {
				HideDragHilite(theDrag);
				cursorInContent = false;
			}
		}
		break;
	case dragTrackingLeaveWindow:
		if (!canAcceptItems)
			break;
		if (cursorInContent) {
			HideDragHilite(theDrag);
			cursorInContent = false;
		}
		break;
	case dragTrackingLeaveHandler:
		break;
	}
	SetPort(savePort);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static pascal OSErr ReceiveHandler(WindowRef theWindow, void *refCon, DragReference theDrag)
{
	extern OSErr MCPStartGraf(long, long, Rect *);
	void SendGrafData(WindowRef);
	
	register GFPtr vars;
	HFSFlavor theHFSFlavor;
	Rect picFrame;
	ItemReference theItem;
	PicHandle picture;
	GrafPtr savePort;
	long cid, picSize;
	OSErr error;
	short refNum;
	Boolean targetIsFolder, wasAliased;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (!canAcceptItems || !cursorInContent)
		return dragNotAcceptedErr;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	HideDragHilite(theDrag);
	cursorInContent = false;
	SetCursorID(watchCursor);
	error = GetDragItemReferenceNumber(theDrag, 1, &theItem);
	if (error != noErr) {
		SetPort(savePort);
		return error;
	}
	error = GetFlavorDataSize(theDrag, theItem, 'PICT', &picSize);
	if (error == noErr) {
		picture = (PicHandle) NewHandle(picSize);
		if (picture == nil) {
			SetPort(savePort);
			return memFullErr;
		}
		HLock((Handle) picture);
		error = GetFlavorData(theDrag, theItem, 'PICT', *picture, &picSize, 0);
		if (error != noErr) {
			DisposeHandle((Handle) picture);
			SetPort(savePort);
			return error;
		}
		HUnlock((Handle) picture);
	} else {
		error = GetFlavorDataSize(theDrag, theItem, flavorTypeHFS, &picSize);
		if (error != noErr) {
			SetPort(savePort);
			return error;
		}
		if (picSize > sizeof(HFSFlavor)) {
			SetPort(savePort);
			return memFullErr;
		}
		error = GetFlavorData(theDrag, theItem, flavorTypeHFS, &theHFSFlavor, &picSize, 0);
		if (error != noErr) {
			SetPort(savePort);
			return error;
		}
		if (theHFSFlavor.fileType != 'PICT') {
			SetPort(savePort);
			return dragNotAcceptedErr;
		}
		if (hasAliasMgr) {
			error = ResolveAliasFile(&theHFSFlavor.fileSpec, true, &targetIsFolder, &wasAliased);
			if (error != noErr) {
				SetPort(savePort);
				return error;
			}
		}
		error = FSpOpenDF(&theHFSFlavor.fileSpec, fsRdPerm, &refNum);
		if (error != noErr) {
			SetPort(savePort);
			return error;
		}
		error = GetEOF(refNum, &picSize);
		if (error != noErr) {
			(void) FSClose(refNum);
			SetPort(savePort);
			return error;
		}
		picSize -= 512;
		if (picSize < 0) {
			(void) FSClose(refNum);
			SetPort(savePort);
			return dragNotAcceptedErr;
		}
		picture = (PicHandle) NewHandle(picSize);
		if (picture == nil) {
			(void) FSClose(refNum);
			SetPort(savePort);
			return memFullErr;
		}
		HLock((Handle) picture);
		error = SetFPos(refNum, fsFromStart, 512);
		if (error != noErr) {
			DisposeHandle((Handle) picture);
			(void) FSClose(refNum);
			SetPort(savePort);
			return error;
		}
		error = FSRead(refNum, &picSize, *picture);
		if (error != noErr) {
			DisposeHandle((Handle) picture);
			(void) FSClose(refNum);
			SetPort(savePort);
			return error;
		}
		HUnlock((Handle) picture);
		error = FSClose(refNum);
		if (error != noErr) {
			DisposeHandle((Handle) picture);
			SetPort(savePort);
			return error;
		}
	}
	picFrame = (*picture)->picFrame;
	cid = vars->cid;
	error = MCPStartGraf(cid, picSize, &picFrame);
	if (error != noErr) {
		DisposeHandle((Handle) picture);
		SetPort(savePort);
		return error;
	}
	if (vars->state != 0)
		DisposeHandle((Handle) vars->picture);
	vars->picture = picture;
	vars->picSize = picSize;
	vars->picFrame = picFrame;
	vars->count = 0;
	vars->state = 1;
	NotifyGraf(vars, gUserName);
	SendGrafData(theWindow);
	SendGrafData(theWindow);
	SendGrafData(theWindow);
	SendGrafData(theWindow);
	AdjustZoom(vars);
	InvalRect(&GetWindowPort(theWindow)->portRect);
	SetPort(savePort);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

WindowRef NewGraf(long cid, WindowRef parent)
{
	register GFPtr vars;
	Str255 title;
	WindowRef behind, theWindow;
	OSErr error;
	
	SetCursorID(watchCursor);
	CenterWindow(203);
	behind = FrontWindow();
	if (behind == nil || GetWindowKind(behind) != dialogKind)
		behind = (WindowRef) -1;
	theWindow = GetNewColorWindow(203, nil, behind);
	if (theWindow == nil)
		return nil;
	vars = (GFPtr) NewPtr(sizeof(GFRecord));
	if (vars == nil) {
		DisposeWindow(theWindow);
		return nil;
	}
	vars->window = theWindow;
	vars->textWin = parent;
	vars->cid = cid;
	vars->picture = nil;
	vars->picSize = 0;
	vars->state = 0;
	GetWTitle(parent, title);
	SetWTitle(theWindow, title);
	SetWRefCon(theWindow, (long) vars);
	if (hasDragMgr) {
		error = InstallTrackingHandler(trackingHandler, theWindow, nil);
		if (error != noErr) {
			DisposePtr((Ptr) vars);
			DisposeWindow(theWindow);
			return nil;
		}
		error = InstallReceiveHandler(receiveHandler, theWindow, nil);
		if (error != noErr) {
			RemoveTrackingHandler(trackingHandler, theWindow);
			DisposePtr((Ptr) vars);
			DisposeWindow(theWindow);
			return nil;
		}
	}
	SetWindowKind(theWindow, grafKind);
	ResizeWindow(vars);
	AdjustZoom(vars);
	ShowWindow(theWindow);
	return theWindow;
}

/*————————————————————————————————————————————————————————————*/

long CloseGraf(WindowRef theWindow)
{
	extern OSErr MCPEndGraf(long);
	extern void GrafWinClosed(WindowRef);
	
	register GFPtr vars;
	WindowRef parent;
	long cid;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	SetCursorID(watchCursor);
	HideWindow(theWindow);
	if (vars->state == 1) {
		vars->state = 0;
		(void) MCPEndGraf(vars->cid);
	}
	if (hasDragMgr) {
		RemoveReceiveHandler(receiveHandler, theWindow);
		RemoveTrackingHandler(trackingHandler, theWindow);
	}
	if (vars->state != 0)
		DisposeHandle((Handle) vars->picture);
	parent = vars->textWin;
	cid = vars->cid;
	DisposePtr((Ptr) vars);
	DisposeWindow(theWindow);
	if (parent != nil) {
		GrafWinClosed(parent);
		cid = 0;
	}
	return cid;
}

/*————————————————————————————————————————————————————————————*/

static Boolean LocationIsTrash(AEDesc *dropLocation)
{
	CInfoPBRec pb;
	AEDesc dropSpec;
	FSSpecPtr theSpec;
	long trashDirID;
	OSErr error;
	short trashVRefNum;
	
	if (dropLocation->descriptorType == typeNull)
		return false;
	error = AECoerceDesc(dropLocation, typeFSS, &dropSpec);
	if (error != noErr)
		return false;
	HLock(dropSpec.dataHandle);
	theSpec = (FSSpecPtr) *dropSpec.dataHandle;
	pb.dirInfo.ioNamePtr = theSpec->name;
	pb.dirInfo.ioVRefNum = theSpec->vRefNum;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioDrDirID = theSpec->parID;
	error = PBGetCatInfoSync(&pb);
	if (error != noErr || !(pb.dirInfo.ioFlAttrib & ioDirMask)) {
		AEDisposeDesc(&dropSpec);
		return false;
	}
	error = FindFolder(theSpec->vRefNum, kTrashFolderType, kCreateFolder,
					   &trashVRefNum, &trashDirID);
	if (error != noErr) {
		AEDisposeDesc(&dropSpec);
		return false;
	}
	AEDisposeDesc(&dropSpec);
	return pb.dirInfo.ioDrDirID == trashDirID;
}

/*————————————————————————————————————————————————————————————*/

static Boolean DragGraf(GFPtr vars, const EventRecord *theEvent)
{
	Rect r;
	AEDesc dropLocation;
	DragReference theDrag;
	DragAttributes attributes;
	RgnHandle dragRgn, tempRgn;
	PicHandle picture;
	long picSize;
	OSErr error;
	short downMods, upMods;
	
	if (!hasDragMgr)
		return false;
	if (!WaitMouseMoved(theEvent->where))
		return false;
	error = NewDrag(&theDrag);
	if (error != noErr)
		return false;
	picture = vars->picture;
	picSize = vars->picSize;
	HLock((Handle) picture);
	error = AddDragItemFlavor(theDrag, 1, 'PICT', *picture, picSize, 0);
	if (error != noErr) {
		HUnlock((Handle) picture);
		DisposeDrag(theDrag);
		return false;
	}
	HUnlock((Handle) picture);
	r = vars->destRect;
	LocalToGlobal((Point *) &r.top);
	LocalToGlobal((Point *) &r.bottom);
	error = SetDragItemBounds(theDrag, 1, &r);
	if (error != noErr) {
		DisposeDrag(theDrag);
		return false;
	}
	dragRgn = NewRgn();
	RectRgn(dragRgn, &r);
	tempRgn = NewRgn();
	CopyRgn(dragRgn, tempRgn);
	InsetRgn(tempRgn, 1, 1);
	DiffRgn(dragRgn, tempRgn, dragRgn);
	DisposeRgn(tempRgn);
	error = TrackDrag(theDrag, theEvent, dragRgn);
	if (error != noErr) {
		DisposeRgn(dragRgn);
		DisposeDrag(theDrag);
		return error == userCanceledErr;
	}
	error = GetDragAttributes(theDrag, &attributes);
	if (error == noErr && !(attributes & dragInsideSenderApplication)) {
		error = GetDragModifiers(theDrag, nil, &downMods, &upMods);
		if (error == noErr && !((downMods | upMods) & optionKey)) {
			error = GetDropLocation(theDrag, &dropLocation);
			if (error == noErr) {
				if (LocationIsTrash(&dropLocation)) {
					if (vars->state != 0)
						DisposeHandle((Handle) vars->picture);
					vars->state = 0;
					AdjustZoom(vars);
					InvalRect(&GetWindowPort(vars->window)->portRect);
				}
				AEDisposeDesc(&dropLocation);
			}
		}
	}
	DisposeRgn(dragRgn);
	DisposeDrag(theDrag);
	return true;
}

/*————————————————————————————————————————————————————————————*/

void ClickGraf(WindowRef theWindow, const EventRecord *theEvent)
{
	register GFPtr vars;
	Point pt;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (vars->state != 3)
		return;
	pt = theEvent->where;
	GlobalToLocal(&pt);
	if (!PtInRect(pt, &vars->destRect))
		return;
	(void) DragGraf(vars, theEvent);
}

/*————————————————————————————————————————————————————————————*/

void BackClickGraf(WindowRef theWindow, const EventRecord *theEvent)
{
	register GFPtr vars;
	Point pt;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (vars->state != 3) {
		SelectWindow(theWindow);
		return;
	}
	pt = theEvent->where;
	GlobalToLocal(&pt);
	if (!PtInRect(pt, &vars->destRect)) {
		SelectWindow(theWindow);
		return;
	}
	if (!DragGraf(vars, theEvent))
		SelectWindow(theWindow);
}

/*————————————————————————————————————————————————————————————*/

void KeyGraf(WindowRef theWindow, long code)
{
	SysBeep(30);
}

/*————————————————————————————————————————————————————————————*/

void UpdateGraf(WindowRef theWindow)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	BeginUpdate(theWindow);
	EraseRect(&GetWindowPort(theWindow)->portRect);
	if (vars->state == 3) {
		DrawPicture(vars->picture, &vars->destRect);
	}
	MyDrawGrowIcon(vars);
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/

void ActivateGraf(WindowRef theWindow, Boolean theFlag)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	MyDrawGrowIcon(vars);
}

/*————————————————————————————————————————————————————————————*/

void GrowGraf(WindowRef theWindow, Point mouseLoc)
{
	register GFPtr vars;
	Rect theRect;
	CGrafPtr windowPort;
	long newSize;
	short w, h;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	SetRect(&theRect, 200, 100, 32000, 32000);
	newSize = GrowWindow(theWindow, mouseLoc, &theRect);
	if (newSize != 0) {
		windowPort = GetWindowPort(theWindow);
		EraseRect(&windowPort->portRect);
		h = HiWord(newSize);
		w = LoWord(newSize);
		SizeWindow(theWindow, w, h, false);
		ResizeWindow(vars);
		InvalRect(&windowPort->portRect);
	}
}

/*————————————————————————————————————————————————————————————*/

void ZoomGraf(WindowRef theWindow, short partCode)
{
	register GFPtr vars;
	CGrafPtr windowPort;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (partCode == inZoomOut)
		AdjustZoom(vars);
	windowPort = GetWindowPort(theWindow);
	EraseRect(&windowPort->portRect);
	ZoomWindow(theWindow, partCode, true);
	ResizeWindow(vars);
	InvalRect(&windowPort->portRect);
}

/*————————————————————————————————————————————————————————————*/

void AdjFileGraf(WindowRef theWindow, MenuRef theMenu)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	EnableItem(theMenu, CloseItem);
	if (vars->state == 3)
		EnableItem(theMenu, SaveAsItem);
	else
		DisableItem(theMenu, SaveAsItem);
	DisableItem(theMenu, PrintItem);
}

/*————————————————————————————————————————————————————————————*/

void AdjEditGraf(WindowRef theWindow, MenuRef theMenu)
{
	register GFPtr vars;
	long offset;
	short state;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	state = vars->state;
	DisableItem(theMenu, UndoItem);
	if (state == 3) {
		EnableItem(theMenu, CutItem);
		EnableItem(theMenu, CopyItem);
		EnableItem(theMenu, ClearItem);
	} else {
		DisableItem(theMenu, CutItem);
		DisableItem(theMenu, CopyItem);
		DisableItem(theMenu, ClearItem);
	}
	if ((state == 0 || state == 3) && GetScrap(nil, 'PICT', &offset) > 0)
		EnableItem(theMenu, PasteItem);
	else
		DisableItem(theMenu, PasteItem);
	DisableItem(theMenu, SelectAllItem);
}

/*————————————————————————————————————————————————————————————*/

void SaveGraf(WindowRef theWindow)
{
	register GFPtr vars;
	Str255 prompt, defaultName;
	StandardFileReply reply;
	RgnHandle updateRgn;
	Ptr header;
	PicHandle picture;
	long count, picSize;
	OSErr error;
	short refNum;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	GetIndString(prompt, 300, 2);
	GetWTitle(vars->window, defaultName);
	SetCursorID(arrowCursor);
	StandardPutFile(prompt, defaultName, &reply);
	if (!reply.sfGood)
		return;
	SetCursorID(watchCursor);
	updateRgn = NewRgn();
	GetWindowUpdateRgn(vars->window, updateRgn);
	if (!EmptyRgn(updateRgn))
		UpdateGraf(vars->window);
	DisposeRgn(updateRgn);
	if (reply.sfReplacing) {
		error = FSpDelete(&reply.sfFile);
		if (error != noErr) {
			SysBeep(30);
			return;
		}
	}
	error = FSpCreate(&reply.sfFile, 'ttxt', 'PICT', reply.sfScript);
	if (error != noErr) {
		SysBeep(30);
		return;
	}
	error = FSpOpenDF(&reply.sfFile, fsWrPerm, &refNum);
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	header = NewPtrClear(512);
	if (header == nil) {
		(void) FSClose(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	count = 512;
	error = FSWrite(refNum, &count, header);
	if (error != noErr) {
		DisposePtr(header);
		(void) FSClose(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	DisposePtr(header);
	picture = vars->picture;
	picSize = vars->picSize;
	HLock((Handle) picture);
	error = FSWrite(refNum, &picSize, *picture);
	if (error != noErr) {
		HUnlock((Handle) picture);
		(void) FSClose(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	HUnlock((Handle) picture);
	error = FSClose(refNum);
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
}

/*————————————————————————————————————————————————————————————*/

void SendGrafData(WindowRef theWindow)
{
	extern OSErr MCPGrafData(long, Ptr, long);
	extern OSErr MCPEndGraf(long);
	
	register GFPtr vars;
	Byte data[500];
	Ptr srcPtr;
	long count, length;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (vars->state != 1)
		return;
	count = vars->count;
	length = vars->picSize - count;
	if (length == 0) {
		vars->state = 3;
		(void) MCPEndGraf(vars->cid);
		ResizeWindow(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		return;
	}
	if (length > 500) length = 500;
	srcPtr = (Ptr) *vars->picture + count;
	BlockMove(srcPtr, data, length);
	vars->count += length;
	(void) MCPGrafData(vars->cid, (Ptr) data, length);
}

/*————————————————————————————————————————————————————————————*/

void EditGraf(WindowRef theWindow, short menuItem)
{
	extern OSErr MCPStartGraf(long, long, Rect *);
	
	register GFPtr vars;
	Rect picFrame;
	PicHandle picture;
	long cid, picSize, offset;
	OSErr error;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	switch (menuItem) {
	case CutItem:
		error = ZeroScrap();
		if (error != noErr) {
			SysBeep(30);
			break;
		}
		picture = vars->picture;
		picSize = vars->picSize;
		HLock((Handle) picture);
		error = PutScrap(picSize, 'PICT', *picture);
		if (error != noErr) {
			HUnlock((Handle) picture);
			SysBeep(30);
			break;
		}
		if (vars->state != 0)
			DisposeHandle((Handle) picture);
		vars->state = 0;
		AdjustZoom(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		break;
	case CopyItem:
		error = ZeroScrap();
		if (error != noErr) {
			SysBeep(30);
			break;
		}
		picture = vars->picture;
		picSize = vars->picSize;
		HLock((Handle) picture);
		error = PutScrap(picSize, 'PICT', *picture);
		if (error != noErr) {
			HUnlock((Handle) picture);
			SysBeep(30);
			break;
		}
		HUnlock((Handle) picture);
		break;
	case PasteItem:
		SetCursorID(watchCursor);
		picture = (PicHandle) NewHandle(0);
		if (picture == nil) {
			SysBeep(30);
			break;
		}
		picSize = GetScrap((Handle) picture, 'PICT', &offset);
		if (picSize < 0) {
			DisposeHandle((Handle) picture);
			SysBeep(30);
			break;
		}
		picFrame = (*picture)->picFrame;
		cid = vars->cid;
		error = MCPStartGraf(cid, picSize, &picFrame);
		if (error != noErr) {
			DisposeHandle((Handle) picture);
			SysBeep(30);
			break;
		}
		if (vars->state != 0)
			DisposeHandle((Handle) vars->picture);
		vars->picture = picture;
		vars->picSize = picSize;
		vars->picFrame = picFrame;
		vars->count = 0;
		vars->state = 1;
		NotifyGraf(vars, gUserName);
		SendGrafData(theWindow);
		SendGrafData(theWindow);
		SendGrafData(theWindow);
		SendGrafData(theWindow);
		AdjustZoom(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		break;
	case ClearItem:
		if (vars->state != 0)
			DisposeHandle((Handle) vars->picture);
		vars->state = 0;
		AdjustZoom(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

void NewPict(WindowRef theWindow, long picSize, Rect *picFrame, Str32 name)
{
	register GFPtr vars;
	PicHandle picture;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	picture = (PicHandle) NewHandle(picSize);
	if (picture == nil)
		return;
	if (vars->state != 0)
		DisposeHandle((Handle) vars->picture);
	vars->picture = picture;
	vars->picSize = picSize;
	vars->picFrame = *picFrame;
	vars->count = 0;
	vars->state = 2;
	NotifyGraf(vars, name);
	AdjustZoom(vars);
	InvalRect(&GetWindowPort(theWindow)->portRect);
}

/*————————————————————————————————————————————————————————————*/

void PictData(WindowRef theWindow, Ptr data, long length)
{
	register GFPtr vars;
	Ptr dstPtr;
	long count;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (vars->state != 2)
		return;
	count = vars->count;
	if (length > vars->picSize - count) {
		DisposeHandle((Handle) vars->picture);
		vars->state = 0;
		AdjustZoom(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		return;
	}
	dstPtr = (Ptr) *vars->picture + count;
	BlockMove(data, dstPtr, length);
	vars->count += length;
}

/*————————————————————————————————————————————————————————————*/

void EndPict(WindowRef theWindow)
{
	register GFPtr vars;
	
	vars = (GFPtr) GetWRefCon(theWindow);
	if (vars->state != 2)
		return;
	if (vars->count != vars->picSize) {
		DisposeHandle((Handle) vars->picture);
		vars->state = 0;
		AdjustZoom(vars);
		InvalRect(&GetWindowPort(theWindow)->portRect);
		return;
	}
	vars->state = 3;
	ResizeWindow(vars);
	InvalRect(&GetWindowPort(theWindow)->portRect);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

/*————————————————————————————————————————————————————————————*/

void InitGrafW(void)
{
	if (hasDragMgr) {
		trackingHandler = NewDragTrackingHandlerProc(TrackingHandler);
		receiveHandler = NewDragReceiveHandlerProc(ReceiveHandler);
	}
}

/*————————————————————————————————————————————————————————————*/
