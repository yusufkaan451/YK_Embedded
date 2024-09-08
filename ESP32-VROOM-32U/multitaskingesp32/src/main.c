#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <inttypes.h>


static const char *TAG = "main";
#define LED_GPIO 2

// Task1: Print counter value
void counter_task(void *pvParameter)
{
    uint32_t counter = 1;
    while(1)
    {
        ESP_LOGI(TAG, "Counter Value: %" PRIu32, counter);
        counter++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1000ms
    }
}

// Task2: Toggle LED on and off
void led_task(void *pvParameter)
{
    while(1)
    {
        gpio_set_level(LED_GPIO, 0);  // LED on
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1000ms

        gpio_set_level(LED_GPIO, 1);  // LED off
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1000ms
    }
}

void app_main()
{
    ESP_LOGI(TAG, "Starting the application...");

    // Configure LED GPIO
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    // Create tasks and pin them to Core 0
    xTaskCreatePinnedToCore(&counter_task, "counter_task", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&led_task, "led_task", 2048, NULL, 5, NULL, 1);
}