#include "cmd_basic.h"
#include "shell.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"

// 命令帮助信息结构
typedef struct {
    const char *name;
    const char *usage;
    const char *description;
    const char *examples;
} cmd_help_info_t;

// 命令帮助信息数组
static const cmd_help_info_t cmd_help_table[] = {
    // 基础命令
    {"help", "help [命令名]", "显示帮助信息。不带参数显示所有命令列表，带参数显示指定命令的详细帮助", 
     "help\r\n"
     "help ls\r\n"
     "help mkdir"},
    
    {"echo", "echo <文本>", "回显输入的文本", 
     "echo Hello World\r\n"
     "echo 测试中文"},
    
    {"version", "version", "显示系统版本信息和ASCII艺术图", "version"},
    
    {"clear", "clear", "清屏", "clear"},
    
    {"test", "test [参数]", "参数测试命令，显示通道ID和参数信息", 
     "test\r\n"
     "test abc 123"},
    
    {"kv", "kv <操作> [参数]", "键值存储操作",
     "kv set mykey 123\r\n"
     "kv get mykey\r\n"
     "kv list\r\n"
     "kv del mykey\r\n"
     "kv clear"},
    
    {"buffer", "buffer [操作] [参数]", "宏缓冲区管理",
     "buffer\r\n"
     "buffer list\r\n"
     "buffer show mymacro\r\n"
     "buffer exec mymacro\r\n"
     "buffer del mymacro\r\n"
     "buffer clear"},
    
    // 系统命令
    {"status", "status [参数]", "显示系统状态信息", "status"},
    
    {"led", "led <1-4|all> <on|off|toggle|blink>", "控制LED开关、切换或闪烁",
     "led 1 on\r\n"
     "led all off\r\n"
     "led 2 toggle\r\n"
     "led 3 blink 5 200\r\n"
     "led status"},
    
    {"tasks", "tasks", "显示所有FreeRTOS任务信息", "tasks"},
    
    {"heap", "heap", "显示内存使用情况", "heap"},
    
    {"uptime", "uptime", "显示系统运行时间", "uptime"},
    
    {"cpu", "cpu", "显示CPU使用率", "cpu"},
    
    {"reset", "reset", "重启系统", "reset"},
    
    {"delay", "delay <毫秒数>", "延时指定毫秒数",
     "delay 1000\r\n"
     "delay 500"},
    
    // FreeRTOS命令  
    {"queue", "queue <操作>", "队列操作",
     "queue create\r\n"
     "queue send\r\n"
     "queue receive"},
    
    {"sem", "sem <操作>", "信号量操作",
     "sem create\r\n"
     "sem take\r\n"
     "sem give"},
    
    {"timer", "timer <操作>", "定时器操作",
     "timer create\r\n"
     "timer start\r\n"
     "timer stop"},
    
    // 文件系统命令
    {"pwd", "pwd", "显示当前工作目录", "pwd"},
    
    {"cd", "cd [目录]", "切换工作目录",
     "cd /sdcard\r\n"
     "cd ..\r\n"
     "cd subfolder"},
    
    {"ls", "ls [目录]", "列出目录内容",
     "ls\r\n"
     "ls /sdcard\r\n"
     "ls subfolder"},
    
    {"mkdir", "mkdir <目录名>", "创建目录",
     "mkdir newfolder\r\n"
     "mkdir /sdcard/data"},
    
    {"rmdir", "rmdir [-r] <目录名>", "删除目录。-r选项递归删除非空目录",
     "rmdir emptyfolder\r\n"
     "rmdir -r fullfolder"},
    
    {"rm", "rm <文件名>", "删除文件",
     "rm file.txt\r\n"
     "rm /sdcard/data.log"},
    
    {"cp", "cp <源文件> <目标文件>", "复制文件",
     "cp file1.txt file2.txt\r\n"
     "cp /sdcard/src.txt /sdcard/backup/dst.txt"},
    
    {"mv", "mv <源文件> <目标文件>", "移动/重命名文件",
     "mv oldname.txt newname.txt\r\n"
     "mv file.txt /sdcard/backup/"},
    
    {"cat", "cat <文件名>", "显示文件内容",
     "cat readme.txt\r\n"
     "cat /sdcard/config.ini"},
    
    {"touch", "touch <文件名>", "创建空文件或更新文件时间戳",
     "touch newfile.txt\r\n"
     "touch /sdcard/log.txt"},
    
    {"du", "du [目录]", "显示目录使用情况",
     "du\r\n"
     "du /sdcard\r\n"
     "du subfolder"},
    
    {"find", "find <文件名模式>", "在当前目录查找文件",
     "find config\r\n"
     "find .txt\r\n"
     "find data"},
     
    // 宏相关命令
    {"macro", "macro <宏名称>", "开始录制宏命令",
     "macro mymacro\r\n"
     "macro backup_files"},
     
    {"endmacro", "endmacro", "停止录制宏命令", "endmacro"},
    
    {"exec", "exec [宏名称]", "执行宏命令",
     "exec\r\n"
     "exec mymacro"},
     
    {"jump", "jump <键名> <行号>", "条件跳转命令（仅在宏内使用）",
     "jump status 5\r\n"
     "jump count 10"},
     
    // 自定义测试命令
    {"test", "test", "开始自动化测试(IO1-8循环,LED1-4循环,终端持续打印)",
     "test"},
     
    {"testoff", "testoff", "停止自动化测试",
     "testoff"}
};

static const size_t cmd_help_table_size = sizeof(cmd_help_table) / sizeof(cmd_help_table[0]);



void task_help(uint32_t channel_id, const char *params) {
    char response[2048];
    
    // 去除参数前后的空格
    while (*params == ' ') params++;
    
    if (strlen(params) == 0) {
        // 不带参数：显示所有命令的简要列表
        snprintf(response, sizeof(response),
                 "=== ESP32 Shell 命令列表 ===\r\n"
                 "使用 'help <命令名>' 查看具体命令的详细帮助\r\n"
                 "\r\n"
                 "【基础命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示基础命令
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i <= 6) { // 基础命令 (help, echo, version, clear, test, kv, buffer)
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), "\r\n【系统命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示系统命令  
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i >= 7 && i <= 13) { // 系统命令
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), "\r\n【FreeRTOS命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示FreeRTOS命令
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i >= 14 && i <= 16) { // FreeRTOS命令
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), "\r\n【文件系统命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示文件系统命令
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i >= 17 && i <= 29) { // 文件系统命令 (pwd到find)
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), "\r\n【宏命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示宏命令
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i >= 30 && i <= 33) { // 宏命令 (macro, endmacro, exec, jump)
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), "\r\n【测试命令】\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        // 显示测试命令
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (i >= 34) { // 测试命令 (test, testoff)
                snprintf(response, sizeof(response), "  %-12s - %s\r\n", 
                        cmd_help_table[i].name, cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        
        snprintf(response, sizeof(response), 
                "\r\n==================\r\n"
                "总共 %zu 个命令可用\r\n"
                "提示: 使用 'help <命令名>' 查看命令的详细用法和示例\r\n", 
                cmd_help_table_size);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
    } else {
        // 带参数：显示指定命令的详细帮助
        bool found = false;
        
        for (size_t i = 0; i < cmd_help_table_size; i++) {
            if (strcmp(cmd_help_table[i].name, params) == 0) {
                found = true;
                
                snprintf(response, sizeof(response),
                        "=== 命令详细帮助: %s ===\r\n"
                        "\r\n"
                        "用法:\r\n"
                        "  %s\r\n"
                        "\r\n"
                        "描述:\r\n"
                        "  %s\r\n"
                        "\r\n"
                        "示例:\r\n",
                        cmd_help_table[i].name,
                        cmd_help_table[i].usage,
                        cmd_help_table[i].description);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                
                // 分行显示示例
                const char *examples = cmd_help_table[i].examples;
                char example_line[256];
                const char *line_start = examples;
                const char *line_end;
                
                while ((line_end = strstr(line_start, "\r\n")) != NULL) {
                    size_t line_len = line_end - line_start;
                    if (line_len < sizeof(example_line) - 4) {
                        strncpy(example_line, line_start, line_len);
                        example_line[line_len] = '\0';
                        snprintf(response, sizeof(response), "  %s\r\n", example_line);
                        cmd_output(channel_id, (uint8_t *)response, strlen(response));
                    }
                    line_start = line_end + 2; // 跳过 \r\n
                }
                
                // 处理最后一行（没有\r\n结尾的）
                if (strlen(line_start) > 0) {
                    snprintf(response, sizeof(response), "  %s\r\n", line_start);
                    cmd_output(channel_id, (uint8_t *)response, strlen(response));
                }
                
                snprintf(response, sizeof(response), "==================\r\n");
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                break;
            }
        }
        
        if (!found) {
            snprintf(response, sizeof(response), 
                    "错误: 未找到命令 '%s'\r\n"
                    "使用 'help' 查看所有可用命令\r\n", params);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
    }
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