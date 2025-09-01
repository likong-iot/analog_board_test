#ifndef __CMD_SYSTEM_H__
#define __CMD_SYSTEM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 显示系统状态
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_status(uint32_t channel_id, const char *params);

/**
 * @brief 显示所有任务信息
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_tasks(uint32_t channel_id, const char *params);

/**
 * @brief 显示内存使用情况
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_heap(uint32_t channel_id, const char *params);

/**
 * @brief 显示系统运行时间
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_uptime(uint32_t channel_id, const char *params);

/**
 * @brief 显示CPU使用率
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_cpu(uint32_t channel_id, const char *params);

/**
 * @brief 控制LED
 * @param channel_id 通信通道ID
 * @param params 参数 (on/off)
 */
void task_led(uint32_t channel_id, const char *params);

/**
 * @brief 重启系统
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_reset(uint32_t channel_id, const char *params);

/**
 * @brief 延时指定毫秒数
 * @param channel_id 通信通道ID
 * @param params 参数 (毫秒数)
 */
void task_delay(uint32_t channel_id, const char *params);

#ifdef __cplusplus
}
#endif

#endif // __CMD_SYSTEM_H__ 