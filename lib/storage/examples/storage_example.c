#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "storage.h"

static const char *TAG = "STORAGE_EXAMPLE";

/**
 * @brief Storage(存储)库功能演示任务
 */
void storage_example_task(void *pvParameters) {
    ESP_LOGI(TAG, "=== 开始 Storage 功能演示 ===");

    // 1. 初始化文件系统（SPIFFS）
    // 必须在使用文件读写功能之前调用此函数！
    if (storage_init() != ESP_OK) {
        ESP_LOGE(TAG, "文件系统初始化失败，退出示例！");
        vTaskDelete(NULL);
        return;
    }

    const char *test_filename = "test_note.txt";

    // 【可选】如果想完全清除Flash中的所有数据，可以取消下面这行的注释：
    // storage_format();

    // 2. 检查文件是否存在
    if (storage_file_exists(test_filename)) {
        ESP_LOGI(TAG, "文件 '%s' 已存在，本次写入将覆盖原有内容。", test_filename);
    } else {
        ESP_LOGI(TAG, "文件 '%s' 不存在，即将新建。", test_filename);
    }

    // 3. 写入数据（覆盖模式）
    const char *msg1 = "Hello, EPS32 Storage!\n这是第一行测试数据。\n";
    if (storage_write_file(test_filename, msg1) == ESP_OK) {
        ESP_LOGI(TAG, "写入第一段数据成功！");
    } else {
        ESP_LOGE(TAG, "写入失败！");
    }

    // 4. 追加数据到文件末尾
    const char *msg2 = "这是追加的第二段测试数据。\n";
    if (storage_append_file(test_filename, msg2) == ESP_OK) {
        ESP_LOGI(TAG, "追加数据成功！");
    } else {
        ESP_LOGE(TAG, "追加失败！");
    }

    // 5. 获取文件大小
    size_t file_size = 0;
    if (storage_get_file_size(test_filename, &file_size) == ESP_OK) {
        ESP_LOGI(TAG, "检测到文件 '%s' 总大小: %u 字节", test_filename, file_size);
    }

    // 6. 读取文件全部内容
    char *read_buf = NULL;    // 用来接收分配的缓冲区指针
    size_t read_bytes = 0;    // 用来接收实际读出的字节数

    if (storage_read_file(test_filename, &read_buf, &read_bytes) == ESP_OK) {
        ESP_LOGI(TAG, "成功读取文件内容 (共 %u 字节)，内容如下：\n", read_bytes);
        printf("---------------------------------------\n");
        printf("%s", read_buf);
        printf("---------------------------------------\n");

        // [!!!!!重要!!!!!]
        // 由于 storage_read_file 内部使用 malloc 动态分配了内容缓存
        // 我们在用完这个 buffer 之后，务必要手动 free 释放，防止内存泄漏！
        free(read_buf);
        read_buf = NULL; // 养成良好的置空习惯
    } else {
        ESP_LOGE(TAG, "读取文件失败！");
    }

    // 7. 删除文件 (如果需要每次运行保持干净环境)
    // 如果在你的应用里你需要永久保存这个文件，请注释掉此段代码
    if (storage_delete_file(test_filename) == ESP_OK) {
        ESP_LOGI(TAG, "测试文件删除成功，环境已清理。");
    } else {
        ESP_LOGE(TAG, "测试文件删除失败！");
    }

    ESP_LOGI(TAG, "=== Storage 示例任务结束 ===");

    // 结束任务
    vTaskDelete(NULL);
}

/**
 * @brief 示例程序的入口配置函数
 */
void app_main(void) {
    // 启动演示任务
    xTaskCreate(storage_example_task, "storage_example_task", 4096, NULL, 5, NULL);
}
