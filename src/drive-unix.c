// Linux GUI drive code. The GUI itself never needs root:
//  - discovery/info via unprivileged udisks2 D-Bus queries
//  - card writes via mlinstall-card, re-executed through pkexec when the
//    helper can't open the device (polkit prompts for a password)
//
// Only drive_write_flag() and drive_get_usable() are linked here; the
// shared flag semantics run inside mlinstall-card.

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gio/gio.h>
#include <udisks/udisks.h>

#include "drive.h"
#include "lang.h"

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

// Card = block object whose filesystem label is EOS_DIGITAL
static UDisksObject *find_card(UDisksClient *client) {
	GList *objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(client));
	UDisksObject *found = NULL;

	for (GList *l = objects; l != NULL; l = l->next) {
		UDisksObject *object = UDISKS_OBJECT(l->data);
		UDisksBlock *block = udisks_object_peek_block(object);
		if (block == NULL) continue;

		const gchar *label = udisks_block_get_id_label(block);
		if (label != NULL && !strcmp(label, "EOS_DIGITAL")) {
			found = g_object_ref(object);
			break;
		}
	}

	g_list_free_full(objects, g_object_unref);
	return found;
}

// Mountpoint of the card for the info button; mounts it if unmounted
int drive_get_usable(char buffer[], int n) {
	UDisksClient *client = client_new();
	if (client == NULL) return DRIVE_ERROR;

	UDisksObject *object = find_card(client);
	if (object == NULL) {
		log_print(T_DRIVE_NOT_FOUND);
		g_object_unref(client);
		return DRIVE_NONE;
	}

	int rc = DRIVE_ERROR;
	UDisksFilesystem *fs = udisks_object_peek_filesystem(object);
	if (fs == NULL) {
		log_print("Card has no filesystem?");
	} else {
		const gchar *const *points = udisks_filesystem_get_mount_points(fs);
		if (points != NULL && points[0] != NULL) {
			snprintf(buffer, n, "%s", points[0]);
			rc = 0;
		} else {
			// Unmounted: mount it for the user (polkit may prompt)
			GError *error = NULL;
			gchar *path = NULL;
			if (udisks_filesystem_call_mount_sync(fs, g_variant_new("a{sv}", NULL),
							       &path, NULL, &error)) {
				snprintf(buffer, n, "%s", path);
				g_free(path);
				rc = 0;
			} else {
				log_print("Couldn't mount card: %s", error->message);
				g_error_free(error);
			}
		}
	}

	g_object_unref(object);
	g_object_unref(client);
	return rc;
}

// Card name (volume label) and capacity, unprivileged D-Bus query
int drive_get_info(char *name, int name_n, uint64_t *bytes) {
	UDisksClient *client = client_new();
	if (client == NULL) return DRIVE_ERROR;

	UDisksObject *object = find_card(client);
	if (object == NULL) {
		g_object_unref(client);
		return DRIVE_NONE;
	}

	UDisksBlock *block = udisks_object_peek_block(object);
	snprintf(name, name_n, "%s", udisks_block_get_id_label(block));
	*bytes = udisks_block_get_size(block);

	g_object_unref(object);
	g_object_unref(client);
	return 0;
}

// ---------- mlinstall-card spawn ----------

static char *mode_name(int mode) {
	switch (mode) {
	case FLAG_ALL: return "all";
	case FLAG_BOOT: return "boot";
	case FLAG_SCRIPT: return "script";
	case FLAG_DESTROY_BOOT: return "destroy-boot";
	case FLAG_DESTROY_SCRIPT: return "destroy-script";
	}

	return NULL;
}

// Prefer the helper installed next to our own binary, fall back to $PATH
static void resolve_helper(char *buf, int n) {
	char self[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (len > 0) {
		self[len] = '\0';
		char *slash = strrchr(self, '/');
		if (slash != NULL) {
			slash[1] = '\0';
			snprintf(buf, n, "%smlinstall-card", self);
			if (access(buf, X_OK) == 0) return;
		}
	}

	snprintf(buf, n, "mlinstall-card");
}

// Run argv, capture stdout/stderr and log it. Returns the exit code,
// or -1 if the process couldn't be spawned
static int run_capture(char *const argv[]) {
	int pipes[2];
	if (pipe(pipes)) {
		log_print("pipe() failed: %s", strerror(errno));
		return -1;
	}

	pid_t pid = fork();
	if (pid < 0) {
		log_print("fork() failed: %s", strerror(errno));
		close(pipes[0]);
		close(pipes[1]);
		return -1;
	}

	if (pid == 0) {
		dup2(pipes[1], STDOUT_FILENO);
		dup2(pipes[1], STDERR_FILENO);
		close(pipes[0]);
		close(pipes[1]);
		execvp(argv[0], argv);

		// Only reached on failure
		fprintf(stderr, "Couldn't run %s: %s\n", argv[0], strerror(errno));
		_exit(127);
	}

	close(pipes[1]);

	char buffer[4096];
	size_t used = 0;
	ssize_t n;
	while ((n = read(pipes[0], buffer + used, sizeof(buffer) - 1 - used)) > 0) {
		used += n;
		if (used >= sizeof(buffer) - 1) {
			buffer[used] = '\0';
			log_print("%s", buffer);
			used = 0;
		}
	}
	close(pipes[0]);

	if (used > 0) {
		buffer[used] = '\0';
		log_print("%s", buffer);
	}

	int status = 0;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) return WEXITSTATUS(status);

	return -1;
}

int drive_write_flag(int mode) {
	char *mname = mode_name(mode);
	if (mname == NULL) return DRIVE_ERROR;

	// Find the card device (unprivileged udisks2 query)
	char dev[128];
	UDisksClient *client = client_new();
	if (client == NULL) return DRIVE_ERROR;

	UDisksObject *object = find_card(client);
	if (object == NULL) {
		log_print(T_DRIVE_NOT_FOUND);
		g_object_unref(client);
		return DRIVE_NONE;
	}

	snprintf(dev, sizeof(dev), "%s", udisks_block_get_device(udisks_object_peek_block(object)));
	g_object_unref(object);
	g_object_unref(client);

	char helper[PATH_MAX];
	resolve_helper(helper, sizeof(helper));

	char *argv[] = { helper, "write", dev, mname, NULL };
	int rc = run_capture(argv);

	if (rc == -DRIVE_PRIV) {
		// Helper couldn't open the device: retry through pkexec,
		// which will prompt for a password via the polkit agent
		log_print("Retrying with elevated permissions...");
		char *pargv[] = { "pkexec", helper, "write", dev, mname, NULL };
		rc = run_capture(pargv);

		if (rc == 126 || rc == 127) {
			// Auth canceled, pkexec missing, or it refuses to run
			// the helper from an untrusted (uninstalled) location
			log_print("Try: sudo %s write %s %s", helper, dev, mname);
			return DRIVE_PRIV;
		}
	}

	if (rc < 0 || rc > 4) {
		log_print("Failed to run mlinstall-card (exit %d).", rc);
		return DRIVE_ERROR;
	}

	switch (rc) {
	case 0:
		return 0;
	case -DRIVE_BADFS:
		return DRIVE_BADFS;
	case -DRIVE_NONE:
		return DRIVE_NONE;
	case -DRIVE_ERROR:
		return DRIVE_ERROR;
	default:
		log_print("Elevated permissions were refused.");
		log_print("Try: sudo %s write %s %s", helper, dev, mname);
		return DRIVE_PRIV;
	}
}