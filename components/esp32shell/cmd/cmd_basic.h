#ifndef __CMD_BASIC_H__
#define __CMD_BASIC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 显示帮助信息
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_help(uint32_t channel_id, const char *params);

/**
 * @brief 回显文本
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_echo(uint32_t channel_id, const char *params);



/**
 * @brief 显示版本信息
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_version(uint32_t channel_id, const char *params);

/**
 * @brief 清屏
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_clear(uint32_t channel_id, const char *params);

/**
 * @brief 参数测试命令
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_test(uint32_t channel_id, const char *params);

/**
 * @brief 键值存储命令
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_kv(uint32_t channel_id, const char *params);

/**
 * @brief 显示命令缓冲区
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_buffer(uint32_t channel_id, const char *params);

#ifdef __cplusplus
}
#endif

#endif // __CMD_BASIC_H__ 