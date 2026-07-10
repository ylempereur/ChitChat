/* Logon.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Quickdraw.h>
#include <Icons.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Controls.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 200

/*————————————————————————————————————————————————————————————*/

typedef struct {
	DialogRef dialog;
	TEHandle text;
	short okState;
} LDRecord, *LDPtr;

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

/*————————————————————————————————————————————————————————————*/

static void GetXText(TEHandle hTE, Str255 text, short maxLen)
{
	unsigned char *p;
	short length;
	
	length = (*hTE)->teLength;
	if (length > 255)
		length = 255;
	if (length > maxLen)
		length = maxLen;
	p = text;
	*p++ = length;
	BlockMove(*TEGetText(hTE), p, length);
}

/*————————————————————————————————————————————————————————————*/

static void SetXText(DialogRef theDialog, short itemNo, TEHandle hTE, ConstStr255Param text)
{
	Str255 tempText;
	unsigned char *p;
	short length;
	
	p = (unsigned char *) text;
	length = *p++;
	TESetText(p, length, hTE);
	p = tempText;
	*p++ = length;
	while (--length >= 0)
		*p++ = (unsigned char) '•';
	NSetDialogItemText(theDialog, itemNo, tempText);
}

/*————————————————————————————————————————————————————————————*/

static void HiliteOK(LDPtr vars, short hiliteState)
{
	if (vars->okState != hiliteState) {
		vars->okState = hiliteState;
		SetDialogItemHilite(vars->dialog, ok, hiliteState);
	}
}

/*————————————————————————————————————————————————————————————*/

static void AdjustHilite(LDPtr vars)
{
	Str32 string;
	
	NGetDialogItemText(vars->dialog, 3, string, 32);
	if (StrLength(string) == 0) {
		HiliteOK(vars, 255);
		return;
	}
	NGetDialogItemText(vars->dialog, 4, string, 8);
	if (StrLength(string) == 0) {
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

static pascal void LineItem(DialogRef theDialog, short itemNo)
{
	PenState pnState;
	Rect box;
	
	GetDialogItemBox(theDialog, itemNo, &box);
	GetPenState(&pnState);
	PenNormal();
	PenSize(2, 2);
	FrameRect(&box);
	SetPenState(&pnState);
}

/*————————————————————————————————————————————————————————————*/

static pascal void TextItem(DialogRef theDialog, short itemNo)
{
	Str255 theString;
	Rect box;
	unsigned char *p;
	long length;
	
	GetDialogItemBox(theDialog, itemNo, &box);
	GetIndString(theString, 200, itemNo - 6);
	p = theString;
	length = *p++;
	TETextBox(p, length, &box, teJustRight);
}

/*————————————————————————————————————————————————————————————*/

static pascal void LIconItem(DialogRef theDialog, short itemNo)
{
	Rect box;
	
	GetDialogItemBox(theDialog, itemNo, &box);
	PlotIconID(&box, atNone, ttNone, 200);
}

/*————————————————————————————————————————————————————————————*/

static void AdjustCursor(LDPtr vars)
{
	Point mouseLoc;
	
	GetMouse(&mouseLoc);
	switch (FindDialogItem(vars->dialog, mouseLoc) + 1) {
	case 3:
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
	
	register LDPtr vars;
	TEPtr pTE;
	WindowRef theWindow;
	short keyCode, charCode, meta;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (LDPtr) GetWRefCon(theWindow);
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
				SelectDialogItemText(theDialog, GetDialogKeyboardFocusItem(theDialog), 0, 32767);
				theEvent->what = nullEvent;
				break;
			}
			if (charCode == 'x' || charCode == 'c' || charCode == 'v')
				if (GetDialogKeyboardFocusItem(theDialog) != 4)
					break;
			SysBeep(30);
			theEvent->what = nullEvent;
			break;
		}
		if (charCode == HT)
			break;
		if (charCode == ESC) {
			charCode = BS;
			theEvent->message = charCode;
		}
		switch (GetDialogKeyboardFocusItem(theDialog)) {
		case 3:
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
		case 4:
			if (charCode != BS) {
				if (charCode < ' ' || charCode == DEL) {
					if (charCode >= FS && charCode <= US)
						break;
					SysBeep(30);
					theEvent->what = nullEvent;
					break;
				}
				pTE = *GetDialogTextH(theDialog);
				if (pTE->teLength + pTE->selStart - pTE->selEnd + 1 > 8) {
					SysBeep(30);
					theEvent->what = nullEvent;
					break;
				}
			}
			pTE = *GetDialogTextH(theDialog);
			TESetSelect(pTE->selStart, pTE->selEnd, vars->text);
			TEKey(charCode, vars->text);
			if (charCode != BS) {
				charCode = (unsigned char) '•';
				theEvent->message = charCode;
			}
			break;
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

OSErr Logon(ConstStr32Param server, Str32 name, Str8 password)
{
	register LDPtr vars;
	Rect r;
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uIconItem, uTextItem, uLineItem, uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short itemHit;
	
	SetCursorID(watchCursor);
	CenterDialog(theDialogID);
	theDialog = GetNewDialog(theDialogID, nil, (WindowRef) -1);
	if (theDialog == nil)
		return memFullErr;
	theWindow = GetDialogWindow(theDialog);
	vars = (LDPtr) NewPtr(sizeof(LDRecord));
	if (vars == nil) {
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->dialog = theDialog;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	SetRect(&r, 0x1000, 0x1000, 0x1100, 0x1100);
	vars->text = TENew(&r, &r);
	if (vars->text == nil) {
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	ParamText(server, nil, nil, nil);
	NSetDialogItemText(theDialog, 3, name);
	SetXText(theDialog, 4, vars->text, password);
	SelectDialogItemText(theDialog, StrLength(name) ? 4 : 3, 0, 32767);
	uIconItem = NewUserItemProc(LIconItem);
	SetDialogItemProc(theDialog, 6, uIconItem);
	uTextItem = NewUserItemProc(TextItem);
	SetDialogItemProc(theDialog, 7, uTextItem);
	SetDialogItemProc(theDialog, 8, uTextItem);
	uLineItem = NewUserItemProc(LineItem);
	SetDialogItemProc(theDialog, 9, uLineItem);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 10, uOutlineItem);
	AdjustHilite(vars);
	myFilter = NewModalFilterProc(MyFilter);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			NGetDialogItemText(theDialog, 3, name, 32);
			GetXText(vars->text, password, 8);
			error = noErr;
			break;
		case 2:
			error = userCanceledErr;
			break;
		case 3:
		case 4:
			AdjustHilite(vars);
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	DisposeRoutineDescriptor(uLineItem);
	DisposeRoutineDescriptor(uTextItem);
	DisposeRoutineDescriptor(uIconItem);
	TEDispose(vars->text);
	SetPort(savePort);
	DisposePtr((Ptr) vars);
	DisposeDialog(theDialog);
	return error;
}

/*————————————————————————————————————————————————————————————*/
