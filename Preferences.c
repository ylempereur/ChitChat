/* Preferences.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <Quickdraw.h>
#include <TextUtils.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Controls.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 207

/*————————————————————————————————————————————————————————————*/

extern QDGlobals qd;
extern Boolean gSplash, gAutoLogon, gTracking;

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

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
	PenPat(&qd.gray);
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
	GetIndString(theString, 207, itemNo - 5);
	p = theString;
	length = *p++;
	TETextBox(p, length, &box, teJustCenter);
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
	
	WindowRef theWindow;
	short keyCode, charCode, meta;
	
	theWindow = GetDialogWindow(theDialog);
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
			FlashButton(theDialog, ok);
			*itemHit = ok;
			return true;
		}
		if (keyCode == 0x351B || charCode == '.' && meta & cmdKey) {
			FlashButton(theDialog, cancel);
			*itemHit = cancel;
			return true;
		}
		break;
	}
	return false;
}

/*————————————————————————————————————————————————————————————*/

void Preferences(void)
{
	DialogRef theDialog;
	WindowRef theWindow;
	GrafPtr savePort;
	UserItemUPP uTextItem, uLineItem, uOutlineItem;
	ModalFilterUPP myFilter;
	OSErr error;
	short itemHit;
	
	SetCursorID(watchCursor);
	CenterDialog(theDialogID);
	theDialog = GetNewDialog(theDialogID, nil, (WindowRef) -1);
	if (theDialog == nil)
		return;
	theWindow = GetDialogWindow(theDialog);
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	SetDialogItemValue(theDialog, 3, gSplash ? 1 : 0);
	SetDialogItemValue(theDialog, 4, gAutoLogon ? 1 : 0);
	SetDialogItemValue(theDialog, 5, gTracking ? 1 : 0);
	uTextItem = NewUserItemProc(TextItem);
	SetDialogItemProc(theDialog, 6, uTextItem);
	uLineItem = NewUserItemProc(LineItem);
	SetDialogItemProc(theDialog, 7, uLineItem);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 8, uOutlineItem);
	myFilter = NewModalFilterProc(MyFilter);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			gSplash = GetDialogItemValue(theDialog, 3) != 0;
			gAutoLogon = GetDialogItemValue(theDialog, 4) != 0;
			gTracking = GetDialogItemValue(theDialog, 5) != 0;
			error = noErr;
			break;
		case 2:
			error = userCanceledErr;
			break;
		case 3:
		case 4:
		case 5:
			SetDialogItemValue(theDialog, itemHit, 1 - GetDialogItemValue(theDialog, itemHit));
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	DisposeRoutineDescriptor(uLineItem);
	DisposeRoutineDescriptor(uTextItem);
	SetPort(savePort);
	DisposeDialog(theDialog);
}

/*————————————————————————————————————————————————————————————*/
