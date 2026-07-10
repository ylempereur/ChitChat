/*

	File:		Semaphore.h
	
	Copyright:	© 1995 by Mainstay, Inc.
				All rights reserved.
	
	Version:	1.0d14
	Created:	Tuesday, March 28, 1995

*/

#ifndef __SEMAPHORE__
#define __SEMAPHORE__

#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __OSUTILS__
#include <OSUtils.h>
#endif

struct SMTask {
	QElemPtr smLink;
	ProcPtr smAddr;
};

typedef struct SMTask SMTask;
typedef SMTask *SMTaskPtr;

#ifdef __cplusplus
extern "C" {
#endif

extern pascal void SMInit(QHdrPtr sem);
extern pascal void SMInstall(SMTaskPtr task, QHdrPtr sem);
extern pascal void SMLock(SMTaskPtr task, QHdrPtr sem);
extern pascal void SMUnlock(QHdrPtr sem);

/* pascal void MyTaskProc(SMTaskPtr task); */

#ifdef __cplusplus
}
#endif

#endif
