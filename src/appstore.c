#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "appstore.h"
#include "drive.h"



FILE *appstore_f = NULL;

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
