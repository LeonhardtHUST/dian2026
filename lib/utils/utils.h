#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define WS2812_PIN  48
#define LIGHT_NUM   1

// 使用 pdMS_TO_TICKS 保证在不足1个tick时至少延时1个tick，避免死循环
#define ticks_from_ms(ms) (pdMS_TO_TICKS(ms) > 0 ? pdMS_TO_TICKS(ms) : 1)
#define delay_ms(ms) vTaskDelay(ticks_from_ms(ms))

#endif // UTILS_H