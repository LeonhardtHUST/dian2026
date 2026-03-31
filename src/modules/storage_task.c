#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "usb_serial_io.h"

#define MOUNT_POINT "/fatfs"
static const char *TAG = "storage_task";

static void vfs_wait_for_input(char *buf, size_t max_len) {
    size_t pos = 0;
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    if (pos > 0) {
                        buf[pos] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    } else if (c == '\r') {
                        buf[0] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    }
                } else if (c == '\b' || c == 127) {
                    if (pos > 0) {
                        pos--;
                        usbio_print(10, "\b \b");
                    }
                } else {
                    if (pos < max_len - 1) {
                        buf[pos++] = c;
                        char echo[2] = {c, '\0'};
                        usbio_print(10, echo);
                    }
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void print_partition_info(void) {
    uint64_t total = 0, free_bytes = 0;
    esp_err_t ret = esp_vfs_fat_info(MOUNT_POINT, &total, &free_bytes);
    if (ret == ESP_OK) {
        char buf[128];
        snprintf(buf, sizeof(buf), "\r\nPartition Info:\r\nTotal Drive Space: %llu KB\r\nFree Space: %llu KB\r\n", 
            total / 1024, free_bytes / 1024);
        usbio_print(100, buf);
    } else {
        usbio_print(100, "\r\nFailed to get partition info.\r\n");
    }
}

extern esp_err_t storage_format(void);

static void format_partition(void) {
    usbio_print(100, "\r\nAre you sure you want to format the partition? Type 'yes' to confirm: ");
    char input[16] = {0};
    vfs_wait_for_input(input, sizeof(input));
    
    // Check case insensitive 'yes'
    if (strcasecmp(input, "yes") == 0) {
        usbio_print(100, "\r\nFormatting... Please wait.\r\n");
        esp_err_t res = storage_format();
        if (res == ESP_OK) {
            usbio_print(100, "Format successful.\r\n");
        } else {
            usbio_print(100, "Format failed.\r\n");
        }
    } else {
        usbio_print(100, "\r\nFormat cancelled.\r\n");
    }
}

static void manage_file(const char *filename) {
    char buf[256];
    snprintf(buf, sizeof(buf), "\r\nFile: %s\r\nOptions: [1] Delete [2] Rename [0] Back\r\nSelect: ", filename);
    usbio_print(100, buf);
    
    char input[16] = {0};
    vfs_wait_for_input(input, sizeof(input));
    
    char filepath[300];
    snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);
    
    if (strcmp(input, "1") == 0) {
        if (unlink(filepath) == 0) {
            usbio_print(100, "\r\nFile deleted successfully.\r\n");
        } else {
            usbio_print(100, "\r\nFailed to delete file.\r\n");
        }
    } else if (strcmp(input, "2") == 0) {
        usbio_print(100, "\r\nEnter new name: ");
        char newname[256] = {0};
        vfs_wait_for_input(newname, sizeof(newname));
        
        for (int i = 0; newname[i]; i++) {
            newname[i] = toupper((unsigned char)newname[i]);
        }
        
        char newpath[300];
        snprintf(newpath, sizeof(newpath), "%s/%s", MOUNT_POINT, newname);
        if (rename(filepath, newpath) == 0) {
            usbio_print(100, "\r\nFile renamed successfully.\r\n");
        } else {
            usbio_print(100, "\r\nFailed to rename file.\r\n");
        }
    }
}

static void list_and_manage_files(void) {
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir) {
        usbio_print(100, "\r\nFailed to open directory.\r\n");
        return;
    }

    struct dirent *entry;
    char filenames[50][256];
    int count = 0;

    usbio_print(100, "\r\nFiles:\r\n");
    while ((entry = readdir(dir)) != NULL && count < 50) {
        if (entry->d_type == DT_REG) {
            strncpy(filenames[count], entry->d_name, sizeof(filenames[count]) - 1);
            filenames[count][sizeof(filenames[count]) - 1] = '\0';
            
            char buf[300];
            snprintf(buf, sizeof(buf), " [%d] %s\r\n", count + 1, filenames[count]);
            usbio_print(100, buf);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        usbio_print(100, " No files found.\r\n");
        return;
    }

    usbio_print(100, "\r\nEnter file number to manage (or 0 to exit): ");
    char input[16] = {0};
    vfs_wait_for_input(input, sizeof(input));
    
    int sel = atoi(input);
    if (sel > 0 && sel <= count) {
        manage_file(filenames[sel - 1]);
    }
}

void storage_task_run(void) {
    bool running = true;
    while (running) {
        usbio_print_multi(100, 5,
            "\r\n--- Storage Management ---\r\n",
            " [1] Partition Info\r\n",
            " [2] List & Manage Files\r\n",
            " [3] Format Partition\r\n",
            " [0] Exit\r\n",
            "Select an option: "
        );

        char input[16] = {0};
        vfs_wait_for_input(input, sizeof(input));

        if (strcmp(input, "1") == 0) {
            print_partition_info();
        } else if (strcmp(input, "2") == 0) {
            list_and_manage_files();
        } else if (strcmp(input, "3") == 0) {
            format_partition();
        } else if (strcmp(input, "0") == 0) {
            running = false;
        } else {
            usbio_print(100, "\r\nInvalid option.\r\n");
        }
    }
}