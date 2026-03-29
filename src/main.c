// 声明外部定义的函数
extern int led(void);
extern void helloworld(void);
extern void helloworlduart(void);

#define MODE_HELLOWORLD     0
#define MODE_HELLOWORLDUART 1
#define MODE_LED            2

#define MODE_SELECTED   MODE_HELLOWORLDUART

void app_main() {
    switch (MODE_SELECTED) {
        case MODE_HELLOWORLD:
            helloworld();
            break;
        case MODE_HELLOWORLDUART:
            helloworlduart();
            break;
        case MODE_LED:
            led();
            break;
    }
}