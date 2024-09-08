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


#define WIFI_SSID "ayy"
#define WIFI_PASSWORD "w!123456"
#define MQTT_URI "mqtt://homeio:79dDJcWriy6q3s5@mqtt.gohm.tech:1883"
#define MQTT_TOPIC "your/topic"
#define INTERRUPT_TOPIC "interrupt/topic"

SemaphoreHandle_t mqttSemaphore;


static const char *TAG = "APP_EXAMPLE";  
static esp_mqtt_client_handle_t mqtt_client;
static TaskHandle_t task1_handle = NULL;
static TaskHandle_t task2_handle = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying connection...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address: %s", ip4addr_ntoa((const ip4_addr_t *) &event->ip_info.ip));


    }
}

static void task1(void *arg) {
    while (1) {
        esp_mqtt_client_publish(mqtt_client, "your/topic", "task1 i running", 0, 0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void task2(void *arg) {
    while (1) {
        esp_mqtt_client_publish(mqtt_client, "your/topic", "task2 is running", 0, 0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
    if (event->event_id == MQTT_EVENT_DATA) {
        if (strncmp(event->topic, INTERRUPT_TOPIC, event->topic_len) == 0) {
            xSemaphoreGive(mqttSemaphore);
        }
    }
    return ESP_OK;
}

static void mqttHandlerTask(void *arg) {
    char *data;
    while (1) {
        if (xSemaphoreTake(mqttSemaphore, portMAX_DELAY) == pdTRUE) {
            esp_mqtt_event_handle_t event = *((esp_mqtt_event_handle_t *)arg);  // Dereference the pointer
            data = malloc(event->data_len + 1);
            strncpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';

            if (strcmp(data, "suspend") == 0) {
                vTaskSuspend(task1_handle);
                vTaskResume(task2_handle);
            } else if (strcmp(data, "resume") == 0) {
                vTaskSuspend(task2_handle);
                vTaskResume(task1_handle);
            }

            free(data);
        }
    }
}


static void mqtt_event_handler_wrapper(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    mqtt_event_handler(event);
}


void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
      mqttSemaphore = xSemaphoreCreateBinary();
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

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_URI,
            
        }};

    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_wrapper, client);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler_wrapper, NULL);

    esp_mqtt_client_start(mqtt_client);  // Start only once

    xTaskCreatePinnedToCore(task1, "task1", 2048, NULL, 5, &task1_handle, 0);
    xTaskCreatePinnedToCore(task2, "task2", 2048, NULL, 5, &task2_handle, 1);
    xTaskCreate(mqttHandlerTask, "mqttHandlerTask", 2048, NULL, 5, NULL);  // <- You might want to pass mqtt_event_handle_t here
}