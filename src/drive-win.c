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

int drive_getfs()
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
	char old_flag[16] = { 0 };

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	memcpy(old_flag, bootsector + offset, strlen(string) % sizeof(old_flag));
	printf("Current Flag: %s\n", old_flag);
	memcpy(bootsector + offset, string, strlen(string));
	printf("New Flag:     %s\n", string);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	if (!WriteFile(d, bootsector, SIZE, &bytesRead, NULL)) {
		printf("Error writing to drive: %ld\n", GetLastError());
		return;
	}

	if (drive_getfs() == EXFAT) {
		printf("Card is ExFAT, writing flags in the backup VBR.\n");
		printf("Writing \"%s\" at 0x%lx\n", string, offset + (512 * 12));

		exfat_write(offset, strlen(string), string);
	}
	
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	FlushFileBuffers(d);
}

// Returns drive letter as int
int drive_get()
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

int drive_get_usable(char buffer[], int n)
{
	int drive = drive_get();
	if (drive < 0) {
		return drive;
	}

	strncpy(buffer, " :", n);
	buffer[0] = (char)drive;
	return 0;
}

int drive_openfs(int mode)
{
	// Windows filesystems must be opened like this: \\.\E:
	char buffer[64] = "\\\\.\\0:";
	int drive = drive_get();
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
	
	// Lock drive while writing to it
	if (!DeviceIoControl(d, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesRead, NULL)) {
		puts("Couldn't lock drive for some reason.");
		return DRIVE_ERROR;
	}

	return 0;
}

void drive_close()
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
	char *dump = malloc(TEMP_DUMP_SIZE);
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, dump, TEMP_DUMP_SIZE, &bytesRead, NULL);

	FILE *f = fopen(name, "w");
	fwrite(dump, 1, TEMP_DUMP_SIZE, f);
	fclose(f);
}

#endif
