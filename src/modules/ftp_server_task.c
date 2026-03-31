#include <utils.h>
#include <usb_serial_io.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "storage.h"
#include "ftp.h"
#include "esp_wifi.h"
#include "esp_log.h"

// Define these because we removed Kconfig
#define CONFIG_FTP_USER "test"
#define CONFIG_FTP_PASSWORD "test"

EventGroupHandle_t xEventTask = NULL;
int FTP_TASK_FINISH_BIT = BIT2;

static TaskHandle_t ftp_server_task_handle = NULL;

static void wait_for_enter(void) {
    usbio_print(100, "Press enter to return...\r\n");
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    usbio_rx_data_destruct(&rx);
                    return;
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        delay_ms(10);
    }
}

// Wrapper to launch the ftp server task and monitor for Enter key
void ftp_server_run(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        usbio_print(100, "Error: No WiFi connection. Please connect to WiFi first.\r\n");
        wait_for_enter();
        return;
    }

    usbio_print(100, "Starting Storage...\r\n");
    storage_init(); // Ensure VFS is mounted to /fatfs

    usbio_print(100, "Starting FTP Server...\r\n");
    usbio_print(100, "FTP User: test, Password: test\r\n");

    // We can call ftp_init() / ftp_enable() if we look at ftp.h
    // But ftp.c actually defines ftp_task which we might need to launch.
    // Wait, ftp_task is already in ftp.c. Let's look at its signature.
    // void ftp_task (void *pvParameters)

    extern void ftp_task(void *pvParameters);
    
    if (xEventTask == NULL) {
        xEventTask = xEventGroupCreate();
    }

    xTaskCreate(ftp_task, "ftp_task", 4096, NULL, 5, &ftp_server_task_handle);

    wait_for_enter();

    // When enter is pressed, we should stop it quietly.
    usbio_print(100, "Stopping FTP Server...\r\n");
    
    ftp_terminate();

    // Wait for task to finish gracefully
    if (xEventTask != NULL) {
        xEventGroupWaitBits(xEventTask, FTP_TASK_FINISH_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
    }

    ftp_server_task_handle = NULL;

    usbio_print(100, "FTP Server returned to menu.\r\n");
}