#ifndef PLATFORM_H
#define PLATFORM_H

#include <camlib.h>

// Defined in main.c
struct PtpRuntime *ptp_get(void);
extern int dev_flag;

// EvProcs
#define ENABLE_BOOT_DISK "EnableBootDisk"
#define DISABLE_BOOT_DISK "DisableBootDisk"
#define TURN_OFF_DISPLAY "TurnOffDisplay"

void log_print(char *format, ...);
void log_clear();

int ptp_connect_deinit();
int ptp_connect_init();

int app_main_window();

// Model string evaluator (doesn't seem to be useful)
int model_get(char name[]);

int ptp_canon_activate_command(struct PtpRuntime *r, int code, int value);
int ptp_canon_exec_evproc(struct PtpRuntime *r, void *data, int length);

int canon_evproc_run(struct PtpRuntime *r, char string[]);

// Mostly old deprecated stuff
int platform_download(char in[], char out[]);

enum InstallerError {
	NO_AVAILABLE_FIRMWARE = 2,
	CAMERA_UNSUPPORTED = 1
};

int installer_start(char model[], char version[]);
int installer_remove();

#endif
