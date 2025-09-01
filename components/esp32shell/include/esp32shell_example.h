#ifndef __ESP32SHELL_EXAMPLE_H__
#define __ESP32SHELL_EXAMPLE_H__

#include "shell.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 示例：如何创建和管理多个shell实例
 * 
 * 这个示例展示了如何：
 * 1. 创建shell配置结构体
 * 2. 启动多个shell实例
 * 3. 动态管理shell实例
 */

// 示例I/O函数实现
void example_output_func(uint8_t uart_num, const uint8_t *data, size_t length);
void* example_get_buffer_func(uint8_t uart_num);
void example_add_data_func(uint8_t uart_num, const uint8_t *data, size_t length);

// 创建shell配置的便捷函数
shell_config_t create_shell_config_example(uint8_t uart_num, const char *prompt);

// 示例：创建多个shell实例
void example_create_multiple_shells(void);

// 示例：动态管理shell实例
void example_manage_shell_instances(void);

#ifdef __cplusplus
}
#endif

#endif // __ESP32SHELL_EXAMPLE_H__ 