#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "lang.h"

struct PtpRuntime ptp_runtime;
int dev_flag = 0;

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d")) {
			dev_flag = 1;
		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf(
				T_APP_NAME " version " T_APP_VERSION " - Use at your own risk!\n"
				"https://github.com/petabyt/mlinstall\n"
				"Command line flags:\n"
				"-d              Enable developer mode\n"
				"-h              Show this message\n"
				"-l              List connected devices\n"
				"-upload <file>  Upload file to SD card through filewrite evproc\n"
				"\n"
			);

			return 0;
		}
	}

	return app_main_window();
}
