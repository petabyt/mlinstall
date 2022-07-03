// Common filesystem drive code for Unix
// fopen/fwrite/fseek Works on Windows,
// just but won't write to the drive, so we
// have to rewrite everything :)

// This is for non-windows. I think it should
// work on Linux and MacOS (?)
#ifndef WIN32

// Uses shell commands: mount cat grep awk
// It's technically not bad practice, some GNU coreutils
// do the same thing I think.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

#include "exfat.h"
#include "drive.h"

FILE *d;
int flag_getfs()
{
	char buffer[64];

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
	fseek(d, 3, SEEK_SET);
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
	if (fwrite(string, 1, strlen(string), d) != strlen(string)) {
		puts("Error writing to drive.");
		return;
	}

	if (flag_getfs() == EXFAT) {
		printf("Card is ExFAT, writing flags in the backup VBR.\n");
		printf("Writing \"%s\" at 0x%lx\n", string, offset + (512 * 12));
		fseek(d, offset + (512 * 12), SEEK_SET);
		fwrite(string,  1, strlen(string), d);
	}

	// fseek-ing seems to be updating the file (?)
	// fflush to be extra safe
	// (required in order to apply fwrites)
	fseek(d, 0, SEEK_SET);
	fflush(d);
}

int flag_getdrive(char buffer[])
{
	if (geteuid() != 0) {
		puts("This app must be run as superuser to modify filesystems, like: sudo ./mlinstall");
		return DRIVE_NONE;
	}

	// Get EOS_DIGITAL Drive
	FILE *c = popen("mount | grep EOS_DIGITAL", "r");
	fgets(buffer, 64, c);

	strtok(buffer, " "); // Get first token before space

	// Check if not a dev drive
	if (strncmp(buffer, "/dev/", 5)) {
		puts("Make sure your EOS_DIGITAL card is mounted.");
		return DRIVE_NONE;
	} else if (!strncmp(buffer, "/dev/sda", 8)) {
		puts("Somehow I got /dev/sda. I'm not writing to it...");
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
		return DRIVE_ERROR;
	}

	// Unmount to prevent other processes from writing to it
	char drive[128];
	flag_usable_drive(drive);
	if (umount(drive)) {
		puts("Error unmounting drive.");
		return DRIVE_ERROR;
	} else {
		puts("Unmounted drive for filesystem safety.");
	}

	return flag_getfs();
}

void flag_close()
{
	fclose(d);
}

void update_exfat()
{
	unsigned int buffer[EXFAT_VBR_SIZE + 512];

	fread(buffer, 1, EXFAT_VBR_SIZE + 512, d);
	int sum = VBRChecksum((unsigned char *)buffer, EXFAT_VBR_SIZE);
	for (int i = 0; i < 512 / 4; i++) {
		buffer[i] = sum;
	}

	// Write the VBR checksum, or as the old install script said:
	// "write VBR checksum (from sector 0 to sector 10) at offset 5632 (sector 11) and offset 11776 (sector 23, for backup VBR)
	// checksum sector is stored in $dump_file at offset 5632"
	fseek(d, 5632, SEEK_SET);
	fwrite(buffer, 1, 512, d);
	fseek(d, 11776, SEEK_SET);
	fwrite(buffer, 1, 512, d);

	fseek(d, 0, SEEK_SET);
	fflush(d);
}

void drive_dump(char name[]) {
	puts("Creating a backup of your SD card first few sectors.");

	char *dump = malloc(512 * 12);
	fseek(d, 0, SEEK_SET);
	fread(dump, 1, 512 * 12, d);

	FILE *f = fopen(name, "w");
	fwrite(dump, 1, 512 * 12, f);
	fclose(f);
}

#endif
