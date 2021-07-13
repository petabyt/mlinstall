// Common filesystem drive code for Unix

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

	return 0;
}

void flag_write(long offset, char string[])
{
	char buffer[64] = {0};
	
	fseek(d, offset, SEEK_SET);
	fread(buffer, 1, 11, d);
	printf("Current Flag: %s\n", buffer);

	fseek(d, offset, SEEK_SET);
	fwrite(string, 1, 11, d);
}

void flag_getdrive(char buffer[]) {
	// Get EOS_DIGITAL Drive
	FILE *c = popen("mount | grep EOS_DIGITAL | awk '{printf $1}'", "r");
	fgets(buffer, 50, c);
}

int flag_usable_drive(char buffer[]) {
	char filesystem[64];
	flag_getdrive(filesystem);

	char command[256];
	sprintf(
		command,
		"cat /proc/mounts | grep %s | awk '{printf $2'}",
		filesystem
	);

	FILE *c = popen(command, "r");
	fgets(buffer, 50, c);
}

int flag_openfs() {
	char buffer[128];
	flag_getdrive(buffer);
	FILE *d = fopen(buffer, "rw+");
	
	if (!d) {
		puts("Could not open filesystem.");
		return -1;
	}

	return flag_getfs();
}