#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "nvs_flash.h"

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp> 

constexpr auto WIFI_SSID = "ayy";
constexpr auto WIFI_PASS = "w!123456";
constexpr auto MAX_FIRMWARE_SIZE = 1024 * 1024;
constexpr auto WIFI_CONNECTED_BIT = BIT0;

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "ota_example";
static LGFX lcd;

httpd_handle_t start_webserver();
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t download_handler(httpd_req_t *req);

void setup_display() {
    lcd.init();
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(2);
    lcd.drawString("Ota testing LCD", lcd.width()/2 - 60, lcd.height()/2 - 10);
}

void convertStringToWifiConfig(char* dest, const char* src, size_t maxLen) {
    strncpy(reinterpret_cast<char*>(dest), src, maxLen);
    dest[maxLen - 1] = '\0'; // Güvenlik için son karakteri null yap
}

extern "C" void app_main() {

    // Initialize NVS
    auto ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize netif and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create event group
    s_wifi_event_group = xEventGroupCreate();

    // Initialize Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_create_default_wifi_sta();

    // Configure Wi-Fi
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    convertStringToWifiConfig(reinterpret_cast<char*>(wifi_config.sta.ssid), WIFI_SSID, sizeof(wifi_config.sta.ssid));
    convertStringToWifiConfig(reinterpret_cast<char*>(wifi_config.sta.password), WIFI_PASS, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_start());

    // ... [Wi-Fi yapılandırmasının devamı]

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, nullptr));

    // Connect to Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Wait for Wi-Fi connection
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    // Setup display
    setup_display();
}

// Wi-Fi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP address, starting webserver");
        start_webserver();
    }
}

// OTA download handler
static esp_err_t download_handler(httpd_req_t *req) {
    char ota_buff[1024];
    int buff_len;
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA...");

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "Partition could not be found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No partition found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx", update_partition->subtype, (unsigned long)update_partition->address);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Begin Failed");
        return err;
    }

    while (1) {
        memset(ota_buff, 0, sizeof(ota_buff));
        buff_len = httpd_req_recv(req, ota_buff, sizeof(ota_buff));
        if (buff_len < 0) {
            ESP_LOGE(TAG, "Receive buffer error: %s", esp_err_to_name(buff_len));
            esp_ota_end(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive Buffer Failed");
            return ESP_FAIL;
        } else if (buff_len > 0) {
            if (esp_ota_write(update_handle, (const void *)ota_buff, buff_len) != ESP_OK) {
                ESP_LOGE(TAG, "OTA Write Failed");
                esp_ota_end(update_handle);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Write Failed");
                return ESP_FAIL;
            }
        } else if (buff_len == 0) {
            ESP_LOGI(TAG, "Firmware image uploaded");
            break;
        }
    }

    if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA End Failed");
        return ESP_FAIL;
    }

    if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
        ESP_LOGE(TAG, "OTA set boot partition failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set Boot Partition Failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA succeeded");
    httpd_resp_send(req, "OTA Update Success", HTTPD_RESP_USE_STRLEN);

    esp_restart();

    return ESP_OK;
}

// HTTP server start function
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Server started on port %d", config.server_port);
        httpd_uri_t ota_update_uri = {
            .uri       = "/update",
            .method    = HTTP_POST,
            .handler   = download_handler,
            .user_ctx  = NULL
        };

        httpd_register_uri_handler(server, &ota_update_uri);
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server on port %d", config.server_port);
    }

    return server;
}
