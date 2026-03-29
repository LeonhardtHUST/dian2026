#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "math.h"

#include <ws2812.h>

// 使用 pdMS_TO_TICKS 保证在不足1个tick时至少延时1个tick，避免死循环
#define delay_ms(ms) vTaskDelay(pdMS_TO_TICKS(ms) > 0 ? pdMS_TO_TICKS(ms) : 1)
#define WS2812_PIN 	48
#define LIGHT_NUM	1

static void light(void *arg) {
	/*
		修改说明：
		1. 之前的 OMEGA = 2000*M_PI。在做 OMEGA * t 时，如果t是整数，结果永远是 2π 的整数倍，
		   导致 cos 的值恒定（始终等于1），因此颜色不会改变。
		2. 初相需要均匀分布在 2π 的空间内 (0, 120°, 240°)，而旧代码是 (0, 180°, 360°)，
		   导致红色和蓝色完全同相。
		3. delay_ms(1) 传入 1ms 时，因为100Hz的Tick下整数除法 1/10=0，可能造成看门狗超时或霸占CPU。
	*/
	uint8_t t = 0;
	double r, g, b;
	const double A = 255.0 / 2.0, C = A;
	
	// t 是从 0 增加到 255 再溢出回 0。我们让 256 次为 1 个完整的颜色循环。
	const double OMEGA = 2.0 * M_PI / 256.0;
	rgbVal rgb;
	
	// 为构成色轮，R、G、B相位应该错开 120 度（也就是 2π/3）
	const double PHI_R = 0, 
		PHI_G = 2.0 * M_PI / 3.0, 
		PHI_B = 4.0 * M_PI / 3.0;

	while (1) {
		r = A * cos(OMEGA * t + PHI_R) + C;
		g = A * cos(OMEGA * t + PHI_G) + C;
		b = A * cos(OMEGA * t + PHI_B) + C;
		rgb = makeRGBVal((uint8_t) floor(r), (uint8_t) floor(g), (uint8_t) floor(b));
		ws2812_setColors(LIGHT_NUM, &rgb);
		t++;
		
		// 每次延时10ms，256次循环耗时大约 2.56s，形成一个平滑的呼吸彩虹变色
		delay_ms(10);
	}
}

void ws2812() {
	nvs_flash_init();
	ws2812_init(WS2812_PIN);
	xTaskCreate(light, "ws2812 rainbow demo", 4096, NULL, 10, NULL);
}