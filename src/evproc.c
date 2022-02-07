#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#include "config.h"
#include "ptp.h"
#include "ptpcam.h"

int evproc_run(char string[])
{
	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		return 0;
	}

	// Command is disabled on some cams	
	ptp_activate_command(&params);
	
	int stringLen = strlen(string);
	
	char data[1024];
	memcpy(data, string, stringLen);

	// Copy in blank padding
	memset(data + stringLen, 0, 30);

	unsigned int r = ptp_run_command(&params, data, stringLen + 30);

	close_camera(&ptp_usb, &params, dev);
	return r;
}
