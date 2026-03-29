/* Created 19 Nov 2016 by Chris Osborn <fozztexx@fozztexx.com>
 * http://insentricity.com
 *
 * Uses the RMT peripheral on the ESP32 for very accurate timing of
 * signals sent to the WS2812 LEDs.
 *
 * This code is placed in the public domain (or CC0 licensed, at your option).
 */

#include "ws2812.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

#define RMT_RESOLUTION_HZ 40000000 // 40MHz

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

/**
 * @brief 在指定的GPIO引脚上初始化WS2812控制外设。
 *
 * @param gpioNum 连接到WS2812数据线的GPIO引脚号。
 */
void ws2812_init(int gpioNum)
{
    if (led_chan != NULL) {
        return; // Initialization already done
    }

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpioNum,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    
    // 40MHz意味着1个tick为25ns。
    // T0H: 350ns -> 14 ticks, T0L: 900ns -> 36 ticks
    // T1H: 900ns -> 36 ticks, T1L: 350ns -> 14 ticks
    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = { .duration0 = 14, .level0 = 1, .duration1 = 36, .level1 = 0 },
        .bit1 = { .duration0 = 36, .level0 = 1, .duration1 = 14, .level1 = 0 },
        .flags = {
            .msb_first = 1,
        }
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

/**
 * @brief 设置WS2812灯带的颜色。
 *
 * @param length 灯带上LED的数量。
 * @param array 包含每个LED颜色值的数组指针。
 */
void ws2812_setColors(uint8_t length, rgbVal *array)
{
    if (led_chan == NULL) {
        return;
    }

    size_t buffer_size = length * 3;
    uint8_t *buffer = (uint8_t *)malloc(buffer_size);
    if (!buffer) return;

    for (uint8_t i = 0; i < length; i++) {
        buffer[i * 3 + 0] = array[i].g; // WS2812期望GRB顺序
        buffer[i * 3 + 1] = array[i].r;
        buffer[i * 3 + 2] = array[i].b;
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, buffer, buffer_size, &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    free(buffer);
}

void ws2812_purge(uint8_t length)
{
    rgbVal off = makeRGBVal(0, 0, 0);
    ws2812_setColors(length, &off); 
}