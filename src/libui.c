#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <ui.h>
#include <libpict.h>
#include "mlinstall.h"
#include "lang.h"
#include "drive.h"

struct AppGlobalState {
	void *connect_button;
	void *usb_widgets[7];
	int is_connected;
	void *title_text;
	void *custom_entry;
	int ticks;
}app = {0};

static uiMultilineEntry *log_widget = NULL;

void ui_connected_state(void *arg);
void ui_disconnected_state(void *arg);

void usb_disable_all(void *arg) {
	for (int i = 0; i < (int)(sizeof(app.usb_widgets) / sizeof(void *)); i++) {
		if (app.usb_widgets[i] == NULL) continue;
		uiControlDisable(uiControl(app.usb_widgets[i]));
	}
}

void usb_enable_all(void *arg) {
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

void log_print(char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);
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

void log_clear(void) {
	uiQueueMain(log_clear_sync, NULL);
}

void ptp_report_error(struct PtpRuntime *r, char *reason, int code) {
	if (r->io_kill_switch) return;
	ptp_mutex_lock(r);
	if (r->io_kill_switch) {
		ptp_mutex_unlock(r);
		return;
	}

	// Safely disconnect if intentional
	if (code == 0) {
		ptp_verbose_log("Closing session\n");
		ptp_close_session(r);
	}

	r->operation_kill_switch = 1;

	ptp_verbose_log("Goodbye\n");

	ptp_device_close(r);

	r->io_kill_switch = 1;

	ptp_mutex_unlock(r);

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			log_print("Disconnected: IO Error");
		} else {
			log_print("Disconnected: Runtime error");
		}
	} else {
		log_print("Disconnected: %s", reason);
	}

	uiQueueMain(ui_disconnected_state, NULL);
}

static int log_drive_error(int rc) {
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

static void app_write_flag(uiButton *b, void *data) {
	log_clear();
	int rc = drive_write_flag(FLAG_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_CARD_FLAGS);
	}
}

static void app_destroy_flag(uiButton *b, void *data) {
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROY_CARD_FLAGS);
	}
}

static void app_script_flag(uiButton *b, void *data) {
	log_clear();
	int rc = drive_write_flag(FLAG_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_SCRIPT_FLAGS);
	}
}

static void app_destroy_script_flag(uiButton *b, void *data) {
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROYED_SCRIPT_FLAGS);
	}
}

static void ui_flip_status(void *data) {
	// ASCII loading wheel - just like Minecraft!
	char status_char = "-\\|/"[app.ticks % 4];

	char buffer[32];
	sprintf(buffer, T_APP_NAME " %c", status_char);

	app.ticks++;

	uiLabelSetText(app.title_text, buffer);
}

int mlinstall_setup_session(struct PtpRuntime *r) {
	ptp_eos_set_remote_mode(r, 1);
	ptp_eos_set_event_mode(r, 1);

	int rc = ptp_eos_get_event(r);
	if (rc) {
		ptp_report_error(r, NULL, rc);
		return rc;
	}

	int shutter_count = 0;

	struct PtpEventReader reader;
	ptp_eos_events_open(r, &reader);

	struct PtpGenericEvent event;
	while (ptp_eos_events_next(r, &reader, &event) == 0) {
		if (event.code == PTP_DPC_EOS_ShutterCounter) {
			shutter_count = event.value;
		}
	}

	// Remove the EOS '3-' prefix
	char *fw_version = r->di->device_version;
	if (fw_version[0] == '3' && fw_version[1] == '-') {
		fw_version += 2;
	}

	log_print("Model: %s", r->di->model);
	log_print("Firmware Version: %s", fw_version);
	if (shutter_count) {
		log_print("Shutter count: %d", shutter_count);
	} else {
		log_print("Shutter count: ??");
	}

	char build_version[16];
	rc = ptp_eos_fa_get_build_version(r, build_version, sizeof(build_version));
	if (rc == 0) {
		log_print("Build version: %s", build_version);
	} else {
		log_print("Failed to get build version (%d)", rc);
	}
	return 0;
}

void *app_connect_start_thread(void *arg) {
	log_clear();

	struct PtpRuntime *r = ptp_get();

	int rc = mlinstall_connect();
	if (rc) {
		ptp_report_error(r, NULL, rc);
		pthread_exit(NULL);
	}

	log_print("Connected to a camera");

	app.is_connected = 1;
	uiQueueMain(ui_connected_state, NULL);

	rc = mlinstall_setup_session(r);
	if (rc) pthread_exit(NULL);

	while (1) {
		rc = ptp_eos_get_event(r);
		if (rc) {
			ptp_report_error(r, NULL, rc);
			pthread_exit(NULL);
		}

		if (!app.is_connected) pthread_exit(NULL);

		uiQueueMain(ui_flip_status, NULL);

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
	struct PtpRuntime *r = ptp_get();
	log_clear();

	log_print("Running '%s'...", arg);

	int rc = ptp_eos_activate_command(r);
	if (rc) goto err;

	rc = ptp_eos_evproc_run(r, arg);
	if (rc == PTP_IO_ERR) goto err;
	int resp = ptp_get_return_code(r);

	log_print(T_RETURN_CODE_OK);

	if (!strcmp(arg, ENABLE_BOOT_DISK)) {
		if (rc) {
			log_print(T_BOOT_DISK_ENABLE_FAIL);
		} else {
			log_print(T_BOOT_DISK_ENABLE_SUCCESS);
		}
	} else if (!strcmp(arg, DISABLE_BOOT_DISK)) {
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

	// TODO: Will leak only in developer mode
	//uiFreeText((char *)arg);

	return (void *)0;

	err:;
	log_print("Error completing operation");
	if (rc == PTP_IO_ERR) {
		ptp_report_error(r, NULL, rc);
	}
	return (void *)(-1);	
}

// Run a custom event proc from input
static void app_run_eventproc(uiButton *b, void *data) {
	pthread_t thread;

	char *entry = uiEntryText(app.custom_entry);

	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)entry)) {
		return;
	}
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

static void *app_disconnect(void *arg) {
	struct PtpRuntime *r = ptp_get();

	ptp_report_error(r, "Intentional", 0);

	return NULL;
}

int on_closing(uiWindow *w, void *data)
{
	// TODO: close down threads and connection
	uiQuit();
	return 1;
}

static void app_disconnect_thread(uiButton *b, void *data)
{
	pthread_t thread;
	if (pthread_create(&thread, NULL, app_disconnect, NULL)) {
		return;
	}
}

void ui_connected_state(void *arg) {
	uiButtonSetText(app.connect_button, T_DISCONNECT);
	uiButtonOnClicked(app.connect_button, app_disconnect_thread, NULL);
	usb_enable_all(NULL);
}

void ui_disconnected_state(void *arg) {
	uiButtonSetText(app.connect_button, T_CONNECT);
	uiButtonOnClicked(app.connect_button, app_connect_start, NULL);
	usb_disable_all(NULL);
}

static uiControl *page_usb(void)
{
	uiBox *vbox;
	uiButton *button;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	button = uiNewButton(T_CONNECT);
	uiControlSetTooltip(uiControl(button), "Connect to the first Canon camera available");
	app.connect_button = button;
	uiBoxAppend(vbox, uiControl(button), 0);

	label = uiNewLabel(T_ABOUT_BOOTDISK);
	uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_ENABLE_BOOT_DISK);
	uiControlSetTooltip(uiControl(button), "Enables the BootDisk flag, enabling the camera to run Magic Lantern");
	uiButtonOnClicked(button, app_enable_bootdisk, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
	app.usb_widgets[0] = button;
	app.usb_widgets[1] = label;

	//label = uiNewLabel("This will reset the camera to the factory state.");
	//uiBoxAppend(vbox, uiControl(label), 0);
	button = uiNewButton(T_DISABLE_BOOT_DISK);
	uiControlSetTooltip(uiControl(button), "Disables the BootDisk flag, which is the factory state");
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

	ui_disconnected_state(NULL);

	return uiControl(vbox);
}

static uiControl *page_card(void) {
	uiBox *vbox;
	uiButton *button;
	uiLabel *label;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

#ifdef __APPLE__
	uiBoxAppend(vbox, uiControl(uiNewLabel("MacOS card tools are\ncurrently not supported.")), 0);
#else

	{
		uiBox *hbox = uiNewHorizontalBox();
		uiBoxSetPadded(hbox, 1);

		uiBoxAppend(vbox, uiControl(uiNewLabel("Card Boot Flags")), 0);
		uiBoxAppend(vbox, uiControl(uiNewLabel("Enabling this changes some bytes on the card which\nallows the camera to boot from it.")), 0);
		
		button = uiNewButton("Enable"); // T_WRITE_CARD_BOOT_FLAGS
		uiControlSetTooltip(uiControl(button), "Make it so that a camera can boot Magic Lantern from the card");
		uiButtonOnClicked(button, app_write_flag, NULL);
		uiBoxAppend(hbox, uiControl(button), 1);
		button = uiNewButton("Disable"); // T_DESTROY_CARD_BOOT_FLAGS
		uiControlSetTooltip(uiControl(button), "Make the card non-bootable");
		uiButtonOnClicked(button, app_destroy_flag, NULL);
		uiBoxAppend(hbox, uiControl(button), 1);
	
		uiBoxAppend(vbox, uiControl(hbox), 0);
	}

	{
		uiBox *hbox = uiNewHorizontalBox();
		uiBoxSetPadded(hbox, 1);

		uiBoxAppend(vbox, uiControl(uiNewLabel("Card scriptable flags")), 0);
		uiBoxAppend(vbox, uiControl(uiNewLabel("Enable this if you want to run Canon Basic scripts.")), 0);

		button = uiNewButton("Enable"); // T_MAKE_CARD_SCRIPTABLE
		uiControlSetTooltip(uiControl(button), "Make the card able to run a canon basic script");
		uiButtonOnClicked(button, app_script_flag, NULL);
		uiBoxAppend(hbox, uiControl(button), 1);
		button = uiNewButton("Disable"); // T_MAKE_CARD_UNSCRIPTABLE
		uiControlSetTooltip(uiControl(button), "Remove the script flag");
		uiButtonOnClicked(button, app_destroy_script_flag, NULL);
		uiBoxAppend(hbox, uiControl(button), 1);
	
		uiBoxAppend(vbox, uiControl(hbox), 0);
	}

	uiBoxAppend(vbox, uiControl(uiNewLabel(T_DETECT_CARD_TITLE)), 0);
	button = uiNewButton(T_DETECT_CARD);
	uiControlSetTooltip(uiControl(button), "Check if the card is inserted");
	uiButtonOnClicked(button, app_show_drive_info, NULL);
	uiBoxAppend(vbox, uiControl(button), 0);
#endif

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
	uiMultilineEntryAppend(entry, "Version: " T_APP_VERSION "\nCompile date: " __DATE__ "\n");
	uiMultilineEntryAppend(entry, T_APP_NAME " is licensed under the GPL2.0\n");
	uiMultilineEntryAppend(entry, "https://github.com/petabyt/mlinstall\n");
	uiMultilineEntryAppend(entry, "\nOpen Source Libraries:\n");
	uiMultilineEntryAppend(entry, "- libpict (Apache License)\n");
	uiMultilineEntryAppend(entry, "- libwpd (MIT License)\n");
	uiMultilineEntryAppend(entry, "- libui-ng (MIT License)\n");
	uiMultilineEntryAppend(entry, "- libusb (LGPL v2.1)\n");

	return uiControl(vbox);
}

int mlinstall_main_window(void) {
	uiInitOptions o;
	uiWindow *w;
	uiBox *b;
	uiLabel *lbl;
	uiTab *tab;
	uiBox *hbox;

	// libui sets this thread as "single-thread apartment"
	memset(&o, 0, sizeof(uiInitOptions));
	if (uiInit(&o) != NULL) {
		puts("uiInit(&o) != NULL, abort");
		abort();
	}

	w = uiNewWindow(T_APP_NAME " " T_APP_VERSION, 800, 500, 0);
	uiWindowSetMargined(w, 1);

	extern const char favicon_ico[] __asm__("favicon_ico");
	extern size_t favicon_ico_length __asm__("favicon_ico_length");

	uiWindowSetIcon(w, favicon_ico, favicon_ico_length);

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

	usb_disable_all(NULL);

	uiWindowOnClosing(w, on_closing, NULL);
	uiControlShow(uiControl(w));
	uiMain();
	return 0;
}
