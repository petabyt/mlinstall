// Platform independent drive code. All I/O goes through the drive_io
// port (declared in drive.h), so
// this runs unchanged against a real device (Windows GUI, mlinstall-card)
// or against an in-memory fake in the unit tests.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drive.h"

#include "exfat.h"

// Flag strings written to the boot sector. Destroy modes flip the first
// character to '_'; the table is always restored afterwards.
static char flag_strings[3][12] = { "EOS_DEVELOP", "BOOTDISK", "SCRIPT" };

// Slots per filesystem: develop, bootdisk, script. Script lives at the
// end of the boot sector on every fs; the others sit in the FAT volume
// label / reserved areas.
static int flag_offsets(int fs, long of[3]) {
	switch (fs) {
	case EXFAT:
		of[0] = 0x82;
		of[1] = 0x7a;
		of[2] = 0x1f0;
		return 0;
	case FAT16:
		of[0] = 0x2b;
		of[1] = 0x40;
		of[2] = 0x1f0;
		return 0;
	case FAT32:
		of[0] = 0x47;
		of[1] = 0x5c;
		of[2] = 0x1f0;
		return 0;
	}

	return DRIVE_BADFS;
}

static char drive_device[128] = { 0 };

void drive_set_device(const char *device) {
	if (device == NULL) {
		drive_device[0] = '\0';
		return;
	}

	snprintf(drive_device, sizeof(drive_device), "%s", device);
}

int drive_openfs(void) {
	char dev[128];

	if (drive_device[0] != '\0') {
		snprintf(dev, sizeof(dev), "%s", drive_device);
	} else {
		int rc = drive_io_discover(dev, sizeof(dev));
		if (rc) return rc;
	}

	return drive_io_open(dev);
}

int drive_getfs(void) {
	unsigned char sector[512];

	if (drive_io_pread(0, sector, sizeof(sector)) < 0) {
		return DRIVE_ERROR;
	}

	if (!memcmp(sector + 54, "FAT16   ", 8)) return FAT16;
	if (!memcmp(sector + 82, "FAT32   ", 8)) return FAT32;
	if (!memcmp(sector + 3, "EXFAT   ", 8)) return EXFAT;

	return DRIVE_BADFS;
}

// Open the card and report the failure to the user, if any.
// Returns 0 or a negative FlagsErr.
static int open_card_log(void) {
	int r = drive_openfs();
	switch (r) {
	case 0:
		break;
	case DRIVE_BADFS:
		puts("The EOS_DIGITAL card must be FAT32, FAT16, or ExFAT.");
		break;
	case DRIVE_NONE:
		puts("Couldn't find an EOS_DIGITAL card. Make sure the EOS_DIGITAL card is mounted.");
		break;
	case DRIVE_PRIV:
		log_print("Needs elevated permissions.");
		break;
	default:
		puts("Error opening drive.");
		break;
	}

	return r;
}

// Open the card and detect the filesystem, closing it on failure.
// Returns FsType or a negative FlagsErr.
static int open_card_fs(void) {
	int r = open_card_log();
	if (r) return r;

	int fs = drive_getfs();
	if (fs == DRIVE_BADFS || fs == DRIVE_ERROR) {
		drive_close();
		return fs;
	}

	return fs;
}

// 1 if `string` is present at `offset`, 0 if not, <0 on read errors.
// Device must be open.
int flag_match(long offset, const char *string) {
	int len = strlen(string);
	char buf[12];

	if (len >= (int)sizeof(buf)) return DRIVE_ERROR;
	if (drive_io_pread(offset, buf, len) < 0) return DRIVE_ERROR;

	return memcmp(buf, string, len) == 0;
}

// Bitmask of flags present at the given offsets, or <0 on read errors.
// Device must be open.
static int flags_present(const long of[3]) {
	int present = 0;

	for (int i = 0; i < 3; i++) {
		int rc = flag_match(of[i], flag_strings[i]);
		if (rc < 0) return rc;
		if (rc) present |= 1 << i;
	}

	return present;
}

// Detect which ML flags are currently written on the card.
// Returns a FlagsPresent bitmask (0 = none) or a negative FlagsErr.
int drive_check_flags(void) {
	int fs = open_card_fs();
	if (fs < 0) return fs;

	long of[3];
	if (flag_offsets(fs, of)) {
		drive_close();
		return DRIVE_BADFS;
	}

	int rc = flags_present(of);

	// Read-only check: a failed close (remount) is only a warning
	if (drive_close()) {
		log_print("Failed to release card (may stay unmounted).");
	}

	return rc;
}

int flag_write(long offset, const char *string) {
	int len = strlen(string);
	char old[16] = { 0 };
	int rc;

	rc = drive_io_pread(offset, old, len < (int)sizeof(old) - 1 ? len : (int)sizeof(old) - 1);
	if (rc < 0) {
		log_print("Error reading old flag at 0x%lx", offset);
		return DRIVE_ERROR;
	}

	// Stop the logged old flag at the first non-printable byte
	for (int i = 0; i < (int)sizeof(old); i++) {
		if (old[i] != '\0' && (old[i] < ' ' || old[i] > '~')) {
			old[i] = '\0';
			break;
		}
	}

	log_print("%s\t-> %s", old, string);

	rc = drive_io_pwrite(offset, string, len);
	if (rc < 0) {
		log_print("Error writing to drive.");
		return DRIVE_ERROR;
	}

	if (drive_getfs() == EXFAT) {
		log_print("Card is ExFAT, writing flags in the backup VBR.");
		log_print("Writing \"%s\" at 0x%lx", string, offset + (512 * 12));

		rc = drive_io_pwrite(offset + (512 * 12), string, len);
		if (rc < 0) {
			log_print("Error writing to backup VBR.");
			return DRIVE_ERROR;
		}
	}

	return drive_io_sync();
}

int update_exfat(void) {
	// Checksum of sectors 0-10, written to sector 11 and
	// sector 23 (for the backup VBR)
	unsigned char buffer[EXFAT_VBR_SIZE + 512];
	unsigned char checksum[512];
	unsigned int sum;
	int rc;

	if (drive_io_pread(0, buffer, sizeof(buffer)) < 0) {
		log_print("Error reading VBR for checksum.");
		return DRIVE_ERROR;
	}

	sum = VBRChecksum(buffer, EXFAT_VBR_SIZE);

	// exFAT stores the checksum as the same little-endian 32-bit
	// value repeated across the whole sector
	for (int i = 0; i < 512; i += 4) {
		checksum[i + 0] = sum & 0xff;
		checksum[i + 1] = (sum >> 8) & 0xff;
		checksum[i + 2] = (sum >> 16) & 0xff;
		checksum[i + 3] = (sum >> 24) & 0xff;
	}

	rc = drive_io_pwrite(5632, checksum, sizeof(checksum));
	if (rc < 0) return DRIVE_ERROR;

	rc = drive_io_pwrite(11776, checksum, sizeof(checksum));
	if (rc < 0) return DRIVE_ERROR;

	return drive_io_sync();
}

int drive_dump(const char *name) {
	char *dump = malloc(TEMP_DUMP_SIZE);
	if (dump == NULL) return DRIVE_ERROR;

	if (drive_io_pread(0, dump, TEMP_DUMP_SIZE) < 0) {
		free(dump);
		return DRIVE_ERROR;
	}

	// Binary mode: text mode would corrupt the dump on Windows
	FILE *f = fopen(name, "wb");
	if (f == NULL) {
		free(dump);
		return DRIVE_ERROR;
	}

	fwrite(dump, 1, TEMP_DUMP_SIZE, f);
	fclose(f);
	free(dump);

	return 0;
}

int drive_close(void) {
	return drive_io_close();
}

// Detect the filesystem and write the flags
// in the correct place
int drive_write_flag(int mode) {
	int drive = open_card_fs();
	if (drive < 0) return drive;

	long int of[3] = { 0, 0, 0 };
	if (flag_offsets(drive, of)) {
		drive_close();
		return DRIVE_BADFS;
	}

	// Best-effort status of what's already on the card. Runs before
	// destroy modes mutate the flag strings.
	int present = flags_present(of);
	if (present < 0) {
		log_print("Couldn't read existing flags.");
	} else if (present == 0) {
		log_print("No existing flags on card.");
	} else {
		log_print("Flags already present:");
		if (present & FLAG_PRESENT_DEVELOP) log_print("\tdevelop");
		if (present & FLAG_PRESENT_BOOTDISK) log_print("\tbootdisk");
		if (present & FLAG_PRESENT_SCRIPT) log_print("\tscript");
	}

	log_print("Backing up card bootsector...");
	log_print("Creating a small backup just in case...");
	if (drive_dump("SD_BACKUP")) {
		log_print("Couldn't back up card, aborting.");
		drive_close();
		return DRIVE_ERROR;
	}

	int rc = 0;
	switch (mode) {
	case FLAG_ALL:
		rc = flag_write(of[0], flag_strings[0]);
		if (!rc) rc = flag_write(of[1], flag_strings[1]);
		if (!rc) rc = flag_write(of[2], flag_strings[2]);
		break;
	case FLAG_SCRIPT:
		rc = flag_write(of[2], flag_strings[2]);
		break;
	case FLAG_BOOT:
		rc = flag_write(of[0], flag_strings[0]);
		if (!rc) rc = flag_write(of[1], flag_strings[1]);
		break;
	case FLAG_DESTROY_BOOT:
		flag_strings[0][0] = '_';
		flag_strings[1][0] = '_';

		rc = flag_write(of[0], flag_strings[0]);
		if (!rc) rc = flag_write(of[1], flag_strings[1]);

		flag_strings[0][0] = 'E';
		flag_strings[1][0] = 'B';
		break;
	case FLAG_DESTROY_SCRIPT:
		flag_strings[2][0] = '_';

		rc = flag_write(of[2], flag_strings[2]);

		flag_strings[2][0] = 'S';
		break;
	}

	if (!rc && drive == EXFAT) {
		rc = update_exfat();
	}

	int cr = drive_close();
	if (cr) {
		// The flags are written; a failed remount is only a warning
		log_print("Failed to release card (may stay unmounted).");
	}

	if (rc) return rc;

	puts("Wrote card flags.");
	return 0;
}