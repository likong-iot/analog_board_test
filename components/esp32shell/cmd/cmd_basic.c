#include "cmd_basic.h"
#include "shell.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"



void task_help(uint32_t channel_id, const char *params) {
    char help_text[1024];
    snprintf(help_text, sizeof(help_text),
             "=== ESP32 Shell 命令列表 ===\r\n"
             "\r\n"
             "【基础命令】\r\n"
             "help - 显示此帮助信息\r\n"
             "echo <text> - 回显文本\r\n"
             "version - 显示版本信息\r\n"
             "clear - 清屏\r\n"
             "test <params> - 参数测试命令\r\n"
             "kv <set|get|del|list|clear|count> - 键值存储操作\r\n"
             "buffer [list|show|clear|exec|del] - 显示宏缓冲区\r\n"
             "macro <名称> - 开始录制宏\r\n"
             "endmacro - 停止录制宏\r\n"
             "exec [macro|<名称>] - 执行宏\r\n"
             "jump <键名> <行号> - 条件跳转命令(仅在宏内使用)\r\n"
             "\r\n"
             "【系统命令】\r\n"
             "status [params] - 显示系统状态\r\n"
             "led <on|off> - 控制LED\r\n"
             "tasks - 显示所有任务信息\r\n"
             "heap - 显示内存使用情况\r\n"
             "uptime - 显示系统运行时间\r\n"
             "cpu - 显示CPU使用率\r\n"
             "reset - 重启系统\r\n"
             "delay <ms> - 延时指定毫秒数\r\n"
             "\r\n"
             "【FreeRTOS命令】\r\n"
             "queue <create|send|receive> - 队列操作\r\n"
             "sem <create|take|give> - 信号量操作\r\n"
             "timer <create|start|stop> - 定时器操作\r\n"
             "==================\r\n"
             "注意: 所有命令输出都会发送到对应的通信通道\r\n");

    cmd_output(channel_id, (uint8_t *)help_text, strlen(help_text));
}

void task_echo(uint32_t channel_id, const char *params) {
    char response[512];
    if (strlen(params) > 0) {
        snprintf(response, sizeof(response), "Echo: %s\r\n", params);
    } else {
        snprintf(response, sizeof(response), "Echo: 无参数\r\n");
    }
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}





void task_version(uint32_t channel_id, const char *params) {
    // 发送回车换行
    cmd_output(channel_id, (uint8_t *)"\r\n", 2);
    
    // 使用字符串数组方式发送ASCII艺术字
    const char *art_lines[] = {
        "      ___                    ___           ___           ___           ___     ",
        "     /  /\\       ___        /  /\\         /  /\\         /  /\\         /  /\\    ",
        "    /  /:/      /__/\\      /  /:/        /  /::\\       /  /::|       /  /::\\   ",
        "   /  /:/       \\__\\:\\    /  /:/        /  /:/\\:\\     /  /:|:|      /  /:/\\:\\  ",
        "  /  /:/        /  /::\\  /  /::\\____   /  /:/  \\:\\   /  /:/|:|__   /  /:/  \\:\\ ",
        " /__/:/      __/  /:/\\/ /__/:/\\:::::\\ /__/:/ \\__\\:\\ /__/:/ |:| /\\ /__/:/_\\_ \\:\\",
        " \\  \\:\\     /__/\\/:/~~  \\__\\/~|:|~~~~ \\  \\:\\ /  /:/ \\__\\/  |:|/:/ \\  \\:\\__/\\_\\/",
        "  \\  \\:\\    \\  \\::/        |  |:|      \\  \\:\\  /:/      |  |:/:/   \\  \\:\\ \\:\\  ",
        "   \\  \\:\\    \\  \\:\\        |  |:|       \\  \\:\\/:/       |__|::/     \\  \\:\\/:/  ",
        "    \\  \\:\\    \\__\\/        |__|:|        \\  \\::/        /__/:/       \\  \\::/   ",
        "     \\__\\/                  \\__\\|         \\__\\/         \\__\\/         \\__\\/    ",
        "      __            ___           ___           ___           ___     ",
        "     |  |\\         /  /\\         /  /\\         /  /\\         /  /\\    ",
        "     |  |:|       /  /:/        /  /::|       /  /::\\       /  /::\\   ",
        "     |  |:|      /  /:/        /  /:|:|      /  /:/\\:\\     /  /:/\\:\\  ",
        "     |__|:|__   /  /:/        /  /:/|:|__   /  /:/  \\:\\   /  /::\\ \\:\\ ",
        " ____/__/::::\\ /__/:/     /\\ /__/:/ |:| /\\ /__/:/ \\__\\:| /__/:/\\:\\_\\:\\",
        " \\__\\::::/~~~~ \\  \\:\\    /:/ \\__\\/  |:|/:/ \\  \\:\\ /  /:/ \\__\\/  \\:\\/:/ ",
        "    |~~|:|      \\  \\:\\  /:/      |  |:/:/   \\  \\:\\  /:/       \\__\\::/ ",
        "    |  |:|       \\  \\:\\/:/       |__|::/     \\  \\:\\/:/        /  /:/  ",
        "    |__|:|        \\  \\::/        /__/:/       \\__\\::/        /__/:/   ",
        "     \\__\\|         \\__\\/         \\__\\/            ~~         \\__\\/    "
    };
    
    // 逐行发送艺术字
    for (int i = 0; i < 22; i++) {
        cmd_output(channel_id, (uint8_t *)art_lines[i], strlen(art_lines[i]));
        cmd_output(channel_id, (uint8_t *)"\r\n", 2);
    }
    
    // 发送版本信息
    cmd_output(channel_id, (uint8_t *)"立控esp32shell v1.0\r\n", 22);
}

void task_clear(uint32_t channel_id, const char *params) {
    const char *clear = "\033[2J\033[H"; // ANSI清屏序列
    cmd_output(channel_id, (uint8_t *)clear, strlen(clear));
}

void task_test(uint32_t channel_id, const char *params) {
    char response[512];
    snprintf(response, sizeof(response),
             "=== 参数测试 ===\r\n"
             "通信通道ID: %lu\r\n"
             "参数: '%s'\r\n"
             "参数长度: %zu\r\n"
             "==================\r\n",
             (unsigned long)channel_id, params, strlen(params));
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_kv(uint32_t channel_id, const char *params) {
    char response[512];
    char cmd[32], key[64], value_str[32];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), 
                "键值存储命令用法:\r\n"
                "kv set <key> <value>  - 设置键值对\r\n"
                "kv get <key>          - 获取键值\r\n"
                "kv del <key>          - 删除键值对\r\n"
                "kv list               - 列出所有键值对\r\n"
                "kv clear              - 清空所有键值对\r\n"
                "kv count              - 显示键值对数量\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (sscanf(params, "%31s %63s %31s", cmd, key, value_str) < 2) {
        snprintf(response, sizeof(response), "错误: 参数不足\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 获取当前shell实例的键值存储
    shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
    if (instance == NULL) {
        snprintf(response, sizeof(response), "错误: 未找到shell实例\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (strcmp(cmd, "set") == 0) {
        if (sscanf(params, "%31s %63s %31s", cmd, key, value_str) == 3) {
            uint32_t value = atoi(value_str);
            if (kv_store_set(&instance->kv_store, key, value)) {
                snprintf(response, sizeof(response), "设置成功: %s = %lu\r\n", key, (unsigned long)value);
            } else {
                snprintf(response, sizeof(response), "设置失败\r\n");
            }
        } else {
            snprintf(response, sizeof(response), "错误: set命令需要键和值\r\n");
        }
    } else if (strcmp(cmd, "get") == 0) {
        uint32_t value;
        if (kv_store_get(&instance->kv_store, key, &value)) {
            snprintf(response, sizeof(response), "%s = %lu\r\n", key, (unsigned long)value);
        } else {
            snprintf(response, sizeof(response), "键 '%s' 不存在\r\n", key);
        }
    } else if (strcmp(cmd, "del") == 0) {
        if (kv_store_delete(&instance->kv_store, key)) {
            snprintf(response, sizeof(response), "删除成功: %s\r\n", key);
        } else {
            snprintf(response, sizeof(response), "键 '%s' 不存在\r\n", key);
        }
    } else if (strcmp(cmd, "list") == 0) {
        size_t count = kv_store_count(&instance->kv_store);
        if (count > 0) {
            snprintf(response, sizeof(response), "=== 键值对列表 (%zu个) ===\r\n", count);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            
            char list_buffer[1024];
            kv_store_list(&instance->kv_store, list_buffer, sizeof(list_buffer));
            cmd_output(channel_id, (uint8_t *)list_buffer, strlen(list_buffer));
            
            snprintf(response, sizeof(response), "==================\r\n");
        } else {
            snprintf(response, sizeof(response), "键值存储为空\r\n");
        }
    } else if (strcmp(cmd, "clear") == 0) {
        kv_store_clear(&instance->kv_store);
        snprintf(response, sizeof(response), "键值存储已清空\r\n");
    } else if (strcmp(cmd, "count") == 0) {
        size_t count = kv_store_count(&instance->kv_store);
        snprintf(response, sizeof(response), "键值对数量: %zu\r\n", count);
    } else {
        snprintf(response, sizeof(response), "未知命令: %s\r\n", cmd);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_buffer(uint32_t channel_id, const char *params) {
    char response[1024];
    
    // 获取当前shell实例
    shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
    if (instance == NULL) {
        snprintf(response, sizeof(response), "错误: 未找到shell实例\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (strlen(params) == 0) {
        // 显示宏缓冲区信息
        size_t macro_count = macro_buffer_count(&instance->macro_buffer);
        bool is_recording = macro_buffer_is_recording(&instance->macro_buffer);
        
        snprintf(response, sizeof(response), 
                "=== 宏缓冲区信息 ===\r\n"
                "宏命令数量: %zu\r\n"
                "录制状态: %s\r\n",
                macro_count,
                is_recording ? "正在录制" : "未录制");
        
        if (is_recording) {
            snprintf(response + strlen(response), sizeof(response) - strlen(response),
                    "当前宏名称: %s\r\n", instance->macro_buffer.current_macro_name);
        }
        
        snprintf(response + strlen(response), sizeof(response) - strlen(response),
                "==================\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示宏缓冲区内容
        if (macro_count > 0 || is_recording) {
            snprintf(response, sizeof(response), "=== 宏命令列表 ===\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            
            char list_buffer[1024];
            macro_buffer_list(&instance->macro_buffer, list_buffer, sizeof(list_buffer));
            cmd_output(channel_id, (uint8_t *)list_buffer, strlen(list_buffer));
            
            snprintf(response, sizeof(response), "==================\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        } else {
            snprintf(response, sizeof(response), "宏缓冲区为空\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
    } else if (strcmp(params, "clear") == 0) {
        // 清空宏缓冲区
        macro_buffer_clear(&instance->macro_buffer);
        snprintf(response, sizeof(response), "宏缓冲区已清空\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    } else if (strcmp(params, "list") == 0) {
        // 显示所有宏的详细信息
        size_t macro_count = macro_buffer_count(&instance->macro_buffer);
        bool is_recording = macro_buffer_is_recording(&instance->macro_buffer);
        
        if (macro_count == 0 && !is_recording) {
            snprintf(response, sizeof(response), "没有保存的宏\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        
        snprintf(response, sizeof(response), "=== 所有宏详细信息 ===\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        if (is_recording) {
            snprintf(response, sizeof(response), 
                    "【正在录制】宏: %s\r\n"
                    "状态: 录制中\r\n"
                    "命令数量: (录制中...)\r\n"
                    "---\r\n", 
                    instance->macro_buffer.current_macro_name);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
        
        // 显示所有已保存的宏
        if (macro_count > 0) {
            char list_buffer[2048];
            macro_buffer_list(&instance->macro_buffer, list_buffer, sizeof(list_buffer));
            cmd_output(channel_id, (uint8_t *)list_buffer, strlen(list_buffer));
        }
        
        snprintf(response, sizeof(response), "==================\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    } else if (strncmp(params, "exec", 4) == 0) {
        // 执行宏
        const char *macro_name = params + 4;
        while (*macro_name == ' ') macro_name++; // 跳过空格
        
        if (strlen(macro_name) == 0) {
            // 执行第一个宏
            if (macro_buffer_execute(&instance->macro_buffer, channel_id)) {
                snprintf(response, sizeof(response), "宏执行完成\r\n");
            } else {
                snprintf(response, sizeof(response), "错误: 没有可执行的宏\r\n");
            }
        } else {
            // 按名称执行宏
            if (macro_buffer_execute_by_name(&instance->macro_buffer, macro_name, channel_id)) {
                snprintf(response, sizeof(response), "宏 '%s' 执行完成\r\n", macro_name);
            } else {
                snprintf(response, sizeof(response), "错误: 宏 '%s' 不存在\r\n", macro_name);
            }
        }
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    } else if (strncmp(params, "del", 3) == 0) {
        // 删除宏
        const char *macro_name = params + 3;
        while (*macro_name == ' ') macro_name++; // 跳过空格
        
        if (strlen(macro_name) == 0) {
            snprintf(response, sizeof(response), "用法: buffer del <宏名称>\r\n");
        } else {
            if (macro_buffer_delete(&instance->macro_buffer, macro_name)) {
                snprintf(response, sizeof(response), "宏 '%s' 已删除\r\n", macro_name);
            } else {
                snprintf(response, sizeof(response), "错误: 宏 '%s' 不存在\r\n", macro_name);
            }
        }
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    } else if (strncmp(params, "show", 4) == 0) {
        // 显示指定宏的详细内容
        const char *macro_name = params + 4;
        while (*macro_name == ' ') macro_name++; // 跳过空格
        
        if (strlen(macro_name) == 0) {
            snprintf(response, sizeof(response), "用法: buffer show <宏名称>\r\n");
        } else {
            // 检查是否是正在录制的宏
            if (macro_buffer_is_recording(&instance->macro_buffer) && 
                strcmp(instance->macro_buffer.current_macro_name, macro_name) == 0) {
                snprintf(response, sizeof(response), 
                        "=== 宏 '%s' 详细信息 ===\r\n"
                        "状态: 正在录制\r\n"
                        "命令列表:\r\n", macro_name);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                
                // 显示临时队列中的命令
                char list_buffer[1024];
                cmd_queue_list(&instance->macro_buffer.temp_queue, list_buffer, sizeof(list_buffer));
                if (strlen(list_buffer) > 0) {
                    cmd_output(channel_id, (uint8_t *)list_buffer, strlen(list_buffer));
                } else {
                    snprintf(response, sizeof(response), "(暂无命令)\r\n");
                    cmd_output(channel_id, (uint8_t *)response, strlen(response));
                }
                
                snprintf(response, sizeof(response), "==================\r\n");
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                return;
            }
            
            // 查找已保存的宏
            if (macro_buffer_exists(&instance->macro_buffer, macro_name)) {
                snprintf(response, sizeof(response), 
                        "=== 宏 '%s' 详细信息 ===\r\n"
                        "状态: 已保存\r\n"
                        "命令列表:\r\n", macro_name);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                
                // 获取并显示宏的命令列表
                char list_buffer[1024];
                macro_buffer_get_commands(&instance->macro_buffer, macro_name, list_buffer, sizeof(list_buffer));
                cmd_output(channel_id, (uint8_t *)list_buffer, strlen(list_buffer));
                
                snprintf(response, sizeof(response), "==================\r\n");
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            } else {
                snprintf(response, sizeof(response), "错误: 宏 '%s' 不存在\r\n", macro_name);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
    } else {
        snprintf(response, sizeof(response), 
                "宏缓冲区命令用法:\r\n"
                "buffer                    - 显示宏缓冲区信息\r\n"
                "buffer list               - 显示所有宏详细信息\r\n"
                "buffer show <宏名称>      - 显示指定宏的详细内容\r\n"
                "buffer clear              - 清空所有宏\r\n"
                "buffer exec [宏名称]      - 执行宏\r\n"
                "buffer del <宏名称>       - 删除指定宏\r\n"
                "\r\n"
                "宏录制命令:\r\n"
                "macro <名称>              - 开始录制宏\r\n"
                "endmacro                  - 停止录制宏\r\n"
                "exec macro                - 执行第一个宏\r\n"
                "exec <宏名称>             - 执行指定宏\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
}