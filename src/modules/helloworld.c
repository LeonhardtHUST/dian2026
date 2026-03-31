#include <utils.h>
#include <usb_serial_io.h>
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"

static const char *TAG = "USB_APP";

#define BUF_SIZE 128

static TaskHandle_t tx_task_handle = NULL;
static TaskHandle_t rx_task_handle = NULL;

static void usb_tx_task(void *arg) {
    const char *msg = "Hello World\r\n";

    while (1) {
        // usb_serial_jtag_write_bytes((const void*)msg, strlen(msg), ticks_from_ms(100));
        usbio_print(100, msg);
        delay_ms(1000);
    }
}

static void usb_rx_task(void *arg) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RX buffer");
        vTaskDelete(NULL);
    }
    
    bool printed_secret = false;
    while (1) {
        int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE - 1), ticks_from_ms(100));
        // int len = usbio_print(data, 100);
        
        if (len > 0) {
            bool q_pressed = false;
            if (!printed_secret) {
                for (int i = 0; i < len; i++) {
                    if (data[i] == 'q' || data[i] == 'Q') {
                        q_pressed = true;
                        break;
                    }
                    if (data[i] == '\r' || data[i] == '\n') {
                        // usb_serial_jtag_write_bytes((const void*)"GEL37KXHDU9G\r\nFXLKNKWHVURC\r\nCE4K7KEYCUPQ\r\n", 42, 100 / portTICK_PERIOD_MS);
                        usbio_print_multi(100, 6, "GEL37KXHDU9G", endl, "FXLKNKWHVURC", endl, "CE4K7KEYCUPQ", endl);
                        printed_secret = true;
                        break;
                    }
                }
            } else {
                for (int i = 0; i < len; i++) {
                    if (data[i] == 'q' || data[i] == 'Q') {
                        q_pressed = true;
                        break;
                    }
                }
            }

            if (q_pressed) {
                free(data);
                if (tx_task_handle != NULL) {
                    vTaskDelete(tx_task_handle);
                    tx_task_handle = NULL;
                }
                rx_task_handle = NULL;
                vTaskDelete(NULL);
            }
        }
    }
}

void helloworld() {
    ESP_LOGI(TAG, "App using USB Serial JTAG");
    usbio_print(100, "Press Enter to show secret. Press 'q' or 'Q' to return to menu.\r\n");

    // 此处可略过如果前面在 prompt 中已初始化完毕，这里只是示范单独调用的形式
    // usbio_init(BUF_SIZE);
    
    xTaskCreate(usb_tx_task, "usb_tx_task", 2048, NULL, 10, &tx_task_handle);
    xTaskCreate(usb_rx_task, "usb_rx_task", 2048, NULL, 10, &rx_task_handle);

    while (rx_task_handle != NULL) {
        delay_ms(100);
    }
    usbio_print(100, "\r\nReturning to menu...\r\n");
}
