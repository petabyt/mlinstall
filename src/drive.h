#ifndef DRIVE_H
#define DRIVE_H

enum FsType {
	FAT16 = 0,
	FAT32 = 1,
	EXFAT = 2
};

enum FlagsMode {
	FLAG_ALL,
	FLAG_BOOT,
	FLAG_SCRIPT,

	FLAG_DESTROY_BOOT
};

static char flag_develop[] = "EOS_DEVELOP";
static char flag_bootdisk[] = "BOOTDISK";
static char flag_script[] = "SCRIPT";

// Get a usable drive directory, for
// making files and stuff
int flag_usable_drive(char buffer[]);

int flags_write(int mode);

// TODO: flag_closefs
int flag_openfs();
void flag_write(long offset, char string[]);

#endif