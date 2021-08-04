// Spiffy GTK+ based GUI app
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <usb.h>
#include <gtk/gtk.h>

#include "src/config.h"
#include "src/ptp.h"
#include "src/ptpcam.h"
#include "src/drive.h"
#include "src/model.h"

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

	printf("model_get() = %d\n", model_get(info.Model));

	close_camera(&ptp_usb, &params, dev);
}

int runcommand(char string[])
{
	int busn = 0;
	int devn = 0;
	short force = 0;
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	g_print("Running %s...\n", string);

	logclear();
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
		returnMessage(0);
		return 1;
	}

	returnMessage((unsigned int)ptp_runeventproc(&params, string));
	close_camera(&ptp_usb, &params, dev);
	return 0;
}

// Run a custom event proc
static void eventproc(GtkWidget *widget, gpointer data)
{
	const gchar *entry = gtk_entry_get_text(GTK_ENTRY(widget));
	runcommand((char *)entry);
}

static void enablebootdisk(GtkWidget *widget, gpointer data)
{
	if (runcommand("EnableBootDisk")) {
		logprint("Couldn't enable boot disk.\n");
	} else {
		logprint("Enabled boot disk\n");
	}
}

static void disablebootdisk(GtkWidget *widget, gpointer data)
{
	if (runcommand("DisableBootDisk")) {
		logprint("Couldn't disable boot disk.\n");
	} else {
		logprint("Disabled boot disk\n");
	}
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

#define MENU_ADD_BUTTON(text, function, tip) \
	button = gtk_button_new_with_label(text); \
	g_signal_connect(button, "clicked", G_CALLBACK(function), NULL); \
	gtk_grid_attach(GTK_GRID(grid), button, 0, order++, 1, 1); \
	gtk_widget_set_tooltip_text( \
		button, \
		tip); \
	gtk_widget_set_hexpand(button, TRUE); \
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

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "ML Install");
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	mainGrid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), mainGrid);

	// Add title label
	label = gtk_label_new(NULL);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_markup(GTK_LABEL(label),
			     "<span size=\"large\">ML USB Installation Tools</span>\n"
			     "(Early Release)\n"
			     "<span size=\"small\">THIS IS NOT GARUNTEED TO WORK\n"
			     "OR NOT KILL YOUR CAMERA\n"
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

	label = gtk_label_new("This will automatically find and write to\n"
							"a card named \"EOS_DIGITAL\".\n\n"
							"This code has not been tested much\n"
							"and may be dangerous. Use EOSCard.\n");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	MENU_ADD_BUTTON(
		"Write card boot flags",
		writeflag,
		"Writes EOS_DIGITAL and BOOTDISK to a\nmounted SD/CF card named EOS_DIGITAL."
	)

	MENU_ADD_BUTTON(
		"Destroy card boot flags",
		destroyflag,
		"Destroys boot flags by replacing their\nfirst character with an underscore."
	)

	MENU_ADD_BUTTON(
		"Make card scriptable",
		scriptflag,
		"Allows SD/CF card to run Canon Basic code."
	)

	MENU_ADD_BUTTON(
		"Make card un-scriptable",
		unscriptflag,
		"Destroys script flags, same method as destroy card boot flags."
	)

	label = gtk_label_new("SD/CF Card");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_widget_show(grid);
	order = 0;

	MENU_ADD_BUTTON(
		"Get Device Info",
		deviceinfo,
		"Show Model, Firmware Version, etc."
	)

	MENU_ADD_BUTTON(
		"Enable Boot Disk",
		enablebootdisk,
		"Write the bootdisk flag inside of the\ncamera, not on the card."
	)

	MENU_ADD_BUTTON(
		"Disable Boot Disk",
		disablebootdisk,
		"Disable the camera's bootdisk flag."
	)

	label = gtk_label_new("USB");
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

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
						"\nMade by <a href='https://petabyt.dev/'>Daniel C</a>\n"
						"Source code: <a href='https://github.com/petabyt/mlinstall'>github.com/petabyt/mlinstall</a>\n\n"
						"Licenced under GNU General Public License v2.0\n"
						"If you break it, you get to keep both pieces!");
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
	gtk_grid_attach(GTK_GRID(grid), label, 0, order++, 1, 1);
	gtk_widget_show(label);

	label = gtk_label_new("Advanced");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, label);

	logw = gtk_label_new(logbuf);
	gtk_grid_attach(GTK_GRID(mainGrid), logw, 0, 2, 1, 1);
	gtk_label_set_justify(GTK_LABEL(logw), GTK_JUSTIFY_CENTER);
	gtk_widget_show(logw);

	gtk_widget_show(mainGrid);
	gtk_widget_show(window);
	gtk_main();

	return 0;
}
