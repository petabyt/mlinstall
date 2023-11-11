// This is the GTK based app for Windows / Linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include <pthread.h>

#include <camlib.h>
#include <ptp.h>

#include "app.h"
#include "lang.h"
#include "drive.h"
#include "appstore.h"

extern struct PtpRuntime ptp_runtime;
extern int dev_flag;

#define ENABLE_BOOT_DISK "EnableBootDisk"
#define DISABLE_BOOT_DISK "DisableBootDisk"
#define TURN_OFF_DISPLAY "TurnOffDisplay"

static GtkWidget *logw;
static char temp_log_buf[1000] = "";

void log_print(char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, 1024, format, args);
	va_end(args);

	strcat(temp_log_buf, buffer);
	strcat(temp_log_buf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), temp_log_buf);
}

void log_clear()
{
	strcpy(temp_log_buf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), temp_log_buf);
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
// #ifndef WIN32
		// log_print("Make sure you open as sudo.");
		// log_print("(sudo ./mlinstall)");
// #endif
		return rc;
	}

	return 0;
}

static void app_write_flag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_CARD_FLAGS);
	}
}

static void app_destroy_flag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_BOOT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROY_CARD_FLAGS);
	}
}

static void app_script_flag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_WROTE_SCRIPT_FLAGS);
	}
}

static void app_destroy_script_flag(GtkWidget *widget, gpointer data)
{
	log_clear();
	int rc = drive_write_flag(FLAG_DESTROY_SCRIPT);
	if (log_drive_error(rc) == 0) {
		log_print(T_DESTROYED_SCRIPT_FLAGS);
	}
}

int ptp_connect_deinit() {
	int rc = ptp_close_session(&ptp_runtime);
	if (rc) return rc;

	ptp_device_close(&ptp_runtime);

	return 0;
}

#define USB_VENDOR_CANON 0x4A9

int ptp_connect_init() {
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
		log_print("No Canon device found");
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

void *app_device_info_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)1;

	ptp_eos_set_remote_mode(&ptp_runtime, 1);
	ptp_eos_set_event_mode(&ptp_runtime, 1);

	int length = 0;
	struct PtpGenericEvent *s = NULL;

	int rc = ptp_eos_get_event(&ptp_runtime);
	if (rc) return (void *)1;

	length = ptp_eos_events(&ptp_runtime, &s);

	int shutter_count = 0;
	for (int i = 0; i < length; i++) {
		if (s[i].code == 0xD1AC) {
			shutter_count = s[i].value;
		}
	}

	char *fw_version = ptp_runtime.di->device_version;
	if (fw_version[0] == '3' && fw_version[1] == '-') {
		fw_version += 2;
	}

	log_print("Model: %s", ptp_runtime.di->model);
	log_print("Firmware Version: %s", ptp_runtime.di->device_version);
	log_print("Serial Number: %s", ptp_runtime.di->serial_number);
	log_print("Shutter count: %d", shutter_count);

	ptp_connect_deinit();
	return (void *)0;
}

static void app_device_info(GtkWidget *widget, gpointer data) {
	pthread_t thread;

	if (pthread_create(&thread, NULL, app_device_info_thread, NULL)) {
		return;
	}

	if (pthread_join(thread, NULL)) {
		return;
	}
}

static void *app_run_eventproc_thread(void *arg) {
	log_clear();
	if (ptp_connect_init()) return (void *)(-1);

	uintptr_t rc = (uintptr_t)canon_evproc_run(&ptp_runtime, (char *)arg);
	if (rc) return (void *)rc;

	ptp_connect_deinit();
	return (void *)0;
}

// Run a custom event proc from input
static void app_run_eventproc(GtkWidget *widget, gpointer data) {
	log_clear();
	pthread_t thread;

	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(widget));

	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)entry)) {
		return;
	}

	int result;
	if (pthread_join(thread, (void **)&result)) {
		return;
	}

	printf("Response Code: %x\n", result);

	switch (result) {
	case PTP_RC_OK:
		log_print(T_RETURN_CODE_OK);
		return;
	case PTP_RC_InvalidParameter:
		log_print(T_RETURN_INVALID_PARAM);
		return;
	case PTP_RC_OperationNotSupported:
		log_print(T_RETURN_UNSUPPORTED);
		return;
	case -1:
		return;
	default:
		log_print(T_UNKNOWN_ERROR);
		return;
	}
}

static void app_enable_bootdisk(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)ENABLE_BOOT_DISK)) {
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

static void app_disable_bootdisk(GtkWidget *widget, gpointer data)
{
	log_clear();
	pthread_t thread;

	if (pthread_create(&thread, NULL, app_run_eventproc_thread, (void *)DISABLE_BOOT_DISK)) {
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

static void app_show_drive_info(GtkWidget *widget, gpointer data)
{
	log_clear();
	char buffer[1024];
	int rc = drive_get_usable(buffer, sizeof(buffer));
	if (log_drive_error(rc) == 0) {
		log_print(buffer);
	}
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

typedef void (*GtkSignalCallback)(GtkWidget *widget, gpointer data);

static GtkWidget *add_big_button(GtkWidget *grid, char *text, char *tip, GtkSignalCallback function, int *order)
{
	GtkWidget *button = gtk_button_new_with_label((const char *)text);                       
	g_signal_connect(button, "clicked", G_CALLBACK(function), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 0, (*order)++, 1, 1);
	gtk_widget_set_tooltip_text(button, (const char *)tip);                       
	gtk_widget_set_hexpand(button, TRUE);
	gtk_widget_set_margin_bottom(button, 4);
	gtk_widget_show(button);

	return button;	
}

int app_main_window()
{
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *grid;
	GtkWidget *mainGrid;

	gtk_init(NULL, NULL);

	g_print(T_APP_NAME " by Daniel C. Use at your own risk!\n");
	g_print("https://github.com/petabyt/mlinstall\n");
	g_print("https://www.magiclantern.fm/forum/index.php?topic=26162\n");

	extern guint8 favicon_ico[] asm("favicon_ico");
	extern gsize favicon_ico_length asm("favicon_ico_length");

	GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(loader, favicon_ico, favicon_ico_length, NULL);
	gdk_pixbuf_loader_close(loader, NULL);
	GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);

	ptp_generic_init(&ptp_runtime);

	// What kind of idiot thinks it's a good idea to write a UI in C?
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_icon(GTK_WINDOW(window), pixbuf);
	gtk_window_set_title(GTK_WINDOW(window), T_APP_NAME);
	gtk_window_set_default_size(GTK_WINDOW(window), 375, 500);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	mainGrid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), mainGrid);

	label = gtk_label_new(NULL);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_markup(GTK_LABEL(label), T_XML_TITLE_TEXT);
	gtk_widget_set_margin_bottom(label, 10);
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
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_widget_show(label);

	add_big_button(grid, T_GET_DEVICE_INFO, "Gets info on the camera.", app_device_info, &order);

	add_big_button(grid, T_ENABLE_BOOT_DISK, "Write the bootdisk flag in the\ncamera, not on the card.", app_enable_bootdisk, &order);

	add_big_button(grid, T_DISABLE_BOOT_DISK, "Disable the camera's bootdisk flag.", app_disable_bootdisk, &order);

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
		"Writes EOS_DIGITAL and BOOTDISK to a\nmounted SD/CF card named EOS_DIGITAL.", app_write_flag, &order);

	add_big_button(grid, T_DESTROY_CARD_BOOT_FLAGS,
		"Destroys boot flags by replacing their\nfirst character with an underscore.", app_destroy_flag, &order);

	add_big_button(grid, T_MAKE_CARD_SCRIPTABLE,
		"Allows SD/CF card to run Canon Basic code.", app_script_flag, &order);

	add_big_button(grid, T_MAKE_CARD_UNSCRIPTABLE,
		"Destroys script flags, same method as destroy card boot flags.", app_destroy_script_flag, &order);

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
	g_signal_connect(entry, "activate", G_CALLBACK(app_run_eventproc), NULL);
	gtk_widget_show(entry);

	add_big_button(grid, T_DETECT_CARD, "Try and detect the EOS_DIGITAL drive.", app_show_drive_info, &order);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), T_APP_INFO);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	label = gtk_label_new(T_ADVANCED);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	GtkWidget *label_box = gtk_box_new(FALSE, 0);
	gtk_widget_show(label_box);

	logw = gtk_label_new(temp_log_buf);
	gtk_widget_set_hexpand(logw, TRUE);
	gtk_container_add(GTK_CONTAINER(label_box), logw);
	gtk_widget_show(logw);

	gtk_grid_attach(GTK_GRID(mainGrid), label_box, 0, 2, 1, 1);

	log_clear();
	log_print(T_WELCOME_LOG);

	gtk_widget_show(mainGrid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
// What kind of idiot would write a UI in C. This is seriously stupid.
