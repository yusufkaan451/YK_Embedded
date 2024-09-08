#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"  // Ensure you have this line to avoid the implicit declaration error
#include <inttypes.h>
#include <string.h>

static const char *TAG = "main";
#define LED_GPIO 2
#define UART_NUM UART_NUM_0

SemaphoreHandle_t xSemaphore;

TaskHandle_t counter_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;

void uart_send_data(const char* data) {
    uart_write_bytes(UART_NUM, data, strlen(data));
}

void counter_task(void *pvParameter) {
    uint32_t counter = 1;
    char buffer[64];
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {  // Try to take the semaphore
            sprintf(buffer, "Counter Value: %" PRIu32 "\n", counter);
            uart_send_data(buffer);
            counter++;
            xSemaphoreGive(xSemaphore);  // Give back the semaphore
        }
    }
}


void led_task(void *pvParameter) {
    int blink_count = 0;
    while(1) {
        // Blink the LED 60 times (each blink is 0.5 seconds on + 0.5 seconds off = 1 second, total 30 seconds)
        while (blink_count < 60) {
            gpio_set_level(LED_GPIO, 0);  // LED on
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(LED_GPIO, 1);  // LED off
            vTaskDelay(500 / portTICK_PERIOD_MS);
            blink_count++;
        }
        
        xSemaphoreGive(xSemaphore);  // Give the semaphore after blinking for 30 seconds
        
        // After giving the semaphore, we want this task to block itself and wait for another task to run
        xSemaphoreTake(xSemaphore, portMAX_DELAY);  // It will wait here indefinitely
        
        blink_count = 0;  // Reset blink_count, so it will blink again for 30 seconds the next time
    }
}




void app_main() {
    ESP_LOGI(TAG, "Starting the application...");
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);  // Initially give the semaphore so that tasks can proceed

    // ... (the rest of your UART and LED GPIO initialization code)

    // Remove the following button GPIO configuration and ISR setup
    // gpio_config_t io_conf = {
    //     .pin_bit_mask = (1ULL << BUTTON_GPIO),
    //     .mode = GPIO_MODE_INPUT,
    //     .intr_type = GPIO_INTR_NEGEDGE,
    // };
    // gpio_config(&io_conf);
    // gpio_install_isr_service(0);
    // gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    xTaskCreate(&counter_task, "Counter Task", 2048, NULL, 5, &counter_task_handle);
    xTaskCreate(&led_task, "LED Task", 2048, NULL, 5, &led_task_handle);
}
