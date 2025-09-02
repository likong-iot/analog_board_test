/**
 * @file shell_encoding.h
 * @brief Shell编码系统头文件
 * 
 * 提供统一的编码管理，支持多种编码格式的自动转换
 */

#ifndef SHELL_ENCODING_H
#define SHELL_ENCODING_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 支持的编码类型
 */
typedef enum {
    SHELL_ENCODING_UTF8 = 0,        /*!< UTF-8编码 */
    SHELL_ENCODING_GB2312 = 1,      /*!< GB2312编码 */
    SHELL_ENCODING_GBK = 2,         /*!< GBK编码 */
    SHELL_ENCODING_ASCII = 3,       /*!< ASCII编码 */
    SHELL_ENCODING_MAX              /*!< 编码类型数量 */
} shell_encoding_type_t;

/**
 * @brief 编码配置结构
 */
typedef struct {
    shell_encoding_type_t type;     /*!< 编码类型 */
    bool auto_detect;               /*!< 是否自动检测编码 */
    bool fallback_to_ascii;         /*!< 转换失败时是否回退到ASCII */
    size_t max_conversion_size;     /*!< 最大转换缓冲区大小 */
} shell_encoding_config_t;

/**
 * @brief 编码转换结果
 */
typedef struct {
    uint8_t *data;                 /*!< 转换后的数据 */
    size_t length;                  /*!< 数据长度 */
    bool success;                   /*!< 转换是否成功 */
    shell_encoding_type_t source;   /*!< 源编码类型 */
    shell_encoding_type_t target;   /*!< 目标编码类型 */
} shell_encoding_result_t;

/**
 * @brief 初始化编码系统
 * 
 * @param config 编码配置
 * @return true 成功, false 失败
 */
bool shell_encoding_init(const shell_encoding_config_t *config);

/**
 * @brief 反初始化编码系统
 */
void shell_encoding_deinit(void);

/**
 * @brief 设置全局编码类型
 * 
 * @param encoding_type 编码类型
 * @return true 成功, false 失败
 */
bool shell_encoding_set_global(shell_encoding_type_t encoding_type);

/**
 * @brief 获取全局编码类型
 * 
 * @return 当前编码类型
 */
shell_encoding_type_t shell_encoding_get_global(void);

/**
 * @brief 获取编码配置
 * 
 * @param config 输出配置结构
 * @return true 成功, false 失败
 */
bool shell_encoding_get_config(shell_encoding_config_t *config);

/**
 * @brief 编码转换函数
 * 
 * @param input 输入数据
 * @param input_length 输入长度
 * @param source_encoding 源编码
 * @param target_encoding 目标编码
 * @return 转换结果，需要调用shell_encoding_free_result释放
 */
shell_encoding_result_t shell_encoding_convert(
    const uint8_t *input, 
    size_t input_length,
    shell_encoding_type_t source_encoding,
    shell_encoding_type_t target_encoding
);

/**
 * @brief 释放编码转换结果
 * 
 * @param result 转换结果
 */
void shell_encoding_free_result(shell_encoding_result_t *result);

/**
 * @brief 检测字符串编码类型
 * 
 * @param data 输入数据
 * @param length 数据长度
 * @return 检测到的编码类型
 */
shell_encoding_type_t shell_encoding_detect(const uint8_t *data, size_t length);

/**
 * @brief 检查字符串是否包含中文字符
 * 
 * @param str 输入字符串
 * @return true 包含中文, false 不包含
 */
bool shell_encoding_contains_chinese(const char *str);

/**
 * @brief 获取编码类型名称
 * 
 * @param encoding_type 编码类型
 * @return 编码类型名称字符串
 */
const char* shell_encoding_get_name(shell_encoding_type_t encoding_type);

/**
 * @brief 获取编码类型描述
 * 
 * @param encoding_type 编码类型
 * @return 编码类型描述字符串
 */
const char* shell_encoding_get_description(shell_encoding_type_t encoding_type);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_ENCODING_H */
