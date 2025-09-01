/**
 * @file i2c_config.c
 * @brief I2C总线配置实现
 */

#include "i2c_config.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "I2C_CONFIG";
static bool i2c_initialized = false;

esp_err_t i2c_master_init(void)
{
    if (i2c_initialized) {
        ESP_LOGW(TAG, "I2C已经初始化过了");
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C参数配置失败: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                            I2C_MASTER_RX_BUF_DISABLE, 
                            I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C驱动安装失败: %s", esp_err_to_name(err));
        return err;
    }

    i2c_initialized = true;
    ESP_LOGI(TAG, "I2C主机初始化成功 (SCL: GPIO%d, SDA: GPIO%d, 频率: %dHz)", 
             I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, I2C_MASTER_FREQ_HZ);
    
    return ESP_OK;
}

esp_err_t i2c_master_deinit(void)
{
    if (!i2c_initialized) {
        ESP_LOGW(TAG, "I2C没有初始化");
        return ESP_OK;
    }

    esp_err_t err = i2c_driver_delete(I2C_MASTER_NUM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C驱动删除失败: %s", esp_err_to_name(err));
        return err;
    }

    i2c_initialized = false;
    ESP_LOGI(TAG, "I2C主机反初始化成功");
    
    return ESP_OK;
}
