// Command line based application
// (very simple, bare bones, could be improved)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>

#include "src/config.h"
#include "src/ptp.h"
#include "src/ptpcam.h"
#include "src/drive.h"

int main()
{
	int busn = 0, devn = 0;
	short force = 0;

	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	puts("ML USB Install Tools (ptpcam backend)");
	puts("Commands:");
	puts("    run <COMMAND>        Run a Canon event proc via micro usb");
	puts("    bootdisk             Run EnableBootDisk and allow autoexec.bin to "
	     "be run.");
	puts("    bootdiskoff          Run DisableBootDisk");
	puts("    flags                Write EOS_DEVELOP and BOOTDISK flag to "
	     "mounted SD card named \"EOS_DEVELOP\"");
	puts("    dflags               Destroy the card flags by writing an underscore on the first character.");
	puts("    info                 Get information on the camera.");

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
		} else if (!strcmp(input, "info")) {
			if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
				continue;
			}
		
			PTPDeviceInfo info;
			ptp_getdeviceinfo(&params, &info);
		
			printf(
				"Manufacturer: %s\n"
				"Model: %s\n"
				"DeviceVersion: %s\n"
				"SerialNumber: %s\n",
				info.Manufacturer, info.Model, info.DeviceVersion, info.SerialNumber
			);
		
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
			flag_write_flag(FLAG_BOOT);
			flag_close();
		} else if (!strncmp(input, "dflags", 6)) {
			flag_write_flag(FLAG_DESTROY_BOOT);
			flag_close();
		} else if (input[0] == 'x' || input[0] == 'q') {
			return 0;
		} else {
			puts("Invalid command.");
		}
	}

	return 0;
}
