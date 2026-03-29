#include <utils.h>
#include <ws2812.h>
 
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
