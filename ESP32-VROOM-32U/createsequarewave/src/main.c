#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#define OUTPUT_PIN 27  // Kare dalga üreteceğiniz pin numarası

void generateSquareWave(void *parameter) {
    while (1) {
        gpio_set_level(OUTPUT_PIN, 1);  // Pin'i yüksek seviyeye ayarla
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Bekle (kare dalga yüksek seviye süresi)
        gpio_set_level(OUTPUT_PIN, 0);  // Pin'i düşük seviyeye ayarla
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Bekle (kare dalga düşük seviye süresi)
    }
}

void app_main() {
    // Pin konfigürasyonunu ayarla
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Kare dalga üretme görevini oluştur
    xTaskCreate(
        generateSquareWave,  // Görev fonksiyonu
        "SquareWaveTask",    // Görev adı
        2048,                // Yığın boyutu
        NULL,                // Görev parametresi
        1,                   // Görev önceliği
        NULL                 // Görev kolu
    );
}
