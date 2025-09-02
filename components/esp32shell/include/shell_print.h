/**
 * @file shell_print.h
 * @brief Shell统一打印函数头文件
 * 
 * 提供统一的打印接口，自动处理编码转换
 */

#ifndef SHELL_PRINT_H
#define SHELL_PRINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include "shell_encoding.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化shell打印系统
 * 
 * @param encoding_config 编码配置，NULL使用默认配置
 * @return true 成功, false 失败
 */
bool shell_print_init(const shell_encoding_config_t *encoding_config);

/**
 * @brief 反初始化shell打印系统
 */
void shell_print_deinit(void);

/**
 * @brief 统一的shell打印函数（自动编码转换）
 * 
 * @param channel_id 通道ID
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 实际打印的字符数
 */
int shell_print(uint32_t channel_id, const char *format, ...);

/**
 * @brief 统一的shell打印函数（va_list版本）
 * 
 * @param channel_id 通道ID
 * @param format 格式化字符串
 * @param args 可变参数列表
 * @return 实际打印的字符数
 */
int shell_vprint(uint32_t channel_id, const char *format, va_list args);

/**
 * @brief 带编码指定的shell打印函数
 * 
 * @param channel_id 通道ID
 * @param target_encoding 目标编码
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 实际打印的字符数
 */
int shell_print_with_encoding(uint32_t channel_id, shell_encoding_type_t target_encoding, const char *format, ...);

/**
 * @brief 带编码指定的shell打印函数（va_list版本）
 * 
 * @param channel_id 通道ID
 * @param target_encoding 目标编码
 * @param format 格式化字符串
 * @param args 可变参数列表
 * @return 实际打印的字符数
 */
int shell_vprint_with_encoding(uint32_t channel_id, shell_encoding_type_t target_encoding, const char *format, va_list args);

/**
 * @brief 打印原始数据（不进行编码转换）
 * 
 * @param channel_id 通道ID
 * @param data 数据
 * @param length 数据长度
 * @return 实际打印的字符数
 */
int shell_print_raw(uint32_t channel_id, const uint8_t *data, size_t length);

/**
 * @brief 打印字符串（自动编码转换）
 * 
 * @param channel_id 通道ID
 * @param str 字符串
 * @return 实际打印的字符数
 */
int shell_print_string(uint32_t channel_id, const char *str);

/**
 * @brief 打印带换行的字符串
 * 
 * @param channel_id 通道ID
 * @param str 字符串
 * @return 实际打印的字符数
 */
int shell_print_line(uint32_t channel_id, const char *str);

/**
 * @brief 打印格式化字符串到缓冲区（不输出到终端）
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param format 格式化字符串
 * @param ... 可变参数
 * @return 实际写入的字符数
 */
int shell_snprintf(char *buffer, size_t buffer_size, const char *format, ...);

/**
 * @brief 打印格式化字符串到缓冲区（va_list版本）
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param format 格式化字符串
 * @param args 可变参数列表
 * @return 实际写入的字符数
 */
int shell_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list args);

/**
 * @brief 设置shell打印的编码类型
 * 
 * @param encoding_type 编码类型
 * @return true 成功, false 失败
 */
bool shell_print_set_encoding(shell_encoding_type_t encoding_type);

/**
 * @brief 获取shell打印的编码类型
 * 
 * @return 当前编码类型
 */
shell_encoding_type_t shell_print_get_encoding(void);

/**
 * @brief 检查shell打印系统是否已初始化
 * 
 * @return true 已初始化, false 未初始化
 */
bool shell_print_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_PRINT_H */
