// Alternative enabler that interacts
// with filesystems via win32 fileapi
#include <stdio.h>
#include <windows.h>

enum FsType { FAT16 = 0, FAT32 = 1, EXFAT = 2 };

#define SIZE 512

char flag1[] = "EOS_DEVELOP";
char flag2[] = "BOOTDISK";

char bootsector[SIZE];
DWORD bytesRead = 0;

int getFilesystem(HANDLE d)
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	if (!strcmp(bootsector + 54, "FAT16   ", 8)) {
		return FAT16;
	}

	if (!strcmp(bootsector + 82, "FAT32   ", 8)) {
		return FAT32;
	}

	if (!strncmp(bootsector + 82, "EXFAT   ", 8)) {
		return EXFAT;
	}

	return 0;
}

void setBoot(HANDLE d, long of1, long of2)
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	printf("Current Flag 1: %s\n", bootsector + of1);
	printf("Current Flag 2: %s\n", bootsector + of2);

	memcpy(bootsector + of1, "EOS_DEVELOP", 11);
	memcpy(bootsector + of2, "BOOTDISK", 8);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	WriteFile(d, bootsector, SIZE, &bytesRead, NULL);
}

int getDrive() {
	char id;

	char command[128];

	// List info usb type mounted filesystems
	FILE *f = popen("wmic logicaldisk where drivetype=2 get deviceid, volumename", "r");

	// Skip first line (title)
	fgets(command, 64, f);

	// Look for EOS_DIGITAL drive
	while (fgets(command, 64, f) != NULL) {
		if (!strncmp(command + 10, "EOS_DIGITAL", 11)) {
			printf("Found EOS_DIGITAL at drive %c\n", buffer[0]);
			id = command[0];
			goto found;
		}
	}

	puts("Could not find drive.");
	return -1;

found:;
	if (id == 'C' || id == 'c') {
		puts("Somehow I got the C drive, and I ain't writing to it.");
		return -1;
	}
	
	return (int)id;
}

int getUsableDrive(char buffer[]) {
	int drive = getDrive();
	if (drive == -1) {
		return 1;
	}

	strcpy(buffer, "X:\\");
	buffer[0] = (char)drive;
	return 0;
}

int writeFlags()
{
	char buffer[64] = "\\\\.\\E:";
	int drive = getDrive(buffer);
	if (drive == -1) {
		return 1;
	}

	buffer[4] = (char)drive;

	HANDLE d = CreateFile(driveID, GENERIC_READ | GENERIC_WRITE,
			      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			      FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);

	if (d == INVALID_HANDLE_VALUE) {
		puts("Could not open filesystem. Try running as Administrator.");
		return 1;
	}

	int drive = getFilesystem(d);

	if (drive == EXFAT) {
		puts("Quitting, no EXFAT support yet. Try EOSCARD.");
		return 1;
	} else if (drive == FAT16) {
		puts("Writing to FAT16 filesystem.");
		setBoot(d, 71, 92);
	} else if (drive == FAT32) {
		puts("Writing to FAT32 filesystem.");
		setBoot(d, 71, 92);
	} else {
		puts("Unsupported FS");
		return 1;
	}

	puts("Wrote EOS_DEVELOP AND BOOTDISK.");
	puts("Unmount the device to save changes.");
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
