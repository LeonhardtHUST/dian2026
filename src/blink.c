#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <ws2812.h>

#define WS2812_PIN  48
#define LIGHT_NUM   2
#define delay_ms(ms) vTaskDelay(pdMS_TO_TICKS(ms) > 0 ? pdMS_TO_TICKS(ms) : 1)
 
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
    xTaskCreate(blink_task, "blink_task", 2048, NULL, 10, NULL);
}
