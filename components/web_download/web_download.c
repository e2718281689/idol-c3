#include "freertos/FreeRTOS.h"
#include "web_download.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "safe_fatfs.h"    // 引入线程安全文件接口
#include <string.h>

static char *DOWNLOAD_URL = "http://idolc3.cjiax.top:3466/request_file/";
static char *FILE_PATH    = "0:/";

static char *TAG = "web_download";

// HTTP 事件处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    FIL *fp = (FIL *)evt->user_data; // FatFS 文件类型

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        UINT bw = 0;
        if (safe_f_write(fp, evt->data, evt->data_len, &bw) != FR_OK || bw != evt->data_len) {
            ESP_LOGE(TAG, "File write error");
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        ESP_LOGI(TAG, "File download finished. Closing file.");
        safe_f_close(fp);
        free(fp); // 我们 malloc 了 fp，最后释放
        break;

    default:
        break;
    }
    return ESP_OK;
}

// 文件下载任务
esp_err_t web_download_file(char *url, char *FILE_url)
{
    char buffer[200];

    snprintf(buffer, sizeof(buffer), "%s%s.bin", FILE_PATH, url);
    strcpy(FILE_url, buffer);

    ESP_LOGI(TAG, "Opening file for writing: %s", buffer);

    // 分配文件句柄（FatFS 的 FIL 类型）
    FIL *fp = malloc(sizeof(FIL));
    if (!fp) {
        ESP_LOGE(TAG, "Failed to allocate FIL");
        return ESP_FAIL;
    }

    if (safe_f_open(fp, buffer, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        free(fp);
        return ESP_FAIL;
    }

    snprintf(buffer, sizeof(buffer), "%s%s", DOWNLOAD_URL, url);
    ESP_LOGI(TAG, "Download URL: %s", buffer);

    esp_http_client_config_t config = {
        .url = buffer,
        .event_handler = _http_event_handler,
        .user_data = fp,        // 传递 FatFS 文件句柄
        .timeout_ms = 10000,    // 10秒超时
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        // 失败也要关闭文件，防止泄漏
        safe_f_close(fp);
        free(fp);
    }

    esp_http_client_cleanup(client);

    return err;
}
