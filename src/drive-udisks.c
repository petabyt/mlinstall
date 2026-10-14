// Linux port of the drive_io interface for mlinstall-card: verifies the target really
// is the EOS_DIGITAL card (we may run as root via pkexec, so never trust
// the caller's path), unmounts it through udisks2, then does POSIX raw I/O.
// On close the card is mounted again.

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <udisks/udisks.h>

#include "drive.h"

static int fd = -1;
static char opened_dev[128] = { 0 };

static UDisksClient *client_new(void) {
	GError *error = NULL;
	UDisksClient *client = udisks_client_new_sync(NULL, &error);
	if (client == NULL) {
		log_print("Couldn't connect to udisks2: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	return client;
}

// Block object for `dev`, or the block object labeled EOS_DIGITAL
// when dev == NULL. Returns a reffed object or NULL.
static UDisksObject *find_block(UDisksClient *client, const char *dev) {
	GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(client));
	UDisksObject *found = NULL;

	for (GList *l = objects; l != NULL; l = l->next) {
		UDisksObject *object = UDISKS_OBJECT(l->data);
		UDisksBlock *block = udisks_object_peek_block(object);
		if (block == NULL) continue;

		if (dev != NULL) {
			if (!strcmp(udisks_block_get_device(block), dev)) {
				found = g_object_ref(object);
				break;
			}
		} else {
			const gchar *label = udisks_block_get_id_label(block);
			if (label != NULL && !strcmp(label, "EOS_DIGITAL")) {
				found = g_object_ref(object);
				break;
			}
		}
	}

	g_list_free_full(objects, g_object_unref);
	return found;
}

int drive_io_discover(char *buf, int n) {
	UDisksClient *client = client_new();
	if (client == NULL) return DRIVE_ERROR;

	UDisksObject *object = find_block(client, NULL);
	if (object == NULL) {
		g_object_unref(client);
		return DRIVE_NONE;
	}

	UDisksBlock *block = udisks_object_peek_block(object);
	snprintf(buf, n, "%s", udisks_block_get_device(block));

	g_object_unref(object);
	g_object_unref(client);

	return 0;
}

int drive_get_usable(char buffer[], int n) {
	UDisksClient *client = client_new();
	if (client == NULL) return DRIVE_ERROR;

	int rc = DRIVE_NONE;
	UDisksObject *object = find_block(client, NULL);
	if (object != NULL) {
		UDisksFilesystem *fs = udisks_object_peek_filesystem(object);
		if (fs != NULL) {
			const gchar *const *points = udisks_filesystem_get_mount_points(fs);
			if (points != NULL && points[0] != NULL) {
				snprintf(buffer, n, "%s", points[0]);
				rc = 0;
			}
		}

		g_object_unref(object);
	}

	g_object_unref(client);
	return rc;
}

static int unmount_card(UDisksObject *object) {
	UDisksFilesystem *fs = udisks_object_peek_filesystem(object);
	if (fs == NULL) return 0;

	// udisks2 waits briefly for holders to release the device; retry
	// a few times in case something is still busy
	for (int i = 0; i < 3; i++) {
		GError *error = NULL;
		if (udisks_filesystem_call_unmount_sync(fs, g_variant_new("a{sv}", NULL), NULL, &error)) {
			return 0;
		}

		char *remote = g_dbus_error_get_remote_error(error);
		int not_mounted = remote != NULL && strstr(remote, "NotMounted") != NULL;
		g_free(remote);
		if (not_mounted) {
			g_error_free(error);
			return 0;
		}

		log_print("Unmount failed (attempt %d): %s", i + 1, error->message);
		g_error_free(error);
		usleep(300 * 1000);
	}

	return DRIVE_ERROR;
}

int drive_io_open(const char *device) {
	if (strncmp(device, "/dev/", 5) != 0) {
		log_print("Refusing non-device path: %s", device);
		return DRIVE_ERROR;
	}

	// Fail fast on permissions before unmounting anything
	fd = open(device, O_RDWR | O_SYNC);
	if (fd < 0) {
		if (errno == EACCES || errno == EPERM) {
			log_print("Need elevated permissions to open %s", device);
			return DRIVE_PRIV;
		}

		if (errno == ENOENT) {
			log_print("No such device: %s", device);
			return DRIVE_NONE;
		}

		log_print("Couldn't open %s: %s", device, strerror(errno));
		return DRIVE_ERROR;
	}

	// We may run as root via pkexec, so never trust the caller's
	// path: check that udisks2 labels this device EOS_DIGITAL
	UDisksClient *client = client_new();
	if (client == NULL) {
		close(fd);
		fd = -1;
		return DRIVE_ERROR;
	}

	UDisksObject *object = find_block(client, device);
	if (object == NULL) {
		log_print("udisks2 doesn't know device %s", device);
		g_object_unref(client);
		close(fd);
		fd = -1;
		return DRIVE_NONE;
	}

	const gchar *label = udisks_block_get_id_label(udisks_object_peek_block(object));
	if (label == NULL || strcmp(label, "EOS_DIGITAL") != 0) {
		log_print("Refusing to write to %s (label '%s' != EOS_DIGITAL)",
			  device, label != NULL ? label : "");
		g_object_unref(object);
		g_object_unref(client);
		close(fd);
		fd = -1;
		return DRIVE_ERROR;
	}

	int rc = unmount_card(object);
	g_object_unref(object);
	g_object_unref(client);
	if (rc) {
		close(fd);
		fd = -1;
		return rc;
	}

	snprintf(opened_dev, sizeof(opened_dev), "%s", device);

	return 0;
}

int drive_io_close(void) {
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	if (opened_dev[0] == '\0') return 0;

	int rc = 0;
	UDisksClient *client = client_new();
	if (client != NULL) {
		UDisksObject *object = find_block(client, opened_dev);
		if (object != NULL) {
			UDisksFilesystem *fs = udisks_object_peek_filesystem(object);
			if (fs != NULL) {
				GError *error = NULL;
				gchar *path = NULL;
				if (!udisks_filesystem_call_mount_sync(fs, g_variant_new("a{sv}", NULL),
								       &path, NULL, &error)) {
					char *remote = g_dbus_error_get_remote_error(error);
					int already = remote != NULL && strstr(remote, "AlreadyMounted") != NULL;
					g_free(remote);

					if (!already) {
						log_print("Couldn't remount %s: %s", opened_dev, error->message);
						rc = DRIVE_ERROR;
					}
					g_error_free(error);
				} else {
					log_print("Remounted %s at %s", opened_dev, path);
					g_free(path);
				}
			}

			g_object_unref(object);
		}

		g_object_unref(client);
	}

	opened_dev[0] = '\0';
	return rc;
}

static int read_full(uint64_t offset, void *buf, int len) {
	unsigned char *p = buf;
	int remaining = len;

	while (remaining > 0) {
		ssize_t n = pread(fd, p, remaining, (off_t)offset);

		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) {
			log_print("Read error at 0x%llx: %s", (unsigned long long)offset,
				  n < 0 ? strerror(errno) : "unexpected end of device");
			return DRIVE_ERROR;
		}

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
		if (n <= 0) {
			log_print("Write error at 0x%llx: %s", (unsigned long long)offset,
				  n < 0 ? strerror(errno) : "unexpected end of device");
			return DRIVE_ERROR;
		}

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
	if (fsync(fd)) {
		log_print("Sync failed: %s", strerror(errno));
		return DRIVE_ERROR;
	}

	return 0;
}