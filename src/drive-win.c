// Windows fileapi based code
#ifdef WIN32

#include <stdio.h>
#include <windows.h>

BOOL IsUserAnAdmin();

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

// WriteFile only writes to a single sector at a time, this
// immitates fwrite to make things a little easier
static int exfat_write(int location, int length, void *bytes)
{
	// Get sector address to write to
	int offset = location + (512 * 12);
	int sector = offset - (offset % 512);

	// Write to specific spot in 
	SetFilePointer(d, sector, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);
	memcpy(bootsector + (offset % 512), bytes, length);
	SetFilePointer(d, sector, NULL, FILE_BEGIN);
	WriteFile(d, bootsector, 512, &bytesRead, NULL);
	
	return GetLastError();
}

void flag_write(long offset, char string[])
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	printf("Current Flag: %s\n", bootsector + offset);
	memcpy(bootsector + offset, string, strlen(string));
	printf("New Flag:     %s\n", bootsector + offset);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	if (!WriteFile(d, bootsector, SIZE, &bytesRead, NULL)) {
		printf("Error writing to drive: %d\n", GetLastError());
		return;
	}

	if (flag_getfs() == EXFAT) {
		printf("Card is ExFAT, writing flags in the backup VBR.\n");
		printf("Writing \"%s\" at 0x%lx\n", string, offset + (512 * 12));

		exfat_write(offset, strlen(string), string);
	}
	
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	FlushFileBuffers(d);
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
				
				//DeleteVolumeMountPointA(dstr);

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
	if (drive == DRIVE_NONE) {
		return DRIVE_NONE;
	}
	
	buffer[4] = (char)drive;

	d = CreateFile(buffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);

	if (d == INVALID_HANDLE_VALUE) {
		puts("Couldn't open the filesystem. Check file explorer and make sure EOS_DIGITAL exists.");
		return DRIVE_ERROR;
	}
	
	if (!IsUserAnAdmin()) {
		puts("MLinstall needs Administrator privileges to write and unmount drive.");
		return DRIVE_ERROR;
	}
	
	// DeviceIoControl returns 1 on success
	if (!DeviceIoControl(d, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesRead, NULL)) {
		puts("Couldn't lock drive for some reason.");
		return DRIVE_ERROR;
	}

	return 0;
}

void flag_close()
{
	CloseHandle(d);
}

void update_exfat()
{
	unsigned int buffer[EXFAT_VBR_SIZE + 512];

	ReadFile(d, buffer, EXFAT_VBR_SIZE + 512, &bytesRead, NULL);
	int sum = VBRChecksum((unsigned char *)buffer, EXFAT_VBR_SIZE);
	for (int i = 0; i < 512 / 4; i++) {
		buffer[i] = sum;
	}

	// "write VBR checksum (from sector 0 to sector 10) at offset 5632 (sector 11) and offset 11776 (sector 23, for backup VBR)"
	// Don't need to use exfat_write since these regions are divisible by 512
	SetFilePointer(d, 5632, NULL, FILE_BEGIN);
	WriteFile(d, buffer, 512, &bytesRead, NULL);
	SetFilePointer(d, 11776, NULL, FILE_BEGIN);
	WriteFile(d, buffer, 512, &bytesRead, NULL);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	FlushFileBuffers(d);
}

void drive_dump(char name[]) {
	puts("Creating a backup of your SD card first few sectors.");

	char *dump = malloc(512 * 12);
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, dump, 512 * 12, &bytesRead, NULL);

	FILE *f = fopen(name, "w");
	fwrite(dump, 1, 512 * 12, f);
	fclose(f);
}

#endif

