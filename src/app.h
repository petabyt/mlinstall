#ifndef PLATFORM_H
#define PLATFORM_H

#include <camlib.h>

void log_print(char *format, ...);
void log_clear();

int platform_download(char in[], char out[]);

// Model string evaluator
int model_get(char name[]);

int ptp_canon_activate_command(struct PtpRuntime *r, int code, int value);
int ptp_canon_exec_evproc(struct PtpRuntime *r, void *data, int length);

int canon_evproc_run(struct PtpRuntime *r, char string[]);

enum InstallerError {
	NO_AVAILABLE_FIRMWARE = 2,
	CAMERA_UNSUPPORTED = 1
};

int installer_start(char model[], char version[]);
int installer_remove();

#endif
