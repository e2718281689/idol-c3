#include "controller.h"
#include "wifi_bt_net_model.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "wifi_prov_mgr.h"
#include "esp_http_client.h"
#include "esp_log.h"
// #include "web_download.h"

static char  *DOWNLOAD_URL = "http://idolc3.cjiax.top:3466/request_file/";
static char  *FILE_PATH = "/littlefs/";

static char *TAG = "web_download";

// HTTP 事件处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    FILE* f = (FILE*)evt->user_data;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // 将接收到的数据块写入文件
            if (fwrite(evt->data, 1, evt->data_len, f) != evt->data_len) {
                ESP_LOGE(TAG, "File write error");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            ESP_LOGI(TAG, "File download finished. Closing file.");
            fclose(f);
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// 文件下载任务
esp_err_t web_download_file(char *url, char* FILE_url)
{

    char buffer[200]; 
    
    // char *xxx = "/littlefs/Chie_240.bin";
    snprintf(buffer, sizeof(buffer), "%s%s.bin", FILE_PATH, url);
    strcpy(FILE_url,buffer);
    
    ESP_LOGI(TAG, "Opening file for writing: %s", buffer);
    FILE* f = fopen(buffer, "wb"); // 使用 "wb" 以二进制写模式打开
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        vTaskDelete(NULL);
        return ESP_FAIL;
    }

    snprintf(buffer, sizeof(buffer), "%s%s", DOWNLOAD_URL, url);
    ESP_LOGI(TAG, "download url: %s", buffer);

    esp_http_client_config_t config = {
        .url = buffer,
        .event_handler = _http_event_handler,
        .user_data = f,        // 将文件句柄传递给事件处理器
        .timeout_ms = 10000,   // 设置超时时间为10秒
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        // 如果请求失败，事件处理器中的 ON_FINISH 可能不会被调用，所以在这里也要确保关闭文件
        fclose(f);
    }

    esp_http_client_cleanup(client);

    return err;
}
