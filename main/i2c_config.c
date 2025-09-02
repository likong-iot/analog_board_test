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

    // 使用±4.096V增益范围以支持0-3.3V电压测量
    ret = ads111x_set_gain(&ads1115_dev, ADS111X_GAIN_4V096);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115增益设置失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }

    // 使用较高的采样率以获得更稳定的读数
    ret = ads111x_set_data_rate(&ads1115_dev, ADS111X_DATA_RATE_250);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115数据速率设置失败: %s", esp_err_to_name(ret));
        ads111x_free_desc(&ads1115_dev);
        return ret;
    }
    
    // 禁用比较器
    ret = ads111x_set_comp_queue(&ads1115_dev, ADS111X_COMP_QUEUE_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115比较器设置失败: %s", esp_err_to_name(ret));
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

esp_err_t ads1115_read_voltage(uint8_t channel, float *voltage_v)
{
    if (!ads1115_initialized) {
        ESP_LOGE(TAG, "ADS1115未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (channel >= ADS1115_CHANNEL_COUNT || voltage_v == NULL) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 设置输入多路复用器到指定通道（单端输入，对地）
    ads111x_mux_t mux_config;
    switch (channel) {
        case 0: mux_config = ADS111X_MUX_0_GND; break;
        case 1: mux_config = ADS111X_MUX_1_GND; break;
        case 2: mux_config = ADS111X_MUX_2_GND; break;
        case 3: mux_config = ADS111X_MUX_3_GND; break;
        default: return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = ads111x_set_input_mux(&ads1115_dev, mux_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置ADS1115通道%d失败: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 启动转换
    ret = ads111x_start_conversion(&ads1115_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动ADS1115转换失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 等待转换完成 (250 SPS需要4ms，增加安全余量)
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // 读取原始值
    int16_t raw_value;
    ret = ads111x_get_value(&ads1115_dev, &raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取ADS1115值失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 数据有效性检查 - 防止异常读数
    if (raw_value < -32768 || raw_value > 32767) {
        ESP_LOGW(TAG, "通道%d原始值超出范围: %d", channel, raw_value);
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // 转换为电压值 (使用4.096V增益)
    // 对于单端模式，使用32768.0f避免数值翻倍
    *voltage_v = (float)raw_value * 4.096f / 32768.0f;
    
    // 电压值合理性检查
    if (*voltage_v < -4.1f || *voltage_v > 4.1f) {
        ESP_LOGW(TAG, "通道%d电压值异常: %.3fV (原始值: %d)", channel, *voltage_v, raw_value);
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    return ESP_OK;
}

esp_err_t ads1115_read_current(uint8_t channel, float *current_ma)
{
    if (current_ma == NULL) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }
    
    float voltage_v;
    esp_err_t ret = ads1115_read_voltage(channel, &voltage_v);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 计算电流: I = V / R，转换为毫安
    *current_ma = (voltage_v / ADS1115_SHUNT_RESISTOR_OHMS) * 1000.0f;
    
    return ESP_OK;
}

esp_err_t ads1115_read_all_currents(float currents_ma[ADS1115_CHANNEL_COUNT])
{
    if (currents_ma == NULL) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }
    
    for (uint8_t channel = 0; channel < ADS1115_CHANNEL_COUNT; channel++) {
        esp_err_t ret = ads1115_read_current(channel, &currents_ma[channel]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取通道%d电流失败", channel);
            return ret;
        }
        
        // 通道切换间隔，避免过快切换
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    return ESP_OK;
}

esp_err_t ads1115_read_all_detailed(ads1115_channel_data_t channel_data[ADS1115_CHANNEL_COUNT])
{
    if (!ads1115_initialized) {
        ESP_LOGE(TAG, "ADS1115未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (channel_data == NULL) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }
    
    for (uint8_t ch = 0; ch < ADS1115_CHANNEL_COUNT; ch++) {
        // 设置输入多路复用器
        ads111x_mux_t mux_config;
        switch (ch) {
            case 0: mux_config = ADS111X_MUX_0_GND; break;
            case 1: mux_config = ADS111X_MUX_1_GND; break;
            case 2: mux_config = ADS111X_MUX_2_GND; break;
            case 3: mux_config = ADS111X_MUX_3_GND; break;
            default: 
                channel_data[ch].status = ESP_ERR_INVALID_ARG;
                continue;
        }
        
        esp_err_t ret = ads111x_set_input_mux(&ads1115_dev, mux_config);
        if (ret != ESP_OK) {
            channel_data[ch].status = ret;
            continue;
        }
        
        // 启动转换
        ret = ads111x_start_conversion(&ads1115_dev);
        if (ret != ESP_OK) {
            channel_data[ch].status = ret;
            continue;
        }
        
        // 等待转换完成
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // 读取原始值
        ret = ads111x_get_value(&ads1115_dev, &channel_data[ch].raw_value);
        if (ret == ESP_OK) {
            // 数据有效性检查 - 防止异常读数
            if (channel_data[ch].raw_value < -32768 || channel_data[ch].raw_value > 32767) {
                ESP_LOGW(TAG, "通道%d原始值超出范围: %d", ch, channel_data[ch].raw_value);
                channel_data[ch].status = ESP_ERR_INVALID_RESPONSE;
                continue;
            }
            
            // 计算电压 (4.096V增益)
            // 对于单端模式，使用32768.0f避免数值翻倍
            channel_data[ch].voltage_v = (float)channel_data[ch].raw_value * 4.096f / 32768.0f;
            
            // 电压值合理性检查
            if (channel_data[ch].voltage_v < -4.1f || channel_data[ch].voltage_v > 4.1f) {
                ESP_LOGW(TAG, "通道%d电压值异常: %.3fV (原始值: %d)", ch, channel_data[ch].voltage_v, channel_data[ch].raw_value);
                channel_data[ch].status = ESP_ERR_INVALID_RESPONSE;
                continue;
            }
            
            channel_data[ch].current_ma = (channel_data[ch].voltage_v / ADS1115_SHUNT_RESISTOR_OHMS) * 1000.0f;
            
            // 电流值合理性检查 (理论最大136.5mA)
            if (channel_data[ch].current_ma < -150.0f || channel_data[ch].current_ma > 150.0f) {
                ESP_LOGW(TAG, "通道%d电流值异常: %.2fmA", ch, channel_data[ch].current_ma);
                channel_data[ch].status = ESP_ERR_INVALID_RESPONSE;
                continue;
            }
            
            channel_data[ch].status = ESP_OK;
        } else {
            channel_data[ch].status = ret;
        }
        
        // 通道间延时
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    return ESP_OK;
}

esp_err_t ads1115_get_config_info(ads1115_config_info_t *config_info)
{
    if (!ads1115_initialized) {
        ESP_LOGE(TAG, "ADS1115未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (config_info == NULL) {
        ESP_LOGE(TAG, "参数无效");
        return ESP_ERR_INVALID_ARG;
    }
    
    static const char* gain_strings[] = {"±6.144V", "±4.096V", "±2.048V", "±1.024V", "±0.512V", "±0.256V", "±0.256V", "±0.256V"};
    static const uint16_t rate_values[] = {8, 16, 32, 64, 128, 250, 475, 860};
    
    // 读取增益配置
    ads111x_gain_t gain;
    esp_err_t ret = ads111x_get_gain(&ads1115_dev, &gain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取增益失败: %s", esp_err_to_name(ret));
        return ret;
    }
    config_info->gain = gain;
    config_info->gain_str = (gain < 8) ? gain_strings[gain] : "未知";
    
    // 读取数据速率
    ads111x_data_rate_t rate;
    ret = ads111x_get_data_rate(&ads1115_dev, &rate);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取数据速率失败: %s", esp_err_to_name(ret));
        return ret;
    }
    config_info->data_rate = rate;
    config_info->rate_sps = (rate < 8) ? rate_values[rate] : 0;
    
    // 读取模式
    ads111x_mode_t mode;
    ret = ads111x_get_mode(&ads1115_dev, &mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取工作模式失败: %s", esp_err_to_name(ret));
        return ret;
    }
    config_info->mode = mode;
    config_info->mode_str = (mode == ADS111X_MODE_CONTINUOUS) ? "连续" : "单次";
    
    return ESP_OK;
}
