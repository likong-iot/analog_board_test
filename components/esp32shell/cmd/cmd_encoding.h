/**
 * @file cmd_encoding.h
 * @brief ESP32Shell编码配置命令头文件
 */

#ifndef CMD_ENCODING_H
#define CMD_ENCODING_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 支持的编码类型 */
typedef enum {
    SHELL_ENCODING_UTF8 = 0,                /*!< UTF-8编码 (默认) */
    SHELL_ENCODING_GB2312 = 1               /*!< GB2312编码 */
} shell_encoding_type_t;

/**
 * @brief 初始化Shell编码配置
 * 
 * @return esp_err_t 
 */
esp_err_t shell_encoding_init(void);

/**
 * @brief 设置Shell输出编码类型
 * 
 * @param encoding 编码类型
 * @return esp_err_t 
 */
esp_err_t shell_encoding_set_type(shell_encoding_type_t encoding);

/**
 * @brief 获取当前Shell编码类型
 * 
 * @return shell_encoding_type_t 当前编码类型
 */
shell_encoding_type_t shell_encoding_get_type(void);

/**
 * @brief 获取Shell编码类型字符串
 * 
 * @param encoding 编码类型
 * @return const char* 编码名称
 */
const char* shell_encoding_get_name(shell_encoding_type_t encoding);

/**
 * @brief Shell安全的字符串输出函数（自动编码转换）
 * 
 * @param dest 目标缓冲区
 * @param dest_size 目标缓冲区大小
 * @param format 格式字符串
 * @param ... 参数
 * @return int 写入字节数
 */
int shell_snprintf(char *dest, size_t dest_size, const char *format, ...);

/**
 * @brief Shell编码配置命令处理函数
 * 
 * @param channel_id Shell通道ID
 * @param params 参数字符串
 */
void task_shell_encoding(uint32_t channel_id, const char *params);

#ifdef __cplusplus
}
#endif

#endif /* CMD_ENCODING_H */
