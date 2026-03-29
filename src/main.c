// 声明外部定义的函数
extern void helloworld(void);
extern void helloworlduart(void);
extern void ws2812(void);

#define MODE_HELLOWORLD     0
#define MODE_HELLOWORLDUART 1
#define MODE_WS2812         2

#define MODE_SELECTED   MODE_WS2812

void app_main() {
    switch (MODE_SELECTED) {
        case MODE_HELLOWORLD:
            helloworld();
            break;
        case MODE_HELLOWORLDUART:
            helloworlduart();
            break;
        case MODE_WS2812:
            ws2812();
            break;
    }
}