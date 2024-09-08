#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define INPUT_PIN 32
#define OUTPUT_PIN 4
#define SQUARE_WAVE_PIN 27
#define SQUARE_WAVE_PERIOD_MS 10  // Square wave period (ms)

void IRAM_ATTR gpio_isr_handler(void* arg) {
    gpio_set_level(OUTPUT_PIN, 1);
    gpio_set_level(OUTPUT_PIN, 0);
}

void generate_square_wave(void* arg) {
    for (;;) {
        // Send high level signal to Pin 27
        gpio_set_level(SQUARE_WAVE_PIN, 1);
        vTaskDelay(SQUARE_WAVE_PERIOD_MS / 2 / portTICK_PERIOD_MS);
        // Send low level signal to Pin 27
        gpio_set_level(SQUARE_WAVE_PIN, 0);
        vTaskDelay(SQUARE_WAVE_PERIOD_MS / 2 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    // Set pin configurations
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,  // Rising edge interrupt
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << OUTPUT_PIN) | (1ULL << SQUARE_WAVE_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;  // Disable interrupt
    gpio_config(&io_conf);

    // Set up ISR service and interrupt handlers with highest priority
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(INPUT_PIN, gpio_isr_handler, NULL);

    // Create task to generate square wave
    xTaskCreatePinnedToCore(
    generate_square_wave,  // task function
    "GenerateSquareWaveTask",  // task name
    2048,  // stack size
    NULL,  // task input parameter
    1,  // task priority
    NULL,  // task handle
    0  // core number
);
}
