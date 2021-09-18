#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>
#include <gtk/gtk.h>
#include <ctype.h>

#include "src/config.h"
#include "src/ptp.h"
#include "src/ptpcam.h"

#include "src/drive.h"
#include "src/model.h"
#include "src/installer.h"
#include "src/evproc.h"
#include "src/appstore.h"
#include "src/platform.h"

char *driveNotFound = "Could not find card. Make sure\nto run as Administrator/superuser.";
char *driveNotSupported = "Card not supported.\nSee console message.";

// Quick, messy, logging mechanism:
GtkWidget *logw;
char logbuf[1000] = "\nLog info will go here.\n";

void logprint(char string[])
{
	strcat(logbuf, string);
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

void logclear()
{
	strcpy(logbuf, "\n");
	gtk_label_set_text(GTK_LABEL(logw), logbuf);
}

// Log a return message after doing a usb thing
int returnMessage(unsigned int code)
{
	switch (code) {
	case 0:
		logprint("No PTP/USB device found.\n");
		return 1;
	}

	char buf[128];
	sprintf(buf, "Response Code: %x\n", code);
	logprint(buf);

	switch (code) {
	case 0x2001:
		logprint("Return Code OK.\n");
		return 0;
	case 0x201d:
		logprint("Return Code INVALID.\n");
		return 1;
	}

	return 0;
}

static void writeflag(GtkWidget *widget, gpointer data)
{
	logclear();
	switch (flag_write_flag(FLAG_BOOT)) {
	case DRIVE_UNSUPPORTED:
		logprint(driveNotSupported);
		return;
	case DRIVE_NOT_AVAILABLE:
		logprint(driveNotFound);
		return;
	case 0:
		logprint("Wrote card flags on EOS_DIGITAL");
		flag_close();
	}
}

static void destroyflag(GtkWidget *widget, gpointer data)
{
	logclear();
	switch (flag_write_flag(FLAG_DESTROY_BOOT)) {
	case DRIVE_UNSUPPORTED:
		logprint(driveNotSupported);
		return;
	case DRIVE_NOT_AVAILABLE:
		logprint(driveNotFound);
		return;
	case 0:
		logprint("Overwrote card flags.");
		flag_close();
	}
}

static void scriptflag(GtkWidget *widget, gpointer data)
{
	logclear();
	switch (flag_write_flag(FLAG_SCRIPT)) {
	case DRIVE_UNSUPPORTED:
		logprint(driveNotSupported);
		return;
	case DRIVE_NOT_AVAILABLE:
		logprint(driveNotFound);
		return;
	case 0:
		logprint("Wrote script flags.");
		flag_close();
	}
}

static void unscriptflag(GtkWidget *widget, gpointer data)
{
	logclear();
	switch (flag_write_flag(FLAG_DESTROY_SCRIPT)) {
	case DRIVE_UNSUPPORTED:
		logprint(driveNotSupported);
		return;
	case DRIVE_NOT_AVAILABLE:
		logprint(driveNotFound);
		return;
	case 0:
		logprint("Destroyed script flags.");
		flag_close();
	}
}

static void deviceinfo(GtkWidget *widget, gpointer data)
{
	logclear();

	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return;
	}

	PTPDeviceInfo info;
	ptp_getdeviceinfo(&params, &info);

	char buffer[256];
	sprintf(buffer,
		"Manufacturer: %s\n"
		"Model: %s\n"
		"DeviceVersion: %s\n"
		"SerialNumber: %s\n",
		info.Manufacturer, info.Model, info.DeviceVersion, info.SerialNumber);

	logprint(buffer);

	// Test model detector
	printf("model_get() = %d\n", model_get(info.Model));

	close_camera(&ptp_usb, &params, dev);
}

// Run a custom event proc from input
static void eventproc(GtkWidget *widget, gpointer data)
{
	logclear();
	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(widget));
	returnMessage(evproc_run((char *)entry));
}

static void enablebootdisk(GtkWidget *widget, gpointer data)
{
	logclear();
	if (returnMessage(evproc_run("EnableBootDisk"))) {
		logprint("Couldn't enable boot disk.\n");
	} else {
		logprint("Enabled boot disk\n");
	}
}

static void disablebootdisk(GtkWidget *widget, gpointer data)
{
	logclear();
	if (returnMessage(evproc_run("DisableBootDisk"))) {
		logprint("Couldn't disable boot disk.\n");
	} else {
		logprint("Disabled boot disk\n");
	}
}

static void showdrive(GtkWidget *widget, gpointer data)
{
	logclear();
	char buffer[128];
	if (flag_usable_drive(buffer)) {
		logprint("Error getting usable drive.\n");
	} else {
		logprint(buffer);
	}
}

static void oneclick(GtkWidget *widget, gpointer data)
{
	logclear();

	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return;
	}

	PTPDeviceInfo info;
	ptp_getdeviceinfo(&params, &info);

	close_camera(&ptp_usb, &params, dev);
	
	switch (installer_start(info.Model, info.DeviceVersion)) {
	case NO_AVAILABLE_FIRMWARE:
		logprint("Your camera model has a working build,\n"
			 "but not for your firmware version.");
		break;
	case CAMERA_UNSUPPORTED:
		logprint("Your camera model is not supported.\n"
			 "Come back in 5 years and check again.");
		break;
	}
}

static void activate9052(GtkWidget *widget, gpointer data)
{
	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return;
	}

	ptp_activate9052(&params);
}

static void removemodule(GtkWidget *widget, gpointer data);

static void downloadmodule(GtkWidget *widget, gpointer data) {
	logclear();

	char *name = g_object_get_data(G_OBJECT(widget), "name");
	char *download = g_object_get_data(G_OBJECT(widget), "download");

	char usableDrive[1024];
	flag_usable_drive(usableDrive);

	// We'll support Windows line endings just in case,
	// Since URLDownloadFileA is going to be called
	char toDownload[2048];
	#ifdef WIN32
		snprintf(toDownload, 2048, "%s\\ML\\modules\\%s", usableDrive, name);
	#endif
	#ifdef __unix__
		snprintf(toDownload, 2048, "%s/ML/modules/%s", usableDrive, name);
	#endif

	if (platform_download(download, toDownload)) {
		logprint("Error downloading module.");
	} else {
		logprint("Module downloaded to card.");
		gtk_button_set_label(GTK_BUTTON(widget), "Remove");
		g_signal_connect(widget, "clicked", G_CALLBACK(removemodule), NULL);
	}
}

static void removemodule(GtkWidget *widget, gpointer data) {
	logclear();
	
	char *name = g_object_get_data(G_OBJECT(widget), "name");
		
	char usableDrive[1024];
	flag_usable_drive(usableDrive);

	char toRemove[2048];
	snprintf(toRemove, 2048, "%s/ML/modules/%s", usableDrive, name);

	remove(toRemove);
	
	logprint("Module removed.");

	gtk_button_set_label(GTK_BUTTON(widget), "Install");
	g_signal_connect(widget, "clicked", G_CALLBACK(downloadmodule), NULL);
}

static void appstore(GtkWidget *widget, gpointer data)
{
	GtkWidget *grid = gtk_widget_get_parent(widget);
	gtk_widget_destroy(widget);

	char usableDrive[1024];
	if (flag_usable_drive(usableDrive)) {
		logprint("Could not find EOS_DIGITAL.");
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
		snprintf(moduleTest, 4096, "%s/ML/modules/%s", usableDrive, fields.name);

		GtkWidget *button;
		FILE *f = fopen(moduleTest, "r");
		if (f == NULL) {
			button = gtk_button_new_with_label("Install");
			g_signal_connect(button, "clicked", G_CALLBACK(downloadmodule), NULL);
		} else {
			button = gtk_button_new_with_label("Remove");
			g_signal_connect(button, "clicked", G_CALLBACK(removemodule), NULL);
			fclose(f);
		}

		gtk_widget_set_halign(button, GTK_ALIGN_END);
		gtk_grid_attach(GTK_GRID(app), button, 1, 1, 1, 1);
		gtk_widget_show(button);

		// TODO: Free memory :/
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

	gtk_init(&argc, &argv);

	g_print("Early release of ML Install. Use at your own risk!\n");
	g_print("https://github.com/petabyt/mlinstall\n");
	g_print("https://www.magiclantern.fm/forum/index.php?topic=26162\n");

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "ML Install");
	gtk_window_set_default_size(GTK_WINDOW(window), 375, 500);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	mainGrid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), mainGrid);

	logw = gtk_label_new(logbuf);
	gtk_label_set_justify(GTK_LABEL(logw), GTK_JUSTIFY_CENTER);
	gtk_widget_show(logw);

	// Add title label
	label = gtk_label_new(NULL);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_markup(GTK_LABEL(label),
			     "<span size=\"large\">ML USB Installation Tools</span>\n"
			     "(Early Release)\n"
			     "<span size=\"small\">THIS IS NOT GARUNTEED TO WORK\n"
			     "OR NOT BLOW UP YOUR CAMERA\n"
			     "KEEP BOTH PIECES IF YOU BREAK IT</span>\n");
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

	label = gtk_label_new("This will run event procedures on your\n"
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
			     "<span size=\"small\">This code has not been tested much and may\n"
			     "be dangerous. Use EOSCard if possible.</span>\n"
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

	label = gtk_label_new("Execute a custom event procedure:");
	gtk_widget_set_hexpand(label, TRUE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	GtkEntryBuffer *buf = gtk_entry_buffer_new("TurnOffDisplay", 14);
	entry = gtk_entry_new_with_buffer(buf);
	gtk_grid_attach(GTK_GRID(grid), entry, 0, order++, 1, 1);
	g_signal_connect(entry, "activate", G_CALLBACK(eventproc), NULL);
	gtk_widget_show(entry);

	MENU_ADD_BUTTON("Detect EOS_DIGITAL", showdrive, "Try and detect the EOS_DIGITAL drive.")

	MENU_ADD_BUTTON("Activate evproc execution", activate9052, "Run command 0x9050 to try and activate commands such as 0x9052")

	label = gtk_label_new(NULL);
	gtk_label_set_markup(
		GTK_LABEL(label),
		"\nMade by <a href='https://petabyt.dev/'>Daniel C</a>\n"
		"Source code: <a href='https://github.com/petabyt/mlinstall'>github.com/petabyt/mlinstall</a>\n\n"
		"Licenced under GNU General Public License v2.0\n"
		"If it breaks, you get to keep both pieces!");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	label = gtk_label_new("Advanced");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

#ifdef DEV

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);
	order = 0;

	MENU_ADD_BUTTON("Don't click me!", oneclick, "No, Bad! No biscuit!")

	label = gtk_label_new("Quick Install");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

#endif

	GtkWidget *scrollWindow = gtk_scrolled_window_new(NULL, NULL);
	//gtk_widget_set_size_request(scrollWindow, -1, 200);
	gtk_widget_show(scrollWindow);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_container_add(GTK_CONTAINER(scrollWindow), grid);
	gtk_widget_show(grid);
	order = 0;

	MENU_ADD_BUTTON("View Repository", appstore, "Update repo")

	label = gtk_label_new("Module Store");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollWindow, label);

	gtk_grid_attach(GTK_GRID(mainGrid), logw, 0, 2, 1, 1);

	gtk_widget_show(mainGrid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
