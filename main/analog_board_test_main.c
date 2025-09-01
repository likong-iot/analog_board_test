#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

// Shell组件头文件
#include "sd.h"
#include "shell.h"
#include "uart_driver.h"

// I2C和TCA9535头文件
#include "i2c_config.h"
#include "tca9535.h"

// LED控制头文件
#include "led.h"
#include "led_commands.h"

static const char *TAG = "MAIN";

// Shell实例指针
static shell_instance_t *uart1_shell = NULL;
static shell_instance_t *uart2_shell = NULL;

// TCA9535设备句柄
static tca9535_handle_t tca9535_handle = NULL;



void app_main(void) {
  // 初始化NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "=== ESP32 模拟板测试系统启动 ===");

  // 初始化LED模块
  ret = led_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "LED模块初始化失败: %s", esp_err_to_name(ret));
    return;
  }

  // 初始化UART驱动
  if (!uart_driver_init()) {
    ESP_LOGE(TAG, "UART驱动初始化失败");
    return;
  }

  // 初始化I2C总线配置
  ESP_LOGI(TAG, "配置I2C总线...");
  ret = i2c_master_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C总线配置失败: %s", esp_err_to_name(ret));
    return;
  }

  // 初始化TCA9535 I/O扩展器
  ESP_LOGI(TAG, "初始化TCA9535 I/O扩展器...");
  tca9535_config_t tca9535_config = {
      .i2c_dev = {.port = I2C_MASTER_NUM,
                  .addr = TCA9535_I2C_ADDR,
                  .cfg = {.sda_io_num = I2C_MASTER_SDA_IO,
                          .scl_io_num = I2C_MASTER_SCL_IO,
                          .sda_pullup_en = true,
                          .scl_pullup_en = true,
                          .master.clk_speed = I2C_MASTER_FREQ_HZ}}};

  ret = tca9535_create(&tca9535_config, &tca9535_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "TCA9535设备创建失败: %s", esp_err_to_name(ret));
    ESP_LOGW(TAG, "系统将继续运行，但TCA9535功能不可用");
    tca9535_handle = NULL;
  } else {
    // 测试TCA9535连接
    tca9535_register_t test_reg;
    ret = tca9535_read_input(tca9535_handle, &test_reg);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "TCA9535初始化成功 (地址: 0x%02X)", TCA9535_I2C_ADDR);
      ESP_LOGI(TAG, "TCA9535输入状态 - P0: 0x%02X, P1: 0x%02X",
               test_reg.ports.port0.byte, test_reg.ports.port1.byte);
    } else {
      ESP_LOGW(TAG, "TCA9535通信测试失败: %s", esp_err_to_name(ret));
      ESP_LOGW(TAG, "系统将继续运行，但TCA9535功能不可用");
      tca9535_delete(tca9535_handle);
      tca9535_handle = NULL;
    }
  }

  // 初始化ADS1115 ADC
  ESP_LOGI(TAG, "初始化ADS1115 ADC...");
  ret = ads1115_init();
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "ADS1115初始化成功");
    // 验证配置
    ads1115_config_info_t config_info;
    if (ads1115_get_config_info(&config_info) == ESP_OK) {
      ESP_LOGI(TAG, "ADS1115配置 - 增益: %s, 速率: %d SPS, 模式: %s", 
               config_info.gain_str, config_info.rate_sps, config_info.mode_str);
    }
  } else {
    ESP_LOGW(TAG, "ADS1115初始化失败: %s", esp_err_to_name(ret));
    ESP_LOGW(TAG, "系统将继续运行，但ADS1115功能不可用");
  }

  // 初始化SD卡
  esp_err_t sd_ret = sd_card_init();
  if (sd_ret == ESP_OK) {
    ESP_LOGI(TAG, "SD卡初始化成功");
    // 执行SD卡基本测试
    sd_card_test_basic();
  } else {
    ESP_LOGW(TAG, "SD卡初始化失败: %s", esp_err_to_name(sd_ret));
    ESP_LOGW(TAG, "系统将继续运行，但SD卡功能不可用");
  }

  // 初始化Shell系统（包含命令注册）
  shell_system_init();
  
  // 注册LED控制命令
  cmd_register_task("led", task_led_control, "控制LED (on/off/toggle/blink)");

  // 创建UART1的Shell实例
  shell_config_t uart1_config =
      create_shell_config(1,                // 通道ID
                          "UART1",          // 通道名称
                          "UART1> ",        // 提示符
                          uart1_output_func // 输出函数
      );

  uart1_shell = shell_create_and_start(&uart1_config);
  if (uart1_shell == NULL) {
    ESP_LOGE(TAG, "UART1 Shell实例创建失败");
    return;
  }

  // 创建UART2的Shell实例
  shell_config_t uart2_config =
      create_shell_config(2,                // 通道ID
                          "UART2",          // 通道名称
                          "UART2> ",        // 提示符
                          uart2_output_func // 输出函数
      );

  uart2_shell = shell_create_and_start(&uart2_config);
  if (uart2_shell == NULL) {
    ESP_LOGE(TAG, "UART2 Shell实例创建失败");
    return;
  }

  // 创建UART接收任务
  xTaskCreate(uart1_rx_task, "uart1_rx", 4096, NULL, 5, NULL);
  xTaskCreate(uart2_rx_task, "uart2_rx", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG, "=== 系统初始化完成 ===");
  ESP_LOGI(TAG, "UART1 Shell: 通道ID=1, 引脚 TX=%d/RX=%d", 22, 23);
  ESP_LOGI(TAG, "UART2 Shell: 通道ID=2, 引脚 TX=%d/RX=%d", 27, 26);
  ESP_LOGI(TAG, "波特率: 115200");
  ESP_LOGI(TAG, "I2C总线: SCL=GPIO%d, SDA=GPIO%d", I2C_MASTER_SCL_IO,
           I2C_MASTER_SDA_IO);
  ESP_LOGI(TAG, "LED状态: LED1=GPIO%d, LED2=GPIO%d, LED3=GPIO%d, LED4=GPIO%d", 
           LED1_GPIO, LED2_GPIO, LED3_GPIO, LED4_GPIO);
  ESP_LOGI(TAG, "SD卡状态: %s", sd_card_is_mounted() ? "已挂载" : "未挂载");
  ESP_LOGI(TAG, "TCA9535状态: %s", tca9535_handle ? "已连接" : "未连接");
  ESP_LOGI(TAG, "ADS1115状态: %s", ads1115_get_handle() ? "已连接" : "未连接");
  ESP_LOGI(TAG, "可用命令: help, echo, version, kv, tasks, heap等");

  // LED启动指示：快速闪烁3次
  ESP_LOGI(TAG, "系统启动完成，LED指示...");
  led_blink(LED_ALL, 3, 200);
  
  static uint32_t loop_count = 0;
  
  while (1) {
    loop_count++;
    
    // 读取ADS1115所有通道数据
    if (ads1115_get_handle() != NULL) {
      ads1115_channel_data_t channel_data[ADS1115_CHANNEL_COUNT];
      if (ads1115_read_all_detailed(channel_data) == ESP_OK) {
        ESP_LOGI(TAG, "=== ADS1115电流监测 (循环%lu) ===", loop_count);
        for (uint8_t ch = 0; ch < ADS1115_CHANNEL_COUNT; ch++) {
          if (channel_data[ch].status == ESP_OK) {
            ESP_LOGI(TAG, "CH%d: %.2fmA (%.4fV, 原始值=%d)", 
                     ch, channel_data[ch].current_ma, channel_data[ch].voltage_v, channel_data[ch].raw_value);
          } else {
            ESP_LOGE(TAG, "CH%d: 读取失败 - %s", ch, esp_err_to_name(channel_data[ch].status));
          }
        }
        ESP_LOGI(TAG, "========================");
      }
    }
    
    // 每10个循环（20秒）LED1闪烁一次作为心跳指示
    if (loop_count % 10 == 0) {
      led_blink(LED_1, 1, 100);
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));  // 每2秒读取一次，方便观察
  }
}