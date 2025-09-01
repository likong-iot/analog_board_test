/**
 * @file test_commands.c
 * @brief 测试命令模块实现
 */

#include "test_commands.h"
#include "led.h"
#include "i2c_config.h"
#include "tca9535.h"
#include "sd.h"
#include "key.h"
#include "shell.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

static const char *TAG = "TEST_CMD";

// 全局测试状态
static test_status_t g_test_status = {0};
static TaskHandle_t test_task_handle = NULL;
static SemaphoreHandle_t test_mutex = NULL;
static uint32_t test_channel_id = 0; // 保存Shell通道ID用于打印

// TCA9535句柄获取函数（在main中实现）
extern tca9535_handle_t get_tca9535_handle(void);

/**
 * @brief 按键事件回调函数
 */
static void key_event_handler(key_event_t event, uint32_t timestamp_ms)
{
    if (!g_test_status.running || test_channel_id == 0) {
        return; // 测试未运行或没有Shell通道，忽略按键事件
    }
    
    char output[128];
    const char *event_str = (event == KEY_EVENT_PRESSED) ? "按下" : "松开";
    
    // 在Shell中打印按键事件
    snprintf(output, sizeof(output), "\r\n>>> 按键%s (时间戳: %lu ms) <<<\r\n", event_str, timestamp_ms);
    cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
    
    // 记录到日志文件
    if (sd_card_is_mounted()) {
        FILE *file = fopen(TEST_LOG_FILE_PATH, "a");
        if (file != NULL) {
            fprintf(file, "KEY_EVENT,%lu,%s,,,,,,,,,,,\n", timestamp_ms, event_str);
            fclose(file);
        }
    }
    
    ESP_LOGI(TAG, "按键%s事件已处理 (时间戳: %lu)", event_str, timestamp_ms);
}

/**
 * @brief 写入测试数据到SD卡
 */
static esp_err_t write_test_data_to_sd(const ads1115_channel_data_t *channel_data)
{
    if (!sd_card_is_mounted()) {
        ESP_LOGE(TAG, "SD卡未挂载");
        return ESP_ERR_INVALID_STATE;
    }
    
    FILE *file = fopen(TEST_LOG_FILE_PATH, "a");
    if (file == NULL) {
        ESP_LOGE(TAG, "无法打开测试日志文件");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 获取当前时间戳
    uint32_t timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 写入时间戳、循环计数、IO状态、LED状态
    // 显示实际拉高的IO和点亮的LED（因为在写入时已经切换到下一个了）
    uint8_t actual_io = (g_test_status.current_io == 0) ? 8 : g_test_status.current_io; // 显示1-8
    uint8_t actual_led = (g_test_status.current_led == 1) ? 4 : g_test_status.current_led - 1;
    
    fprintf(file, "%lu,%lu,%d,%d,", 
            timestamp_ms, g_test_status.cycle_count, 
            actual_io, actual_led);
    
    // 写入4个通道的电压和电流数据，包含单位
    for (uint8_t ch = 0; ch < ADS1115_CHANNEL_COUNT; ch++) {
        if (channel_data[ch].status == ESP_OK) {
            fprintf(file, "%.4fV,%.2fmA", channel_data[ch].voltage_v, channel_data[ch].current_ma);
        } else {
            fprintf(file, "ERROR,ERROR");
        }
        if (ch < ADS1115_CHANNEL_COUNT - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "\n");
    
    fclose(file);
    return ESP_OK;
}

/**
 * @brief 测试任务主循环
 */
static void test_task_main(void *arg)
{
    ESP_LOGI(TAG, "测试任务启动 - 终端将持续打印测试数据");
    
    while (g_test_status.running) {
        if (xSemaphoreTake(test_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_test_status.cycle_count++;
            
            // 1. 读取ADS1115数据
            ads1115_channel_data_t channel_data[ADS1115_CHANNEL_COUNT];
            if (ads1115_get_handle() != NULL) {
                if (ads1115_read_all_detailed(channel_data) == ESP_OK) {
                    // 写入数据到SD卡
                    write_test_data_to_sd(channel_data);
                }
            }
            
            // 2. 控制TCA9535 IO (循环拉高0-7)
            tca9535_handle_t tca_handle = get_tca9535_handle();
            if (tca_handle != NULL) {
                // 先将所有IO设为低电平
                tca9535_register_t output_reg = {0};
                tca9535_write_output(tca_handle, &output_reg);
                
                // 拉高当前IO
                if (g_test_status.current_io < 8) {
                    output_reg.ports.port0.byte = (1 << g_test_status.current_io);
                    tca9535_write_output(tca_handle, &output_reg);
                }
                
                // 切换到下一个IO
                g_test_status.current_io = (g_test_status.current_io + 1) % TEST_IO_COUNT;
            }
            
            // 3. 控制LED循环点亮
            // 先关闭所有LED
            led_set_all_state(LED_OFF);
            // 点亮当前LED
            led_set_state((led_num_t)g_test_status.current_led, LED_ON);
            // 切换到下一个LED
            g_test_status.current_led = (g_test_status.current_led % TEST_LED_COUNT) + 1;
            
            // 4. 持续打印测试数据到Shell终端
            if (test_channel_id > 0) {
                char output[512];
                uint8_t display_io = (g_test_status.current_io == 0) ? 8 : g_test_status.current_io; // 显示1-8
                uint8_t display_led = (g_test_status.current_led == 1) ? 4 : g_test_status.current_led - 1; // 显示实际点亮的LED
                
                snprintf(output, sizeof(output), "\r\n=== 测试循环 %lu ===\r\n", g_test_status.cycle_count);
                cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
                
                snprintf(output, sizeof(output), "当前拉高IO: %d | 当前点亮LED: %d\r\n", display_io, display_led);
                cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
                
                // 打印ADS1115数据到Shell终端
                if (ads1115_get_handle() != NULL) {
                    snprintf(output, sizeof(output), "ADS1115数据: ");
                    for (uint8_t ch = 0; ch < ADS1115_CHANNEL_COUNT; ch++) {
                        char ch_data[64];
                        if (channel_data[ch].status == ESP_OK) {
                            snprintf(ch_data, sizeof(ch_data), "CH%d:%.4fV,%.2fmA ", 
                                   ch, channel_data[ch].voltage_v, channel_data[ch].current_ma);
                        } else {
                            snprintf(ch_data, sizeof(ch_data), "CH%d:ERROR ", ch);
                        }
                        strncat(output, ch_data, sizeof(output) - strlen(output) - 1);
                    }
                    strncat(output, "\r\n", sizeof(output) - strlen(output) - 1);
                    cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
                } else {
                    snprintf(output, sizeof(output), "ADS1115: 未连接\r\n");
                    cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
                }
                
                snprintf(output, sizeof(output), "==================\r\n");
                cmd_output(test_channel_id, (uint8_t *)output, strlen(output));
            }
            
            xSemaphoreGive(test_mutex);
        }
        
        // 等待指定间隔
        vTaskDelay(pdMS_TO_TICKS(TEST_CYCLE_INTERVAL_MS));
    }
    
    // 测试结束，关闭所有LED和IO
    led_set_all_state(LED_OFF);
    tca9535_handle_t tca_handle = get_tca9535_handle();
    if (tca_handle != NULL) {
        tca9535_register_t output_reg = {0};
        tca9535_write_output(tca_handle, &output_reg);
    }
    
    ESP_LOGI(TAG, "测试任务结束");
    test_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t test_module_init(void)
{
    test_mutex = xSemaphoreCreateMutex();
    if (test_mutex == NULL) {
        ESP_LOGE(TAG, "创建测试互斥锁失败");
        return ESP_FAIL;
    }
    
    // 初始化测试状态
    memset(&g_test_status, 0, sizeof(test_status_t));
    g_test_status.current_led = 1;  // 从LED1开始
    
    ESP_LOGI(TAG, "测试模块初始化成功");
    return ESP_OK;
}

const test_status_t* test_get_status(void)
{
    return &g_test_status;
}

void task_test_control(uint32_t channel_id, const char *params)
{
    char response[512];
    
    // test命令直接开始测试，无需参数
    if (strlen(params) == 0) {
        if (g_test_status.running) {
            snprintf(response, sizeof(response), "测试已在运行中，使用 'testoff' 停止测试\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        
        // 检查必要的组件是否可用
        if (!sd_card_is_mounted()) {
            snprintf(response, sizeof(response), "错误: SD卡未挂载，无法记录日志\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        
        if (ads1115_get_handle() == NULL) {
            snprintf(response, sizeof(response), "警告: ADS1115未连接，将跳过数据记录\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
        
        if (get_tca9535_handle() == NULL) {
            snprintf(response, sizeof(response), "警告: TCA9535未连接，将跳过IO控制\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
        
        // 检查是否已存在日志文件，如果存在则添加分界线
        struct stat st;
        if (stat(TEST_LOG_FILE_PATH, &st) == 0) {
            FILE *file = fopen(TEST_LOG_FILE_PATH, "a");
            if (file != NULL) {
                fprintf(file, "\n=== 新测试会话开始 ===\n");
                fprintf(file, "时间戳(ms),循环计数,拉高IO号(1-8),点亮LED号(1-4),CH0电压(V),CH0电流(mA),CH1电压(V),CH1电流(mA),CH2电压(V),CH2电流(mA),CH3电压(V),CH3电流(mA)\n");
                fclose(file);
            }
        } else {
            // 创建新文件并写入表头
            FILE *file = fopen(TEST_LOG_FILE_PATH, "w");
            if (file != NULL) {
                fprintf(file, "=== ESP32模拟板测试日志 ===\n");
                fprintf(file, "时间戳(ms),循环计数,拉高IO号(1-8),点亮LED号(1-4),CH0电压(V),CH0电流(mA),CH1电压(V),CH1电流(mA),CH2电压(V),CH2电流(mA),CH3电压(V),CH3电流(mA)\n");
                fclose(file);
            }
        }
        
        // 启动测试
        g_test_status.running = true;
        g_test_status.cycle_count = 0;
        g_test_status.current_io = 0;
        g_test_status.current_led = 1;
        g_test_status.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        test_channel_id = channel_id; // 保存Shell通道ID
        
        // 设置按键事件回调并启动按键检测
        key_set_event_callback(key_event_handler);
        key_start_detection();
        
        // 创建测试任务
        BaseType_t ret = xTaskCreate(test_task_main, "test_task", 4096, NULL, 5, &test_task_handle);
        if (ret == pdPASS) {
            snprintf(response, sizeof(response), 
                    "=== 自动化测试启动 ===\r\n"
                    "功能:\r\n"
                    "- ADS1115数据记录到SD卡\r\n"
                    "- TCA9535 IO1-8循环拉高\r\n"
                    "- LED1-4循环点亮\r\n"
                    "- 循环间隔: %dms\r\n"
                    "- Shell终端持续打印测试数据\r\n"
                    "- 按键检测(GPIO35)和事件记录\r\n"
                    "\r\n"
                    "使用 'testoff' 停止测试\r\n"
                    "Shell将开始持续显示测试数据...\r\n"
                    "========================\r\n", TEST_CYCLE_INTERVAL_MS);
            ESP_LOGI(TAG, "自动化测试启动成功 - 终端将持续打印数据");
        } else {
            g_test_status.running = false;
            snprintf(response, sizeof(response), "错误: 无法创建测试任务\r\n");
            ESP_LOGE(TAG, "创建测试任务失败");
        }
    } else {
        snprintf(response, sizeof(response), 
                "test命令用法:\r\n"
                "test      - 开始自动化测试\r\n"
                "testoff   - 停止自动化测试\r\n"
                "\r\n"
                "测试功能:\r\n"
                "- ADS1115数据记录到SD卡\r\n"
                "- TCA9535 IO1-8循环拉高\r\n"
                "- LED1-4循环点亮\r\n"
                "- 循环间隔: %dms\r\n"
                "- Shell终端持续打印测试数据\r\n"
                "- 按键检测(GPIO35)和事件记录\r\n", TEST_CYCLE_INTERVAL_MS);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_testoff_control(uint32_t channel_id, const char *params)
{
    char response[512];
    
    if (!g_test_status.running) {
        snprintf(response, sizeof(response), "测试未在运行\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 停止测试
    g_test_status.running = false;
    test_channel_id = 0; // 清除Shell通道ID
    
    // 停止按键检测
    key_stop_detection();
    key_set_event_callback(NULL);
    
    // 等待测试任务结束
    if (test_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(600)); // 等待超过一个循环周期
    }
    
    // 写入结束标记到日志文件
    if (sd_card_is_mounted()) {
        FILE *file = fopen(TEST_LOG_FILE_PATH, "a");
        if (file != NULL) {
            uint32_t end_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            uint32_t duration_ms = end_time_ms - g_test_status.start_time_ms;
            fprintf(file, "\n=== 测试会话结束 ===\n");
            fprintf(file, "总循环次数: %lu\n", g_test_status.cycle_count);
            fprintf(file, "测试时长: %lu ms (%.1f秒)\n", duration_ms, duration_ms / 1000.0f);
            fprintf(file, "===================\n\n");
            fclose(file);
        }
    }
    
    snprintf(response, sizeof(response), 
            "=== 测试已停止 ===\r\n"
            "总循环次数: %lu\r\n"
            "测试时长: %.1f秒\r\n"
            "Shell终端打印已停止\r\n"
            "==================\r\n",
            g_test_status.cycle_count,
            (xTaskGetTickCount() * portTICK_PERIOD_MS - g_test_status.start_time_ms) / 1000.0f);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    ESP_LOGI(TAG, "自动化测试停止 - Shell终端打印已停止");
}