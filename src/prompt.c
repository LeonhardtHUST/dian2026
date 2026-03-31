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
extern void network_status_run(void);
extern void storage_task_run(void);
extern void ftp_server_run(void);

// 定义各个功能的启用/禁用状态宏 (1=启用, 0=禁用)
#define ENABLE_TASK_0_BLINK           1
#define ENABLE_TASK_1_WS2812          1
#define ENABLE_TASK_2_HELLOWORLD_USB  1
#define ENABLE_TASK_3_HELLOWORLD_UART 0
#define ENABLE_TASK_4_WIFI_CONNECT    1
#define ENABLE_TASK_5_NETWORK_STATUS  1
#define ENABLE_TASK_6_STORAGE         1
#define ENABLE_TASK_7_FTP_SERVER      1

#define WS2812_TASK         1

static void wait_for_input(char *buf, size_t max_len) {
    size_t pos = 0;
    while (1) {
        usbio_rx_data_t rx = usbio_read(100);
        if (rx.length > 0) {
            char *d = (char *)rx.data;
            for (int i = 0; i < rx.length; i++) {
                char c = d[i];
                if (c == '\r' || c == '\n') {
                    if (pos > 0) {
                        buf[pos] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    } else if (c == '\r') {
                        buf[0] = '\0';
                        usbio_rx_data_destruct(&rx);
                        return;
                    }
                } else if (c == '\b' || c == 127) { // backspace
                    if (pos > 0) {
                        pos--;
                        usbio_print(10, "\b \b");
                    }
                } else {
                    if (pos < max_len - 1) {
                        buf[pos++] = c;
                        char echo[2] = {c, '\0'};
                        usbio_print(10, echo);
                    }
                }
            }
        }
        usbio_rx_data_destruct(&rx);
        delay_ms(10);
    }
}

void app_prompt(void) {
    static bool is_init = false;
    if (!is_init) {
        // 1. 初始化 USB Serial JTAG 驱动并分配缓冲
        usbio_init(128);
        is_init = true;
    }

    // 2. 循环输出 "Press enter to continue...\r\n"
    static bool started = false;
    while (!started) {
        usbio_print(100, "Press enter to continue...\r\n");
        
        
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
    usbio_print_multi(100, 13,
        "========================================\r\n",
        "          Welcome to Dian2026!          \r\n",
        "========================================\r\n",
        " [0] Blink\r\n",
        " [1] WS2812\r\n",
        " [2] Hello World (USB)\r\n",
        " [3] Hello World (UART)\r\n",
        " [4] WiFi Connect\r\n",
        " [5] Network Status\r\n",
        " [6] Storage Management\r\n",
        " [7] FTP Server\r\n",
        "========================================\r\n",
        "Please select a task: "
    );

    int selected_task = -1;
    char input_buf[16] = {0};
    
    while (selected_task == -1) {
        wait_for_input(input_buf, sizeof(input_buf));
        
        bool is_valid = true;
        size_t len = strlen(input_buf);
        for(int i=0; i<len; i++) {
            if(input_buf[i] < '0' || input_buf[i] > '9') {
                is_valid = false;
                break;
            }
        }
        
        if(is_valid && len > 0) {
            int num = 0;
            for(int i=0; i<len; i++) {
                num = num * 10 + (input_buf[i] - '0');
            }
            if(num >= 0 && num <= 7) {
                selected_task = num;
            } else {
                usbio_print(100, "\r\nInvalid task number. Try again: ");
            }
        } else {
            usbio_print(100, "\r\nInvalid input. Try again: ");
        }
    }

    // 打印用户的选择
    char sel_str[32];
    snprintf(sel_str, sizeof(sel_str), "\r\nSelected task: %d\r\n", selected_task);
    usbio_print(100, sel_str);

    // 检查并执行对应的任务
    bool task_enabled = false;
    switch (selected_task) {
        case 0:
#if ENABLE_TASK_0_BLINK
            task_enabled = true;
            blink();
#endif
            break;
        case 1:
#if ENABLE_TASK_1_WS2812
            task_enabled = true;
            ws2812(WS2812_TASK);
#endif
            break;
        case 2:
#if ENABLE_TASK_2_HELLOWORLD_USB
            task_enabled = true;
            helloworld();
#endif
            break;
        case 3:
#if ENABLE_TASK_3_HELLOWORLD_UART
            task_enabled = true;
            helloworlduart();
#endif
            break;
        case 4:
#if ENABLE_TASK_4_WIFI_CONNECT
            task_enabled = true;
            wifi_connect_run();
#endif
            break;
        case 5:
#if ENABLE_TASK_5_NETWORK_STATUS
            task_enabled = true;
            network_status_run();
#endif
            break;
                case 6:
#if ENABLE_TASK_6_STORAGE
            task_enabled = true;
            storage_task_run();
#endif
            break;
        case 7:
#if ENABLE_TASK_7_FTP_SERVER
            task_enabled = true;
            ftp_server_run();
#endif
            break;
    }

    if (!task_enabled) {
        usbio_print(100, "This component is disabled. Press enter to return...\r\n");
        char dummy[2];
        wait_for_input(dummy, sizeof(dummy));
    }
}
