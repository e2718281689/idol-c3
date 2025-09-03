#ifndef WEB_DOWNLOAD_H
#define WEB_DOWNLOAD_H

#include "esp_err.h"
#include <stddef.h>

/**
 * @brief OTA 检查和下载结果的状态枚举
 */
typedef enum {
    OTA_UPDATE_SUCCESSFUL,   // 固件成功下载并保存
    OTA_NO_UPDATE_AVAILABLE, // 已是最新版本，无需更新
    OTA_CHECK_FAILED         // 检查或下载过程中发生错误
} ota_status_t;

/**
 * @brief [业务函数] 通过文件别名下载文件。
 *
 * 内部会构建 "http://.../request_file/<alias>" 格式的URL，并调用核心下载函数。
 * 只有在HTTP状态码为200时才认为成功。
 *
 * @param alias 文件的别名 (例如, "latest_firmware")
 * @param out_file_path 指向一个缓冲区的指针，函数会将下载文件的完整路径写入此缓冲区。
 * @param path_buffer_size 缓冲区的最大大小。
 * @return esp_err_t ESP_OK 表示成功, 其他表示失败。
 */
esp_err_t web_download_file_by_alias(const char *alias, char *out_file_path, size_t path_buffer_size);

/**
 * @brief [业务函数] 检查并下载OTA固件更新。
 *
 * 内部会构建 "http://.../ota?device_model=...&current_version=..." 格式的URL。
 * 它能正确处理 HTTP 200 (有更新) 和 304 (无更新) 两种情况。
 *
 * @param device_model 设备型号 (e.g., "esp32-c3")
 * @param current_version 当前固件版本 (e.g., "1.0.0")
 * @param out_file_path 用于存储下载文件完整路径的缓冲区 (仅当有更新时写入)。
 * @param path_buffer_size 缓冲区的最大大小。
 * @return ota_status_t 返回OTA检查和下载的状态。
 */
ota_status_t web_ota_check_and_download(const char *device_model, const char *current_version, char *out_file_path, size_t path_buffer_size);

#endif // WEB_DOWNLOAD_H
