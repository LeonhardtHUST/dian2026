#include <utils.h>
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include <stdarg.h>
#include <stdbool.h>

static const char *DEFAULT_TAG = "USB_APP";
#define DEFAULT_BUF_SIZE 128
#define DEFAULT_PRINT_WAIT_MS 100

const char *endl = "\r\n";
static uint8_t buf_size = DEFAULT_BUF_SIZE;

/**
 * @brief 获取当前的 USB 的输入输出缓冲区大小。
 * 
 * @return uint8_t 当前的缓冲区大小。
 */
uint8_t usb_io_get_buffer_size() {
    return buf_size;
}

/**
 * @brief 设置 USB 的输入输出缓冲区大小。
 * 
 * @param n 新的缓冲区大小。
 */
void usb_io_set_buffer_size(const uint8_t n) {
    buf_size = n;
}

/**
 * @brief 通过 USB Serial JTAG 打印字符串。
 * 
 * @param str 要打印的字符串。
 * @param ms_to_wait 最大的阻塞等待时间（毫秒）。
 * @return int 成功发送的字节数，或在发生错误时返回负值。
 */
int usb_print(const char* str, const uint32_t ms_to_wait) {
    return usb_serial_jtag_write_bytes((const void*)str, strlen(str), ticks_from_ms(ms_to_wait));
}

/**
 * @brief 通过 USB Serial JTAG 连续打印多个字符串。
 * 
 * @param num 传入的字符串数量。
 * @param str 第一个字符串，后续参数为其它字符串 (const char*)。
 * @return int 如果所有打印都未报错，返回成功打印的最大字节数；如果某次打印报错，返回第一次报错时的负值。
 */
int usb_print_multi(const uint8_t num, const char* str, ...) {
    if (num == 0) return 0;

    int first_neg = 0;
    bool has_neg = false;

    int current_max = usb_print(str, DEFAULT_PRINT_WAIT_MS);
    if (current_max < 0 && !has_neg) {
        first_neg = current_max;
        has_neg = true;
    }

    va_list args;
    va_start(args, str);
    for (uint8_t i = 1; i < num; i++) {
        const char* next_str = va_arg(args, const char*);
        int res = usb_print(next_str, DEFAULT_PRINT_WAIT_MS);
        
        if (res < 0 && !has_neg) {
            first_neg = res;
            has_neg = true;
        }
        
        if (!has_neg) {
            current_max = max(2, current_max, res);
        }
    }
    va_end(args);

    if (has_neg) {
        return first_neg;
    }
    return current_max;
}