#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include <ui.h>
#include <camlib.h>
#include "app.h"
#include "lang.h"
#include "drive.h"

/*
malloc_consolidate(): unaligned fastbin chunk detected
*/

struct AppGlobalState {
	void *connect_button;
	void *usb_widgets[7];
	int is_connected;
	void *title_text;
	void *custom_entry;
	int ticks;
}app = {0};

static uiMultilineEntry *log_widget = NULL;

void ui_connected_state();
void ui_disconnected_state();

void usb_disable_all() {
	for (int i = 0; i < (int)(sizeof(app.usb_widgets) / sizeof(void *)); i++) {
		if (app.usb_widgets[i] == NULL) continue;
		uiControlDisable(uiControl(app.usb_widgets[i]));
	}
}

void usb_enable_all() {
	for (int i = 0; i < (int)(sizeof(app.usb_widgets) / sizeof(void *)); i++) {
		if (app.usb_widgets[i] == NULL) continue;
		uiControlEnable(uiControl(app.usb_widgets[i]));
	}
}

static void log_print_str(void *arg) {
	uiMultilineEntryAppend(log_widget, (const char *)arg);
	free(arg);
}

static void log_clear_sync(void *arg) {
	uiMultilineEntrySetText(log_widget, "");
}

void log_print(char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, 1024, format, args);
	va_end(args);

	if (log_widget == NULL) {
		printf("LOG: %s\n", buffer);
		return;
	}

	char *s = strdup(buffer);
	uiQueueMain(log_print_str, s);
	s = strdup("\n");
	uiQueueMain(log_print_str, s);
}

void log_clear()
{
	uiQueueMain(log_clear_sync, NULL);
}

void ptp_report_error(struct PtpRuntime *r, char *reason, int code) {
	puts("Reported error");
	if (r->io_kill_switch) return;
	r->io_kill_switch = 1;
	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			log_print("Disconnected: IO Error");
		} else {
			log_print("Disconnected: Runtime error");
		}
	} else {
		log_print("Disconnected: %s", reason);
	}
}

static int log_drive_error(int rc)
{
	switch (rc) {
	case DRIVE_BADFS:
		log_print(T_DRIVE_NOT_SUPPORTED);
		return rc;
	case DRIVE_NONE:
		log_print(T_DRIVE_NOT_FOUND);
		return rc;
	case DRIVE_ERROR:
		log_print(T_DRIVE_ERROR);
		return rc;
	}

	return 0;
}

static void app_write_flag(uiButton *b, void *data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_CARD_FLAGS);
	}
}

static void app_destroy_flag(uiButton *b, void *data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROY_CARD_FLAGS);
	}
}

static void app_script_flag(uiButton *b, void *data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_SCRIPT_FLAGS);
	}
}

static void app_destroy_script_flag(uiButton *b, void *data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROYED_SCRIPT_FLAGS);
	}
}

void *app_connect_start_thread(void *arg) {
	log_clear();

	struct PtpRuntime *r = &ptp_runtime;

	int rc = ptp_connect_init();
	if (rc) {
		ptp_report_error(r, NULL, rc);
		return (void *)1;
	}

	log_print("Connected to a camera");

	app.is_connected = 1;
	ui_connected_state();

	ptp_eos_set_remote_mode(r, 1);
	ptp_eos_set_event_mode(r, 1);

	int length = 0;
	struct PtpGenericEvent *s = NULL;

	rc = ptp_eos_get_event(r);
	if (rc) {
		ptp_report_error(r, NULL, rc);
		return (void *)1;
	}

	int shutter_count = 0;

	length = ptp_eos_events(r, &s);
	for (int i = 0; i < length; i++) {
		if (s[i].code == PTP_PC_EOS_ShutterCounter) {
			shutter_count = s[i].value;
		}
	}

	if (length != 0) free(s);

	// Remove the EOS '3-' prefix
	char *fw_version = r->di->device_version;
	if (fw_version[0] == '3' && fw_version[1] == '-') {
		fw_version += 2;
	}

	log_print("Model:				%s", r->di->model);
	log_print("Firmware Version:	%s", r->di->device_version);
	log_print("Serial Number:		%s", r->di->serial_number);
	if (shutter_count) {
		log_print("Shutter count:		%d", shutter_count);
	} else {
		log_print("Shutter count:		??");
	}

	while (1) {
		int rc = ptp_eos_get_event(r);
		if (rc) {
			ptp_report_error(r, NULL, rc);
			return (void *)1;
		}

		if (!app.is_connected) return (void *)1;

		length = ptp_eos_events(r, &s);
		for (int i = 0; i < length; i++) {
			// TODO...
		}

		if (length != 0) free(s);

		char buffer[32];
		sprintf(buffer, T_APP_NAME " %c", "-\\|/"[app.ticks % 4]);

		app.ticks++;

		uiLabelSetText(app.title_text, buffer);

		usleep(1000 * 200);
	}
}

static void app_connect_start(uiButton *b, void *data) {
	pthread_t thread;

	if (pthread_create(&thread, NULL, app_connect_start_thread, NULL)) {
		return;
	}
}

static void *app_run_eventproc_thread(void *arg) {
	log_clear();

	const char *evproc = (const char *)(arg);

	log_print("Running '%s'...", evproc);

	int rc = ptp_eos_activate_command(&ptp_runtime);
	if (rc) goto err;

	rc = ptp_eos_evproc_run(&ptp_runtime, evproc);
	if (rc == PTP_IO_ERR) goto err;
	int resp = ptp_get_return_code(&ptp_runtime);

	log_print(T_RETURN_CODE_OK);

	if (!strcmp(evproc, ENABLE_BOOT_DISK)) {
		if (rc) {
			log_print(T_BOOT_DISK_ENABLE_FAIL);
		} else {
			log_print(T_BOOT_DISK_ENABLE_SUCCESS);
		}
	} else if (!strcmp(evproc, DISABLE_BOOT_DISK)) {
		if (rc) {
			log_print(T_DISABLE_BOOT_DISK_FAIL);
		} else {
			log_print(T_DISABLE_BOOT_DISK_SUCCESS);
		}		
	} else {
		printf("Response Code: 0x%X\n", resp);
		switch (resp) {
		case PTP_RC_OK:
			log_print(T_RETURN_CODE_OK);
			break;
		case PTP_RC_InvalidParameter:
			log_print(T_RETURN_INVALID_PARAM);
			break;
		case PTP_RC_OperationNotSupported:
			log_print(T_RETURN_UNSUPPORTED);
			break;
		case -1:
			break;
		default:
			log_print(T_UNKNOWN_ERROR);
			break;
		}

	}

	return (void *)0;

	err:;
	log_print("Error completing operation");
	if (rc == PTP_IO_ERR) {
		ptp_report_error(&ptp_runtime, NULL, rc);
	}
	return (void *)(-1);	
}

// Run a custom event proc from input
static void app_run_eventproc(uiButton *b, void *data) {
	pthread_t thread;

	const char *entry = uiEntryText(app.custom_entry);

	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)entry)) {
		return;
	}

	// 'entry' will be leaked
}

static void app_enable_bootdisk(uiButton *b, void *data)
{
	pthread_t thread;
	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)ENABLE_BOOT_DISK)) {
		return;
	}
}

static void app_disable_bootdisk(uiButton *b, void *data)
{
	pthread_t thread;
	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)DISABLE_BOOT_DISK)) {
		return;
	}
}

static void app_show_drive_info(uiButton *b, void *data)
{
	log_clear();
	char buffer[1024];
	int rc = drive_get_usable(buffer, sizeof(buffer));
	if (log_drive_error(rc) == 0) {
		log_print(buffer);
	}
}

static void app_disconnect() {
	ptp_mutex_keep_locked(&ptp_runtime);

	ptp_close_session(&ptp_runtime);

	ptp_report_error(&ptp_runtime, "Intentional", 0);

	ptp_device_close(&ptp_runtime);

	ptp_mutex_unlock(&ptp_runtime);

	ui_disconnected_state();
}

int on_closing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

void ui_connected_state() {
	uiButtonSetText(app.connect_button, T_DISCONNECT);
	uiButtonOnClicked(app.connect_button, app_disconnect, NULL);
	usb_enable_all();
}

void ui_disconnected_state() {
	uiButtonSetText(app.connect_button, T_CONNECT);
	uiButtonOnClicked(app.connect_button, app_connect_start, NULL);
	usb_disable_all();	
}

static uiControl *page_usb(void)
{
	uiBox *vbox;
	uiButton *button;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	// label = uiNewLabel("Camera not connected");
	// uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_CONNECT);
	app.connect_button = button;
	uiBoxAppend(vbox, uiControl(button), 0);

	label = uiNewLabel("Lorem ipsum dolor sit amet, consectetur\nadipiscing elit.");
	uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_ENABLE_BOOT_DISK);
	uiButtonOnClicked(button, app_enable_bootdisk, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
	app.usb_widgets[0] = button;
	app.usb_widgets[1] = label;

	label = uiNewLabel("Lorem ipsum dolor sit amet,consectetur\nadipiscing elit.");
	uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_DISABLE_BOOT_DISK);
	uiButtonOnClicked(button, app_disable_bootdisk, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
	app.usb_widgets[2] = button;
	app.usb_widgets[3] = label;

	if (dev_flag) {
		label = uiNewLabel(T_DEV_MODE_WARNING);
		uiBoxAppend(vbox, uiControl(label), 0);

		uiEntry *entry = uiNewEntry();
		uiEntrySetText(entry, TURN_OFF_DISPLAY);
		uiBoxAppend(vbox, uiControl(entry), 0);
		app.custom_entry = entry;

		button = uiNewButton("Run command");
		uiBoxAppend(vbox, uiControl(button), 0);
		uiButtonOnClicked(button, app_run_eventproc, NULL);

		app.usb_widgets[4] = label;
		app.usb_widgets[5] = entry;
		app.usb_widgets[6] = button;
	}

	ui_disconnected_state();

	return uiControl(vbox);
}

static uiControl *page_card(void) {
	uiBox *vbox;
	uiButton *button;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	label = uiNewLabel(T_CARD_STUFF_TITLE);
	uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_WRITE_CARD_BOOT_FLAGS);
	uiButtonOnClicked(button, app_write_flag, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
	button = uiNewButton(T_DESTROY_CARD_BOOT_FLAGS);
	uiButtonOnClicked(button, app_destroy_flag, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);

	label = uiNewLabel(T_CARD_STUFF_TITLE);
	uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_MAKE_CARD_SCRIPTABLE);
	uiButtonOnClicked(button, app_script_flag, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
	button = uiNewButton(T_MAKE_CARD_UNSCRIPTABLE);
	uiButtonOnClicked(button, app_destroy_script_flag, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);

	return uiControl(vbox);
}

static uiControl *page_about(void) {
	uiBox *vbox;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	uiMultilineEntry *entry = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(entry, 1);
	uiBoxAppend(vbox, uiControl(entry), 1);

	uiMultilineEntryAppend(entry, "Written by Daniel Cook (https://danielc.dev/)\n");
	uiMultilineEntryAppend(entry, T_APP_NAME " is licensed under the GPL2.0\n");
	uiMultilineEntryAppend(entry, "https://github.com/petabyt/mlinstall\n");
	uiMultilineEntryAppend(entry, "\nOpen Source Libraries:\n");
	uiMultilineEntryAppend(entry, "- camlib (Apache License)\n");
	uiMultilineEntryAppend(entry, "- libwpd (MIT License)\n");
	uiMultilineEntryAppend(entry, "- libui-ng (MIT License)\n");

	return uiControl(vbox);
}

int app_main_window() {
	uiInitOptions o;
	uiWindow *w;
	uiBox *b;
	uiLabel *lbl;
	uiTab *tab;
	uiBox *hbox;

	memset(&o, 0, sizeof(uiInitOptions));
	if (uiInit(&o) != NULL) {
		puts("uiInit(&o) != NULL, abort");
		abort();
	}

	w = uiNewWindow("MLinstall", 800, 500, 0);
	uiWindowSetMargined(w, 1);

	hbox = uiNewHorizontalBox();
	uiBoxSetPadded(hbox, 1);
	uiWindowSetChild(w, uiControl(hbox));

	b = uiNewVerticalBox();
	uiBoxSetPadded(b, 1);
	uiBoxAppend(hbox, uiControl(b), 1);

	uiBox *b2 = uiNewVerticalBox();
	uiBoxSetPadded(b2, 0);
		lbl = uiNewLabel(T_APP_NAME);
		uiBoxAppend(b2, uiControl(lbl), 0);
		app.title_text = lbl;
		lbl = uiNewLabel(T_APP_SUBTITLE);
		uiBoxAppend(b2, uiControl(lbl), 0);
	uiBoxAppend(b, uiControl(b2), 0);

	log_widget = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(log_widget, 1);
	uiBoxAppend(hbox, uiControl(log_widget), 1);

	log_print(T_APP_NAME " by Daniel C. Use at your own risk!");
	log_print("https://github.com/petabyt/mlinstall");

	tab = uiNewTab();
	uiBoxAppend(b, uiControl(tab), 1);

	uiTabAppend(tab, T_USB, page_usb());
	uiTabSetMargined(tab, 0, 1);

	uiTabAppend(tab, T_CARD, page_card());
	uiTabSetMargined(tab, 1, 1);

	uiTabAppend(tab, T_ABOUT, page_about());
	uiTabSetMargined(tab, 2, 1);

	uiControlShow(uiControl(tab));

	usb_disable_all();

	uiWindowOnClosing(w, on_closing, NULL);
	uiControlShow(uiControl(w));
	uiMain();
	return 0;
}
