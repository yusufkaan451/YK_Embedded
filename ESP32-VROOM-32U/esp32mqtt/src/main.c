#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "freertos/semphr.h"


static SemaphoreHandle_t mutex;

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_URI "m"
#define MQTT_TOPIC "your/topic"

static const char *TAG = "WIFI_MQTT_APP";
esp_mqtt_client_handle_t mqtt_client;
char last_message[256];

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    if (event->event_id == MQTT_EVENT_DATA) {
        if (strncmp(event->topic, "topic1", event->topic_len) == 0) {
            xSemaphoreTake(mutex, portMAX_DELAY);
            memset(last_message, 0, sizeof(last_message));
            strncpy(last_message, event->data, event->data_len);
            xSemaphoreGive(mutex);
            ESP_LOGI(TAG, "Received message on topic1: %s", last_message);
        }
    }
    return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying connection...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connected to WiFi");

        esp_mqtt_client_config_t mqtt_cfg = {
            .broker= {
                .address.uri = MQTT_URI,
            }
        };

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(mqtt_client);
        esp_mqtt_client_subscribe(mqtt_client, "topic1", 0);
    }
}

void message_publisher_task(void *pvParameter) {
    while(1) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (strlen(last_message) > 0) {
            esp_mqtt_client_publish(mqtt_client, "topic2", last_message, 0, 0, 0);
            ESP_LOGI(TAG, "Published message to topic2: %s", last_message);
            memset(last_message, 0, sizeof(last_message)); // Clear after publishing
        }
        xSemaphoreGive(mutex);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    mutex = xSemaphoreCreateMutex();

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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreatePinnedToCore(&message_publisher_task, "message_publisher_task", 2048, NULL, 5, NULL, 0);

}

