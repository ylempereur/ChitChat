/* Splash.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1
#define OLDROUTINELOCATIONS 0

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <QuickdrawText.h>
#include <Windows.h>
#include <Palettes.h>
#include <LowMem.h>
#include <OSUtils.h>

#include "Misc.h"
#include "Client.h"

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
	PaletteHandle thePalette;
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
	GetFNum("\pGeneva", &theNum);
	TextFont(theNum);
	TextSize(9);
	thePicture = nil, thePalette = nil;
	pixelSize = hasColorQD ? (*(*GetMaxDevice(&box))->gdPMap)->pixelSize : 1;
	if (pixelSize >= 8) {
		thePicture = GetPicture(302);
		if (thePicture != nil) {
			HNoPurge((Handle) thePicture);
			thePalette = GetNewPalette(302);
		}
	}
	if (thePicture == nil && pixelSize >= 4) {
		thePicture = GetPicture(301);
		if (thePicture != nil) {
			HNoPurge((Handle) thePicture);
			thePalette = GetNewPalette(301);
		}
	}
	if (thePicture == nil) {
		thePicture = GetPicture(300);
		if (thePicture != nil)
			HNoPurge((Handle) thePicture);
	}
	if (thePicture == nil) {
		SetPort(savePort);
		DisposeWindow(theWindow);
		return nil;
	}
	p = *thePicture;
	if (box.bottom - box.top - p->picFrame.bottom + p->picFrame.top != 0 ||
		box.right - box.left - p->picFrame.right + p->picFrame.left != 0) {
		if (thePalette != nil)
			DisposePalette(thePalette);
		HPurge((Handle) thePicture);
		SetPort(savePort);
		DisposeWindow(theWindow);
		return nil;
	}
	SetWRefCon(theWindow, (long) thePicture);
	if (thePalette != nil)
		SetPalette(theWindow, thePalette, true);
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
	PaletteHandle thePalette;
	
	HideWindow(theWindow);
	if (hasColorQD) {
		thePalette = GetPalette(theWindow);
		if (thePalette != nil)
			DisposePalette(thePalette);
	}
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
	Rect portRect;
	PicHandle thePicture;
	VersRecHndl version;
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
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/

void KillSplash(void)
{
	long finalTicks;
	
	if (splashWindow != nil) {
		Delay(60, &finalTicks);
		CloseSplash(splashWindow);
	}
}

/*————————————————————————————————————————————————————————————*/
