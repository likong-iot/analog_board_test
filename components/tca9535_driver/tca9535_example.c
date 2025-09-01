/**
 * @file tca9535_example.c
 * @brief TCA9535 I/O扩展器使用示例
 * 
 * 这个文件展示了如何使用TCA9535驱动程序。
 * 注意：这个文件不会被编译到组件中，仅作为参考。
 */

#include "tca9535.h"
#include "i2c_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TCA9535_EXAMPLE";

void tca9535_example_task(void *pvParameters)
{
    esp_err_t ret;
    
    // 1. 初始化I2C总线（在main函数中调用）
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C初始化失败");
        vTaskDelete(NULL);
        return;
    }
    
    // 2. 创建TCA9535设备句柄
    tca9535_config_t config = {
        .i2c_port = I2C_MASTER_NUM,
        .device_addr = TCA9535_I2C_ADDR,
        .timeout_ms = I2C_MASTER_TIMEOUT_MS
    };
    
    tca9535_handle_t tca9535_handle = NULL;
    ret = tca9535_create(&config, &tca9535_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TCA9535设备创建失败");
        vTaskDelete(NULL);
        return;
    }
    
    // 3. 示例1：配置单个引脚
    ESP_LOGI(TAG, "设置引脚0为输出，高电平");
    ret = tca9535_set_pin_output(tca9535_handle, 0, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置引脚0失败");
    }
    
    ESP_LOGI(TAG, "设置引脚1为输入");
    ret = tca9535_set_pin_input(tca9535_handle, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置引脚1失败");
    }
    
    // 4. 示例2：批量配置端口
    tca9535_register_t config_reg = {0};
    tca9535_register_t output_reg = {0};
    
    // 设置端口0的低4位为输出，高4位为输入
    config_reg.ports.port0.byte = 0xF0;  // 1=输入, 0=输出
    config_reg.ports.port1.byte = 0xFF;  // 端口1全部设为输入
    
    ret = tca9535_write_config(tca9535_handle, &config_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写入配置失败");
    }
    
    // 设置输出引脚的初始状态
    output_reg.ports.port0.byte = 0x0F;  // 低4位全部输出高电平
    output_reg.ports.port1.byte = 0x00;  // 端口1为输入，输出寄存器无意义
    
    ret = tca9535_write_output(tca9535_handle, &output_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "写入输出失败");
    }
    
    // 5. 示例3：读取输入状态
    while (1) {
        tca9535_register_t input_reg;
        ret = tca9535_read_input(tca9535_handle, &input_reg);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "输入状态 - 端口0: 0x%02X, 端口1: 0x%02X", 
                     input_reg.ports.port0.byte, input_reg.ports.port1.byte);
        }
        
        // 读取单个引脚状态
        uint8_t pin_level;
        ret = tca9535_get_pin_level(tca9535_handle, 8, &pin_level);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "引脚8状态: %d", pin_level);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // 6. 清理资源
    tca9535_delete(tca9535_handle);
    vTaskDelete(NULL);
}

void app_main(void)
{
    // 创建TCA9535示例任务
    xTaskCreate(tca9535_example_task, "tca9535_example", 4096, NULL, 5, NULL);
}
