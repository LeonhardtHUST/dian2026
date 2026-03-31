#include <utils.h>
#include "usb_serial_io.h"
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
 * @brief 初始化并设置 USB 的输入输出缓冲区大小。
 * 
 * @param n 新的缓冲区大小，物理驱动将会分配 2 倍大小的缓冲。
 */
void usb_io_init(const uint8_t n) {
    buf_size = n;
    usb_serial_jtag_driver_config_t usb_config = {
        .rx_buffer_size = buf_size * 2,
        .tx_buffer_size = buf_size * 2,
    };
    usb_serial_jtag_driver_install(&usb_config);
}

/**
 * @brief 通过 USB Serial JTAG 打印字符串。
 * 
 * @param ms_to_wait 最大的阻塞等待时间（毫秒）。
 * @param str 要打印的字符串。
 * @return int 成功发送的字节数，或在发生错误时返回负值。
 */
int usb_print(const uint32_t ms_to_wait, const char* str) {
    return usb_serial_jtag_write_bytes((const void*)str, strlen(str), ticks_from_ms(ms_to_wait));
}

/**
 * @brief 通过 USB Serial JTAG 连续打印多个字符串。
 * 
 * @param num 传入的字符串数量。
 * @param str 第一个字符串，后续参数为其它字符串 (const char*)。
 * @return int 如果所有打印都未报错，返回成功打印的最大字节数；如果某次打印报错，返回第一次报错时的负值。
 */
int usb_print_multi(const uint32_t ms_to_wait, const uint8_t num, const char* str, ...) {
    if (num == 0) return 0;

    int first_neg = 0;
    bool has_neg = false;

    int current_max = usb_print(ms_to_wait, str);
    if (current_max < 0 && !has_neg) {
        first_neg = current_max;
        has_neg = true;
    }

    va_list args;
    va_start(args, str);
    for (uint8_t i = 1; i < num; i++) {
        const char* next_str = va_arg(args, const char*);
        int res = usb_print(ms_to_wait, next_str);
        
        if (res < 0 && !has_neg) {
            first_neg = res;
            has_neg = true;
        }
        
        if (!has_neg) {
            current_max = max_list(2, current_max, res);
        }
    }
    va_end(args);

    if (has_neg) {
        return first_neg;
    }
    return current_max;
}

/**
 * @brief 从 USB Serial JTAG 读取数据。
 * 
 * @param ms_to_wait 最大的阻塞等待时间（毫秒）。
 * @return USB_RX_Data 包含读取到的字节数和数据指针。如果不为 0，数据指针指向通过 malloc() 分配的内存，调用者处理完毕后【必须】使用 free((void*)data) 手动释放内存避免内存泄漏！
 */
USB_RX_Data usb_read(const uint32_t ms_to_wait) {
    uint8_t *data = (uint8_t *)malloc(buf_size);
    if (data == NULL) {
        ESP_LOGE(DEFAULT_TAG, "Failed to allocate RX buffer");
        USB_RX_Data ret = { -1, NULL };
        return ret;
    }

    int len = usb_serial_jtag_read_bytes(data, (buf_size - 1), ticks_from_ms(ms_to_wait));
    
    if (len <= 0) {
        // 如果没有读取到数据或者出错，释放刚刚分配的内存，防止内存泄漏
        free(data);
        USB_RX_Data ret = { len, NULL };
        return ret;
    }

    // 可选：为了方便打印可以添加一个字符串结束符（取决于具体需求，已预留了buf_size-1的空位）
    data[len] = '\0';

    USB_RX_Data ret = { len, data };
    return ret;
}

/**
 * @brief 释放由 usb_read 函数分配的 USB 接收数据内存。
 * 
 * @param rx_data 指向包含待释放内存指针的结构体的指针，释放后结构体数据会被清空。
 */
void usb_rx_data_destruct(USB_RX_Data* rx_data) {
    if (rx_data != NULL && rx_data->data != NULL) {
        free(rx_data->data);
        rx_data->data = NULL;
        rx_data->length = -1;
    }
}