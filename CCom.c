/* CCom.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <string.h>
#include <Strings.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <Resources.h>
#include <AppleTalk.h>
#include <ADSP.h>
#include <Sound.h>
#include <Windows.h>

#include "IPC.h"
#include "Protocol.h"
#include "Misc.h"
#include "Client.h"

/*————————————————————————————————————————————————————————————*/

#define EOF (-1)

/*————————————————————————————————————————————————————————————*/

enum {
	kLogin,
	kChPasswd,
	kNewConf,
	kGetConfList,
	kGetNextConf,
	kGetUserList,
	kGetNextUser,
	kGetMemberList,
	kGetNextMember,
	kInvite,
	kJoin,
	kLeave,
	kMessage,
	kStartGraf,
	kEndGraf,
	kStartVoice,
	kEndVoice
};

/*————————————————————————————————————————————————————————————*/

typedef struct {
	short type;
	Byte data[510];
} PackRec, *PackPtr;

/*————————————————————————————————————————————————————————————*/

#define MCPParamHeader \
	short code; \
	OSErr result;

typedef struct {
	MCPParamHeader
	long *uid;
	StringPtr name;
} MCPLoginPB;

typedef struct {
	MCPParamHeader
} MCPPasswdPB;

typedef struct {
	MCPParamHeader
	long *cid;
} MCPNewConfPB;

typedef struct {
	MCPParamHeader
	long *cid;
	StringPtr title;
} MCPConfListPB;

typedef struct {
	MCPParamHeader
	long *uid;
	StringPtr name;
} MCPUserListPB;

typedef struct {
	MCPParamHeader
	long *uid;
	StringPtr name;
} MCPMemberListPB;

typedef struct {
	MCPParamHeader
} MCPInvitePB;

typedef struct {
	MCPParamHeader
} MCPJoinPB;

typedef struct {
	MCPParamHeader
} MCPLeavePB;

typedef struct {
	MCPParamHeader
} MCPMessagePB;

typedef struct {
	MCPParamHeader
} MCPStartGrafPB;

typedef struct {
	MCPParamHeader
} MCPEndGrafPB;

typedef struct {
	MCPParamHeader
} MCPStartVoicePB;

typedef struct {
	MCPParamHeader
} MCPEndVoicePB;

typedef union {
	struct {MCPParamHeader} header;
	MCPLoginPB loginPB;
	MCPPasswdPB passPB;
	MCPNewConfPB newCnfPB;
	MCPConfListPB cnfLstPB;
	MCPUserListPB usrLstPB;
	MCPMemberListPB memLstPB;
	MCPInvitePB invitePB;
	MCPJoinPB joinPB;
	MCPLeavePB leavePB;
	MCPMessagePB mesgPB;
	MCPStartGrafPB strGrfPB;
	MCPEndGrafPB endGrfPB;
	MCPStartVoicePB strVoiPB;
	MCPEndVoicePB endVoiPB;
} MCPParamBlock, *MCPParamBlockPtr;

/*————————————————————————————————————————————————————————————*/

extern Str32 gUserName;
extern Str8 gPassword;
long gUID;

static MCPParamBlock gPB;
static IPCParamBlock gParamBlock[2];
static PackRec gPack[2];
static SndListHandle gClick = nil;
static short gRefNum;
static short gFlags = 0x0000;
static short gClickState = 0;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

/*————————————————————————————————————————————————————————————*/

OSErr PD_RPL_YOUREIN(PackPtr thePack, long *uid, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*uid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_RPL_CREATED(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_RPL_LIST(PackPtr thePack, long *cid, Str32 title)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = title;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_RPL_USERS(PackPtr thePack, long *uid, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*uid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_RPL_MEMBERS(PackPtr thePack, long *uid, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*uid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_INVITE(PackPtr thePack, long *cid, Str32 title, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = title;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_NOTICE(PackPtr thePack, long *cid, short *code, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*code = GetSWORD(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_MESSAGE(PackPtr thePack, long *cid, long *uid, Str32 name, Str255 text)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*uid = GetSLONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	q = text;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_STARTGRAF(PackPtr thePack, long *cid, long *picSize, Rect *picFrame, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*picSize = GetSLONG(p);
	picFrame->top = GetSWORD(p);
	picFrame->left = GetSWORD(p);
	picFrame->bottom = GetSWORD(p);
	picFrame->right = GetSWORD(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_GRAFDATA(PackPtr thePack, long *cid, Ptr data, long *count)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	n = GetSWORD(p);
	if (n <= 0 || n > 500)
		return paramErr;
	q = (Byte *) data;
	*count = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_ACKGRAF(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_ENDGRAF(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_STARTVOICE(PackPtr thePack, long *cid, UnsignedFixed *sampRate, OSType *compType, Str32 name)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*sampRate = GetULONG(p);
	*compType = GetULONG(p);
	n = GetUBYTE(p);
	if (n == 0 || n > 32)
		return paramErr;
	q = name;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_VOICEDATA(PackPtr thePack, long *cid, Ptr data, unsigned long *count)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	n = GetSWORD(p);
	if (n <= 0 || n > 504)
		return paramErr;
	q = (Byte *) data;
	*count = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_ACKVOICE(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_ENDVOICE(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_LOGIN(PackPtr thePack, unsigned short version, Str32 name, Str8 password)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_LOGIN;
	p = thePack->data;
	PutUWORD(version, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = password;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_QUIT(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = CMD_QUIT;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_CHPASSWD(PackPtr thePack, Str8 oPassword, Str8 nPassword)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_CHPASSWD;
	p = thePack->data;
	q = oPassword;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = nPassword;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_NEWCONF(PackPtr thePack, Boolean private, Str32 title)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_NEWCONF;
	p = thePack->data;
	PutUBYTE(private, p);
	q = title;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_LIST(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = CMD_LIST;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_USERS(PackPtr thePack, long id)
{
	Byte *p;
	
	thePack->type = CMD_USERS;
	p = thePack->data;
	PutSLONG(id, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_MEMBERS(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_MEMBERS;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_INVITE(PackPtr thePack, long cid, long uid)
{
	Byte *p;
	
	thePack->type = CMD_INVITE;
	p = thePack->data;
	PutSLONG(cid, p);
	PutSLONG(uid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_JOIN(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_JOIN;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_LEAVE(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_LEAVE;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_MESSAGE(PackPtr thePack, long id, Str255 text)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_MESSAGE;
	p = thePack->data;
	PutSLONG(id, p);
	q = text;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_STARTGRAF(PackPtr thePack, long cid, long picSize, Rect *picFrame)
{
	Byte *p;
	
	thePack->type = CMD_STARTGRAF;
	p = thePack->data;
	PutSLONG(cid, p);
	PutSLONG(picSize, p);
	PutSWORD(picFrame->top, p);
	PutSWORD(picFrame->left, p);
	PutSWORD(picFrame->bottom, p);
	PutSWORD(picFrame->right, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_GRAFDATA(PackPtr thePack, long cid, Ptr data, long count)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_GRAFDATA;
	p = thePack->data;
	PutSLONG(cid, p);
	q = (Byte *) data;
	n = count;
	PutSWORD(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_ENDGRAF(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_ENDGRAF;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_STARTVOICE(PackPtr thePack, long cid, UnsignedFixed sampRate, OSType compType)
{
	Byte *p;
	
	thePack->type = CMD_STARTVOICE;
	p = thePack->data;
	PutSLONG(cid, p);
	PutULONG(sampRate, p);
	PutULONG(compType, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_VOICEDATA(PackPtr thePack, long cid, Ptr data, unsigned long count)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_VOICEDATA;
	p = thePack->data;
	PutSLONG(cid, p);
	q = (Byte *) data;
	n = count;
	PutSWORD(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_ENDVOICE(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_ENDVOICE;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_NONE(PackPtr thePack)
{
	gPB.header.result = noErr;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_YOUREIN(PackPtr thePack)
{
	gPB.loginPB.result = PD_RPL_YOUREIN(thePack, gPB.loginPB.uid, gPB.loginPB.name);
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_CHANGED(PackPtr thePack)
{
	gPB.passPB.result = noErr;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_CREATED(PackPtr thePack)
{
	gPB.newCnfPB.result = PD_RPL_CREATED(thePack, gPB.newCnfPB.cid);
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_LIST(PackPtr thePack)
{
	gPB.cnfLstPB.result = PD_RPL_LIST(thePack, gPB.cnfLstPB.cid, gPB.cnfLstPB.title);
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_LISTEND(PackPtr thePack)
{
	gPB.cnfLstPB.result = EOF;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_USERS(PackPtr thePack)
{
	gPB.usrLstPB.result = PD_RPL_USERS(thePack, gPB.usrLstPB.uid, gPB.usrLstPB.name);
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_ENDOFUSERS(PackPtr thePack)
{
	gPB.usrLstPB.result = EOF;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_MEMBERS(PackPtr thePack)
{
	gPB.memLstPB.result = PD_RPL_MEMBERS(thePack, gPB.memLstPB.uid, gPB.memLstPB.name);
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_ENDOFMEMBERS(PackPtr thePack)
{
	gPB.memLstPB.result = EOF;
}

/*————————————————————————————————————————————————————————————*/

void RCV_RPL_INVITING(PackPtr thePack)
{
	gPB.invitePB.result = noErr;
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_INVITE(PackPtr thePack)
{
	extern OSErr NotifyUser(long, Str32, Str32);
	
	Str32 title, name;
	long cid;
	OSErr error;
	
	error = PD_CMD_INVITE(thePack, &cid, title, name);
	if (error != noErr)
		return;
	(void) NotifyUser(cid, title, name);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_NOTICE(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void AddToTrack(ConstStr255Param);
	extern void AddToConf(WindowRef, short, const void *, long);
	
	char cstr[64];
	Str63 pstr;
	Str32 name;
	WindowRef window;
	long cid, length;
	short code;
	OSErr error;
	
	error = PD_CMD_NOTICE(thePack, &cid, &code, name);
	if (error != noErr)
		return;
	if (cid == 0) {
		PLstrcpy(pstr, name);
		switch (code) {
		case 0:
			PLstrcat(pstr, "\p has joined the ChitChat server.");
			break;
		case 1:
			PLstrcat(pstr, "\p has left the ChitChat server.");
			break;
		}
		AddToTrack(pstr);
	} else {
		strcpy(cstr, "• ");
		strcat(cstr, p2cstr(name));
		switch (code) {
		case 0:
			strcat(cstr, " has joined the conference.\n");
			break;
		case 1:
			strcat(cstr, " has left the conference.\n");
			break;
		}
		length = strlen(cstr);
		if (gClickState == 0 && gClick != nil) {
			HLock((Handle) gClick);
			(void) SndPlay(nil, gClick, false);
			HUnlock((Handle) gClick);
		}
		window = FrontWindow();
		while (window != nil) {
			switch (GetWindowKind(window)) {
			case confKind:
				if (GetConfWindowCID(window) == cid)
					AddToConf(window, 2, cstr, length);
				break;
			}
			window = GetNextWindow(window);
		}
	}
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_MESSAGE(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void AddToConf(WindowRef, short, const void *, long);
	
	char string[292];
	Str255 text;
	Str32 name;
	WindowRef window;
	long cid, uid, length;
	OSErr error;
	
	error = PD_CMD_MESSAGE(thePack, &cid, &uid, name, text);
	if (error != noErr)
		return;
	strcpy(string, p2cstr(name));
	strcat(string, ": ");
	strcat(string, p2cstr(text));
	strcat(string, "\n");
	length = strlen(string);
	if (gClickState == 0 && gClick != nil) {
		HLock((Handle) gClick);
		(void) SndPlay(nil, gClick, false);
		HUnlock((Handle) gClick);
	}
	window = FrontWindow();
	while (window != nil) {
		switch (GetWindowKind(window)) {
		case confKind:
			if (GetConfWindowCID(window) == cid)
				AddToConf(window, uid == gUID ? 1 : 0, string, length);
			break;
		}
		window = GetNextWindow(window);
	}
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_STARTGRAF(PackPtr thePack)
{
	extern long GetGrafWindowCID(WindowRef);
	extern long GetConfWindowCID(WindowRef);
	extern WindowRef NewGraf(long, WindowRef);
	extern void GrafWinOpened(WindowRef, WindowRef);
	extern void NewPict(WindowRef, long, Rect *, Str32);
	
	Str32 name;
	WindowRef window, textWin;
	GrafPtr savePort;
	Rect picFrame;
	long cid, picSize;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_STARTGRAF(thePack, &cid, &picSize, &picFrame, name);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == grafKind && GetGrafWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found) {
		textWin = FrontWindow(), found = false;
		while (textWin != nil) {
			if (GetWindowKind(textWin) == confKind && GetConfWindowCID(textWin) == cid) {
				found = true;
				break;
			}
			textWin = GetNextWindow(textWin);
		}
		if (!found)
			return;
		window = NewGraf(cid, textWin);
		if (window == nil)
			return;
		GrafWinOpened(textWin, window);
	}
	GetPort(&savePort);
	SetPortWindowPort(window);
	NewPict(window, picSize, &picFrame, name);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_GRAFDATA(PackPtr thePack)
{
	extern long GetGrafWindowCID(WindowRef);
	extern void PictData(WindowRef, Ptr, long);
	
	WindowRef window;
	GrafPtr savePort;
	Byte data[500];
	long cid, length;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_GRAFDATA(thePack, &cid, (Ptr) data, &length);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == grafKind && GetGrafWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	PictData(window, (Ptr) data, length);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_ACKGRAF(PackPtr thePack)
{
	extern long GetGrafWindowCID(WindowRef);
	extern void SendGrafData(WindowRef);
	
	WindowRef window;
	GrafPtr savePort;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ACKGRAF(thePack, &cid);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == grafKind && GetGrafWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	SendGrafData(window);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_ENDGRAF(PackPtr thePack)
{
	extern long GetGrafWindowCID(WindowRef);
	extern void EndPict(WindowRef);
	
	WindowRef window;
	GrafPtr savePort;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ENDGRAF(thePack, &cid);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == grafKind && GetGrafWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	EndPict(window);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_STARTVOICE(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void StartVoice(WindowRef, UnsignedFixed, OSType, Str32);
	
	Str32 name;
	WindowRef window;
	GrafPtr savePort;
	UnsignedFixed sampRate;
	OSType compType;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_STARTVOICE(thePack, &cid, &sampRate, &compType, name);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == confKind && GetConfWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	StartVoice(window, sampRate, compType, name);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_VOICEDATA(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void VoiceData(WindowRef, Ptr, unsigned long);
	
	WindowRef window;
	GrafPtr savePort;
	Byte data[504];
	unsigned long length;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_VOICEDATA(thePack, &cid, (Ptr) data, &length);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == confKind && GetConfWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	VoiceData(window, (Ptr) data, length);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_ACKVOICE(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void SendVoiceData(WindowRef);
	
	WindowRef window;
	GrafPtr savePort;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ACKVOICE(thePack, &cid);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == confKind && GetConfWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	SendVoiceData(window);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void RCV_CMD_ENDVOICE(PackPtr thePack)
{
	extern long GetConfWindowCID(WindowRef);
	extern void EndVoice(WindowRef);
	
	WindowRef window;
	GrafPtr savePort;
	long cid;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ENDVOICE(thePack, &cid);
	if (error != noErr)
		return;
	window = FrontWindow(), found = false;
	while (window != nil) {
		if (GetWindowKind(window) == confKind && GetConfWindowCID(window) == cid) {
			found = true;
			break;
		}
		window = GetNextWindow(window);
	}
	if (!found)
		return;
	GetPort(&savePort);
	SetPortWindowPort(window);
	EndVoice(window);
	SetPort(savePort);
}

/*————————————————————————————————————————————————————————————*/

void MCPInit(void)
{
	if (gClick == nil)
		gClick = (SndListHandle) GetResource('snd ', 129);
	gClickState = 0;
}

/*————————————————————————————————————————————————————————————*/

Boolean MCPGetClickState(void)
{
	return gClickState ? false : true;
}

/*————————————————————————————————————————————————————————————*/

void MCPSetClickState(Boolean clickState)
{
	if (clickState)
		gClickState--;
	else
		gClickState++;
}

/*————————————————————————————————————————————————————————————*/

Boolean MCPOnLine(void)
{
	return (gFlags & 0x2000) != 0;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPOpen(EntityPtr location)
{
	IPCParamBlock paramBlock;
	IPCParamBlockPtr pb;
	OSErr error;
	
	pb = &paramBlock;
	pb->openPB.localSocket = 0;
	if ((error = IPCOpen(pb)) == noErr) {
		gRefNum = pb->openPB.sessRefNum;
		gFlags |= 0x8000;
		pb = &paramBlock;
		pb->startPB.sessRefNum = gRefNum;
		pb->startPB.locationName = location;
		if ((error = IPCStart(pb, false)) == noErr) {
			gFlags |= 0x4000;
			gPB.header.result = noErr;
			pb = &gParamBlock[0];
			pb->readPB.ioCompletion = nil;
			pb->readPB.sessRefNum = gRefNum;
			pb->readPB.bufferLength = sizeof(PackRec);
			pb->readPB.bufferPtr = (Ptr) &gPack[0];
			(void) IPCRead(pb, true);
			pb = &gParamBlock[1];
			pb->writePB.ioResult = noErr;
			return noErr;
		}
		pb = &paramBlock;
		pb->closePB.sessRefNum = gRefNum;
		pb->closePB.abort = false;
		(void) IPCClose(pb);
		gFlags &= ~0x8000;
	}
	return error;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPClose(void)
{
	extern void CloseTrack(WindowRef);
	extern long CloseConf(WindowRef);
	extern long CloseGraf(WindowRef);
	OSErr MCPLeave(long);
	void SND_CMD_QUIT(void);
	
	IPCParamBlock paramBlock;
	IPCParamBlockPtr pb;
	WindowRef window, nextWindow;
	long cid;
	
	window = FrontWindow();
	while (window != nil) {
		nextWindow = GetNextWindow(window);
		switch (GetWindowKind(window)) {
		case tracKind:
			CloseTrack(window);
			break;
		case confKind:
			cid = CloseConf(window);
			(void) MCPLeave(cid);
			break;
		case grafKind:
			cid = CloseGraf(window);
			(void) MCPLeave(cid);
			break;
		}
		window = nextWindow;
	}
	if (gFlags & 0x2000) {
		SND_CMD_QUIT();
		gFlags &= ~0x2000;
	}
	if (gFlags & 0x4000) {
		pb = &paramBlock;
		pb->endPB.sessRefNum = gRefNum;
		pb->endPB.abort = false;
		(void) IPCEnd(pb, false);
		gFlags &= ~0x4000;
	}
	if (gFlags & 0x8000) {
		pb = &paramBlock;
		pb->closePB.sessRefNum = gRefNum;
		pb->closePB.abort = false;
		(void) IPCClose(pb);
		gFlags &= ~0x8000;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

void MCPIdle(void)
{
	extern void IdleTrack(void);
	extern OSErr NotifyDisc(void);
	
	IPCParamBlockPtr pb;
	short type;
	OSErr error;
	
	IdleTrack();
	if (!(gFlags & 0x4000))
		return;
	while (true) {
		error = gParamBlock[0].readPB.ioResult;
		if (error > 0)
			return;
		if (error == errState) {
			gFlags &= ~0x6000;
			(void) MCPClose();
			(void) NotifyDisc();
			gPB.header.result = errState;
			return;
		}
		if (error == noErr) {
			type = gPack[0].type;
			if (type >= 100 && type < 300) {
				switch (type) {
				case CMD_INVITE:
					RCV_CMD_INVITE(&gPack[0]);
					break;
				case CMD_NOTICE:
					RCV_CMD_NOTICE(&gPack[0]);
					break;
				case CMD_MESSAGE:
					RCV_CMD_MESSAGE(&gPack[0]);
					break;
				case CMD_STARTGRAF:
					RCV_CMD_STARTGRAF(&gPack[0]);
					break;
				case CMD_GRAFDATA:
					RCV_CMD_GRAFDATA(&gPack[0]);
					break;
				case CMD_ACKGRAF:
					RCV_CMD_ACKGRAF(&gPack[0]);
					break;
				case CMD_ENDGRAF:
					RCV_CMD_ENDGRAF(&gPack[0]);
					break;
				case CMD_STARTVOICE:
					RCV_CMD_STARTVOICE(&gPack[0]);
					break;
				case CMD_VOICEDATA:
					RCV_CMD_VOICEDATA(&gPack[0]);
					break;
				case CMD_ACKVOICE:
					RCV_CMD_ACKVOICE(&gPack[0]);
					break;
				case CMD_ENDVOICE:
					RCV_CMD_ENDVOICE(&gPack[0]);
					break;
				}
			} else if (type >= 300 && type < 500) {
				if (gPB.header.result == 1) {
					switch (gPB.header.code) {
					case kLogin:
						switch (type) {
						case RPL_YOUREIN:
							RCV_RPL_YOUREIN(&gPack[0]);
							break;
						}
						break;
					case kChPasswd:
						switch (type) {
						case RPL_CHANGED:
							RCV_RPL_CHANGED(&gPack[0]);
							break;
						}
						break;
					case kNewConf:
						switch (type) {
						case RPL_CREATED:
							RCV_RPL_CREATED(&gPack[0]);
							break;
						}
						break;
					case kGetConfList:
					case kGetNextConf:
						switch (type) {
						case RPL_LIST:
							RCV_RPL_LIST(&gPack[0]);
							break;
						case RPL_LISTEND:
							RCV_RPL_LISTEND(&gPack[0]);
							break;
						}
						break;
					case kGetUserList:
					case kGetNextUser:
						switch (type) {
						case RPL_USERS:
							RCV_RPL_USERS(&gPack[0]);
							break;
						case RPL_ENDOFUSERS:
							RCV_RPL_ENDOFUSERS(&gPack[0]);
							break;
						}
						break;
					case kGetMemberList:
					case kGetNextMember:
						switch (type) {
						case RPL_MEMBERS:
							RCV_RPL_MEMBERS(&gPack[0]);
							break;
						case RPL_ENDOFMEMBERS:
							RCV_RPL_ENDOFMEMBERS(&gPack[0]);
							break;
						}
						break;
					case kInvite:
						switch (type) {
						case RPL_INVITING:
							RCV_RPL_INVITING(&gPack[0]);
							break;
						}
						break;
					case kJoin:
					case kLeave:
					case kMessage:
					case kStartGraf:
					case kEndGraf:
					case kStartVoice:
					case kEndVoice:
						switch (type) {
						case RPL_NONE:
							RCV_RPL_NONE(&gPack[0]);
							break;
						}
						break;
					}
				}
			} else if (type >= 500 && type < 700) {
				if (gPB.header.result == 1) {
					gPB.header.result = type;
				}
			}
		}
		pb = &gParamBlock[0];
		pb->readPB.ioCompletion = nil;
		pb->readPB.sessRefNum = gRefNum;
		pb->readPB.bufferLength = sizeof(PackRec);
		pb->readPB.bufferPtr = (Ptr) &gPack[0];
		(void) IPCRead(pb, true);
	}
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_LOGIN(unsigned short version, Str32 name, Str8 password)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_LOGIN(&gPack[1], version, name, password);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_QUIT(void)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_QUIT(&gPack[1]);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_CHPASSWD(Str8 oPassword, Str8 nPassword)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_CHPASSWD(&gPack[1], oPassword, nPassword);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_NEWCONF(Boolean private, Str32 title)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_NEWCONF(&gPack[1], private, title);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_LIST(void)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_LIST(&gPack[1]);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_USERS(long id)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_USERS(&gPack[1], id);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_MEMBERS(long cid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_MEMBERS(&gPack[1], cid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_INVITE(long cid, long uid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_INVITE(&gPack[1], cid, uid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_JOIN(long cid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_JOIN(&gPack[1], cid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_LEAVE(long cid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_LEAVE(&gPack[1], cid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_MESSAGE(long id, Str255 text)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_MESSAGE(&gPack[1], id, text);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_STARTGRAF(long cid, long picSize, Rect *picFrame)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_STARTGRAF(&gPack[1], cid, picSize, picFrame);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_GRAFDATA(long cid, Ptr data, long count)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_GRAFDATA(&gPack[1], cid, data, count);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_ENDGRAF(long cid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_ENDGRAF(&gPack[1], cid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_STARTVOICE(long cid, UnsignedFixed sampRate, OSType compType)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_STARTVOICE(&gPack[1], cid, sampRate, compType);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_VOICEDATA(long cid, Ptr data, unsigned long count)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_VOICEDATA(&gPack[1], cid, data, count);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_ENDVOICE(long cid)
{
	IPCParamBlockPtr pb;
	unsigned short length;
	
	while (gParamBlock[1].writePB.ioResult > 0)
		YieldToSystem();
	length = PA_CMD_ENDVOICE(&gPack[1], cid);
	pb = &gParamBlock[1];
	pb->writePB.ioCompletion = nil;
	pb->writePB.sessRefNum = gRefNum;
	pb->writePB.bufferLength = length;
	pb->writePB.bufferPtr = (Ptr) &gPack[1];
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPLogin(Str32 name, Str8 password)
{
	if (!(gFlags & 0x4000))
		return errState;
	gPB.loginPB.code = kLogin;
	gPB.loginPB.result = 1;
	gPB.loginPB.uid = &gUID;
	gPB.loginPB.name = gUserName;
	SND_CMD_LOGIN(8, name, password);
	while (gPB.loginPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	if (gPB.loginPB.result == noErr)
		gFlags |= 0x2000;
	return gPB.loginPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPChangePass(Str8 oPassword, Str8 nPassword)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.passPB.code = kChPasswd;
	gPB.passPB.result = 1;
	SND_CMD_CHPASSWD(oPassword, nPassword);
	while (gPB.passPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	if (gPB.passPB.result == noErr)
		PLstrcpy(gPassword, nPassword);
	return gPB.passPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPNewConf(Boolean private, Str32 title, long *cid)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.newCnfPB.code = kNewConf;
	gPB.newCnfPB.result = 1;
	gPB.newCnfPB.cid = cid;
	SND_CMD_NEWCONF(private, title);
	while (gPB.newCnfPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.newCnfPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetConfList(long *cid, Str32 title)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.cnfLstPB.code = kGetConfList;
	gPB.cnfLstPB.result = 1;
	gPB.cnfLstPB.cid = cid;
	gPB.cnfLstPB.title = title;
	SND_CMD_LIST();
	while (gPB.cnfLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.cnfLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetNextConf(long *cid, Str32 title)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.cnfLstPB.code = kGetNextConf;
	gPB.cnfLstPB.result = 1;
	gPB.cnfLstPB.cid = cid;
	gPB.cnfLstPB.title = title;
	while (gPB.cnfLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.cnfLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetUserList(long cid, long *uid, Str32 name)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.usrLstPB.code = kGetUserList;
	gPB.usrLstPB.result = 1;
	gPB.usrLstPB.uid = uid;
	gPB.usrLstPB.name = name;
	SND_CMD_USERS(cid);
	while (gPB.usrLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.usrLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetNextUser(long *uid, Str32 name)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.usrLstPB.code = kGetNextUser;
	gPB.usrLstPB.result = 1;
	gPB.usrLstPB.uid = uid;
	gPB.usrLstPB.name = name;
	while (gPB.usrLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.usrLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetMemberList(long cid, long *uid, Str32 name)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.memLstPB.code = kGetMemberList;
	gPB.memLstPB.result = 1;
	gPB.memLstPB.uid = uid;
	gPB.memLstPB.name = name;
	SND_CMD_MEMBERS(cid);
	while (gPB.memLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.memLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGetNextMember(long *uid, Str32 name)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.memLstPB.code = kGetNextMember;
	gPB.memLstPB.result = 1;
	gPB.memLstPB.uid = uid;
	gPB.memLstPB.name = name;
	while (gPB.memLstPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.memLstPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPInvite(long cid, long uid)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.invitePB.code = kInvite;
	gPB.invitePB.result = 1;
	SND_CMD_INVITE(cid, uid);
	while (gPB.invitePB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.invitePB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPJoin(long cid)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.joinPB.code = kJoin;
	gPB.joinPB.result = 1;
	SND_CMD_JOIN(cid);
	while (gPB.joinPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.joinPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPLeave(long cid)
{
	if (!(gFlags & 0x2000))
		return errState;
	if (cid == 0)
		return noErr;
	gPB.leavePB.code = kLeave;
	gPB.leavePB.result = 1;
	SND_CMD_LEAVE(cid);
	while (gPB.leavePB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.leavePB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPMessage(long cid, Str255 text)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.mesgPB.code = kMessage;
	gPB.mesgPB.result = 1;
	SND_CMD_MESSAGE(cid, text);
	while (gPB.mesgPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.mesgPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPStartGraf(long cid, long picSize, Rect *picFrame)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.strGrfPB.code = kStartGraf;
	gPB.strGrfPB.result = 1;
	SND_CMD_STARTGRAF(cid, picSize, picFrame);
	while (gPB.strGrfPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.strGrfPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPGrafData(long cid, Ptr data, long length)
{
	if (!(gFlags & 0x2000))
		return errState;
	SND_CMD_GRAFDATA(cid, data, length);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPEndGraf(long cid)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.endGrfPB.code = kEndGraf;
	gPB.endGrfPB.result = 1;
	SND_CMD_ENDGRAF(cid);
	while (gPB.endGrfPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.endGrfPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPStartVoice(long cid, UnsignedFixed sampRate, OSType compType)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.strVoiPB.code = kStartVoice;
	gPB.strVoiPB.result = 1;
	SND_CMD_STARTVOICE(cid, sampRate, compType);
	while (gPB.strVoiPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.strVoiPB.result;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPVoiceData(long cid, Ptr data, unsigned long length)
{
	if (!(gFlags & 0x2000))
		return errState;
	SND_CMD_VOICEDATA(cid, data, length);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPEndVoice(long cid)
{
	if (!(gFlags & 0x2000))
		return errState;
	gPB.endVoiPB.code = kEndVoice;
	gPB.endVoiPB.result = 1;
	SND_CMD_ENDVOICE(cid);
	while (gPB.endVoiPB.result == 1) {
		YieldToSystem();
		MCPIdle();
	}
	return gPB.endVoiPB.result;
}

/*————————————————————————————————————————————————————————————*/
