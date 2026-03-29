/* Created 19 Nov 2016 by Chris Osborn <fozztexx@fozztexx.com>
 * http://insentricity.com
 *
 * Demo of driving WS2812 RGB LEDs using the RMT peripheral.
 *
 * This code is placed in the public domain (or CC0 licensed, at your option).
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>
#include "ws2812.h"

#define WS2812_PIN	18

#define delay_ms(ms) vTaskDelay((ms) / portTICK_RATE_MS)

// 主任务：运行彩虹流水灯效果
// 作为一个FreeRTOS任务，它接收一个参数pvParameters，必须是一个死循环
void rainbow(void *pvParameters)
{
  const uint8_t anim_step = 10;   // 每次颜色变化的步进值
  const uint8_t anim_max = 250;   // 这是为了防止溢出的亮度最大值
  const uint8_t pixel_count = 64; // 你串联的LED灯珠总数
  const uint8_t delay = 25;       // 颜色变化之间的延时（毫秒）
  
  rgbVal color = makeRGBVal(anim_max, 0, 0); // 初始颜色：红色
  uint8_t step = 0;
  rgbVal color2 = makeRGBVal(anim_max, 0, 0);
  uint8_t step2 = 0;
  rgbVal *pixels; // 动态分配的数组，用来存所有灯珠的颜色

  // 在堆内存分配用于保存所有LED颜色数据的数组（避免占用太多栈空间）
  pixels = malloc(sizeof(rgbVal) * pixel_count);

  while (1) {
    color = color2;
    step = step2;

    // 遍历所有LED，计算当前这一帧每一个LED的颜色
    for (uint8_t i = 0; i < pixel_count; i++) {
      pixels[i] = color;

      if (i == 1) {
        color2 = color;
        step2 = step;
      }

      // 这个状态机负责颜色渐变: 红->黄->绿->青->蓝->紫->红
      switch (step) {
      case 0: // 加绿，向黄色过渡
        color.g += anim_step;
        if (color.g >= anim_max)
          step++;
        break;
      case 1: // 减红，向绿色过渡
        color.r -= anim_step;
        if (color.r == 0)
          step++;
        break;
      case 2: // 加蓝，向青色过渡
        color.b += anim_step;
        if (color.b >= anim_max)
          step++;
        break;
      case 3: // 减绿，向蓝色过渡
        color.g -= anim_step;
        if (color.g == 0)
          step++;
        break;
      case 4: // 加红，向紫色过渡
        color.r += anim_step;
        if (color.r >= anim_max)
          step++;
        break;
      case 5: // 减蓝，回到红色
        color.b -= anim_step;
        if (color.b == 0)
          step = 0;
        break;
      }
    }

    // 将计算好的颜色数组发送给硬件上的LED
    ws2812_setColors(pixel_count, pixels);

    // 暂停一段时间，让出CPU给其他任务，也是控制动画刷新率
    delay_ms(delay);
  }
}

void app_main()
{
  // 1. 初始化NVS(非易失性存储)
  // 这是大多数ESP32程序推荐的第一步，很多系统级库(如Wi-Fi)会依赖它
  nvs_flash_init();

  // 2. 初始化WS2812驱动，配置绑定的GPIO引脚 (默认18)
  ws2812_init(WS2812_PIN);
  
  // 3. 创建一个FreeRTOS任务来运行我们的灯带动画
  // ESP32基于FreeRTOS操作系统，使用多任务来让程序同时做多件事。
  // 参数: 任务函数名, 任务名, 栈大小(字节), 传递参数, 优先级, 任务句柄
  xTaskCreate(rainbow, "ws2812 rainbow demo", 4096, NULL, 10, NULL);

  return;
}
