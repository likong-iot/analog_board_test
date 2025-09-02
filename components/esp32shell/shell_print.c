/**
 * @file shell_print.c
 * @brief Shell统一打印函数实现
 * 
 * 提供统一的打印接口，自动处理编码转换
 */

#include "shell_print.h"
#include "shell.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "SHELL_PRINT";

// 全局状态
static bool g_initialized = false;
static shell_encoding_type_t g_current_encoding = SHELL_ENCODING_UTF8;

// 内部缓冲区大小
#define SHELL_PRINT_BUFFER_SIZE 1024

/**
 * @brief 内部打印函数，处理编码转换
 */
static int internal_print(uint32_t channel_id, const char *format, va_list args, shell_encoding_type_t target_encoding)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "Shell打印系统未初始化");
        return -1;
    }
    
    if (format == NULL) {
        return 0;
    }
    
    // 格式化字符串到缓冲区
    char buffer[SHELL_PRINT_BUFFER_SIZE];
    int formatted_len = vsnprintf(buffer, sizeof(buffer), format, args);
    
    if (formatted_len < 0 || formatted_len >= (int)sizeof(buffer)) {
        ESP_LOGE(TAG, "格式化字符串失败或缓冲区溢出");
        return -1;
    }
    
    // 如果目标编码不是UTF-8，需要进行编码转换
    if (target_encoding != SHELL_ENCODING_UTF8) {
        ESP_LOGI(TAG, "需要进行编码转换: UTF-8 -> %s, 原始长度: %d", 
                  shell_encoding_get_name(target_encoding), formatted_len);
        
        // 源编码固定为UTF-8（因为代码中写的是UTF-8字符串）
        shell_encoding_result_t result = shell_encoding_convert(
            (const uint8_t*)buffer, 
            formatted_len, 
            SHELL_ENCODING_UTF8,  // 源编码固定为UTF-8
            target_encoding        // 目标编码为用户设置的编码
        );
        
        if (result.success) {
            ESP_LOGI(TAG, "编码转换成功: 转换后长度: %d", result.length);
            // 使用转换后的数据
            cmd_output(channel_id, result.data, result.length);
            shell_encoding_free_result(&result);
            return result.length;
        } else {
            // 转换失败，使用原始数据
            ESP_LOGW(TAG, "编码转换失败，使用原始数据");
            cmd_output(channel_id, (const uint8_t*)buffer, formatted_len);
            return formatted_len;
        }
    } else {
        // 目标编码是UTF-8，直接输出
        ESP_LOGI(TAG, "目标编码是UTF-8，直接输出，长度: %d", formatted_len);
        cmd_output(channel_id, (const uint8_t*)buffer, formatted_len);
        return formatted_len;
    }
}

bool shell_print_init(const shell_encoding_config_t *encoding_config)
{
    if (g_initialized) {
        ESP_LOGW(TAG, "Shell打印系统已经初始化");
        return true;
    }
    
    // 初始化编码系统
    if (!shell_encoding_init(encoding_config)) {
        ESP_LOGE(TAG, "编码系统初始化失败");
        return false;
    }
    
    // 获取当前编码设置
    g_current_encoding = shell_encoding_get_global();
    
    // 设置默认编码为GB2312
    g_current_encoding = SHELL_ENCODING_GB2312;
    shell_encoding_set_global(SHELL_ENCODING_GB2312);
    
    g_initialized = true;
    ESP_LOGI(TAG, "Shell打印系统初始化成功，当前编码: %s", 
              shell_encoding_get_name(g_current_encoding));
    
    return true;
}

void shell_print_deinit(void)
{
    if (!g_initialized) {
        return;
    }
    
    shell_encoding_deinit();
    g_initialized = false;
    ESP_LOGI(TAG, "Shell打印系统已反初始化");
}

int shell_print(uint32_t channel_id, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = internal_print(channel_id, format, args, g_current_encoding);
    va_end(args);
    return result;
}

int shell_vprint(uint32_t channel_id, const char *format, va_list args)
{
    return internal_print(channel_id, format, args, g_current_encoding);
}

int shell_print_with_encoding(uint32_t channel_id, shell_encoding_type_t target_encoding, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = internal_print(channel_id, format, args, target_encoding);
    va_end(args);
    return result;
}

int shell_vprint_with_encoding(uint32_t channel_id, shell_encoding_type_t target_encoding, const char *format, va_list args)
{
    return internal_print(channel_id, format, args, target_encoding);
}

int shell_print_raw(uint32_t channel_id, const uint8_t *data, size_t length)
{
    if (!g_initialized || !data || length == 0) {
        return -1;
    }
    
    // 根据当前编码设置决定是否转换
    if (g_current_encoding != SHELL_ENCODING_UTF8) {
        ESP_LOGI(TAG, "shell_print_raw: 转换UTF-8 -> %s", shell_encoding_get_name(g_current_encoding));
        
        shell_encoding_result_t result = shell_encoding_convert(
            data, length, SHELL_ENCODING_UTF8, g_current_encoding);
        
        if (result.success) {
            cmd_output(channel_id, result.data, result.length);
            shell_encoding_free_result(&result);
            return result.length;
        } else {
            ESP_LOGW(TAG, "shell_print_raw: 编码转换失败，使用原始数据");
        }
    }
    
    // 直接输出或转换失败时使用原始数据
    cmd_output(channel_id, data, length);
    return length;
}

int shell_print_string(uint32_t channel_id, const char *str)
{
    if (!str) {
        return 0;
    }
    
    return shell_print(channel_id, "%s", str);
}

int shell_print_line(uint32_t channel_id, const char *str)
{
    if (!str) {
        return shell_print(channel_id, "\r\n");
    }
    
    return shell_print(channel_id, "%s\r\n", str);
}

int shell_snprintf(char *buffer, size_t buffer_size, const char *format, ...)
{
    if (!buffer || buffer_size == 0 || !format) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    
    // 如果当前编码不是UTF-8，需要进行编码转换
    if (g_initialized && g_current_encoding != SHELL_ENCODING_UTF8) {
        ESP_LOGI(TAG, "shell_snprintf: 转换UTF-8 -> %s", shell_encoding_get_name(g_current_encoding));
        
        shell_encoding_result_t conv_result = shell_encoding_convert(
            (const uint8_t*)buffer,
            result,
            SHELL_ENCODING_UTF8,
            g_current_encoding
        );
        
        if (conv_result.success && conv_result.length < buffer_size) {
            // 将转换后的结果复制回缓冲区
            memcpy(buffer, conv_result.data, conv_result.length);
            buffer[conv_result.length] = '\0';
            result = conv_result.length;
            shell_encoding_free_result(&conv_result);
        } else if (conv_result.success) {
            // 转换成功但缓冲区太小，只复制能容纳的部分
            memcpy(buffer, conv_result.data, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            result = buffer_size - 1;
            shell_encoding_free_result(&conv_result);
        }
    }
    
    return result;
}

int shell_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list args)
{
    if (!buffer || buffer_size == 0 || !format) {
        return -1;
    }
    
    int result = vsnprintf(buffer, buffer_size, format, args);
    
    // 如果当前编码不是UTF-8，需要进行编码转换
    if (g_initialized && g_current_encoding != SHELL_ENCODING_UTF8) {
        ESP_LOGI(TAG, "shell_vsnprintf: 转换UTF-8 -> %s", shell_encoding_get_name(g_current_encoding));
        
        shell_encoding_result_t conv_result = shell_encoding_convert(
            (const uint8_t*)buffer,
            result,
            SHELL_ENCODING_UTF8,
            g_current_encoding
        );
        
        if (conv_result.success && conv_result.length < buffer_size) {
            // 将转换后的结果复制回缓冲区
            memcpy(buffer, conv_result.data, conv_result.length);
            buffer[conv_result.length] = '\0';
            result = conv_result.length;
            shell_encoding_free_result(&conv_result);
        } else if (conv_result.success) {
            // 转换成功但缓冲区太小，只复制能容纳的部分
            memcpy(buffer, conv_result.data, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            result = buffer_size - 1;
            shell_encoding_free_result(&conv_result);
        }
    }
    
    return result;
}

bool shell_print_set_encoding(shell_encoding_type_t encoding_type)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "Shell打印系统未初始化");
        return false;
    }
    
    if (!shell_encoding_set_global(encoding_type)) {
        return false;
    }
    
    g_current_encoding = encoding_type;
    ESP_LOGI(TAG, "Shell打印编码已设置为: %s", shell_encoding_get_name(encoding_type));
    
    return true;
}

shell_encoding_type_t shell_print_get_encoding(void)
{
    // 直接从编码系统获取当前全局编码设置，确保同步
    if (g_initialized) {
        return shell_encoding_get_global();
    }
    return g_current_encoding;
}

bool shell_print_is_initialized(void)
{
    return g_initialized;
}
