// Windows fileapi based code
#ifdef WIN32

#include <stdio.h>
#include <windows.h>

#include "drive.h"
#include "exfat.h"

// Bootsector size
#define SIZE 512

char bootsector[SIZE];
DWORD bytesRead = 0;

HANDLE d;

int flag_getfs()
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	if (!strncmp(bootsector + 54, "FAT16   ", 8)) {
		return FAT16;
	}

	if (!strncmp(bootsector + 82, "FAT32   ", 8)) {
		return FAT32;
	}

	if (!strncmp(bootsector + 3, "EXFAT   ", 8)) {
		return EXFAT;
	}

	return DRIVE_BADFS;
}

void flag_write(long offset, char string[])
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	printf("Current Flag: %s\n", bootsector + offset);
	memcpy(bootsector + offset, string, strlen(string));
	printf("New Flag:     %s\n", bootsector + offset);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	WriteFile(d, bootsector, SIZE, &bytesRead, NULL);

	if (flag_getfs() == EXFAT) {
		printf("Card is ExFAT, writing flags in the backup VBR.\n");
		printf("Writing \"%s\" at 0x%lx\n", string, offset + (512 * 12));
		SetFilePointer(d, offset + (512 * 12), NULL, FILE_BEGIN);
		WriteFile(d, string, strlen(string), &bytesRead, NULL);
	}
}

int flag_getdrive()
{
	char id;
	char dstr[] = "X:\\";

	DWORD drives = GetLogicalDrives();
	for (int i = 0; i < 32; i++) {
		// Check if bit is 1
		if ((drives >> i) & 1) {
			dstr[0] = i + 'A';

			char volname[100];
			GetVolumeInformationA(dstr, volname, sizeof(volname), NULL, NULL, NULL,
					      NULL, 0);

			if (!strncmp(volname, "EOS_DIGITAL", 11)) {
				if (i + 'A' == 'C') {
					puts("PANIC, somehow got C drive...");
					return DRIVE_NONE;
				}

				return (int)(i + 'A');
			}
		}
	}

	return DRIVE_NONE;
}

// Should never buffer overflow
int flag_usable_drive(char buffer[])
{
	int drive = flag_getdrive();
	if (drive < 0) {
		return drive;
	}

	strcpy(buffer, " :");
	buffer[0] = (char)drive;
	return 0;
}

int flag_openfs(int mode)
{
	// Windows filesystems must be opened like this: \\.\E:
	char buffer[64] = "\\\\.\\0:";
	int drive = flag_getdrive();
	if (drive < 0) {
		return drive;
	}

	buffer[4] = (char)drive;

	d = CreateFile(buffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);

	if (d == INVALID_HANDLE_VALUE) {
		puts("Couldn't open the filesystem. Try running as Administrator.\n"
		     "Check file explorer and make sure EOS_DIGITAL is mounted.");
		return -1;
	}

	return flag_getfs();
}

void flag_close()
{
	CloseHandle(d);
}

void updateExFAT()
{
	unsigned int buffer[EXFAT_VBR_SIZE + 512];

	ReadFile(d, buffer, EXFAT_VBR_SIZE + 512, &bytesRead, NULL);
	int sum = VBRChecksum((unsigned char *)buffer, EXFAT_VBR_SIZE);
	for (int i = 0; i < 512 / 4; i++) {
		buffer[i] = sum;
	}

	// Write the VBR checksum, or as the old install script said:
	// "write VBR checksum (from sector 0 to sector 10) at offset 5632 (sector 11) and offset 11776 (sector 23, for backup VBR)
	// checksum sector is stored in $dump_file at offset 5632"
	SetFilePointer(d, 5632, NULL, FILE_BEGIN);
	WriteFile(d, buffer, 512, &bytesRead, NULL);
	SetFilePointer(d, 11776, NULL, FILE_BEGIN);
	WriteFile(d, buffer, 512, &bytesRead, NULL);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	FlushFileBuffers(d);
}

#endif

