#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "appstore.h"

FILE *appstore_f = NULL;

int appstore_init()
{
	platform_download("https://petabyt.github.io/mlinstall/repo/store", "ML_TEMP");
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
	platform_delete("ML_TEMP");
}
