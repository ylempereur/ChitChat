/* Server resource file */

#include "SysTypes.r"
#include "Types.r"

include "Server.rsrc";

resource 'MBAR' (128, preload) {
	{
		128, 129, 130
	}
};

resource 'MENU' (128, preload) {
	128, textMenuProc, 0x7FFFFFFD, enabled, apple,
	{
		"About ChitChat Server…", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (129, preload) {
	129, textMenuProc, 0x7FFFFFFD, enabled, "File",
	{
		"Close", noIcon, "W", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Quit", noIcon, "Q", noMark, plain
	}
};

resource 'MENU' (130, preload) {
	130, textMenuProc, 0x7FFFFFFD, enabled, "Edit",
	{
		"Undo", noIcon, "Z", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Cut", noIcon, "X", noMark, plain,
		"Copy", noIcon, "C", noMark, plain,
		"Paste", noIcon, "V", noMark, plain,
		"Clear", noIcon, noKey, noMark, plain,
		"Select All", noIcon, "A", noMark, plain
	}
};

resource 'SIZE' (-1, purgeable) {
	reserved,
	acceptSuspendResumeEvents,
	reserved,
	canBackground,
	doesActivateOnFGSwitch,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreAppDiedEvents,
	is32BitCompatible,
	isHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
	352 * 1024,			/* 704, 352, 256 */
	288 * 1024			/* 640, 288, 192 */
};

resource 'vers' (1, purgeable) {
	0x01, 0x00, release, 0x00, verUS,
	"1.0",
	"1.0, ©1996 Mainstay"
};

resource 'vers' (2, purgeable) {
	0x01, 0x00, release, 0x00, verUS,
	"1.0",
	"ChitChat Server™ v1.0"
};

type 'TMP3' as 'STR ';

resource 'TMP3' (0, purgeable) {
	"ChitChat Server™ v1.0 -- 8/1/96"
};

resource 'BNDL' (128, purgeable) {
	'TMP3', 0,
	{
		'FREF', {0, 128},
		'ICN#', {0, 128}
	}
};

resource 'FREF' (128, purgeable) {
	'APPL', 0,
	""
};

