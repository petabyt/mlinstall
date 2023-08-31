// This is the GTK based app for Windows / Linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include <pthread.h>

#include <camlib.h>
#include <ptp.h>

#include "drive.h"
#include "model.h"
#include "installer.h"
#include "evproc.h"
#include "appstore.h"
#include "platform.h"

// Activated with CLI flag -d
int dev_flag = 0;

// TODO: localize... somehow...
char *deviceNotFound = "Couldn't find a PTP/USB device.\n";
char *driveNotFound = "Couldn't find card. Make sure\nthe EOS_DIGITAL card is mounted.";
char *driveNotSupported = "Only ExFAT, FAT32, and FAT16\ncards are supported.";
#ifdef WIN32
	char *driveError = "Error opening drive.";
#else
	char *driveError = "Error opening drive. Make sure to\nrun as sudo. Make sure drive is mounted.";
#endif

// Quick, messy, logging mechanism:
GtkWidget *logw;
char logbuf[1000] = "\nLog info will go here.\n";

void log_print(char string[])
{
	strcat(logbuf, string);
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

void log_clear()
{
	strcpy(logbuf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

struct PtpRuntime ptp_runtime;

// Detect PTP return codes
int returnMessage(unsigned int code)
{
	char buf[128];
	snprintf(buf, sizeof(buf), "Response Code: %x\n", code);
	log_print(buf);

	switch (code) {
	case PTP_RC_OK:
		log_print("Return Code OK.\n");
		return 0;
	case PTP_RC_InvalidParameter:
		log_print("Return Code INVALID_PARAMETER.\n");
		return 1;
	case PTP_RC_OperationNotSupported:
		log_print("Operation not supported. Your camera is probably unsupported.");
		return 1;
	case 1:
		log_print("Parser error. See console.");
		return 1;
	}

	return 0;
}

#define HANDLE_DRIVE_ERROR() \
		case DRIVE_BADFS: \
		log_print(driveNotSupported); \
		return; \
	case DRIVE_NONE: \
		log_print(driveNotFound); \
		return; \
	case DRIVE_ERROR: \
		log_print(driveError); \
		return; 

static void writeflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	switch (drive_write_flag(FLAG_BOOT)) {
	HANDLE_DRIVE_ERROR()
	case 0:
		log_print("Wrote card flags on EOS_DIGITAL");
	}
}

static void destroyflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	switch (drive_write_flag(FLAG_DESTROY_BOOT)) {
	HANDLE_DRIVE_ERROR()
	case 0:
		log_print("Overwrote card flags.");
	}
}

static void scriptflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	switch (drive_write_flag(FLAG_SCRIPT)) {
	HANDLE_DRIVE_ERROR()
	case 0:
		log_print("Wrote script flags.");
	}
}

static void unscriptflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	switch (drive_write_flag(FLAG_DESTROY_SCRIPT)) {
	HANDLE_DRIVE_ERROR()
	case 0:
		log_print("Destroyed script flags.");
	}
}

int ptp_connect_init() {
	int r = ptp_device_init(&ptp_runtime);
	if (r) {
		log_print(deviceNotFound);
		return r;
	}

	r = ptp_open_session(&ptp_runtime);
	if (r) {
		return r;
	}

	ptp_runtime.di = (struct PtpDeviceInfo *)malloc(sizeof(struct PtpDeviceInfo));
	r = ptp_get_device_info(&ptp_runtime, ptp_runtime.di);
	if (r) {
		return r;
	}

	if (strcmp(ptp_runtime.di->manufacturer, "Canon Inc.")) {
		log_print("Not a Canon device!");
		return -1;
	}

	return r;
}

void *deviceinfo_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)1;

	char buffer[512];
	snprintf(buffer, sizeof(buffer),
		"Manufacturer: %s\n"
		"Model: %s\n"
		"DeviceVersion: %s\n"
		"SerialNumber: %s\n",
		ptp_runtime.di->manufacturer, ptp_runtime.di->model,
		ptp_runtime.di->device_version, ptp_runtime.di->serial_number);

	log_print(buffer);

	// Test Canon model detector
	printf("Model ID is %d\n", model_get(ptp_runtime.di->model));

	ptp_device_close(&ptp_runtime);
	return (void *)0;
}

static void deviceinfo(GtkWidget *widget, gpointer data) {
	pthread_t thread;

	if (pthread_create(&thread, NULL, deviceinfo_thread, NULL)) {
		return;
	}

	if (pthread_join(thread, NULL)) {
		return;
	}
}

void *eventproc_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)1;

	uintptr_t r = (uintptr_t)canon_evproc_run(&ptp_runtime, (char *)arg);
	return (void *)r;
}

// Run a custom event proc from input
static void eventproc(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(widget));

	if (pthread_create(&thread, NULL, eventproc_thread, (void *)entry)) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	returnMessage(result);
}

static void enablebootdisk(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	if (pthread_create(&thread, NULL, eventproc_thread, (void *)"EnableBootDisk")) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	if (result) {
		log_print("Couldn't enable boot disk.\n");
	} else {
		log_print("Enabled boot disk\n");
	}
}

static void disablebootdisk(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	if (pthread_create(&thread, NULL, eventproc_thread, (void *)"DisableBootDisk")) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	if (result) {
		log_print("Couldn't disable boot disk.\n");
	} else {
		log_print("Disabled boot disk\n");
	}
}

static void showdrive(GtkWidget *widget, gpointer data)
{
	log_clear();
	char buffer[1024];
	switch (drive_get_usable(buffer, sizeof(buffer))) {
		HANDLE_DRIVE_ERROR()
		case 0:
			log_print(buffer);
	}
}

void *oneclick_thread(void *arg) {
	int result = 0;
	log_clear();

	if (ptp_connect_init()) pthread_exit(&result);

	struct PtpDeviceInfo di;
	ptp_get_device_info(&ptp_runtime, &di);

	//close_camera(&ptp_usb, &params, dev);

	switch (installer_start(di.model, di.device_version)) {
	case NO_AVAILABLE_FIRMWARE:
		log_print("Your camera model has a working build,\n"
			 "but not for your firmware version.");
		break;
	case CAMERA_UNSUPPORTED:
		log_print("Your camera model is not supported.\n"
			 "Come back in 5 years and check again.");
		break;
	}

	pthread_exit(&result);
}

static void oneclick(GtkWidget *widget, gpointer data)
{
}

static int downloadmodule(GtkWidget *widget, gpointer data)
{
	log_clear();

	char *name = g_object_get_data(G_OBJECT(widget), "name");
	char *download = g_object_get_data(G_OBJECT(widget), "download");

	if (appstore_download(name, download)) {
		log_print("Error downloading module.\nIs ML installed?");
		return 1;
	} else {
		log_print("Module downloaded to card.");
		return 0;
	}
}

static int removemodule(GtkWidget *widget, gpointer data)
{
	log_clear();

	char *name = g_object_get_data(G_OBJECT(widget), "name");

	if (appstore_remove(name)) {
		log_print("Error removing module.");
		return 1;
	} else {
		log_print("Module removed.");
		return 0;
	}
}

static void modulebtn_callback(GtkWidget *widget, gpointer data)
{
	const char *name = gtk_button_get_label(GTK_BUTTON(widget));
	if (!strcmp(name, "Remove")) {
		if (!removemodule(widget, data)) {
			gtk_button_set_label(GTK_BUTTON(widget), "Install");
		}
	} else if (!strcmp(name, "Install")) {
		if (!downloadmodule(widget, data)) {
			gtk_button_set_label(GTK_BUTTON(widget), "Remove");
		}
	}
}

static void appstore(GtkWidget *widget, gpointer data)
{
	log_clear();
	GtkWidget *grid = gtk_widget_get_parent(widget);
	gtk_widget_destroy(widget);

	char usableDrive[1024];
	if (drive_get_usable(usableDrive, sizeof(usableDrive))) {
		log_print(driveNotFound);
		return;
	}

	struct AppstoreFields fields;
	appstore_init();

	int order = 0;
	int status = appstore_next(&fields);
	while (1) {
		GtkWidget *app = gtk_grid_new();
		gtk_grid_attach(GTK_GRID(grid), app, 0, order++, 1, 1);
		gtk_widget_set_vexpand(grid, TRUE);
		gtk_widget_show(app);

		char text[1024 * 3];
		snprintf(text, sizeof(text), "\n%s\n%s", fields.name, fields.description);

		GtkWidget *label = gtk_label_new(text);
		gtk_widget_set_hexpand(label, TRUE);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
		gtk_grid_attach(GTK_GRID(app), label, 0, 0, 1, 1);
		gtk_widget_show(label);

		char moduleTest[4096];

		appstore_getname(moduleTest, fields.name, sizeof(moduleTest));

		GtkWidget *button;
		FILE *f = fopen(moduleTest, "r");
		if (f == NULL) {
			button = gtk_button_new_with_label("Install");
		} else {
			button = gtk_button_new_with_label("Remove");
			fclose(f);
		}

		g_signal_connect(button, "clicked", G_CALLBACK(modulebtn_callback), NULL);
		gtk_widget_set_halign(button, GTK_ALIGN_END);
		gtk_grid_attach(GTK_GRID(app), button, 1, 1, 1, 1);
		gtk_widget_show(button);

		// Note: Can't free memory because this is used throughout runtime.
		char *name = malloc(strlen(fields.name) + 1);
		strcpy(name, fields.name);

		char *download = malloc(strlen(fields.download) + 1);
		strcpy(download, fields.download);

		g_object_set_data(G_OBJECT(button), "name", name);
		g_object_set_data(G_OBJECT(button), "download", download);

		order++;

		if (status) {
			break;
		}

		status = appstore_next(&fields);
	}

	appstore_close();
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

#define MENU_ADD_BUTTON(text, function, tip)                                                   \
	button = gtk_button_new_with_label(text);                                                  \
	g_signal_connect(button, "clicked", G_CALLBACK(function), NULL);                           \
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1);                                 \
	gtk_widget_set_tooltip_text(button, tip);                                                  \
	gtk_widget_set_hexpand(button, TRUE);                                                      \
	gtk_widget_show(button);

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *notebook;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *grid;
	GtkWidget *mainGrid;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d")) {
			dev_flag = 1;
		}
	}

	gtk_init(&argc, &argv);

	g_print("MLinstall by Daniel C. Use at your own risk!\n");
	g_print("https://github.com/petabyt/mlinstall\n");
	g_print("https://www.magiclantern.fm/forum/index.php?topic=26162\n");

	ptp_generic_init(&ptp_runtime);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "MLinstall");
	gtk_window_set_default_size(GTK_WINDOW(window), 375, 500);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	mainGrid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), mainGrid);

	logw = gtk_label_new(logbuf);
	gtk_label_set_justify(GTK_LABEL(logw), GTK_JUSTIFY_CENTER);
	gtk_widget_show(logw);

	label = gtk_label_new(NULL);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"large\">MLinstall</span>\nFor Canon cameras only!\n");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(mainGrid), label, 0, 0, 1, 1);
	gtk_widget_show(label);

	// Create "notebook"
	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_grid_attach(GTK_GRID(mainGrid), notebook, 0, 1, 1, 1);
	gtk_widget_show(notebook);

	// Add widgets horizontally
	int order = 0;

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);
	order = 0;

	label = gtk_label_new("This will run commands on your\n"
			      "camera via USB/PTP. Make sure to run as\n"
			      "Administrator/Sudo.\n");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	MENU_ADD_BUTTON("Get Device Info", deviceinfo, "Show Model, Firmware Version, etc.")

	MENU_ADD_BUTTON("Enable Boot Disk", enablebootdisk,
			"Write the bootdisk flag in the\ncamera, not on the card.")

	MENU_ADD_BUTTON("Disable Boot Disk", disablebootdisk, "Disable the camera's bootdisk flag.")

	label = gtk_label_new("USB");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
			     "This will automatically find and write to\n"
			     "a card named \"EOS_DIGITAL\".\n");
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	MENU_ADD_BUTTON(
		"Write card boot flags", writeflag,
		"Writes EOS_DIGITAL and BOOTDISK to a\nmounted SD/CF card named EOS_DIGITAL.")

	MENU_ADD_BUTTON(
		"Destroy card boot flags", destroyflag,
		"Destroys boot flags by replacing their\nfirst character with an underscore.")

	MENU_ADD_BUTTON("Make card scriptable", scriptflag,
			"Allows SD/CF card to run Canon Basic code.")

	MENU_ADD_BUTTON("Make card un-scriptable", unscriptflag,
			"Destroys script flags, same method as destroy card boot flags.")

	label = gtk_label_new("Card");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);
	order = 0;

	label = gtk_label_new("Entering the wrong command can brick your\ncamera! Only use with guidance\nfrom a ML developer!");
	gtk_widget_set_hexpand(label, TRUE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	GtkEntryBuffer *buf = gtk_entry_buffer_new("TurnOffDisplay", 14);
	entry = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), entry, 0, order++, 1, 1);
	g_signal_connect(entry, "activate", G_CALLBACK(eventproc), NULL);
	gtk_widget_show(entry);

	MENU_ADD_BUTTON("Detect EOS_DIGITAL", showdrive, "Try and detect the EOS_DIGITAL drive.")

	label = gtk_label_new(NULL);
	gtk_label_set_markup(
		GTK_LABEL(label),
		"\nMade by <a href='https://danielc.dev/'>Daniel C</a>\n"
		"Source code: <a href='https://github.com/petabyt/mlinstall'>github.com/petabyt/mlinstall</a>\n\n"
		"Licenced under GNU General Public License v2.0\n"
		"If it breaks, you get to keep both pieces!");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	label = gtk_label_new("Advanced");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

#if 0
		grid = gtk_grid_new();
		gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
		gtk_widget_show(grid);
		order = 0;

		MENU_ADD_BUTTON("One Click Magic Lantern Install", oneclick, "Download+Install Magic Lantern.")

		label = gtk_label_new("Quick Install");
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);
#endif

#if 0
	GtkWidget *scrollWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrollWindow);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_container_add(GTK_CONTAINER(scrollWindow), grid);
	gtk_widget_show(grid);
	order = 0;

	MENU_ADD_BUTTON("Pull Database", appstore, "Load the list of downloadable modules")

	label = gtk_label_new("App Store");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollWindow, label);
#endif

	gtk_grid_attach(GTK_GRID(mainGrid), logw, 0, 2, 1, 1);

	gtk_widget_show(mainGrid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
// What kind of idiot would write a UI in C. This is seriously stupid.
