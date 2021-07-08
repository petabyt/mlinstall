// Alternative enabler that interacts
// with filesystems via win32 fileapi
#include <stdio.h>
#include <windows.h>

enum FsType {
	FAT16 = 0,
	FAT32 = 1,
	EXFAT = 2
};

#define SIZE 512

char bootsector[SIZE];
DWORD bytesRead;

int getDrive(HANDLE d) {
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	if (!strcmp(bootsector + 54, "FAT16   ")) {
		return FAT16;
	}
	
	if (!strcmp(bootsector + 82, "FAT32   ")) {
		return FAT32;
	}

	if (!strcmp(bootsector + 82, "EXFAT   ")) {
		return EXFAT;
	}

	return 0;
}

void setboot(HANDLE d, long of1, long of2) {
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	printf("Current Flag 1: %s\n", bootsector + of1);
	printf("Current Flag 2: %s\n", bootsector + of2);

	memcpy(bootsector + of1, "EOS_DEVELOP", 11);
	memcpy(bootsector + of2, "BOOTDISK", 8);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	WriteFile(d, bootsector, SIZE, &bytesRead, NULL);
}

int enableFlag() {
	char buffer[64];

	// List info usb type mounted filesystems
    FILE *f = popen("wmic logicaldisk where drivetype=2 get deviceid, volumename", "r");

    // Skip first line (title)
    fgets(buffer, 64, f);

	// Look for EOS_DIGITAL drive
    while (fgets(buffer, 64, f) != NULL) {
        if (!strncmp(buffer + 10, "EOS_DIGITAL", 11)) {
        	printf("Found EOS_DIGITAL at drive %c\n", buffer[0]);
            id = buffer[0];
            break;
        }
    }

	char driveID[] = "\\\\.\\E:";
	driveID[4] = id;

	HANDLE d = CreateFile(
		driveID,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
		NULL
	);

	if (d == INVALID_HANDLE_VALUE) {
		puts("Could not open filesystem. Try running as Administrator.");
		return 1;
	}

	int drive = getDrive(d);

	if (drive == EXFAT) {
		puts("Quitting, no EXFAT support yet. Try EOSCARD.");
		return 1;
	} else if (drive == FAT16) {
		puts("Writing to FAT16 filesystem.");
		setboot(d, 71, 92);
	} else if (drive == FAT32) {
		puts("Writing to FAT32 filesystem.");
		setboot(d, 71, 92);
	} else {
		puts("Unsupported FS");
		return 1;
	}

	puts("Wrote EOS_DEVELOP AND BOOTDISK.");
	puts("Unmount the device to save changes.");
}

#ifdef TEST
int main() {
	// E:\ drive
	enableFlag('E');
}
#endif