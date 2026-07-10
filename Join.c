/* Join.c */

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
#include <LowMem.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 202

/*————————————————————————————————————————————————————————————*/

typedef struct {
	unsigned long when;
	Str63 string;
} KNRecord, *KNPtr;

typedef struct {
	DialogRef dialog;
	ListRef list;
	KNRecord keyNav;
	short okState;
} JDRecord, *JDPtr;

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

/*————————————————————————————————————————————————————————————*/

static void KNInit(KNPtr pKN)
{
	pKN->when = 0;
	pKN->string[0] = 0;
}

/*————————————————————————————————————————————————————————————*/

static Boolean KNKey(EventRecord *theEvent, KNPtr pKN)
{
	short charCode, keyThresh;
	
	if (theEvent->what != keyDown)
		return false;
	charCode = theEvent->message & charCodeMask;
	if (theEvent->modifiers & cmdKey || charCode == BS || charCode == ESC) {
		KNInit(pKN);
		return false;
	}
	if (StrLength(pKN->string)) {
		keyThresh = LMGetKeyThresh();
		if (keyThresh > 60)
			keyThresh = 60;
		if (theEvent->when - pKN->when > keyThresh * 2)
			KNInit(pKN);
	}
	if (StrLength(pKN->string) >= 63)
		return false;
	pKN->string[++pKN->string[0]] = charCode;
	pKN->when = theEvent->when;
	return true;
}

/*————————————————————————————————————————————————————————————*/

static short KNCompString(Str255 theString, KNPtr pKN)
{
	register unsigned char *aPtr, *bPtr;
	short aLen, bLen;
	
	aPtr = pKN->string;
	bPtr = theString;
	aLen = *aPtr++;
	bLen = *bPtr++;
	if (aLen == bLen && IdenticalText(aPtr, bPtr, aLen, bLen, nil) == 0)
		return 0;
	return CompareText(aPtr, bPtr, aLen, bLen, nil);
}

/*————————————————————————————————————————————————————————————*/

static void HiliteOK(JDPtr vars, short hiliteState)
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
		LSetSelFlags(lOnlyOne + lNoNilHilite, lHandle);
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

static void SetSelect(Cell theCell, ListRef lHandle)
{
	Rect r;
	
	LGetVisible(&r, lHandle);
	if (!PtInRect(theCell, &r))
		LScroll(0, theCell.v - r.top, lHandle);
	LSetSelect(true, theCell, lHandle);
}

/*————————————————————————————————————————————————————————————*/

static void SetSelectSeq(Cell theCell, ListRef lHandle)
{
	Rect r;
	short top, bottom;
	
	LGetVisible(&r, lHandle);
	top = r.top;
	bottom = r.bottom - 1;
	if (theCell.v < top)
		LScroll(0, theCell.v - top, lHandle);
	else if (theCell.v > bottom)
		LScroll(0, theCell.v - bottom, lHandle);
	LSetSelect(true, theCell, lHandle);
}

/*————————————————————————————————————————————————————————————*/

static void GetConfList(ListRef lHandle)
{
	extern OSErr MCPGetConfList(long *, Str32);
	extern OSErr MCPGetNextConf(long *, Str32);
	
	Str32 title;
	Rect r;
	Cell theCell;
	long cid;
	OSErr error;
	
	LSetDrawingMode(false, lHandle);
	theCell.h = 1;
	error = MCPGetConfList(&cid, title);
	while (error == noErr) {
		(void) InsertCell(title, &theCell.v, lHandle);
		LSetCell(&cid, sizeof cid, theCell, lHandle);
		error = MCPGetNextConf(&cid, title);
	}
	LSetDrawingMode(true, lHandle);
	LGetViewRect(&r, lHandle);
	InvalRect(&r);
}

/*————————————————————————————————————————————————————————————*/

static OSErr JoinConf(ListRef lHandle)
{
	extern OSErr MCPJoin(long);
	extern OSErr MCPLeave(long);
	extern WindowRef NewConf(long, Str255);
	
	Str32 title;
	Cell theCell;
	WindowRef theWindow;
	long cid;
	OSErr error;
	short dataLen;
	
	if (!GetSelect(&theCell, lHandle))
		return paramErr;
	dataLen = 32;
	LGetCell(&title[1], &dataLen, theCell, lHandle);
	title[0] = dataLen;
	theCell.h = 1;
	dataLen = sizeof cid;
	LGetCell(&cid, &dataLen, theCell, lHandle);
	error = MCPJoin(cid);
	if (error != noErr)
		return error;
	theWindow = NewConf(cid, title);
	if (theWindow == nil) {
		(void) MCPLeave(cid);
		return memFullErr;
	}
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
	register JDPtr vars;
	PenState pnState;
	Rect box;
	WindowRef theWindow;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (JDPtr) GetWRefCon(theWindow);
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

static short ButtonKey(JDPtr vars, EventRecord *theEvent)
{
	short keyCode, charCode;
	
	keyCode = theEvent->message;
	charCode = keyCode & charCodeMask;
	if (charCode == CR || charCode == ETX) {
		if (vars->okState == 0) {
			FlashButton(vars->dialog, ok);
			return ok;
		}
	} else if (keyCode == 0x351B || charCode == '.' && theEvent->modifiers & cmdKey) {
		FlashButton(vars->dialog, cancel);
		return cancel;
	}
	return 0;
}

/*————————————————————————————————————————————————————————————*/

static void ListArrow(short charCode, ListRef lHandle)
{
	Rect dataBounds;
	Cell theCell;
	short top, bottom;
	
	LGetDataBounds(&dataBounds, lHandle);
	top = dataBounds.top;
	bottom = dataBounds.bottom - 1;
	if (!GetSelect(&theCell, lHandle)) {
		theCell.h = dataBounds.left;
		if (charCode == US)
			theCell.v = top;
		else
			theCell.v = bottom;
	} else {
		if (theCell.v == top && charCode == RS)
			return;
		if (theCell.v == bottom && charCode == US)
			return;
		LSetSelect(false, theCell, lHandle);
		if (charCode == RS)
			theCell.v--;
		else
			theCell.v++;
	}
	SetSelectSeq(theCell, lHandle);
}

/*————————————————————————————————————————————————————————————*/

static void ListKeyNav(KNPtr pKN, ListRef lHandle)
{
	Str32 theString;
	Rect dataBounds;
	Cell oldCell, newCell;
	short dataLen;
	
	LGetDataBounds(&dataBounds, lHandle);
	newCell = *(Point *) &dataBounds.top;	// dataBounds.topLeft
	do {
		dataLen = 32;
		LGetCell(&theString[1], &dataLen, newCell, lHandle);
		theString[0] = dataLen;
		if (KNCompString(theString, pKN) <= 0)
			break;
		newCell.v++;
		if (newCell.v == dataBounds.bottom) {
			newCell.v--;
			break;
		}
	} while (true);
	if (GetSelect(&oldCell, lHandle)) {
		if (EqualPt(newCell, oldCell))
			return;
		LSetSelect(false, oldCell, lHandle);
	}
	SetSelect(newCell, lHandle);
}

/*————————————————————————————————————————————————————————————*/

static void ListKey(JDPtr vars, EventRecord *theEvent)
{
	Rect dataBounds;
	short charCode;
	
	charCode = theEvent->message & charCodeMask;
	if (charCode == HT) {
		KNInit(&vars->keyNav);
		return;
	}
	LGetDataBounds(&dataBounds, vars->list);
	if (dataBounds.top == dataBounds.bottom)
		return;
	if (charCode == RS || charCode == US) {
		ListArrow(charCode, vars->list);
		KNInit(&vars->keyNav);
		return;
	}
	if (theEvent->what == autoKey)
		return;
	if (KNKey(theEvent, &vars->keyNav))
		ListKeyNav(&vars->keyNav, vars->list);
}

/*————————————————————————————————————————————————————————————*/

static pascal Boolean MyFilter(DialogRef theDialog, EventRecord *theEvent, short *itemHit)
{
	extern void MCPIdle(void);
	extern void DoActivate(const EventRecord *);
	extern void DoUpdate(const EventRecord *);
	
	register JDPtr vars;
	Point thePt;
	Cell theCell;
	WindowRef theWindow;
	short itemNo;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (JDPtr) GetWRefCon(theWindow);
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
		itemNo = ButtonKey(vars, theEvent);
		if (itemNo != 0) {
			*itemHit = itemNo;
			return true;
		}
		ListKey(vars, theEvent);
		if (GetSelect(&theCell, vars->list))
			HiliteOK(vars, 0);
		else
			HiliteOK(vars, 255);
		break;
	case mouseDown:
		thePt = theEvent->where;
		GlobalToLocal(&thePt);
		itemNo = FindDialogItem(theDialog, thePt) + 1;
		if (itemNo == 4) {
			if (LClick(thePt, theEvent->modifiers, vars->list)) {
				FlashButton(theDialog, ok);
				*itemHit = ok;
				return true;
			}
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

OSErr Join(void)
{
	register JDPtr vars;
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
	vars = (JDPtr) NewPtr(sizeof(JDRecord));
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
	KNInit(&vars->keyNav);
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	GetConfList(vars->list);
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
			error = JoinConf(vars->list);
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
