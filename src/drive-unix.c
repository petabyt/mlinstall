// Common filesystem drive code for Linux/Darwin
// Untested on Mac OS

#ifdef WIN32
	#error "not windows code"
#endif

// Uses shell commands: mount cat grep awk
// some GNU coreutils seem to do the same thing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

#include "mlinstall.h"
#include "exfat.h"
#include "drive.h"

FILE *d;
int drive_getfs(void) {
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

void flag_write(long int offset, char string[]) {
	char buffer[64] = { 0 };

	fseek(d, offset, SEEK_SET);
	fread(buffer, 1, strlen(string), d);

	// Sanitize the string
	buffer[sizeof(buffer) - 1] = '\0';
	for (int i = 0; i < sizeof(buffer); i++) {
		if (!(i >= 'A' && i <= 'Z' || i == '_')) {
			buffer[i] = '\0';
		}
	}

	log_print("%s\t-> %s", buffer, string);

	fseek(d, offset, SEEK_SET);
	if (fwrite(string, 1, strlen(string), d) != strlen(string)) {
		puts("Error writing to drive.");
		return;
	}

	if (drive_getfs() == EXFAT) {
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

int drive_get(char buffer[], int n) {
	// Get EOS_DIGITAL Drive
	FILE *c = popen("mount | grep EOS_DIGITAL", "r");
	fgets(buffer, n, c);

	strtok(buffer, " "); // Get first token before space

	// Check if extracted result is valid
	if (strncmp(buffer, "/dev/", 5) != 0) {
		log_print("Make sure your EOS_DIGITAL card is mounted.");
		return DRIVE_NONE;
	}

	return 0;
}

int drive_get_usable(char buffer[], int n) {
	char filesystem[64];
	if (drive_get(filesystem, sizeof(filesystem))) {
		return DRIVE_NONE;
	}

	char command[256];
	sprintf(command, "cat /proc/mounts | grep %s | awk '{printf $2'}", filesystem);
	if (command[1] == '\0') {
		puts("Couldn't find the drive mounted.");
		return DRIVE_NONE;
	}

	FILE *c = popen(command, "r");
	fgets(buffer, n, c);

	return 0;
}

int darwin_unmount(char *path) {
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "diskutil unmount %s", path);
	return system(buffer);
}

int drive_openfs(void) {
	if (geteuid() != 0) {
		log_print("We needs sudo permissions. (sudo ./mlinstall)");
		return DRIVE_ERROR;
	}

	char buffer[128];
	drive_get(buffer, 128);
	d = fopen(buffer, "rw+");

	if (d == NULL) {
		return DRIVE_ERROR;
	}

	char drive[128];
	drive_get_usable(drive, sizeof(drive));
#ifdef __APPLE__
	int rc = darwin_unmount(drive);
#else
	int rc = umount(drive);
#endif
	if (rc) {
		puts("Error unmounting drive.");
		return DRIVE_ERROR;
	} else {
		puts("Unmounted drive.");
	}

	return 0;
}

void drive_close(void) {
	fclose(d);
}

void update_exfat(void) {
	unsigned int buffer[EXFAT_VBR_SIZE + 512];

	fread(buffer, 1, EXFAT_VBR_SIZE + 512, d);
	unsigned int sum = VBRChecksum((unsigned char *)buffer, EXFAT_VBR_SIZE);
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
	char *dump = malloc(TEMP_DUMP_SIZE);
	fseek(d, 0, SEEK_SET);
	fread(dump, 1, TEMP_DUMP_SIZE, d);

	FILE *f = fopen(name, "w");
	fwrite(dump, 1, TEMP_DUMP_SIZE, f);
	fclose(f);
	free(dump);
}
