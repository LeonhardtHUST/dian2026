#include <utils.h>
#include <ws2812.h>
#include <usb_serial_io.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t blink_task_handle = NULL;

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

static void blink_task(void *arg) {
    uint8_t led_state = 0;
    rgbVal rgb;

    while (1) {
        rgb = makeRGBVal(255 * led_state, 255 * led_state, 255 * led_state);
		ws2812_setColors(LIGHT_NUM, &rgb);
        
        led_state = !led_state;

        delay_ms(1000);
    }
}

void blink() {
    nvs_flash_init();
	ws2812_init(WS2812_PIN);
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 10, &blink_task_handle);

    wait_for_enter();
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
    }
    ws2812_purge(LIGHT_NUM);
}
