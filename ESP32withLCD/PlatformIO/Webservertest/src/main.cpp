#include <string.h>
#include <cstring> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include <ctime>
#include <stdlib.h>  
#include <sstream>
#include "lwip/apps/sntp.h"
#include <time.h>
#include <sys/time.h>
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"


bool isLedOn = false;
constexpr gpio_num_t BLUE_LED_PIN = GPIO_NUM_17;
std::string sensorDataLog;
esp_mqtt_client_handle_t mqtt_client;
char last_message[256];
static bool mqtt_connected = false;
std::string sharedData;



constexpr auto WIFI_SSID = "ayy"; // Replace with your Wi-Fi SSID
constexpr auto WIFI_PASS = "w!123456"; // Replace with your Wi-Fi Password
constexpr auto WIFI_CONNECTED_BIT = BIT0;
#define MQTT_URI "mqtt://homeio:79dDJcWriy6q3s5@mqtt.gohm.tech:1883"



static EventGroupHandle_t s_wifi_event_group;
constexpr char TAG[] = "Webserver";

struct SensorData {
    std::string timestamp; // Changed to store a string
    uint64_t measurement;  // Placeholder for sensor measurement
    uint64_t power;        // Placeholder for power data
};

void initialize_sntp() {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org"); // Use an NTP server
    sntp_init();

    // Set timezone to Eastern European Time (Istanbul)
    setenv("TZ", "EET-3EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();
}

struct tm timeinfo = { 0 };
void obtain_time() {
    initialize_sntp();

    // Wait for time to be set
    time_t now = 0;
    // struct tm timeinfo = { 0 }; // Remove this local declaration
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGE(TAG, "Failed to obtain time from NTP server.");
    } else {
        ESP_LOGI(TAG, "System time synchronized with NTP server.");
    }
}

SensorData sensorData;

// Function to simulate sensor measurement
uint64_t getSensorMeasurement() {
    // Placeholder for sensor measurement logic
    return rand(); // Using a random number for demonstration
}

// Function to simulate power data calculation
uint64_t calculatePowerData() {
    // Placeholder for power calculation logic
    return rand(); // Using a random number for demonstration
}

void updateSensorDataLog() {
    std::stringstream logEntry;
    logEntry << "<tr><td>" << sensorData.timestamp 
             << "</td><td>" << sensorData.measurement 
             << "</td><td>" << sensorData.power << "</td></tr>\n";

    // Add the new entry to the existing log
    sensorDataLog = logEntry.str() + sensorDataLog;
}


void updateSensorDataTask(void *pvParameters) {
    while (true) {
        time_t now;
        time(&now); 
        struct tm *timeinfo = localtime(&now);
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%d-%m-%Y %H:%M:%S", timeinfo);

        // Update sensor data
        sensorData.timestamp = strftime_buf; // Store formatted string
        sensorData.measurement = getSensorMeasurement();
        sensorData.power = calculatePowerData();

        // Log the updated sensor data
        ESP_LOGI(TAG, "Updated sensor data: Timestamp=%s, Measurement=%llu, Power=%llu",
                 strftime_buf, sensorData.measurement, sensorData.power);
        
        updateSensorDataLog();

        if (mqtt_connected) {
            std::string mqtt_payload = "Sensor Data: Timestamp=" + sensorData.timestamp +
                                       ", Measurement=" + std::to_string(sensorData.measurement) +
                                       ", Power=" + std::to_string(sensorData.power);
            int msg_id = esp_mqtt_client_publish(mqtt_client, "esp32/data", mqtt_payload.c_str(), 0, 1, 0);
            if (msg_id < 0) {
                ESP_LOGE(TAG, "Failed to publish MQTT message");
            } else {
                ESP_LOGI(TAG, "Published MQTT message with ID %d", msg_id);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}
static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    auto* event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            esp_mqtt_client_subscribe(event->client, "esp32/data", 0);

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
}

static esp_err_t post_handler(httpd_req_t *req) {
    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req) {
    gpio_set_level(BLUE_LED_PIN, 0);

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%d-%m-%Y %H:%M:%S", &timeinfo);

    sharedData = "<p>Current Time: " + std::string(strftime_buf) +
                 "</p><table border=\"1\"><tr><th>Timestamp</th><th>Measurement</th><th>Power</th></tr>" +
                 sensorDataLog + "</table>";

    // HTTP yanıtını gönder
    httpd_resp_send(req, sharedData.c_str(), sharedData.length());

    gpio_set_level(BLUE_LED_PIN, 1);
    return ESP_OK;
}

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Server started on port %d", config.server_port);

        httpd_uri_t post_uri = {
            .uri       = "/post",
            .method    = HTTP_POST,
            .handler   = post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &post_uri);

        httpd_uri_t get_uri = {
            .uri       = "/get",
            .method    = HTTP_GET,
            .handler   = get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &get_uri);

    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server on port %d", config.server_port);
    }

    return server;
}


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
        esp_mqtt_client_config_t mqtt_cfg = {};
        mqtt_cfg.broker.address.uri = MQTT_URI;

        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(mqtt_client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, mqtt_client);
        esp_mqtt_client_start(mqtt_client);
        esp_mqtt_client_subscribe(mqtt_client, "esp32/data", 0);
    }
}



extern "C" void app_main() {
    auto ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    gpio_set_level(BLUE_LED_PIN, 1);
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_wifi_event_group = xEventGroupCreate();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

    ESP_ERROR_CHECK(esp_wifi_connect());

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    obtain_time();

    if (timeinfo.tm_year >= (2016 - 1900)) {
        xTaskCreate(updateSensorDataTask, "update_sensor_data", 2048, NULL, 5, NULL);
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << BLUE_LED_PIN);
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
        while (true) {
            gpio_set_level(BLUE_LED_PIN, 0);
            isLedOn = true; 
            vTaskDelay(250 / portTICK_PERIOD_MS);

            if (isLedOn) {
                gpio_set_level(BLUE_LED_PIN, 1);
                isLedOn = false; 
            }

            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
    }
}