/* Invite.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <Memory.h>
#include <Quickdraw.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Lists.h>
#include <Controls.h>
#include <Windows.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 5001

/*————————————————————————————————————————————————————————————*/

typedef struct {
	DialogRef dialog;
	ListRef list;
	short okState;
} IDRecord, *IDPtr;

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

/*————————————————————————————————————————————————————————————*/

static void HiliteOK(IDPtr vars, short hiliteState)
{
	if (vars->okState != hiliteState) {
		vars->okState = hiliteState;
		SetDialogItemHilite(vars->dialog, ok, hiliteState);
	}
}

/*————————————————————————————————————————————————————————————*/

static ListRef NewList(DialogRef theDialog, short itemNo, short colCount)
{
	Rect rView, dataBounds;
	Point cSize;
	WindowRef theWindow;
	ListRef lHandle;
	
	GetDialogItemBox(theDialog, itemNo, &rView);
	InsetRect(&rView, 1, 1);
	rView.right -= 15;
	SetRect(&dataBounds, 0, 0, colCount, 0);
	cSize.v = 0;
	cSize.h = rView.right - rView.left;
	theWindow = GetDialogWindow(theDialog);
	lHandle = LNew(&rView, &dataBounds, cSize, 0, theWindow, true, false, false, true);
	if (lHandle != nil)
		LSetSelFlags(lNoNilHilite, lHandle);
	return lHandle;
}

/*————————————————————————————————————————————————————————————*/

static Boolean InsertCell(Str32 theString, short *rowNum, ListRef lHandle)
{
	register unsigned char *aPtr, *bPtr;
	unsigned char data[32];
	Rect dataBounds;
	Cell theCell;
	short aLen, bLen, top, bottom, n;
	
	bPtr = theString;
	aPtr = data;
	bLen = *bPtr++;
	LGetDataBounds(&dataBounds, lHandle);
	top = dataBounds.top;
	bottom = dataBounds.bottom;
	theCell.h = 0;
	while (top != bottom) {
		theCell.v = (top + bottom) / 2;
		aLen = 32;
		LGetCell(aPtr, &aLen, theCell, lHandle);
		n = CompareText(aPtr, bPtr, aLen, bLen, nil);
		if (n > 0)
			bottom = theCell.v;
		else if (n < 0)
			top = theCell.v + 1;
		else {
			*rowNum = theCell.v;
			return false;
		}
	}
	theCell.v = LAddRow(1, top, lHandle);
	LSetCell(bPtr, bLen, theCell, lHandle);
	*rowNum = theCell.v;
	return true;
}

/*————————————————————————————————————————————————————————————*/

static void GetUserList(ListRef lHandle)
{
	extern OSErr MCPGetUserList(long, long *, Str32);
	extern OSErr MCPGetNextUser(long *, Str32);
	
	Str32 name;
	Rect r;
	Cell theCell;
	long uid;
	OSErr error;
	
	LSetDrawingMode(false, lHandle);
	theCell.h = 1;
	error = MCPGetUserList(0, &uid, name);
	while (error == noErr) {
		(void) InsertCell(name, &theCell.v, lHandle);
		LSetCell(&uid, sizeof uid, theCell, lHandle);
		error = MCPGetNextUser(&uid, name);
	}
	LSetDrawingMode(true, lHandle);
	LGetViewRect(&r, lHandle);
	InvalRect(&r);
}

/*————————————————————————————————————————————————————————————*/

static OSErr CreateConf(Boolean private, Str32 title, ListRef lHandle)
{
	extern OSErr MCPNewConf(Boolean, Str32, long *);
	extern OSErr MCPLeave(long);
	extern OSErr MCPInvite(long, long);
	extern WindowRef NewConf(long, Str255);
	
	Rect dataBounds;
	Cell theCell;
	WindowRef theWindow;
	long cid, uid;
	OSErr error;
	short dataLen;
	Boolean gotCell;
	
	error = MCPNewConf(private, title, &cid);
	if (error != noErr)
		return error;
	theWindow = NewConf(cid, title);
	if (theWindow == nil) {
		(void) MCPLeave(cid);
		return memFullErr;
	}
	LGetDataBounds(&dataBounds, lHandle);
	theCell = *(Point *) &dataBounds.top;	/* dataBounds.topLeft */
	do {
		gotCell = LGetSelect(true, &theCell, lHandle);
		if (gotCell) {
			theCell.h = 1;
			dataLen = sizeof uid;
			LGetCell(&uid, &dataLen, theCell, lHandle);
			(void) MCPInvite(cid, uid);
		}
	} while (gotCell);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static void AdjustHilite(IDPtr vars)
{
	Str32 title;
	
	NGetDialogItemText(vars->dialog, 4, title, 32);
	if (StrLength(title) == 0) {
		HiliteOK(vars, 255);
		return;
	}
	HiliteOK(vars, 0);
}

/*————————————————————————————————————————————————————————————*/

static pascal void OutlineItem(DialogRef theDialog, short itemNo)
{
	PenState pnState;
	Rect box;
	
	GetDialogItemBox(theDialog, ok, &box);
	GetPenState(&pnState);
	PenNormal();
	PenSize(3, 3);
	InsetRect(&box, -4, -4);
	FrameRoundRect(&box, 16, 16);
	SetPenState(&pnState);
}

/*————————————————————————————————————————————————————————————*/

static pascal void ListItem(DialogRef theDialog, short itemNo)
{
	register IDPtr vars;
	PenState pnState;
	Rect box;
	WindowRef theWindow;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (IDPtr) GetWRefCon(theWindow);
	GetDialogItemBox(theDialog, itemNo, &box);
	GetPenState(&pnState);
	PenNormal();
	FrameRect(&box);
	LUpdate(GetWindowPort(theWindow)->visRgn, vars->list);
	SetPenState(&pnState);
}

/*————————————————————————————————————————————————————————————*/

static void AdjustCursor(IDPtr vars)
{
	Point mouseLoc;
	
	GetMouse(&mouseLoc);
	switch (FindDialogItem(vars->dialog, mouseLoc) + 1) {
	case 4:
		SetCursorID(iBeamCursor);
		break;
	default:
		SetCursorID(arrowCursor);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

static void FlashButton(DialogRef theDialog, short itemNo)
{
	Rect box;
	Handle item;
	long finalTicks;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	HiliteControl((ControlRef) item, inButton);
	Delay(8, &finalTicks);
	HiliteControl((ControlRef) item, 0);
}

/*————————————————————————————————————————————————————————————*/

static pascal Boolean MyFilter(DialogRef theDialog, EventRecord *theEvent, short *itemHit)
{
	extern void MCPIdle(void);
	extern void DoActivate(const EventRecord *);
	extern void DoUpdate(const EventRecord *);
	
	register IDPtr vars;
	TEPtr pTE;
	Point thePt;
	WindowRef theWindow;
	short keyCode, charCode, meta, itemNo;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (IDPtr) GetWRefCon(theWindow);
	AdjustCursor(vars);
	switch (theEvent->what) {
	case nullEvent:
		MCPIdle();
		break;
	case activateEvt:
		if ((WindowRef) theEvent->message != theWindow) {
			DoActivate(theEvent);
			theEvent->what = nullEvent;
		}
		break;
	case updateEvt:
		if ((WindowRef) theEvent->message != theWindow) {
			DoUpdate(theEvent);
			theEvent->what = nullEvent;
		}
		break;
	case keyDown:
	case autoKey:
		keyCode = theEvent->message;
		charCode = (unsigned char) keyCode;
		meta = theEvent->modifiers;
		if (charCode == CR || charCode == ETX) {
			if (vars->okState == 0) {
				FlashButton(theDialog, ok);
				*itemHit = ok;
				return true;
			}
			theEvent->what = nullEvent;
			break;
		}
		if (keyCode == 0x351B || charCode == '.' && meta & cmdKey) {
			FlashButton(theDialog, cancel);
			*itemHit = cancel;
			return true;
		}
		if (meta & cmdKey) {
			if (charCode == 'a') {
				SelectDialogItemText(theDialog, 4, 0, 32767);
				theEvent->what = nullEvent;
				break;
			}
			if (charCode == 'x' || charCode == 'c' || charCode == 'v')
				break;
			SysBeep(30);
			theEvent->what = nullEvent;
			break;
		}
		if (charCode == HT) {
			SelectDialogItemText(theDialog, 4, 0, 32767);
			theEvent->what = nullEvent;
			break;
		}
		if (charCode == ESC) {
			charCode = BS;
			theEvent->message = charCode;
		}
		if (charCode != BS) {
			if (charCode < ' ' || charCode == DEL) {
				if (charCode >= FS && charCode <= US)
					break;
				SysBeep(30);
				theEvent->what = nullEvent;
				break;
			}
			pTE = *GetDialogTextH(theDialog);
			if (pTE->teLength + pTE->selStart - pTE->selEnd + 1 > 32) {
				SysBeep(30);
				theEvent->what = nullEvent;
				break;
			}
		}
		break;
	case mouseDown:
		thePt = theEvent->where;
		GlobalToLocal(&thePt);
		itemNo = FindDialogItem(theDialog, thePt) + 1;
		if (itemNo == 7) {
			meta = theEvent->modifiers | cmdKey;
			(void) LClick(thePt, meta, vars->list);
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

OSErr Invite(void)
{
	register IDPtr vars;
	Str32 title;
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uListItem, uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short itemHit;
	Boolean private;
	
	SetCursorID(watchCursor);
	CenterDialog(theDialogID);
	theDialog = GetNewDialog(theDialogID, nil, (WindowRef) -1);
	if (theDialog == nil)
		return memFullErr;
	theWindow = GetDialogWindow(theDialog);
	vars = (IDPtr) NewPtr(sizeof(IDRecord));
	if (vars == nil) {
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->dialog = theDialog;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	vars->list = NewList(theDialog, 7, 2);
	if (vars->list == nil) {
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	GetUserList(vars->list);
	SetDialogItemValue(theDialog, 3, 1);
	uListItem = NewUserItemProc(ListItem);
	SetDialogItemProc(theDialog, 7, uListItem);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 8, uOutlineItem);
	SelectDialogItemText(theDialog, 4, 0, 32767);
	AdjustHilite(vars);
	myFilter = NewModalFilterProc(MyFilter);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			private = GetDialogItemValue(theDialog, 3) != 0;
			NGetDialogItemText(theDialog, 4, title, 32);
			error = CreateConf(private, title, vars->list);
			break;
		case 2:
			error = userCanceledErr;
			break;
		case 3:
			SetDialogItemValue(theDialog, 3, 1 - GetDialogItemValue(theDialog, 3));
			break;
		case 4:
			AdjustHilite(vars);
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	DisposeRoutineDescriptor(uListItem);
	LDispose(vars->list);
	SetPort(savePort);
	DisposePtr((Ptr) vars);
	DisposeDialog(theDialog);
	return error;
}

/*————————————————————————————————————————————————————————————*/
