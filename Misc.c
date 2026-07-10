/* Misc.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Traps.h>
#include <Errors.h>
#include <Gestalt.h>
#include <Memory.h>
#include <MacRuntime.h>
#include <OSUtils.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <Lists.h>
#include <Windows.h>
#include <Controls.h>
#include <Menus.h>
#include <Dialogs.h>
#include <LowMem.h>
#include <Events.h>

/*————————————————————————————————————————————————————————————*/

#define TIMEOUT 0
#define FROM 0xB0004000	// Sunday, July 27, 1997 12:00:00 AM
#define TO 0xB0DDBC00	// Sunday, January 11, 1998 12:00:00 AM

#define DEMO 0
#define DURATION 4*60*60

/*————————————————————————————————————————————————————————————*/

enum {arrowCursor};

/*————————————————————————————————————————————————————————————*/

extern Boolean hasColorQD, quitting;

#if DEMO
unsigned long launchTime;
#endif

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

/*————————————————————————————————————————————————————————————*/

void SetCursorID(short cursorID)
{
	CursHandle crsr;
	
	if (cursorID != arrowCursor && (crsr = GetCursor(cursorID)) != nil)
		SetCursor(*crsr);
	else
		SetCursor(&qd.arrow);
}

/*————————————————————————————————————————————————————————————*/

void YieldToSystem(void)
{
	EventRecord theEvent;
	
	(void) EventAvail(everyEvent, &theEvent);
}

/*————————————————————————————————————————————————————————————*/

WindowRef GetControlOwner(ControlRef theControl)
{
	return (*theControl)->contrlOwner;
}

/*————————————————————————————————————————————————————————————*/

void GetControlRect(ControlRef theControl, Rect *boundsRect)
{
	*boundsRect = (*theControl)->contrlRect;
}

/*————————————————————————————————————————————————————————————*/

void SetControlVis(ControlRef theControl, short visState)
{
	(*theControl)->contrlVis = visState;
}

/*————————————————————————————————————————————————————————————*/

short GetControlVis(ControlRef theControl)
{
	return (*theControl)->contrlVis;
}

/*————————————————————————————————————————————————————————————*/

TEHandle GetDialogTextH(DialogRef theDialog)
{
	return ((DialogPeek) theDialog)->textH;
}

/*————————————————————————————————————————————————————————————*/

void LSetSelFlags(short selFlags, ListRef lHandle)
{
	(*lHandle)->selFlags = selFlags;
}

/*————————————————————————————————————————————————————————————*/

void LGetViewRect(Rect *rView, ListRef lHandle)
{
	*rView = (*lHandle)->rView;
}

/*————————————————————————————————————————————————————————————*/

void LGetVisible(Rect *visible, ListRef lHandle)
{
	*visible = (*lHandle)->visible;
}

/*————————————————————————————————————————————————————————————*/

void LGetDataBounds(Rect *dataBounds, ListRef lHandle)
{
	*dataBounds = (*lHandle)->dataBounds;
}

/*————————————————————————————————————————————————————————————*/

WindowRef NewColorWindow(void *wStorage, const Rect *boundsRect, ConstStr255Param title,
			Boolean visible, short procID, WindowRef behind, Boolean goAwayFlag, long refCon)
{
	if (hasColorQD)
		return NewCWindow(wStorage, boundsRect, title, visible, procID, behind, goAwayFlag, refCon);
	else
		return NewWindow(wStorage, boundsRect, title, visible, procID, behind, goAwayFlag, refCon);
}

/*————————————————————————————————————————————————————————————*/

WindowRef GetNewColorWindow(short windowID, void *wStorage, WindowRef behind)
{
	if (hasColorQD)
		return GetNewCWindow(windowID, wStorage, behind);
	else
		return GetNewWindow(windowID, wStorage, behind);
}

/*————————————————————————————————————————————————————————————*/

short GetDialogItemValue(DialogRef theDialog, short itemNo)
{
	Rect box;
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	return GetControlValue((ControlRef) item);
}

/*————————————————————————————————————————————————————————————*/

void SetDialogItemValue(DialogRef theDialog, short itemNo, short newValue)
{
	Rect box;
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetControlValue((ControlRef) item, newValue);
}

/*————————————————————————————————————————————————————————————*/

void NGetDialogItemText(DialogRef theDialog, short itemNo, Str255 text, short maxLen)
{
	register unsigned char *p, *q;
	Str255 tempText;
	Rect box;
	Handle item;
	short itemType, length;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	GetDialogItemText(item, tempText);
	p = tempText;
	length = *p++;
	if (length > maxLen)
		length = maxLen;
	q = text;
	*q++ = length;
	BlockMove(p, q, length);
}

/*————————————————————————————————————————————————————————————*/

void NSetDialogItemText(DialogRef theDialog, short itemNo, ConstStr255Param text)
{
	Rect box;
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetDialogItemText(item, text);
}

/*————————————————————————————————————————————————————————————*/

void SetDialogItemHilite(DialogRef theDialog, short itemNo, ControlPartCode hiliteState)
{
	Rect box;
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	HiliteControl((ControlRef) item, hiliteState);
}

/*————————————————————————————————————————————————————————————*/

void GetDialogItemBox(DialogRef theDialog, short itemNo, Rect *box)
{
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, box);
}

/*————————————————————————————————————————————————————————————*/

void SetDialogItemProc(DialogRef theDialog, short itemNo, UserItemUPP userItem)
{
	Rect box;
	Handle item;
	short itemType;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetDialogItem(theDialog, itemNo, itemType, (Handle) userItem, &box);
}

/*————————————————————————————————————————————————————————————*/

void CenterDialog(short dialogID)
{
	DialogTHndl dialog;
	DialogTPtr d;
	short dh, dv;
	
	dialog = (DialogTHndl) GetResource('DLOG', dialogID);
	if (dialog == nil)
		return;
	HNoPurge((Handle) dialog);
	d = *dialog;
	dv = ((GetMBarHeight() + qd.screenBits.bounds.top - d->boundsRect.top) * 2
		 + qd.screenBits.bounds.bottom - d->boundsRect.bottom) / 3;
	dh = (qd.screenBits.bounds.left + qd.screenBits.bounds.right
		 - d->boundsRect.left - d->boundsRect.right) / 2;
	OffsetRect(&d->boundsRect, dh, dv);
}

/*————————————————————————————————————————————————————————————*/

void CenterRect(Rect *r)
{
	short dh, dv;
	
	dv = ((GetMBarHeight() + qd.screenBits.bounds.top - r->top) * 2
		 + qd.screenBits.bounds.bottom - r->bottom) / 3;
	dh = (qd.screenBits.bounds.left + qd.screenBits.bounds.right - r->left - r->right) / 2;
	OffsetRect(r, dh, dv);
}

/*————————————————————————————————————————————————————————————*/

Boolean CheckEnvirons(void)
{
	long response;
#if TIMEOUT
	unsigned long secs;
#endif
	OSErr error;
	
	if (!TrapAvailable(_Gestalt))
		return false;
	error = Gestalt(gestaltAppleEventsAttr, &response);
	if ((error != noErr) || !(response & (1 << gestaltAppleEventsPresent)))
		return false;
	error = Gestalt(gestaltSystemVersion, &response);
	if ((error != noErr) || (response < 0x0700))
		return false;
#if TIMEOUT
	GetDateTime(&secs);
	if (secs < FROM || secs >= TO)
		return false;
#endif
#if DEMO
	GetDateTime(&launchTime);
#endif
	return true;
}

/*————————————————————————————————————————————————————————————*/

void CheckTimeout(void)
{
	unsigned long secs;
	
	GetDateTime(&secs);
#if TIMEOUT
	if (secs < FROM || secs >= TO)
		quitting = true;
#endif
#if DEMO
	if (secs - launchTime > DURATION)
		quitting = true;
#endif
}

/*————————————————————————————————————————————————————————————*/
