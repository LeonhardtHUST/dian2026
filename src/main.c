#include <ws2812.h>

extern void blink(void);
extern void ws2812(uint8_t);
extern void helloworld(void);
extern void helloworlduart(void);

#define WS2812_PURGE        0
#define WS2812_TASK         1

#define MODE_BLINK          0
#define MODE_WS2812         1
#define MODE_HELLOWORLD     2
#define MODE_HELLOWORLDUART 3

#define MODE_SELECTED   MODE_BLINK

void app_main() {
    ws2812(WS2812_PURGE);   // 关闭板载 WS2812 灯光

    switch (MODE_SELECTED) {
        case MODE_BLINK:
            blink();
            break;
        case MODE_HELLOWORLD:
            helloworld();
            break;
        case MODE_HELLOWORLDUART:
            helloworlduart();
            break;
        case MODE_WS2812:
            ws2812(WS2812_TASK);
            break;
    }
}