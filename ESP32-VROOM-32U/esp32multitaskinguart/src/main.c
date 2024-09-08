#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include <inttypes.h>
#include <string.h>
static const char *TAG = "main";
#define LED_GPIO 2
#define BUTTON_GPIO 0
#define UART_NUM UART_NUM_0
static int led_state = 0;  // LED state (0 off, 1 on)
static bool tasks_running = true;
TaskHandle_t counter_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;
void uart_send_data(const char* data) {
    uart_write_bytes(UART_NUM, data, strlen(data));
}
void button_task(void *pvParameter) {
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    int last_button_state = 1;
    while (1) {
        int button_state = gpio_get_level(BUTTON_GPIO);
        if (button_state == 0 && last_button_state == 1) {
            ESP_LOGI(TAG, "Button pressed.");
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
            tasks_running = !tasks_running;  // Toggle the state of the tasks
            if (tasks_running) {
                vTaskResume(counter_task_handle);
                vTaskResume(led_task_handle);
            } else {
                vTaskSuspend(counter_task_handle);
                vTaskSuspend(led_task_handle);
            }
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        last_button_state = button_state;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void led_task(void *pvParameter)
{
    while(1)
    {
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void counter_task(void *pvParameter)
{
    uint32_t counter = 0;
    char buffer[64];
    while(1)
    {
        sprintf(buffer, "Counter: %lu\n", counter);
        uart_send_data(buffer);
        counter++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void app_main()
{
    ESP_LOGI(TAG, "Starting the application...");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    xTaskCreatePinnedToCore(&counter_task, "counter_task", 2048, NULL, 5, &counter_task_handle, 0);
    xTaskCreatePinnedToCore(&led_task, "led_task", 2048, NULL, 5, &led_task_handle, 1);
    xTaskCreatePinnedToCore(&button_task, "button_task", 2048, NULL, 5, NULL, 0);
}