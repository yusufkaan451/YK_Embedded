#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define INPUT_PIN 32
#define OUTPUT_PIN 4
#define SQUARE_WAVE_PIN 27
#define SQUARE_WAVE_PERIOD_MS 10  // Square wave period (ms)

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

void check_input_pin(void* arg) {
    int last_state = 0;
    for (;;) {
        int current_state = gpio_get_level(INPUT_PIN);
        if (current_state != last_state && current_state == 1) {
            printf("Possible rising edge detected\n");  // Move this line here
            current_state = gpio_get_level(INPUT_PIN);  // Check the pin state again
            if (current_state == 1) {  // Confirm the rising edge
                printf("Rising edge confirmed, sending signal to Pin 4\n");
                gpio_set_level(OUTPUT_PIN, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);  // Hold high level for 10 ms
                gpio_set_level(OUTPUT_PIN, 0);
            }
        }
        last_state = current_state;
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Increase this delay
    }
}

void app_main() {
    // Set pin configurations
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,  // Enable pull-up resistor
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << OUTPUT_PIN) | (1ULL << SQUARE_WAVE_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 1;  // Enable pull-down resistor
    gpio_config(&io_conf);

    // Create tasks
    xTaskCreate(generate_square_wave, "SquareWaveTask", 2048, NULL, 1, NULL);  // Lower priority
    xTaskCreate(check_input_pin, "CheckInputPinTask", 2048, NULL, 2, NULL);  // Higher priority
}

