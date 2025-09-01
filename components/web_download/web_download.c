#include "web_download.h"
#include "freertos/FreeRTOS.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "safe_fatfs.h" // 假设这是您的线程安全文件系统接口
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_download";

// --- 服务器和文件系统配置 ---
static const char *BASE_REQUEST_URL = "http://192.168.2.76:8000/request_file/";
static const char *BASE_OTA_URL = "http://192.168.2.76:8000/ota";
static const char *FILE_SYSTEM_PREFIX = "0:/";

// --- 内部辅助结构体和函数 ---

/**
 * @brief 用于在HTTP事件回调之间传递状态的上下文结构体
 */
typedef struct {
    FIL *fp;                            // FatFS 文件句柄指针
    char *output_path_buffer;           // 指向外部提供的、用于存储最终文件路径的缓冲区
    size_t output_path_buffer_size;     // 缓冲区大小
    bool header_processed;              // 标记是否已成功处理了响应头并打开了文件
} download_context_t;

// 函数声明
static esp_err_t web_download_from_url(const char *url, char *out_file_path, size_t path_buffer_size, int *out_http_status_code);
static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static esp_err_t parse_filename_from_header(const char *header_value, char *out_filename, size_t max_len);


/**
 * @brief 从 Content-Disposition 头中解析文件名
 */
static esp_err_t parse_filename_from_header(const char *header_value, char *out_filename, size_t max_len)
{
    const char *filename_ptr = strstr(header_value, "filename=");
    if (filename_ptr == NULL) {
        ESP_LOGE(TAG, "Content-Disposition header does not contain 'filename='");
        return ESP_FAIL;
    }
    filename_ptr += strlen("filename=");

    char *start = (char*)filename_ptr;
    if (*start == '\"') {
        start++;
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

/**
 * @brief 核心HTTP事件处理函数
 */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    download_context_t *ctx = (download_context_t *)evt->user_data;
    if (!ctx) return ESP_FAIL;

    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            if (ctx->fp) {
                safe_f_close(ctx->fp);
                free(ctx->fp);
                ctx->fp = NULL;
            }
            return ESP_FAIL;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            ctx->header_processed = false;
            break;

        case HTTP_EVENT_HEADERS_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADERS_SENT, awaiting response headers...");
            break;

        case HTTP_EVENT_ON_HEADER:
            // 此事件在 esp-idf 4.3+ 中非常有用，可以在这里直接解析
            if (strcasecmp(evt->header_key, "Content-Disposition") == 0) {
                 ESP_LOGI(TAG, "Found Content-Disposition header: %s", (char *)evt->header_value);
                
                char filename[128] = {0};
                if (parse_filename_from_header(evt->header_value, filename, sizeof(filename)) == ESP_OK) {
                    snprintf(ctx->output_path_buffer, ctx->output_path_buffer_size, "%s%s", FILE_SYSTEM_PREFIX, filename);
                    ESP_LOGI(TAG, "Opening file for writing: %s", ctx->output_path_buffer);

                    ctx->fp = malloc(sizeof(FIL));
                    if (!ctx->fp || safe_f_open(ctx->fp, ctx->output_path_buffer, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
                        ESP_LOGE(TAG, "Failed to open file for writing: %s", ctx->output_path_buffer);
                        if(ctx->fp) free(ctx->fp);
                        ctx->fp = NULL;
                        return ESP_FAIL;
                    }
                    ctx->header_processed = true;
                } else {
                    return ESP_FAIL;
                }
            }
            break;
        
        case HTTP_EVENT_ON_DATA:
            if (!ctx->header_processed) break; // 如果头还没处理完，忽略数据
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!ctx->fp) {
                ESP_LOGE(TAG, "Received data but file is not open!");
                return ESP_FAIL;
            }
            UINT bytes_written = 0;
            if (safe_f_write(ctx->fp, evt->data, evt->data_len, &bytes_written) != FR_OK || bytes_written != evt->data_len) {
                ESP_LOGE(TAG, "File write error");
                return ESP_FAIL;
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (ctx->fp) {
                ESP_LOGI(TAG, "File download finished. Closing file.");
                safe_f_close(ctx->fp);
                free(ctx->fp);
                ctx->fp = NULL;
            }
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            if (ctx->fp) {
                ESP_LOGW(TAG, "Disconnected unexpectedly, closing file.");
                safe_f_close(ctx->fp);
                free(ctx->fp);
                ctx->fp = NULL;
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

/**
 * @brief [核心通用函数] 从一个完整的URL下载文件。
 */
static esp_err_t web_download_from_url(const char *url, char *out_file_path, size_t path_buffer_size, int *out_http_status_code)
{
    download_context_t *ctx = calloc(1, sizeof(download_context_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate download context");
        return ESP_FAIL;
    }
    ctx->output_path_buffer = out_file_path;
    ctx->output_path_buffer_size = path_buffer_size;
    if (out_file_path) out_file_path[0] = '\0';

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .user_data = ctx,
        .timeout_ms = 20000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    *out_http_status_code = esp_http_client_get_status_code(client);
    
    // 逻辑错误检查：如果服务器返回200，但我们因为某些原因没能打开文件和解析出路径，也应视为失败
    if (err == ESP_OK && *out_http_status_code == 200 && (out_file_path == NULL || strlen(out_file_path) == 0)) {
         ESP_LOGE(TAG, "HTTP 200 OK, but filename was not resolved. Check server headers and logs.");
         err = ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    free(ctx);

    return err;
}


// --- 公开的业务函数实现 ---

esp_err_t web_download_file_by_alias(const char *alias, char *out_file_path, size_t path_buffer_size)
{
    char full_url[256];
    snprintf(full_url, sizeof(full_url), "%s%s", BASE_REQUEST_URL, alias);
    
    int http_status = 0;
    esp_err_t err = web_download_from_url(full_url, out_file_path, path_buffer_size, &http_status);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Download by alias failed (transaction error): %s", esp_err_to_name(err));
        return err;
    }
    if (http_status != 200) {
        ESP_LOGE(TAG, "Download by alias failed (server status %d for alias '%s')", http_status, alias);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Successfully downloaded file by alias. Path: %s", out_file_path);
    return ESP_OK;
}

ota_status_t web_ota_check_and_download(const char *device_model, const char *current_version, char *out_file_path, size_t path_buffer_size)
{
    char full_url[256];
    snprintf(full_url, sizeof(full_url), "%s?device_model=%s&current_version=%s",
             BASE_OTA_URL, device_model, current_version);

    int http_status = 0;
    esp_err_t err = web_download_from_url(full_url, out_file_path, path_buffer_size, &http_status);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA check failed (transaction error): %s", esp_err_to_name(err));
        return OTA_CHECK_FAILED;
    }

    if (http_status == 200) {
        ESP_LOGI(TAG, "OTA update successfully downloaded. Path: %s", out_file_path);
        return OTA_UPDATE_SUCCESSFUL;
    } else if (http_status == 304) {
        ESP_LOGI(TAG, "Device firmware is already up-to-date.");
        return OTA_NO_UPDATE_AVAILABLE;
    } else {
        ESP_LOGE(TAG, "OTA check failed (server status %d)", http_status);
        return OTA_CHECK_FAILED;
    }
}
