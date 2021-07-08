#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>

#include "config.h"
#include "ptp.h"
#include "ptpcam.h"

// From ptp.c
uint16_t ptp_runeventproc(PTPParams* params, char string[]);

// flag.c, or flag-win.c
int enableFlag();

int main() {
	int busn = 0, devn = 0;
	short force = 0;

	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device* dev;

	puts("ML USB Install Tools (ptpcam backend)");
	puts("Commands:");
	puts("    run <COMMAND>        Run a Canon event proc via micro usb");
	puts("    bootdisk             Run EnableBootDisk and allow autoexec.bin to be run.");
	puts("    bootdiskoff          Run DisableBootDisk");
	puts("    flags <DRIVE LETTER> Write EOS_DEVELOP and BOOTDISK flag to mounted SD card named \"EOS_DEVELOP\"");

	while (1) {
		putchar(':');
		char input[64];
		fgets(input, 64, stdin);
		strtok(input, "\n");

		if (!strncmp(input, "run ", 4)) {
			if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
				continue;
			}

			ptp_runeventproc(&params, input + 4);

			close_camera(&ptp_usb, &params, dev);
		} else if (!strcmp(input, "bootdisk")) {
			if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
				continue;
			}

			ptp_runeventproc(&params, "EnableBootDisk");
			close_camera(&ptp_usb, &params, dev);

			puts("Enabled boot disk.");
		} else if (!strcmp(input, "bootdiskoff")) {
			if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
				continue;
			}

			ptp_runeventproc(&params, "EnableBootDisk");
			close_camera(&ptp_usb, &params, dev);

			puts("Enabled boot disk.");
		} else if (!strncmp(input, "flags", 6)) {
			enableFlag();
		} else if (input[0] == 'x' || input[0] == 'q') {
			return 0;
		} else {
			puts("Invalid command.");
		}
	}

	return 0;
}