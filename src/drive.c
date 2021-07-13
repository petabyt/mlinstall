#include <stdio.h>
#include "drive.h"

// Detect the filesystem and write the flags
// in the correct place
int flags_write(int mode)
{
	char *fsNames[] = { "FAT16", "FAT32", "EXFAT" };

	int drive = flag_openfs();
	if (drive == -1) {
		return 1;
	}

	printf("FS Type: %s\n", fsNames[drive]);

	int of[3] = { 0, 0, 0 };

	if (drive == EXFAT) {
		puts("Quitting, no EXFAT support yet. Try EOSCARD.");
		return 1;
	} else if (drive == FAT16) {
		puts("Writing to FAT16 filesystem.");
		of[0] = 0x2b;
		of[1] = 0x40;
		of[2] = 0x1f0;
	} else if (drive == FAT32) {
		puts("Writing to FAT32 filesystem.");
		of[0] = 0x2b;
		of[1] = 0x40;
		of[2] = 0x1f0;
	} else {
		puts("Unsupported FS");
		return 1;
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
		flag_script[0] = '_';

		flag_write(of[0], flag_develop);
		flag_write(of[1], flag_bootdisk);

		flag_develop[0] = 'E';
		flag_bootdisk[0] = 'B';
		flag_script[0] = 'S';
	}

	puts("Wrote card flags.");
	puts("Unmount the device to save changes.");
	return 0;
}