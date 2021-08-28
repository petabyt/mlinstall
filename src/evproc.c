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

	char command[128];
	char buffer[1024];

	strcpy(buffer, string);
	char *s = strtok(buffer, " ");
	strcpy(command, s);

	// Parse base 10 arguments following command name
	s = strtok(NULL, " ");

	int p = 1;
	unsigned int iparam[6];
	while (s != NULL) {
		iparam[p] = atoi(s);
		s = strtok(NULL, " ");
		p++;
	}

	iparam[0] = p - 1;

	printf("Running '%s' with %d params...\n", command, p - 1);

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		return 0;
		return 1;
	}

	// Don't send parameters if there aren't any
	unsigned int r;
	if (p - 1 == 0) {
		r = ptp_runeventproc(&params, string, NULL);
	} else {
		r = ptp_runeventproc(&params, string, iparam);
	}

	close_camera(&ptp_usb, &params, dev);
	return r;
}
