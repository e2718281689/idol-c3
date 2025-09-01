#include "freertos/FreeRTOS.h"
#include "web_download.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "safe_fatfs.h"    // 引入线程安全文件接口
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_download";
static const char *BASE_DOWNLOAD_URL = "http://idolc3.cjiax.top:3466/request_file/";
static const char *FILE_SYSTEM_PREFIX = "0:/";

/**
 * @brief 用于在HTTP事件回调之间传递状态的上下文结构体
 */
typedef struct {
    FIL *fp;                            // FatFS 文件句柄指针
    char *output_path_buffer;           // 指向外部提供的、用于存储最终文件路径的缓冲区
    size_t output_path_buffer_size;     // 缓冲区大小
    bool header_processed;              // 标记是否已成功处理了响应头并打开了文件
} download_context_t;


/**
 * @brief 从 Content-Disposition 头中解析文件名
 *
 * @param header_value 例如: "attachment; filename=\"my_firmware.bin\""
 * @param out_filename 指向用于存储文件名的缓冲区的指针
 * @param max_len 缓冲区的最大长度
 * @return esp_err_t ESP_OK 表示成功, ESP_FAIL 表示失败
 */
static esp_err_t parse_filename_from_header(const char *header_value, char *out_filename, size_t max_len)
{
    // 查找 "filename="
    const char *filename_ptr = strstr(header_value, "filename=");
    if (filename_ptr == NULL) {
        ESP_LOGE(TAG, "Content-Disposition header does not contain 'filename='");
        return ESP_FAIL;
    }

    // 跳过 "filename=" 部分
    filename_ptr += strlen("filename=");

    // 处理带引号和不带引号的文件名, e.g., "file.bin" or file.bin
    char *start = (char*)filename_ptr;
    if (*start == '\"') {
        start++; // 跳过起始引号
    }

    char *end = start;
    while (*end != '\0' && *end != '\"') {
        end++;
    }

    size_t len = end - start;
    if (len >= max_len) {
        ESP_LOGE(TAG, "Filename is too long for the buffer.");
        return ESP_FAIL;
    }

    strncpy(out_filename, start, len);
    out_filename[len] = '\0';
    ESP_LOGI(TAG, "Parsed filename from header: %s", out_filename);
    return ESP_OK;
}


// 核心HTTP事件处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    // 从 user_data 获取我们的上下文
    download_context_t *ctx = (download_context_t *)evt->user_data;

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        // 如果文件已打开，在这里关闭它
        if (ctx && ctx->fp) {
            safe_f_close(ctx->fp);
            free(ctx->fp); // 释放文件句柄内存
            ctx->fp = NULL;
        }
        return ESP_FAIL;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        ctx->header_processed = false; // 重置标志
        return ESP_OK;

    case HTTP_EVENT_HEADERS_SENT:
        // 在这个事件之后，我们就可以获取响应头了
        ESP_LOGD(TAG, "HTTP_EVENT_HEADERS_SENT");
        
        char content_disposition[256] = {0};
        // 获取 Content-Disposition 头
        if (esp_http_client_get_header(evt->client, "Content-Disposition", content_disposition) == ESP_OK) {
            
            char filename[128] = {0};
            // 从头信息中解析出文件名
            if (parse_filename_from_header(content_disposition, filename, sizeof(filename)) == ESP_OK) {
                
                // 构建完整的文件系统路径
                snprintf(ctx->output_path_buffer, ctx->output_path_buffer_size, "%s%s", FILE_SYSTEM_PREFIX, filename);
                ESP_LOGI(TAG, "Opening file for writing: %s", ctx->output_path_buffer);

                // 分配并打开文件
                ctx->fp = malloc(sizeof(FIL));
                if (!ctx->fp) {
                    ESP_LOGE(TAG, "Failed to allocate FIL");
                    return ESP_FAIL; // 中止下载
                }
                
                if (safe_f_open(ctx->fp, ctx->output_path_buffer, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
                    ESP_LOGE(TAG, "Failed to open file for writing: %s", ctx->output_path_buffer);
                    free(ctx->fp);
                    ctx->fp = NULL;
                    return ESP_FAIL; // 中止下载
                }
                
                // 标记头处理成功，文件已打开
                ctx->header_processed = true;

            } else {
                 ESP_LOGE(TAG, "Failed to parse filename from header. Aborting.");
                 return ESP_FAIL; // 中止下载
            }

        } else {
            ESP_LOGE(TAG, "Server did not provide Content-Disposition header. Aborting.");
            return ESP_FAIL; // 中止下载
        }
        return ESP_OK;


    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // 确保文件已成功打开
        if (!ctx->header_processed || !ctx->fp) {
            ESP_LOGE(TAG, "Received data but file is not open. Header processing might have failed.");
            return ESP_FAIL;
        }
        
        UINT bytes_written = 0;
        if (safe_f_write(ctx->fp, evt->data, evt->data_len, &bytes_written) != FR_OK || bytes_written != evt->data_len) {
            ESP_LOGE(TAG, "File write error");
            // 关闭文件并返回失败
            safe_f_close(ctx->fp);
            free(ctx->fp);
            ctx->fp = NULL;
            return ESP_FAIL;
        }
        return ESP_OK;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        ESP_LOGI(TAG, "File download finished.");
        if (ctx && ctx->fp) {
            safe_f_close(ctx->fp);
            free(ctx->fp);
            ctx->fp = NULL;
        }
        return ESP_OK;
    
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        // 如果因为错误而断开连接，确保文件已关闭
        if (ctx && ctx->fp) {
            ESP_LOGW(TAG, "Disconnected event, but file was still open. Closing now.");
            safe_f_close(ctx->fp);
            free(ctx->fp);
            ctx->fp = NULL;
        }
        break; // 继续执行
    }
    return ESP_OK;
}


/**
 * @brief 通过别名从服务器下载文件，并自动从响应头获取文件名。
 *
 * @param alias 文件的别名 (例如, "latest_firmware")
 * @param out_file_path 指向一个缓冲区的指针，函数会将下载文件的完整路径写入此缓冲区
 * @param path_buffer_size 缓冲区的最大大小
 * @return esp_err_t ESP_OK 表示成功, 其他表示失败
 */
esp_err_t web_download_file_by_alias(const char *alias, char *out_file_path, size_t path_buffer_size)
{
    // 1. 准备上下文
    download_context_t *ctx = malloc(sizeof(download_context_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate download context");
        return ESP_FAIL;
    }
    ctx->fp = NULL;
    ctx->output_path_buffer = out_file_path;
    ctx->output_path_buffer_size = path_buffer_size;
    ctx->header_processed = false;
    ctx->output_path_buffer[0] = '\0'; // 初始化输出缓冲区为空字符串

    // 2. 构建下载URL
    char download_url[256];
    snprintf(download_url, sizeof(download_url), "%s%s", BASE_DOWNLOAD_URL, alias);
    ESP_LOGI(TAG, "Requesting file from URL: %s", download_url);

    // 3. 配置HTTP客户端
    esp_http_client_config_t config = {
        .url = download_url,
        .event_handler = _http_event_handler,
        .user_data = ctx,        // 传递我们的上下文结构体
        .timeout_ms = 10000,     // 10秒超时
        .disable_auto_redirect = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    // 4. 处理结果
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200) {
            if (strlen(out_file_path) > 0) {
                ESP_LOGI(TAG, "Download successful. File saved to: %s", out_file_path);
            } else {
                ESP_LOGE(TAG, "Download finished but filename was not resolved.");
                err = ESP_FAIL; // 这是一个逻辑错误
            }
        } else {
             ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
             err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // 5. 清理
    esp_http_client_cleanup(client);
    free(ctx); // 释放上下文内存

    return err;
}
