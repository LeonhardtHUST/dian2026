#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "UART_APP";

#define APP_UART_NUM UART_NUM_0
#define BUF_SIZE 128
#define UART_BAUD_RATE 115200

// 发送 "Hello World" 的任务
static void uart_tx_task(void *arg) {
    const char *msg = "Hello World\r\n";

    while (1) {
        uart_write_bytes(APP_UART_NUM, msg, strlen(msg));
        // ESP_LOGI(TAG, "Hello World");
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延迟 1000ms (1Hz)
    }
}

// 接收用户输入的任务
static void uart_rx_task(void *arg) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RX buffer");
        vTaskDelete(NULL);
    }
    
    while (1) {
        // 接收数据
        int len = uart_read_bytes(APP_UART_NUM, data, (BUF_SIZE - 1), 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            // 检查是否包含回车符 (\r 或 \n)
            for (int i = 0; i < len; i++) {
                if (data[i] == '\r' || data[i] == '\n') {
                    // 输出指定的文本
                    uart_write_bytes(APP_UART_NUM, "GEL37KXHDU9G\r\nFXLKNKWHVURC\r\nCE4K7KEYCUPQ\r\n", 42);
                    break;
                }
            }
        }
    }
}

void app_main() {
    ESP_LOGI(TAG, "App UART is UART0 (%d baud)", UART_BAUD_RATE);

    // 配置 UART 参数
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 应用配置
    uart_driver_install(APP_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(APP_UART_NUM, &uart_config);
    
    // 创建发送任务（1Hz 发送 "Hello World"）
    xTaskCreate(uart_tx_task, "uart_tx_task", 2048, NULL, 10, NULL);
    
    // 创建接收任务（接收用户输入）
    xTaskCreate(uart_rx_task, "uart_rx_task", 2048, NULL, 10, NULL);
}