#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#include "config.h"
#include "ptp.h"
#include "ptpcam.h"

#include "drive.h"
#include "model.h"
#include "installer.h"

// TODO: avoid system shell commands, use
// actual libraries

struct Release {
	char name[1024];
	char version[1024];
	char description[1024];
	char download_url[1024];
	char forum_url[1024];	
};

int find(struct Release *release, char name[], char version[]) {
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

int download(char in[], char out[]) {
	char command[512];
	
	#ifdef WIN32
		snprintf(command, 512,
			"certutil -urlcache -split -f \"%s\" %s", in, out);
	#endif

	#ifdef __unix__
		snprintf(command, 512,
			"curl -L -4 %s --output %s", in, out);
	#endif

	return system(command);
}

int installer_start() {
	download("https://petabyt.dev/mlinstall_repo", "ML_TEMP");

	struct Release release;
	int r = find(&release, "Canon EOS Rebel T6", "1.1.0");
	if (r) {
		return r;
	}

	printf("Found a match for model/firmware version. Downloading\n%s\n",
		release.download_url);

	printf("%s\n", release.download_url);
	download(release.download_url, "ML_RELEASE.ZIP");

	char input[5];
	puts("This will write to your SD card. Continue? (y)");
	fgets(input, 5, stdin);
	if (input[0] != 'y') {
		return 0;
	}

	puts("Unpacking file into SD card...");

	char file[128];
	flag_usable_drive(file);

	char command[512];
	#ifdef __unix__
		snprintf(command, 512, "unzip -o ML_RELEASE.ZIP -d %s", file);
		system(command);
	#endif

	#ifdef WIN32
		FILE *test = fopen("C:\\Program Files\\7-Zip\\7z.exe", "r");
		if (test == NULL) {
			puts("!!!! 7z not found, unzip ML_RELEASE.ZIP onto card manually.");
		} else {
			fclose(test);
			snprintf(command, 512, "C:\\Program Files\\7-Zip\\7z.exe x ML_RELEASE.ZIP -o %s", file);
			system(command);
		}
	#endif

	puts("Writing card flags...");
	if (flag_write_flag(FLAG_BOOT)) {
		puts("Can't write card flags");
		return 1;
	}

	flag_close();

	puts("Running 'EnableBootDisk'...");

	{
		int busn = 0;
		int devn = 0;
		short force = 0;
		PTPParams params;
		PTP_USB ptp_usb;
		struct usb_device *dev;
		
		if (open_camera(busn, devn, force, &ptp_usb, &params, &dev) < 0) {
			puts("Can't open PTP camera!");
			return 1;
		}

		ptp_runeventproc(&params, "EnableBootDisk", NULL);
		puts("Enabled boot disk.");
		close_camera(&ptp_usb, &params, dev);
	}

	puts("Magic Lantern successfully installed.");

	return 0;
}

int installer_remove() {
	char command[512];

	char input[5];
	puts("This will write to your SD card. Continue? (y)");
	fgets(input, 5, stdin);
	if (input[0] != 'y') {
		return 0;
	}

	char file[128];
	flag_usable_drive(file);	

	#ifdef __unix__
		snprintf(command, 512,
			"rm -rf %s/autoexec.bin %s/ML", file, file);
	#endif

	#ifdef WIN32
		snprintf(command, 512,
			"del %s/autoexec.bin %s/ML", file, file);
	#endif

	printf("Will execute '%s'\n", command);
	system(command);

	puts("Destroying card flags...");
	if (flag_write_flag(FLAG_DESTROY_BOOT)) {
		puts("Can't destroy card flags");
		return 1;
	}

	flag_close();
}
