/*
 	File:		SmartScrollAPI.h
 
 	Contains:	Smart Scroll Application Programming Interface
 
 	Version:	1.2
 
 	Copyright:	© 1996 by Marc Moini, portions by Marc Menschenfreund,
				Alessandro Levi Montalcini and Mark Shirley (Thanks!)
  				All rights reserved.

 	Bugs?:		If you find a problem with this file, please email Marc@Kagi.com
 
*/

#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __CONTROLS__
#include <Controls.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern pascal void SetSmartScrollInfo (ControlRef theScrollBar, long amountVisible , long amountTotal);
extern pascal void SetSmartScrollProp (ControlHandle theScrollBar, Fract proportion);
extern pascal Fract GetSmartScrollProp (ControlHandle theScrollBar);
extern pascal void DisposeAllSmartScrolls (void);


#ifdef __cplusplus
}
#endif
