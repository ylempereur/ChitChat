/* InviteMore.c */

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

#define theDialogID 5003

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

static Boolean GetSelect(Cell *theCell, ListRef lHandle)
{
	Rect dataBounds;
	
	LGetDataBounds(&dataBounds, lHandle);
	*theCell = *(Point *) &dataBounds.top;	// dataBounds.topLeft
	return LGetSelect(true, theCell, lHandle);
}

/*————————————————————————————————————————————————————————————*/

static void GetUserList(long cid, ListRef lHandle)
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
	error = MCPGetUserList(cid, &uid, name);
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

static OSErr InviteUsers(long cid, ListHandle lHandle)
{
	extern OSErr MCPInvite(long, long);
	
	Rect dataBounds;
	Cell theCell;
	long uid;
	short dataLen;
	Boolean gotCell;
	
	LGetDataBounds(&dataBounds, lHandle);
	theCell = *(Point *) &dataBounds.top;	// dataBounds.topLeft
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
	Point thePt;
	Cell theCell;
	WindowRef theWindow;
	short keyCode, charCode, meta, itemNo;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (IDPtr) GetWRefCon(theWindow);
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
		break;
	case mouseDown:
		thePt = theEvent->where;
		GlobalToLocal(&thePt);
		itemNo = FindDialogItem(theDialog, thePt) + 1;
		if (itemNo == 4) {
			meta = theEvent->modifiers | cmdKey;
			(void) LClick(thePt, meta, vars->list);
			if (GetSelect(&theCell, vars->list))
				HiliteOK(vars, 0);
			else
				HiliteOK(vars, 255);
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

OSErr InviteMore(long cid)
{
	register IDPtr vars;
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uListItem, uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short itemHit;
	
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
	vars->list = NewList(theDialog, 4, 2);
	if (vars->list == nil) {
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	GetUserList(cid, vars->list);
	uListItem = NewUserItemProc(ListItem);
	SetDialogItemProc(theDialog, 4, uListItem);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 5, uOutlineItem);
	HiliteOK(vars, 255);
	myFilter = NewModalFilterProc(MyFilter);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			error = InviteUsers(cid, vars->list);
			break;
		case 2:
			error = userCanceledErr;
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
