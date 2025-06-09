#include <stdlib.h>
#include <string.h>
#include <libpict.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "mlinstall.h"
#include "lang.h"

int app_test(struct PtpRuntime *r);
struct PtpRuntime *ptp_runtime;
struct PtpRuntime *ptp_get(void) {
	return ptp_runtime;
}
int dev_flag = 0;
static int attempts = 0;

int main (int argc, char ** argv) {
	ptp_runtime = ptp_new(PTP_USB);

#ifdef VCAM
	void vcam_add_usbt_device(const char *name, int argc, char **argv);
	ptp_comm_init(ptp_runtime);
	vcam_add_usbt_device("canon_1300d", )
#endif

#ifdef _WIN32
	// Route stdout to console
	AttachConsole(-1);
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
				puts("Invalid usage");
				return 1;
			}

			if (ptp_connect_init()) {
				printf("%s\n", T_DEV_NOT_FOUND);
				return 1;
			}

			printf("Uploading '%s' to cam as '%s'\n", argv[i + 1], argv[i + 2]);

			int rc = ptp_chdk_upload_file(ptp_runtime, argv[i + 1], argv[i + 2]);
			if (rc) return rc;

			printf("File uploaded\n");

			return ptp_connect_deinit();
		} else if (!strcmp(argv[i], "--test")) {
			if (app_test(ptp_runtime)) {
				puts("Test failed");
				return 1;
			}
			return 0;
		}
	}

	puts("Starting main window...");
	return mlinstall_main_window();
}

int app_test(struct PtpRuntime *r) {
	int rc = 0;

	if (ptp_connect_init()) {
		printf("%s\n", T_DEV_NOT_FOUND);
		return 1;
	}

	r->comm_dump = fopen("test_dump", "wb");
	if (r->comm_dump == NULL) abort();

	rc = mlinstall_setup_session(r);
	if (rc) return rc;

	rc = ptp_eos_activate_command(r);
	if (rc) return rc;

	rc = ptp_eos_evproc_run(r, ENABLE_BOOT_DISK);
	if (rc) return rc;

	rc = ptp_eos_evproc_run(r, DISABLE_BOOT_DISK);
	if (rc) return rc;

	rc = ptp_close_session(ptp_runtime);
	if (rc) return rc;

	ptp_device_close(ptp_runtime);

	fclose(r->comm_dump);

	return 0;
}

int ptp_connect_deinit(void) {
	int rc = ptp_close_session(ptp_runtime);
	if (rc) return rc;

	ptp_device_close(ptp_runtime);

	return 0;
}

int ptp_connect_init(void) {
	attempts++;

	int rc;
#if 0
	// For LibWPD, this will work just fine to detect cameras
	rc = ptp_device_init(ptp_runtime);
	if (rc) {
		log_print(T_CANON_NOT_FOUND_FMT, attempts - 1);
		return PTP_NO_DEVICE;
	}
#endif
	// TODO: libWPD doesn't have ptpusb_device_list yet

	struct PtpDeviceEntry *list = ptpusb_device_list(ptp_runtime);

	struct PtpDeviceEntry *selected = NULL;
	for (struct PtpDeviceEntry *curr = list; curr != NULL; curr = curr->next) {
		printf("Device: %s\tVendor: \t%X\n", curr->name, curr->vendor_id);
		if (curr->vendor_id == USB_VENDOR_CANON) {
			selected = curr;
		}
	}

	if (selected == NULL) {
		log_print(T_CANON_NOT_FOUND_FMT, attempts - 1);
		return PTP_NO_DEVICE;
	}

	rc = ptp_device_open(ptp_runtime, selected);

	attempts = 0;

	rc = ptp_open_session(ptp_runtime);
	if (rc) {
		return rc;
	}

	ptp_runtime->di = (struct PtpDeviceInfo *)malloc(sizeof(struct PtpDeviceInfo));
	rc = ptp_get_device_info(ptp_runtime, ptp_runtime->di);
	if (rc) {
		ptp_connect_deinit();
		return rc;
	}

	if (ptp_device_type(ptp_runtime) != PTP_DEV_EOS) {
		log_print(T_NOT_CANON_DEVICE, ptp_runtime->di->model);
		ptp_connect_deinit();
		return -1;
	}

	return 0;
}
