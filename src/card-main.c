// mlinstall-card: minimal privileged disk tool for mlinstall on Linux.
// All card I/O lives here so the GUI itself never needs root:
//
//   mlinstall-card write <device> <all|boot|script|destroy-boot|destroy-script>
//   mlinstall-card check <device>
//
// The GUI spawns this directly and retries through pkexec when it exits
// with code 4 (needs privileges). Exit code is -rc of the drive call:
//   0 ok, 1 bad fs, 2 card not found, 3 I/O error, 4 needs privileges
// check exits 0 on success and prints the bitmask as "flags: <n>".

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drive.h"

void log_print(char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n');
	fflush(stdout);
}

static int parse_mode(const char *name) {
	if (!strcmp(name, "all")) return FLAG_ALL;
	if (!strcmp(name, "boot")) return FLAG_BOOT;
	if (!strcmp(name, "script")) return FLAG_SCRIPT;
	if (!strcmp(name, "destroy-boot")) return FLAG_DESTROY_BOOT;
	if (!strcmp(name, "destroy-script")) return FLAG_DESTROY_SCRIPT;
	return -1;
}

static void usage(void) {
	puts("Usage: mlinstall-card write <device> <mode>\n"
	     "Modes: all, boot, script, destroy-boot, destroy-script\n"
	     "       mlinstall-card check <device>\n"
	     "The device must be a partition labeled 'EOS_DIGITAL'.");
}

static int cmd_write(const char *device, const char *mode_name) {
	int mode = parse_mode(mode_name);
	if (mode < 0) {
		fprintf(stderr, "Unknown mode: %s\n", mode_name);
		usage();
		return -DRIVE_ERROR;
	}

	drive_set_device(device);

	int rc = drive_write_flag(mode);
	if (rc < 0) return -rc;
	return 0;
}

static int cmd_check(const char *device) {
	drive_set_device(device);

	int rc = drive_check_flags();
	if (rc < 0) return -rc;

	if (rc == 0) {
		log_print("No flags present.");
	} else {
		log_print("Flags present:");
		if (rc & FLAG_PRESENT_DEVELOP) log_print("\tdevelop");
		if (rc & FLAG_PRESENT_BOOTDISK) log_print("\tbootdisk");
		if (rc & FLAG_PRESENT_SCRIPT) log_print("\tscript");
	}

	// Machine-readable status line for the GUI proxy / scripts
	log_print("flags: %d", rc);
	return 0;
}

int main(int argc, char **argv) {
	if (argc == 4 && !strcmp(argv[1], "write")) {
		return cmd_write(argv[2], argv[3]);
	}

	if (argc == 3 && !strcmp(argv[1], "check")) {
		return cmd_check(argv[2]);
	}

	usage();
	return -DRIVE_ERROR;
}