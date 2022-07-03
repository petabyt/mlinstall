// Platform independent code. Will use functions from
// either drive-win.c or drive-unix.c
#include <stdio.h>
#include "drive.h"

// Detect the filesystem and write the flags
// in the correct place
int drive_write_flag(int mode)
{
	int drive = flag_openfs();
	switch (drive) {
	case DRIVE_BADFS:
		puts("The EOS_DIGITAL card must be FAT32, FAT16, or ExFAT.");
		return DRIVE_BADFS;
	case DRIVE_NONE:
		puts("Couldn't find an EOS_DIGITAL card. Make sure the EOS_DIGITAL card is mounted.");
		return DRIVE_NONE;
	case DRIVE_ERROR:
		puts("Error opening drive.");
		return DRIVE_ERROR;
	}

	long int of[3] = { 0, 0, 0 };

	if (drive == EXFAT) {
		of[0] = 0x82;
		of[1] = 0x7a;
		of[2] = 0x1f0;
	} else if (drive == FAT16) {
		of[0] = 0x2b;
		of[1] = 0x40;
		of[2] = 0x1f0;
	} else if (drive == FAT32) {
		of[0] = 0x47;
		of[1] = 0x5c;
		of[2] = 0x1f0;
	} else {
		puts("Unsupported FS");
		return DRIVE_BADFS;
	}

	drive_dump("SD_BACKUP");

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

	if (drive == EXFAT) {
		update_exfat();
	}

	puts("Wrote card flags.");
	return 0;
}
