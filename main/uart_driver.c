#include "uart_driver.h"
#include "shell.h"
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "UART_DRIVER";

bool uart_driver_init(void) {
    esp_err_t ret;
    
    // 配置UART1
    uart_config_t uart1_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 安装UART1驱动
    ret = uart_driver_install(UART_NUM_1, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART1驱动安装失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 配置UART1参数
    ret = uart_param_config(UART_NUM_1, &uart1_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART1参数配置失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 设置UART1引脚
    ret = uart_set_pin(UART_NUM_1, UART1_TXD_PIN, UART1_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART1引脚设置失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 配置UART2
    uart_config_t uart2_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 安装UART2驱动
    ret = uart_driver_install(UART_NUM_2, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART2驱动安装失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 配置UART2参数
    ret = uart_param_config(UART_NUM_2, &uart2_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART2参数配置失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 设置UART2引脚
    ret = uart_set_pin(UART_NUM_2, UART2_TXD_PIN, UART2_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART2引脚设置失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "UART驱动初始化成功");
    ESP_LOGI(TAG, "UART1: TX=%d, RX=%d, 波特率=%d", UART1_TXD_PIN, UART1_RXD_PIN, UART_BAUD_RATE);
    ESP_LOGI(TAG, "UART2: TX=%d, RX=%d, 波特率=%d", UART2_TXD_PIN, UART2_RXD_PIN, UART_BAUD_RATE);
    
    return true;
}

int uart_send_data(uart_port_t uart_num, const uint8_t *data, size_t length) {
    return uart_write_bytes(uart_num, data, length);
}

int uart_receive_data(uart_port_t uart_num, uint8_t *data, size_t max_length, uint32_t timeout_ms) {
    return uart_read_bytes(uart_num, data, max_length, pdMS_TO_TICKS(timeout_ms));
}

void uart1_rx_task(void *arg) {
    uint8_t data[UART_BUF_SIZE];
    int length;
    
    ESP_LOGI(TAG, "UART1接收任务启动");
    
    while (1) {
        // 从UART1读取数据
        length = uart_receive_data(UART_NUM_1, data, UART_BUF_SIZE - 1, 100);
        
        if (length > 0) {
            // 将接收到的数据发送到Shell实例
            shell_add_data_to_instance(1, data, length);
            
            // 记录接收到的数据（调试用）
            data[length] = '\0';
            ESP_LOGI(TAG, "UART1接收到 %d 字节: %s", length, data);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void uart2_rx_task(void *arg) {
    uint8_t data[UART_BUF_SIZE];
    int length;
    
    ESP_LOGI(TAG, "UART2接收任务启动");
    
    while (1) {
        // 从UART2读取数据
        length = uart_receive_data(UART_NUM_2, data, UART_BUF_SIZE - 1, 100);
        
        if (length > 0) {
            // 将接收到的数据发送到Shell实例
            shell_add_data_to_instance(2, data, length);
            
            // 记录接收到的数据（调试用）
            data[length] = '\0';
            ESP_LOGI(TAG, "UART2接收到 %d 字节: %s", length, data);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void uart1_output_func(uint32_t channel_id, const uint8_t *data, size_t length) {
    // 通过UART1发送数据
    uart_send_data(UART_NUM_1, data, length);
}

void uart2_output_func(uint32_t channel_id, const uint8_t *data, size_t length) {
    // 通过UART2发送数据
    uart_send_data(UART_NUM_2, data, length);
}
