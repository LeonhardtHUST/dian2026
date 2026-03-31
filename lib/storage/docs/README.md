# SPIFFS 存储库说明 (Storage Library)

这是一个基于 ESP-IDF SPIFFS (SPI Flash File System) 的轻量级文件系统操作封装库。它为设备提供了一个简单易用的持久化存储接口，支持常见的文件读写、追加、删除和信息查询等功能。

## 目录结构
- `src/storage.c` & `src/storage.h`: 核心实现代码
- `docs/README.md`: 本文档
- `examples`: 包含如何使用该库的示例代码

## 依赖与配置

1. **分区表 (Partition Table)**:
   要使用此库，您的 ESP32 设备需要在自定义的分区表 (`partitions.csv`) 中包含一个名为 `storage` 的 `spiffs` 数据分区。例如：
   ```csv
   # Name,   Type, SubType, Offset,  Size, Flags
   nvs,      data, nvs,     ,        0x4000,
   otadata,  data, ota,     ,        0x2000,
   phy_init, data, phy,     ,        0x1000,
   factory,  app,  factory, ,        4M,
   storage,  data, spiffs,  ,        11M,
   ```
2. **PlatformIO 配置**:
   在 `platformio.ini` 中应用上述分区表：
   ```ini
   board_build.partitions = partitions.csv
   ```

## API 概览

详细的参数和返回值请参考 `storage.h` 和 `storage.c` 中的函数注释。以下是提供的主要功能：

| 函数名称                 | 功能描述                                   |
|--------------------------|------------------------------------------|
| `storage_init()`         | 初始化并挂载 SPIFFS 文件系统，基路径为 `/spiffs`。 |
| `storage_format()`       | 格式化 `storage` 分区（清空所有数据）。         |
| `storage_write_file()`   | 将数据写入指定文件（如果文件存在则覆盖）。       |
| `storage_append_file()`  | 将数据追加写入到指定文件末尾。                 |
| `storage_read_file()`    | 动态分配内存并读取文件完整内容，用完需手动释放！ |
| `storage_file_exists()`  | 检查指定文件名是否存在。                       |
| `storage_delete_file()`  | 删除指定的文件。                             |
| `storage_get_file_size()`| 获取文件的确切字节大小。                       |

## 快速使用示例

```c
#include "storage.h"
#include <esp_log.h>
#include <stdlib.h>

static const char *TAG = "Storage_Example";

void app_main() {
    // 1. 初始化
    if (storage_init() != ESP_OK) {
        ESP_LOGE(TAG, "Storage 初始化失败");
        return;
    }

    // 2. 写入文件
    const char *test_data = "Hello, ESP32 Storage!";
    storage_write_file("test.txt", test_data, strlen(test_data));

    // 3. 读取文件
    char *read_data = NULL;
    size_t read_len = 0;
    if (storage_read_file("test.txt", &read_data, &read_len) == ESP_OK) {
        ESP_LOGI(TAG, "读取到的内容: %s (长度: %d)", read_data, read_len);
        // 重要：由于 storage_read_file 使用了 malloc，完成后必须手动 free
        free(read_data);
    }

    // 4. 删除文件
    storage_delete_file("test.txt");
}
```

## 注意事项

- 所有传入的文件名不需要加前缀，直接传入文件名即可（例如：`"config.json"`，库内部会自动转为 `/spiffs/config.json`）。
- **内存泄漏警告**：`storage_read_file` 为了灵活性会在内部分配足够的内存来装载文件数据，请**务必在处理完数据后调用 `free()` 释放内存**。
- SPIFFS 不具有真正的目录结构，它表现为一个扁平的文件系统。
