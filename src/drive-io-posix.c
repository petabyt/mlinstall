// Darwin port of the drive_io interface: POSIX raw I/O + diskutil unmount.
// Like the old drive-unix.c darwin code, this path is largely untested.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

#include "drive.h"

static int fd = -1;

// First mounted volume labeled EOS_DIGITAL
static int find_mount(char *dev, int dn, char *path, int pn) {
	FILE *c = popen("mount | grep EOS_DIGITAL", "r");
	if (c == NULL) return DRIVE_ERROR;

	char line[256];
	if (fgets(line, sizeof(line), c) == NULL) {
		pclose(c);
		return DRIVE_NONE;
	}
	pclose(c);

	// "/dev/disk2s1 on /Volumes/EOS_DIGITAL (msdos, local, ...)"
	char *on = strstr(line, " on ");
	if (on == NULL) return DRIVE_NONE;
	*on = '\0';

	if (strncmp(line, "/dev/", 5) != 0) return DRIVE_NONE;

	char *mountpoint = on + 4;
	char *end = strchr(mountpoint, ' ');
	if (end != NULL) *end = '\0';
	end = strchr(mountpoint, '\n');
	if (end != NULL) *end = '\0';

	snprintf(dev, dn, "%s", line);
	snprintf(path, pn, "%s", mountpoint);

	return 0;
}

int drive_io_discover(char *buf, int n) {
	char path[128];
	return find_mount(buf, n, path, sizeof(path));
}

int drive_get_usable(char buffer[], int n) {
	char dev[128];
	return find_mount(dev, sizeof(dev), buffer, n);
}

int drive_get_info(char *name, int name_n, uint64_t *bytes) {
	char dev[128];
	char path[128];
	int rc = find_mount(dev, sizeof(dev), path, sizeof(path));
	if (rc) return rc;

	struct statfs s;
	if (statfs(path, &s) != 0) return DRIVE_ERROR;

	// Volume name is the basename of the mountpoint
	char *slash = strrchr(path, '/');
	snprintf(name, name_n, "%s", slash != NULL ? slash + 1 : path);

	*bytes = (uint64_t)s.f_blocks * s.f_bsize;
	return 0;
}

int drive_io_open(const char *device) {
	fd = open(device, O_RDWR);
	if (fd < 0) {
		if (errno == EACCES || errno == EPERM) return DRIVE_PRIV;
		if (errno == ENOENT) return DRIVE_NONE;
		return DRIVE_ERROR;
	}

	// Best effort unmount, like the old darwin_unmount()
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "diskutil unmount %s", device);
	if (system(cmd) != 0) {
		log_print("Error unmounting drive.");
		close(fd);
		fd = -1;
		return DRIVE_ERROR;
	}

	return 0;
}

static int read_full(uint64_t offset, void *buf, int len) {
	unsigned char *p = buf;
	int remaining = len;

	while (remaining > 0) {
		ssize_t n = pread(fd, p, remaining, (off_t)offset);

		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) return DRIVE_ERROR;

		p += n;
		offset += n;
		remaining -= (int)n;
	}

	return len;
}

static int write_full(uint64_t offset, const void *buf, int len) {
	const unsigned char *p = buf;
	int remaining = len;

	while (remaining > 0) {
		ssize_t n = pwrite(fd, p, remaining, (off_t)offset);

		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) return DRIVE_ERROR;

		p += n;
		offset += n;
		remaining -= (int)n;
	}

	return len;
}

int drive_io_pread(uint64_t offset, void *buf, int len) {
	return read_full(offset, buf, len);
}

int drive_io_pwrite(uint64_t offset, const void *buf, int len) {
	return write_full(offset, buf, len);
}

int drive_io_sync(void) {
	if (fsync(fd)) return DRIVE_ERROR;
	return 0;
}

int drive_io_close(void) {
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	return 0;
}