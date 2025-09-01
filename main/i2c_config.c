/**
 * @file i2c_config.c
 * @brief I2C总线配置实现
 */

#include "i2c_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ads111x.h"
#include "i2cdev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_CONFIG";

// ADS1115设备描述符
static i2c_dev_t ads1115_dev = {0};
static bool ads1115_initialized = false;

esp_err_t i2c_master_init(void)
{
    // 初始化i2cdev库
    esp_err_t ret = i2cdev_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2cdev库初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C总线配置成功 (SCL: GPIO%d, SDA: GPIO%d, 频率: %dHz)", 
             I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, I2C_MASTER_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_master_deinit(void)
{
    // i2cdev库会自动管理I2C驱动的生命周期
    ESP_LOGI(TAG, "I2C主机反初始化");
    return ESP_OK;
}

esp_err_t ads1115_init(void)
{
    if (ads1115_initialized) {
        ESP_LOGW(TAG, "ADS1115已经初始化过了");
        return ESP_OK;
    }

    // 初始化ADS1115设备描述符
    esp_err_t ret = ads111x_init_desc(&ads1115_dev, ADS1115_I2C_ADDR, I2C_MASTER_NUM, 
                                      I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115设备描述符初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }

    // 配置ADS1115基本参数
    ret = ads111x_set_mode(&ads1115_dev, ADS111X_MODE_SINGLE_SHOT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115模式设置失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }

    ret = ads111x_set_gain(&ads1115_dev, ADS111X_GAIN_4V096);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115增益设置失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }

    ret = ads111x_set_data_rate(&ads1115_dev, ADS111X_DATA_RATE_128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115数据速率设置失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }

    // 测试ADS1115连接
    int16_t test_value;
    ret = ads111x_start_conversion(&ads1115_dev);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));  // 等待转换完成
        ret = ads111x_get_value(&ads1115_dev, &test_value);
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADS1115初始化成功 (地址: 0x%02X)", ADS1115_I2C_ADDR);
        ESP_LOGI(TAG, "ADS1115测试读取值: %d", test_value);
        ads1115_initialized = true;
    } else {
        ESP_LOGW(TAG, "ADS1115通信测试失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }

    return ESP_OK;
}

void* ads1115_get_handle(void)
{
    return ads1115_initialized ? &ads1115_dev : NULL;
}
