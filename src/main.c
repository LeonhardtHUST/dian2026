#include <utils.h>

extern void app_prompt(void);
extern void ws2812(uint8_t);

#define WS2812_PURGE        0

void app_main() {
    ws2812(WS2812_PURGE);   // 关闭板载 WS2812 灯光

    app_prompt();
}
