#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "main";
#define LED_GPIO 2
#define BUTTON_GPIO 0

static int led_state = 0;  // LED durumu (0 kapalı, 1 açık)

void button_task(void *pvParameter) {
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

    int last_button_state = 1;  // Son tuş durumu (varsayılan olarak serbest)
    while (1) {
        int button_state = gpio_get_level(BUTTON_GPIO);  // Tuşun mevcut durumunu oku

        if (button_state == 0 && last_button_state == 1) {  // Tuşa basılmış
            ESP_LOGI(TAG, "Button pressed. Changing LED state.");
            led_state = !led_state;  // LED durumunu değiştir
            gpio_set_level(LED_GPIO, led_state);
            vTaskDelay(200 / portTICK_PERIOD_MS);  // Debounce için kısa bir gecikme
        }

        last_button_state = button_state;  // Son tuş durumunu güncelle
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    ESP_LOGI(TAG, "Starting the application...");
    
    // LED'i yapılandır
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, led_state);  // Başlangıçta LED kapalı

    // Görevleri oluştur
    xTaskCreate(&button_task, "button_task", 2048, NULL, 5, NULL);
}
