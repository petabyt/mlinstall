#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "app.h"
#include "appstore.h"
#include "drive.h"
#include "lang.h"

FILE *appstore_f = NULL;

static int downloadmodule(GtkWidget *widget, gpointer data) {
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

static int removemodule(GtkWidget *widget, gpointer data) {
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
		log_print(T_DRIVE_NOT_FOUND);
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

int appstore_add_to_notebook(GtkWidget *notebook, int order) {
	GtkWidget *scrollWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrollWindow);

	GtkWidget *grid = gtk_grid_new();
	gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
	gtk_container_add(GTK_CONTAINER(scrollWindow), grid);
	gtk_widget_show(grid);
	order = 0;

	// MENU_ADD_BUTTON("Pull Database", appstore, "Load the list of downloadable modules")

	GtkWidget *label = gtk_label_new("App Store");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollWindow, label);

	return 0;
}

int appstore_getname(char *buffer, char filename[], int n)
{
	char usableDrive[1024];
	drive_get_usable(usableDrive, sizeof(usableDrive));

	char *extension = filename;
	while (*extension != '.') {
		extension++;
		if (*extension == '\0') {
			puts("File name does not have extension!");
			return 1;
		}
	}

	if (!strcmp(extension, ".mo")) {
		snprintf(buffer, n, "%s/ML/modules/%s", usableDrive, filename);
	} else if (!strcmp(extension, ".lua")) {
		snprintf(buffer, n, "%s/ML/scripts/%s", usableDrive, filename);
	} else {
		printf("Unsupported file extension %s\n", extension);
		return 2;
	}

	return 0;
}

int appstore_init()
{
	platform_download("https://raw.githubusercontent.com/petabyt/mlinstall/master/repo/store",
			  "ML_TEMP");
	if (appstore_f == NULL) {
		appstore_f = fopen("ML_TEMP", "r");
		if (appstore_f == NULL) {
			return 1;
		}
	}
}

int appstore_next(struct AppstoreFields *fields)
{
	char buffer[MAX_FIELD];
	int order = 0;
	while (1) {
		if (!fgets(buffer, MAX_FIELD, appstore_f)) {
			return 1;
		}

		// Allow repository comments
		if (buffer[0] == '#') {
			continue;
		}

		// Strip newline from fgets
		strtok(buffer, "\n");

		if (!strncmp(buffer, "-----", 5)) {
			return 0;
		}

		switch (order) {
		case 0:
			strncpy(fields->name, buffer, MAX_FIELD);
			break;
		case 1:
			strncpy(fields->website, buffer, MAX_FIELD);
			break;
		case 2:
			strncpy(fields->download, buffer, MAX_FIELD);
			break;
		case 3:
			strncpy(fields->description, buffer, MAX_FIELD);
			break;
		case 4:
			strncpy(fields->author, buffer, MAX_FIELD);
			break;
		default:
			puts("appstore_next: Found an extra field");
		}

		order++;
	}

	return 0;
}

int appstore_close()
{
	fclose(appstore_f);
	remove("ML_TEMP");
}

int appstore_download(char name[], char download[])
{
	char usableDrive[1024];
	if (drive_get_usable(usableDrive, sizeof(usableDrive))) {
		return 1;
	}

	char toDownload[1024];
	appstore_getname(toDownload, name, sizeof(toDownload));

	return platform_download(download, toDownload);
}

int appstore_remove(char name[])
{
	char usableDrive[1024];
	drive_get_usable(usableDrive, sizeof(usableDrive));

	char toRemove[1024];

	appstore_getname(toRemove, name, sizeof(toRemove));

	remove(toRemove);

	return 0;
}
