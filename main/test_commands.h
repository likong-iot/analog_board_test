/**
 * @file test_commands.h
 * @brief 测试命令模块头文件
 * 
 * 实现自动化测试功能，包括：
 * - ADS1115数据记录到SD卡
 * - TCA9535 IO循环控制
 * - LED循环点亮
 * - 测试日志查看和控制
 */

#ifndef TEST_COMMANDS_H
#define TEST_COMMANDS_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 测试配置常量 */
#define TEST_LOG_FILE_PATH      "/sdcard/testlog.txt"    /*!< 测试日志文件路径 */
#define TEST_CYCLE_INTERVAL_MS  500                      /*!< 测试循环间隔(毫秒) */
#define TEST_IO_COUNT           8                        /*!< TCA9535 IO数量(显示为1-8) */
#define TEST_LED_COUNT          4                        /*!< LED数量(1-4) */

/* 测试状态结构体 */
typedef struct {
    bool running;                                        /*!< 测试是否正在运行 */
    uint32_t cycle_count;                               /*!< 循环计数 */
    uint8_t current_io;                                 /*!< 当前拉高的IO号(内部0-7，显示1-8) */
    uint8_t current_led;                                /*!< 当前点亮的LED号(1-4) */
    uint32_t start_time_ms;                             /*!< 测试开始时间(毫秒) */
} test_status_t;

/**
 * @brief 测试命令处理函数
 * 
 * 支持的命令：
 * - test start    - 开始自动化测试
 * - test stop     - 停止自动化测试
 * - test status   - 显示测试状态
 * 
 * @param channel_id 通道ID
 * @param params 命令参数
 */
void task_test_control(uint32_t channel_id, const char *params);

/**
 * @brief testoff命令处理函数
 * 
 * 停止自动化测试
 * 
 * @param channel_id 通道ID
 * @param params 命令参数
 */
void task_testoff_control(uint32_t channel_id, const char *params);

/**
 * @brief 获取当前测试状态
 * 
 * @return 指向测试状态结构体的指针
 */
const test_status_t* test_get_status(void);

/**
 * @brief 初始化测试模块
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_FAIL: 初始化失败
 */
esp_err_t test_module_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_COMMANDS_H */
