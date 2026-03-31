#include <utils.h>
#include <usb_serial_io.h>
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include <string.h>

extern void blink(void);
extern void ws2812(uint8_t);
extern void helloworld(void);
extern void helloworlduart(void);
extern void wifi_connect_run(void);

#define WS2812_TASK         1

void app_prompt(void) {
    static bool is_init = false;
    if (!is_init) {
        // 1. 初始化 USB Serial JTAG 驱动并分配缓冲
        usbio_init(128);
        is_init = true;
    }

    // 2. 循环输出 "Press any key to continue...\r\n"
    bool started = false;
    while (!started) {
        usbio_print(100, "Press any key to continue...\r\n");
        
        // 尝试等待读取，相当于延时 + 检查输入
        // 每次循环等待约 1 秒，将 1 秒分成多次读取以提高响应速度
        for (int j = 0; j < 10; j++) {
            usbio_rx_data_t rx = usbio_read(100);
            if (rx.length > 0) {
                char *data = (char *)rx.data;
                for (int i = 0; i < rx.length; i++) {
                    if (data[i] == '\r' || data[i] == '\n') {
                        started = true;
                        break;
                    }
                }
            }
            usbio_rx_data_destruct(&rx);
            if (started) {
                break;
            }
        }
    }

    // 3. 打印欢迎界面和任务列表
    usbio_print_multi(100, 10,
        "========================================\r\n",
        "          Welcome to Dian2026!          \r\n",
        "========================================\r\n",
        " [0] Blink\r\n",
        " [1] WS2812\r\n",
        " [2] Hello World (USB)\r\n",
        " [3] Hello World (UART)\r\n",
        " [4] WiFi Connect\r\n",
        "========================================\r\n",
        "Please select a task: "
    );

    int selected_task = -1;
    while (selected_task == -1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *data = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = data[i];
                if (c >= '0' && c <= '4') {
                    selected_task = c - '0';
                    break;
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        if (selected_task == -1) {
            delay_ms(100);
        }
    }

    // 打印用户的选择
    usbio_print_multi(100, 2, "\r\nSelected task: ", 
        (selected_task == 0 ? "0\r\n" : selected_task == 1 ? "1\r\n" : selected_task == 2 ? "2\r\n" : selected_task == 3 ? "3\r\n" : "4\r\n")
    );

    // 4. 执行对应的任务
    switch (selected_task) {
        case 0:
            blink();
            break;
        case 1:
            ws2812(WS2812_TASK);
            break;
        case 2:
            helloworld();
            break;
        case 3:
            helloworlduart();
            break;
        case 4:
            wifi_connect_run();
            // Optional: run prompt again or restart chip for menu?
            // Actually app_prompt() returns, then main loop can re-call it.
            break;
    }
}
