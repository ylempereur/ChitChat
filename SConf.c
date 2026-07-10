/* SConf.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Memory.h>			// Types.h
#include <Scrap.h>			// Types.h
#include <Quickdraw.h>		// Types.h
#include <Fonts.h>			// Types.h
#include <Windows.h>		// Quickdraw.h, Events.h, Controls.h
#include <Menus.h>			// Quickdraw.h
#include <TextEdit.h>		// Quickdraw.h
#include <ToolUtils.h>		// Quickdraw.h

#include "Misc.h"
#include "Server.h"

/*————————————————————————————————————————————————————————————*/

typedef struct {
	WindowRecord window;
	Rect editRect;
	TEHandle hTE;
	short lineHeight;
} CFRecord, *CFPeek;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

WindowPtr xNewConf(void)		// conflict with SCom.c !!!
{
	Ptr wStorage;
	WindowPtr theWindow;
	Rect destRect;
	Rect viewRect;
	FontInfo info;
	
	if ((wStorage = NewPtr(sizeof(CFRecord))) != nil) {
		SetRect(&destRect, 10, 50, 450, 300);
		if ((theWindow = NewWindow(wStorage, &destRect, "\p", false, zoomDocProc, (WindowPtr) -1,
		 true, 0)) != nil) {
			SetPort(theWindow);
			TextFont(kFontIDMonaco);
			TextSize(9);
			GetFontInfo(&info);
			((CFPeek) theWindow)->lineHeight = info.ascent + info.descent + info.leading;
			destRect = theWindow->portRect;
			destRect.top = destRect.bottom - ((CFPeek) theWindow)->lineHeight * 2 - 4;
			destRect.right -= 15;
			((CFPeek) theWindow)->editRect = destRect;
			InsetRect(&destRect, 2, 2);
			viewRect = destRect;
			InsetRect(&destRect, 2, 0);
			if ((((CFPeek) theWindow)->hTE = TENew(&destRect, &viewRect)) != nil) {
				((WindowPeek) theWindow)->windowKind = confKind;
				ShowWindow(theWindow);
				return theWindow;
			}
			CloseWindow(theWindow);
		}
		DisposePtr(wStorage);
	}
	return nil;
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void CloseConf(WindowPtr theWindow)
{
	HideWindow(theWindow);
	TEDispose(((CFPeek) theWindow)->hTE);
	CloseWindow(theWindow);
	DisposePtr((Ptr) theWindow);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ClickConf(WindowPtr theWindow, Point mouseLoc, short modifiers)
{
	GlobalToLocal(&mouseLoc);
	if (PtInRect(mouseLoc, &((CFPeek) theWindow)->editRect)) {
		ClipRect(&((CFPeek) theWindow)->editRect);
		TEClick(mouseLoc, (modifiers & shiftKey) != 0, ((CFPeek) theWindow)->hTE);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void KeyConf(WindowPtr theWindow, long code)
{
	ClipRect(&((CFPeek) theWindow)->editRect);
	TEKey(code & charCodeMask, ((CFPeek) theWindow)->hTE);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void UpdateConf(WindowPtr theWindow)
{
	Rect theRect;
	
	BeginUpdate(theWindow);
	ClipRect(&theWindow->portRect);
	EraseRect(&theWindow->portRect);
	theRect = theWindow->portRect;
	theRect.left = theRect.right - 15;
	ClipRect(&theRect);
	DrawGrowIcon(theWindow);
	theRect = ((CFPeek) theWindow)->editRect;
	InsetRect(&theRect, -1, -1);
	ClipRect(&theRect);
	FrameRect(&theRect);
	ClipRect(&((CFPeek) theWindow)->editRect);
	TEUpdate(&((CFPeek) theWindow)->editRect, ((CFPeek) theWindow)->hTE);
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ActivateConf(WindowPtr theWindow, Boolean theFlag)
{
	Rect theRect;
	
	theRect = theWindow->portRect;
	theRect.left = theRect.right - 15;
	ClipRect(&theRect);
	DrawGrowIcon(theWindow);
	ClipRect(&((CFPeek) theWindow)->editRect);
	if (theFlag)
		TEActivate(((CFPeek) theWindow)->hTE);
	else
		TEDeactivate(((CFPeek) theWindow)->hTE);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void IdleConf(WindowPtr theWindow)
{
	ClipRect(&((CFPeek) theWindow)->editRect);
	TEIdle(((CFPeek) theWindow)->hTE);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjCurConf(WindowPtr theWindow, Point mouseLoc)
{
	if (PtInRect(mouseLoc, &((CFPeek) theWindow)->editRect))
		SetCursorID(iBeamCursor);
	else
		SetCursorID(arrowCursor);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GrowConf(WindowPtr theWindow, Point mouseLoc)
{
	short w, h;
	long newSize;
	Rect theRect;
	
	SetRect(&theRect, 200, 50, 32000, 32000);
	newSize = GrowWindow(theWindow, mouseLoc, &theRect);
	if (newSize != 0) {
		ClipRect(&theWindow->portRect);
		EraseRect(&theWindow->portRect);
		h = HiWord(newSize);
		w = LoWord(newSize);
		SizeWindow(theWindow, w, h, false);
		InvalRect(&theWindow->portRect);
		theRect = theWindow->portRect;
		theRect.top = theRect.bottom - ((CFPeek) theWindow)->lineHeight * 2 - 4;
		theRect.right -= 15;
		((CFPeek) theWindow)->editRect = theRect;
		InsetRect(&theRect, 2, 2);
		(*((CFPeek) theWindow)->hTE)->viewRect = theRect;
		InsetRect(&theRect, 2, 0);
		(*((CFPeek) theWindow)->hTE)->destRect = theRect;
		TECalText(((CFPeek) theWindow)->hTE);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void GoAwayConf(WindowPtr theWindow, Point mouseLoc)
{
	if (TrackGoAway(theWindow, mouseLoc))
		CloseConf(theWindow);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void ZoomConf(WindowPtr theWindow, Point mouseLoc, short partCode)
{
	Rect theRect;
	
	if (TrackBox(theWindow, mouseLoc, partCode)) {
		ClipRect(&theWindow->portRect);
		EraseRect(&theWindow->portRect);
		ZoomWindow(theWindow, partCode, true);
		InvalRect(&theWindow->portRect);
		theRect = theWindow->portRect;
		theRect.top = theRect.bottom - ((CFPeek) theWindow)->lineHeight * 2 - 4;
		theRect.right -= 15;
		((CFPeek) theWindow)->editRect = theRect;
		InsetRect(&theRect, 2, 2);
		(*((CFPeek) theWindow)->hTE)->viewRect = theRect;
		InsetRect(&theRect, 2, 0);
		(*((CFPeek) theWindow)->hTE)->destRect = theRect;
		TECalText(((CFPeek) theWindow)->hTE);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void AdjEditConf(WindowPtr theWindow, MenuHandle theMenu)
{
	long offset;
	TEPtr pTE;
	
	pTE = *((CFPeek) theWindow)->hTE;
	DisableItem(theMenu, UndoItem);
	if (pTE->selEnd - pTE->selStart > 0) {
		EnableItem(theMenu, CutItem);
		EnableItem(theMenu, CopyItem);
		EnableItem(theMenu, ClearItem);
	} else {
		DisableItem(theMenu, CutItem);
		DisableItem(theMenu, CopyItem);
		DisableItem(theMenu, ClearItem);
	}
	if (GetScrap(nil, 'TEXT', &offset) > 0)
		EnableItem(theMenu, PasteItem);
	else
		DisableItem(theMenu, PasteItem);
	if (pTE->teLength > 0)
		EnableItem(theMenu, SelectAllItem);
	else
		DisableItem(theMenu, SelectAllItem);
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

void EditConf(WindowPtr theWindow, short menuItem)
{
	ClipRect(&((CFPeek) theWindow)->editRect);
	switch (menuItem) {
	case CutItem:
		TECut(((CFPeek) theWindow)->hTE);
		if (ZeroScrap() == noErr)
			(void) TEToScrap();
		break;
	case CopyItem:
		TECopy(((CFPeek) theWindow)->hTE);
		if (ZeroScrap() == noErr)
			(void) TEToScrap();
		break;
	case PasteItem:
		if (TEFromScrap() == noErr && TEGetScrapLength() < 256)
			TEPaste(((CFPeek) theWindow)->hTE);
		break;
	case ClearItem:
		TEDelete(((CFPeek) theWindow)->hTE);
		break;
	case SelectAllItem:
		TESetSelect(0, 32767, ((CFPeek) theWindow)->hTE);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/
