/* SCom.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1

/*————————————————————————————————————————————————————————————*/

#include <StdDef.h>
#include <Types.h>
#include <Errors.h>
#include <PLStringFuncs.h>
#include <Memory.h>
#include <Files.h>
#include <Devices.h>
#include <AppleTalk.h>
#include <TextUtils.h>
#include <Sound.h>

#include "IPC.h"
#include "Protocol.h"
#include "Semaphore.h"
#include "Misc.h"
#include "Server.h"

/*————————————————————————————————————————————————————————————*/

#define MAXUSER 30		/* 100, 30, 10 */
#define MAXSESS 15		/*  50, 15,  5 */
#define MAXCONF 30		/* 100, 30, 20 */

#define MAXTASK MAXSESS*8
#define MAXMESG MAXSESS*8
#define MAXMHDR MAXSESS*MAXSESS*4

/*————————————————————————————————————————————————————————————*/

enum {

sClosed		= 0,
sOpening	= 1,
sOpen		= 2,
sClosing	= 3

};

/*————————————————————————————————————————————————————————————*/

typedef struct QMElem {
	struct QMElem *link;
} QMElem, *QMElemPtr;

typedef struct QMHdr {
	QMElemPtr head;
	QMElemPtr tail;
} QMHdr, *QMHdrPtr;

/*————————————————————————————————————————————————————————————*/

typedef struct {
	short type;
	Byte data[510];
} PackRec, *PackPtr;

typedef struct {
	unsigned short count;
	unsigned short length;
	long from;
	PackRec pack;
} MesgRec, *MesgPtr;

typedef struct {
	Ptr link;
	MesgPtr data;
} MHdrRec, *MHdrPtr;

typedef struct {
	short flags;
	long id;
	unsigned long time;
	Str32 name;
	Str8 password;
} UserRec, *UserPtr;

typedef struct {
	short flags;
	short state;
	short refNum;
	short uix;
	UserPtr user;
	QMHdr messages;
	IPCParamBlock paramBlock[2];
} SessRec, *SessPtr;

typedef struct {
	short flags;
	short count;
	long id;
	long graf;
	long voice;
	Byte users[MAXUSER];
	Str32 title;
} ConfRec, *ConfPtr;

typedef struct {
	QElemPtr link;
	ProcPtr addr;
	SessPtr sess;
	MesgPtr mesg;
} TaskRec, *TaskPtr;

/*————————————————————————————————————————————————————————————*/

static QHdr gTaskQueue;
static QMHdr gFreeTasks;
static QMHdr gFreeMesgs;
static QMHdr gFreeMHdrs;
static TaskPtr gTasks;
static MesgPtr gMesgs;
static MHdrPtr gMHdrs;
static UserPtr gUsers;
static ConfPtr gConfs;
static SessPtr gSessions;
static Ptr gNTE;
static unsigned char gSocket = 0;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

/*————————————————————————————————————————————————————————————*/

void QMInitQueue(QMHdrPtr q)
{
	q->head = nil;
	q->tail = nil;
}

/*————————————————————————————————————————————————————————————*/

void QMEnqueue(QMElemPtr e, QMHdrPtr q)
{
	QMElemPtr p;
	short saveSR;
	
	e->link = nil;
	saveSR = DisableInt();
	if ((p = q->tail) == nil)
		q->head = e;
	else
		p->link = e;
	q->tail = e;
	RestoreInt(saveSR);
}

/*————————————————————————————————————————————————————————————*/

QMElemPtr QMDequeue(QMHdrPtr q)
{
	QMElemPtr e, p;
	short saveSR;
	
	saveSR = DisableInt();
	if ((e = q->head) != nil) {
		if ((p = e->link) == nil)
			q->tail = nil;
		q->head = p;
	}
	RestoreInt(saveSR);
	return e;
}

/*————————————————————————————————————————————————————————————*/

OSErr InitDefer(void)
{
	TaskPtr tasks, t;
	short i;
	
	tasks = (TaskPtr) NewPtr(sizeof(TaskRec)*MAXTASK);
	if (tasks == nil)
		return memFullErr;
	QMInitQueue(&gFreeTasks);
	t = tasks;
	for (i = 0; i < MAXTASK; i++) {
		QMEnqueue((QMElemPtr) t, &gFreeTasks);
		t++;
	}
	gTasks = tasks;
	SMInit(&gTaskQueue);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

TaskPtr NewTask(void)
{
	TaskPtr t;
	
	if ((t = (TaskPtr) QMDequeue(&gFreeTasks)) == nil) {
		SysError(1000);
		Debugger();
	}
	return t;
}

/*————————————————————————————————————————————————————————————*/

void DisposTask(TaskPtr task)
{
	QMEnqueue((QMElemPtr) task, &gFreeTasks);
}

/*————————————————————————————————————————————————————————————*/

void DeferFn(TaskPtr task)
{
	SMInstall((SMTaskPtr) task, &gTaskQueue);
}

/*————————————————————————————————————————————————————————————*/

OSErr InitMesgs(void)
{
	MesgPtr mesgs, m;
	short i;
	
	mesgs = (MesgPtr) NewPtr(sizeof(MesgRec)*MAXMESG);
	if (mesgs == nil)
		return memFullErr;
	QMInitQueue(&gFreeMesgs);
	m = mesgs;
	for (i = 0; i < MAXMESG; i++) {
		QMEnqueue((QMElemPtr) m, &gFreeMesgs);
		m++;
	}
	gMesgs = mesgs;
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

MesgPtr NewMesg(void)
{
	MesgPtr m;
	
	if ((m = (MesgPtr) QMDequeue(&gFreeMesgs)) == nil) {
		SysError(1001);
		Debugger();
	}
	m->count = 1;
	return m;
}

/*————————————————————————————————————————————————————————————*/

void DisposMesg(MesgPtr m)
{
	if (--m->count == 0)
		QMEnqueue((QMElemPtr) m, &gFreeMesgs);
}

/*————————————————————————————————————————————————————————————*/

MesgPtr DupMesg(MesgPtr m)
{
	m->count++;
	return m;
}

/*————————————————————————————————————————————————————————————*/

OSErr InitMHdrs(void)
{
	MHdrPtr mhdrs, mh;
	short i;
	
	mhdrs = (MHdrPtr) NewPtr(sizeof(MHdrRec)*MAXMHDR);
	if (mhdrs == nil)
		return memFullErr;
	QMInitQueue(&gFreeMHdrs);
	mh = mhdrs;
	for (i = 0; i < MAXMHDR; i++) {
		QMEnqueue((QMElemPtr) mh, &gFreeMHdrs);
		mh++;
	}
	gMHdrs = mhdrs;
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

void PostMesg(MesgPtr m, SessPtr s)
{
	MHdrPtr mh;
	
	if ((mh = (MHdrPtr) QMDequeue(&gFreeMHdrs)) == nil) {
		SysError(1002);
		Debugger();
	}
	mh->data = m;
	QMEnqueue((QMElemPtr) mh, &s->messages);
}

/*————————————————————————————————————————————————————————————*/

MesgPtr GetMesg(SessPtr s)
{
	MHdrPtr mh;
	MesgPtr m;
	
	if ((mh = (MHdrPtr) QMDequeue(&s->messages)) != nil) {
		m = mh->data;
		QMEnqueue((QMElemPtr) mh, &gFreeMHdrs);
	} else
		m = nil;
	return m;
}

/*————————————————————————————————————————————————————————————*/

long NewUID(void)
{
	static long seed = 0;
	long id;
	short saveSR;
	
	saveSR = DisableInt();
	do
		id = seed++;
	while (!id);
	RestoreInt(saveSR);
	return id;
}

/*————————————————————————————————————————————————————————————*/

OSErr OpenUsers(void)
{
	UserPtr users, u;
	unsigned long theTime;
	short i;
	
	users = (UserPtr) NewPtr(sizeof(UserRec)*MAXUSER);
	if (users == nil)
		return memFullErr;
	u = users;
	for (i = 0; i < MAXUSER; i++) {
		u->flags = 0x0000;
		u++;
	}
	GetDateTime(&theTime);
	u = users;
	
	u->flags = 0x8000;
	u->id = NewUID();
	u->time = theTime;
	PLstrcpy(u->name, "\pYves Lempereur");
	PLstrcpy(u->password, "\pyves");
	
	gUsers = users;
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr CloseUsers(void)
{
	UserPtr u;
	short i;
	
	u = gUsers;
	for (i = 0; i < MAXUSER; i++) {
		u->flags = 0x0000;
		u++;
	}
	DisposePtr((Ptr) gUsers);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

long NewCID(void)
{
	static long seed = 0;
	long cid;
	short saveSR;
	
	saveSR = DisableInt();
	do
		cid = ++seed;
	while (!cid);
	RestoreInt(saveSR);
	return cid;
}

/*————————————————————————————————————————————————————————————*/

OSErr OpenConfs(void)
{
	ConfPtr confs, c;
	short i;
	
	confs = (ConfPtr) NewPtr(sizeof(ConfRec)*MAXCONF);
	if (confs == nil)
		return memFullErr;
	c = confs;
	for (i = 0; i < MAXCONF; i++) {
		c->flags = 0x0000;
		c++;
	}
	gConfs = confs;
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr CloseConfs(void)
{
	ConfPtr c;
	short i;
	
	c = gConfs;
	for (i = 0; i < MAXCONF; i++) {
		c->flags = 0x0000;
		c++;
	}
	DisposePtr((Ptr) gConfs);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr OpenSessions(void)
{
	pascal void Read_0(IPCParamBlockPtr);
	
	IPCParamBlock paramBlock;
	IPCParamBlockPtr pb;
	SessPtr sessions, s;
	OSErr error;
	short i;
	
	sessions = (SessPtr) NewPtr(sizeof(SessRec)*MAXSESS);
	if (sessions == nil)
		return memFullErr;
	error = noErr;
	s = sessions;
	for (i = 0; i < MAXSESS; i++) {
		s->flags = 0x0000;
		if (error == noErr) {
			pb = &paramBlock;
			pb->openPB.localSocket = gSocket;
			error = IPCOpen(pb);
			if (error == noErr) {
				gSocket = pb->openPB.localSocket;
				s->refNum = pb->openPB.sessRefNum;
				s->flags |= 0x8001;
				pb = &s->paramBlock[0];
				pb->listenPB.ioCompletion = (ProcPtr) Read_0;
				pb->listenPB.sessRefNum = s->refNum;
				error = IPCListen(pb, true);
				if (error == noErr) {
					QMInitQueue(&s->messages);
					s->state = sClosed;
					s->flags |= 0x4000;
				}
			}
		}
		s++;
	}
	if (error == noErr) {
		gSessions = sessions;
		return noErr;
	}
	s = sessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x4000) {
			pb = &paramBlock;
			pb->endPB.sessRefNum = s->refNum;
			pb->endPB.abort = true;
			(void) IPCEnd(pb, false);
		}
		if (s->flags & 0x8000) {
			pb = &paramBlock;
			pb->closePB.sessRefNum = s->refNum;
			pb->closePB.abort = true;
			(void) IPCClose(pb);
		}
		s++;
	}
	DisposePtr((Ptr) sessions);
	return error;
}

/*————————————————————————————————————————————————————————————*/

OSErr CloseSessions(void)
{
	IPCParamBlock paramBlock;
	IPCParamBlockPtr pb;
	SessPtr s;
	short i;
	
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x4000) {
			pb = &paramBlock;
			pb->endPB.sessRefNum = s->refNum;
			pb->endPB.abort = false;
			(void) IPCEnd(pb, false);
			s->flags &= ~0x4000;
		}
		if (s->flags & 0x8000) {
			pb = &paramBlock;
			pb->closePB.sessRefNum = s->refNum;
			pb->closePB.abort = false;
			(void) IPCClose(pb);
			s->flags &= ~0x8000;
		}
		s++;
	}
	DisposePtr((Ptr) gSessions);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

pascal void Listen_0(IPCParamBlockPtr paramBlock)
{
	pascal void Read_0(IPCParamBlockPtr);
	
	IPCParamBlockPtr pb;
	SessPtr s;
	OSErr error;
	
	s = (SessPtr) ((Ptr) paramBlock - offsetof(SessRec, paramBlock[1]));
	error = paramBlock->endPB.ioResult;
	s->flags &= ~0x0002;
	if (error != noErr && error != errAborted)
		return;
	s->state = sClosed;
	s->flags |= 0x0001;
	pb = &s->paramBlock[0];
	pb->listenPB.ioCompletion = (ProcPtr) Read_0;
	pb->listenPB.sessRefNum = s->refNum;
	(void) IPCListen(pb, true);
}

/*————————————————————————————————————————————————————————————*/

pascal void Read_0(IPCParamBlockPtr paramBlock)
{
	void RecvMesg(SessPtr);
	
	SessPtr s;
	OSErr error;
	
	s = (SessPtr) ((Ptr) paramBlock - offsetof(SessRec, paramBlock[0]));
	error = paramBlock->listenPB.ioResult;
	s->flags &= ~0x0001;
	if (error != noErr)
		return;
	s->state = sOpening;
	RecvMesg(s);
}

/*————————————————————————————————————————————————————————————*/

void RecvMesg(SessPtr s)
{
	pascal void Read_1(IPCParamBlockPtr);
	
	IPCParamBlockPtr pb;
	MesgPtr m;
	
	m = NewMesg();
	s->flags |= 0x0001;
	pb = &s->paramBlock[0];
	pb->readPB.ioCompletion = (ProcPtr) Read_1;
	pb->readPB.sessRefNum = s->refNum;
	pb->readPB.bufferLength = sizeof(PackRec);
	pb->readPB.bufferPtr = (Ptr) &m->pack;
	(void) IPCRead(pb, true);
}

/*————————————————————————————————————————————————————————————*/

pascal void Read_1(IPCParamBlockPtr paramBlock)
{
	pascal void XComm(TaskPtr);
	pascal void XDisc(TaskPtr);
	
	TaskPtr t;
	SessPtr s;
	MesgPtr m;
	OSErr error;
	
	s = (SessPtr) ((Ptr) paramBlock - offsetof(SessRec, paramBlock[0]));
	m = (MesgPtr) ((Ptr) paramBlock->readPB.bufferPtr - offsetof(MesgRec, pack));
	m->length = paramBlock->readPB.actualLength;
	error = paramBlock->readPB.ioResult;
	s->flags &= ~0x0001;
	if (s->state == sOpen || s->state == sOpening) {
		if (error == noErr && m->length >= 2) {
			t = NewTask();
			t->addr = (ProcPtr) XComm;
			t->sess = s;
			t->mesg = m;
			DeferFn(t);
			if (s->state == sOpen || s->state == sOpening)
				RecvMesg(s);
		} else {
			DisposMesg(m);
			t = NewTask();
			t->addr = (ProcPtr) XDisc;
			t->sess = s;
			DeferFn(t);
		}
	} else {
		DisposMesg(m);
	}
}

/*————————————————————————————————————————————————————————————*/

pascal void XDisc(TaskPtr t)
{
	void Disconnect(SessPtr);
	
	SessPtr s;
	
	s = t->sess;
	DisposTask(t);
	Disconnect(s);
}

/*————————————————————————————————————————————————————————————*/

pascal void XComm(TaskPtr t)
{
	void Command(MesgPtr, SessPtr);
	
	SessPtr s;
	MesgPtr m;
	
	s = t->sess;
	m = t->mesg;
	DisposTask(t);
	Command(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SendMesg(MesgPtr m, SessPtr s)
{
	pascal void Send_0(IPCParamBlockPtr);
	
	IPCParamBlockPtr pb;
	short saveSR;
	
	saveSR = DisableInt();
	if (!(s->state == sOpen || s->state == sOpening)) {
		DisposMesg(m);
		RestoreInt(saveSR);
		return;
	}
	if (s->flags & 0x0002) {
		PostMesg(m, s);
		RestoreInt(saveSR);
		return;
	}
	s->flags |= 0x0002;
	RestoreInt(saveSR);
	pb = &s->paramBlock[1];
	pb->writePB.ioCompletion = (ProcPtr) Send_0;
	pb->writePB.sessRefNum = s->refNum;
	pb->writePB.bufferLength = m->length;
	pb->writePB.bufferPtr = (Ptr) &m->pack;
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

pascal void Send_0(IPCParamBlockPtr paramBlock)
{
	pascal void XAck(TaskPtr);
	
	IPCParamBlockPtr pb;
	TaskPtr t;
	SessPtr s;
	MesgPtr m;
	short saveSR;
	
	s = (SessPtr) ((Ptr) paramBlock - offsetof(SessRec, paramBlock[1]));
	m = (MesgPtr) ((Ptr) paramBlock->writePB.bufferPtr - offsetof(MesgRec, pack));
	if (m->pack.type == CMD_GRAFDATA || m->pack.type == CMD_VOICEDATA) {
		saveSR = DisableInt();
		if (m->count == 1) {
			RestoreInt(saveSR);
			t = NewTask();
			t->addr = (ProcPtr) XAck;
			t->mesg = m;
			DeferFn(t);
		} else {
			DisposMesg(m);
			RestoreInt(saveSR);
		}
	} else
		DisposMesg(m);
	saveSR = DisableInt();
	if ((m = GetMesg(s)) == nil) {
		if (s->state == sClosing) {
			RestoreInt(saveSR);
			pb = &s->paramBlock[1];
			pb->endPB.ioCompletion = (ProcPtr) Listen_0;
			pb->endPB.sessRefNum = s->refNum;
			pb->endPB.abort = false;
			(void) IPCEnd(pb, true);
			return;
		}
		s->flags &= ~0x0002;
		RestoreInt(saveSR);
		return;
	}
	RestoreInt(saveSR);
	pb = &s->paramBlock[1];
	pb->writePB.ioCompletion = (ProcPtr) Send_0;
	pb->writePB.sessRefNum = s->refNum;
	pb->writePB.bufferLength = m->length;
	pb->writePB.bufferPtr = (Ptr) &m->pack;
	pb->writePB.more = false;
	(void) IPCWrite(pb, true);
}

/*————————————————————————————————————————————————————————————*/

pascal void XAck(TaskPtr t)
{
	void AckGraf(MesgPtr);
	void AckVoice(MesgPtr);
	
	MesgPtr m;
	
	m = t->mesg;
	DisposTask(t);
	if (m->pack.type == CMD_GRAFDATA) {
		AckGraf(m);
	} else if (m->pack.type == CMD_VOICEDATA) {
		AckVoice(m);
	}
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_LOGIN(PackPtr thePack, unsigned short *version, Str32 name, Str8 password)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*version = GetUWORD(p);
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
	if (n == 0 || n > 8)
		return paramErr;
	q = password;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_CHPASSWD(PackPtr thePack, Str8 oPassword, Str8 nPassword)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	n = GetUBYTE(p);
	if (n == 0 || n > 8)
		return paramErr;
	q = oPassword;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	if (n == 0 || n > 8)
		return paramErr;
	q = nPassword;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_NEWCONF(PackPtr thePack, Boolean *private, Str32 title)
{
	Byte *p, *q;
	short n;
	
	p = thePack->data;
	*private = GetUBYTE(p);
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

OSErr PD_CMD_LIST(PackPtr thePack)
{
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_USERS(PackPtr thePack, long *id)
{
	Byte *p;
	
	p = thePack->data;
	*id = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_MEMBERS(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_INVITE(PackPtr thePack, long *cid, long *uid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*uid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_JOIN(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_LEAVE(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_MESSAGE(PackPtr thePack, long *id, Str255 text)
{
	Byte *p, *q, c;
	short n;
	
	p = thePack->data;
	*id = GetSLONG(p);
	n = GetUBYTE(p);
	q = text;
	*q++ = n;
	while (n > 0) {
		c = GetUBYTE(p);
		if (c != CR)
			*q++ = c;
		else
			*q++ = (Byte) '¬';
		--n;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_STARTGRAF(PackPtr thePack, long *cid, long *picSize, Rect *picFrame)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*picSize = GetSLONG(p);
	picFrame->top = GetSWORD(p);
	picFrame->left = GetSWORD(p);
	picFrame->bottom = GetSWORD(p);
	picFrame->right = GetSWORD(p);
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

OSErr PD_CMD_ENDGRAF(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_STARTVOICE(PackPtr thePack, long *cid, UnsignedFixed *sampRate, OSType *compType)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	*sampRate = GetULONG(p);
	*compType = GetULONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr PD_CMD_VOICEDATA(PackPtr thePack, long *cid, Ptr data, long *count)
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

OSErr PD_CMD_ENDVOICE(PackPtr thePack, long *cid)
{
	Byte *p;
	
	p = thePack->data;
	*cid = GetSLONG(p);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

long PA_ERR(PackPtr thePack, short error)
{
	Byte *p;
	
	thePack->type = error;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_NONE(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_NONE;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_YOUREIN(PackPtr thePack, long uid, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = RPL_YOUREIN;
	p = thePack->data;
	PutSLONG(uid, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_CHANGED(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_CHANGED;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_CREATED(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = RPL_CREATED;
	p = thePack->data;
	PutSLONG(cid, p);
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_LIST(PackPtr thePack, long cid, Str32 title)
{
	Byte *p, *q;
	short n;
	
	thePack->type = RPL_LIST;
	p = thePack->data;
	PutSLONG(cid, p);
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

long PA_RPL_LISTEND(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_LISTEND;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_USERS(PackPtr thePack, long id, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = RPL_USERS;
	p = thePack->data;
	PutSLONG(id, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_ENDOFUSERS(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_ENDOFUSERS;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_MEMBERS(PackPtr thePack, long uid, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = RPL_MEMBERS;
	p = thePack->data;
	PutSLONG(uid, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_ENDOFMEMBERS(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_ENDOFMEMBERS;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_RPL_INVITING(PackPtr thePack)
{
	Byte *p;
	
	thePack->type = RPL_INVITING;
	p = thePack->data;
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_INVITE(PackPtr thePack, long cid, Str32 title, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_INVITE;
	p = thePack->data;
	PutSLONG(cid, p);
	q = title;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_NOTICE(PackPtr thePack, long cid, short code, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_NOTICE;
	p = thePack->data;
	PutSLONG(cid, p);
	PutSWORD(code, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_MESSAGE(PackPtr thePack, long cid, long uid, Str32 name, Str255 text)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_MESSAGE;
	p = thePack->data;
	PutSLONG(cid, p);
	PutSLONG(uid, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
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

long PA_CMD_STARTGRAF(PackPtr thePack, long cid, long picSize, Rect *picFrame, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_STARTGRAF;
	p = thePack->data;
	PutSLONG(cid, p);
	PutSLONG(picSize, p);
	PutSWORD(picFrame->top, p);
	PutSWORD(picFrame->left, p);
	PutSWORD(picFrame->bottom, p);
	PutSWORD(picFrame->right, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
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

long PA_CMD_ACKGRAF(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_ACKGRAF;
	p = thePack->data;
	PutSLONG(cid, p);
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

long PA_CMD_STARTVOICE(PackPtr thePack, long cid, UnsignedFixed sampRate, OSType compType, Str32 name)
{
	Byte *p, *q;
	short n;
	
	thePack->type = CMD_STARTVOICE;
	p = thePack->data;
	PutSLONG(cid, p);
	PutULONG(sampRate, p);
	PutULONG(compType, p);
	q = name;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	return p - (Byte *) thePack;
}

/*————————————————————————————————————————————————————————————*/

long PA_CMD_VOICEDATA(PackPtr thePack, long cid, Ptr data, long count)
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

long PA_CMD_ACKVOICE(PackPtr thePack, long cid)
{
	Byte *p;
	
	thePack->type = CMD_ACKVOICE;
	p = thePack->data;
	PutSLONG(cid, p);
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

void SND_ERR(short error, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_ERR(&m->pack, error);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_NONE(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_NONE(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_YOUREIN(long uid, Str32 name, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_YOUREIN(&m->pack, uid, name);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_CHANGED(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_CHANGED(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_CREATED(long cid, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_CREATED(&m->pack, cid);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_LIST(long cid, Str32 title, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_LIST(&m->pack, cid, title);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_LISTEND(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_LISTEND(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_USERS(long id, Str32 name, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_USERS(&m->pack, id, name);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_ENDOFUSERS(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_ENDOFUSERS(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_MEMBERS(long uid, Str32 name, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_MEMBERS(&m->pack, uid, name);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_ENDOFMEMBERS(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_ENDOFMEMBERS(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_RPL_INVITING(SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_RPL_INVITING(&m->pack);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_INVITE(long cid, Str32 title, Str32 name, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_CMD_INVITE(&m->pack, cid, title, name);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_ACKGRAF(long cid, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_CMD_ACKGRAF(&m->pack, cid);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void SND_CMD_ACKVOICE(long cid, SessPtr s)
{
	MesgPtr m;
	
	m = NewMesg();
	m->length = PA_CMD_ACKVOICE(&m->pack, cid);
	SendMesg(m, s);
}

/*————————————————————————————————————————————————————————————*/

void Connect(MesgPtr mesg, SessPtr sess)
{
	Str32 name;
	Str8 password;
	MesgPtr m;
	SessPtr s;
	UserPtr user, u;
	unsigned long theTime, timeDiff, tempDiff;
	OSErr error;
	short i, j;
	unsigned short version;
	Boolean found;
	
	error = PD_CMD_LOGIN(&mesg->pack, &version, name, password);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	if (version != 8) {
		SND_ERR(ERR_BADVERSION, sess);
		return;
	}
	GetDateTime(&theTime);
	user = gUsers, found = false;
	for (i = 0; i < MAXUSER; i++) {
		if (user->flags & 0x8000 && EqualString(name, user->name, false, false)) {
			found = true;
			break;
		}
		user++;
	}
	if (!found) {
		if (false) {
			SND_ERR(ERR_NOSUCHNAME, sess);
			return;
		}
		user = gUsers, found = false, timeDiff = 0;
		for (i = 0; i < MAXUSER; i++) {
			if (!(user->flags & 0x8000)) {
				found = true;
				break;
			}
			if (!(user->flags & 0x4000)) {
				tempDiff = theTime - user->time;
				if (tempDiff > timeDiff) {
					timeDiff = tempDiff;
					u = user;
					j = i;
				}
			}
			user++;
		}
		if (!found) {
			if (timeDiff == 0) {
				SND_ERR(ERR_NOSUCHNAME, sess);
				return;
			}
			user = u;
			i = j;
		}
		user->flags = 0xC000;
		user->id = NewUID();
		user->time = theTime;
		PLstrcpy(user->name, name);
		PLstrcpy(user->password, password);
	} else {
		if (!EqualString(password, user->password, true, true)) {
			SND_ERR(ERR_PASSWDMISMATCH, sess);
			return;
		}
		if (user->flags & 0x4000) {
			SND_ERR(ERR_NAMEINUSE, sess);
			return;
		}
		user->flags |= 0x4000;
		user->time = theTime;
	}
	sess->flags |= 0x2000;
	sess->state = sOpen;
	sess->uix = i;
	sess->user = user;
	SND_RPL_YOUREIN(user->id, user->name, sess);
	m = NewMesg();
	m->length = PA_CMD_NOTICE(&m->pack, 0, 0, user->name);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void Disconnect(SessPtr sess)
{
	void LeaveAllConfs(SessPtr);
	
	IPCParamBlockPtr pb;
	MesgPtr m;
	SessPtr s;
	UserPtr user;
	short saveSR, i;
	
	if (sess->state == sOpen || sess->state == sOpening) {
		if (sess->flags & 0x2000) {
			LeaveAllConfs(sess);
			user = sess->user;
			m = NewMesg();
			m->length = PA_CMD_NOTICE(&m->pack, 0, 1, user->name);
			s = gSessions;
			for (i = 0; i < MAXSESS; i++) {
				if (s != sess && s->flags & 0x2000)
					SendMesg(DupMesg(m), s);
				s++;
			}
			DisposMesg(m);
			user->flags &= ~0x4000;
			sess->flags &= ~0x2000;
		}
		saveSR = DisableInt();
		sess->state = sClosing;
		if (sess->flags & 0x0002) {
			RestoreInt(saveSR);
			return;
		}
		sess->flags |= 0x0002;
		RestoreInt(saveSR);
		pb = &sess->paramBlock[1];
		pb->endPB.ioCompletion = (ProcPtr) Listen_0;
		pb->endPB.sessRefNum = sess->refNum;
		pb->endPB.abort = false;
		(void) IPCEnd(pb, true);
	}
}

/*————————————————————————————————————————————————————————————*/

void ChangePassword(MesgPtr mesg, SessPtr sess)
{
	Str8 oPassword, nPassword;
	UserPtr user;
	OSErr error;
	
	error = PD_CMD_CHPASSWD(&mesg->pack, oPassword, nPassword);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	user = sess->user;
	if (!EqualString(oPassword, user->password, true, true)) {
		SND_ERR(ERR_PASSWDMISMATCH, sess);
		return;
	}
	PLstrcpy(user->password, nPassword);
	SND_RPL_CHANGED(sess);
}

/*————————————————————————————————————————————————————————————*/

void NewConf(MesgPtr mesg, SessPtr sess)
{
	Str32 title;
	ConfPtr c;
	Byte *p;
	short i;
	OSErr error;
	Boolean private, found;
	
	error = PD_CMD_NEWCONF(&mesg->pack, &private, title);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags == 0x0000) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_TOOMANYCONFS, sess);
		return;
	}
	c->flags = 0x8000;
	if (private)
		c->flags |= 0x4000;
	c->id = NewCID();
	c->graf = 0;
	c->voice = 0;
	PLstrcpy(c->title, title);
	p = c->users;
	for (i = 0; i < MAXUSER; i++)
		*p++ = 0x00;
	c->users[sess->uix] |= 0xC0;
	c->count = 1;
	SND_RPL_CREATED(c->id, sess);
}

/*————————————————————————————————————————————————————————————*/

void ListConfs(MesgPtr mesg, SessPtr sess)
{
	ConfPtr c;
	short i;
	OSErr error;
	
	error = PD_CMD_LIST(&mesg->pack);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && (!(c->flags & 0x4000) || c->users[sess->uix] & 0x80)
		 && !(c->users[sess->uix] & 0x40)) {
		 	SND_RPL_LIST(c->id, c->title, sess);
		}
		c++;
	}
	SND_RPL_LISTEND(sess);
}

/*————————————————————————————————————————————————————————————*/

void ListUsers(MesgPtr mesg, SessPtr sess)
{
	ConfPtr c;
	SessPtr s;
	UserPtr u;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_USERS(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	if (cid == 0) {
		s = gSessions;
		for (i = 0; i < MAXSESS; i++) {
			if (s != sess && s->flags & 0x2000) {
				u = s->user;
				SND_RPL_USERS(u->id, u->name, sess);
			}
			s++;
		}
		SND_RPL_ENDOFUSERS(sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (c->flags & 0x4000 && !(c->users[sess->uix] & 0x80)) {
		SND_ERR(ERR_INVITEONLYCONF, sess);
		return;
	}
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && !(c->users[s->uix] & 0x40)) {
			u = s->user;
			SND_RPL_USERS(u->id, u->name, sess);
		}
		s++;
	}
	SND_RPL_ENDOFUSERS(sess);
}

/*————————————————————————————————————————————————————————————*/

void ListMembers(MesgPtr mesg, SessPtr sess)
{
	ConfPtr c;
	SessPtr s;
	UserPtr u;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_MEMBERS(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (c->flags & 0x4000 && !(c->users[sess->uix] & 0x80)) {
		SND_ERR(ERR_INVITEONLYCONF, sess);
		return;
	}
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40) {
			u = s->user;
			SND_RPL_MEMBERS(u->id, u->name, sess);
		}
		s++;
	}
	SND_RPL_ENDOFMEMBERS(sess);
}

/*————————————————————————————————————————————————————————————*/

void Invite(MesgPtr mesg, SessPtr sess)
{
	ConfPtr c;
	SessPtr s;
	long cid, uid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_INVITE(&mesg->pack, &cid, &uid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (c->flags & 0x4000 && !(c->users[sess->uix] & 0x80)) {
		SND_ERR(ERR_INVITEONLYCONF, sess);
		return;
	}
	s = gSessions, found = false;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x2000 && s->user->id == uid) {
			found = true;
			break;
		}
		s++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHUSER, sess);
		return;
	}
	if (c->users[s->uix] & 0x40) {
		SND_ERR(ERR_USERONCONF, sess);
		return;
	}
	c->users[s->uix] |= 0x80;
	SND_RPL_INVITING(sess);
	SND_CMD_INVITE(c->id, c->title, sess->user->name, s);
}

/*————————————————————————————————————————————————————————————*/

void JoinConf(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_JOIN(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (c->flags & 0x4000 && !(c->users[sess->uix] & 0x80)) {
		SND_ERR(ERR_INVITEONLYCONF, sess);
		return;
	}
	if (c->users[sess->uix] & 0x40) {
		SND_ERR(ERR_ALREADYONCONF, sess);
		return;
	}
	c->users[sess->uix] |= 0x40;
	c->count++;
	SND_RPL_NONE(sess);
	m = NewMesg();
	m->length = PA_CMD_NOTICE(&m->pack, c->id, 0, sess->user->name);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void LeaveConf(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_LEAVE(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	SND_RPL_NONE(sess);
	if (--c->count == 0)
		c->flags = 0x0000;
	else {
		c->users[sess->uix] &= ~0x40;
		if (c->graf == sess->user->id) {
			c->graf = 0;
			m = NewMesg();
			m->length = PA_CMD_ENDGRAF(&m->pack, cid);
			s = gSessions;
			for (i = 0; i < MAXSESS; i++) {
				if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
					SendMesg(DupMesg(m), s);
				s++;
			}
			DisposMesg(m);
		}
		if (c->voice == sess->user->id) {
			c->voice = 0;
			m = NewMesg();
			m->length = PA_CMD_ENDVOICE(&m->pack, cid);
			s = gSessions;
			for (i = 0; i < MAXSESS; i++) {
				if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
					SendMesg(DupMesg(m), s);
				s++;
			}
			DisposMesg(m);
		}
		m = NewMesg();
		m->length = PA_CMD_NOTICE(&m->pack, cid, 1, sess->user->name);
		s = gSessions;
		for (i = 0; i < MAXSESS; i++) {
			if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
				SendMesg(DupMesg(m), s);
			s++;
		}
		DisposMesg(m);
	}
}

/*————————————————————————————————————————————————————————————*/

void LeaveAllConfs(SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	short i, j;
	
	c = gConfs;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->users[sess->uix] & 0x40) {
			if (--c->count == 0)
				c->flags = 0x0000;
			else {
				c->users[sess->uix] &= ~0x40;
				if (c->graf == sess->user->id) {
					c->graf = 0;
					m = NewMesg();
					m->length = PA_CMD_ENDGRAF(&m->pack, c->id);
					s = gSessions;
					for (j = 0; j < MAXSESS; j++) {
						if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
							SendMesg(DupMesg(m), s);
						s++;
					}
					DisposMesg(m);
				}
				if (c->voice == sess->user->id) {
					c->voice = 0;
					m = NewMesg();
					m->length = PA_CMD_ENDVOICE(&m->pack, c->id);
					s = gSessions;
					for (j = 0; j < MAXSESS; j++) {
						if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
							SendMesg(DupMesg(m), s);
						s++;
					}
					DisposMesg(m);
				}
				m = NewMesg();
				m->length = PA_CMD_NOTICE(&m->pack, c->id, 1, sess->user->name);
				s = gSessions;
				for (j = 0; j < MAXSESS; j++) {
					if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
						SendMesg(DupMesg(m), s);
					s++;
				}
				DisposMesg(m);
			}
		}
		c++;
	}
}

/*————————————————————————————————————————————————————————————*/

void Broadcast(MesgPtr mesg, SessPtr sess)
{
	Str255 text;
	MesgPtr m;
	UserPtr u;
	SessPtr s;
	ConfPtr c;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_MESSAGE(&mesg->pack, &cid, text);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	SND_RPL_NONE(sess);
	u = sess->user;
	m = NewMesg();
	m->length = PA_CMD_MESSAGE(&m->pack, cid, u->id, u->name, text);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void StartGraf(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	UserPtr u;
	SessPtr s;
	ConfPtr c;
	Rect picFrame;
	long cid, picSize;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_STARTGRAF(&mesg->pack, &cid, &picSize, &picFrame);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->graf != 0) {
		SND_ERR(ERR_GRAFBUSY, sess);
		return;
	}
	SND_RPL_NONE(sess);
	u = sess->user;
	c->graf = u->id;
	m = NewMesg();
	m->length = PA_CMD_STARTGRAF(&m->pack, cid, picSize, &picFrame, u->name);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void GrafData(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	Byte data[500];
	long cid, length;
	short i, saveSR;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_GRAFDATA(&mesg->pack, &cid, (Ptr) data, &length);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->graf != sess->user->id) {
		SND_ERR(ERR_GRAFBUSY, sess);
		return;
	}
	m = NewMesg();
	m->length = PA_CMD_GRAFDATA(&m->pack, cid, (Ptr) data, length);
	m->from = sess->user->id;
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	saveSR = DisableInt();
	if (m->count == 1) {
		DisposMesg(m);
		RestoreInt(saveSR);
		SND_CMD_ACKGRAF(cid, sess);
	} else {
		DisposMesg(m);
		RestoreInt(saveSR);
	}
}

/*————————————————————————————————————————————————————————————*/

void AckGraf(MesgPtr mesg)
{
	SessPtr s;
	long uid, cid;
	short i;
	
	uid = mesg->from;
	cid = *(long *) &mesg->pack.data;
	DisposMesg(mesg);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x2000 && s->user->id == uid) {
			SND_CMD_ACKGRAF(cid, s);
			break;
		}
		s++;
	}
}

/*————————————————————————————————————————————————————————————*/

void EndGraf(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ENDGRAF(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->graf != sess->user->id) {
		SND_ERR(ERR_VOICEBUSY, sess);
		return;
	}
	SND_RPL_NONE(sess);
	c->graf = 0;
	m = NewMesg();
	m->length = PA_CMD_ENDGRAF(&m->pack, cid);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void StartVoice(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	UserPtr u;
	SessPtr s;
	ConfPtr c;
	OSType compType;
	UnsignedFixed sampRate;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_STARTVOICE(&mesg->pack, &cid, &sampRate, &compType);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->voice != 0) {
		SND_ERR(ERR_VOICEBUSY, sess);
		return;
	}
	SND_RPL_NONE(sess);
	u = sess->user;
	c->voice = u->id;
	m = NewMesg();
	m->length = PA_CMD_STARTVOICE(&m->pack, cid, sampRate, compType, u->name);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void VoiceData(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	Byte data[504];
	long cid, length;
	short i, saveSR;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_VOICEDATA(&mesg->pack, &cid, (Ptr) data, &length);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->voice != sess->user->id) {
		SND_ERR(ERR_VOICEBUSY, sess);
		return;
	}
	m = NewMesg();
	m->length = PA_CMD_VOICEDATA(&m->pack, cid, (Ptr) data, length);
	m->from = sess->user->id;
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	saveSR = DisableInt();
	if (m->count == 1) {
		DisposMesg(m);
		RestoreInt(saveSR);
		SND_CMD_ACKVOICE(cid, sess);
	} else {
		DisposMesg(m);
		RestoreInt(saveSR);
	}
}

/*————————————————————————————————————————————————————————————*/

void AckVoice(MesgPtr mesg)
{
	SessPtr s;
	long uid, cid;
	short i;
	
	uid = mesg->from;
	cid = *(long *) &mesg->pack.data;
	DisposMesg(mesg);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s->flags & 0x2000 && s->user->id == uid) {
			SND_CMD_ACKVOICE(cid, s);
			break;
		}
		s++;
	}
}

/*————————————————————————————————————————————————————————————*/

void EndVoice(MesgPtr mesg, SessPtr sess)
{
	MesgPtr m;
	SessPtr s;
	ConfPtr c;
	long cid;
	short i;
	OSErr error;
	Boolean found;
	
	error = PD_CMD_ENDVOICE(&mesg->pack, &cid);
	DisposMesg(mesg);
	if (error != noErr) {
		SND_ERR(ERR_BADPARAMS, sess);
		return;
	}
	c = gConfs, found = false;
	for (i = 0; i < MAXCONF; i++) {
		if (c->flags & 0x8000 && c->id == cid) {
			found = true;
			break;
		}
		c++;
	}
	if (!found) {
		SND_ERR(ERR_NOSUCHCONF, sess);
		return;
	}
	if (!(c->users[sess->uix] & 0x40)) {
		SND_ERR(ERR_NOTONCONF, sess);
		return;
	}
	if (c->voice != sess->user->id) {
		SND_ERR(ERR_VOICEBUSY, sess);
		return;
	}
	SND_RPL_NONE(sess);
	c->voice = 0;
	m = NewMesg();
	m->length = PA_CMD_ENDVOICE(&m->pack, cid);
	s = gSessions;
	for (i = 0; i < MAXSESS; i++) {
		if (s != sess && s->flags & 0x2000 && c->users[s->uix] & 0x40)
			SendMesg(DupMesg(m), s);
		s++;
	}
	DisposMesg(m);
}

/*————————————————————————————————————————————————————————————*/

void Command(MesgPtr mesg, SessPtr sess)
{
	if (sess->state == sOpening) {
		switch (mesg->pack.type) {
		case CMD_LOGIN:
			Connect(mesg, sess);
			break;
		default:
			DisposMesg(mesg);
			SND_ERR(ERR_UNKNOWNCOMMAND, sess);
			break;
		}
	} else if (sess->state == sOpen) {
		switch (mesg->pack.type) {
		case CMD_QUIT:
			DisposMesg(mesg);
			Disconnect(sess);
			break;
		case CMD_CHPASSWD:
			ChangePassword(mesg, sess);
			break;
		case CMD_NEWCONF:
			NewConf(mesg, sess);
			break;
		case CMD_LIST:
			ListConfs(mesg, sess);
			break;
		case CMD_USERS:
			ListUsers(mesg, sess);
			break;
		case CMD_MEMBERS:
			ListMembers(mesg, sess);
			break;
		case CMD_INVITE:
			Invite(mesg, sess);
			break;
		case CMD_JOIN:
			JoinConf(mesg, sess);
			break;
		case CMD_LEAVE:
			LeaveConf(mesg, sess);
			break;
		case CMD_MESSAGE:
			Broadcast(mesg, sess);
			break;
		case CMD_STARTGRAF:
			StartGraf(mesg, sess);
			break;
		case CMD_GRAFDATA:
			GrafData(mesg, sess);
			break;
		case CMD_ENDGRAF:
			EndGraf(mesg, sess);
			break;
		case CMD_STARTVOICE:
			StartVoice(mesg, sess);
			break;
		case CMD_VOICEDATA:
			VoiceData(mesg, sess);
			break;
		case CMD_ENDVOICE:
			EndVoice(mesg, sess);
			break;
		default:
			DisposMesg(mesg);
			SND_ERR(ERR_UNKNOWNCOMMAND, sess);
			break;
		}
	}
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPOpen(void)
{
	MPPParamBlock paramBlock;
	MPPPBPtr pb;
	OSErr error;
	
	{
		pb = &paramBlock;
		pb->SETSELF.ioRefNum = mppRefNum;
		pb->SETSELF.csCode = setSelfSend;
		pb->SETSELF.newSelfFlag = true;
		if ((error = PBControlSync((ParmBlkPtr) pb)) != noErr)
			return error;
	}
	if ((error = InitDefer()) != noErr)
		return error;
	if ((error = InitMesgs()) != noErr)
		return error;
	if ((error = InitMHdrs()) != noErr)
		return error;
	if ((error = OpenUsers()) != noErr)
		return error;
	if ((error = OpenConfs()) != noErr)
		return error;
	if ((error = OpenSessions()) != noErr)
		return error;
	if ((gNTE = NewPtrClear(sizeof(NamesTableEntry))) == nil)
		return memFullErr;
	NBPSetNTE(gNTE, "\pChitChat", "\pMCPServer", "\p*", gSocket);
	{
		pb = &paramBlock;
		pb->NBP.ioRefNum = mppRefNum;
		pb->NBP.csCode = registerName;
		pb->NBP.interval = 8;
		pb->NBP.count = 5;
		pb->NBP.nbpPtrs.ntQElPtr = gNTE;
		pb->NBP.parm.verifyFlag = true;
		if ((error = PBControlSync((ParmBlkPtr) pb)) != noErr)
			return error;
	}
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

OSErr MCPClose(void)
{
	MPPParamBlock paramBlock;
	MPPPBPtr pb;
	
	{
		pb = &paramBlock;
		pb->NBP.ioRefNum = mppRefNum;
		pb->NBP.csCode = removeName;
		pb->NBP.nbpPtrs.entityPtr = (Ptr) &((NamesTableEntry *) gNTE)->nt.entityData;
		(void) PBControlSync((ParmBlkPtr) pb);
	}
	DisposePtr(gNTE);
	(void) CloseSessions();
	(void) CloseConfs();
	(void) CloseUsers();
	return noErr;
}

/*————————————————————————————————————————————————————————————*/
