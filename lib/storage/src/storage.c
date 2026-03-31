#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "wear_levelling.h"

static const char *TAG = "STORAGE";

#define BASE_PATH "/fatfs"

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// 内部辅助函数：给文件名自动补全基础路径
static void get_full_path(char *dest, size_t max_len, const char *filename) {
    if (filename[0] == '/') {
        snprintf(dest, max_len, "%s%s", BASE_PATH, filename);
    } else {
        snprintf(dest, max_len, "%s/%s", BASE_PATH, filename);
    }
}

/**
 * @brief 初始化文件系统（FAT）。
 * 
 * 在使用任何文件操作（读、写、删等）前必须调用此函数挂载分区。
 * 详情：使用给定的分区表自动挂载（默认寻找 "storage" 分区），并在失败时自动进行格式化尝试。
 * 
 * @return esp_err_t 初始化成功返回 ESP_OK，如果设备没有对应分区则返回 ESP_ERR_NOT_FOUND。
 */
esp_err_t storage_init(void) {
    ESP_LOGI(TAG, "Initializing FAT filesystem...");

    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 5,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };

    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl(BASE_PATH, "storage", &mount_config, &s_wl_handle);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find FAT partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize FAT (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    uint64_t total = 0, free_bytes = 0;
    ret = esp_vfs_fat_info(BASE_PATH, &total, &free_bytes);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get FAT partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %llu bytes, free: %llu bytes", total, free_bytes);
    }

    return ESP_OK;
}

/**
 * @brief 格式化此存储分区（破坏性操作）。
 * 
 * 将立刻擦除分区中的整个文件系统及所有文件。
 * 
 * @return esp_err_t 格式化成功返回 ESP_OK，格式化失败返回相应的错误码。
 */
esp_err_t storage_format(void) {
    ESP_LOGW(TAG, "Formatting FAT partition. This will erase all data.");
    
    // 如果已经挂载，我们需要先卸载才能格式化
    if (s_wl_handle != WL_INVALID_HANDLE) {
        esp_vfs_fat_spiflash_unmount_rw_wl(BASE_PATH, s_wl_handle);
        s_wl_handle = WL_INVALID_HANDLE;
    }

    esp_err_t ret = esp_vfs_fat_spiflash_format_rw_wl(BASE_PATH, "storage");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FAT (%s)", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Format successful, partition cleared!");
    
    // 重新挂载
    ESP_LOGI(TAG, "Remounting...");
    return storage_init();
}

/**
 * @brief 在此分区下写入字符串文件。
 * 
 * 如果该同名文件不存在，则自动创建；如果已存在该文件，该文件的旧有内容将被立刻丢弃覆盖。
 * 
 * @param filename 要保存的文件名（例如："my_config.txt"，不需要包含绝对挂载路径/fatfs）。
 * @param data 要写入的字符串内容指针（需以 '\0' 结尾）。
 * @return esp_err_t 成功写入返回 ESP_OK，传参错误返回 ESP_ERR_INVALID_ARG，写入失败返回 ESP_FAIL。
 */
esp_err_t storage_write_file(const char *filename, const char *data) {
    if (!filename || !data) return ESP_ERR_INVALID_ARG;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    ESP_LOGI(TAG, "Writing file: %s", filepath);
    FILE *f = fopen(filepath, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    return ESP_OK;
}

/**
 * @brief 在文件末尾追加字符串。
 * 
 * 如果文件存在，将在已有内容末尾直接写入；若文件不存在，则创建该文件并写入数据。
 * 
 * @param filename 要保存的文件名。
 * @param data 要追加的字符串内容指针。
 * @return esp_err_t 成功追加返回 ESP_OK，传参错误返回 ESP_ERR_INVALID_ARG。
 */
esp_err_t storage_append_file(const char *filename, const char *data) {
    if (!filename || !data) return ESP_ERR_INVALID_ARG;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    ESP_LOGI(TAG, "Appending to file: %s", filepath);
    FILE *f = fopen(filepath, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", filepath);
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    return ESP_OK;
}

/**
 * @brief 从存储中读取文件的全部内容。
 * 
 * 内部会根据文件原始大小，通过 malloc 分配足以包含完整内容及末尾 `\0` 的新内存块。
 * ⚠️ 极其重要: 调用者负责在读取完成后，通过 `free(*data_out)` 妥善释放这块被分配的内存堆，否则会造成内存泄漏！
 * 
 * @param filename 要读取的文件名。
 * @param data_out 字符指针的地址，用于回传指向新分配内存块的指针。
 * @param size_out 若不需要获取实际读取字节数可传 NULL。否则将向该指针存入实际提取的字节内容。
 * @return esp_err_t 成功返回 ESP_OK（此时 *data_out 已分配数据）。若文件不存在返回 ESP_ERR_NOT_FOUND，若堆用尽返回 ESP_ERR_NO_MEM。
 */
esp_err_t storage_read_file(const char *filename, char **data_out, size_t *size_out) {
    if (!filename || !data_out) return ESP_ERR_INVALID_ARG;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    *data_out = NULL;
    if (size_out != NULL) {
        *size_out = 0;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        return ESP_ERR_NOT_FOUND;
    }

    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filepath);
        return ESP_FAIL;
    }

    size_t fsize = st.st_size;
    char *buf = (char *)malloc(fsize + 1);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for reading");
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    size_t read_bytes = fread(buf, 1, fsize, f);
    buf[read_bytes] = '\0'; // 确保字符串正确收尾
    fclose(f);

    *data_out = buf;
    if (size_out != NULL) {
        *size_out = read_bytes;
    }

    return ESP_OK;
}

/**
 * @brief 删除文件。
 * 
 * 若文件原本已存在将被移除；如果文件不存在将抛出告警。
 * 
 * @param filename 要删除的文件名。
 * @return esp_err_t 成功删除返回 ESP_OK，如果执行遇到错误返回 ESP_FAIL。
 */
esp_err_t storage_delete_file(const char *filename) {
    if (!filename) return ESP_ERR_INVALID_ARG;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    ESP_LOGI(TAG, "Deleting file: %s", filepath);
    if (unlink(filepath) != 0) {
        ESP_LOGE(TAG, "Failed to delete file: %s", filepath);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief 判断探测文件是否存在。
 * 
 * 若需要处理覆盖保护防呆或初始化检测，可以使用本函数。
 * 
 * @param filename 要检查的文件名。
 * @return true 该文件当前确切存在于分区中。
 * @return false 该路径为空。
 */
bool storage_file_exists(const char *filename) {
    if (!filename) return false;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    struct stat st;
    if (stat(filepath, &st) == 0) {
        return true;
    }
    return false;
}

/**
 * @brief 取得此文件的预分配文件总字节大小。
 * 
 * 适用于你只想读取信息或知道容量，又尚未或不想分配内存读取文件的场景。
 * 
 * @param filename 要检查的文件名。
 * @param size_out 输出用的尺寸内存指针。
 * @return esp_err_t 文件不存在返回 ESP_ERR_NOT_FOUND，正常查询获取返回 ESP_OK。
 */
esp_err_t storage_get_file_size(const char *filename, size_t *size_out) {
    if (!filename || !size_out) return ESP_ERR_INVALID_ARG;

    char filepath[128];
    get_full_path(filepath, sizeof(filepath), filename);

    struct stat st;
    if (stat(filepath, &st) == 0) {
        *size_out = st.st_size;
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}
