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

#include "app.h"
#include "lang.h"
#include "drive.h"
#include "model.h"
#include "installer.h"
#include "evproc.h"
#include "appstore.h"

struct PtpRuntime ptp_runtime;

// Activated with CLI flag -d
int dev_flag = 0;

#define ENABLE_BOOT_DISK "EnableBootDisk"
#define DISABLE_BOOT_DISK "DisableBootDisk"
#define TURN_OFF_DISPLAY "TurnOffDisplay"

static GtkWidget *logw;
static char logbuf[1000] = "";

void log_print(char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, 1024, format, args);
	va_end(args);

	strcat(logbuf, buffer);
	strcat(logbuf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

void log_clear()
{
	strcpy(logbuf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

// Detect PTP return codes
int returnMessage(unsigned int code)
{
	printf("Response Code: %x", code);

	switch (code) {
	case PTP_RC_OK:
		log_print(T_RETURN_CODE_OK);
		return 0;
	case PTP_RC_InvalidParameter:
		log_print(T_RETURN_INVALID_PARAM);
		return -1;
	case PTP_RC_OperationNotSupported:
		log_print(T_RETURN_UNSUPPORTED);
		return -1;
	default:
		log_print(T_UNKNOWN_ERROR);
		return -1;
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

static void writeflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_CARD_FLAGS);
	}
}

static void destroyflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROY_CARD_FLAGS);
	}
}

static void scriptflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_SCRIPT_FLAGS);
	}
}

static void unscriptflag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROYED_SCRIPT_FLAGS);
	}
}

int ptp_connect_init() {
	int rc = ptp_device_init(&ptp_runtime);
	if (rc) {
		log_print(T_DEV_NOT_FOUND);
		return rc;
	}

	rc = ptp_open_session(&ptp_runtime);
	if (rc) {
		return rc;
	}

	ptp_runtime.di = (struct PtpDeviceInfo *)malloc(sizeof(struct PtpDeviceInfo));
	rc = ptp_get_device_info(&ptp_runtime, ptp_runtime.di);
	if (rc) {
		return rc;
	}

	if (strcmp(ptp_runtime.di->manufacturer, "Canon Inc.")) {
		log_print(T_NOT_CANON_DEVICE, ptp_runtime.di->model);
		return -1;
	}

	return 0;
}

int ptp_connect_deinit() {
	int rc = ptp_close_session(&ptp_runtime);
	if (rc) return rc;

	ptp_device_close(&ptp_runtime);

	return 0;
}

void *deviceinfo_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)1;

	log_print(
		"Manufacturer: %s\n"
		"Model: %s\n"
		"DeviceVersion: %s\n"
		"SerialNumber: %s\n",
		ptp_runtime.di->manufacturer, ptp_runtime.di->model,
		ptp_runtime.di->device_version, ptp_runtime.di->serial_number
	);

	ptp_connect_deinit();
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

static void *eventproc_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)1;

	uintptr_t rc = (uintptr_t)canon_evproc_run(&ptp_runtime, (char *)arg);
	if (rc) return (void *)rc;

	rc = (uintptr_t)ptp_connect_deinit();
	return (void *)rc;
}

// Run a custom event proc from input
static void eventproc(GtkWidget *widget, gpointer data) {
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

	if (pthread_create(&thread, NULL, eventproc_thread, (void *)ENABLE_BOOT_DISK)) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	if (result) {
		log_print(T_BOOT_DISK_ENABLE_FAIL);
	} else {
		log_print(T_BOOT_DISK_ENABLE_SUCCESS);
	}
}

static void disablebootdisk(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	if (pthread_create(&thread, NULL, eventproc_thread, (void *)DISABLE_BOOT_DISK)) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	if (result) {
		log_print(T_DISABLE_BOOT_DISK_FAIL);
	} else {
		log_print(T_DISABLE_BOOT_DISK_SUCCESS);
	}
}

static void showdrive(GtkWidget *widget, gpointer data)
{
	log_clear();
	char buffer[1024];
	int rc = drive_get_usable(buffer, sizeof(buffer));
	if (log_drive_error(rc) == 0) {
		log_print(buffer);
	}
}

void *oneclick_thread(void *arg) {
	return NULL;
}

static void oneclick(GtkWidget *widget, gpointer data) {
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
	gtk_main_quit();
	return FALSE;
}

typedef void (*GtkSignalCallback)(GtkWidget *widget, gpointer data);

static GtkWidget *add_big_button(GtkWidget *grid, char *text, char *tip, GtkSignalCallback function, int *order) {
	GtkWidget *button = gtk_button_new_with_label((const char *)text);                       
	g_signal_connect(button, "clicked", G_CALLBACK(function), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, (*order)++, 1, 1);
	gtk_widget_set_tooltip_text(button, (const char *)tip);                       
	gtk_widget_set_hexpand(button, TRUE);                           
	gtk_widget_show(button);

	return button;	
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

	g_print(T_APP_NAME " by Daniel C. Use at your own risk!\n");
	g_print("https://github.com/petabyt/mlinstall\n");
	g_print("https://www.magiclantern.fm/forum/index.php?topic=26162\n");

	ptp_generic_init(&ptp_runtime);

	// What kind of idiot thinks it's a good idea to write a UI in C?
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), T_APP_NAME);
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
	gtk_label_set_markup(GTK_LABEL(label), T_XML_TITLE_TEXT);
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

	label = gtk_label_new(T_USB_STUFF_TITLE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	add_big_button(grid, T_GET_DEVICE_INFO, "Gets info on the camera.", deviceinfo, &order);

	add_big_button(grid, T_ENABLE_BOOT_DISK, "Write the bootdisk flag in the\ncamera, not on the card.", enablebootdisk, &order);

	add_big_button(grid, T_DISABLE_BOOT_DISK, "Disable the camera's bootdisk flag.", disablebootdisk, &order);

	label = gtk_label_new(T_USB);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), T_CARD_STUFF_TITLE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	add_big_button(grid, T_WRITE_CARD_BOOT_FLAGS,
		"Writes EOS_DIGITAL and BOOTDISK to a\nmounted SD/CF card named EOS_DIGITAL.", writeflag, &order);

	add_big_button(grid, T_DESTROY_CARD_BOOT_FLAGS,
		"Destroys boot flags by replacing their\nfirst character with an underscore.", destroyflag, &order);

	add_big_button(grid, T_MAKE_CARD_SCRIPTABLE,
		"Allows SD/CF card to run Canon Basic code.", scriptflag, &order);

	add_big_button(grid, T_MAKE_CARD_UNSCRIPTABLE,
		"Destroys script flags, same method as destroy card boot flags.", unscriptflag, &order);

	label = gtk_label_new(T_CARD);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);
	order = 0;

	label = gtk_label_new(T_DEV_MODE_WARNING);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	GtkEntryBuffer *buf = gtk_entry_buffer_new(TURN_OFF_DISPLAY, strlen(TURN_OFF_DISPLAY));
	entry = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), entry, 0, order++, 1, 1);
	g_signal_connect(entry, "activate", G_CALLBACK(eventproc), NULL);
	gtk_widget_show(entry);

	add_big_button(grid, T_DETECT_CARD, "Try and detect the EOS_DIGITAL drive.", unscriptflag, &order);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), T_APP_INFO);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	label = gtk_label_new(T_ADVANCED);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	gtk_grid_attach(GTK_GRID(mainGrid), logw, 0, 2, 1, 1);

	log_clear();
	log_print(T_WELCOME_LOG);

	gtk_widget_show(mainGrid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
// What kind of idiot would write a UI in C. This is seriously stupid.
