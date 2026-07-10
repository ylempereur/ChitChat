/* Misc.h */

/*————————————————————————————————————————————————————————————*/

#include <Lists.h>
#include <Dialogs.h>

/*————————————————————————————————————————————————————————————*/

enum {arrowCursor};

enum {
	ETX = 0x03,
	BS  = 0x08,
	HT  = 0x09,
	CR  = 0x0D,
	ESC = 0x1B,
	FS  = 0x1C,
	GS  = 0x1D,
	RS  = 0x1E,
	US  = 0x1F,
	DEL = 0x7F
};

/*————————————————————————————————————————————————————————————*/

typedef unsigned char Str8[9];

typedef struct {
	Str32 name;
	Str255 text;
} MacroRec, *MacroPtr;

typedef struct {
	short count;
	MacroRec macros[50];
} MacroList, *MLPtr, **MLHandle;

/*————————————————————————————————————————————————————————————*/

#define GetSBYTE(p) (*((SInt8 *) (p))++)
#define GetUBYTE(p) (*((UInt8 *) (p))++)
#define GetSWORD(p) (*((SInt16 *) (p))++)
#define GetUWORD(p) (*((UInt16 *) (p))++)
#define GetSLONG(p) (*((SInt32 *) (p))++)
#define GetULONG(p) (*((UInt32 *) (p))++)

#define PutSBYTE(x, p) (*((SInt8 *) (p))++ = (x))
#define PutUBYTE(x, p) (*((UInt8 *) (p))++ = (x))
#define PutSWORD(x, p) (*((SInt16 *) (p))++ = (x))
#define PutUWORD(x, p) (*((UInt16 *) (p))++ = (x))
#define PutSLONG(x, p) (*((SInt32 *) (p))++ = (x))
#define PutULONG(x, p) (*((UInt32 *) (p))++ = (x))

/*————————————————————————————————————————————————————————————*/

extern short DisableInt(void);
extern void RestoreInt(short status);

void NOP(void) = 0x4E71;

/*————————————————————————————————————————————————————————————*/

extern void SetCursorID(short cursorID);
extern void YieldToSystem(void);
extern WindowRef GetControlOwner(ControlRef theControl);
extern void GetControlRect(ControlRef theControl, Rect *boundsRect);
extern void SetControlVis(ControlRef theControl, short visState);
extern short GetControlVis(ControlRef theControl);
extern TEHandle GetDialogTextH(DialogRef theDialog);
extern void LSetSelFlags(short selFlags, ListRef lHandle);
extern void LGetViewRect(Rect *rView, ListRef lHandle);
extern void LGetVisible(Rect *visible, ListRef lHandle);
extern void LGetDataBounds(Rect *dataBounds, ListRef lHandle);
extern WindowRef NewColorWindow(void *wStorage, const Rect *boundsRect, ConstStr255Param title, Boolean visible, short procID, WindowRef behind, Boolean goAwayFlag, long refCon);
extern WindowRef GetNewColorWindow(short windowID, void *wStorage, WindowRef behind);
extern short GetDialogItemValue(DialogRef theDialog, short itemNo);
extern void SetDialogItemValue(DialogRef theDialog, short itemNo, short newValue);
extern void NGetDialogItemText(DialogRef theDialog, short itemNo, Str255 text, short maxLen);
extern void NSetDialogItemText(DialogRef theDialog, short itemNo, ConstStr255Param text);
extern void SetDialogItemHilite(DialogRef theDialog, short itemNo, ControlPartCode hiliteState);
extern void GetDialogItemBox(DialogRef theDialog, short itemNo, Rect *box);
extern void SetDialogItemProc(DialogRef theDialog, short itemNo, UserItemUPP userItem);
extern void CenterDialog(short dialogID);
extern void CenterRect(Rect *r);
extern Boolean CheckEnvirons(void);
extern void CheckTimeout(void);

/*————————————————————————————————————————————————————————————*/
