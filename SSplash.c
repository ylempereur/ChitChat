/* SSplash.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1
#define OLDROUTINELOCATIONS 0

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <string.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <QuickdrawText.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>

#include "Misc.h"
#include "Server.h"

/*————————————————————————————————————————————————————————————*/

extern Boolean hasColorQD;

static WindowRef splashWindow = nil;

/*————————————————————————————————————————————————————————————*/

#pragma segment Windows

/*————————————————————————————————————————————————————————————*/

WindowRef NewSplash(void)
{
	Rect box;
	WindowRef theWindow;
	PicHandle thePicture;
	PicPtr p;
	GrafPtr savePort;
	short theNum, pixelSize;
	
	SetCursorID(watchCursor);
	if (splashWindow != nil) {
		if (splashWindow != FrontWindow())
			SelectWindow(splashWindow);
		return splashWindow;
	}
	SetRect(&box, 0, 0, 432, 270);
	CenterRect(&box);
	theWindow = NewColorWindow(nil, &box, "\p", false, altDBoxProc, (WindowRef) -1, false, 0);
	if (theWindow == nil)
		return nil;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	GetFNum("\pMonaco", &theNum);
	TextFont(theNum);
	TextSize(9);
	thePicture = nil;
	pixelSize = hasColorQD ? (*(*GetMaxDevice(&box))->gdPMap)->pixelSize : 1;
	if (pixelSize >= 8)
		thePicture = GetPicture(302);
	if (thePicture == nil && pixelSize >= 4)
		thePicture = GetPicture(301);
	if (thePicture == nil)
		thePicture = GetPicture(300);
	if (thePicture == nil) {
		SetPort(savePort);
		DisposeWindow(theWindow);
		return nil;
	}
	HNoPurge((Handle) thePicture);
	p = *thePicture;
	if (box.bottom - box.top - p->picFrame.bottom + p->picFrame.top != 0 ||
		box.right - box.left - p->picFrame.right + p->picFrame.left != 0) {
		HPurge((Handle) thePicture);
		SetPort(savePort);
		DisposeWindow(theWindow);
		return nil;
	}
	SetWRefCon(theWindow, (long) thePicture);
	SetWindowKind(theWindow, splashKind);
	ShowWindow(theWindow);
	SetPort(savePort);
	splashWindow = theWindow;
	return splashWindow;
}

/*————————————————————————————————————————————————————————————*/

void CloseSplash(WindowRef theWindow)
{
	PicHandle thePicture;
	
	HideWindow(theWindow);
	thePicture = (PicHandle) GetWRefCon(theWindow);
	HPurge((Handle) thePicture);
	DisposeWindow(theWindow);
	if (theWindow == splashWindow)
		splashWindow = nil;
}

/*————————————————————————————————————————————————————————————*/

void UpdateSplash(WindowRef theWindow)
{
	Str255 shortVersion;
	Rect portRect, box;
	PicHandle thePicture;
	VersRecHndl version;
	char *text;
	short width;
	
	portRect = GetWindowPort(theWindow)->portRect;
	thePicture = (PicHandle) GetWRefCon(theWindow);
	version = (VersRecHndl) Get1Resource('vers', 1);
	PLstrcpy(shortVersion, (*version)->shortVersion);
	width = StringWidth(shortVersion);
	BeginUpdate(theWindow);
	EraseRect(&portRect);
	DrawPicture(thePicture, &portRect);
	MoveTo(portRect.right - width - 3, portRect.bottom - 4);
	DrawString(shortVersion);
/*
	SetRect(&box, 417 - (22 * 6), 54, 428, 75);
	FrameRect(&box);
	InsetRect(&box, 1, 1);
	EraseRect(&box);
	InsetRect(&box, 4, 4);
	text = "Mark Altenberg version";
	TETextBox(text, strlen(text), &box, teJustLeft);
*/
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/
