// Platform independent code. Will use functions from
// either drive-win.c or drive-unix.c
#include <stdio.h>
#include "drive.h"

// Detect the filesystem and write the flags
// in the correct place
int flag_write_flag(int mode)
{
	int drive = flag_openfs();
	switch (drive) {
	case DRIVE_BADFS:
		puts("No usable filesystem on card.");
		return DRIVE_UNSUPPORTED;
	case DRIVE_NONE:
		puts("Couldn't find an EOS_DIGITAL card.");
		return DRIVE_NOT_AVAILABLE;
	case DRIVE_ERROR:
		puts("Error opening drive. Make sure you run mlinstall as Administrator or 'sudo'.");
		return DRIVE_NOT_AVAILABLE;
	}

	long int of[3] = { 0, 0, 0 };

	if (drive == EXFAT) {
		#ifdef WIN32
		puts("Quitting, no Windows EXFAT support yet. Use EOSCARD or reformat as FAT32/FAT16.");
		return DRIVE_UNSUPPORTED;
		#endif

		of[0] = 0x82;
		of[1] = 0x7a;
		of[2] = 0x1f0;
	} else if (drive == FAT16) {
		puts("Writing to FAT16 filesystem.");
		of[0] = 0x2b;
		of[1] = 0x40;
		of[2] = 0x1f0;
	} else if (drive == FAT32) {
		puts("Writing to FAT32 filesystem.");
		of[0] = 0x47;
		of[1] = 0x5c;
		of[2] = 0x1f0;
	} else {
		puts("Unsupported FS");
		return DRIVE_UNSUPPORTED;
	}

	switch (mode) {
	case FLAG_ALL:
		flag_write(of[0], flag_develop);
		flag_write(of[1], flag_bootdisk);
		flag_write(of[2], flag_script);
		break;
	case FLAG_SCRIPT:
		flag_write(of[2], flag_script);
		break;
	case FLAG_BOOT:
		flag_write(of[0], flag_develop);
		flag_write(of[1], flag_bootdisk);
		break;
	case FLAG_DESTROY_BOOT:
		flag_develop[0] = '_';
		flag_bootdisk[0] = '_';

		flag_write(of[0], flag_develop);
		flag_write(of[1], flag_bootdisk);

		flag_develop[0] = 'E';
		flag_bootdisk[0] = 'B';
		break;
	case FLAG_DESTROY_SCRIPT:
		flag_script[0] = '_';

		flag_write(of[2], flag_script);

		flag_script[0] = 'S';
		break;
	}

	#ifndef WIN32
	if (drive == EXFAT) {
		updateExFAT();
	}
	#endif

	puts("Wrote card flags.");
	puts("Unmount the device to save changes.");
	return 0;
}
