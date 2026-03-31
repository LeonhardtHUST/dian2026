#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 FAT 存储与文件系统。
 *
 * @return esp_err_t 初始化成功返回 ESP_OK。
 */
esp_err_t storage_init(void);

/**
 * @brief 格式化/清除整个 FAT storage 分区。
 *
 * @return esp_err_t 格式化成功返回 ESP_OK。
 */
esp_err_t storage_format(void);

/**
 * @brief 将数据写入文件 (如果文件存在则覆盖)。
 *
 * @param filename 文件名 (如 "test.txt", 前面不需要加斜杠)
 * @param data 待写入的一段文本内容，以 '\0' 结尾
 * @return esp_err_t 写入成功返回 ESP_OK。
 */
esp_err_t storage_write_file(const char *filename, const char *data);

/**
 * @brief 追加数据到文件末尾。
 *
 * @param filename 文件名
 * @param data 待追加的文本内容，以 '\0' 结尾
 * @return esp_err_t 追加成功返回 ESP_OK。
 */
esp_err_t storage_append_file(const char *filename, const char *data);

/**
 * @brief 从文件中读取全部内容。
 *
 * @param filename 文件名
 * @param data_out 指针的指针，用于接收内部分配的包含文件内容的缓存。使用完毕后【必须使用 free(*data_out)】释放内存！
 * @param size_out 读取到的字节数（可传 NULL 如果不需要）
 * @return esp_err_t 读取成功返回 ESP_OK。如果返回失败，*data_out 会被置为空。
 */
esp_err_t storage_read_file(const char *filename, char **data_out, size_t *size_out);

/**
 * @brief 删除文件。
 *
 * @param filename 文件名
 * @return esp_err_t 删除成功返回 ESP_OK。
 */
esp_err_t storage_delete_file(const char *filename);

/**
 * @brief 检查文件是否存在。
 *
 * @param filename 文件名
 * @return true 存在，false 不存在。
 */
bool storage_file_exists(const char *filename);

/**
 * @brief 获取文件大小。
 *
 * @param filename 文件名
 * @param size_out 输出文件大小用的指针
 * @return esp_err_t 如果文件存在并成功获得大小返回 ESP_OK。
 */
esp_err_t storage_get_file_size(const char *filename, size_t *size_out);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_H
