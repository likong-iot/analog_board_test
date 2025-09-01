/**
 * @file tca9535.c
 * @brief TCA9535 I/O扩展器驱动程序实现
 */

#include "tca9535.h"
#include "i2c_config.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "TCA9535";

/**
 * @brief TCA9535设备结构体
 */
typedef struct tca9535_dev_s {
    i2c_port_t i2c_port;            /*!< I2C端口号 */
    uint8_t device_addr;             /*!< 设备I2C地址 */
    uint32_t timeout_ms;             /*!< I2C操作超时时间 */
} tca9535_dev_t;

esp_err_t tca9535_create(const tca9535_config_t *config, tca9535_handle_t *handle)
{
    if (config == NULL || handle == NULL) {
        ESP_LOGE(TAG, "配置或句柄参数为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    tca9535_dev_t *dev = (tca9535_dev_t *)malloc(sizeof(tca9535_dev_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "内存分配失败");
        return ESP_ERR_NO_MEM;
    }

    dev->i2c_port = config->i2c_port;
    dev->device_addr = config->device_addr;
    dev->timeout_ms = config->timeout_ms;

    *handle = dev;
    
    ESP_LOGI(TAG, "TCA9535设备创建成功 (地址: 0x%02X, 端口: %d)", 
             dev->device_addr, dev->i2c_port);
    
    return ESP_OK;
}

esp_err_t tca9535_delete(tca9535_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "设备句柄为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    free(handle);
    ESP_LOGI(TAG, "TCA9535设备删除成功");
    
    return ESP_OK;
}

esp_err_t tca9535_read_register(tca9535_handle_t handle, tca9535_reg_t reg, uint8_t *data)
{
    if (handle == NULL || data == NULL) {
        ESP_LOGE(TAG, "参数为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    tca9535_dev_t *dev = (tca9535_dev_t *)handle;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        ESP_LOGE(TAG, "创建I2C命令链接失败");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    
    // 写寄存器地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN);
    
    // 重新启动并读取数据
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_READ_BIT, I2C_ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, I2C_NACK_VAL);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(dev->i2c_port, cmd, pdMS_TO_TICKS(dev->timeout_ms));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "读取寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t tca9535_write_register(tca9535_handle_t handle, tca9535_reg_t reg, uint8_t data)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "设备句柄为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    tca9535_dev_t *dev = (tca9535_dev_t *)handle;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        ESP_LOGE(TAG, "创建I2C命令链接失败");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(dev->i2c_port, cmd, pdMS_TO_TICKS(dev->timeout_ms));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写入寄存器0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t tca9535_read_register_pair(tca9535_handle_t handle, tca9535_reg_t reg, tca9535_register_t *data)
{
    if (handle == NULL || data == NULL) {
        ESP_LOGE(TAG, "参数为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    tca9535_dev_t *dev = (tca9535_dev_t *)handle;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        ESP_LOGE(TAG, "创建I2C命令链接失败");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    uint8_t read_data[2];
    
    // 写寄存器地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN);
    
    // 重新启动并读取2字节数据
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_READ_BIT, I2C_ACK_CHECK_EN);
    i2c_master_read(cmd, read_data, 2, I2C_NACK_VAL);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(dev->i2c_port, cmd, pdMS_TO_TICKS(dev->timeout_ms));
    i2c_cmd_link_delete(cmd);
    
    if (ret == ESP_OK) {
        data->ports.port0.byte = read_data[0];
        data->ports.port1.byte = read_data[1];
    } else {
        ESP_LOGE(TAG, "读取寄存器对0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t tca9535_write_register_pair(tca9535_handle_t handle, tca9535_reg_t reg, const tca9535_register_t *data)
{
    if (handle == NULL || data == NULL) {
        ESP_LOGE(TAG, "参数为NULL");
        return ESP_ERR_INVALID_ARG;
    }

    tca9535_dev_t *dev = (tca9535_dev_t *)handle;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL) {
        ESP_LOGE(TAG, "创建I2C命令链接失败");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    uint8_t write_data[2] = {data->ports.port0.byte, data->ports.port1.byte};
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->device_addr << 1) | I2C_WRITE_BIT, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, I2C_ACK_CHECK_EN);
    i2c_master_write(cmd, write_data, 2, I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(dev->i2c_port, cmd, pdMS_TO_TICKS(dev->timeout_ms));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写入寄存器对0x%02X失败: %s", reg, esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t tca9535_read_input(tca9535_handle_t handle, tca9535_register_t *data)
{
    return tca9535_read_register_pair(handle, TCA9535_INPUT_REG0, data);
}

esp_err_t tca9535_read_output(tca9535_handle_t handle, tca9535_register_t *data)
{
    return tca9535_read_register_pair(handle, TCA9535_OUTPUT_REG0, data);
}

esp_err_t tca9535_write_output(tca9535_handle_t handle, const tca9535_register_t *data)
{
    return tca9535_write_register_pair(handle, TCA9535_OUTPUT_REG0, data);
}

esp_err_t tca9535_read_polarity(tca9535_handle_t handle, tca9535_register_t *data)
{
    return tca9535_read_register_pair(handle, TCA9535_POLARITY_REG0, data);
}

esp_err_t tca9535_write_polarity(tca9535_handle_t handle, const tca9535_register_t *data)
{
    return tca9535_write_register_pair(handle, TCA9535_POLARITY_REG0, data);
}

esp_err_t tca9535_read_config(tca9535_handle_t handle, tca9535_register_t *data)
{
    return tca9535_read_register_pair(handle, TCA9535_CONFIG_REG0, data);
}

esp_err_t tca9535_write_config(tca9535_handle_t handle, const tca9535_register_t *data)
{
    return tca9535_write_register_pair(handle, TCA9535_CONFIG_REG0, data);
}

esp_err_t tca9535_set_pin_output(tca9535_handle_t handle, uint8_t pin, uint8_t level)
{
    if (handle == NULL || pin > 15) {
        ESP_LOGE(TAG, "参数无效 (pin: %d)", pin);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    tca9535_register_t config_reg, output_reg;
    
    // 读取当前配置
    ret = tca9535_read_config(handle, &config_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 读取当前输出
    ret = tca9535_read_output(handle, &output_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 设置引脚为输出模式
    if (pin < 8) {
        config_reg.ports.port0.byte &= ~(1 << pin);
        if (level) {
            output_reg.ports.port0.byte |= (1 << pin);
        } else {
            output_reg.ports.port0.byte &= ~(1 << pin);
        }
    } else {
        uint8_t bit = pin - 8;
        config_reg.ports.port1.byte &= ~(1 << bit);
        if (level) {
            output_reg.ports.port1.byte |= (1 << bit);
        } else {
            output_reg.ports.port1.byte &= ~(1 << bit);
        }
    }
    
    // 写入配置和输出
    ret = tca9535_write_config(handle, &config_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    return tca9535_write_output(handle, &output_reg);
}

esp_err_t tca9535_set_pin_input(tca9535_handle_t handle, uint8_t pin)
{
    if (handle == NULL || pin > 15) {
        ESP_LOGE(TAG, "参数无效 (pin: %d)", pin);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    tca9535_register_t config_reg;
    
    // 读取当前配置
    ret = tca9535_read_config(handle, &config_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 设置引脚为输入模式
    if (pin < 8) {
        config_reg.ports.port0.byte |= (1 << pin);
    } else {
        uint8_t bit = pin - 8;
        config_reg.ports.port1.byte |= (1 << bit);
    }
    
    return tca9535_write_config(handle, &config_reg);
}

esp_err_t tca9535_get_pin_level(tca9535_handle_t handle, uint8_t pin, uint8_t *level)
{
    if (handle == NULL || level == NULL || pin > 15) {
        ESP_LOGE(TAG, "参数无效 (pin: %d)", pin);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    tca9535_register_t input_reg;
    
    ret = tca9535_read_input(handle, &input_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    if (pin < 8) {
        *level = (input_reg.ports.port0.byte >> pin) & 0x01;
    } else {
        uint8_t bit = pin - 8;
        *level = (input_reg.ports.port1.byte >> bit) & 0x01;
    }
    
    return ESP_OK;
}