/* ChangeName.c */

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
#include <Controls.h>
#include <Windows.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 302

/*————————————————————————————————————————————————————————————*/

typedef struct {
	DialogRef dialog;
	short okState;
} CDRecord, *CDPtr;

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

/*————————————————————————————————————————————————————————————*/

static void HiliteOK(CDPtr vars, short hiliteState)
{
	if (vars->okState != hiliteState) {
		vars->okState = hiliteState;
		SetDialogItemHilite(vars->dialog, ok, hiliteState);
	}
}

/*————————————————————————————————————————————————————————————*/

static void AdjustHilite(CDPtr vars)
{
	Str32 string;
	
	NGetDialogItemText(vars->dialog, 3, string, 32);
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

static void AdjustCursor(CDPtr vars)
{
	Point mouseLoc;
	
	GetMouse(&mouseLoc);
	switch (FindDialogItem(vars->dialog, mouseLoc) + 1) {
	case 3:
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
	
	register CDPtr vars;
	TEPtr pTE;
	WindowRef theWindow;
	short keyCode, charCode, meta;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (CDPtr) GetWRefCon(theWindow);
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
		charCode = keyCode & charCodeMask;
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
				SelectDialogItemText(theDialog, 3, 0, 32767);
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
			SelectDialogItemText(theDialog, 3, 0, 32767);
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
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

OSErr ChangeName(Str32 name)
{
	register CDPtr vars;
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short itemHit;
	
	SetCursorID(watchCursor);
	CenterDialog(theDialogID);
	theDialog = GetNewDialog(theDialogID, nil, (WindowRef) -1);
	if (theDialog == nil)
		return memFullErr;
	theWindow = GetDialogWindow(theDialog);
	vars = (CDPtr) NewPtr(sizeof(CDRecord));
	if (vars == nil) {
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->dialog = theDialog;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	NSetDialogItemText(theDialog, 3, name);
	SelectDialogItemText(theDialog, 3, 0, 32767);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 5, uOutlineItem);
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
			error = noErr;
			break;
		case 2:
			error = userCanceledErr;
			break;
		case 3:
			AdjustHilite(vars);
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	SetPort(savePort);
	DisposePtr((Ptr) vars);
	DisposeDialog(theDialog);
	return error;
}

/*————————————————————————————————————————————————————————————*/
