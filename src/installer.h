#ifndef INSTALLER_H
#define INSTALLER_H

enum InstallerError { NO_AVAILABLE_FIRMWARE = 2, CAMERA_UNSUPPORTED = 1 };

int installer_start(char model[], char version[]);
int installer_remove();

#endif
