// PoC for "one click install" - unfinished, don't use
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <camlib.h>

#include "evproc.h"
#include "drive.h"
#include "model.h"
#include "installer.h"
#include "platform.h"

// Linux: unzip
// Windows: 7z (optional)

extern struct PtpRuntime ptp_runtime;

struct Release {
	char name[1024];
	char version[1024];
	char description[1024];
	char download_url[1024];
	char forum_url[1024];
};

static int find(struct Release *release, char name[], char version[])
{
	char buffer[1024];
	FILE *f = fopen("ML_TEMP", "r");
	if (f == NULL) {
		return 1;
	}

	int match = 0;
	int order = 0;
	while (1) {
		if (!fgets(buffer, 1024, f)) {
			return 0;
		}

		// Allow repository comments
		if (buffer[0] == '#') {
			continue;
		}

		// Strip newline from fgets
		strtok(buffer, "\n");

		if (!strncmp(buffer, "-----", 5)) {
			order = 0;
			continue;
		}

		switch (order) {
		case 0:
			strncpy(release->name, buffer, 1024);
			break;
		case 1:
			strncpy(release->version, buffer, 1024);
			break;
		case 2:
			strncpy(release->description, buffer, 1024);
			break;
		case 3:
			strncpy(release->download_url, buffer, 1024);
			break;
		case 4:
			strncpy(release->forum_url, buffer, 1024);
			break;
		default:
			puts("Found an extra field");
		}

		order++;

		if (model_get(release->name) == model_get(name)) {
			match = 1;
			if (!strcmp(release->version, version)) {
				return 0;
			}
		}
	}

	if (match) {
		return NO_AVAILABLE_FIRMWARE;
	}

	return CAMERA_UNSUPPORTED;
}

int installer_start(char model[], char version[])
{
	platform_download("https://raw.githubusercontent.com/petabyt/mlinstall/master/repo/install",
			  "ML_TEMP");

	struct Release release;
	int r = find(&release, model, version);
	if (r) {
		return r;
	}

	remove("ML_TEMP");

	printf("Found a match for your model/firmware version. Downloading\n%s\n",
	       release.download_url);

	printf("%s\n", release.download_url);
	platform_download(release.download_url, "ML_RELEASE.ZIP");

	char input[5];
	puts("This will write to your SD card. Continue? (y)");
	fgets(input, 5, stdin);
	if (input[0] != 'y') {
		return 0;
	}

	puts("Unpacking file into SD card...");

	char file[128];
	drive_get_usable(file, sizeof(file));

	char command[512];
#ifdef __unix__
	snprintf(command, 512, "unzip -o ML_RELEASE.ZIP -d %s", file);
	if (system(command)) {
		puts("!!!! Could not unzip ML_RELEASE.ZIP for some reason. Unzip manually onto card.");
	}
#endif

#ifdef WIN32
	FILE *test = fopen("C:\\Program Files\\7-Zip\\7z.exe", "r");
	if (test == NULL) {
		puts("!!!! 7z not found, unzip ML_RELEASE.ZIP onto card manually.");
	} else {
		fclose(test);
		snprintf(command, 512, "C:\\Program Files\\7-Zip\\7z.exe x ML_RELEASE.ZIP -o %s",
			 file);
		system(command);
	}
#endif

	puts("Writing card flags...");
	if (drive_write_flag(FLAG_BOOT)) {
		puts("Can't write card flags");
		return 1;
	}

	puts("Running 'EnableBootDisk'...");

	canon_evproc_run(&ptp_runtime, "EnableBootDisk");

	puts("Magic Lantern successfully installed.");

	return 0;
}

int installer_remove()
{
	char command[512];

	char input[5];
	puts("This will write to your SD card. Continue? (y)");
	fgets(input, 5, stdin);
	if (input[0] != 'y') {
		return 0;
	}

	char file[128];
	drive_get_usable(file, sizeof(file));

#ifdef __unix__
	snprintf(command, 512, "rm -rf %s/autoexec.bin %s/ML", file, file);
#endif

#ifdef WIN32
	snprintf(command, 512, "del %s/autoexec.bin %s/ML", file, file);
#endif

	printf("Will execute '%s'\n", command);
	system(command);

	puts("Destroying card flags...");
	if (drive_write_flag(FLAG_DESTROY_BOOT)) {
		puts("Can't destroy card flags");
		return 1;
	}
}
