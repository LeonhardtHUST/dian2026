#include <utils.h>
#include "driver/uart.h"
#include "esp_log.h"

// 定义日志标签
static const char *TAG = "UART_APP";

// UART 配置

// ESP32 的 UART0 默认连接到 USB 转串口芯片，通常用于调试输出和用户交互
// 这里我们使用 UART0 来发送和接收数据
// 如果你想使用其他 UART（如 UART1 或 UART2），需要修改引脚连接并相应调整代码
// UART0 的默认引脚配置（TX: GPIO1, RX: GPIO3）
// 注意：如果你使用 UART0，确保不要同时使用它进行调试输出，否则可能会干扰正常的串口通信
// 使用其他的 UART 端口需要使用 USB 转串口模块连接，并相应调整引脚配置和代码中的 UART_NUM
#define APP_UART_NUM UART_NUM_0

// BUF_SIZE: UART 接收缓冲区的大小，单位为字节（Bytes）
// 注意区分发送缓冲区和接收缓冲区。在本程序中，初始化 freertos 时将发送缓冲区设置为零，但是接收缓冲区一定要有！
// 接收缓冲区的意义在于，它可以化解“不可预测的外部数据”和“CPU多任务调度”之间的时间差，确保在 CPU 处理其他任务时，UART 接收的数据不会丢失。UART 接收缓冲区会暂存接收到的数据，直到 CPU 有时间处理它们。
// 128 字节的缓冲区大小适合大多数简单的串口通信应用。如果你需要接收更大的数据包，可以增加这个值，但要注意内存使用情况。
#define BUF_SIZE 128

// UART_BAUD_RATE: UART 波特率
#define UART_BAUD_RATE 115200

static TaskHandle_t uart_tx_handle = NULL;
static TaskHandle_t uart_rx_handle = NULL;

static void uart_tx_task(void *arg) {
    const char *msg = "Hello World\r\n";

    while (1) {
        uart_write_bytes(APP_UART_NUM, msg, strlen(msg));
        // ESP_LOGI(TAG, "Hello World");

        /*
            注意以上两行代码。如果取消第二行的注释，你会看到监视器中 Hello World 和 ESP_LOGI 交替输出，这是因为输出任务使用了 UART0，而 ESP_LOGI 也默认使用 UART0 进行日志输出。两者会互相干扰，导致输出混乱。
            如果你想同时使用 UART0 进行数据通信和日志输出，建议将日志输出重定向到其他 UART（如 UART1 或 UART2），或者使用其他调试方法（如 JTAG 调试）来避免干扰。
        */

        vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延迟 1000ms (1Hz)
        // vTaskDelay() 参数为延迟的 ticks，portTICK_PERIOD_MS 是 FreeRTOS 定义的一个宏，表示每个 tick 的时间长度（以毫秒为单位）。
        // 所以 1000 / portTICK_PERIOD_MS 就是 1000ms 对应了多少个 ticks
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
            bool q_pressed = false;
            // 检查是否包含回车符 (\r 或 \n) 或者退出符 (q)
            for (int i = 0; i < len; i++) {
                if (data[i] == 'q' || data[i] == 'Q') {
                    q_pressed = true;
                    break;
                }
                if (data[i] == '\r' || data[i] == '\n') {
                    // 输出指定的文本
                    uart_write_bytes(APP_UART_NUM, "GEL37KXHDU9G\r\nFXLKNKWHVURC\r\nCE4K7KEYCUPQ\r\n", 42);
                    break;
                }
            }

            if (q_pressed) {
                free(data);
                if (uart_tx_handle != NULL) {
                    vTaskDelete(uart_tx_handle);
                    uart_tx_handle = NULL;
                }
                uart_rx_handle = NULL;
                vTaskDelete(NULL);
            }
        }
    }
}

void helloworlduart() {
    ESP_LOGI(TAG, "App UART is UART0 (%d baud)", UART_BAUD_RATE);
    
    // Print instructions to the console so user knows what to do
    const char* instr = "\r\n--- UART TASK ---\r\nPress Enter to show secret. Press 'q' or 'Q' to return to menu.\r\n";
    uart_write_bytes(APP_UART_NUM, instr, strlen(instr));

    // 配置 UART 参数
    // 8-N-1 配置：8 数据位、无校验、1 停止位
    // 由于 UART 的校验方法使用奇偶校验，并不实用（实践中常用其协议层上位替代，例如CRC），因此通常禁用校验位（UART_PARITY_DISABLE）。
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,            // 波特率，115200 是常用的波特率，适合大多数应用场景。更高的波特率可以提高数据传输速度，但可能会增加通信错误的风险，特别是在长距离或干扰较大的环境中。
        .data_bits = UART_DATA_8_BITS,          // 数据位，8 bits = 1 Byte，这是最常用的配置
        .parity = UART_PARITY_DISABLE,          // 校验位
        .stop_bits = UART_STOP_BITS_1,          // 停止位，1 stop bit 是最常用的配置，2 stop bits 会增加通信的稳定性，但会降低数据传输效率
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // 流控制
        .source_clk = UART_SCLK_DEFAULT,        // 时钟源
    };
    
    // 应用配置
    /*
        uart_driver_install(
            APP_UART_NUM,   // 1. 串口号
            BUF_SIZE * 2,   // 2. 接收（RX）缓冲区大小
            0,              // 3. 发送（TX）缓冲区大小
            0,              // 4. 事件队列的大小
            NULL,           // 5. 事件队列的句柄
            0               // 6. 中断分配标志
        );

        uart_param_config() 函数的参数说明：
        1. 串口号：指定要配置的 UART 端口，例如 APP_UART_NUM。
        2. 接收（RX）缓冲区大小：为 UART 接收数据分配的缓冲区大小，单位为字节，例如 BUF_SIZE * 2。
            这里我们设置为 BUF_SIZE 的两倍，以确保有足够的空间来存储接收的数据，避免数据丢失。
        3. 发送（TX）缓冲区大小：为 UART 发送数据分配的缓冲区大小，单位为字节，例如 0。
            如果设置为 0，表示不使用发送缓冲区，直接将数据发送出去（阻塞型通信）。这适用于发送数据量较小且不频繁的情况。
            当数据量较大时，如果仍然使用零发送缓冲区，可能会造成较大的发送阻塞。此时一般开一个 tx buffer 启用非阻塞型通信，发送消息时只需要把消息往缓冲区里丢就行了，底层会自动处理发送过程，避免阻塞。
        4. 事件队列的大小：如果你需要使用 UART 事件（如接收完成、发送完成等），可以设置事件队列的大小。这里我们不使用事件队列，所以设置为 0。
        5. 事件队列的句柄：如果你设置了事件队列，传递一个指向事件队列句柄的指针。这里我们不使用事件队列，所以设置为 NULL。
        6. 中断分配标志：用于指定中断分配的选项，通常设置为 0 表示默认分配。
    */
    uart_driver_install(APP_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(APP_UART_NUM, &uart_config);
    
    /*
        xTaskCreate(
            uart_tx_task,   // 1. 回调函数
            "uart_tx_task", // 2. 任务名
            2048,           // 3. 栈大小
            NULL,           // 4. 传递参数
            10,             // 5. 优先级
            NULL            // 6. 任务句柄
        );

        xTaskCreate() 函数的参数说明：
        1. 回调函数：任务执行的函数指针，例如 uart_tx_task。
            在 C 语言中，函数名本身就是一个指向函数的指针，所以我们可以直接使用函数名来传递函数指针。例如，uart_tx_task 等价于 &uart_tx_task。
        2. 任务名：一个字符串，用于标识任务，例如 "uart_tx_task"。
            这是一个单纯给人看的名字。
            它主要用在调试和查看系统状态时。比如如果有一天系统内存爆了，或者跑飞了，通过调试工具你能看到是名叫 "uart_tx_task" 的任务出了问题，而不是一堆看不懂的内存地址。
        3. 栈大小：为任务分配的栈空间大小，单位为字节，例如 2048。
            在原版的 FreeRTOS 中，它的单位是字（Word，即 4 字节），但 ESP32 对其进行了修改，这里的单位是“字节（Byte）”。
            所以 2048 意味着给它分配了 2KB 的专属运行内存（Stack）。如果你的任务里写了极大的数组，2KB 不够用了，ESP32 就会触发 Stack Overflow（栈溢出）导致无限重启。
        4. 传递参数：传递给任务函数的参数。
            这个参数会被传递给任务函数的 void *arg 参数。比如你想让 uart_tx_task 发送不同的消息，你可以把消息字符串的指针传递进去。
            在这个例子中，我们不需要传递任何参数，所以设置为 NULL。
        5. 优先级：任务的优先级。
            在 FreeRTOS 中，优先级是一个整数，数值越大优先级越高，例如 10。
            ESP32 的最高优先级通常可以到 24 左右。一旦发生抢占，如果有几个任务都在排队等 CPU，系统会优先让优先级数字最高的任务先执行。10 算是中等偏高的优先级，用来处理串口数据是合适的。
        6. 任务句柄：用于存储任务句柄的变量指针。
           如果你将来想在代码的其他地方“杀掉（Delete）”这个任务、甚至“挂起休眠（Suspend）”它，你需要有一个“遥控器”来控制它。你可以传一个 TaskHandle_t 类型的变量地址进去。
            在这个例子中，我们不需要控制任务，所以设置为 NULL。
    */

    // 创建发送任务（1Hz 发送 "Hello World"）
    xTaskCreate(uart_tx_task, "uart_tx_task", 2048, NULL, 10, &uart_tx_handle);

    // 创建接收任务（接收用户输入）
    xTaskCreate(uart_rx_task, "uart_rx_task", 2048, NULL, 10, &uart_rx_handle);
    
    while (uart_rx_handle != NULL) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    // Uninstall the driver so it can be re-installed normally if task is run again, or just let prompt use it smoothly
    uart_driver_delete(APP_UART_NUM);
    ESP_LOGI(TAG, "Returning to menu...");
}