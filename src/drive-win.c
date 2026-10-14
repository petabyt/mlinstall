// Windows port of the drive_io interface: raw volume access via CreateFile.
// fopen/fwrite won't write to a volume, and volume handles opened with
// FILE_FLAG_NO_BUFFERING require sector-aligned transfers, hence the
// read-modify-write emulation below.

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "drive.h"

#define SECTOR_SIZE 512

static HANDLE d = INVALID_HANDLE_VALUE;
static unsigned char sector_buf[SECTOR_SIZE];

static int find_card_drive(char *letter) {
	char dstr[] = "X:\\";

	DWORD drives = GetLogicalDrives();
	for (int i = 0; i < 32; i++) {
		// Check if bit is 1
		if (!((drives >> i) & 1)) continue;
		dstr[0] = (char)(i + 'A');

		char volname[100];
		if (!GetVolumeInformationA(dstr, volname, sizeof(volname), NULL, NULL, NULL,
					   NULL, 0)) {
			continue;
		}

		if (!strncmp(volname, "EOS_DIGITAL", 11)) {
			if (dstr[0] == 'C') {
				puts("PANIC, somehow got C drive...");
				return DRIVE_NONE;
			}

			*letter = dstr[0];
			return 0;
		}
	}

	return DRIVE_NONE;
}

int drive_io_discover(char *buf, int n) {
	char letter;
	int rc = find_card_drive(&letter);
	if (rc) return rc;

	snprintf(buf, n, "\\\\.\\%c:", letter);
	return 0;
}

int drive_get_usable(char buffer[], int n) {
	char letter;
	int rc = find_card_drive(&letter);
	if (rc) return rc;

	if (n < 3) return DRIVE_ERROR;
	buffer[0] = letter;
	buffer[1] = ':';
	buffer[2] = '\0';
	return 0;
}

int drive_get_info(char *name, int name_n, uint64_t *bytes) {
	char letter;
	char dstr[] = "X:\\";
	int rc = find_card_drive(&letter);
	if (rc) return rc;

	dstr[0] = letter;

	char volname[100];
	if (!GetVolumeInformationA(dstr, volname, sizeof(volname), NULL, NULL, NULL,
				   NULL, 0)) {
		return DRIVE_ERROR;
	}

	ULARGE_INTEGER total;
	if (!GetDiskFreeSpaceExA(dstr, NULL, &total, NULL)) {
		return DRIVE_ERROR;
	}

	snprintf(name, name_n, "%s", volname);
	*bytes = total.QuadPart;
	return 0;
}

int drive_io_open(const char *device) {
	DWORD bytes;

	d = CreateFile(device, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);

	if (d == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err == ERROR_ACCESS_DENIED) {
			log_print("Access denied opening %s", device);
			return DRIVE_PRIV;
		}

		log_print("Couldn't open %s (error %ld)", device, (long)err);
		return DRIVE_ERROR;
	}

	// Lock drive while writing to it
	if (!DeviceIoControl(d, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL)) {
		log_print("Couldn't lock drive for some reason.");
		CloseHandle(d);
		d = INVALID_HANDLE_VALUE;
		return DRIVE_ERROR;
	}

	return 0;
}

static int read_sector(uint64_t offset, unsigned char *buf) {
	DWORD bytes;
	LONG low = (LONG)(offset & 0xffffffff);
	LONG high = (LONG)(offset >> 32);

	SetFilePointer(d, low, &high, FILE_BEGIN);
	if (!ReadFile(d, buf, SECTOR_SIZE, &bytes, NULL) || bytes != SECTOR_SIZE) {
		return DRIVE_ERROR;
	}

	return 0;
}

static int write_sector(uint64_t offset, const unsigned char *buf) {
	DWORD bytes;
	LONG low = (LONG)(offset & 0xffffffff);
	LONG high = (LONG)(offset >> 32);

	SetFilePointer(d, low, &high, FILE_BEGIN);
	if (!WriteFile(d, buf, SECTOR_SIZE, &bytes, NULL) || bytes != SECTOR_SIZE) {
		return DRIVE_ERROR;
	}

	return 0;
}

// Emulate byte-granular I/O over sector-aligned volume transfers
int drive_io_pread(uint64_t offset, void *buf, int len) {
	unsigned char *out = buf;
	int total = 0;

	while (len > 0) {
		uint64_t sector = offset - (offset % SECTOR_SIZE);
		int off = (int)(offset % SECTOR_SIZE);
		int chunk = SECTOR_SIZE - off;
		if (chunk > len) chunk = len;

		if (read_sector(sector, sector_buf)) return DRIVE_ERROR;
		memcpy(out, sector_buf + off, chunk);

		out += chunk;
		offset += chunk;
		len -= chunk;
		total += chunk;
	}

	return total;
}

int drive_io_pwrite(uint64_t offset, const void *buf, int len) {
	const unsigned char *in = buf;
	int total = 0;

	while (len > 0) {
		uint64_t sector = offset - (offset % SECTOR_SIZE);
		int off = (int)(offset % SECTOR_SIZE);
		int chunk = SECTOR_SIZE - off;
		if (chunk > len) chunk = len;

		if (read_sector(sector, sector_buf)) return DRIVE_ERROR;
		memcpy(sector_buf + off, in, chunk);
		if (write_sector(sector, sector_buf)) return DRIVE_ERROR;

		in += chunk;
		offset += chunk;
		len -= chunk;
		total += chunk;
	}

	return total;
}

int drive_io_sync(void) {
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	if (!FlushFileBuffers(d)) {
		return DRIVE_ERROR;
	}

	return 0;
}

int drive_io_close(void) {
	if (d != INVALID_HANDLE_VALUE) {
		// The volume lock is released when the handle closes
		CloseHandle(d);
		d = INVALID_HANDLE_VALUE;
	}

	return 0;
}