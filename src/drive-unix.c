// Common filesystem drive code for Unix
// fopen/fwrite/fseek Works on Windows,
// just but won't write to the drive, so we
// have to rewrite everything :)

// This is for non-windows. I think it should
// work on Linux and MacOS (?)
#ifndef WIN32

// Uses shell commands: mount cat grep awk

#include <stdio.h>
#include <string.h>

#include "drive.h"

FILE *d;
int flag_getfs()
{
	char buffer[50];

	memset(buffer, '\0', sizeof(buffer));
	fseek(d, 54, SEEK_SET);
	fread(buffer, 1, 8, d);
	if (!strcmp(buffer, "FAT16   ")) {
		return FAT16;
	}

	memset(buffer, '\0', sizeof(buffer));
	fseek(d, 82, SEEK_SET);
	fread(buffer, 1, 8, d);
	if (!strcmp(buffer, "FAT32   ")) {
		return FAT32;
	}

	memset(buffer, '\0', sizeof(buffer));
	fseek(d, 82, SEEK_SET);
	fread(buffer, 1, 8, d);
	if (!strcmp(buffer, "EXFAT   ")) {
		return EXFAT;
	}

	return DRIVE_BADFS;
}

void flag_write(long int offset, char string[])
{
	char buffer[64] = { 0 };

	fseek(d, offset, SEEK_SET);
	fread(buffer, 1, strlen(string), d);

	printf("Current Flag: \"%s\"\n", buffer);
	printf("Writing \"%s\" at 0x%lx\n", string, offset);

	fseek(d, offset, SEEK_SET);
	fwrite(string, 1, strlen(string), d);

	// fseek-ing seems to be updating the file
	// (required in order to apply fwrites)
	fseek(d, 0, SEEK_SET);
}

int flag_getdrive(char buffer[])
{
	// Get EOS_DIGITAL Drive
	FILE *c = popen("mount | grep EOS_DIGITAL | awk '{printf $1}'", "r");
	fgets(buffer, 64, c);

	// Check if not a drive (just carriage return)
	if (strncmp(buffer, "/dev/", 5)) {
		puts("Couldn't find a /dev/ drive.");
		puts("Make sure you are running as superuser and EOS_DIGITAL is mounted.");
		return DRIVE_NONE;
	}

	return 0;
}

int flag_usable_drive(char buffer[])
{
	char filesystem[64];
	if (flag_getdrive(filesystem)) {
		return DRIVE_NONE;
	}

	char command[256];
	sprintf(command, "cat /proc/mounts | grep %s | awk '{printf $2'}", filesystem);
	if (command[1] == '\0') {
		puts("Couldn't find the drive mounted.");
		return DRIVE_NONE;
	}

	FILE *c = popen(command, "r");
	fgets(buffer, 50, c);

	return 0;
}

int flag_openfs()
{
	char buffer[128];
	flag_getdrive(buffer);
	d = fopen(buffer, "rw+");

	if (d == NULL) {
		puts("Could not open filesystem.");
		return DRIVE_ERROR;
	}

	return flag_getfs();
}

void flag_close()
{
	fclose(d);
}

#endif
