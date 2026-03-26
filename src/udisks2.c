#include <udisks/udisks.h>
#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to format size in human-readable form
static const char *format_size(guint64 size) {
    static char buffer[32];
    if (size >= 1024ULL * 1024 * 1024 * 1024) { // TB
        snprintf(buffer, sizeof(buffer), "%.2f TB", size / (1024.0 * 1024 * 1024 * 1024));
    } else if (size >= 1024ULL * 1024 * 1024) { // GB
        snprintf(buffer, sizeof(buffer), "%.2f GB", size / (1024.0 * 1024 * 1024));
    } else if (size >= 1024ULL * 1024) { // MB
        snprintf(buffer, sizeof(buffer), "%.2f MB", size / (1024.0 * 1024));
    } else if (size >= 1024ULL) { // KB
        snprintf(buffer, sizeof(buffer), "%.2f KB", size / 1024.0);
    } else {
        snprintf(buffer, sizeof(buffer), "0.00 KB");
    }
    return buffer;
}

void print_disk_info(UDisksClient *client) {
    GList *objects, *l;

    // Get all managed objects from UDisks2
    objects = g_dbus_object_manager_get_objects(udisks_client_get_object_manager(client));
    if (!objects) {
        fprintf(stderr, "No objects found. Is UDisks2 running?\n");
        return;
    }

    printf("Found %d objects:\n", g_list_length(objects));

    for (l = objects; l != NULL; l = l->next) {
        UDisksObject *object = UDISKS_OBJECT(l->data);
        UDisksBlock *block = udisks_object_peek_block(object);
        UDisksDrive *drive = NULL;

        // Check if the object is a block device
        if (block != NULL) {
            // Skip if this is a partition
            UDisksPartition *partition = udisks_object_peek_partition(object);
            if (partition != NULL) {
                continue; // Skip partitions
            }

            const gchar *device = udisks_block_get_device(block);
            const gchar *drive_path = udisks_block_get_drive(block);
            guint64 size = udisks_block_get_size(block);

            // Skip devices with zero capacity
            if (size == 0) {
                continue; // Skip fake/invalid entries
            }

            // Try to get the associated drive
            if (drive_path && g_strcmp0(drive_path, "/") != 0) {
                UDisksObject *drive_object = udisks_client_get_object(client, drive_path);
                if (drive_object) {
                    drive = udisks_object_peek_drive(drive_object);
                }

                if (drive) {
                    const gchar *connection_bus = udisks_drive_get_connection_bus(drive);

                    // Filter for USB, MMC, or SDHCI (sdio)
                    if (connection_bus && (g_strcmp0(connection_bus, "usb") == 0 || 
                                          g_strcmp0(connection_bus, "sdio") == 0)) {
                        const gchar *drive_model = udisks_drive_get_model(drive);
                        const gchar *drive_vendor = udisks_drive_get_vendor(drive);

                        printf("Device: %s\n", device ? device : "Unknown");
                        printf("  Vendor: %s\n", drive_vendor && *drive_vendor ? drive_vendor : "Unknown");
                        printf("  Model: %s\n", drive_model && *drive_model ? drive_model : "Unknown");
                        printf("  Connection: %s\n", connection_bus);
                        printf("  Capacity: %s\n", format_size(size));
                        printf("\n");
                    }

                    g_object_unref(drive_object);
                }
            }
        }
    }

    g_list_free_full(objects, g_object_unref);
}

int main(void) {
    UDisksClient *client = NULL;
    GError *error = NULL;

    // Initialize UDisks client
    client = udisks_client_new_sync(NULL, &error);
    if (client == NULL) {
        fprintf(stderr, "Error creating UDisks client: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    // Print disk information for USB/MMC/SDHCI
    print_disk_info(client);

    // Cleanup
    g_object_unref(client);
    return 0;
}
