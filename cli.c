// Command line based application
// (very simple, bare bones, needs to be improved)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>

#include "src/config.h"
#include "src/ptp.h"
#include "src/ptpcam.h"
#include "src/drive.h"

void help() {
	puts("ML USB Install Tools CLI");
	puts("Usage: mlinstall <command> <argument>");
	puts("    run <COMMAND>  Run a Canon event proc via USB");
	puts("    enable         Run EnableBootDisk");
	puts("    disable        Run DisableBootDisk");
	puts("    cardboot       Write EOS_DEVELOP and BOOTDISK flags to a mounted SD card named \"EOS_DEVELOP\"");
	puts("    noboot         Destroy the card flags by writing an underscore on the first character.");
	puts("    info           Get information on the camera.");	
}

int main(int argc, char *argv[])
{
	int busn = 0, devn = 0;
	short force = 0;

	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (argc < 2) {
		help();
		return 0;
	}

	if (!strcmp(argv[1], "run")) {
		if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
			puts("Connection error!");
			return -1;
		}

		ptp_runeventproc(&params, argv[2], NULL);

		close_camera(&ptp_usb, &params, dev);
	} else if (!strcmp(argv[1], "info")) {
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
			puts("Connection error!");
			return -1;
		}

		PTPDeviceInfo info;
		ptp_getdeviceinfo(&params, &info);

		printf("Manufacturer: %s\n"
		       "Model: %s\n"
		       "DeviceVersion: %s\n"
		       "SerialNumber: %s\n",
		       info.Manufacturer, info.Model, info.DeviceVersion,
		       info.SerialNumber);

		close_camera(&ptp_usb, &params, dev);
	} else if (!strcmp(argv[1], "enable")) {
		if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
			puts("Connection error!");
			return -1;
		}

		ptp_runeventproc(&params, "EnableBootDisk", NULL);
		close_camera(&ptp_usb, &params, dev);

		puts("Enabled boot disk.");
	} else if (!strcmp(argv[1], "disable")) {
		if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
			puts("Connection error!");
			return -1;
		}

		ptp_runeventproc(&params, "DisableBootDisk", NULL);
		close_camera(&ptp_usb, &params, dev);

		puts("Disabled boot disk.");
	} else if (!strcmp(argv[1], "cardboot")) {
		flag_write_flag(FLAG_BOOT);
		flag_close();
	} else if (!strcmp(argv[1], "noboot")) {
		flag_write_flag(FLAG_DESTROY_BOOT);
		flag_close();
	} else {
		puts("Invalid command");
	}

	return 0;
}
