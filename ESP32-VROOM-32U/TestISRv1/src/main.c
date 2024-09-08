#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <driver/uart.h>

#define BUTTON_PIN 0
#define OUTPUT_PIN_27 27
#define OUTPUT_PIN_32 32
#define LED_PIN 2
#define UART_NUM UART_NUM_0

SemaphoreHandle_t xSemaphore;
int64_t time_stamp_27 = 0;
int64_t time_stamp_32 = 0;

void IRAM_ATTR onButtonPress() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
    gpio_set_level(LED_PIN, 1);  // Turn on LED
    portYIELD_FROM_ISR();
}

void generateSquareWaveCore0(void * parameter) {
    for(;;) {
        // Generate square wave while waiting for semaphore
        while (xSemaphoreTake(xSemaphore, 0) == pdFALSE) {
            // Generate a square wave on Pin 27
            gpio_set_level(OUTPUT_PIN_27, 1);
            vTaskDelay(10 / portTICK_PERIOD_MS);  // Square wave width
            gpio_set_level(OUTPUT_PIN_27, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);  // Square wave width
        }
        // Suspend this task when semaphore is taken
        vTaskSuspend(NULL);
    }
}

void generateSquareWaveCore1(void * parameter) {
    bool generateWave = false;  // Square wave generation control variable

    for(;;) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
            generateWave = true;  // Start square wave generation when semaphore is taken
        }

        while (generateWave) {
            // Generate a square wave on Pin 32
            gpio_set_level(OUTPUT_PIN_32, 1);
            time_stamp_32 = esp_timer_get_time(); 
            vTaskDelay(10 / portTICK_PERIOD_MS);  // Square wave width
            gpio_set_level(OUTPUT_PIN_32, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);  // Square wave width
        }

        // Check time difference after exiting generateWave loop
        if (time_stamp_27 != 0 && time_stamp_32 != 0) {
            int64_t time_difference = (time_stamp_32 - time_stamp_27) / 1000;  // Convert microseconds to milliseconds
            printf("Time difference: %lld ms\n", time_difference);
            time_stamp_27 = 0;  // Reset time stamps
            time_stamp_32 = 0;
        }
    }
}

void app_main() {
    xSemaphore = xSemaphoreCreateBinary();
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);

    // Set pin configurations
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<OUTPUT_PIN_27) | (1ULL<<OUTPUT_PIN_32) | (1ULL<<LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL<<BUTTON_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;  // Enable pull-up resistor
    io_conf.intr_type = GPIO_INTR_ANYEDGE;  // Trigger interrupt on both edges
    gpio_config(&io_conf);

    // Set up ISR service and interrupt handlers
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, onButtonPress, NULL);

    // Create a task on Core 0
    xTaskCreatePinnedToCore(
        generateSquareWaveCore0,   // Task function
        "SquareWaveCore0",         // Task name
        10000,                     // Stack size
        NULL,                      // Task parameter
        1,                         // Task priority
        NULL,                      // Task handle
        0                          // Core number
    );

    // Create a task on Core 1
    xTaskCreatePinnedToCore(
        generateSquareWaveCore1,   // Task function
        "SquareWaveCore1",         // Task name
        10000,                     // Stack size
        NULL,                      // Task parameter
        1,                         // Task priority
        NULL,                      // Task handle
        1                          // Core number
    );
}
