#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "/home/yusuf/.platformio/packages/framework-espidf/components/BME280_driver/bme280.h"

#define I2C_NUM I2C_NUM_0
#define SDA_PIN 5
#define SCL_PIN 4
#define BME280_I2C_ADDRESS 0x77

struct bme280_dev dev;
#define BME280_E_NULL_PTR -1
#define BME280_OK 0
#define BME280_E_COMM_FAIL -2

int8_t user_i2c_read(uint8_t chip_id, uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    ESP_LOGI("I2C", "Read function started");

    if (!reg_data) {
        ESP_LOGE("I2C", "Data pointer is NULL");
        return BME280_E_NULL_PTR;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_id << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_id << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, reg_data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t espRslt = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return (espRslt == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

int8_t user_i2c_write(uint8_t chip_id, uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    ESP_LOGI("I2C", "Write function started");

    if (!reg_data) {
        ESP_LOGE("I2C", "Data pointer is NULL");
        return BME280_E_NULL_PTR;
    }

    uint8_t *buf = (uint8_t *) malloc(len + 1);
    buf[0] = reg_addr;
    for (uint32_t i = 0; i < len; i++) {
        buf[i + 1] = reg_data[i];
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_id << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, buf, len + 1, true); // +1 ekledim çünkü 'buf' de reg_addr de dahil.
    i2c_master_stop(cmd);
    esp_err_t espRslt = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    free(buf);

    return (espRslt == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

void user_delay_us(uint32_t period, void *intf_ptr) {
    vTaskDelay((period + 999) / 1000 / portTICK_PERIOD_MS);
}

void i2c_master_init(void) {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM, &i2c_config);
    i2c_driver_install(I2C_NUM, i2c_config.mode, 0, 0, 0);
}

void app_main(void) {
    ESP_LOGI("MAIN", "App started");
    i2c_master_init();

    dev.chip_id= BME280_I2C_ADDRESS;
    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;  // Make sure the function signature matches the expected signature
    dev.write = user_i2c_write;  // Make sure the function signature matches the expected signature
    dev.delay_us = user_delay_us;

    int8_t rslt = bme280_init(&dev);
    if (rslt != BME280_OK) {
        ESP_LOGE("BME280", "Initialization failed with error: %d", rslt);
    } else {
        ESP_LOGI("BME280", "Initialization successful");
    }

    while (1) {
        struct bme280_data comp_data;
        bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        bme280_get_sensor_data(BME280_TEMP, &comp_data, &dev);
        printf("Temperature: %0.2f °C\n", (float)comp_data.temperature / 100.0f);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}