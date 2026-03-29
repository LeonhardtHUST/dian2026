/* 
 * 这是一个为ESP32平台编写的WS2812 RGB LED驱动程序。
 * 使用了ESP32硬件独有的RMT（远程控制）外设来实现纳秒级的精确时序。
 * 对于初学者：WS2812对时间要求非常苛刻，一般的软件延时很难满足，
 * 因此使用硬件(RMT)来发送数据是ESP32的标准做法。
 */

#ifndef WS2812_DRIVER_H
#define WS2812_DRIVER_H

#include <stdint.h>

// RGB颜色联合体，方便我们设置颜色
// 既可以按照R, G, B单独赋值，也可以当成一个32位整数直接操作
typedef union {
  struct __attribute__ ((packed)) {
    uint8_t r, g, b; // 红色(Red)、绿色(Green)、蓝色(Blue)通道(0-255)
  };
  uint32_t num; // 作为整体读取/写入
} rgbVal;

// 初始化WS2812，指定驱动的GPIO引脚
extern void ws2812_init(int gpioNum);

// 发送颜色数据到灯带
// length: 灯带上LED的数量
// array: 存放每个LED颜色数据的数组首地址
extern void ws2812_setColors(uint8_t length, rgbVal *array);

// 关闭WS2812灯光
extern void ws2812_purge(uint8_t length);

// 辅助函数：根据r, g, b值快速生成一个rgbVal结构体
static inline rgbVal makeRGBVal(uint8_t r, uint8_t g, uint8_t b)
{
  rgbVal v;

  v.r = r;
  v.g = g;
  v.b = b;
  return v;
}

#endif /* WS2812_DRIVER_H */
