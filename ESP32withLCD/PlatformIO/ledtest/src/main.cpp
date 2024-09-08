#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

constexpr gpio_num_t BLUE_LED_PIN = GPIO_NUM_17;

extern "C" void app_main() {
    // GPIO pinini çıkış olarak ayarla
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << BLUE_LED_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    while (true) {
        // LED'i aç
        gpio_set_level(BLUE_LED_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 saniye bekle

        // LED'i kapat
        gpio_set_level(BLUE_LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 saniye bekle
    }
}
