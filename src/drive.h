#ifndef DRIVE_H
#define DRIVE_H

#include <stdint.h>

#define TEMP_DUMP_SIZE (512 * 12)

enum FsType { FAT16 = 0, FAT32 = 1, EXFAT = 2 };

enum FlagsMode {
	FLAG_ALL,
	FLAG_BOOT,
	FLAG_SCRIPT,

	FLAG_DESTROY_BOOT,
	FLAG_DESTROY_SCRIPT,
};

// Result codes shared by the whole drive stack, from the io ports
// up to the UI. The io layer may only return these.
enum FlagsErr {
	DRIVE_BADFS = -1,
	DRIVE_NONE = -2,
	DRIVE_ERROR = -3,
	DRIVE_PRIV = -4, // permission denied, caller may retry elevated
};

// Bitmask of which ML flags are currently written on the card,
// returned by flag detection (drive_check_flags())
enum FlagsPresent {
	FLAG_PRESENT_DEVELOP = 1 << 0, // "EOS_DEVELOP"
	FLAG_PRESENT_BOOTDISK = 1 << 1, // "BOOTDISK"
	FLAG_PRESENT_SCRIPT = 1 << 2, // "SCRIPT"
};

// Progress logging, implemented by the consumer (libui.c for the GUI,
// card-main.c for mlinstall-card, test/fake-io.c for tests)
void log_print(char *format, ...);

// Orchestration, shared between the Windows GUI and mlinstall-card
int drive_write_flag(int mode);
int drive_check_flags(void);
int drive_getfs(void);
int drive_openfs(void);
int drive_close(void);

// Optional fixed device target (mlinstall-card argv, tests). When unset,
// the device is found per-platform via drive_io_discover()
void drive_set_device(const char *device);

// Display path of the card (mountpoint on Linux, "X:" on Windows),
// for the UI info button. Mounts the card if unmounted.
int drive_get_usable(char buffer[], int n);

// Card display name (volume label) and capacity in bytes, for the
// Card tab. Pure query, no mount side effects.
// Returns 0, DRIVE_NONE or DRIVE_ERROR.
int drive_get_info(char *name, int name_n, uint64_t *bytes);

// Shared semantics, unit-testable against a fake io port. All I/O
// failures propagate (these used to be void and report success).
int flag_write(long offset, const char *string);
int flag_match(long offset, const char *string);
int update_exfat(void);
int drive_dump(const char *name);

// ---------------------------------------------------------------------------
// Raw block device port. Exactly one implementation is linked per target:
//   drive-win.c       Windows GUI     (CreateFile, sector RMW, volume lock)
//   drive-udisks.c    mlinstall-card  (udisks2 verify/unmount + POSIX I/O)
//   drive-io-posix.c  Darwin GUI      (POSIX I/O + diskutil)
//   test/fake-io.c    unit tests      (in-memory image, bounds checked)
//
// Contract:
//  - Calls are legal only between drive_io_open() and drive_io_close().
//  - Single-threaded, one device at a time (replaces the old global FILE *d).
//  - Byte-granular access at any offset/length: implementations must
//    perform sector read-modify-write internally if the platform
//    requires aligned I/O.
//  - Returns are >= 0 on success (byte counts for pread/pwrite) or a
//    negative FlagsErr value from above.

// Open `device` raw read-write. Platform side effects belong here:
// Windows locks the volume, Linux unmounts via udisks2, Darwin diskutil.
// Returns 0, or DRIVE_PRIV on access denied (caller may retry elevated),
// DRIVE_NONE if the device doesn't exist, DRIVE_ERROR otherwise.
int drive_io_open(const char *device);
int drive_io_pread(uint64_t offset, void *buf, int len);
int drive_io_pwrite(uint64_t offset, const void *buf, int len);
int drive_io_sync(void);

// Flush, unlock, release, and re-mount (Linux). Idempotent after success.
int drive_io_close(void);

// Find the EOS_DIGITAL card and copy its raw device path (platform syntax)
// into buf. Returns 0, DRIVE_NONE or DRIVE_ERROR.
int drive_io_discover(char *buf, int n);

#endif