/*

	File:		IPC.h
	
	Copyright:	© 1994-1995 by Mainstay, Inc.
				All rights reserved.
	
	Version:	1.0d14
	Created:	Tuesday, April 5, 1994

*/

#ifndef __IPC__
#define __IPC__

#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __APPLETALK__
#include <AppleTalk.h>
#endif

enum {
	kIPCNoGlobalsErr = -32000,
	kIPCNotInitErr = -32001,
	kIPCNoSessionErr = -32002,
	kIPCBadReqErr = -32003,
	kIPCNotFoundErr = -32004
};

#define IPCParamHeader \
	Ptr qLink; \
	long reserved; \
	long saveA5; \
	ProcPtr ioCompletion; \
	OSErr ioResult;

struct IPCOpenPB {
	IPCParamHeader
	short sessRefNum;
	unsigned char localSocket;
};

typedef struct IPCOpenPB IPCOpenPB;

struct IPCClosePB {
	IPCParamHeader
	short sessRefNum;
	Boolean abort;
};

typedef struct IPCClosePB IPCClosePB;

struct IPCStartPB {
	IPCParamHeader
	short sessRefNum;
	EntityPtr locationName;
};

typedef struct IPCStartPB IPCStartPB;

struct IPCEndPB {
	IPCParamHeader
	short sessRefNum;
	Boolean abort;
};

typedef struct IPCEndPB IPCEndPB;

struct IPCListenPB {
	IPCParamHeader
	short sessRefNum;
};

typedef struct IPCListenPB IPCListenPB;

struct IPCReadPB {
	IPCParamHeader
	short sessRefNum;
	unsigned short bufferLength;
	unsigned short actualLength;
	Ptr bufferPtr;
	Boolean more;
};

typedef struct IPCReadPB IPCReadPB;

struct IPCWritePB {
	IPCParamHeader
	short sessRefNum;
	unsigned short bufferLength;
	unsigned short actualLength;
	Ptr bufferPtr;
	Boolean more;
};

typedef struct IPCWritePB IPCWritePB;

union IPCParamBlock {
	struct {IPCParamHeader} header;
	IPCOpenPB openPB;
	IPCClosePB closePB;
	IPCStartPB startPB;
	IPCEndPB endPB;
	IPCListenPB listenPB;
	IPCReadPB readPB;
	IPCWritePB writePB;
};

typedef union IPCParamBlock IPCParamBlock;
typedef IPCParamBlock *IPCParamBlockPtr;

#ifdef __cplusplus
extern "C" {
#endif

extern pascal OSErr IPCInit(void);
extern pascal OSErr IPCOpen(IPCParamBlockPtr paramBlock);
extern pascal OSErr IPCClose(IPCParamBlockPtr paramBlock);
extern pascal OSErr IPCStart(IPCParamBlockPtr paramBlock, Boolean async);
extern pascal OSErr IPCEnd(IPCParamBlockPtr paramBlock, Boolean async);
extern pascal OSErr IPCListen(IPCParamBlockPtr paramBlock, Boolean async);
extern pascal OSErr IPCRead(IPCParamBlockPtr paramBlock, Boolean async);
extern pascal OSErr IPCWrite(IPCParamBlockPtr paramBlock, Boolean async);
extern pascal OSErr IPCBrowser(Boolean hasDefault, EntityName *theLocation, ConstStr32Param theLocNBPType);

/* pascal void MyCompletionRoutine(IPCParamBlockPtr paramBlock); */

#ifdef __cplusplus
}
#endif

#endif
