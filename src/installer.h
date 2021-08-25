#ifndef INSTALLER_H
#define INSTALLER_H

enum InstallerError {
	NO_AVAILABLE_FIRMWARE = 2,
	CAMERA_UNSUPPORTED = 1
};

int installer_start();
int installer_remove();

#endif
