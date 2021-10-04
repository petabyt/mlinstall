#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "appstore.h"
#include "drive.h"

FILE *appstore_f = NULL;

int appstore_init()
{
	platform_download("https://raw.githubusercontent.com/petabyt/mlinstall/master/repo/store", "ML_TEMP");
	if (appstore_f == NULL) {
		appstore_f = fopen("ML_TEMP", "r");
		if (appstore_f == NULL) {
			return 1;
		}
	}
}

int appstore_next(struct AppstoreFields *fields) {
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
			puts("Found an extra field");
		}

		order++;
	}

	return 0;
}

int appstore_close() {
	fclose(appstore_f);
	remove("ML_TEMP");
}


int appstore_download(char name[], char download[]) {
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

	return platform_download(download, toDownload);
}

int appstore_remove(char name[]) {
	char usableDrive[1024];
	flag_usable_drive(usableDrive);

	char toRemove[2048];
	snprintf(toRemove, 2048, "%s/ML/modules/%s", usableDrive, name);

	remove(toRemove);
	
	return 0;
}
