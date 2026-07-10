/* Prefs.c */

/*————————————————————————————————————————————————————————————*/

#define SystemSevenOrLater 1
#define OLDROUTINELOCATIONS 0

/*————————————————————————————————————————————————————————————*/

#include <Types.h>
#include <Errors.h>
#include <PLStringFuncs.h>
#include <Files.h>
#include <Folders.h>
#include <TextUtils.h>
#include <Processes.h>

#include "Misc.h"

/*————————————————————————————————————————————————————————————*/

extern Str32 gObject, gZone, gUserName;
extern Str8 gPassword;
extern MLHandle macroList;
extern short gRate;
extern Boolean gSplash, gAutoLogon, gTracking, hasFindFolder;

/*————————————————————————————————————————————————————————————*/

#pragma segment Main

/*————————————————————————————————————————————————————————————*/

static OSErr MakeSpec(FSSpec *spec)
{
	OSErr error;
	
	if (!hasFindFolder)
		return unimpErr;
	error = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder,
		&spec->vRefNum, &spec->parID);
	if (error != noErr)
		return error;
	GetIndString(spec->name, 301, 1);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static OSErr ReadPrefs(void)
{
	FSSpec spec;
	MLPtr ml;
	MacroPtr m;
	Ptr prefs;
	Byte *p, *q;
	long prefSize;
	short refNum, c, n;
	unsigned short version;
	OSErr error;
	
	error = MakeSpec(&spec);
	if (error != noErr)
		return error;
	error = FSpOpenDF(&spec, fsRdPerm, &refNum);
	if (error != noErr)
		return error;
	error = GetEOF(refNum, &prefSize);
	if (error != noErr) {
		(void) FSClose(refNum);
		return error;
	}
	prefs = NewPtr(prefSize);
	if (prefs == nil) {
		(void) FSClose(refNum);
		return memFullErr;
	}
	error = FSRead(refNum, &prefSize, prefs);
	if (error != noErr) {
		DisposePtr(prefs);
		(void) FSClose(refNum);
		return error;
	}
	error = FSClose(refNum);
	if (error != noErr) {
		DisposePtr(prefs);
		return error;
	}
	p = (Byte *) prefs;
	version = GetUWORD(p);
	if (version != 3) {
		DisposePtr(prefs);
		return paramErr;
	}
	n = GetUBYTE(p);
	if (n > 32) {
		DisposePtr(prefs);
		return paramErr;
	}
	q = gObject;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	if (n > 32) {
		DisposePtr(prefs);
		return paramErr;
	}
	q = gZone;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	if (n > 32) {
		DisposePtr(prefs);
		return paramErr;
	}
	q = gUserName;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	n = GetUBYTE(p);
	if (n > 8) {
		DisposePtr(prefs);
		return paramErr;
	}
	q = gPassword;
	*q++ = n;
	while (n > 0) {
		*q++ = GetUBYTE(p);
		--n;
	}
	gSplash = GetUBYTE(p);
	gAutoLogon = GetUBYTE(p);
	gTracking = GetUBYTE(p);
	gRate = GetSWORD(p);
	c = GetSWORD(p);
	HLock((Handle) macroList);
	ml = *macroList;
	ml->count = c;
	m = ml->macros;
	while (c) {
		n = GetUBYTE(p);
		q = m->name;
		*q++ = n;
		while (n) {
			*q++ = GetUBYTE(p);
			n--;
		}
		n = GetUBYTE(p);
		q = m->text;
		*q++ = n;
		while (n) {
			*q++ = GetUBYTE(p);
			n--;
		}
		m++;
		c--;
	}
	HUnlock((Handle) macroList);
	DisposePtr(prefs);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

static void GetUserName(Str32 userName)
{
	StringHandle theName;
	
	theName = GetString(-16096);
	if (theName == nil)
		PLstrcpy(userName, "\p");
	else
		PLstrcpy(userName, *theName);
}

/*————————————————————————————————————————————————————————————*/

void GetPrefs(void)
{
	OSErr error;
	
	macroList = (MLHandle) NewHandle(sizeof(MacroList));
	if (macroList == nil)
		ExitToShell();
	error = ReadPrefs();
	if (error == noErr)
		return;
	PLstrcpy(gObject, "\p");
	PLstrcpy(gZone, "\p*");
	GetUserName(gUserName);
	PLstrcpy(gPassword, "\p");
	gSplash = true;
	gAutoLogon = false;
	gTracking = true;
	gRate = 2;
	(*macroList)->count = 0;
}

/*————————————————————————————————————————————————————————————*/

static OSErr WritePrefs(void)
{
	FSSpec spec;
	MLPtr ml;
	MacroPtr m;
	Ptr prefs;
	Byte *p, *q;
	long prefSize;
	OSErr error;
	short refNum, c, n;
	
	prefs = NewPtr(128+14452);
	if (prefs == nil)
		return memFullErr;
	p = (Byte *) prefs;
	PutUWORD(3, p);
	q = gObject;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = gZone;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = gUserName;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	q = gPassword;
	n = *q++;
	PutUBYTE(n, p);
	while (n > 0) {
		PutUBYTE(*q++, p);
		--n;
	}
	PutUBYTE(gSplash, p);
	PutUBYTE(gAutoLogon, p);
	PutUBYTE(gTracking, p);
	PutSWORD(gRate, p);
	HLock((Handle) macroList);
	ml = *macroList;
	c = ml->count;
	m = ml->macros;
	PutSWORD(c, p);
	while (c) {
		q = m->name;
		n = *q++;
		PutUBYTE(n, p);
		while (n) {
			PutUBYTE(*q++, p);
			n--;
		}
		q = m->text;
		n = *q++;
		PutUBYTE(n, p);
		while (n) {
			PutUBYTE(*q++, p);
			n--;
		}
		m++;
		c--;
	}
	HUnlock((Handle) macroList);
	prefSize = p - (Byte *) prefs;
	error = MakeSpec(&spec);
	if (error != noErr) {
		DisposePtr(prefs);
		return error;
	}
	error = FSpOpenDF(&spec, fsWrPerm, &refNum);
	if (error == fnfErr) {
		error = FSpCreate(&spec, 'TMP2', 'PREF', smSystemScript);
		if (error != noErr) {
			DisposePtr(prefs);
			return error;
		}
		error = FSpOpenDF(&spec, fsWrPerm, &refNum);
		if (error != noErr) {
			(void) FSpDelete(&spec);
			DisposePtr(prefs);
			return error;
		}
	} else if (error != noErr) {
		DisposePtr(prefs);
		return error;
	} else {
		error = SetEOF(refNum, 0);
		if (error != noErr) {
			(void) FSClose(refNum);
			DisposePtr(prefs);
			return error;
		}
	}
	error = FSWrite(refNum, &prefSize, prefs);
	if (error != noErr) {
		(void) FSClose(refNum);
		(void) FSpDelete(&spec);
		DisposePtr(prefs);
		return error;
	}
	error = FSClose(refNum);
	if (error != noErr) {
		(void) FSpDelete(&spec);
		DisposePtr(prefs);
		return error;
	}
	DisposePtr(prefs);
	return noErr;
}

/*————————————————————————————————————————————————————————————*/

void PutPrefs(void)
{
	OSErr error;
	
	if (!gAutoLogon)
		PLstrcpy(gPassword, "\p");
	error = WritePrefs();
}

/*————————————————————————————————————————————————————————————*/
