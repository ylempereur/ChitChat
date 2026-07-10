/*
 	File:		SmartScrollAPI.c
 
 	Contains:	Smart Scroll Application Programming Interface code
 
 	Version:	1.2
 
 	Copyright:	© 1996 by Marc Moini, portions by Marc Menschenfreund,
				Alessandro Levi Montalcini and Mark Shirley (Thanks!)
 				All rights reserved.
 
 	Bugs?:		If you find a problem with this file, please email Marc@Kagi.com
 
*/

#define SystemSevenOrLater 1

#include <Errors.h>
#include <Traps.h>
#include <OSUtils.h>
#include <Gestalt.h>

#ifndef __SMARTSCROLLAPI__
#include "SmartScrollAPI.h"
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#define	kgestaltSmartScroll	'MMBS'

enum
	{
	kSetSmartScrollInfo = 0L,
	kSetSmartScrollProp,
	kGetSmartScrollProp,
	kDisposeAllSmartScrolls
	};

typedef pascal void (*SmartScrollProcPtr) (long selector,long *result,ControlRef theScrollBar,long param1,long param2);

typedef struct {
	char				privateStuff[16];
	SmartScrollProcPtr	dispatchProc;
	long				smartScrollSignature;
} SmartScrollGestaltRec, *SmartScrollGestaltPtr;


#if GENERATINGCFM
#define CallSmartScrollProc(userRoutine, selector, result, theControl, param1, param2)		\
		CallUniversalProc((UniversalProcPtr)(userRoutine), uppSmartScrollProcInfo, (selector), (result), (theControl), (param1), (param2))
#else
#define CallSmartScrollProc(userRoutine, selector, result, theControl, param1, param2)		\
		(*(userRoutine))((selector), (result), (theControl), (param1), (param2))
#endif

enum	{
	uppSmartScrollProcInfo = kPascalStackBased
		| STACK_ROUTINE_PARAMETER (1, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER (2, SIZE_CODE(sizeof(long *)))
		| STACK_ROUTINE_PARAMETER (3, SIZE_CODE(sizeof(ControlRef)))
		| STACK_ROUTINE_PARAMETER (4, SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER (5, SIZE_CODE(sizeof(long)))
};
/****************************************************************************************/

static OSErr __SmartScrollDispatch(long selector,long *result,ControlRef theControl,long param1,long param2)
	{
	OSErr 					ret = paramErr;
	SmartScrollGestaltPtr	ssRec;
	
	if (NGetTrapAddress (_Gestalt, OSTrap) != NGetTrapAddress (_Unimplemented, OSTrap))
		{
		if (Gestalt ( kgestaltSmartScroll, (long *)&ssRec)==noErr
			&& ssRec && ssRec->smartScrollSignature==kgestaltSmartScroll)
			{
				CallSmartScrollProc(ssRec->dispatchProc,selector,result,theControl,param1,param2);
				ret = noErr;
			}
		}
	return ret;
	}
	
/****************************************************************************************/
/* Call this routine to set the Visible/Total proportion for a scrollbar. */
/*  amountVisible is a 32bit value representing the size of the portion of the document that is visible now. */
/*  amountTotal is a 32bit value representing the size of the whole document. */
/*  these two parameters must share the same unit (pixels, lines, characters, frames, etc). */

pascal void SetSmartScrollInfo (ControlRef theScrollBar, long amountVisible , long amountTotal)
	{
	long 	dummy;
	
	(void)__SmartScrollDispatch(kSetSmartScrollInfo,&dummy,theScrollBar,amountVisible,amountTotal);
	}

/****************************************************************************************/
/* Call this routine to set the Visible/Total proportion for a scrollbar. */
/*  proportion is a Fract value (32bit) representing the Visible/Total ratio */
/*  This call has the exact same effect as SetSmartScrollInfo, you may use either one. */

pascal void SetSmartScrollProp (ControlHandle theScrollBar, Fract proportion)
/*
Comment
*/
	{
	long 	dummy;
	
	(void)__SmartScrollDispatch(kSetSmartScrollProp,&dummy,theScrollBar,(long)proportion,0);
	}

/****************************************************************************************/
/* Call this routine to get the last proportion you stored for this scrollbar. */
/* the value returned will be 0 if there is an error (Smart Scroll not installed, or no value stored)  */

pascal Fract GetSmartScrollProp (ControlHandle theScrollBar)
	{
	Fract 	result = 0L;
	
	__SmartScrollDispatch(kGetSmartScrollProp,(long *)&result,theScrollBar,0,0);
	return result;
	}

/****************************************************************************************/
/* Call this routine before your code Quits, to help SmartScroll */
/* free the memory it reserved for the scrollbars in your Application. */

pascal void DisposeAllSmartScrolls (void)
	{
	long 	dummy;
	
	(void)__SmartScrollDispatch(kDisposeAllSmartScrolls,&dummy,NULL,0,0);
	}
