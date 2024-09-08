#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"

#define WIFI_SSID "ayy"
#define WIFI_PASSWORD "w!123456"
#define MQTT_URI "mqtt://homeio:79dDJcWriy6q3s5@mqtt.gohm.tech:1883"
#define MQTT_TOPIC "your/topic"

static const char *TAG = "WIFI_MQTT_APP";
esp_mqtt_client_handle_t mqtt_client;
char last_message[256];

extern "C" {
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void app_main();
}
static bool mqtt_connected = false;

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            // Now it's safe to subscribe or publish
            esp_mqtt_client_subscribe(event->client, "esp32/data", 0);
            // ... additional subscriptions or publications
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            // Implement reconnection strategy here if desired
            break;

        case MQTT_EVENT_DATA:
            if (strncmp(event->topic, "esp32/data", event->topic_len) == 0) {
                memset(last_message, 0, sizeof(last_message));
                strncpy(last_message, event->data, event->data_len);
                ESP_LOGI(TAG, "Received message on esp32/data: %s", last_message);
            }
            break;
    }
    // No return statement needed as the function's return type is void
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying connection...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to WiFi");

        esp_mqtt_client_config_t mqtt_cfg = {};
        mqtt_cfg.broker.address.uri = MQTT_URI;

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(mqtt_client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, mqtt_client);
        esp_mqtt_client_start(mqtt_client);
        esp_mqtt_client_subscribe(mqtt_client, "esp32/data", 0);
    }
}

void message_publisher_task(void *pvParameter) {
    while(true) {
        if (std::strlen(last_message) > 0) {
            esp_mqtt_client_publish(mqtt_client, "", last_message, 0, 0, 0);
            ESP_LOGI(TAG, "Published message to : %s", last_message);
            memset(last_message, 0, sizeof(last_message)); // Clear after publishing
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, nullptr, nullptr));

    wifi_config_t wifi_config = {};
    std::strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID);
    std::strcpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreatePinnedToCore(message_publisher_task, "message_publisher_task", 2048, nullptr, 5, nullptr, 0);
}
