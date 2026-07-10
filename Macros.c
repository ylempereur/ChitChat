/* Macros.c */

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

#define theDialogID 208

/*————————————————————————————————————————————————————————————*/

typedef struct {
	unsigned long when;
	Str63 string;
} KNRecord, *KNPtr;

typedef struct {
	DialogRef dialog;
	ListRef list;
	KNRecord keyNav;
	short newState, editState;
} MDRecord, *MDPtr;

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

static void HiliteNew(MDPtr vars, short hiliteState)
{
	if (vars->newState != hiliteState) {
		vars->newState = hiliteState;
		SetDialogItemHilite(vars->dialog, 2, hiliteState);
	}
}

/*————————————————————————————————————————————————————————————*/

static void HiliteEdit(MDPtr vars, short hiliteState)
{
	if (vars->editState != hiliteState) {
		vars->editState = hiliteState;
		SetDialogItemHilite(vars->dialog, 3, hiliteState);
		SetDialogItemHilite(vars->dialog, 4, hiliteState);
		SetDialogItemHilite(vars->dialog, 5, hiliteState);
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

static short InsertCell(Str32 theString, ListRef lHandle)
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
			top = theCell.v + 1;
			break;
		}
	}
	theCell.v = LAddRow(1, top, lHandle);
	LSetCell(bPtr, bLen, theCell, lHandle);
	return theCell.v;
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

static void AdjustHilite(MDPtr vars)
{
	Rect dataBounds;
	Cell theCell;
	
	LGetDataBounds(&dataBounds, vars->list);
	if (dataBounds.bottom - dataBounds.top < 50)
		HiliteNew(vars, 0);
	else
		HiliteNew(vars, 255);
	if (GetSelect(&theCell, vars->list))
		HiliteEdit(vars, 0);
	else
		HiliteEdit(vars, 255);
}

/*————————————————————————————————————————————————————————————*/

static void ReadList(MDPtr vars, MLHandle macroList)
{
	Cell theCell;
	MLPtr ml;
	MacroPtr m;
	short n;
	
	LSetDrawingMode(false, vars->list);
	HLockHi((Handle) macroList);
	ml = *macroList;
	n = ml->count;
	m = ml->macros;
	while (n) {
		theCell.v = InsertCell(m->name, vars->list);
		theCell.h = 1;
		LSetCell(&m->text[1], m->text[0], theCell, vars->list);
		m++;
		n--;
	}
	HUnlock((Handle) macroList);
	LSetDrawingMode(true, vars->list);
}

/*————————————————————————————————————————————————————————————*/

static void WriteList(MDPtr vars, MLHandle macroList)
{
	Rect dataBounds;
	Cell theCell;
	MLPtr ml;
	MacroPtr m;
	short n, dataLen;
	
	LGetDataBounds(&dataBounds, vars->list);
	theCell = *(Point *) &dataBounds.top;
	n = dataBounds.bottom - dataBounds.top;
	HLock((Handle) macroList);
	ml = *macroList;
	ml->count = n;
	m = ml->macros;
	while (n) {
		theCell.h = 0;
		dataLen = 32;
		LGetCell(&m->name[1], &dataLen, theCell, vars->list);
		m->name[0] = dataLen;
		theCell.h = 1;
		dataLen = 255;
		LGetCell(&m->text[1], &dataLen, theCell, vars->list);
		m->text[0] = dataLen;
		theCell.v++;
		m++;
		n--;
	}
	HUnlock((Handle) macroList);
}

/*————————————————————————————————————————————————————————————*/

static void AddMacro(MDPtr vars)
{
	extern OSErr NewMacro(Str32, Str255);
	
	Str255 text;
	Str32 name;
	Rect r;
	Cell oldCell, newCell;
	OSErr error;
	
	error = NewMacro(name, text);
	SetCursorID(arrowCursor);
	if (error != noErr)
		return;
	LSetDrawingMode(false, vars->list);
	newCell.v = InsertCell(name, vars->list);
	newCell.h = 1;
	LSetCell(&text[1], text[0], newCell, vars->list);
	newCell.h = 0;
	if (GetSelect(&oldCell, vars->list))
		LSetSelect(false, oldCell, vars->list);
	SetSelect(newCell, vars->list);
	LSetDrawingMode(true, vars->list);
	GetDialogItemBox(vars->dialog, 7, &r);
	InvalRect(&r);
	AdjustHilite(vars);
}

/*————————————————————————————————————————————————————————————*/

static void ChangeMacro(MDPtr vars)
{
	extern OSErr ChangeText(Str255);
	
	Str255 text;
	Cell theCell;
	OSErr error;
	short dataLen;
	
	if (!GetSelect(&theCell, vars->list))
		return;
	theCell.h = 1;
	dataLen = 255;
	LGetCell(&text[1], &dataLen, theCell, vars->list);
	text[0] = dataLen;
	error = ChangeText(text);
	SetCursorID(arrowCursor);
	if (error != noErr)
		return;
	LSetCell(&text[1], text[0], theCell, vars->list);
	AdjustHilite(vars);
}

/*————————————————————————————————————————————————————————————*/

static void RenameMacro(MDPtr vars)
{
	extern OSErr ChangeName(Str32);
	
	Str255 text;
	Str32 name;
	Rect r;
	Cell theCell;
	OSErr error;
	short dataLen;
	
	if (!GetSelect(&theCell, vars->list))
		return;
	dataLen = 32;
	LGetCell(&name[1], &dataLen, theCell, vars->list);
	name[0] = dataLen;
	error = ChangeName(name);
	SetCursorID(arrowCursor);
	if (error != noErr)
		return;
	theCell.h = 1;
	dataLen = 255;
	LGetCell(&text[1], &dataLen, theCell, vars->list);
	text[0] = dataLen;
	LSetDrawingMode(false, vars->list);
	LDelRow(1, theCell.v, vars->list);
	theCell.v = InsertCell(name, vars->list);
	theCell.h = 1;
	LSetCell(&text[1], text[0], theCell, vars->list);
	theCell.h = 0;
	SetSelect(theCell, vars->list);
	LSetDrawingMode(true, vars->list);
	GetDialogItemBox(vars->dialog, 7, &r);
	InvalRect(&r);
	AdjustHilite(vars);
}

/*————————————————————————————————————————————————————————————*/

static void RemoveMacro(MDPtr vars)
{
	Cell theCell;
	
	if (!GetSelect(&theCell, vars->list))
		return;
	LDelRow(1, theCell.v, vars->list);
	AdjustHilite(vars);
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
	register MDPtr vars;
	PenState pnState;
	Rect box;
	WindowRef theWindow;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (MDPtr) GetWRefCon(theWindow);
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

static short ButtonKey(MDPtr vars, EventRecord *theEvent)
{
	short keyCode, charCode;
	
	keyCode = theEvent->message;
	charCode = keyCode & charCodeMask;
	if (charCode == CR || charCode == ETX) {
		FlashButton(vars->dialog, ok);
		return ok;
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

static void ListKey(MDPtr vars, EventRecord *theEvent)
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
	
	register MDPtr vars;
	Point thePt;
	WindowRef theWindow;
	short itemNo;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (MDPtr) GetWRefCon(theWindow);
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
		AdjustHilite(vars);
		break;
	case mouseDown:
		thePt = theEvent->where;
		GlobalToLocal(&thePt);
		itemNo = FindDialogItem(theDialog, thePt) + 1;
		if (itemNo == 7) {
			(void) LClick(thePt, theEvent->modifiers, vars->list);
			AdjustHilite(vars);
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

void EditMacros(MLHandle macroList)
{
	register MDPtr vars;
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
		return;
	theWindow = GetDialogWindow(theDialog);
	vars = (MDPtr) NewPtr(sizeof(MDRecord));
	if (vars == nil) {
		DisposeDialog(theDialog);
		return;
	}
	vars->dialog = theDialog;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	vars->list = NewList(theDialog, 7, 2);
	if (vars->list == nil) {
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeDialog(theDialog);
		return;
	}
	KNInit(&vars->keyNav);
	vars->newState = 0;
	vars->editState = 0;
	SetWRefCon(theWindow, (long) vars);
	ReadList(vars, macroList);
	uListItem = NewUserItemProc(ListItem);
	SetDialogItemProc(theDialog, 7, uListItem);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 8, uOutlineItem);
	AdjustHilite(vars);
	myFilter = NewModalFilterProc(MyFilter);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			WriteList(vars, macroList);
			error = noErr;
			break;
		case 2:
			AddMacro(vars);
			break;
		case 3:
			ChangeMacro(vars);
			break;
		case 4:
			RenameMacro(vars);
			break;
		case 5:
			RemoveMacro(vars);
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
}

/*————————————————————————————————————————————————————————————*/
