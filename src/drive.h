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

	FLAG_DESTROY_BOOT,
	FLAG_DESTROY_SCRIPT
};

enum FlagsErr {
	DRIVE_UNSUPPORTED = 1,
	DRIVE_NOT_AVAILABLE = 2,
};

static char flag_develop[] = "EOS_DEVELOP";
static char flag_bootdisk[] = "BOOTDISK";
static char flag_script[] = "SCRIPT";

// Get a usable drive directory, for
// making files and stuff
int flag_usable_drive(char buffer[]);

int flag_write_flag(int mode);
void flag_close();

// TODO: flag_closefs
int flag_openfs();
void flag_write(long offset, char string[]);

#endif