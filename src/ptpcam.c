/* ptpcam.c
 *
 * Copyright (C) 2001-2005 Mariusz Woloszyn <emsi@ipartners.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "config.h"
#include "ptp.h"

#ifndef WIN32
#include <sys/mman.h>
#endif
#include <usb.h>

#ifdef WIN32
#define usleep(usec) Sleep((usec) / 1000)
#define sleep(sec) Sleep(sec * 1000)
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#undef _
#define _(String) dgettext(GETTEXT_PACKAGE, String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain, Message) (Message)
#define dcgettext(Domain, Message, Type) (Message)
#define bindtextdomain(Domain, Directory) (Domain)
#define _(String) (String)
#define N_(String) (String)
#endif

#include "ptpcam.h"

/* some defines comes here */

/* CHDK additions */
#define CHDKBUFS 65535
#define CHDK_MODE_INTERACTIVE 0
#define CHDK_MODE_CLI 1
#define MAXCONNRETRIES 10

/* USB interface class */
#ifndef USB_CLASS_PTP
#define USB_CLASS_PTP 6
#endif

/* USB control message data phase direction */
#ifndef USB_DP_HTD
#define USB_DP_HTD (0x00 << 7) /* host to device */
#endif
#ifndef USB_DP_DTH
#define USB_DP_DTH (0x01 << 7) /* device to host */
#endif

/* PTP class specific requests */
#ifndef USB_REQ_DEVICE_RESET
#define USB_REQ_DEVICE_RESET 0x66
#endif
#ifndef USB_REQ_GET_DEVICE_STATUS
#define USB_REQ_GET_DEVICE_STATUS 0x67
#endif

/* USB Feature selector HALT */
#ifndef USB_FEATURE_HALT
#define USB_FEATURE_HALT 0x00
#endif

/* OUR APPLICATION USB URB (2MB) ;) */
#define PTPCAM_USB_URB 2097152

#define USB_TIMEOUT 5000
#define USB_CAPTURE_TIMEOUT 20000

/* one global variable (yes, I know it sucks) */
short verbose = 0;
/* the other one, it sucks definitely ;) */
int ptpcam_usb_timeout = USB_TIMEOUT;

/* we need it for a proper signal handling :/ */
PTPParams *globalparams;

#define PTPBUF_MAGIC 0xEAEA3388
#define PTPBUF_MAGIC_REV 0x8833EAEA

typedef struct {
	uint32_t bytes_used;
	uint8_t data;
} ptpbuf_buffer_t;

typedef struct {
	uint32_t type;
	uint32_t length;
} ptpbuf_packet_t;

typedef struct {
	uint32_t magic;
	uint32_t commit;
	uint32_t buffer_count;
	uint32_t buffer_size;
	uint32_t current_buffer;
	uint32_t overflow;
	ptpbuf_buffer_t *buffers;
	uint32_t *fetchable;
} ptpbuf_t;

static short ptp_read_func(unsigned char *bytes, unsigned int size, void *data)
{
	int result = -1;
	PTP_USB *ptp_usb = (PTP_USB *)data;
	int toread = 0;
	signed long int rbytes = size;

	do {
		bytes += toread;
		if (rbytes > PTPCAM_USB_URB)
			toread = PTPCAM_USB_URB;
		else
			toread = rbytes;
		result = USB_BULK_READ(ptp_usb->handle, ptp_usb->inep, (char *)bytes, toread,
				       ptpcam_usb_timeout);
		/* sometimes retry might help */
		if (result == 0)
			result = USB_BULK_READ(ptp_usb->handle, ptp_usb->inep, (char *)bytes,
					       toread, ptpcam_usb_timeout);
		if (result < 0)
			break;
		rbytes -= PTPCAM_USB_URB;
	} while (rbytes > 0);

	if (result >= 0) {
		return (PTP_RC_OK);
	} else {
		if (verbose)
			perror("usb_bulk_read");
		return PTP_ERROR_IO;
	}
}

static short ptp_write_func(unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb = (PTP_USB *)data;

	result = USB_BULK_WRITE(ptp_usb->handle, ptp_usb->outep, (char *)bytes, size,
				ptpcam_usb_timeout);
	if (result >= 0)
		return (PTP_RC_OK);
	else {
		if (verbose)
			perror("usb_bulk_write");
		return PTP_ERROR_IO;
	}
}

/* XXX this one is suposed to return the number of bytes read!!! */
static short ptp_check_int(unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb = (PTP_USB *)data;

	result = USB_BULK_READ(ptp_usb->handle, ptp_usb->intep, (char *)bytes, size,
			       ptpcam_usb_timeout);
	if (result == 0)
		result = USB_BULK_READ(ptp_usb->handle, ptp_usb->intep, (char *)bytes, size,
				       ptpcam_usb_timeout);
	if (verbose > 2)
		fprintf(stderr, "USB_BULK_READ returned %i, size=%i\n", result, size);

	if (result >= 0) {
		return result;
	} else {
		if (verbose)
			perror("ptp_check_int");
		return result;
	}
}

void ptpcam_debug(void *data, const char *format, va_list args);
void ptpcam_debug(void *data, const char *format, va_list args)
{
	if (verbose < 2)
		return;
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
}

void ptpcam_error(void *data, const char *format, va_list args);
void ptpcam_error(void *data, const char *format, va_list args)
{
	/*	if (!verbose) return; */
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
}

void init_ptp_usb(PTPParams *params, PTP_USB *ptp_usb, struct usb_device *dev)
{
	usb_dev_handle *device_handle;

	params->write_func = ptp_write_func;
	params->read_func = ptp_read_func;
	params->check_int_func = ptp_check_int;
	params->check_int_fast_func = ptp_check_int;
	params->error_func = ptpcam_error;
	params->debug_func = ptpcam_debug;
	params->sendreq_func = ptp_usb_sendreq;
	params->senddata_func = ptp_usb_senddata;
	params->getresp_func = ptp_usb_getresp;
	params->getdata_func = ptp_usb_getdata;
	params->data = ptp_usb;
	params->transaction_id = 0;
	params->byteorder = PTP_DL_LE;

	if ((device_handle = usb_open(dev))) {
		if (!device_handle) {
			perror("usb_open()");
			exit(0);
		}
		ptp_usb->handle = device_handle;
		usb_set_configuration(device_handle, dev->config->bConfigurationValue);
		usb_claim_interface(device_handle,
				    dev->config->interface->altsetting->bInterfaceNumber);
	}
	globalparams = params;
}

void clear_stall(PTP_USB *ptp_usb)
{
	uint16_t status = 0;
	int ret;

	/* check the inep status */
	ret = usb_get_endpoint_status(ptp_usb, ptp_usb->inep, &status);
	if (ret < 0)
		perror("inep: usb_get_endpoint_status()");
	/* and clear the HALT condition if happend */
	else if (status) {
		printf("Resetting input pipe!\n");
		ret = usb_clear_stall_feature(ptp_usb, ptp_usb->inep);
		/*usb_clear_halt(ptp_usb->handle,ptp_usb->inep); */
		if (ret < 0)
			perror("usb_clear_stall_feature()");
	}
	status = 0;

	/* check the outep status */
	ret = usb_get_endpoint_status(ptp_usb, ptp_usb->outep, &status);
	if (ret < 0)
		perror("outep: usb_get_endpoint_status()");
	/* and clear the HALT condition if happend */
	else if (status) {
		printf("Resetting output pipe!\n");
		ret = usb_clear_stall_feature(ptp_usb, ptp_usb->outep);
		/*usb_clear_halt(ptp_usb->handle,ptp_usb->outep); */
		if (ret < 0)
			perror("usb_clear_stall_feature()");
	}

	/*usb_clear_halt(ptp_usb->handle,ptp_usb->intep); */
}

void close_usb(PTP_USB *ptp_usb, struct usb_device *dev)
{
	// clear_stall(ptp_usb);
	usb_release_interface(ptp_usb->handle,
			      dev->config->interface->altsetting->bInterfaceNumber);
	usb_reset(ptp_usb->handle);
	usb_close(ptp_usb->handle);
}

struct usb_bus *init_usb()
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	return (usb_get_busses());
}

/*
   find_device() returns the pointer to a usb_device structure matching
   given busn, devicen numbers. If any or both of arguments are 0 then the
   first matching PTP device structure is returned.
*/
struct usb_device *find_device(int busn, int devicen, short force);
struct usb_device *find_device(int busn, int devn, short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	bus = init_usb();
	for (; bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			if (dev->config)
				if ((dev->config->interface->altsetting->bInterfaceClass ==
				     USB_CLASS_PTP) ||
				    force)
					if (dev->descriptor.bDeviceClass != USB_CLASS_HUB) {
						int curbusn, curdevn;

						curbusn = strtol(bus->dirname, NULL, 10);
#ifdef WIN32
						curdevn = strtol(strchr(dev->filename, '-') + 1,
								 NULL, 10);
#else
						curdevn = strtol(dev->filename, NULL, 10);
#endif

						if (devn == 0) {
							if (busn == 0)
								return dev;
							if (curbusn == busn)
								return dev;
						} else {
							if ((busn == 0) && (curdevn == devn))
								return dev;
							if ((curbusn == busn) && (curdevn == devn))
								return dev;
						}
					}
	return NULL;
}

void find_endpoints(struct usb_device *dev, int *inep, int *outep, int *intep);
void find_endpoints(struct usb_device *dev, int *inep, int *outep, int *intep)
{
	int i, n;
	struct usb_endpoint_descriptor *ep;

	ep = dev->config->interface->altsetting->endpoint;
	n = dev->config->interface->altsetting->bNumEndpoints;

	for (i = 0; i < n; i++) {
		if (ep[i].bmAttributes == USB_ENDPOINT_TYPE_BULK) {
			if ((ep[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
			    USB_ENDPOINT_DIR_MASK) {
				*inep = ep[i].bEndpointAddress;
				if (verbose > 1)
					fprintf(stderr, "Found inep: 0x%02x\n", *inep);
			}
			if ((ep[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) == 0) {
				*outep = ep[i].bEndpointAddress;
				if (verbose > 1)
					fprintf(stderr, "Found outep: 0x%02x\n", *outep);
			}
		} else if ((ep[i].bmAttributes == USB_ENDPOINT_TYPE_INTERRUPT) &&
			   ((ep[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
			    USB_ENDPOINT_DIR_MASK)) {
			*intep = ep[i].bEndpointAddress;
			if (verbose > 1)
				fprintf(stderr, "Found intep: 0x%02x\n", *intep);
		}
	}
}

int open_camera(int busn, int devn, short force, PTP_USB *ptp_usb, PTPParams *params,
		struct usb_device **dev)
{
	int retrycnt = 0;
	uint16_t ret = 0;

#ifdef DEBUG
	printf("dev %i\tbus %i\n", devn, busn);
#endif

	// retry device find for a while (in case the user just powered it on or
	// called restart)
	while ((retrycnt++ < MAXCONNRETRIES) && !ret) {
		*dev = find_device(busn, devn, force);
		if (*dev != NULL)
			ret = 1;
		else {
			puts("Could not find a USB/PTP device.");
			return -1;
		}
	}

	if (*dev == NULL) {
		fprintf(stderr, "could not find any device matching given "
				"bus/dev numbers\n");
		return -1;
	}
	find_endpoints(*dev, &ptp_usb->inep, &ptp_usb->outep, &ptp_usb->intep);
	init_ptp_usb(params, ptp_usb, *dev);

	// first connection attempt often fails if some other app or driver has
	// accessed the camera, retry for a while
	retrycnt = 0;
	while ((retrycnt++ < MAXCONNRETRIES) && ((ret = ptp_opensession(params, 1)) != PTP_RC_OK)) {
		printf("Failed to connect (attempt %d), retrying in 1 s...\n", retrycnt);
		close_usb(ptp_usb, *dev);
		sleep(1);
		find_endpoints(*dev, &ptp_usb->inep, &ptp_usb->outep, &ptp_usb->intep);
		init_ptp_usb(params, ptp_usb, *dev);
	}
	if (ret != PTP_RC_OK) {
		fprintf(stderr, "ERROR: Could not open session!\n");
		close_usb(ptp_usb, *dev);
		return -1;
	}

	if (ptp_getdeviceinfo(params, &params->deviceinfo) != PTP_RC_OK) {
		fprintf(stderr, "ERROR: Could not get device info!\n");
		close_usb(ptp_usb, *dev);
		return -1;
	}
	return 0;
}

void close_camera(PTP_USB *ptp_usb, PTPParams *params, struct usb_device *dev)
{
	if (ptp_closesession(params) != PTP_RC_OK)
		fprintf(stderr, "ERROR: Could not close session!\n");
	close_usb(ptp_usb, dev);
}

void list_devices(short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	int found = 0;

	bus = init_usb();
	for (; bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next) {
			/* if it's a PTP device try to talk to it */
			if (dev->config)
				if ((dev->config->interface->altsetting->bInterfaceClass ==
				     USB_CLASS_PTP) ||
				    force)
					if (dev->descriptor.bDeviceClass != USB_CLASS_HUB) {
						PTPParams params;
						PTP_USB ptp_usb;
						PTPDeviceInfo deviceinfo;

						if (!found) {
							printf("\nListing devices...\n");
							printf("bus/dev\tvendorID/prodID\tdevice model\n");
							found = 1;
						}

						find_endpoints(dev, &ptp_usb.inep, &ptp_usb.outep,
							       &ptp_usb.intep);
						init_ptp_usb(&params, &ptp_usb, dev);

						CC(ptp_opensession(&params, 1),
						   "Could not open session!\n"
						   "Try to reset the camera.\n");
						CC(ptp_getdeviceinfo(&params, &deviceinfo),
						   "Could not get device info!\n");

						printf("%s/%s\t0x%04X/0x%04X\t%s\n", bus->dirname,
						       dev->filename, dev->descriptor.idVendor,
						       dev->descriptor.idProduct, deviceinfo.Model);

						CC(ptp_closesession(&params),
						   "Could not close session!\n");
						close_usb(&ptp_usb, dev);
					}
		}
	if (!found)
		printf("\nFound no PTP devices\n");
	printf("\n");
}

void show_info(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	printf("\nCamera information\n");
	printf("==================\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Model: %s\n", params.deviceinfo.Model);
	printf("  manufacturer: %s\n", params.deviceinfo.Manufacturer);
	printf("  serial number: '%s'\n", params.deviceinfo.SerialNumber);
	printf("  device version: %s\n", params.deviceinfo.DeviceVersion);
	printf("  extension ID: 0x%08lx\n", (long unsigned)params.deviceinfo.VendorExtensionID);
	printf("  extension description: %s\n", params.deviceinfo.VendorExtensionDesc);
	printf("  extension version: 0x%04x\n", params.deviceinfo.VendorExtensionVersion);
	printf("\n");
	close_camera(&ptp_usb, &params, dev);
}

void capture_image(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	PTPContainer event;
	int ExposureTime = 0;
	struct usb_device *dev;
	short ret;

	printf("\nInitiating captue...\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	if (!ptp_operation_issupported(&params, PTP_OC_InitiateCapture)) {
		printf("Your camera does not support InitiateCapture operation!\nSorry, "
		       "blame the %s!\n",
		       params.deviceinfo.Manufacturer);
		goto out;
	}

	/* obtain exposure time in miliseconds */
	if (ptp_property_issupported(&params, PTP_DPC_ExposureTime)) {
		PTPDevicePropDesc dpd;
		memset(&dpd, 0, sizeof(dpd));
		ret = ptp_getdevicepropdesc(&params, PTP_DPC_ExposureTime, &dpd);
		if (ret == PTP_RC_OK)
			ExposureTime = (*(int32_t *)(dpd.CurrentValue)) / 10;
	}

	/* adjust USB timeout */
	if (ExposureTime > USB_TIMEOUT)
		ptpcam_usb_timeout = ExposureTime;

	CR(ptp_initiatecapture(&params, 0x0, 0), "Could not capture.\n");

	ret = ptp_usb_event_wait(&params, &event);
	if (ret != PTP_RC_OK)
		goto err;
	if (verbose)
		printf("Event received %08x, ret=%x\n", event.Code, ret);
	if (event.Code == PTP_EC_CaptureComplete) {
		printf("Camera reported 'capture completed' but the object information is "
		       "missing.\n");
		goto out;
	}

	while (event.Code == PTP_EC_ObjectAdded) {
		printf("Object added 0x%08lx\n", (long unsigned)event.Param1);
		if (ptp_usb_event_wait(&params, &event) != PTP_RC_OK)
			goto err;
		if (verbose)
			printf("Event received %08x, ret=%x\n", event.Code, ret);
		if (event.Code == PTP_EC_CaptureComplete) {
			printf("Capture completed successfully!\n");
			goto out;
		}
	}

err:
	printf("Events receiving error. Capture status unknown.\n");
out:

	ptpcam_usb_timeout = USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}

void loop_capture(int busn, int devn, short force, int n, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	PTPContainer event;
	struct usb_device *dev;
	int file;
	PTPObjectInfo oi;
	uint32_t handle = 0;
	char *image;
	int ret;
	char *filename;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	/* capture timeout should be longer */
	ptpcam_usb_timeout = USB_CAPTURE_TIMEOUT;

	printf("Camera: %s\n", params.deviceinfo.Model);

	/* local loop */
	while (n > 0) {
		/* capture */
		printf("\nInitiating captue...\n");
		CR(ptp_initiatecapture(&params, 0x0, 0), "Could not capture\n");
		n--;

		ret = ptp_usb_event_wait(&params, &event);
		if (verbose)
			printf("Event received %08x, ret=%x\n", event.Code, ret);
		if (ret != PTP_RC_OK)
			goto err;
		if (event.Code == PTP_EC_CaptureComplete) {
			printf("CANNOT DOWNLOAD: got 'capture completed' but the object "
			       "information is missing.\n");
			goto out;
		}

		while (event.Code == PTP_EC_ObjectAdded) {
			printf("Object added 0x%08lx\n", (long unsigned)event.Param1);
			handle = event.Param1;
			if (ptp_usb_event_wait(&params, &event) != PTP_RC_OK)
				goto err;
			if (verbose)
				printf("Event received %08x, ret=%x\n", event.Code, ret);
			if (event.Code == PTP_EC_CaptureComplete)
				goto download;
		}
	download:

		memset(&oi, 0, sizeof(PTPObjectInfo));
		if (verbose)
			printf("Downloading: 0x%08lx\n", (long unsigned)handle);
		if ((ret = ptp_getobjectinfo(&params, handle, &oi)) != PTP_RC_OK) {
			fprintf(stderr, "ERROR: Could not get object info\n");
			ptp_perror(&params, ret);
			if (ret == PTP_ERROR_IO)
				clear_stall(&ptp_usb);
			continue;
		}

		if (oi.ObjectFormat == PTP_OFC_Association)
			goto out;
		filename = (oi.Filename);
#ifdef WIN32
		goto out;
#else
		file = open(filename,
			    (overwrite == OVERWRITE_EXISTING ? 0 : O_EXCL) | O_RDWR | O_CREAT |
				    O_TRUNC,
			    S_IRWXU | S_IRGRP);
#endif
		if (file == -1) {
			if (errno == EEXIST) {
				printf("Skipping file: \"%s\", file exists!\n", filename);
				goto out;
			}
			perror("open");
			goto out;
		}
		lseek(file, oi.ObjectCompressedSize - 1, SEEK_SET);
		ret = write(file, "", 1);
		if (ret == -1) {
			perror("write");
			goto out;
		}
#ifndef WIN32
		image = mmap(0, oi.ObjectCompressedSize, PROT_READ | PROT_WRITE, MAP_SHARED, file,
			     0);
		if (image == MAP_FAILED) {
			perror("mmap");
			close(file);
			goto out;
		}
#endif
		printf("Saving file: \"%s\" ", filename);
		fflush(NULL);
		ret = ptp_getobject(&params, handle, &image);
		munmap(image, oi.ObjectCompressedSize);
		close(file);
		if (ret != PTP_RC_OK) {
			printf("error!\n");
			ptp_perror(&params, ret);
			if (ret == PTP_ERROR_IO)
				clear_stall(&ptp_usb);
		} else {
			/* and delete from camera! */
			printf("is done...\nDeleting from camera.\n");
			CR(ptp_deleteobject(&params, handle, 0), "Could not delete object\n");
			printf("Object 0x%08lx (%s) deleted.\n", (long unsigned)handle,
			       oi.Filename);
		}
	out:;
	}
err:

	ptpcam_usb_timeout = USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}

void nikon_initiate_dc(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	printf("Camera: %s\n", params.deviceinfo.Model);
	printf("\nInitiating direct captue...\n");

	if (params.deviceinfo.VendorExtensionID != PTP_VENDOR_NIKON) {
		printf("Your camera is not Nikon!\nDo not buy from %s!\n",
		       params.deviceinfo.Manufacturer);
		goto out;
	}

	if (!ptp_operation_issupported(&params, PTP_OC_NIKON_DirectCapture)) {
		printf("Sorry, your camera dows not support Nikon DirectCapture!\nDo not "
		       "buy from %s!\n",
		       params.deviceinfo.Manufacturer);
		goto out;
	}

	/* perform direct capture */
	result = ptp_nikon_directcapture(&params, 0xffffffff);
	if (result != PTP_RC_OK) {
		ptp_perror(&params, result);
		fprintf(stderr, "ERROR: Could not capture.\n");
		if (result != PTP_RC_StoreFull) {
			close_camera(&ptp_usb, &params, dev);
			return;
		}
	}
	usleep(300 * 1000);
out:
	close_camera(&ptp_usb, &params, dev);
}

void nikon_direct_capture(int busn, int devn, short force, char *filename, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	uint16_t nevent = 0;
	PTPUSBEventContainer *events = NULL;
	int ExposureTime = 0; /* exposure time in miliseconds */
	int BurstNumber = 1;
	PTPDevicePropDesc dpd;
	PTPObjectInfo oi;
	int i;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	printf("Camera: %s\n", params.deviceinfo.Model);

	if ((result = ptp_getobjectinfo(&params, 0xffff0001, &oi)) == PTP_RC_OK) {
		if (filename == NULL)
			filename = oi.Filename;
		save_object(&params, 0xffff0001, filename, oi, overwrite);
		goto out;
	}

	printf("\nInitiating direct captue...\n");

	if (params.deviceinfo.VendorExtensionID != PTP_VENDOR_NIKON) {
		printf("Your camera is not Nikon!\nDo not buy from %s!\n",
		       params.deviceinfo.Manufacturer);
		goto out;
	}

	if (!ptp_operation_issupported(&params, PTP_OC_NIKON_DirectCapture)) {
		printf("Sorry, your camera dows not support Nikon DirectCapture!\nDo not "
		       "buy from %s!\n",
		       params.deviceinfo.Manufacturer);
		goto out;
	}

	/* obtain exposure time in miliseconds */
	memset(&dpd, 0, sizeof(dpd));
	result = ptp_getdevicepropdesc(&params, PTP_DPC_ExposureTime, &dpd);
	if (result == PTP_RC_OK)
		ExposureTime = (*(int32_t *)(dpd.CurrentValue)) / 10;

	/* obtain burst number */
	memset(&dpd, 0, sizeof(dpd));
	result = ptp_getdevicepropdesc(&params, PTP_DPC_BurstNumber, &dpd);
	if (result == PTP_RC_OK)
		BurstNumber = *(uint16_t *)(dpd.CurrentValue);
	/*
    if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))==PTP_RC_OK)
    {
        if (filename==NULL) filename=oi.Filename;
        save_object(&params, 0xffff0001, filename, oi, overwrite);
        ptp_nikon_keepalive(&params);
        ptp_nikon_keepalive(&params);
        ptp_nikon_keepalive(&params);
        ptp_nikon_keepalive(&params);
    }
*/

	/* perform direct capture */
	result = ptp_nikon_directcapture(&params, 0xffffffff);
	if (result != PTP_RC_OK) {
		ptp_perror(&params, result);
		fprintf(stderr, "ERROR: Could not capture.\n");
		if (result != PTP_RC_StoreFull) {
			close_camera(&ptp_usb, &params, dev);
			return;
		}
	}
	if (BurstNumber > 1)
		printf("Capturing %i frames in burst.\n", BurstNumber);

	/* sleep in case of exposure longer than 1/100 */
	if (ExposureTime > 10) {
		printf("sleeping %i miliseconds\n", ExposureTime);
		usleep(ExposureTime * 1000);
	}

	while (BurstNumber > 0) {
#if 0 /* Is this really needed??? */
	    ptp_nikon_keepalive(&params);
#endif

		result = ptp_nikon_checkevent(&params, &events, &nevent);
		if (result != PTP_RC_OK) {
			fprintf(stderr, "Error checking Nikon events\n");
			ptp_perror(&params, result);
			goto out;
		}
		for (i = 0; i < nevent; i++) {
			ptp_nikon_keepalive(&params);
			void *prop;
			if (events[i].code == PTP_EC_DevicePropChanged) {
				printf("Checking: %s\n",
				       ptp_prop_getname(&params, events[i].param1));
				ptp_getdevicepropvalue(&params, events[i].param1, &prop,
						       PTP_DTC_UINT64);
			}

			printf("Event [%i] = 0x%04x,\t param: %08x\n", i, events[i].code,
			       events[i].param1);
			if (events[i].code == PTP_EC_NIKON_CaptureOverflow) {
				printf("Ram cache overflow? Shooting to fast!\n");
				if ((result = ptp_getobjectinfo(&params, 0xffff0001, &oi)) !=
				    PTP_RC_OK) {
					fprintf(stderr, "Could not get object info\n");
					ptp_perror(&params, result);
					goto out;
				}
				if (filename == NULL)
					filename = oi.Filename;
				save_object(&params, 0xffff0001, filename, oi, overwrite);
				BurstNumber = 0;
				usleep(100);
			} else if (events[i].code == PTP_EC_NIKON_ObjectReady) {
				if ((result = ptp_getobjectinfo(&params, 0xffff0001, &oi)) !=
				    PTP_RC_OK) {
					fprintf(stderr, "Could not get object info\n");
					ptp_perror(&params, result);
					goto out;
				}
				if (filename == NULL)
					filename = oi.Filename;
				save_object(&params, 0xffff0001, filename, oi, overwrite);
				BurstNumber--;
			}
		}
		free(events);
	}

out:
	ptpcam_usb_timeout = USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}

void nikon_direct_capture2(int busn, int devn, short force, char *filename, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	PTPObjectInfo oi;

	dev = find_device(busn, devn, force);
	if (dev == NULL) {
		fprintf(stderr, "could not find any device matching given "
				"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev, &ptp_usb.inep, &ptp_usb.outep, &ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);

	if (ptp_opensession(&params, 1) != PTP_RC_OK) {
		fprintf(stderr, "ERROR: Could not open session!\n");
		close_usb(&ptp_usb, dev);
		return;
	}
	/*
    memset(&dpd,0,sizeof(dpd));
    result=ptp_getdevicepropdesc(&params,PTP_DPC_BurstNumber,&dpd);
    memset(&dpd,0,sizeof(dpd));
    result=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
*/

	/* perform direct capture */
	result = ptp_nikon_directcapture(&params, 0xffffffff);
	if (result != PTP_RC_OK) {
		ptp_perror(&params, result);
		fprintf(stderr, "ERROR: Could not capture.\n");
		if (result != PTP_RC_StoreFull) {
			close_camera(&ptp_usb, &params, dev);
			return;
		}
	}

	if (ptp_closesession(&params) != PTP_RC_OK) {
		fprintf(stderr, "ERROR: Could not close session!\n");
		return;
	}

	usleep(300 * 1000);

	if (ptp_opensession(&params, 1) != PTP_RC_OK) {
		fprintf(stderr, "ERROR: Could not open session!\n");
		close_usb(&ptp_usb, dev);
		return;
	}
loop:
	if ((result = ptp_getobjectinfo(&params, 0xffff0001, &oi)) == PTP_RC_OK) {
		if (filename == NULL)
			filename = oi.Filename;
		save_object(&params, 0xffff0001, filename, oi, overwrite);
	} else {
		ptp_nikon_keepalive(&params);
		goto loop;
	}

	/*out:	*/
	close_camera(&ptp_usb, &params, dev);

#if 0
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	uint16_t nevent=0;
	PTPUSBEventContainer* events=NULL;
	int ExposureTime=0;	/* exposure time in miliseconds */
	int BurstNumber=1;
	PTPDevicePropDesc dpd;
	PTPObjectInfo oi;
	int i;
	char *filename=NULL;
    
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s\n",params.deviceinfo.Model);
	printf("\nInitiating direct captue...\n");
	
	if (params.deviceinfo.VendorExtensionID!=PTP_VENDOR_NIKON)
	{
	    printf ("Your camera is not Nikon!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	if (!ptp_operation_issupported(&params,PTP_OC_NIKON_DirectCapture)) {
	    printf ("Sorry, your camera dows not support Nikon DirectCapture!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	/* obtain exposure time in miliseconds */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
	if (result==PTP_RC_OK) ExposureTime=(*(int32_t*)(dpd.CurrentValue))/10;

	/* obtain burst number */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_BurstNumber,&dpd);
	if (result==PTP_RC_OK) BurstNumber=*(uint16_t*)(dpd.CurrentValue);

	if (BurstNumber>1) printf("Capturing %i frames in burst.\n",BurstNumber);
#if 0
	/* sleep in case of exposure longer than 1/100 */
	if (ExposureTime>10) {
	    printf ("sleeping %i miliseconds\n", ExposureTime);
	    usleep (ExposureTime*1000);
	}
#endif

	while (num>0) {
	    /* perform direct capture */
	    result=ptp_nikon_directcapture (&params, 0xffffffff);
	    if (result!=PTP_RC_OK) {
	        if (result==PTP_ERROR_IO) {
	    	close_camera(&ptp_usb, &params, dev);
	    	return;
	        }
	    }

#if 0 /* Is this really needed??? */
	    ptp_nikon_keepalive(&params);
#endif
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);

	    result=ptp_nikon_checkevent (&params, &events, &nevent);
	    if (result != PTP_RC_OK) goto out;
	    
	    for(i=0;i<nevent;i++) {
		printf("Event [%i] = 0x%04x,\t param: %08x\n",i, events[i].code ,events[i].param1);
		if (events[i].code==PTP_EC_NIKON_ObjectReady) 
		{
		    num--;
		    if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))!=PTP_RC_OK) {
		        fprintf(stderr, "Could not get object info\n");
		        ptp_perror(&params,result);
		        goto out;
		    }
		    if (filename==NULL) filename=oi.Filename;
		    save_object(&params, 0xffff0001, filename, oi, overwrite);
		}
		if (events[i].code==PTP_EC_NIKON_CaptureOverflow) {
		    printf("Ram cache overflow, capture terminated\n");
		    //BurstNumber=0;
		}
	    }
	    free (events);
	}

out:	
	ptpcam_usb_timeout=USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
#endif
}

void list_files(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;
	PTPObjectInfo oi;
	struct tm *tm;

	printf("\nListing files...\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Camera: %s\n", params.deviceinfo.Model);
	CR(ptp_getobjecthandles(&params, 0xffffffff, 0x000000, 0x000000, &params.handles),
	   "Could not get object handles\n");
	printf("Handler:           Size: \tCaptured:      \tname:\n");
	for (i = 0; i < params.handles.n; i++) {
		CR(ptp_getobjectinfo(&params, params.handles.Handler[i], &oi),
		   "Could not get object info\n");
		if (oi.ObjectFormat == PTP_OFC_Association)
			continue;
		tm = gmtime(&oi.CaptureDate);
		printf("0x%08lx: %12u\t%4i-%02i-%02i %02i:%02i\t%s\n",
		       (long unsigned)params.handles.Handler[i], (unsigned)oi.ObjectCompressedSize,
		       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		       oi.Filename);
	}
	printf("\n");
	close_camera(&ptp_usb, &params, dev);
}

void delete_object(int busn, int devn, short force, uint32_t handle)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPObjectInfo oi;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	CR(ptp_getobjectinfo(&params, handle, &oi), "Could not get object info\n");
	CR(ptp_deleteobject(&params, handle, 0), "Could not delete object\n");
	printf("\nObject 0x%08lx (%s) deleted.\n", (long unsigned)handle, oi.Filename);
	close_camera(&ptp_usb, &params, dev);
}

void delete_all_files(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPObjectInfo oi;
	uint32_t handle;
	int i;
	int ret;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Camera: %s\n", params.deviceinfo.Model);
	CR(ptp_getobjecthandles(&params, 0xffffffff, 0x000000, 0x000000, &params.handles),
	   "Could not get object handles\n");

	for (i = 0; i < params.handles.n; i++) {
		handle = params.handles.Handler[i];
		if ((ret = ptp_getobjectinfo(&params, handle, &oi)) != PTP_RC_OK) {
			fprintf(stderr, "Handle: 0x%08lx\n", (long unsigned)handle);
			fprintf(stderr, "ERROR: Could not get object info\n");
			ptp_perror(&params, ret);
			if (ret == PTP_ERROR_IO)
				clear_stall(&ptp_usb);
			continue;
		}
		if (oi.ObjectFormat == PTP_OFC_Association)
			continue;
		CR(ptp_deleteobject(&params, handle, 0), "Could not delete object\n");
		printf("Object 0x%08lx (%s) deleted.\n", (long unsigned)handle, oi.Filename);
	}
	close_camera(&ptp_usb, &params, dev);
}

void save_object(PTPParams *params, uint32_t handle, char *filename, PTPObjectInfo oi,
		 int overwrite)
{
	int file;
	char *image;
	int ret;
	struct utimbuf timebuf;

#ifdef WIN32
	goto out;
#else
	file = open(filename,
		    (overwrite == OVERWRITE_EXISTING ? 0 : O_EXCL) | O_RDWR | O_CREAT | O_TRUNC,
		    S_IRWXU | S_IRGRP);
#endif
	if (file == -1) {
		if (errno == EEXIST) {
			printf("Skipping file: \"%s\", file exists!\n", filename);
			goto out;
		}
		perror("open");
		goto out;
	}
	lseek(file, oi.ObjectCompressedSize - 1, SEEK_SET);
	ret = write(file, "", 1);
	if (ret == -1) {
		perror("write");
		goto out;
	}
#ifndef WIN32
	image = mmap(0, oi.ObjectCompressedSize, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	if (image == MAP_FAILED) {
		perror("mmap");
		close(file);
		goto out;
	}
#endif
	printf("Saving file: \"%s\" ", filename);
	fflush(NULL);
	ret = ptp_getobject(params, handle, &image);
	munmap(image, oi.ObjectCompressedSize);
	if (close(file) == -1) {
		perror("close");
	}
	timebuf.actime = oi.ModificationDate;
	timebuf.modtime = oi.CaptureDate;
	utime(filename, &timebuf);
	if (ret != PTP_RC_OK) {
		printf("error!\n");
		ptp_perror(params, ret);
		if (ret == PTP_ERROR_IO)
			clear_stall((PTP_USB *)(params->data));
	} else {
		printf("is done.\n");
	}
out:
	return;
}

void get_save_object(PTPParams *params, uint32_t handle, char *filename, int overwrite)
{
	PTPObjectInfo oi;
	int ret;

	memset(&oi, 0, sizeof(PTPObjectInfo));
	if (verbose)
		printf("Handle: 0x%08lx\n", (long unsigned)handle);
	if ((ret = ptp_getobjectinfo(params, handle, &oi)) != PTP_RC_OK) {
		fprintf(stderr, "Could not get object info\n");
		ptp_perror(params, ret);
		if (ret == PTP_ERROR_IO)
			clear_stall((PTP_USB *)(params->data));
		goto out;
	}
	if (oi.ObjectFormat == PTP_OFC_Association)
		goto out;
	if (filename == NULL)
		filename = (oi.Filename);

	save_object(params, handle, filename, oi, overwrite);
out:
	return;
}

void get_file(int busn, int devn, short force, uint32_t handle, char *filename, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Camera: %s\n", params.deviceinfo.Model);

	get_save_object(&params, handle, filename, overwrite);

	close_camera(&ptp_usb, &params, dev);
}

void get_all_files(int busn, int devn, short force, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Camera: %s\n", params.deviceinfo.Model);

	CR(ptp_getobjecthandles(&params, 0xffffffff, 0x000000, 0x000000, &params.handles),
	   "Could not get object handles\n");

	for (i = 0; i < params.handles.n; i++) {
		get_save_object(&params, params.handles.Handler[i], NULL, overwrite);
	}
	close_camera(&ptp_usb, &params, dev);
}

void list_operations(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;
	const char *name;

	printf("\nListing supported operations...\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
	printf("Camera: %s\n", params.deviceinfo.Model);
	for (i = 0; i < params.deviceinfo.OperationsSupported_len; i++) {
		name = ptp_get_operation_name(&params, params.deviceinfo.OperationsSupported[i]);

		if (name == NULL)
			printf("  0x%04x: UNKNOWN\n", params.deviceinfo.OperationsSupported[i]);
		else
			printf("  0x%04x: %s\n", params.deviceinfo.OperationsSupported[i], name);
	}
	close_camera(&ptp_usb, &params, dev);
}

void list_properties(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	const char *propname;
	int i;

	printf("\nListing properties...\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;
/* XXX */
#if 0
	CR(ptp_nikon_setcontrolmode(&params, 0x01),
		"Unable to set Nikon PC controll mode\n");
#endif
	printf("Camera: %s\n", params.deviceinfo.Model);
	for (i = 0; i < params.deviceinfo.DevicePropertiesSupported_len; i++) {
		propname =
			ptp_prop_getname(&params, params.deviceinfo.DevicePropertiesSupported[i]);
		if (propname != NULL)
			printf("  0x%04x: %s\n", params.deviceinfo.DevicePropertiesSupported[i],
			       propname);
		else
			printf("  0x%04x: UNKNOWN\n",
			       params.deviceinfo.DevicePropertiesSupported[i]);
	}
	close_camera(&ptp_usb, &params, dev);
}

short print_propval(uint16_t datatype, void *value, short hex);
short print_propval(uint16_t datatype, void *value, short hex)
{
	switch (datatype) {
	case PTP_DTC_INT8:
		printf("%hhi", *(char *)value);
		return 0;
	case PTP_DTC_UINT8:
		printf("%hhu", *(unsigned char *)value);
		return 0;
	case PTP_DTC_INT16:
		printf("%hi", *(int16_t *)value);
		return 0;
	case PTP_DTC_UINT16:
		if (hex == PTPCAM_PRINT_HEX)
			printf("0x%04hX (%hi)", *(uint16_t *)value, *(uint16_t *)value);
		else
			printf("%hi", *(uint16_t *)value);
		return 0;
	case PTP_DTC_INT32:
		printf("%li", (long int)*(int32_t *)value);
		return 0;
	case PTP_DTC_UINT32:
		if (hex == PTPCAM_PRINT_HEX)
			printf("0x%08lX (%lu)", (long unsigned)*(uint32_t *)value,
			       (long unsigned)*(uint32_t *)value);
		else
			printf("%lu", (long unsigned)*(uint32_t *)value);
		return 0;
	case PTP_DTC_STR:
		printf("\"%s\"", (char *)value);
	}
	return -1;
}

uint16_t set_property(PTPParams *params, uint16_t property, const char *value, uint16_t datatype);
uint16_t set_property(PTPParams *params, uint16_t property, const char *value, uint16_t datatype)
{
	void *val = NULL;

	switch (datatype) {
	case PTP_DTC_INT8:
		val = malloc(sizeof(int8_t));
		*(int8_t *)val = (int8_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_UINT8:
		val = malloc(sizeof(uint8_t));
		*(uint8_t *)val = (uint8_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_INT16:
		val = malloc(sizeof(int16_t));
		*(int16_t *)val = (int16_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_UINT16:
		val = malloc(sizeof(uint16_t));
		*(uint16_t *)val = (uint16_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_INT32:
		val = malloc(sizeof(int32_t));
		*(int32_t *)val = (int32_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_UINT32:
		val = malloc(sizeof(uint32_t));
		*(uint32_t *)val = (uint32_t)strtol(value, NULL, 0);
		break;
	case PTP_DTC_STR:
		val = (void *)value;
	}
	return (ptp_setdevicepropvalue(params, property, val, datatype));
	free(val);
	return 0;
}
void getset_property_internal(PTPParams *params, uint16_t property, const char *value);
void getset_property_internal(PTPParams *params, uint16_t property, const char *value)
{
	PTPDevicePropDesc dpd;
	const char *propname;
	const char *propdesc;
	uint16_t result;

	memset(&dpd, 0, sizeof(dpd));
	result = ptp_getdevicepropdesc(params, property, &dpd);
	if (result != PTP_RC_OK) {
		ptp_perror(params, result);
		fprintf(stderr, "ERROR: "
				"Could not get device property description!\n"
				"Try to reset the camera.\n");
		return;
	}
	/* until this point dpd has to be free()ed */
	propdesc = ptp_prop_getdesc(params, &dpd, NULL);
	propname = ptp_prop_getname(params, property);

	if (value == NULL) { /* property GET */
		if (!verbose) { /* short output, default */
			printf("'%s' is set to: ", propname == NULL ? "UNKNOWN" : propname);
			if (propdesc != NULL)
				printf("[%s]", propdesc);
			else {
				if (dpd.FormFlag == PTP_DPFF_Enumeration)
					PRINT_PROPVAL_HEX(dpd.CurrentValue);
				else
					PRINT_PROPVAL_DEC(dpd.CurrentValue);
			}
			printf("\n");
		} else { /* verbose output */

			printf("%s: [0x%04x, ", propname == NULL ? "UNKNOWN" : propname, property);
			if (dpd.GetSet == PTP_DPGS_Get)
				printf("readonly, ");
			else
				printf("readwrite, ");
			printf("%s] ", ptp_get_datatype_name(params, dpd.DataType));

			printf("\n  Current value: ");
			if (dpd.FormFlag == PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else
				PRINT_PROPVAL_DEC(dpd.CurrentValue);

			if (propdesc != NULL)
				printf(" [%s]", propdesc);
			printf("\n  Factory value: ");
			if (dpd.FormFlag == PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.FactoryDefaultValue);
			else
				PRINT_PROPVAL_DEC(dpd.FactoryDefaultValue);
			propdesc = ptp_prop_getdesc(params, &dpd, dpd.FactoryDefaultValue);
			if (propdesc != NULL)
				printf(" [%s]", propdesc);
			printf("\n");

			switch (dpd.FormFlag) {
			case PTP_DPFF_Enumeration: {
				int i;
				printf("Enumerated:\n");
				for (i = 0; i < dpd.FORM.Enum.NumberOfValues; i++) {
					PRINT_PROPVAL_HEX(dpd.FORM.Enum.SupportedValue[i]);
					propdesc = ptp_prop_getdesc(
						params, &dpd, dpd.FORM.Enum.SupportedValue[i]);
					if (propdesc != NULL)
						printf("\t[%s]", propdesc);
					printf("\n");
				}
			} break;
			case PTP_DPFF_Range:
				printf("Range [");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MinimumValue);
				printf(" - ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MaximumValue);
				printf("; step ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.StepSize);
				printf("]\n");
				break;
			case PTP_DPFF_None:
				break;
			}
		}
	} else {
		uint16_t r;
		propdesc = ptp_prop_getdesc(params, &dpd, NULL);
		printf("'%s' is set to: ", propname == NULL ? "UNKNOWN" : propname);
		if (propdesc != NULL)
			printf("[%s]", propdesc);
		else {
			if (dpd.FormFlag == PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else
				PRINT_PROPVAL_DEC(dpd.CurrentValue);
		}
		printf("\n");

		propdesc = ptp_prop_getdescbystring(params, &dpd, value);
		/*
        if (propdesc==NULL) {
                fprintf(stderr, "ERROR: Unable to set property to
unidentified value: '%s'\n", value); goto out;
        }
*/
		printf("Changing property value to %s [%s] ", value, propdesc);
		r = (set_property(params, property, value, dpd.DataType));
		if (r != PTP_RC_OK) {
			printf("FAILED!!!\n");
			fflush(NULL);
			ptp_perror(params, r);
		} else
			printf("succeeded.\n");
	}
	/*	out: */

	ptp_free_devicepropdesc(&dpd);
}

void getset_propertybyname(int busn, int devn, char *property, char *value, short force);
void getset_propertybyname(int busn, int devn, char *property, char *value, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	char *p;
	uint16_t dpc;
	const char *propval = NULL;

	printf("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	printf("Camera: %s", params.deviceinfo.Model);
	if ((devn != 0) || (busn != 0))
		printf(" (bus %i, dev %i)\n", busn, devn);
	else
		printf("\n");

	if (property == NULL) {
		fprintf(stderr, "ERROR: no such property\n");
		return;
	}

	/*
1. change all '-' in property and value to ' '
2. change all '  ' in property and value to '-'
3. get property code by name
4. get value code by name
5. set property
*/
	while ((p = strchr(property, '-')) != NULL) {
		*p = ' ';
	}

	dpc = ptp_prop_getcodebyname(&params, property);
	if (dpc == 0) {
		fprintf(stderr, "ERROR: Could not find property '%s'\n", property);
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	if (!ptp_property_issupported(&params, dpc)) {
		fprintf(stderr, "The device does not support this property!\n");
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	if (value != NULL) {
		while ((p = strchr(value, '-')) != NULL) {
			*p = ' ';
		}
		propval = ptp_prop_getvalbyname(&params, value, dpc);
		if (propval == NULL)
			propval = value;
	}

	getset_property_internal(&params, dpc, propval);

	close_camera(&ptp_usb, &params, dev);
}
void getset_property(int busn, int devn, uint16_t property, char *value, short force);
void getset_property(int busn, int devn, uint16_t property, char *value, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	printf("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	printf("Camera: %s", params.deviceinfo.Model);
	if ((devn != 0) || (busn != 0))
		printf(" (bus %i, dev %i)\n", busn, devn);
	else
		printf("\n");
	if (!ptp_property_issupported(&params, property)) {
		fprintf(stderr, "The device does not support this property!\n");
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	getset_property_internal(&params, property, value);
#if 0
	memset(&dpd,0,sizeof(dpd));
	CR(ptp_getdevicepropdesc(&params,property,&dpd),
		"Could not get device property description!\n"
		"Try to reset the camera.\n");
	propdesc= ptp_prop_getdesc(&params, &dpd, NULL);
	propname=ptp_prop_getname(&params,property);

	if (value==NULL) { /* property GET */
		if (!verbose) { /* short output, default */
			printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
			if (propdesc!=NULL)
				printf("%s [%s]", ptp_prop_tostr(&params, &dpd,
							NULL), propdesc);
			else 
			{
				if (dpd.FormFlag==PTP_DPFF_Enumeration)
					PRINT_PROPVAL_HEX(dpd.CurrentValue);
				else 
					PRINT_PROPVAL_DEC(dpd.CurrentValue);
			}
			printf("\n");
	
		} else { /* verbose output */
	
			printf("%s: [0x%04x, ",propname==NULL?"UNKNOWN":propname,
					property);
			if (dpd.GetSet==PTP_DPGS_Get)
				printf ("readonly, ");
			else
				printf ("readwrite, ");
			printf ("%s] ",
				ptp_get_datatype_name(&params, dpd.DataType));

			printf ("\n  Current value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);

			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf ("\n  Factory value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.FactoryDefaultValue);
			else 
				PRINT_PROPVAL_DEC(dpd.FactoryDefaultValue);
			propdesc=ptp_prop_getdesc(&params, &dpd,
						dpd.FactoryDefaultValue);
			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf("\n");

			switch (dpd.FormFlag) {
			case PTP_DPFF_Enumeration:
				{
					int i;
					printf ("Enumerated:\n");
					for(i=0;i<dpd.FORM.Enum.NumberOfValues;i++){
						PRINT_PROPVAL_HEX(
						dpd.FORM.Enum.SupportedValue[i]);
						propdesc=ptp_prop_getdesc(&params, &dpd, dpd.FORM.Enum.SupportedValue[i]);
						if (propdesc!=NULL) printf("\t[%s]", propdesc);
						printf("\n");
					}
				}
				break;
			case PTP_DPFF_Range:
				printf ("Range [");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MinimumValue);
				printf(" - ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MaximumValue);
				printf("; step ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.StepSize);
				printf("]\n");
				break;
			case PTP_DPFF_None:
				break;
			}
		}
	} else {
		uint16_t r;
		propdesc= ptp_prop_getdesc(&params, &dpd, NULL);
		printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
		if (propdesc!=NULL)
			printf("%s [%s]", ptp_prop_tostr(&params, &dpd, NULL), propdesc);
		else
		{
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);
		}
		printf("\n");
		printf("Changing property value to '%s' ",value);
		r=(set_property(&params, property, value, dpd.DataType));
		if (r!=PTP_RC_OK)
		{
			printf ("FAILED!!!\n");
			fflush(NULL);
		        ptp_perror(&params,r);
		}
		else 
			printf ("succeeded.\n");
	}
	ptp_free_devicepropdesc(&dpd);
#endif

	close_camera(&ptp_usb, &params, dev);
}

void show_all_properties(int busn, int devn, short force, int unknown);
void show_all_properties(int busn, int devn, short force, int unknown)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPDevicePropDesc dpd;
	const char *propname;
	const char *propdesc;
	int i;

	printf("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0)
		return;

	printf("Camera: %s", params.deviceinfo.Model);
	if ((devn != 0) || (busn != 0))
		printf(" (bus %i, dev %i)\n", busn, devn);
	else
		printf("\n");

	for (i = 0; i < params.deviceinfo.DevicePropertiesSupported_len; i++) {
		propname =
			ptp_prop_getname(&params, params.deviceinfo.DevicePropertiesSupported[i]);
		if ((unknown) && (propname != NULL))
			continue;

		printf("0x%04x: ", params.deviceinfo.DevicePropertiesSupported[i]);
		memset(&dpd, 0, sizeof(dpd));
		CR(ptp_getdevicepropdesc(&params, params.deviceinfo.DevicePropertiesSupported[i],
					 &dpd),
		   "Could not get device property description!\n"
		   "Try to reset the camera.\n");
		propdesc = ptp_prop_getdesc(&params, &dpd, NULL);

		PRINT_PROPVAL_HEX(dpd.CurrentValue);
		if (verbose) {
			printf(" (%s", propname == NULL ? "UNKNOWN" : propname);
			if (propdesc != NULL)
				printf(": %s)", propdesc);
			else
				printf(")");
		}

		printf("\n");
		ptp_free_devicepropdesc(&dpd);
	}

	close_camera(&ptp_usb, &params, dev);
}

int usb_get_endpoint_status(PTP_USB *ptp_usb, int ep, uint16_t *status)
{
	return (usb_control_msg(ptp_usb->handle, USB_DP_DTH | USB_RECIP_ENDPOINT,
				USB_REQ_GET_STATUS, USB_FEATURE_HALT, ep, (char *)status, 2, 3000));
}

int usb_clear_stall_feature(PTP_USB *ptp_usb, int ep)
{
	return (usb_control_msg(ptp_usb->handle, USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,
				USB_FEATURE_HALT, ep, NULL, 0, 3000));
}

int usb_ptp_get_device_status(PTP_USB *ptp_usb, uint16_t *devstatus);
int usb_ptp_get_device_status(PTP_USB *ptp_usb, uint16_t *devstatus)
{
	return (usb_control_msg(ptp_usb->handle, USB_DP_DTH | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				USB_REQ_GET_DEVICE_STATUS, 0, 0, (char *)devstatus, 4, 3000));
}

int usb_ptp_device_reset(PTP_USB *ptp_usb);
int usb_ptp_device_reset(PTP_USB *ptp_usb)
{
	return (usb_control_msg(ptp_usb->handle, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				USB_REQ_DEVICE_RESET, 0, 0, NULL, 0, 3000));
}

void reset_device(int busn, int devn, short force);
void reset_device(int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t status;
	uint16_t devstatus[2] = { 0, 0 };
	int ret;

#ifdef DEBUG
	printf("dev %i\tbus %i\n", devn, busn);
#endif
	dev = find_device(busn, devn, force);
	if (dev == NULL) {
		fprintf(stderr, "could not find any device matching given "
				"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev, &ptp_usb.inep, &ptp_usb.outep, &ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);

	/* get device status (devices likes that regardless of its result)*/
	usb_ptp_get_device_status(&ptp_usb, devstatus);

	/* check the in endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb, ptp_usb.inep, &status);
	if (ret < 0)
		perror("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf("Resetting input pipe!\n");
		ret = usb_clear_stall_feature(&ptp_usb, ptp_usb.inep);
		if (ret < 0)
			perror("usb_clear_stall_feature()");
	}
	status = 0;
	/* check the out endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb, ptp_usb.outep, &status);
	if (ret < 0)
		perror("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf("Resetting output pipe!\n");
		ret = usb_clear_stall_feature(&ptp_usb, ptp_usb.outep);
		if (ret < 0)
			perror("usb_clear_stall_feature()");
	}
	status = 0;
	/* check the interrupt endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb, ptp_usb.intep, &status);
	if (ret < 0)
		perror("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf("Resetting interrupt pipe!\n");
		ret = usb_clear_stall_feature(&ptp_usb, ptp_usb.intep);
		if (ret < 0)
			perror("usb_clear_stall_feature()");
	}

	/* get device status (now there should be some results)*/
	ret = usb_ptp_get_device_status(&ptp_usb, devstatus);
	if (ret < 0)
		perror("usb_ptp_get_device_status()");
	else {
		if (devstatus[1] == PTP_RC_OK)
			printf("Device status OK\n");
		else
			printf("Device status 0x%04x\n", devstatus[1]);
	}

	/* finally reset the device (that clears prevoiusly opened sessions)*/
	ret = usb_ptp_device_reset(&ptp_usb);
	if (ret < 0)
		perror("usb_ptp_device_reset()");
	/* get device status (devices likes that regardless of its result)*/
	usb_ptp_get_device_status(&ptp_usb, devstatus);

	close_usb(&ptp_usb, dev);
}

int chdk(int busn, int devn, short force);
uint8_t chdkmode = 0;
char chdkarg[CHDKBUFS];

static int camera_bus = 0;
static int camera_dev = 0;
static int camera_force = 0;
static PTP_USB ptp_usb;
static PTPParams params;
static struct usb_device *dev;
static int connected = 0;

static void open_connection()
{
	connected =
		(0 == open_camera(camera_bus, camera_dev, camera_force, &ptp_usb, &params, &dev));
	if (connected) {
		int major, minor;
		if (!ptp_chdk_get_version(&params, &params.deviceinfo, &major, &minor)) {
			printf("error: cannot get camera CHDK PTP version; either it has an "
			       "unsupported version or no CHDK PTP support at all\n");
		} else if (major != PTP_CHDK_VERSION_MAJOR || minor < PTP_CHDK_VERSION_MINOR) {
			printf("error: camera has unsupported camera version %i.%i; some "
			       "functionality may be missing or cause unintented consequences\n",
			       major, minor);
		}
	}
}

static void close_connection()
{
	close_camera(&ptp_usb, &params, dev);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

static void reset_connection()
{
	if (connected) {
		close_connection();
	}
	open_connection();
}

static void print_safe(char *buf, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		if (buf[i] < ' ' || buf[i] > '~') {
			printf(".");
		} else {
			printf("%c", buf[i]);
		}
	}
}

static void hexdump(char *buf, unsigned int size, unsigned int offset)
{
	unsigned int start_offset = offset;
	unsigned int i;
	char s[16];

	if (offset % 16 != 0) {
		printf("0x%08X (+0x%04X)  ", offset, offset - start_offset);
		for (i = 0; i < (offset % 16); i++) {
			printf("   ");
		}
		if (offset % 16 > 8) {
			printf(" ");
		}
		memset(s, ' ', offset % 16);
	}
	for (i = 0;; i++, offset++) {
		if (offset % 16 == 0) {
			if (i > 0) {
				printf(" |");
				print_safe(s, 16);
				printf("|\n");
			}
			printf("0x%08X (+0x%04X)", offset, offset - start_offset);
			if (i < size) {
				printf(" ");
			}
		}
		if (offset % 8 == 0) {
			printf(" ");
		}
		if (i == size) {
			break;
		}
		printf("%02x ", (unsigned char)buf[i]);
		s[offset % 16] = buf[i];
	}
	if (offset % 16 != 0) {
		for (i = 0; i < 16 - (offset % 16); i++) {
			printf("   ");
		}
		if (offset % 16 < 8) {
			printf(" ");
		}
		memset(s + (offset % 16), ' ', 16 - (offset % 16));
		printf(" |");
		print_safe(s, 16);
		printf("|");
	}
	printf("\n");
}

static void hexdump4(char *buf, unsigned int size, unsigned int offset)
{
	unsigned int i;
	char s[16];

	if (offset % 16 != 0) {
		printf("%08x  ", offset);
		for (i = 0; i < (offset % 16); i++) {
			printf("   ");
		}
		if (offset % 16 > 8) {
			printf(" ");
		}
		memset(s, ' ', offset % 16);
	}
	for (i = 0;; i += 4, offset += 4) {
		if (offset % 32 == 0) {
			if (i > 0) {
				printf("\n");
			}
			printf("%08x", offset);
			if (i < size) {
				printf(" ");
			}
		}
		if (i == size) {
			break;
		}
		printf("%02x", (unsigned char)buf[i + 3]);
		printf("%02x", (unsigned char)buf[i + 2]);
		printf("%02x", (unsigned char)buf[i + 1]);
		printf("%02x ", (unsigned char)buf[i]);
	}
	if (offset % 16 != 0) {
		printf("\n%08x", offset);
	}
	printf("\n");
}

int engio_dump(unsigned char *data_buf, int length, int addr)
{
	unsigned int reg = 0;
	unsigned int data = 0;
	unsigned int pos = 0;

	do {
		data = data_buf[pos + 0] | (data_buf[pos + 1] << 8) | (data_buf[pos + 2] << 16) |
		       (data_buf[pos + 3] << 24);
		pos += 4;
		reg = data_buf[pos + 0] | (data_buf[pos + 1] << 8) | (data_buf[pos + 2] << 16) |
		      (data_buf[pos + 3] << 24);
		pos += 4;
		if (reg != 0xFFFFFFFF) {
			printf("[0x%08X] <- [0x%08X]\r\n", reg, data);
		}
	} while (reg != 0xFFFFFFFF && pos <= length - 8);

	printf("\r\n");
}

int adtg_dump(unsigned char *data_buf, int length, int addr)
{
	unsigned int reg = 0;
	unsigned int data = 0;
	unsigned int pos = 0;

	do {
		data = data_buf[pos + 0] | (data_buf[pos + 1] << 8);
		pos += 2;
		reg = data_buf[pos + 0] | (data_buf[pos + 1] << 8);
		pos += 2;
		if (reg != 0xFFFF && data != 0xFFFF) {
			printf("[0x%04X] <- [0x%04X]\r\n", reg, data);
		}
	} while (reg != 0xFFFF && data != 0xFFFF && pos <= length - 4);

	printf("\r\n");
}