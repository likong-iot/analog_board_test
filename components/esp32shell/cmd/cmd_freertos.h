#ifndef __CMD_FREERTOS_H__
#define __CMD_FREERTOS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 队列操作命令
 * @param channel_id 通信通道ID
 * @param params 参数 (create|send|receive)
 */
void task_queue(uint32_t channel_id, const char *params);

/**
 * @brief 信号量操作命令
 * @param channel_id 通信通道ID
 * @param params 参数 (create|take|give)
 */
void task_sem(uint32_t channel_id, const char *params);

/**
 * @brief 定时器操作命令
 * @param channel_id 通信通道ID
 * @param params 参数 (create|start|stop)
 */
void task_timer(uint32_t channel_id, const char *params);

#ifdef __cplusplus
}
#endif

#endif // __CMD_FREERTOS_H__ 