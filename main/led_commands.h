/**
 * @file led_commands.h
 * @brief LED命令处理函数头文件
 */

#ifndef LED_COMMANDS_H
#define LED_COMMANDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED控制命令处理函数
 * 
 * 支持的命令：
 * - led <1-4> on/off      - 控制单个LED
 * - led all on/off        - 控制所有LED
 * - led <1-4> toggle      - 切换单个LED状态
 * - led all toggle        - 切换所有LED状态
 * - led <1-4> blink <次数> [间隔ms] - LED闪烁
 * - led status            - 显示所有LED状态
 * 
 * @param channel_id 通道ID
 * @param params 命令参数
 */
void task_led_control(uint32_t channel_id, const char *params);

#ifdef __cplusplus
}
#endif

#endif /* LED_COMMANDS_H */
