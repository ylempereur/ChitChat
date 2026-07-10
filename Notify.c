/* Notify.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <Quickdraw.h>
#include <ToolUtils.h>
#include <Events.h>
#include <Controls.h>
#include <Windows.h>
#include <Dialogs.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

#define theDialogID 5002

/*————————————————————————————————————————————————————————————*/

#pragma segment Dialogs

/*————————————————————————————————————————————————————————————*/

static OSErr JoinConf(long cid, Str32 title)
{
	extern OSErr MCPJoin(long);
	extern OSErr MCPLeave(long);
	extern WindowRef NewConf(long, Str255);
	
	WindowRef theWindow;
	OSErr error;
	
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

OSErr Notify(long cid, Str32 title, Str32 name)
{
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
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	ParamText(title, name, nil, nil);
	uOutlineItem = NewUserItemProc(OutlineItem);
	SetDialogItemProc(theDialog, 5, uOutlineItem);
	myFilter = NewModalFilterProc(MyFilter);
	SysBeep(30);
	ShowWindow(theWindow);
	SetCursorID(arrowCursor);
	error = 1;
	do {
		ModalDialog(myFilter, &itemHit);
		switch (itemHit) {
		case 1:
			error = JoinConf(cid, title);
			break;
		case 2:
			error = userCanceledErr;
			break;
		}
	} while (error == 1);
	HideWindow(theWindow);
	DisposeRoutineDescriptor(myFilter);
	DisposeRoutineDescriptor(uOutlineItem);
	SetPort(savePort);
	DisposeDialog(theDialog);
	return error;
}

/*————————————————————————————————————————————————————————————*/
