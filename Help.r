/* Client help resource file */

#include "SysTypes.r"
#include "Types.r"
#include "BalloonTypes.r"

resource 'hfdr' (kHMHelpID, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	{
		HMSTRResItem {
			1000
		}
	}
};

resource 'STR ' (1000, purgeable) {
	"ChitChat\n"
	"\n"
	"This application allows you to connect to a ChitChat server and communicate with the "
	"other people connected to it."
};

resource 'hmnu' (128, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			128, 1,
			0, 0,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (128, purgeable) {
	{
		"Displays information about ChitChat."
	}
};

resource 'hmnu' (129, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			129, 1,
			129, 2,
			0, 0,
			0, 0
		},
		HMStringResItem {
			129, 3,
			129, 4,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			129, 5,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			129, 6,
			129, 7,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			129, 8,
			0, 0,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			129, 9,
			0, 0,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (129, purgeable) {
	{
		"Closes the active window.",
		
		"Closes the active window. Not available because no window is active.",
		
		"Displays a dialog box which allows you to save the contents of the active window.",
		
		"Displays a dialog box which allows you to save the contents of the active window. "
		"Not available because no window is active.",
		
		"Displays a dialog box which allows you to set paper size, orientation and other "
		"printing options.",
		
		"Displays a dialog box which allows you to print the contents of the active window.",
		
		"Displays a dialog box which allows you to print the contents of the active window. "
		"Not available because no window is active.",
		
		"Displays a dialog box which allows you to set your preferences.",
		
		"Disconnects from the server and quits the application."
	}
};

resource 'hmnu' (130, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			130, 1,
			130, 2,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			130, 3,
			130, 4,
			0, 0,
			0, 0
		},
		HMStringResItem {
			130, 5,
			130, 6,
			0, 0,
			0, 0
		},
		HMStringResItem {
			130, 7,
			130, 8,
			0, 0,
			0, 0
		},
		HMStringResItem {
			130, 9,
			130, 10,
			0, 0,
			0, 0
		},
		HMStringResItem {
			130, 11,
			130, 12,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (130, purgeable) {
	{
		"Undoes your last action if it involved cutting, clearing, pasting or typing.",
		
		"Undoes your last action if it involved cutting, clearing, pasting or typing. "
		"Not available because there is no active window or your last action cannot be "
		"undone.",
		
		"Removes the selected text or graphics and temporarily places it into a storage "
		"area called the Clipboard.",
		
		"Removes the selected text or graphics and temporarily places it into a storage "
		"area called the Clipboard. Not available because there is no active window or "
		"nothing is selected.",
		
		"Copies the selected text or graphics. The original selection remains where it is. "
		"The copy is temporarily placed into a storage area called the Clipboard.",
		
		"Copies the selected text or graphics. The original selection remains where it is. "
		"The copy is temporarily placed into a storage area called the Clipboard. Not "
		"available because there is no active window or nothing is selected.",
		
		"Inserts the contents of the Clipboard at the location of the insertion point.",
		
		"Inserts the contents of the Clipboard at the location of the insertion point. "
		"Not available because there is no active window, there is nothing on the Clipboard "
		"or the contents of the Clipboard are of an incompatible type.",
		
		"Removes the selected text or graphics without storing it on the Clipboard.",
		
		"Removes the selected text or graphics without storing it on the Clipboard. "
		"Not available because there is no active window or nothing is selected.",
		
		"Selects the entire contents of the active window.",
		
		"Selects the entire contents of the active window. Not available because there is "
		"no active window or there is nothing to select."
	}
};

resource 'hmnu' (131, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			131, 1,
			131, 2,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			131, 3,
			131, 4,
			0, 0,
			0, 0
		},
		HMStringResItem {
			131, 5,
			131, 6,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			131, 7,
			131, 8,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (131, purgeable) {
	{
		"Displays a dialog box which allows you to connect to a server.",
		
		"Displays a dialog box which allows you to connect to a server. Not available "
		"because you are already connected.",
		
		"Displays a dialog box which allows you to create a new conference and invite people "
		"to it.",
		
		"Displays a dialog box which allows you to create a new conference and invite people "
		"to it. Not available because you are not connected to a server.",
		
		"Displays a dialog box which allows you to join an existing conference.",
		
		"Displays a dialog box which allows you to join an existing conference. Not available "
		"because you are not connected to a server.",
		
		"Displays a dialog box which allows you to change your password.",
		
		"Displays a dialog box which allows you to change your password. Not available "
		"because you are not connected to a server.",
	}
};

resource 'hmnu' (132, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			132, 1,
			0, 0,
			132, 2,
			0, 0
		},
		HMStringResItem {
			132, 3,
			0, 0,
			132, 4,
			0, 0
		},
		HMStringResItem {
			132, 5,
			0, 0,
			132, 6,
			0, 0
		},
		HMStringResItem {
			132, 7,
			0, 0,
			132, 8,
			0, 0
		}
	}
};

resource 'STR#' (132, purgeable) {
	{
		"Choose this item for direct connection via Ethernet.",
		
		"Choose this item for direct connection via Ethernet. Checked because this is the "
		"option currently used.",
		
		"Choose this item for direct connection via LocalTalk.",
		
		"Choose this item for direct connection via LocalTalk. Checked because this is the "
		"option currently used.",
		
		"Choose this item for connection via Modem at 28.8 Kbps.",
		
		"Choose this item for connection via Modem at 28.8 Kbps. Checked because this is the "
		"option currently used.",
		
		"Choose this item for connection via Modem at 14.4 Kbps or lower.",
		
		"Choose this item for connection via Modem at 14.4 Kbps or lower. Checked because "
		"this is the option currently used."
	}
};

resource 'hmnu' (150, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMSkipItem {
		},
		HMStringResItem {
			150, 1,
			0, 0,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (150, purgeable) {
	{
		"Displays a dialog box which allows you to edit the list of text macros."
	}
};

resource 'hdlg' (1000, purgeable) {
	HelpMgrVersion, 0, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1000, 1,
			1000, 2,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1000, 3,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1000, 4,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1000, 5,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1000, 6,
			0, 0,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (1000, purgeable) {
	{
		"To change your password, click this button (or press the Return or Enter key).",
		
		"To change your password, click this button. Not available because one of "
		"the fields is empty or the new passwords don’t match.",
		
		"To close the dialog box without changing your password, click this button.",
		
		"Type your old password in this box.",
		
		"Type your new password in this box.",
		
		"Type your new password in this box for verification."
	}
};

resource 'hdlg' (1001, purgeable) {
	HelpMgrVersion, 0, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1001, 1,
			1001, 2,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1001, 3,
			0, 0,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			0, 0,
			1001, 4,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (1001, purgeable) {
	{
		"To join the selected conference, click this button (or press the Return or Enter key).",
		
		"To join a conference, click this button. Not available because no conference is "
		"selected.",
		
		"To close the dialog box without joining a conference, click this button.",
		
		"Select the conference you wish to join from this list."
	}
};

resource 'hdlg' (1002, purgeable) {
	HelpMgrVersion, 0, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1002, 1,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1002, 2,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1002, 3,
			0, 0,
			1002, 4,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1002, 5,
			0, 0,
			1002, 6,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1002, 7,
			0, 0,
			1002, 8,
			0, 0
		}
	}
};

resource 'STR#' (1002, purgeable) {
	{
		"To save your changes, click this button (or press the Return or Enter key).",
		
		"To close the dialog box without saving your changes, click this button.",
		
		"To have the splash screen displayed at startup, check this box.",
		
		"To not have the splash screen displayed at startup, uncheck this box.",
		
		"To automatically log on to the server, check this box.",
		
		"To manually log on to the server, uncheck this box.",
		
		"To have the Tracking window displayed, check this box.",
		
		"To not have the Tracking window displayed, uncheck this box."
	}
};

resource 'hdlg' (1003, purgeable) {
	HelpMgrVersion, 0, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1003, 1,
			1003, 2,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1003, 3,
			0, 0,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1003, 4,
			0, 0,
			1003, 5,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1003, 6,
			0, 0,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMSkipItem {
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			0, 0,
			1003, 7,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (1003, purgeable) {
	{
		"To create the conference and invite the people, click this button (or press the "
		"Return or Enter key).",
		
		"To create the conference and invite the people, click this button. Not available "
		"because you have not provided a name for the conference.",
		
		"To close the dialog box without creating a conference, click this button.",
		
		"To create a private conference, check this box.",
		
		"To create a public conference, uncheck this box.",
		
		"Type a name for the conference in this box.",
		
		"Select the people you wish to invite from this list."
	}
};

resource 'hdlg' (1004, purgeable) {
	HelpMgrVersion, 0, hmDefaultOptions, 0, 0,
	HMSkipItem {
	},
	{
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1004, 1,
			1004, 2,
			0, 0,
			0, 0
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			1004, 3,
			0, 0,
			0, 0,
			0, 0
		},
		HMSkipItem {
		},
		HMStringResItem {
			{0, 0}, {0, 0, 0, 0},
			0, 0,
			1004, 4,
			0, 0,
			0, 0
		}
	}
};

resource 'STR#' (1004, purgeable) {
	{
		"To invite the people to the conference, click this button (or press the Return "
		"or Enter key).",
		
		"To invite the people to the conference, click this button. Not available because "
		"nobody is selected.",
		
		"To close the dialog without inviting anybody, click this button.",
		
		"Select the people you wish to invite from this list."
	}
};

resource 'hwin' (1000, purgeable) {
	HelpMgrVersion, hmDefaultOptions,
	{
		1005, 'hrct', -12, ""
	}
};

resource 'hrct' (1005, purgeable) {
	HelpMgrVersion, hmDefaultOptions, 0, 0,
	{
		HMStringResItem {
			{0, 0}, {5, 5, 30, 30},
			1005, 1
		},
		HMStringResItem {
			{0, 0}, {5, 35, 30, 60},
			1005, 2
		},
		HMStringResItem {
			{0, 0}, {5, 65, 30, 90},
			1005, 3
		},
		HMStringResItem {
			{0, 0}, {5, 95, 30, 120},
			1005, 4
		},
		HMStringResItem {
			{0, 0}, {5, 125, 30, 150},
			1005, 5
		},
		HMStringResItem {
			{0, 0}, {5, 155, 30, 180},
			1005, 6
		},
		HMStringResItem {
			{0, 0}, {5, 210, 30, 235},
			1005, 7
		}
	}
};

resource 'STR#' (1005, purgeable) {
	{
		"Displays a dialog box which allows you to save the contents of this window.",
		
		"Displays a dialog box which allows you to print the contents of this window.",
		
		"Displays a dialog box which lists the names of the people in this conference.",
		
		"Displays a dialog box which allows you to invite people to this conference.",
		
		"Opens a graphics window connected to this conference.",
		
		"Sends a text macro or displays a dialog box which allows you to edit the list "
		"of text macros.",
		
		"Click and hold this button while speaking in the microphone."
	}
};

