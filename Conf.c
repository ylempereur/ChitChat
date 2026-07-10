/* Conf.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <PLStringFuncs.h>
#include <Gestalt.h>
#include <Memory.h>
#include <Folders.h>
#include <Resources.h>
#include <OSUtils.h>
#include <Scrap.h>
#include <Quickdraw.h>
#include <Icons.h>
#include <Fonts.h>
#include <Windows.h>
#include <Palettes.h>
#include <Menus.h>
#include <AppleEvents.h>
#include <TextEdit.h>
#include <Drag.h>
#include <ToolUtils.h>
#include <StandardFile.h>
#include <AIFF.h>
#include <SoundInput.h>
#include <LowMem.h>
#include <Printing.h>

#include "SmartScrollAPI.h"

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
	Byte flags, ratio;
	unsigned short count, index, outdex;
	Byte buffer[8192];
} BFRecord, *BFPtr;

typedef struct {
	Rect box;
	short iconID;
	Boolean visible;
	Boolean enabled;
} KYRecord, *KYPtr;

typedef struct {
	WindowRef window, grafWin;
	long cid;
	Rect padRect, nameRect;
	KYRecord keys[7];
	TEHandle iText, oText;
	ControlRef vScroll;
	Boolean active, inAct;
	short state;
	SndChannelPtr sndChan;
	UnsignedFixed sampRate;
	short count, index, outdex, wait;
	SndListHandle header[8];
	StateBlock inState, outState;
	Str32 name;
} CFRecord, *CFPtr;

/*————————————————————————————————————————————————————————————*/

extern Str32 gUserName;
extern THPrint prRecHdl;
extern MLHandle macroList;
extern MenuRef macroMenu;
extern short gRate;
extern Boolean hasDragMgr, hasGetHiliteRgn, hasSoundInput, hasAaron;

static ControlActionUPP scrollTextProc;
static TEClickLoopUPP autoScrollProc;
static DragTrackingHandlerUPP trackingHandler;
static DragReceiveHandlerUPP receiveHandler;
static unsigned long caretTime;
static short caretOffset, lastOffset, insertPosition;
static Boolean canAcceptItems, caretShow, cursorInContent, dragHasLeftInputField;

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
	if (hasAaron) {
		dv = qd.screenBits.bounds.top - w->boundsRect.top + GetMBarHeight() + 24;
		dh = qd.screenBits.bounds.left - w->boundsRect.left + 8;
	} else {
		dv = qd.screenBits.bounds.top - w->boundsRect.top + GetMBarHeight() + 21;
		dh = qd.screenBits.bounds.left - w->boundsRect.left + 3;
	}
	OffsetRect(&w->boundsRect, dh, dv);
}

/*————————————————————————————————————————————————————————————*/

static void ActivateInput(CFPtr vars)
{
	if (!vars->inAct) {
		vars->inAct = true;
		if (vars->active) {
			TEDeactivate(vars->oText);
			TEActivate(vars->iText);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

static void ActivateOutput(CFPtr vars)
{
	if (vars->inAct) {
		vars->inAct = false;
		if (vars->active) {
			TEDeactivate(vars->iText);
			TEActivate(vars->oText);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

static void MyDrawGrowIcon(CFPtr vars)
{
	Rect theRect;
	RgnHandle saveClip;
	
	GetControlRect(vars->vScroll, &theRect);
	FrameRect(&theRect);
	ValidRect(&theRect);
	
	saveClip = NewRgn();
	GetClip(saveClip);
	theRect = GetWindowPort(vars->window)->portRect;
	theRect.top = theRect.bottom - 15;
	theRect.left = theRect.right - 15;
	ClipRect(&theRect);
	DrawGrowIcon(vars->window);
	SetClip(saveClip);
	DisposeRgn(saveClip);
	ValidRect(&theRect);
}

/*————————————————————————————————————————————————————————————*/

static void ResizeWindow(CFPtr vars)
{
	register KYPtr k;
	register TEPtr pTE;
	Rect portRect, theRect;
	short i, v, saveVis, value, max, firstChar, nLines, vLines;
	Boolean flag;
	
	portRect = GetWindowPort(vars->window)->portRect;
	
	theRect = portRect;
	theRect.bottom = theRect.top + 35;
	InsetRect(&theRect, -1, -1);
	vars->padRect = theRect;
	
	SetRect(&theRect, 5, 5, 37, 37);
	k = vars->keys;
	for (i = 0; i < 7; i++) {
		k->box = theRect;
		k->iconID = 300 + i * 2;
		k->visible = true;
		k->enabled = true;
		OffsetRect(&theRect, i != 5 ? 30 : 55, 0);
		k++;
	}
	
	theRect = portRect;
	theRect.top += 12;
	theRect.bottom = theRect.top + 11;
	theRect.left += 240;
	theRect.right -= 5;
	vars->nameRect = theRect;
	
	pTE = *vars->iText;
	theRect = portRect;
	theRect.top = theRect.bottom - (v = pTE->lineHeight * 2 + 4);
	theRect.right -= 15;
	InsetRect(&theRect, 2, 2);
	pTE->destRect = theRect;
	pTE->viewRect = theRect;
	TECalText(vars->iText);
	
	saveVis = GetControlVis(vars->vScroll);
	SetControlVis(vars->vScroll, 0);
	
	MoveControl(vars->vScroll, portRect.right - 15, portRect.top + 35);
	SizeControl(vars->vScroll, 16, portRect.bottom - portRect.top - v - 35);
	
	pTE = *vars->oText;
	value = GetControlValue(vars->vScroll);
	max = GetControlMaximum(vars->vScroll);
	flag = value == max;
	firstChar = pTE->lineStarts[value];
	
	theRect = portRect;
	theRect.top += 36;
	theRect.bottom -= v + 1;
	theRect.right -= 15;
	InsetRect(&theRect, 2, 2);
	theRect.bottom -= (theRect.bottom - theRect.top) % pTE->lineHeight;
	pTE->destRect = theRect;
	pTE->viewRect = theRect;
	TECalText(vars->oText);
	
	pTE = *vars->oText;
	nLines = pTE->nLines;
	vLines = (pTE->viewRect.bottom - pTE->viewRect.top) / pTE->lineHeight;
	if (vLines > nLines) vLines = nLines;
	max = nLines - vLines;
	SetControlMaximum(vars->vScroll, max);
	SetSmartScrollInfo(vars->vScroll, vLines, nLines);
	
	if (flag)
		value = GetControlMaximum(vars->vScroll);
	else {
		pTE = *vars->oText;
		value = 0;
		while (value < pTE->nLines && firstChar >= pTE->lineStarts[value+1])
			value++;
	}
	SetControlValue(vars->vScroll, value);
	
	value = GetControlValue(vars->vScroll);
	if (value != 0) {
		pTE = *vars->oText;
		pTE->destRect.top -= value * pTE->lineHeight;
	}
	
	SetControlVis(vars->vScroll, saveVis);
}

/*————————————————————————————————————————————————————————————*/

static void AdjustZoom(CFPtr vars)
{
	Rect r;
	
	r = qd.screenBits.bounds;
	if (hasAaron) {
		r.top += GetMBarHeight() + 24;
		r.left += 8;
		r.bottom -= 8;
		r.right -= 8;
	} else {
		r.top += GetMBarHeight() + 21;
		r.left += 3;
		r.bottom -= 3;
		r.right -= 3;
	}
	SetWindowStandardState(vars->window, &r);
}

/*————————————————————————————————————————————————————————————*/

long GetConfWindowCID(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	return vars->cid;
}

/*————————————————————————————————————————————————————————————*/

void GrafWinOpened(WindowRef theWindow, WindowRef grafWin)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	vars->grafWin = grafWin;
}

/*————————————————————————————————————————————————————————————*/

void GrafWinClosed(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	vars->grafWin = nil;
}

/*————————————————————————————————————————————————————————————*/

static Boolean IsLineStart(short offset, TEHandle hTE)
{
	register TEPtr pTE;
	register short *p;
	short length, line;
	
	pTE = *hTE;
	length = pTE->teLength;
	if (length == 0)
		return true;
	if (offset >= length)
		return (*pTE->hText)[length - 1] == CR;
	p = pTE->lineStarts;
	while ((line = *p++) < offset) ;
	return line == offset;
}

/*————————————————————————————————————————————————————————————*/

static short HitTest(Point pt, TEHandle hTE)
{
	Point loc;
	short offset;
	
	offset = TEGetOffset(pt, hTE);
	if (offset > 0 && IsLineStart(offset, hTE) && (*(*hTE)->hText)[offset - 1] != CR) {
		loc = TEGetPoint(offset - 1, hTE);
		if (loc.v > pt.v)
			offset--;
	}
	return offset;
}

/*————————————————————————————————————————————————————————————*/

static void DrawCaret(short offset, TEHandle hTE)
{
	register TEPtr pTE;
	Rect r;
	
	*(Point *) &r.bottom = TEGetPoint(offset, hTE);
	pTE = *hTE;
	if (offset > 0 && offset == pTE->teLength && (*pTE->hText)[offset - 1] == CR)
		r.bottom += pTE->lineHeight;
	r.top = r.bottom - pTE->lineHeight;
	r.left = r.right - 1;
	InvertRect(&r);
}

/*————————————————————————————————————————————————————————————*/

static pascal OSErr TrackingHandler(DragTrackingMessage message, WindowRef theWindow,
									void *refCon, DragReference theDrag)
{
	register CFPtr vars;
	register TEPtr pTE;
	Point mouse;
	DragAttributes attributes;
	ItemReference theItem;
	FlavorFlags flavorFlags;
	RgnHandle theRgn;
	GrafPtr savePort;
	unsigned long theTime;
	OSErr error;
	short offset;
	unsigned short count, index;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	switch (message) {
	case dragTrackingEnterHandler:
		canAcceptItems = true;
		CountDragItems(theDrag, &count);
		for (index = 1; index <= count; index++) {
			GetDragItemReferenceNumber(theDrag, index, &theItem);
			error = GetFlavorFlags(theDrag, theItem, 'TEXT', &flavorFlags);
			if (error != noErr) {
				canAcceptItems = false;
				break;
			}
		}
		break;
	case dragTrackingEnterWindow:
		if (canAcceptItems) {
			dragHasLeftInputField = false;
			cursorInContent = false;
			caretTime = TickCount();
			caretOffset = lastOffset = -1;
			caretShow = true;
		}
		break;
	case dragTrackingInWindow:
		if (canAcceptItems) {
			GetDragAttributes(theDrag, &attributes);
			GetDragMouse(theDrag, &mouse, nil);
			GlobalToLocal(&mouse);
			if (PtInRect(mouse, &(*vars->iText)->viewRect)) {
				if (!cursorInContent) {
					if (attributes & dragHasLeftSenderWindow || dragHasLeftInputField) {
						theRgn = NewRgn();
						RectRgn(theRgn, &(*vars->iText)->viewRect);
						ShowDragHilite(theDrag, theRgn, false);
						DisposeRgn(theRgn);
						cursorInContent = true;
					}
				}
				offset = HitTest(mouse, vars->iText);
				if (attributes & dragInsideSenderWindow && vars->inAct) {
					pTE = *vars->iText;
					if (offset >= pTE->selStart && offset <= pTE->selEnd)
						offset = -1;
				}
			} else {
				if (cursorInContent) {
					HideDragHilite(theDrag);
					cursorInContent = false;
				}
				dragHasLeftInputField = true;
				offset = -1;
			}
			insertPosition = offset;
			theTime = TickCount();
			if (offset != lastOffset) {
				caretTime = theTime;
				caretShow = true;
				lastOffset = offset;
			}
			if (theTime - caretTime > LMGetCaretTime()) {
				caretTime = theTime;
				caretShow = !caretShow;
			}
			if (!caretShow)
				offset = -1;
			if (offset != caretOffset) {
				if (caretOffset != -1)
					DrawCaret(caretOffset, vars->iText);
				if (offset != -1)
					DrawCaret(offset, vars->iText);
				caretOffset = offset;
			}
		}
		break;
	case dragTrackingLeaveWindow:
		if (canAcceptItems) {
			if (caretOffset != -1) {
				DrawCaret(caretOffset, vars->iText);
				caretOffset = -1;
			}
			if (cursorInContent) {
				HideDragHilite(theDrag);
				cursorInContent = false;
			}
		}
		break;
	case dragTrackingLeaveHandler:
		break;
	}
	SetPort(savePort);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static char GetCharAtOffset(short offset, TEHandle hTE)
{
	register TEPtr pTE;
	
	pTE = *hTE;
	if (offset < 0 || offset >= pTE->teLength)
		return CR;
	return ((char *) *pTE->hText)[offset];
}

/*————————————————————————————————————————————————————————————*/

static Boolean WhiteSpace(char theChar)
{
	return theChar == ' ' || theChar == CR;
}

/*————————————————————————————————————————————————————————————*/

static Boolean WhiteSpaceAtOffset(short offset, TEHandle hTE)
{
	register TEPtr pTE;
	char theChar;
	
	pTE = *hTE;
	if (offset < 0 || offset >= pTE->teLength)
		return true;
	theChar = ((char *) *pTE->hText)[offset];
	return theChar == ' ' || theChar == CR;
}

/*————————————————————————————————————————————————————————————*/

static void InsertTextAtOffset(short offset, Ptr textData, Size *textSize, TEHandle hTE)
{
	Size extra;
	
	extra = 0;
	if (!WhiteSpaceAtOffset(offset - 1, hTE) &&
		WhiteSpaceAtOffset(offset, hTE) &&
		!WhiteSpace(textData[0])) {
		TESetSelect(offset, offset, hTE);
		TEKey(' ', hTE);
		extra++;
		offset++;
	}
	if (WhiteSpaceAtOffset(offset - 1, hTE) &&
		!WhiteSpaceAtOffset(offset, hTE) &&
		!WhiteSpace(textData[*textSize - 1])) {
		TESetSelect(offset, offset, hTE);
		TEKey(' ', hTE);
		extra++;
	}
	TESetSelect(offset, offset, hTE);
	TEInsert(textData, *textSize, hTE);
	*textSize += extra;
}

/*————————————————————————————————————————————————————————————*/

static pascal OSErr ReceiveHandler(WindowRef theWindow, void *refCon, DragReference theDrag)
{
	register CFPtr vars;
	register TEPtr pTE;
	DragAttributes attributes;
	ItemReference theItem;
	Ptr textData;
	Size textSize;
	GrafPtr savePort;
	OSErr error;
	short downMods, upMods;
	unsigned short startPosition, items, index;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (!canAcceptItems || insertPosition == -1)
		return dragNotAcceptedErr;
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	if (caretOffset != -1) {
		DrawCaret(caretOffset, vars->iText);
		caretOffset = -1;
	}
	if (cursorInContent) {
		HideDragHilite(theDrag);
		cursorInContent = false;
	}
	GetDragAttributes(theDrag, &attributes);
	GetDragModifiers(theDrag, nil, &downMods, &upMods);
	if ((attributes & dragInsideSenderWindow) && vars->inAct &&
		!((downMods | upMods) & optionKey)) {
		pTE = *vars->iText;
		if (WhiteSpaceAtOffset(pTE->selStart - 1, vars->iText) &&
			!WhiteSpaceAtOffset(pTE->selStart, vars->iText) &&
			!WhiteSpaceAtOffset(pTE->selEnd - 1, vars->iText) &&
			WhiteSpaceAtOffset(pTE->selEnd, vars->iText)) {
			if (GetCharAtOffset(pTE->selStart - 1, vars->iText) == ' ')
				TESetSelect(pTE->selStart - 1, pTE->selEnd, vars->iText);
			else if (GetCharAtOffset(pTE->selEnd, vars->iText) == ' ')
				TESetSelect(pTE->selStart, pTE->selEnd + 1, vars->iText);
		}
		if (insertPosition > pTE->selStart)
			insertPosition -= pTE->selEnd - pTE->selStart;
		TEDelete(vars->iText);
	}
	startPosition = insertPosition;
	CountDragItems(theDrag, &items);
	for (index = 1; index <= items; index++) {
		GetDragItemReferenceNumber(theDrag, index, &theItem);
		error = GetFlavorDataSize(theDrag, theItem, 'TEXT', &textSize);
		if (error == noErr && textSize != 0) {
			textData = NewPtr(textSize);
			if (textData == nil) {
				SetPort(savePort);
				return memFullErr;
			}
			GetFlavorData(theDrag, theItem, 'TEXT', textData, &textSize, 0);
			InsertTextAtOffset(insertPosition, textData, &textSize, vars->iText);
			insertPosition += textSize;
			DisposePtr(textData);
		}
	}
	TESetSelect(startPosition, insertPosition, vars->iText);
	ActivateInput(vars);
	SetPort(savePort);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

WindowRef NewConf(long cid, Str255 title)
{
	extern void NullCaret(void);
	
	register CFPtr vars;
	register TEPtr pTE;
	FontInfo info;
	Rect theRect;
	WindowRef behind, theWindow;
	GrafPtr savePort;
	OSErr error;
	short theNum;
	
	SetCursorID(watchCursor);
	CenterWindow(200);
	behind = FrontWindow();
	if (behind == nil || GetWindowKind(behind) != dialogKind)
		behind = (WindowRef) -1;
	theWindow = GetNewColorWindow(200, nil, behind);
	if (theWindow == nil)
		return nil;
	vars = (CFPtr) NewPtr(sizeof(CFRecord));
	if (vars == nil) {
		DisposeWindow(theWindow);
		return nil;
	}
	vars->window = theWindow;
	vars->grafWin = nil;
	vars->cid = cid;
	SetWTitle(theWindow, title);
	GetPort(&savePort);
	SetPortWindowPort(theWindow);
	GetFNum("\pMonaco", &theNum);
	TextFont(theNum);
	TextSize(9);
	SetRect(&theRect, 0, 0, 64, 32);
	vars->iText = TENew(&theRect, &theRect);
	if (vars->iText == nil) {
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeWindow(theWindow);
		return nil;
	}
	TEAutoView(true, vars->iText);
	vars->oText = TEStyleNew(&theRect, &theRect);
	if (vars->oText == nil) {
		TEDispose(vars->iText);
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeWindow(theWindow);
		return nil;
	}
	GetFontInfo(&info);
	pTE = *vars->oText;
	pTE->lineHeight = info.ascent + info.descent + info.leading;
	pTE->fontAscent = info.ascent;
	pTE->caretHook = (CaretHookUPP) NullCaret;
	vars->vScroll = GetNewControl(200, theWindow);
	if (vars->vScroll == nil) {
		TEDispose(vars->oText);
		TEDispose(vars->iText);
		SetPort(savePort);
		DisposePtr((Ptr) vars);
		DisposeWindow(theWindow);
		return nil;
	}
	vars->active = false;
	vars->inAct = true;
	vars->state = 0;
	SetWRefCon(theWindow, (long) vars);
	if (hasDragMgr) {
		error = InstallTrackingHandler(trackingHandler, theWindow, nil);
		if (error != noErr) {
			DisposeControl(vars->vScroll);
			TEDispose(vars->oText);
			TEDispose(vars->iText);
			SetPort(savePort);
			DisposePtr((Ptr) vars);
			DisposeWindow(theWindow);
			return nil;
		}
		error = InstallReceiveHandler(receiveHandler, theWindow, nil);
		if (error != noErr) {
			RemoveTrackingHandler(trackingHandler, theWindow);
			DisposeControl(vars->vScroll);
			TEDispose(vars->oText);
			TEDispose(vars->iText);
			SetPort(savePort);
			DisposePtr((Ptr) vars);
			DisposeWindow(theWindow);
			return nil;
		}
	}
	SetWindowKind(theWindow, confKind);
	ResizeWindow(vars);
	AdjustZoom(vars);
	ShowWindow(theWindow);
	SetPort(savePort);
	return theWindow;
}

/*————————————————————————————————————————————————————————————*/

long CloseConf(WindowRef theWindow)
{
	void EndVoice(WindowRef);
	extern void TextWinClosed(WindowRef);
	extern long CloseGraf(WindowRef);
	
	register CFPtr vars;
	WindowRef child;
	long cid;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	SetCursorID(watchCursor);
	HideWindow(theWindow);
	EndVoice(theWindow);
	if (hasDragMgr) {
		RemoveReceiveHandler(receiveHandler, theWindow);
		RemoveTrackingHandler(trackingHandler, theWindow);
	}
	DisposeControl(vars->vScroll);
	TEDispose(vars->oText);
	TEDispose(vars->iText);
	child = vars->grafWin;
	cid = vars->cid;
	DisposePtr((Ptr) vars);
	DisposeWindow(theWindow);
	if (child != nil) {
		TextWinClosed(child);
		cid = CloseGraf(child);
	}
	return cid;
}

/*————————————————————————————————————————————————————————————*/

static pascal void ScrollText(ControlRef theControl, short thePart)
{
	register CFPtr vars;
	register TEPtr pTE;
	WindowRef theWindow;
	short delta, value;
	
	if (thePart != 0) {
		theWindow = GetControlOwner(theControl);
		vars = (CFPtr) GetWRefCon(theWindow);
		switch (thePart) {
		case inUpButton:
			delta = -1;
			break;
		case inDownButton:
			delta = +1;
			break;
		case inPageUp:
			pTE = *vars->oText;
			delta = (pTE->viewRect.top - pTE->viewRect.bottom) / pTE->lineHeight + 1;
			break;
		case inPageDown:
			pTE = *vars->oText;
			delta = (pTE->viewRect.bottom - pTE->viewRect.top) / pTE->lineHeight - 1;
			break;
		default:
			delta = 0;
			break;
		}
		value = GetControlValue(vars->vScroll);
		SetControlValue(vars->vScroll, value + delta);
		value -= GetControlValue(vars->vScroll);
		if (value != 0) {
			pTE = *vars->oText;
			TEScroll(0, value * pTE->lineHeight, vars->oText);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

static pascal Boolean AutoScroll(void)
{
	register CFPtr vars;
	register TEPtr pTE;
	Point thePoint;
	WindowRef theWindow;
	RgnHandle saveClip;
	
	theWindow = FrontWindow();
	vars = (CFPtr) GetWRefCon(theWindow);
	saveClip = NewRgn();
	GetClip(saveClip);
	ClipRect(&GetWindowPort(theWindow)->portRect);
	GetMouse(&thePoint);
	pTE = *vars->oText;
	if (thePoint.v < pTE->viewRect.top)
		ScrollText(vars->vScroll, inUpButton);
	else if (thePoint.v >= pTE->viewRect.bottom)
		ScrollText(vars->vScroll, inDownButton);
	SetClip(saveClip);
	DisposeRgn(saveClip);
	return true;
}

/*————————————————————————————————————————————————————————————*/

static void DrawPTT(CFPtr vars)
{
	register KYPtr k;
	
	k = &vars->keys[6];
	if (vars->state == 0) {
		k->enabled = true;
		PlotIconID(&k->box, atNone, ttNone, k->iconID);
	} else {
		k->enabled = false;
		PlotIconID(&k->box, atNone, ttDisabled, k->iconID);
	}
}

/*————————————————————————————————————————————————————————————*/

static void DrawName(CFPtr vars)
{
	Rect theRect;
	RGBColor saveColor;
	CGrafPtr windowPort;
	RgnHandle saveClip;
	
	theRect = vars->nameRect;
	windowPort = GetWindowPort(vars->window);
	if (windowPort->portVersion < 0 && (*windowPort->portPixMap)->pixelSize >= 4) {
		GetForeColor(&saveColor);
		PmForeColor(3);
		PaintRect(&theRect);
		RGBForeColor(&saveColor);
	} else
		EraseRect(&theRect);
	if (vars->state == 1) {
		saveClip = NewRgn();
		GetClip(saveClip);
		ClipRect(&theRect);
		MoveTo(theRect.left, theRect.top + 9);
		DrawString(vars->name);
		SetClip(saveClip);
		DisposeRgn(saveClip);
	}
}

/*————————————————————————————————————————————————————————————*/

static void NotifySpeak(CFPtr vars, Str32 name)
{
	void AddToConf(WindowRef, short, const void *, long);
	
	Str63 string;
	
	PLstrcpy(string, "\p• ");
	PLstrcat(string, name);
	PLstrcat(string, "\p is speaking...\n");
	AddToConf(vars->window, 2, &string[1], string[0]);
}

/*————————————————————————————————————————————————————————————*/

void BuildMenu(void)
{
	MLPtr ml;
	MacroPtr m;
	short c;
	
	if (macroMenu != nil)
		ReleaseResource((Handle) macroMenu);
	macroMenu = GetMenu(150);
	if (macroMenu == nil)
		return;
	HLockHi((Handle) macroList);
	ml = *macroList;
	c = ml->count;
	m = ml->macros;
	while (c) {
		AppendMenu(macroMenu, "\p ");
		SetMenuItemText(macroMenu, CountMItems(macroMenu), m->name);
		m++;
		c--;
	}
	HUnlock((Handle) macroList);
}

/*————————————————————————————————————————————————————————————*/

static void DoList(CFPtr vars)
{
	extern void EditMacros(MLHandle);
	extern OSErr MCPMessage(long, Str255);
	
	Point topLeft;
	MLPtr ml;
	long result;
	short menuItem;
	
	topLeft = *(Point *) &vars->keys[5].box.top;
	topLeft.v += 25;
	topLeft.h += 1;
	LocalToGlobal(&topLeft);
	InsertMenu(macroMenu, -1);
	result = PopUpMenuSelect(macroMenu, topLeft.v, topLeft.h, 1);
	DeleteMenu(150);
	if (result == 0)
		return;
	menuItem = LoWord(result);
	switch (menuItem) {
	case 1:
		EditMacros(macroList);
		BuildMenu();
		break;
	case 2:
		break;
	default:
		HLock((Handle) macroList);
		ml = *macroList;
		(void) MCPMessage(vars->cid, ml->macros[menuItem - 3].text);
		HUnlock((Handle) macroList);
		break;
	}
}

/*————————————————————————————————————————————————————————————*/

void StartVoice(WindowRef theWindow, UnsignedFixed sampRate, OSType compType, Str32 name)
{
	extern void MCPSetClickState(Boolean);
	
	register CFPtr vars;
	SndChannelPtr chan;
	SndListHandle header;
	long length;
	OSErr error;
	short i;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (vars->state != 0)
		return;
	if (compType != MACE6Type)
		return;
	chan = nil;
	error = SndNewChannel(&chan, sampledSynth, initMono, nil);
	if (error != noErr)
		return;
	vars->sndChan = chan;
	vars->sampRate = sampRate;
	vars->count = vars->index = vars->outdex = 0;
	vars->wait = 4;
	for (i = 0; i < 8; i++) {
		header = (SndListHandle) NewHandle(100 + 3024);
		if (header != nil)
			HLockHi((Handle) header);
		vars->header[i] = header;
	}
	memset(&vars->inState, 0, sizeof(StateBlock));
	memset(&vars->outState, 0, sizeof(StateBlock));
	PLstrcpy(vars->name, name);
	vars->state = 1;
	MCPSetClickState(false);
	DrawPTT(vars);
	DrawName(vars);
	NotifySpeak(vars, name);
}

/*————————————————————————————————————————————————————————————*/

static OSErr PlaySnd(SndChannelPtr chan, SndListHandle sndHdl)
{
	SndCommand sndCmd;
	long offset;
	OSErr error;
	
	error = GetSoundHeaderOffset(sndHdl, &offset);
	if (error == noErr) {
		sndCmd.cmd = bufferCmd;
		sndCmd.param1 = 0;
		sndCmd.param2 = (long) *sndHdl + offset;
		error = SndDoCommand(chan, &sndCmd, false);
	}
	return error;
}

/*————————————————————————————————————————————————————————————*/

void VoiceData(WindowRef theWindow, Ptr data, unsigned long length)
{
	register CFPtr vars;
	SndListHandle header;
	OSErr error;
	short headerLen;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (vars->state != 1)
		return;
	header = vars->header[vars->index];
	if (header == nil)
		return;
	error = SetupSndHeader(header, 1, vars->sampRate, 8, NoneType, 60, length * 6, &headerLen);
	if (error != noErr)
		return;
	Exp1to6(data, (Ptr) *header + headerLen, length, &vars->inState, &vars->outState, 1, 1);
	vars->index = (vars->index + 1) & 7;
	vars->count++;
	if (vars->wait != 0)
		vars->wait--;
	if (vars->wait == 0) {
		while (vars->count != 0) {
			(void) PlaySnd(vars->sndChan, vars->header[vars->outdex]);
			vars->outdex = (vars->outdex + 1) & 7;
			vars->count--;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

void EndVoice(WindowRef theWindow)
{
	extern void MCPSetClickState(Boolean);
	
	register CFPtr vars;
	SndListHandle header;
	short i;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (vars->state != 1)
		return;
	while (vars->count != 0) {
		(void) PlaySnd(vars->sndChan, vars->header[vars->outdex]);
		vars->outdex = (vars->outdex + 1) & 7;
		vars->count--;
	}
	(void) SndDisposeChannel(vars->sndChan, false);
	for (i = 0; i < 8; i++) {
		header = vars->header[i];
		if (header != nil)
			DisposeHandle((Handle) header);
	}
	vars->state = 0;
	MCPSetClickState(true);
	DrawPTT(vars);
	DrawName(vars);
}

/*————————————————————————————————————————————————————————————*/

void SendVoiceData(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (vars->state != 2)
		return;
	vars->count++;
}

/*————————————————————————————————————————————————————————————*/

static OSErr GetSamples(void *data, unsigned long length, BFPtr bufs)
{
	unsigned long extent;
	
	extent = 8192 - bufs->outdex;
	if (extent >= length)
		BlockMove(bufs->buffer + bufs->outdex, data, length);
	else {
		BlockMove(bufs->buffer + bufs->outdex, data, extent);
		BlockMove(bufs->buffer, (Ptr) data + extent, length - extent);
	}
	bufs->outdex = (bufs->outdex + length) & 8191;
	bufs->count -= length;
	if (bufs->flags & 0x80)
		return eofErr;
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static void DoRecord(CFPtr vars)
{
	extern OSErr MCPStartVoice(long, UnsignedFixed, OSType);
	extern OSErr MCPVoiceData(long, void *, unsigned long);
	extern OSErr MCPEndVoice(long);
	extern void MCPIdle(void);
	extern Boolean MCPOnLine(void);
	extern void MCPSetClickState(Boolean);
	extern void SIInterrupt(void);
	
	typedef struct {
		unsigned short length;
		Byte data[504];
	} AFRecord, *AFPtr;
	
	static Byte sampBuf[3024];
	static AFRecord buffer[4];
	
	register SPBPtr pb;
	SPB paramBlock;
	BFPtr bufs;
	SIInterruptUPP siInterruptProc;
	UnsignedFixed sampRate;
	OSType compType;
	unsigned long len;
	long inRefNum;
	OSErr error;
	short hasAsync, numChans, sampSize, continuous, agc, count, index, outdex;
	
	if (vars->state != 0) {
		SysBeep(30);
		return;
	}
	if (!hasSoundInput) {
		SysBeep(30);
		return;
	}
	error = SPBOpenDevice(nil, siWritePermission, &inRefNum);
	if (error != noErr) {
		SysBeep(30);
		return;
	}
	error = SPBGetDeviceInfo(inRefNum, siAsync, &hasAsync);
	if (error != noErr || hasAsync == 0) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	numChans = 1;
	error = SPBSetDeviceInfo(inRefNum, siNumberChannels, &numChans);
	if (error != noErr) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	sampRate = rate22050hz;
	error = SPBSetDeviceInfo(inRefNum, siSampleRate, &sampRate);
	if (error != noErr) {
		sampRate = rate22khz;
		error = SPBSetDeviceInfo(inRefNum, siSampleRate, &sampRate);
		if (error != noErr) {
			SPBCloseDevice(inRefNum);
			SysBeep(30);
			return;
		}
	}
	sampSize = 8;
	error = SPBSetDeviceInfo(inRefNum, siSampleSize, &sampSize);
	if (error != noErr) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	compType = NoneType;
	error = SPBSetDeviceInfo(inRefNum, siCompressionType, &compType);
	if (error != noErr) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	continuous = 1;
	(void) SPBSetDeviceInfo(inRefNum, siContinuous, &continuous);
	agc = 1;
	(void) SPBSetDeviceInfo(inRefNum, siAGCOnOff, &agc);
	bufs = (BFPtr) NewPtr(sizeof(BFRecord));
	if (bufs == nil) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	error = MCPStartVoice(vars->cid, sampRate / gRate, MACE6Type);
	if (error != noErr) {
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	vars->state = 2;
	vars->count = 4;
	count = index = outdex = 0;
	bufs->flags = 0x00;
	bufs->ratio = gRate;
	bufs->count = bufs->index = bufs->outdex = 0;
	memset(&vars->inState, 0, sizeof(StateBlock));
	memset(&vars->outState, 0, sizeof(StateBlock));
	MCPSetClickState(false);
	NotifySpeak(vars, gUserName);
	siInterruptProc = NewSIInterruptProc(SIInterrupt);
	pb = &paramBlock;
	pb->inRefNum = inRefNum;
	pb->count = 0;
	pb->milliseconds = 0;
	pb->bufferLength = 0;
	pb->bufferPtr = nil;
	pb->completionRoutine = nil;
	pb->interruptRoutine = siInterruptProc;
	pb->userLong = (long) bufs;
	pb->error = noErr;
	pb->unused1 = 0;
	error = SPBRecord(pb, true);
	if (error != noErr) {
		DisposeRoutineDescriptor(siInterruptProc);
		vars->state = 0;
		MCPSetClickState(true);
		MCPEndVoice(vars->cid);
		DisposePtr((Ptr) bufs);
		SPBCloseDevice(inRefNum);
		SysBeep(30);
		return;
	}
	while (StillDown()) {
		MCPIdle();
		if (bufs->count >= 3024) {
			error = GetSamples(sampBuf, 3024, bufs);
			if (error != noErr || count == 4) {
				DisposeRoutineDescriptor(siInterruptProc);
				vars->state = 0;
				MCPSetClickState(true);
				MCPEndVoice(vars->cid);
				DisposePtr((Ptr) bufs);
				SPBCloseDevice(inRefNum);
				SysBeep(30);
				return;
			}
			Comp6to1(sampBuf, buffer[index].data, 3024, &vars->inState, &vars->outState, 1, 1);
			buffer[index].length = 504;
			index = (index + 1) & 3;
			count++;
		}
		if (count != 0 && vars->count != 0) {
			error = MCPVoiceData(vars->cid, buffer[outdex].data, buffer[outdex].length);
			if (error != noErr) {
				DisposeRoutineDescriptor(siInterruptProc);
				vars->state = 0;
				MCPSetClickState(true);
				MCPEndVoice(vars->cid);
				DisposePtr((Ptr) bufs);
				SPBCloseDevice(inRefNum);
				SysBeep(30);
				return;
			}
			outdex = (outdex + 1) & 3;
			count--;
			vars->count--;
		}
	}
	error = SPBStopRecording(inRefNum);
	DisposeRoutineDescriptor(siInterruptProc);
	while (bufs->count >= 48) {
		len = bufs->count >= 3024 ? 3024 : (bufs->count / 48) * 48;
		error = GetSamples(sampBuf, len, bufs);
		if (error != noErr || count == 4) {
			vars->state = 0;
			MCPSetClickState(true);
			MCPEndVoice(vars->cid);
			DisposePtr((Ptr) bufs);
			SPBCloseDevice(inRefNum);
			SysBeep(30);
			return;
		}
		Comp6to1(sampBuf, buffer[index].data, len, &vars->inState, &vars->outState, 1, 1);
		buffer[index].length = len / 6;
		index = (index + 1) & 3;
		count++;
	}
	while (count != 0 && MCPOnLine()) {
		MCPIdle();
		if (count != 0 && vars->count != 0) {
			error = MCPVoiceData(vars->cid, buffer[outdex].data, buffer[outdex].length);
			if (error != noErr) {
				vars->state = 0;
				MCPSetClickState(true);
				MCPEndVoice(vars->cid);
				DisposePtr((Ptr) bufs);
				SPBCloseDevice(inRefNum);
				SysBeep(30);
				return;
			}
			outdex = (outdex + 1) & 3;
			count--;
			vars->count--;
		}
	}
	vars->state = 0;
	MCPSetClickState(true);
	MCPEndVoice(vars->cid);
	DisposePtr((Ptr) bufs);
	SPBCloseDevice(inRefNum);
}

/*————————————————————————————————————————————————————————————*/

static short TrackKeypad(CFPtr vars, Point thePoint)
{
	register KYPtr k;
	Point mouseLoc;
	short i;
	Boolean flag;
	
	k = vars->keys;
	for (i = 0; i < 7; i++) {
		if (k->visible && k->enabled && PtInIconID(thePoint, &k->box, atNone, k->iconID)) {
			if (i < 5) {
				PlotIconID(&k->box, atNone, ttNone, k->iconID + 1);
				flag = true;
				while (StillDown()) {
					GetMouse(&mouseLoc);
					if (PtInIconID(mouseLoc, &k->box, atNone, k->iconID)) {
						if (!flag) {
							PlotIconID(&k->box, atNone, ttNone, k->iconID + 1);
							flag = true;
						}
					} else {
						if (flag) {
							PlotIconID(&k->box, atNone, ttNone, k->iconID);
							flag = false;
						}
					}
				}
				if (flag) {
					PlotIconID(&k->box, atNone, ttNone, k->iconID);
					return i + 1;
				}
			} else {
				PlotIconID(&k->box, atNone, ttNone, k->iconID + 1);
				switch (i) {
				case 5:
					DoList(vars);
					break;
				case 6:
					DoRecord(vars);
					break;
				}
				PlotIconID(&k->box, atNone, ttNone, k->iconID);
			}
			return 0;
		}
		k++;
	}
	return 0;
}

/*————————————————————————————————————————————————————————————*/

static void SaveText(CFPtr vars)
{
	void UpdateConf(WindowRef);
	
	register TEPtr pTE;
	Str255 prompt, defaultName;
	StandardFileReply reply;
	RgnHandle updateRgn;
	TEHandle hTE;
	StScrpHandle style;
	long count;
	OSErr error;
	short refNum, selStart, selEnd;
	
	GetIndString(prompt, 300, 1);
	GetWTitle(vars->window, defaultName);
	SetCursorID(arrowCursor);
	StandardPutFile(prompt, defaultName, &reply);
	if (!reply.sfGood)
		return;
	SetCursorID(watchCursor);
	updateRgn = NewRgn();
	GetWindowUpdateRgn(vars->window, updateRgn);
	if (!EmptyRgn(updateRgn))
		UpdateConf(vars->window);
	DisposeRgn(updateRgn);
	if (reply.sfReplacing) {
		error = FSpDelete(&reply.sfFile);
		if (error != noErr) {
			SysBeep(30);
			return;
		}
	}
	FSpCreateResFile(&reply.sfFile, 'ttxt', 'TEXT', reply.sfScript);
	error = ResError();
	if (error != noErr) {
		SysBeep(30);
		return;
	}
	refNum = FSpOpenResFile(&reply.sfFile, fsCurPerm);
	error = ResError();
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	hTE = vars->oText;
	pTE = *hTE;
	selStart = pTE->selStart;
	selEnd = pTE->selEnd;
	pTE->selStart = 0;
	pTE->selEnd = 32767;
	style = TEGetStyleScrapHandle(hTE);
	pTE = *hTE;
	pTE->selStart = selStart;
	pTE->selEnd = selEnd;
	if (style == nil) {
		CloseResFile(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	AddResource((Handle) style, 'styl', 128, "\p");
	error = ResError();
	if (error != noErr) {
		CloseResFile(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	CloseResFile(refNum);
	error = ResError();
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	error = FSpOpenDF(&reply.sfFile, fsWrPerm, &refNum);
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	pTE = *hTE;
	count = pTE->teLength;
	error = FSWrite(refNum, &count, *pTE->hText);
	if (error != noErr) {
		(void) FSClose(refNum);
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
	error = FSClose(refNum);
	if (error != noErr) {
		(void) FSpDelete(&reply.sfFile);
		SysBeep(30);
		return;
	}
}

/*————————————————————————————————————————————————————————————*/

static void PrintText(CFPtr vars)
{
	register TEPtr pTE;
	TPrStatus prStatus;
	Rect rPaper, rPage, r;
	TPPrJob prJob;
	TPPrPort prPort;
	TEHandle hTE;
	Handle hText;
	short linesPerPage, numPages, firstPage, lastPage, inPage, pageNum, theNum;
	Boolean doPrint;
	
	SetCursorID(watchCursor);
	PrOpen();
	if (PrError() != noErr) {
		PrClose();
		SysBeep(30);
		return;
	}
	if (prRecHdl == nil) {
		prRecHdl = (THPrint) NewHandleClear(sizeof(TPrint));
		if (prRecHdl == nil) {
			PrClose();
			SysBeep(30);
			return;
		}
		PrintDefault(prRecHdl);
		if (PrError() != noErr) {
			DisposeHandle((Handle) prRecHdl);
			prRecHdl = nil;
			PrClose();
			SysBeep(30);
			return;
		}
	} else {
		(void) PrValidate(prRecHdl);
		if (PrError() != noErr) {
			PrClose();
			SysBeep(30);
			return;
		}
	}
	SetCursorID(arrowCursor);
	doPrint = PrJobDialog(prRecHdl);
	if (!doPrint) {
		PrClose();
		return;
	}
	SetCursorID(watchCursor);
	rPaper = (*prRecHdl)->rPaper;
	rPage = (*prRecHdl)->prInfo.rPage;
	if (rPaper.bottom - rPage.bottom < rPage.top - rPaper.top)
		rPage.bottom = rPaper.bottom - rPage.top + rPaper.top;
	prJob = &(*prRecHdl)->prJob;
	firstPage = prJob->iFstPage;
	lastPage = prJob->iLstPage;
	prJob->iFstPage = 1;
	prJob->iLstPage = iPrPgMax;
	prPort = PrOpenDoc(prRecHdl, nil, nil);
	if (PrError() != noErr) {
		PrCloseDoc(prPort);
		PrClose();
		SysBeep(30);
		return;
	}
	GetFNum("\pCourier", &theNum);
	TextFont(theNum);
	TextSize(10);
	hTE = TENew(&rPage, &rPage);
	if (hTE == nil) {
		PrCloseDoc(prPort);
		PrClose();
		SysBeep(30);
		return;
	}
	pTE = *hTE;
	r = rPage;
	r.bottom -= (r.bottom - r.top) % pTE->lineHeight;
	pTE->destRect = r;
	pTE->viewRect = r;
	hText = pTE->hText;
	pTE->hText = (*vars->oText)->hText;
	pTE->teLength = (*vars->oText)->teLength;
	TECalText(hTE);
	pTE = *hTE;
	linesPerPage = (pTE->viewRect.bottom - pTE->viewRect.top) / pTE->lineHeight;
	numPages = (pTE->nLines + linesPerPage - 1) / linesPerPage;
	if (lastPage > numPages)
		lastPage = numPages;
	inPage = 1;
	for (pageNum = firstPage; pageNum <= lastPage; pageNum++) {
		PrOpenPage(prPort, nil);
		if (PrError() != noErr) {
			PrClosePage(prPort);
			(*hTE)->hText = hText;
			TEDispose(hTE);
			PrCloseDoc(prPort);
			PrClose();
			SysBeep(30);
			return;
		}
		if (inPage != pageNum) {
			pTE = *hTE;
			OffsetRect(&pTE->destRect, 0,
				(pTE->viewRect.top - pTE->viewRect.bottom) * (pageNum - inPage));
			inPage = pageNum;
		}
		TEUpdate(&rPage, hTE);
		PrClosePage(prPort);
		if (PrError() != noErr) {
			(*hTE)->hText = hText;
			TEDispose(hTE);
			PrCloseDoc(prPort);
			PrClose();
			SysBeep(30);
			return;
		}
	}
	(*hTE)->hText = hText;
	TEDispose(hTE);
	PrCloseDoc(prPort);
	if (PrError() != noErr) {
		PrClose();
		SysBeep(30);
		return;
	}
	if ((*prRecHdl)->prJob.bJDocLoop == bSpoolLoop) {
		PrPicFile(prRecHdl, nil, nil, nil, &prStatus);
		if (PrError() != noErr) {
			PrClose();
			SysBeep(30);
			return;
		}
	}
	PrClose();
}

/*————————————————————————————————————————————————————————————*/

static void GetText(TEHandle hTE, Str255 text, short maxLen)
{
	register unsigned char *p;
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

static void SendLine(CFPtr vars)
{
	extern OSErr MCPMessage(long, Str255);
	
	Str255 text;
	long cid;
	
	ActivateInput(vars);
	cid = vars->cid;
	GetText(vars->iText, text, 255);
	(void) MCPMessage(cid, text);
	TEDeactivate(vars->iText);
	TESetSelect(0, 32767, vars->iText);
	TEDelete(vars->iText);
	TEActivate(vars->iText);
}

/*————————————————————————————————————————————————————————————*/

static Boolean LocationIsTrash(AEDesc *dropLocation)
{
	CInfoPBRec pb;
	AEDesc dropSpec;
	FSSpecPtr theSpec;
	long trashDirID;
	OSErr error;
	short trashVRefNum;
	
	if (dropLocation->descriptorType == typeNull)
		return false;
	error = AECoerceDesc(dropLocation, typeFSS, &dropSpec);
	if (error != noErr)
		return false;
	HLock(dropSpec.dataHandle);
	theSpec = (FSSpecPtr) *dropSpec.dataHandle;
	pb.dirInfo.ioNamePtr = theSpec->name;
	pb.dirInfo.ioVRefNum = theSpec->vRefNum;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioDrDirID = theSpec->parID;
	error = PBGetCatInfoSync(&pb);
	if (error != noErr || !(pb.dirInfo.ioFlAttrib & ioDirMask)) {
		AEDisposeDesc(&dropSpec);
		return false;
	}
	error = FindFolder(theSpec->vRefNum, kTrashFolderType, kCreateFolder,
					   &trashVRefNum, &trashDirID);
	if (error != noErr) {
		AEDisposeDesc(&dropSpec);
		return false;
	}
	AEDisposeDesc(&dropSpec);
	return pb.dirInfo.ioDrDirID == trashDirID;
}

/*————————————————————————————————————————————————————————————*/

static Size GetSelectionSize(TEHandle hTE)
{
	register TEPtr pTE;
	
	pTE = *hTE;
	return pTE->selEnd - pTE->selStart;
}

/*————————————————————————————————————————————————————————————*/

static Ptr GetSelectedTextPtr(TEHandle hTE)
{
	register TEPtr pTE;
	
	pTE = *hTE;
	return *pTE->hText + pTE->selStart;
}

/*————————————————————————————————————————————————————————————*/

static Boolean DragText(CFPtr vars, const EventRecord *theEvent, Point thePoint, TEHandle hTE)
{
	AEDesc dropLocation;
	Point mouseLoc;
	RgnHandle hiliteRgn, tempRgn;
	DragReference theDrag;
	DragAttributes attributes;
	OSErr error;
	short downMods, upMods;
	
	if (!hasGetHiliteRgn)
		return false;
	hiliteRgn = NewRgn();
	error = TEGetHiliteRgn(hiliteRgn, hTE);
	if (error != noErr || !PtInRgn(thePoint, hiliteRgn)) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	if (!hasDragMgr) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	if (!WaitMouseMoved(theEvent->where)) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	error = NewDrag(&theDrag);
	if (error != noErr) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	error = AddDragItemFlavor(theDrag, 1, 'TEXT', GetSelectedTextPtr(hTE),
							  GetSelectionSize(hTE), 0);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		return false;
	}
	SetPt(&mouseLoc, 0, 0);
	LocalToGlobal(&mouseLoc);
	OffsetRgn(hiliteRgn, mouseLoc.h, mouseLoc.v);
	error = SetDragItemBounds(theDrag, 1, &(*hiliteRgn)->rgnBBox);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		return false;
	}
	tempRgn = NewRgn();
	CopyRgn(hiliteRgn, tempRgn);
	InsetRgn(tempRgn, 1, 1);
	DiffRgn(hiliteRgn, tempRgn, hiliteRgn);
	DisposeRgn(tempRgn);
	error = TrackDrag(theDrag, theEvent, hiliteRgn);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		if (error == userCanceledErr)
			return true;
		else
			return false;
	}
	error = GetDragAttributes(theDrag, &attributes);
	if (error == noErr && !(attributes & dragInsideSenderApplication)) {
		error = GetDragModifiers(theDrag, nil, &downMods, &upMods);
		if (error == noErr && !((downMods | upMods) & optionKey)) {
			error = GetDropLocation(theDrag, &dropLocation);
			if (error == noErr) {
				if (LocationIsTrash(&dropLocation))
					TEDelete(hTE);
				AEDisposeDesc(&dropLocation);
			}
		}
	}
	DisposeDrag(theDrag);
	DisposeRgn(hiliteRgn);
	return true;
}

/*————————————————————————————————————————————————————————————*/

static Boolean DragStyleText(CFPtr vars, const EventRecord *theEvent, Point thePoint, TEHandle hTE)
{
	Point mouseLoc;
	RgnHandle hiliteRgn, tempRgn;
	DragReference theDrag;
	StScrpHandle style;
	OSErr error;
	
	if (!hasGetHiliteRgn)
		return false;
	hiliteRgn = NewRgn();
	error = TEGetHiliteRgn(hiliteRgn, hTE);
	if (error != noErr || !PtInRgn(thePoint, hiliteRgn)) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	if (!hasDragMgr) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	if (!WaitMouseMoved(theEvent->where)) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	error = NewDrag(&theDrag);
	if (error != noErr) {
		DisposeRgn(hiliteRgn);
		return false;
	}
	error = AddDragItemFlavor(theDrag, 1, 'TEXT', GetSelectedTextPtr(hTE), GetSelectionSize(hTE), 0);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		return false;
	}
	style = TEGetStyleScrapHandle(hTE);
	if (style != nil) {
		HLock((Handle) style);
		error = AddDragItemFlavor(theDrag, 1, 'styl', *style, GetHandleSize((Handle) style), 0);
		DisposeHandle((Handle) style);
		if (error != noErr) {
			DisposeDrag(theDrag);
			DisposeRgn(hiliteRgn);
			return false;
		}
	}
	SetPt(&mouseLoc, 0, 0);
	LocalToGlobal(&mouseLoc);
	OffsetRgn(hiliteRgn, mouseLoc.h, mouseLoc.v);
	error = SetDragItemBounds(theDrag, 1, &(*hiliteRgn)->rgnBBox);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		return false;
	}
	tempRgn = NewRgn();
	CopyRgn(hiliteRgn, tempRgn);
	InsetRgn(tempRgn, 1, 1);
	DiffRgn(hiliteRgn, tempRgn, hiliteRgn);
	DisposeRgn(tempRgn);
	error = TrackDrag(theDrag, theEvent, hiliteRgn);
	if (error != noErr) {
		DisposeDrag(theDrag);
		DisposeRgn(hiliteRgn);
		if (error == userCanceledErr)
			return true;
		else
			return false;
	}
	DisposeDrag(theDrag);
	DisposeRgn(hiliteRgn);
	return true;
}

/*————————————————————————————————————————————————————————————*/

void ClickConf(WindowRef theWindow, const EventRecord *theEvent, Point thePoint)
{
	extern OSErr ListMembers(long);
	extern OSErr InviteMore(long);
	extern WindowRef NewGraf(long, WindowRef);
	
	register CFPtr vars;
	register TEPtr pTE;
	Rect theRect;
	ControlRef theControl;
	short thePart, value;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	thePart = FindControl(thePoint, theWindow, &theControl);
	if (theControl == vars->vScroll)
		switch (thePart) {
		case inThumb:
			value = GetControlValue(vars->vScroll);
			thePart = TrackControl(theControl, thePoint, nil);
			if (thePart != 0) {
				value -= GetControlValue(vars->vScroll);
				if (value != 0) {
					pTE = *vars->oText;
					TEScroll(0, value * pTE->lineHeight, vars->oText);
				}
			}
			break;
		default:
			thePart = TrackControl(theControl, thePoint, scrollTextProc);
			break;
		}
	else if (theControl == nil)
		if (PtInRect(thePoint, &(*vars->iText)->viewRect)) {
			ActivateInput(vars);
			if (!DragText(vars, theEvent, thePoint, vars->iText)) {
				TEClick(thePoint, (theEvent->modifiers & shiftKey) != 0, vars->iText);
			}
		} else if (PtInRect(thePoint, &(*vars->oText)->viewRect)) {
			ActivateOutput(vars);
			if (!DragStyleText(vars, theEvent, thePoint, vars->oText)) {
				TESetClickLoop(autoScrollProc, vars->oText);
				TEClick(thePoint, (theEvent->modifiers & shiftKey) != 0, vars->oText);
			}
		} else if (PtInRect(thePoint, &vars->padRect)) {
			switch (TrackKeypad(vars, thePoint)) {
			case 1:
				SaveText(vars);
				break;
			case 2:
				PrintText(vars);
				break;
			case 3:
				(void) ListMembers(vars->cid);
				break;
			case 4:
				(void) InviteMore(vars->cid);
				break;
			case 5:
				if (vars->grafWin == nil)
					vars->grafWin = NewGraf(vars->cid, theWindow);
				else
					SelectWindow(vars->grafWin);
				break;
			}
		}
}

/*————————————————————————————————————————————————————————————*/

void KeyConf(WindowRef theWindow, long code)
{
	register CFPtr vars;
	register TEPtr pTE;
	short charCode;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	charCode = code & charCodeMask;
	if (charCode < ' ' || charCode == DEL) {
		if (charCode == CR || charCode == ETX)
			SendLine(vars);
		else if (charCode == HT) {
			if (vars->inAct)
				ActivateOutput(vars);
			else
				ActivateInput(vars);
		} else if (charCode == BS || (charCode >= FS && charCode <= US)) {
			ActivateInput(vars);
			TEKey(charCode, vars->iText);
		} else
			SysBeep(30);
	} else {
		ActivateInput(vars);
		pTE = *vars->iText;
		if (pTE->teLength + pTE->selStart - pTE->selEnd + 1 <= 255)
			TEKey(charCode, vars->iText);
		else
			SysBeep(30);
	}
}

/*————————————————————————————————————————————————————————————*/

static void DrawPad(CFPtr vars)
{
	register KYPtr k;
	Rect theRect;
	RGBColor saveColor;
	CGrafPtr windowPort;
	RgnHandle saveClip;
	short i;
	
	FrameRect(&vars->padRect);
	windowPort = GetWindowPort(vars->window);
	if (windowPort->portVersion < 0 && (*windowPort->portPixMap)->pixelSize >= 4) {
		GetForeColor(&saveColor);
		theRect = vars->padRect;
		InsetRect(&theRect, 1, 1);
		theRect.bottom -= 1;
		theRect.right -= 1;
		PmForeColor(1);
		MoveTo(theRect.left, theRect.bottom);
		LineTo(theRect.left, theRect.top);
		LineTo(theRect.right, theRect.top);
		PmForeColor(2);
		MoveTo(theRect.left, theRect.bottom);
		LineTo(theRect.right, theRect.bottom);
		LineTo(theRect.right, theRect.top);
		theRect.top += 1;
		theRect.left += 1;
		PmForeColor(3);
		PaintRect(&theRect);
		RGBForeColor(&saveColor);
	}
	k = vars->keys;
	for (i = 0; i < 7; i++) {
		if (k->visible)
			PlotIconID(&k->box, atNone, k->enabled ? ttNone : ttDisabled, k->iconID);
		k++;
	}
	if (vars->state == 1) {
		saveClip = NewRgn();
		GetClip(saveClip);
		theRect = vars->nameRect;
		ClipRect(&theRect);
		MoveTo(theRect.left, theRect.top + 9);
		DrawString(vars->name);
		SetClip(saveClip);
		DisposeRgn(saveClip);
	}
}

/*————————————————————————————————————————————————————————————*/

static void DrawField(CFPtr vars)
{
	Rect theRect;
	
	theRect = (*vars->iText)->viewRect;
	InsetRect(&theRect, -3, -3);
	FrameRect(&theRect);
}

/*————————————————————————————————————————————————————————————*/

void UpdateConf(WindowRef theWindow)
{
	register CFPtr vars;
	Rect portRect;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	portRect = GetWindowPort(theWindow)->portRect;
	BeginUpdate(theWindow);
	EraseRect(&portRect);
	DrawControls(theWindow);
	DrawPad(vars);
	DrawField(vars);
	MyDrawGrowIcon(vars);
	TEUpdate(&portRect, vars->iText);
	TEUpdate(&portRect, vars->oText);
	EndUpdate(theWindow);
}

/*————————————————————————————————————————————————————————————*/

void ActivateConf(WindowRef theWindow, Boolean theFlag)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (theFlag != vars->active)
		if (theFlag) {
			vars->active = true;
			ShowControl(vars->vScroll);
			MyDrawGrowIcon(vars);
			if (vars->inAct)
				TEActivate(vars->iText);
			else
				TEActivate(vars->oText);
		} else {
			vars->active = false;
			HideControl(vars->vScroll);
			MyDrawGrowIcon(vars);
			if (vars->inAct)
				TEDeactivate(vars->iText);
			else
				TEDeactivate(vars->oText);
		}
}

/*————————————————————————————————————————————————————————————*/

void IdleConf(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	TEIdle(vars->iText);
}

/*————————————————————————————————————————————————————————————*/

void AdjCurConf(WindowRef theWindow, Point mouseLoc)
{
	register CFPtr vars;
	RgnHandle hiliteRgn;
	OSErr error;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (PtInRect(mouseLoc, &(*vars->iText)->viewRect)) {
		if (hasGetHiliteRgn) {
			hiliteRgn = NewRgn();
			error = TEGetHiliteRgn(hiliteRgn, vars->iText);
			if (error == noErr && PtInRgn(mouseLoc, hiliteRgn))
				SetCursorID(arrowCursor);
			else
				SetCursorID(iBeamCursor);
			DisposeRgn(hiliteRgn);
		} else
			SetCursorID(iBeamCursor);
	} else if (PtInRect(mouseLoc, &(*vars->oText)->viewRect)) {
		if (hasGetHiliteRgn) {
			hiliteRgn = NewRgn();
			error = TEGetHiliteRgn(hiliteRgn, vars->oText);
			if (error == noErr && PtInRgn(mouseLoc, hiliteRgn))
				SetCursorID(arrowCursor);
			else
				SetCursorID(iBeamCursor);
			DisposeRgn(hiliteRgn);
		} else
			SetCursorID(iBeamCursor);
	} else
		SetCursorID(arrowCursor);
}

/*————————————————————————————————————————————————————————————*/

void GrowConf(WindowRef theWindow, Point mouseLoc)
{
	register CFPtr vars;
	Rect theRect;
	CGrafPtr windowPort;
	long newSize;
	short w, h;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	SetRect(&theRect, 251, 111, 1550, 32000);
	newSize = GrowWindow(theWindow, mouseLoc, &theRect);
	if (newSize != 0) {
		windowPort = GetWindowPort(theWindow);
		EraseRect(&windowPort->portRect);
		h = HiWord(newSize);
		w = LoWord(newSize);
		SizeWindow(theWindow, w, h, false);
		ResizeWindow(vars);
		InvalRect(&windowPort->portRect);
	}
}

/*————————————————————————————————————————————————————————————*/

void ZoomConf(WindowRef theWindow, short partCode)
{
	register CFPtr vars;
	CGrafPtr windowPort;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	windowPort = GetWindowPort(theWindow);
	EraseRect(&windowPort->portRect);
	ZoomWindow(theWindow, partCode, true);
	ResizeWindow(vars);
	InvalRect(&windowPort->portRect);
}

/*————————————————————————————————————————————————————————————*/

void AdjFileConf(WindowRef theWindow, MenuRef theMenu)
{
	EnableItem(theMenu, CloseItem);
	EnableItem(theMenu, SaveAsItem);
	EnableItem(theMenu, PrintItem);
}

/*————————————————————————————————————————————————————————————*/

void AdjEditConf(WindowRef theWindow, MenuRef theMenu)
{
	register CFPtr vars;
	register TEPtr pTE;
	long offset;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	DisableItem(theMenu, UndoItem);
	if (vars->inAct) {
		pTE = *vars->iText;
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
		pTE = *vars->iText;
		if (pTE->teLength > 0)
			EnableItem(theMenu, SelectAllItem);
		else
			DisableItem(theMenu, SelectAllItem);
	} else {
		DisableItem(theMenu, CutItem);
		pTE = *vars->oText;
		if (pTE->selEnd - pTE->selStart > 0)
			EnableItem(theMenu, CopyItem);
		else
			DisableItem(theMenu, CopyItem);
		DisableItem(theMenu, PasteItem);
		DisableItem(theMenu, ClearItem);
		pTE = *vars->oText;
		if (pTE->teLength > 0)
			EnableItem(theMenu, SelectAllItem);
		else
			DisableItem(theMenu, SelectAllItem);
	}
}

/*————————————————————————————————————————————————————————————*/

void SaveConf(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	SaveText(vars);
}

/*————————————————————————————————————————————————————————————*/

void PrintConf(WindowRef theWindow)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	PrintText(vars);
}

/*————————————————————————————————————————————————————————————*/

void EditConf(WindowRef theWindow, short menuItem)
{
	register CFPtr vars;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	if (vars->inAct) {
		switch (menuItem) {
		case CutItem:
			TECut(vars->iText);
			if (ZeroScrap() == noErr)
				(void) TEToScrap();
			break;
		case CopyItem:
			TECopy(vars->iText);
			if (ZeroScrap() == noErr)
				(void) TEToScrap();
			break;
		case PasteItem:
			if (TEFromScrap() == noErr && TEGetScrapLength() <= 255)
				TEPaste(vars->iText);
			break;
		case ClearItem:
			TEDelete(vars->iText);
			break;
		case SelectAllItem:
			TESetSelect(0, 32767, vars->iText);
			break;
		}
	} else {
		switch (menuItem) {
		case CopyItem:
			TECopy(vars->oText);
			break;
		case SelectAllItem:
			TESetSelect(0, 32767, vars->oText);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

void AddToConf(WindowRef theWindow, short style, const void *text, long length)
{
	register CFPtr vars;
	register TEPtr pTE;
	static RGBColor colors[] = {{0, 0, 0}, {30583, 30583, 30583}, {65535, 0, 0}};
	TextStyle newStyle;
	GrafPtr savePort;
	RgnHandle updateRgn;
	short selStart, selEnd, saveVis, value, max, nLines, vLines;
	Boolean flag;
	
	vars = (CFPtr) GetWRefCon(theWindow);
	pTE = *vars->oText;
	if (pTE->teLength + length <= 32767) {
		GetPort(&savePort);
		SetPortWindowPort(theWindow);
		
		updateRgn = NewRgn();
		GetWindowUpdateRgn(theWindow, updateRgn);
		if (!EmptyRgn(updateRgn))
			UpdateConf(theWindow);
		DisposeRgn(updateRgn);
		
		pTE = *vars->oText;
		selStart = pTE->selStart;
		selEnd = pTE->selEnd;
		TESetSelect(32767, 32767, vars->oText);
		
		if (style >= 0 && style <= 2)
			newStyle.tsColor = colors[style];
		else
			newStyle.tsColor = colors[0];
		TESetStyle(doColor, &newStyle, true, vars->oText);
		
		TEInsert(text, length, vars->oText);
		
		TESetSelect(selStart, selEnd, vars->oText);
		
		saveVis = GetControlVis(vars->vScroll);
		SetControlVis(vars->vScroll, 0);
		
		value = GetControlValue(vars->vScroll);
		max = GetControlMaximum(vars->vScroll);
		flag = value == max;
		
		pTE = *vars->oText;
		nLines = pTE->nLines;
		vLines = (pTE->viewRect.bottom - pTE->viewRect.top) / pTE->lineHeight;
		if (vLines > nLines) vLines = nLines;
		max = nLines - vLines;
		SetControlMaximum(vars->vScroll, max);
		SetSmartScrollInfo(vars->vScroll, vLines, nLines);
		
		if (flag) {
			value = GetControlValue(vars->vScroll);
			max = GetControlMaximum(vars->vScroll);
			SetControlValue(vars->vScroll, max);
			value -= GetControlValue(vars->vScroll);
			if (value != 0) {
				pTE = *vars->oText;
				TEScroll(0, value * pTE->lineHeight, vars->oText);
			}
		}
		
		SetControlVis(vars->vScroll, saveVis);
		Draw1Control(vars->vScroll);
		
		SetPort(savePort);
	}
}

/*————————————————————————————————————————————————————————————*/

#pragma segment Initialize

/*————————————————————————————————————————————————————————————*/

void InitConf(void)
{
	scrollTextProc = NewControlActionProc(ScrollText);
	autoScrollProc = NewTEClickLoopProc(AutoScroll);
	if (hasDragMgr) {
		trackingHandler = NewDragTrackingHandlerProc(TrackingHandler);
		receiveHandler = NewDragReceiveHandlerProc(ReceiveHandler);
	}
}

/*————————————————————————————————————————————————————————————*/
