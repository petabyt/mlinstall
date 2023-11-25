#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "app.h"
#include "lang.h"

struct PtpRuntime ptp_runtime;
int dev_flag = 0;
static int attempts = 0;

int main (int argc, char ** argv) {
	ptp_generic_init(&ptp_runtime);

#ifdef _WIN32
	// Redirect stdout
	AttachConsole(-1);
	//freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d")) {
			dev_flag = 1;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf(
				T_APP_NAME " version " T_APP_VERSION " - Use at your own risk!\n"
				"https://github.com/petabyt/mlinstall\n"
				"Command line flags:\n"
				"-d              Enable developer features\n"
				"-h              Show this message\n"
				"-l              List connected devices\n"
				"--upload <filename> <target_location>       Upload file to SD card through filewrite evproc\n"
				"\n"
			);

			return 0;
		} else if (!strcmp(argv[i], "--upload")) {
			if (argc - i < 3) {
				puts("Not enough args");
				return -1;
			}
			if (ptp_connect_init()) return -1;

			int rc = ptp_chdk_upload_file(&ptp_runtime, argv[i + 1], argv[i + 2]);
			if (rc) return rc;

			return ptp_connect_deinit();
		}
	}

	puts("Starting main window...");
	return app_main_window();
}

int ptp_connect_deinit() {
	int rc = ptp_close_session(&ptp_runtime);
	if (rc) return rc;

	ptp_device_close(&ptp_runtime);

	return 0;
}

int ptp_connect_init() {
	attempts++;

	int rc;
#ifdef WIN32
	// For LibWPD, this will work just fine to detect cameras
	rc = ptp_device_init(&ptp_runtime);
	if (rc) {
		log_print(T_DEV_NOT_FOUND);
		return rc;
	}
#else
	// TODO: libWPD doesn't have this yet
	ptp_comm_init(&ptp_runtime);

	struct PtpDeviceEntry *list = ptpusb_device_list(&ptp_runtime);

	struct PtpDeviceEntry *selected = NULL;
	for (struct PtpDeviceEntry *curr = list; curr != NULL; curr = curr->next) {
		printf("Device: %s\tVendor: \t%X\n", curr->name, curr->vendor_id);
		if (curr->vendor_id == USB_VENDOR_CANON) {
			selected = curr;
		}
	}

	if (selected == NULL) {
		log_print("No Canon device found (%d)", attempts - 1);
		return PTP_NO_DEVICE;
	}

	rc = ptp_device_open(&ptp_runtime, selected);
#endif

	rc = ptp_open_session(&ptp_runtime);
	if (rc) {
		return rc;
	}

	ptp_runtime.di = (struct PtpDeviceInfo *)malloc(sizeof(struct PtpDeviceInfo));
	rc = ptp_get_device_info(&ptp_runtime, ptp_runtime.di);
	if (rc) {
		ptp_connect_deinit();
		return rc;
	}

	if (strcmp(ptp_runtime.di->manufacturer, "Canon Inc.")) {
		log_print(T_NOT_CANON_DEVICE, ptp_runtime.di->model);
		ptp_connect_deinit();
		return -1;
	}

	return 0;
}
