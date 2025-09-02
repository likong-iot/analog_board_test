#include "cmd_system.h"
#include "cmd_encoding.h"
#include "shell.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>



void task_tasks(uint32_t channel_id, const char *params) {
    char response[1024];
    
    // 获取总任务数
    UBaseType_t total_tasks = uxTaskGetNumberOfTasks();
    
    shell_snprintf(response, sizeof(response),
             "=== 任务信息 ===\r\n"
             "总任务数: %lu\r\n"
             "当前任务: %s\r\n"
             "==================\r\n",
             (unsigned long)total_tasks,
             pcTaskGetName(NULL));
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    // 显示状态代码含义
    shell_snprintf(response, sizeof(response),
             "状态代码说明:\r\n"
             "R = Running (运行中)\r\n"
             "B = Blocked (阻塞)\r\n"
             "S = Suspended (挂起)\r\n"
             "D = Deleted (删除)\r\n"
             "==================\r\n");
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    // 使用vTaskList获取任务信息
    char *task_list_buffer = malloc(4096);
    if (task_list_buffer != NULL) {
        // 获取任务列表
        vTaskList(task_list_buffer);
        
        // 直接输出任务列表
        cmd_output(channel_id, (uint8_t *)task_list_buffer, strlen(task_list_buffer));
        
        free(task_list_buffer);
    } else {
        shell_snprintf(response, sizeof(response),
                 "无法获取任务列表\r\n"
                 "==================\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
}

void task_heap(uint32_t channel_id, const char *params) {
    char response[512];
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    
    // 获取总堆内存大小（ESP32-S3通常是512KB）
    size_t total_heap = 512 * 1024; // 512KB
    size_t used_heap = total_heap - free_heap;
    
    // 计算百分比
    float free_percent = (float)free_heap / total_heap * 100.0f;
    float used_percent = (float)used_heap / total_heap * 100.0f;
    float min_free_percent = (float)min_free_heap / total_heap * 100.0f;
    
    shell_snprintf(response, sizeof(response),
             "=== 内存信息 ===\r\n"
             "总堆内存: %lu bytes (100%%)\r\n"
             "已用内存: %lu bytes (%.1f%%)\r\n"
             "可用内存: %lu bytes (%.1f%%)\r\n"
             "最小可用: %lu bytes (%.1f%%)\r\n"
             "==================\r\n",
             (unsigned long)total_heap,
             (unsigned long)used_heap, used_percent,
             (unsigned long)free_heap, free_percent,
             (unsigned long)min_free_heap, min_free_percent);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_uptime(uint32_t channel_id, const char *params) {
    char response[512];
    uint32_t uptime_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t hours = uptime_ms / (1000 * 60 * 60);
    uint32_t minutes = (uptime_ms % (1000 * 60 * 60)) / (1000 * 60);
    uint32_t seconds = (uptime_ms % (1000 * 60)) / 1000;
    
    shell_snprintf(response, sizeof(response),
             "=== 运行时间 ===\r\n"
             "总运行时间: %02lu:%02lu:%02lu\r\n"
             "毫秒数: %lu ms\r\n"
             "==================\r\n",
             hours, minutes, seconds, uptime_ms);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_cpu(uint32_t channel_id, const char *params) {
    char response[512];
    
    // 获取当前核心ID
    BaseType_t current_core = xPortGetCoreID();
    
    // 获取运行时统计信息
    char *runtime_stats_buffer = malloc(2048);
    float total_cpu_time = 0.0f;
    float idle_cpu_time = 0.0f;
    float cpu_usage = 0.0f;
    
    if (runtime_stats_buffer != NULL) {
        // 获取运行时统计
        vTaskGetRunTimeStats(runtime_stats_buffer);
        
        char *line = strtok(runtime_stats_buffer, "\n");
        while (line != NULL) {
            char task_name[32];
            char runtime_str[32];
            char percentage_str[32];
            
            if (sscanf(line, "%s %s %s", task_name, runtime_str, percentage_str) == 3) {
                float percentage = atof(percentage_str);
                total_cpu_time += percentage;
                
                // 找到空闲任务
                if (strstr(task_name, "IDLE") != NULL || 
                    strstr(task_name, "idle") != NULL ||
                    strstr(task_name, "Idle") != NULL) {
                    idle_cpu_time = percentage;
                }
            }
            line = strtok(NULL, "\n");
        }
        free(runtime_stats_buffer);
        
        // 计算CPU使用率 = 100% - 空闲时间
        if (total_cpu_time > 0) {
            cpu_usage = 100.0f - idle_cpu_time;
        }
    }
    
    // 如果无法获取运行时统计，使用简单估算
    if (cpu_usage <= 0.0f) {
        UBaseType_t total_tasks = uxTaskGetNumberOfTasks();
        UBaseType_t running_tasks = 0;
        
        char *task_list_buffer = malloc(1024);
        if (task_list_buffer != NULL) {
            vTaskList(task_list_buffer);
            
            char *line = strtok(task_list_buffer, "\n");
            while (line != NULL) {
                char task_name[32];
                char state_code[8];
                char priority[8];
                char stack_free[8];
                char stack_size[8];
                char task_num[8];
                
                if (sscanf(line, "%s %s %s %s %s %s",
                          task_name, state_code, priority, stack_free, stack_size, task_num) == 6) {
                    if (strcmp(state_code, "R") == 0) {
                        running_tasks++;
                    }
                }
                line = strtok(NULL, "\n");
            }
            free(task_list_buffer);
            
            if (total_tasks > 0) {
                cpu_usage = (float)running_tasks / total_tasks * 100.0f;
            }
        }
    }
    
    shell_snprintf(response, sizeof(response),
             "=== CPU状态 ===\r\n"
             "当前核心: %ld\r\n"
             "CPU使用率: %.1f%%\r\n"
             "空闲时间: %.1f%%\r\n"
             "==================\r\n",
             (long)current_core,
             cpu_usage,
             100.0f - cpu_usage);
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_reset(uint32_t channel_id, const char *params) {
    char response[256];
    shell_snprintf(response, sizeof(response), "系统将在3秒后重启...\r\n");
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}

void task_status(uint32_t channel_id, const char *params) {
    char response[512];
    shell_snprintf(response, sizeof(response),
             "=== 系统状态 ===\r\n"
             "通信通道ID: %lu\r\n"
             "可用内存: %lu bytes\r\n"
             "运行时间: %lu ms\r\n"
             "参数: %s\r\n"
             "==================\r\n",
             (unsigned long)channel_id, (unsigned long)esp_get_free_heap_size(),
             (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS),
             strlen(params) > 0 ? params : "无");

    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_led(uint32_t channel_id, const char *params) {
    char response[256];
    if (strlen(params) == 0) {
        shell_snprintf(response, sizeof(response), "LED控制: 请提供参数 'on' 或 'off'\r\n");
    } else if (strcmp(params, "on") == 0) {
        shell_snprintf(response, sizeof(response), "LED已开启\r\n");
    } else if (strcmp(params, "off") == 0) {
        shell_snprintf(response, sizeof(response), "LED已关闭\r\n");
    } else {
        shell_snprintf(response, sizeof(response), "错误: 参数 '%s' 无效，应为 'on' 或 'off'\r\n", params);
    }
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_delay(uint32_t channel_id, const char *params) {
    char response[256];
    if (strlen(params) == 0) {
        shell_snprintf(response, sizeof(response), "用法: delay <毫秒数>\r\n");
    } else {
        int ms = atoi(params);
        if (ms > 0) {
            shell_snprintf(response, sizeof(response), "延时 %d 毫秒...\r\n", ms);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            vTaskDelay(pdMS_TO_TICKS(ms));
            shell_snprintf(response, sizeof(response), "延时完成\r\n");
        } else {
            shell_snprintf(response, sizeof(response), "错误: 无效的延时时间 '%s'\r\n", params);
        }
    }
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
} 