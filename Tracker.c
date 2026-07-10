/* Tracker.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>

#include "Misc.h"
#include "Client.h"

/*————————————————————————————————————————————————————————————*/

typedef struct {
	Rect boundsRect;
	short procID;
	Boolean visible;
	Boolean filler1;
	Boolean goAwayFlag;
	Boolean filler2;
	long refCon;
	Str255 title;
} WindowTemplate, *WindowTPtr, **WindowTHndl;

typedef struct {
	WindowRef window;
	unsigned long ticks;
	Str255 text;
} TKRecord, *TKPtr;

/*————————————————————————————————————————————————————————————*/

extern Boolean hasAaron;

static WindowRef trackWindow = nil;

/*————————————————————————————————————————————————————————————*/

#pragma segment Windows

/*————————————————————————————————————————————————————————————*/

static void CenterWindow(short windowID)
{
	WindowTHndl window;
	WindowTPtr w;
	short dh, dv;
	
	window = (WindowTHndl) GetResource('WIND', windowID);
	if (window == nil)
		return;
	HNoPurge((Handle) window);
	w = *window;
	if (hasAaron)
		dv = qd.screenBits.bounds.bottom - w->boundsRect.bottom - 8;
	else
		dv = qd.screenBits.bounds.bottom - w->boundsRect.bottom - 3;
	dh = (qd.screenBits.bounds.left + qd.screenBits.bounds.right
		 - w->boundsRect.left - w->boundsRect.right) / 2;
	OffsetRect(&w->boundsRect, dh, dv);
}

/*————————————————————————————————————————————————————————————*/

WindowRef NewTrack(void)
{
	register TKPtr vars;
	WindowRef theWindow;
	GrafPtr savePort;
	short theNum;
	
	SetCursorID(watchCursor);
	if (trackWindow != nil) {
		if (trackWindow != FrontWindow())
			SelectWindow(trackWindow);
		return trackWindow;
	}
	CenterWindow(202);
	theWindow = GetNewColorWindow(202, nil, (WindowRef) -1);
	if (theWindow == nil)
		return nil;
	vars = (TKPtr) NewPtr(sizeof(TKRecord));
	if (vars == nil) {
		DisposeWindow(theWindow);
		return nil;
	}
	vars->window = theWindow;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	GetFNum("\pMonaco", &theNum);
	TextFont(theNum);
	TextSize(9);
	vars->ticks = 0;
	vars->text[0] = 0;
	SetWRefCon(theWindow, (long) vars);
	SetWindowKind(theWindow, tracKind);
	ShowWindow(theWindow);
	SetPort(savePort);
	trackWindow = theWindow;
	return trackWindow;
}

/*————————————————————————————————————————————————————————————*/

void CloseTrack(WindowRef theWindow)
{
	register TKPtr vars;
	
	vars = (TKPtr) GetWRefCon(theWindow);
	HideWindow(theWindow);
	DisposePtr((Ptr) vars);
	DisposeWindow(theWindow);
	if (theWindow == trackWindow)
		trackWindow = nil;
}

/*————————————————————————————————————————————————————————————*/

void UpdateTrack(WindowRef theWindow)
{
	register TKPtr vars;
	Rect box;
	StringPtr text;
	long length;
	
	vars = (TKPtr) GetWRefCon(theWindow);
	box = GetWindowPort(theWindow)->portRect;
	BeginUpdate(theWindow);
	EraseRect(&box);
	if (StrLength(vars->text)) {
		InsetRect(&box, 4, 4);
		text = vars->text;
		length = *text++;
		TETextBox(text, length, &box, teJustCenter);
	}
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/

void AddToTrack(ConstStr255Param text)
{
	register TKPtr vars;
	GrafPtr savePort;
	
	if (trackWindow == nil)
		return;
	vars = (TKPtr) GetWRefCon(trackWindow);
	GetPort(&savePort);
	SetPortWindowPort(trackWindow);
	vars->ticks = TickCount();
	PLstrcpy(vars->text, text);
	InvalRect(&GetWindowPort(trackWindow)->portRect);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void IdleTrack(void)
{
	register TKPtr vars;
	
	if (trackWindow == nil)
		return;
	vars = (TKPtr) GetWRefCon(trackWindow);
	if (!StrLength(vars->text))
		return;
	if (TickCount() - vars->ticks < 30*60)
		return;
	AddToTrack("\p");
}

/*————————————————————————————————————————————————————————————*/
