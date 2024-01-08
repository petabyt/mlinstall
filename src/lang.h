// Cheap i18n until we need it

#ifndef ML_LANG_H
#define ML_LANG_H

#ifndef T_APP_VERSION
#define T_APP_VERSION "v1.1.0"
#endif

#define T_APP_NAME "MLinstall"
#define T_APP_SUBTITLE "Tool to help install Magic Lantern"

#define T_WELCOME_LOG ("Welcome to " T_APP_NAME)

#define T_USB "USB"
#define T_ADVANCED "Advanced"
#define T_ABOUT "About"
#define T_CARD "Card"
#define T_DEV_NOT_FOUND "Couldn't find a PTP/USB device."
#define T_DRIVE_NOT_FOUND "Couldn't find card. Make sure\nthe EOS_DIGITAL card is mounted."
#define T_DRIVE_NOT_SUPPORTED "Only ExFAT, FAT32, and FAT16\ncards are supported."
#define T_DRIVE_ERROR "Error opening drive."
#define T_DRIVE_ERROR_LINUX "Make sure to run with sudo."
#define T_WROTE_CARD_FLAGS "Wrote card flags on EOS_DIGITAL"
#define T_DESTROY_CARD_FLAGS "Overwrote card flags."
#define T_WROTE_SCRIPT_FLAGS "Wrote script flags."
#define T_DESTROYED_SCRIPT_FLAGS "Disabled script flags."
#define T_NOT_CANON_DEVICE "'%s' is not a Canon device!"
#define T_BOOT_DISK_ENABLE_FAIL "Couldn't enable boot disk."
#define T_BOOT_DISK_ENABLE_SUCCESS "Enabled boot disk"
#define T_DISABLE_BOOT_DISK_FAIL "Couldn't disable boot disk."
#define T_DISABLE_BOOT_DISK_SUCCESS "Disabled boot disk"
#define T_XML_TITLE_TEXT "<span size=\"large\">" T_APP_NAME "</span>\nTool to help install Magic Lantern"
#define T_USB_STUFF_TITLE "This will run commands on your camera\nvia PTP/USB.\n"

#define T_RETURN_CODE_OK "Success!"
#define T_RETURN_INVALID_PARAM "Error: invalid parameter."
#define T_RETURN_UNSUPPORTED "Error: operation not supported."
#define T_UNKNOWN_ERROR "Unknown error."

#define T_CONNECT "Connect to Camera"
#define T_DISCONNECT "Disconnect"
#define T_GET_DEVICE_INFO "Show Camera Info"
#define T_ENABLE_BOOT_DISK "Enable Custom Boot"
#define T_DISABLE_BOOT_DISK "Disable Custom Boot"
#define T_ABOUT_BOOTDISK "This will toggle a switch in your camera that\nallows it to boot custom software like Magic Lantern."

#define T_CARD_STUFF_TITLE "This will automatically find and write to\na card named 'EOS_DIGITAL'."
#define T_WRITE_CARD_BOOT_FLAGS "Enable Card Boot Flags"
#define T_DESTROY_CARD_BOOT_FLAGS "Disable Card Boot Flags"
#define T_MAKE_CARD_SCRIPTABLE "Enable Card Scripting Flags"
#define T_MAKE_CARD_UNSCRIPTABLE "Disable Card Scripting Flags"
#define T_DETECT_CARD "Detect EOS_DIGITAL"

#define T_DEV_MODE_WARNING "Entering the wrong command can brick your\ncamera! Only use with guidance\nfrom a ML developer!"

#define T_APP_INFO "\nMade by <a href='https://danielc.dev/'>Daniel C</a>\n" \
		"Source code: <a href='https://github.com/petabyt/mlinstall'>github.com/petabyt/mlinstall</a>\n\n" \
		"Licenced under GNU General Public License v2.0\n" \
		"If it breaks, you get to keep both pieces!"

#endif
