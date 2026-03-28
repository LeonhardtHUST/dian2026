#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"

static const char *TAG = "USB_APP";

#define BUF_SIZE 128

static void usb_tx_task(void *arg) {
    const char *msg = "Hello World\r\n";

    while (1) {
        usb_serial_jtag_write_bytes((const void*)msg, strlen(msg), 100 / portTICK_PERIOD_MS);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void usb_rx_task(void *arg) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RX buffer");
        vTaskDelete(NULL);
    }
    
    while (1) {
        int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (data[i] == '\r' || data[i] == '\n') {
                    usb_serial_jtag_write_bytes((const void*)"GEL37KXHDU9G\r\nFXLKNKWHVURC\r\nCE4K7KEYCUPQ\r\n", 42, 100 / portTICK_PERIOD_MS);
                    break;
                }
            }
        }
    }
}

void app_main() {
    ESP_LOGI(TAG, "App using USB Serial JTAG");

    usb_serial_jtag_driver_config_t usb_config = {
        .rx_buffer_size = BUF_SIZE * 2,
        .tx_buffer_size = BUF_SIZE * 2,
    };
    
    usb_serial_jtag_driver_install(&usb_config);
    
    xTaskCreate(usb_tx_task, "usb_tx_task", 2048, NULL, 10, NULL);
    xTaskCreate(usb_rx_task, "usb_rx_task", 2048, NULL, 10, NULL);
}
