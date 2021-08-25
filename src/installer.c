#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drive.h"
#include "model.h"

struct Release {
	char name[1024];
	char version[1024];
	char description[1024];
	char download_url[1024];
	char forum_url[1024];	
};

int find(struct Release *release, char name[]) {
	char buffer[1024];
	FILE *f = fopen("ML_TEMP", "r");
	if (!f) {
		return 1;
	}

	int order = 0;
	while (1) {
		if (!fgets(buffer, 1024, f)) {
			return 0;
		}

		strtok(buffer, "\n");

		if (!strncmp(buffer, "-----", 5)) {
			order = 0;
			continue;
		}

		switch (order) {
			case 0:
				strcpy(release->name, buffer);
				break;
			case 1:
				strcpy(release->version, buffer);
				break;
			case 2:
				strcpy(release->description, buffer);
				break;
			case 3:
				strcpy(release->download_url, buffer);
				break;
			case 4:
				strcpy(release->forum_url, buffer);
				break;
		}

		order++;

		if (model_get(release->name) == model_get(name)) {
			return 0;
		}
	}

	return 1;
}

int installer_start() {
	char command[512];

	// Temporary URL
	char *url = "https://petabyt.dev/mlinstall_repo";

	#ifdef WIN32
		snprintf(command, 512,
			"certutil -urlcache -split -f \"%s\" ML_TEMP", url);
	#endif

	#ifdef __unix__
		snprintf(command, 512,
			"curl %s > ML_TEMP", url);
	#endif

	char ret = system(command);
	printf("%u\n", ret);

	struct Release release;
	if (find(&release, "Canon EOS Rebel T6")) {
		puts("Find error");
		return 1;
	}

	printf("Name: %s\n", release.name);
	printf("Required firmware: %s\n", release.version);

	return 0;
}

int installer_remove() {
	char command[512];

	char file[128];
	flag_usable_drive(file);	

	#ifdef __unix__
		snprintf(command, 512,
			"rm -rf %s/autoexec.bin %s/ML", file, file);
	#endif

	printf("Will execute '%s'\n", command);
	system(command);
}
