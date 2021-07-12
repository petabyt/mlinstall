// Unix Utility to enable
// boot disk on EOS_DIGITAL card
#include <stdio.h>
#include <string.h>

enum FsType { FAT16 = 0, FAT32 = 1, EXFAT = 2 };

char flag1[] = "EOS_DEVELOP";
char flag2[] = "BOOTDISK";

int getFilesystem(FILE *d)
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

void setBoot(FILE *d, long of1, long of2)
{
	char buffer[64];

	memset(buffer, '\0', sizeof(buffer));
	fseek(d, of1, SEEK_SET);
	fread(buffer, 1, 11, d);
	printf("Current Flag 1: %s\n", buffer);

	memset(buffer, '\0', sizeof(buffer));
	fseek(d, of2, SEEK_SET);
	fread(buffer, 1, 8, d);
	printf("Current Flag 2: %s\n", buffer);

	fseek(d, of1, SEEK_SET);
	fwrite(flag1, 1, 11, d);

	fseek(d, of2, SEEK_SET);
	fwrite(flag2, 1, 8, d);

	printf("Wrote %s and %s.\n", flag1, flag2);
}

void getDrive(char buffer[]) {
	// Get EOS_DIGITAL Drive
	FILE *c = popen("mount | grep EOS_DIGITAL | awk '{printf $1}'", "r");
	fgets(buffer, 50, c);
}

int getUsableDrive(char buffer[]) {
	char filesystem[64];
	getDrive(filesystem);

	char command[256];
	sprintf(
		command,
		"cat /proc/mounts | grep %s | awk '{printf $2'}",
		filesystem
	);

	FILE *c = popen(command, "r");
	fgets(buffer, 50, c);
}

// Detect the filesystem and write the flags
// in the correct place
int writeFlags()
{
	char buffer[128];
	getDrive(buffer);
	FILE *d = fopen(buffer, "rw+");
	
	if (!d) {
		puts("Could not open filesystem.");
		return 1;
	}

	char *fsNames[] = { "FAT16", "FAT32", "EXFAT" };
	int drive = getFilesystem(d);

	printf("FS Type: %s\n", fsNames[drive]);

	if (drive == EXFAT) {
		puts("Quitting, no EXFAT support. Try EOSCARD.");
		fclose(d);
		return 1;
	} else if (drive == FAT16) {
		setBoot(d, 71, 92);
	} else if (drive == FAT32) {
		setBoot(d, 71, 92);
	} else {
		puts("Unsupported FS");
		fclose(d);
		return 1;
	}

	puts("Wrote card flags.");
	puts("Unmount the device to save changes.");
	fclose(d);
	return 0;
}

// Disable the flag by writing underscores
// on the first character
int disableFlag()
{
	flag1[0] = '_';
	flag2[0] = '_';
	if (writeFlags()) {
		return 1;
	}

	flag1[0] = 'E';
	flag2[0] = 'B';
	return 0;
}

int enableFlag()
{
	if (writeFlags()) {
		return 1;
	}

	return 0;
}

#ifdef TEST
int main()
{
	char buffer[128];
	getUsableDrive(buffer);
	puts(buffer);
}
#endif