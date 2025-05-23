#ifndef DRIVE_H
#define DRIVE_H

#define TEMP_DUMP_SIZE (512 * 12)

enum FsType { FAT16 = 0, FAT32 = 1, EXFAT = 2 };

enum FlagsMode {
	FLAG_ALL,
	FLAG_BOOT,
	FLAG_SCRIPT,

	FLAG_DESTROY_BOOT,
	FLAG_DESTROY_SCRIPT
};

enum FlagsErr {
	DRIVE_BADFS = -1,
	DRIVE_NONE = -2,
	DRIVE_ERROR = -3
};

int drive_get_usable(char buffer[], int n);
int drive_write_flag(int mode);
int drive_openfs(void);
int drive_getfs(void);
void drive_close(void);
void drive_dump(char name[]);

void flag_write(long offset, char string[]);
void update_exfat(void);

#endif
