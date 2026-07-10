/* Client resource file */

#define evenNewerTemp

#include "SysTypes.r"
#include "Types.r"

#define DEMO 0

#define pmCourteous 0x0000
#define pmTolerant  0x0002
#define pmAnimated  0x0004
#define pmExplicit  0x0008
#define pmWhite     0x0010
#define pmBlack     0x0020
#define pmInhibitG2 0x0100
#define pmInhibitC2 0x0200
#define pmInhibitG4 0x0400
#define pmInhibitC4 0x0800
#define pmInhibitG8 0x1000
#define pmInhibitC8 0x2000
#define grayDevInhibit (pmInhibitG2 + pmInhibitG4 + pmInhibitG8)

include "Client.rsrc";

resource 'MBAR' (128, preload) {
	{
		128, 129, 130, 131, 132
	}
};

resource 'MENU' (128, preload) {
	128, textMenuProc, 0x7FFFFFFD, enabled, apple,
	{
		"About ChitChat…", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (129, preload) {
	129, textMenuProc, 0x7FFFFF5B, enabled, "File",
	{
		"Close", noIcon, "W", noMark, plain,
		"Save as…", noIcon, "S", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Page Setup…", noIcon, noKey, noMark, plain,
		"Print…", noIcon, "P", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Preferences…", noIcon, noKey, noMark, plain,
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
		"Clear", noIcon, "B", noMark, plain,
		"Select All", noIcon, "A", noMark, plain
	}
};

resource 'MENU' (131, preload) {
	131, textMenuProc, 0x7FFFFFED, enabled, "Special",
	{
		"Connect…", noIcon, "K", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Invite…", noIcon, "I", noMark, plain,
		"Join…", noIcon, "J", noMark, plain,
		"-", noIcon, noKey, noMark, plain,
		"Change Password…", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (132, preload) {
	132, textMenuProc, allEnabled, enabled, "Voice",
	{
		"22 kHz (Ethernet)", noIcon, noKey, noMark, plain,
		"11 kHz (LocalTalk)", noIcon, noKey, noMark, plain,
		"7 kHz (Modem-Fast)", noIcon, noKey, noMark, plain,
		"5 kHz (Modem-Slow)", noIcon, noKey, noMark, plain
	}
};

resource 'MENU' (150, preload) {
	150, textMenuProc, 0x7FFFFFFD, enabled, "List",
	{
		"Edit List…", noIcon, noKey, noMark, plain,
		"-", noIcon, noKey, noMark, plain
	}
};

resource 'WIND' (200, preload) {
	{50, 10, 326, 414}, zoomDocProc, invisible, goAway, 0,
	"Conference"
};

resource 'pltt' (200, preload) {
	{
		0x0000, 0x0000, 0x0000, pmTolerant, 0,
		0xFFFF, 0xFFFF, 0xFFFF, pmTolerant, 0,
		0x7777, 0x7777, 0x7777, (pmTolerant + grayDevInhibit), 0,
		0xDDDD, 0xDDDD, 0xDDDD, (pmTolerant + grayDevInhibit), 0,
		0xFFFF, 0x0000, 0x0000, (pmTolerant + grayDevInhibit), 0
	}
};

resource 'wctb' (200, preload) {
	{
	},
	{
	}
};

resource 'CNTL' (200, preload) {
	{0, 0, 60, 16}, 0, invisible, 0, 0, scrollBarProc, 0,
	""
};

resource 'STR#' (300, preload) {
	{
		"Save text as:",
		"Save picture as:"
	}
};

resource 'WIND' (202, preload) {
	{0, 0, 20, 400}, noGrowDocProc, invisible, goAway, 0,
	"Tracking"
};

resource 'pltt' (202, preload) {
	{
		0x0000, 0x0000, 0x0000, pmTolerant, 0,
		0xFFFF, 0xFFFF, 0xFFFF, pmTolerant, 0
	}
};

resource 'wctb' (202, preload) {
	{
	},
	{
	}
};

resource 'WIND' (203, preload) {
	{50, 10, 250, 310}, zoomDocProc, invisible, goAway, 0,
	"Graphic"
};

resource 'pltt' (203, preload) {
	{
		0x0000, 0x0000, 0x0000, pmTolerant, 0,
		0xFFFF, 0xFFFF, 0xFFFF, pmTolerant, 0
	}
};

resource 'wctb' (203, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (200, preload) {
	{0, 0, 170, 400}, dBoxProc, invisible, noGoAway, 0, 200,
	""
};

resource 'DITL' (200, preload) {
	{
		{138, 318, 158, 388}, Button {enabled, "OK"},
		{138, 234, 158, 304}, Button {enabled, "Cancel"},
		{63, 173, 79, 377}, EditText {enabled, ""},
		{95, 173, 111, 233}, EditText {enabled, ""},
		{8, 8, 40, 392}, StaticText {disabled, "Connect to conference server “^0” as:"},
		{71, 31, 103, 63}, UserItem {disabled},
		{63, 86, 79, 166}, UserItem {disabled},
		{95, 86, 111, 166}, UserItem {disabled},
		{48, 8, 126, 392}, UserItem {disabled},
		{134, 314, 162, 392}, UserItem {disabled}
	}
};

resource 'ICN#' (200, preload) {
	{
		$"0007 FF80 0008 0000 0008 7E20 0008 0120"
		$"0008 0120 0408 0120 0C08 0120 0808 0120"
		$"6608 0120 9908 0120 8108 FE20 8008 0020"
		$"8008 0020 4A00 1F20 3400 0020 00FC 0020"
		$"0300 0000 0401 FFF0 0400 0008 03E0 1FE4"
		$"0010 0552 0010 00A9 0020 0001 0020 01FE"
		$"0018 0000 0004 0000 0000 C000 0001 2000"
		$"0002 5000 0000 8800 0000 0800 0000 1000",

		$"0007 FF80 0008 0000 0008 7E20 0008 0120"
		$"0008 0120 0408 0120 0C08 0120 0808 0120"
		$"6608 0120 9908 0120 8108 FE20 8008 0020"
		$"8008 0020 4A00 1F20 3400 0020 00FC 0020"
		$"0300 0000 0401 FFF0 0400 0008 03E0 1FE4"
		$"0010 0552 0010 00A9 0020 0001 0020 01FE"
		$"0018 0000 0004 0000 0000 C000 0001 2000"
		$"0002 5000 0000 8800 0000 0800 0000 1000"
	}
};

resource 'icl8' (200, preload) {
	$"0000 0000 0000 0000 0000 0000 00FD FDFD"
	$"FDFD FDFD FDFD FDFD FD00 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 FD00 0000"
	$"002F 2F2F 2F2F 2F00 0000 FD00 0000 0000"
	$"0000 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"0000 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"0000 0000 00E7 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"0000 0000 E7E7 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"0000 0000 E700 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"00FD FD00 00FD FD00 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"FD00 00FD FD00 00FD 0000 0000 FD00 0000"
	$"0000 0000 0000 002F 0000 FD00 0000 0000"
	$"FD00 0000 0000 00FD 0000 0000 FD00 0000"
	$"2F2F 2F2F 2F2F 2F00 0000 FD00 0000 0000"
	$"FD00 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 0000 0000 FD00 0000 0000"
	$"FD00 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 0000 0000 FD00 0000 0000"
	$"00FD 0000 FD00 FD00 0000 0000 0000 0000"
	$"0000 00FD FDFD FDFD 0000 FD00 0000 0000"
	$"0000 FDFD 00FD 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 FD00 0000 0000"
	$"0000 0000 0000 0000 D7D7 D7D7 D7D7 0000"
	$"0000 0000 0000 0000 0000 FD00 0000 0000"
	$"0000 0000 0000 D7D7 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 00D7 0000 0000 0000 0000 00FD"
	$"FDFD FDFD FDFD FDFD FDFD FDFD 0000 0000"
	$"0000 0000 00D7 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 FD00 0000"
	$"0000 0000 0000 D7D7 D7D7 D700 0000 0000"
	$"0000 00EB EBEB EBEB EBEB EB00 00FD 0000"
	$"0000 0000 0000 0000 0000 00D7 0000 0000"
	$"0000 0000 00EB 00EB 00EB 00EB 0000 FD00"
	$"0000 0000 0000 0000 0000 00D7 0000 0000"
	$"0000 0000 0000 0000 EB00 EB00 EB00 00FD"
	$"0000 0000 0000 0000 0000 D700 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 00FD"
	$"0000 0000 0000 0000 0000 D700 0000 0000"
	$"0000 0000 0000 00FD FDFD FDFD FDFD FD00"
	$"0000 0000 0000 0000 0000 00D7 D700 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 00D7 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"FDFD 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 00FD"
	$"0000 FD00 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 FD00"
	$"002F 00FD 0000 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"2F00 0000 FD00 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 0000 FD00 0000 0000 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 00FD 0000 0000 0000 0000 0000 0000"
};

resource 'dctb' (200, preload) {
	{
	},
	{
	}
};

resource 'STR#' (200, preload) {
	{
		"Name:",
		"Password:"
	}
};

resource 'DLOG' (201, preload) {
	{0, 0, 178, 232}, dBoxProc, invisible, noGoAway, 0, 201,
	""
};

resource 'DITL' (201, preload) {
	{
		{146, 150, 166, 220}, Button {enabled, "OK"},
		{146, 66, 166, 136}, Button {enabled, "Cancel"},
		{43, 153, 59, 213}, EditText {enabled, ""},
		{75, 153, 91, 213}, EditText {enabled, ""},
		{107, 153, 123, 213}, EditText {enabled, ""},
		{8, 8, 24, 224}, StaticText {disabled, "Change your access password."},
		{43, 12, 59, 146}, UserItem {disabled},
		{75, 12, 91, 146}, UserItem {disabled},
		{107, 12, 123, 146}, UserItem {disabled},
		{142, 146, 170, 224}, UserItem {disabled},
		{0, 0, 0, 0}, HelpItem {disabled, HMScanhdlg {1000}}
	}
};

resource 'dctb' (201, preload) {
	{
	},
	{
	}
};

resource 'STR#' (201, preload) {
	{
		"Old password:",
		"New password:",
		"Retype new password:"
	}
};

resource 'DLOG' (5000, preload) {
	{0, 0, 272, 442}, dBoxProc, invisible, noGoAway, 0, 5000,
	""
};

resource 'DITL' (5000, preload) {
	{
		{244, 363, 264, 433}, Button {enabled, "OK"},
		{244, 279, 264, 349}, Button {enabled, "Cancel"},
		{2, 5, 18, 437}, UserItem {disabled},
		{20, 5, 36, 216}, UserItem {disabled},
		{20, 226, 36, 437}, UserItem {disabled},
		{39, 5, 233, 216}, UserItem {disabled},
		{39, 226, 233, 437}, UserItem {disabled},
		{240, 359, 268, 437}, UserItem {disabled}
	}
};

resource 'dctb' (5000, preload) {
	{
	},
	{
	}
};

resource 'STR#' (5000, preload) {
	{
		"Choose a conference server:",
		"AppleTalk Zones",
		"Conference Servers"
	}
};

resource 'DLOG' (5001, preload) {
	{0, 0, 299, 250}, dBoxProc, invisible, noGoAway, 0, 5001,
	""
};

resource 'DITL' (5001, preload) {
	{
		{270, 171, 290, 241}, Button {enabled, "OK"},
		{270, 87, 290, 157}, Button {enabled, "Cancel"},
		{271, 3, 289, 80}, CheckBox {enabled, "Private"},
		{24, 8, 40, 242}, EditText {enabled, "Untitled"},
		{46, 5, 62, 245}, StaticText {disabled, "Select Users to Invite:"},
		{2, 5, 18, 245}, StaticText {disabled, "Name the Conference:"},
		{65, 5, 259, 245}, UserItem {disabled},
		{266, 167, 294, 245}, UserItem {disabled},
		{0, 0, 0, 0}, HelpItem {disabled, HMScanhdlg {1003}}
	}
};

resource 'dctb' (5001, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (202, preload) {
	{0, 0, 255, 250}, dBoxProc, invisible, noGoAway, 0, 202,
	""
};

resource 'DITL' (202, preload) {
	{
		{226, 171, 246, 241}, Button {enabled, "Join"},
		{226, 87, 246, 157}, Button {enabled, "Cancel"},
		{2, 5, 18, 245}, StaticText {disabled, "Select a Conference:"},
		{21, 5, 215, 245}, UserItem {disabled},
		{222, 167, 250, 245}, UserItem {disabled},
		{0, 0, 0, 0}, HelpItem {disabled, HMScanhdlg {1001}}
	}
};

resource 'dctb' (202, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (5002, preload) {
	{0, 0, 107, 341}, dBoxProc, invisible, noGoAway, 0, 5002,
	""
};

resource 'DITL' (5002, preload) {
	{
		{74, 264, 94, 328}, Button {enabled, "OK"},
		{74, 187, 94, 251}, Button {enabled, "Cancel"},
		{13, 23, 45, 55}, Icon {disabled, 1},
		{13, 78, 61, 328}, StaticText {disabled,
			"You have been invited to the conference “^0” by “^1”."},
		{70, 260, 98, 332}, UserItem {disabled}
	}
};

resource 'dctb' (5002, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (5003, preload) {
	{0, 0, 255, 250}, dBoxProc, invisible, noGoAway, 0, 5003,
	""
};

resource 'DITL' (5003, preload) {
	{
		{226, 171, 246, 241}, Button {enabled, "OK"},
		{226, 87, 246, 157}, Button {enabled, "Cancel"},
		{2, 5, 18, 245}, StaticText {disabled, "Select Users To Invite:"},
		{21, 5, 215, 245}, UserItem {disabled},
		{222, 167, 250, 245}, UserItem {disabled},
		{0, 0, 0, 0}, HelpItem {disabled, HMScanhdlg {1004}}
	}
};

resource 'dctb' (5003, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (5004, preload) {
	{0, 0, 255, 250}, dBoxProc, invisible, noGoAway, 0, 5004,
	""
};

resource 'DITL' (5004, preload) {
	{
		{226, 171, 246, 241}, Button {enabled, "OK"},
		{2, 5, 18, 245}, StaticText {disabled, "Member List:"},
		{21, 5, 215, 245}, UserItem {disabled},
		{222, 167, 250, 245}, UserItem {disabled}
	}
};

resource 'dctb' (5004, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (204, preload) {
	{0, 0, 91, 341}, dBoxProc, invisible, noGoAway, 0, 204,
	""
};

resource 'DITL' (204, preload) {
	{
		{58, 264, 78, 328}, Button {enabled, "OK"},
		{58, 187, 78, 251}, Button {enabled, "Cancel"},
		{13, 23, 45, 55}, Icon {disabled, 0},
		{13, 78, 45, 328}, StaticText {disabled,
			"Are you sure you want to disconnect from the server?"},
		{54, 260, 82, 332}, UserItem {disabled}
	}
};

resource 'dctb' (204, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (205, preload) {
	{0, 0, 91, 341}, dBoxProc, invisible, noGoAway, 0, 205,
	""
};

resource 'DITL' (205, preload) {
	{
		{58, 264, 78, 328}, Button {enabled, "OK"},
		{13, 23, 45, 55}, Icon {disabled, 0},
		{13, 78, 45, 328}, StaticText {disabled, "^0"},
		{54, 260, 82, 332}, UserItem {disabled}
	}
};

resource 'dctb' (205, preload) {
	{
	},
	{
	}
};

resource 'STR#' (205, preload) {
	{
		"Sorry, your ChitChat software is not compatible with this server.",
		"Sorry, your name is incorrect. Please reenter it.",
		"Sorry, your password is incorrect. Please reenter it.",
		"Sorry, your account is already in use.",
		"Sorry, the server is full. Please try again later."
	}
};

resource 'DLOG' (206, preload) {
	{0, 0, 107, 341}, dBoxProc, invisible, noGoAway, 0, 206,
	""
};

resource 'DITL' (206, preload) {
	{
		{74, 264, 94, 328}, Button {enabled, "OK"},
		{13, 23, 45, 55}, Icon {disabled, 0},
		{13, 78, 61, 328}, StaticText {disabled, "^0"},
		{70, 260, 98, 332}, UserItem {disabled}
	}
};

resource 'dctb' (206, preload) {
	{
	},
	{
	}
};

resource 'STR#' (206, preload) {
	{
		"Sorry, this conference no longer exists.",
		
		"Sorry, your password is incorrect.",
		
#if !DEMO
		"Your connection to the server has been disconnected."
#else
		"Your 4 hour demo has expired. Please contact Mainstay to purchase the full version."
#endif
	}
};

resource 'DLOG' (207, preload) {
	{0, 0, 118, 250}, dBoxProc, invisible, noGoAway, 0, 207,
	""
};

resource 'DITL' (207, preload) {
	{
		{89, 171, 109, 241}, Button {enabled, "OK"},
		{89, 87, 109, 157}, Button {enabled, "Cancel"},
		{27, 5, 45, 245}, CheckBox {enabled, "Show splash-screen at startup"},
		{45, 5, 63, 245}, CheckBox {enabled, "Auto-Logon"},
		{63, 5, 81, 245}, CheckBox {enabled, "Show tracking window"},
		{2, 5, 18, 245}, UserItem {disabled},
		{22, 5, 23, 245}, UserItem {disabled},
		{85, 167, 113, 245}, UserItem {disabled},
		{0, 0, 0, 0}, HelpItem {disabled, HMScanhdlg {1002}}
	}
};

resource 'dctb' (207, preload) {
	{
	},
	{
	}
};

resource 'STR#' (207, preload) {
	{
		"Preferences"
	}
};

resource 'DLOG' (208, preload) {
	{0, 0, 220, 333}, dBoxProc, invisible, noGoAway, 0, 208,
	""
};

resource 'DITL' (208, preload) {
	{
		{191, 254, 211, 324}, Button {enabled, "Done"},
		{26, 254, 46, 324}, Button {enabled, "New…"},
		{56, 254, 76, 324}, Button {enabled, "Change…"},
		{86, 254, 106, 324}, Button {enabled, "Rename…"},
		{116, 254, 136, 324}, Button {enabled, "Delete"},
		{2, 5, 18, 328}, StaticText {disabled, "Macro Editing:"},
		{21, 5, 215, 245}, UserItem {disabled},
		{187, 250, 215, 328}, UserItem {disabled}
	}
};

resource 'dctb' (208, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (300, preload) {
	{0, 0, 271, 250}, dBoxProc, invisible, noGoAway, 0, 300,
	""
};

resource 'DITL' (300, preload) {
	{
		{242, 171, 262, 241}, Button {enabled, "OK"},
		{242, 87, 262, 157}, Button {enabled, "Cancel"},
		{24, 8, 40, 242}, EditText {enabled, ""},
		{68, 8, 228, 242}, EditText {enabled, ""},
		{2, 5, 18, 245}, StaticText {disabled, "Name:"},
		{46, 5, 62, 245}, StaticText {disabled, "Text:"},
		{238, 167, 266, 245}, UserItem {disabled}
	}
};

resource 'dctb' (300, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (301, preload) {
	{0, 0, 227, 250}, dBoxProc, invisible, noGoAway, 0, 301,
	""
};

resource 'DITL' (301, preload) {
	{
		{198, 171, 218, 241}, Button {enabled, "OK"},
		{198, 87, 218, 157}, Button {enabled, "Cancel"},
		{24, 8, 184, 242}, EditText {enabled, ""},
		{2, 5, 18, 245}, StaticText {disabled, "Change Text:"},
		{194, 167, 222, 245}, UserItem {disabled}
	}
};

resource 'dctb' (301, preload) {
	{
	},
	{
	}
};

resource 'DLOG' (302, preload) {
	{0, 0, 83, 250}, dBoxProc, invisible, noGoAway, 0, 302,
	""
};

resource 'DITL' (302, preload) {
	{
		{54, 171, 74, 241}, Button {enabled, "OK"},
		{54, 87, 74, 157}, Button {enabled, "Cancel"},
		{24, 8, 40, 242}, EditText {enabled, ""},
		{2, 5, 18, 245}, StaticText {disabled, "Change Name:"},
		{50, 167, 78, 245}, UserItem {disabled}
	}
};

resource 'dctb' (302, preload) {
	{
	},
	{
	}
};

resource 'STR#' (301, preload) {
	{
		"ChitChat Preferences"
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
	512 * 1024,			/* 512 */
	256 * 1024			/* 256 */
};

resource 'vers' (1, purgeable) {
	0x01, 0x01, beta, 0x01, verUS,
	"1.0.1b1",
	"1.0.1b1, ©1996-97 Mainstay"
};

resource 'vers' (2, purgeable) {
	0x01, 0x01, release, 0x00, verUS,
	"1.0.1",
	"ChitChat™ v1.0.1"
};

type 'TMP2' as 'STR ';

resource 'TMP2' (0, purgeable) {
	"ChitChat™ v1.0.1b1 -- 2/8/97"
};

resource 'BNDL' (128, purgeable) {
	'TMP2', 0,
	{
		'ICN#', {0, 128},
		'FREF', {0, 128}
	}
};

resource 'FREF' (128, purgeable) {
	'APPL', 0,
	""
};

