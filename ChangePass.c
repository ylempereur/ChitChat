/* ChangePass.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <OSUtils.h>
#include <TextUtils.h>
#include <Events.h>
#include <Controls.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 201

/*————————————————————————————————————————————————————————————*/

typedef struct {
	DialogRef dialog;
	TEHandle text[3];
	short okState;
} CPRecord, *CPPtr;

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

static void HiliteOK(CPPtr vars, short hiliteState)
{
	if (vars->okState != hiliteState) {
		vars->okState = hiliteState;
		SetDialogItemHilite(vars->dialog, ok, hiliteState);
	}
}

/*————————————————————————————————————————————————————————————*/

static void AdjustHilite(CPPtr vars)
{
	Str8 string, vString;
	
	NGetDialogItemText(vars->dialog, 3, string, 8);
	if (StrLength(string) == 0) {
		HiliteOK(vars, 255);
		return;
	}
	NGetDialogItemText(vars->dialog, 4, string, 8);
	if (StrLength(string) == 0) {
		HiliteOK(vars, 255);
		return;
	}
	NGetDialogItemText(vars->dialog, 5, string, 8);
	if (StrLength(string) == 0) {
		HiliteOK(vars, 255);
		return;
	}
	GetXText(vars->text[1], string, 8);
	GetXText(vars->text[2], vString, 8);
	if (!EqualString(string, vString, true, true)) {
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

static pascal void TextItem(DialogRef theDialog, short itemNo)
{
	Str255 theString;
	Rect box;
	unsigned char *p;
	long length;
	
	GetDialogItemBox(theDialog, itemNo, &box);
	GetIndString(theString, 201, itemNo - 6);
	p = theString;
	length = *p++;
	TETextBox(p, length, &box, teJustRight);
}

/*————————————————————————————————————————————————————————————*/

static void AdjustCursor(CPPtr vars)
{
	Point mouseLoc;
	
	GetMouse(&mouseLoc);
	switch (FindDialogItem(vars->dialog, mouseLoc) + 1) {
	case 3:
	case 4:
	case 5:
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
	
	register CPPtr vars;
	TEPtr pTE;
	WindowRef theWindow;
	short keyCode, charCode, meta, i;
	
	theWindow = GetDialogWindow(theDialog);
	vars = (CPPtr) GetWRefCon(theWindow);
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
		i = GetDialogKeyboardFocusItem(theDialog) - 3;
		pTE = *GetDialogTextH(theDialog);
		TESetSelect(pTE->selStart, pTE->selEnd, vars->text[i]);
		TEKey(charCode, vars->text[i]);
		if (charCode != BS) {
			charCode = (unsigned char) '•';
			theEvent->message = charCode;
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

OSErr ChangePass(void)
{
	extern OSErr MCPChangePass(Str8, Str8);
	
	register CPPtr vars;
	Str8 oPassword, nPassword;
	Rect r;
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uTextItem, uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short i, itemHit;
	
	SetCursorID(watchCursor);
	CenterDialog(theDialogID);
	theDialog = GetNewDialog(theDialogID, nil, (WindowRef) -1);
	if (theDialog == nil)
		return memFullErr;
	theWindow = GetDialogWindow(theDialog);
	vars = (CPPtr) NewPtr(sizeof(CPRecord));
	if (vars == nil) {
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->dialog = theDialog;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	SetRect(&r, 0x1000, 0x1000, 0x1100, 0x1100);
	vars->text[0] = TENew(&r, &r);
	vars->text[1] = TENew(&r, &r);
	vars->text[2] = TENew(&r, &r);
	if (vars->text[0] == nil || vars->text[1] == nil || vars->text[2] == nil) {
		for (i = 0; i < 3; i++)
			if (vars->text[i] != nil)
				TEDispose(vars->text[i]);
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeDialog(theDialog);
		return memFullErr;
	}
	vars->okState = 0;
	SetWRefCon(theWindow, (long) vars);
	uTextItem = NewUserItemProc(TextItem);
	SetDialogItemProc(theDialog, 7, uTextItem);
	SetDialogItemProc(theDialog, 8, uTextItem);
	SetDialogItemProc(theDialog, 9, uTextItem);
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
			GetXText(vars->text[0], oPassword, 8);
			GetXText(vars->text[1], nPassword, 8);
			error = MCPChangePass(oPassword, nPassword);
			break;
		case 2:
			error = userCanceledErr;
			break;
		case 3:
		case 4:
		case 5:
			AdjustHilite(vars);
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	DisposeRoutineDescriptor(uTextItem);
	TEDispose(vars->text[2]);
	TEDispose(vars->text[1]);
	TEDispose(vars->text[0]);
	SetPort(savePort);
	DisposePtr((Ptr) vars);
	DisposeDialog(theDialog);
	return error;
}

/*————————————————————————————————————————————————————————————*/
