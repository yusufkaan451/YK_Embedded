#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"  // Ensure you have this line to avoid the implicit declaration error
#include <inttypes.h>
#include <string.h>

static const char *TAG = "main";
#define LED_GPIO 2
#define BUTTON_GPIO 0
#define UART_NUM UART_NUM_0

volatile bool uart_enabled = true;  // Flag to control UART data sending
static int64_t last_button_press_time = 0;

TaskHandle_t counter_task_handle = NULL;
TaskHandle_t led_task_handle = NULL;

void uart_send_data(const char* data) {
    if (uart_enabled) {  // Only send data if uart_enabled is true
        uart_write_bytes(UART_NUM, data, strlen(data));
    }
}

static void IRAM_ATTR button_isr_handler(void* arg) {
    int64_t now = esp_timer_get_time();
    if ((now - last_button_press_time) > (1000 * 1000)) {  // 50 ms debounce time, adjusted to 1 second
        uart_enabled = !uart_enabled;  // Toggle the uart_enabled flag
    }
    last_button_press_time = now;
}

void counter_task(void *pvParameter) {
    uint32_t counter = 1;
    char buffer[64];
    while (1) {
        // ESP_LOGI(TAG, "Counter Value: %" PRIu32, counter);  // This line has been commented out
        sprintf(buffer, "Counter Value: %" PRIu32 "\n", counter);
        uart_send_data(buffer);
        counter++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1000ms
    }
}


void led_task(void *pvParameter) {
    while(1) {
        {
            gpio_set_level(LED_GPIO, 0);  // LED on
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(LED_GPIO, 1);  // LED off
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }  
        
    }
}


void app_main() {
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
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    xTaskCreate(&counter_task, "Counter Task", 2048, NULL, 5, &counter_task_handle);
    xTaskCreate(&led_task, "LED Task", 2048, NULL, 5, &led_task_handle);
}
