// Cheap i18n until we need it

#ifndef ML_LANG_H
#define ML_LANG_H

#define T_APP_NAME "MLinstall"

#define T_WELCOME_LOG ("Welcome to " T_APP_NAME)

#define T_USB "USB"
#define T_ADVANCED "Advanced"
#define T_CARD "Card"
#define T_DEV_NOT_FOUND "Couldn't find a PTP/USB device."
#define T_DRIVE_NOT_FOUND "Couldn't find card. Make sure\nthe EOS_DIGITAL card is mounted."
#define T_DRIVE_NOT_SUPPORTED "Only ExFAT, FAT32, and FAT16\ncards are supported."
#define T_DRIVE_ERROR "Error opening drive."
#define T_DRIVE_ERROR_LINUX "Make sure to run with sudo."
#define T_WROTE_CARD_FLAGS "Wrote card flags on EOS_DIGITAL"
#define T_DESTROY_CARD_FLAGS "Overwrote card flags."
#define T_WROTE_SCRIPT_FLAGS "Wrote script flags."
#define T_DESTROYED_SCRIPT_FLAGS "Destroyed script flags."
#define T_NOT_CANON_DEVICE "'%s' is not a Canon device!"
#define T_BOOT_DISK_ENABLE_FAIL "Couldn't enable boot disk."
#define T_BOOT_DISK_ENABLE_SUCCESS "Enabled boot disk"
#define T_DISABLE_BOOT_DISK_FAIL "Couldn't disable boot disk."
#define T_DISABLE_BOOT_DISK_SUCCESS "Disabled boot disk"
#define T_XML_TITLE_TEXT "<span size=\"large\">" T_APP_NAME "</span>\nTool to help install Magic Lantern"
#define T_USB_STUFF_TITLE "This will run commands on your\n" \
		"camera via USB/PTP. Make sure to run as\n" \
		"Administrator/Sudo.\n"

#define T_RETURN_CODE_OK "Return Code OK."
#define T_RETURN_INVALID_PARAM "Return Code INVALID_PARAMETER."
#define T_RETURN_UNSUPPORTED "Operation not supported. Your camera is probably unsupported."
#define T_UNKNOWN_ERROR "Unknown error."

#define T_GET_DEVICE_INFO "Get Device Info"
#define T_ENABLE_BOOT_DISK "Enable Boot Disk"
#define T_DISABLE_BOOT_DISK "Disable Boot Disk"
#define T_CARD_STUFF_TITLE "This will automatically find and write to\na card named 'EOS_DIGITAL'.\n"

#define T_WRITE_CARD_BOOT_FLAGS "Write card boot flags"
#define T_DESTROY_CARD_BOOT_FLAGS "Destroy card boot flags"
#define T_MAKE_CARD_SCRIPTABLE "Make card scriptable"
#define T_MAKE_CARD_UNSCRIPTABLE "Make card un-scriptable"
#define T_DETECT_CARD "Detect EOS_DIGITAL"

#define T_DEV_MODE_WARNING "Entering the wrong command can brick your\ncamera! Only use with guidance\nfrom a ML developer!"

#define T_APP_INFO "\nMade by <a href='https://danielc.dev/'>Daniel C</a>\n" \
		"Source code: <a href='https://github.com/petabyt/mlinstall'>github.com/petabyt/mlinstall</a>\n\n" \
		"Licenced under GNU General Public License v2.0\n" \
		"If it breaks, you get to keep both pieces!"


#endif
