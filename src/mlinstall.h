#ifndef PLATFORM_H
#define PLATFORM_H

#include <libpict.h>
#include <ui.h>

// Defined in main.c
struct PtpRuntime *ptp_get(void);
extern int dev_flag;

// EvProcs
#define ENABLE_BOOT_DISK "EnableBootDisk"
#define DISABLE_BOOT_DISK "DisableBootDisk"
#define TURN_OFF_DISPLAY "TurnOffDisplay"

void log_print(char *format, ...);
void log_clear(void);

int mlinstall_disconnect(void);
int mlinstall_connect(void);

int mlinstall_main_window(void);

int mlinstall_rim_window(struct RimContext *ctx, void *arg);

int mlinstall_setup_session(struct PtpRuntime *r);

#include "lang.h"
#include "drive.h"

#endif
