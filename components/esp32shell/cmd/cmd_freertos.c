#include "cmd_freertos.h"
#include "shell.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "CMD_FREERTOS";

// 全局变量用于演示
static QueueHandle_t demo_queue = NULL;
static SemaphoreHandle_t demo_sem = NULL;
static TimerHandle_t demo_timer = NULL;

void task_queue(uint32_t channel_id, const char *params) {
    char response[512];
    char cmd[32], arg[32];
    
    if (sscanf(params, "%31s %31s", cmd, arg) < 1) {
        snprintf(response, sizeof(response), "用法: queue <create|send|receive>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (strcmp(cmd, "create") == 0) {
        if (demo_queue != NULL) {
            vQueueDelete(demo_queue);
        }
        int size = strlen(arg) > 0 ? atoi(arg) : 5; // 默认5个元素
        demo_queue = xQueueCreate(size, sizeof(int));
        if (demo_queue != NULL) {
            snprintf(response, sizeof(response), "队列创建成功，大小: %d\r\n", size);
        } else {
            snprintf(response, sizeof(response), "队列创建失败\r\n");
        }
    } else if (strcmp(cmd, "send") == 0) {
        if (demo_queue == NULL) {
            snprintf(response, sizeof(response), "错误: 队列未创建\r\n");
        } else {
            int value = strlen(arg) > 0 ? atoi(arg) : 42; // 默认值42
            if (xQueueSend(demo_queue, &value, pdMS_TO_TICKS(1000)) == pdTRUE) {
                snprintf(response, sizeof(response), "成功发送数据: %d\r\n", value);
            } else {
                snprintf(response, sizeof(response), "发送数据失败\r\n");
            }
        }
    } else if (strcmp(cmd, "receive") == 0) {
        if (demo_queue == NULL) {
            snprintf(response, sizeof(response), "错误: 队列未创建\r\n");
        } else {
            int value;
            if (xQueueReceive(demo_queue, &value, pdMS_TO_TICKS(1000)) == pdTRUE) {
                snprintf(response, sizeof(response), "接收到数据: %d\r\n", value);
            } else {
                snprintf(response, sizeof(response), "接收数据失败\r\n");
            }
        }
    } else {
        snprintf(response, sizeof(response), "未知命令: %s\r\n", cmd);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_sem(uint32_t channel_id, const char *params) {
    char response[512];
    char cmd[32];
    
    if (sscanf(params, "%31s", cmd) < 1) {
        snprintf(response, sizeof(response), "用法: sem <create|take|give>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (strcmp(cmd, "create") == 0) {
        if (demo_sem != NULL) {
            vSemaphoreDelete(demo_sem);
        }
        demo_sem = xSemaphoreCreateBinary();
        if (demo_sem != NULL) {
            snprintf(response, sizeof(response), "信号量创建成功\r\n");
        } else {
            snprintf(response, sizeof(response), "信号量创建失败\r\n");
        }
    } else if (strcmp(cmd, "take") == 0) {
        if (demo_sem == NULL) {
            snprintf(response, sizeof(response), "错误: 信号量未创建\r\n");
        } else {
            if (xSemaphoreTake(demo_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
                snprintf(response, sizeof(response), "成功获取信号量\r\n");
            } else {
                snprintf(response, sizeof(response), "获取信号量失败\r\n");
            }
        }
    } else if (strcmp(cmd, "give") == 0) {
        if (demo_sem == NULL) {
            snprintf(response, sizeof(response), "错误: 信号量未创建\r\n");
        } else {
            if (xSemaphoreGive(demo_sem) == pdTRUE) {
                snprintf(response, sizeof(response), "成功释放信号量\r\n");
            } else {
                snprintf(response, sizeof(response), "释放信号量失败\r\n");
            }
        }
    } else {
        snprintf(response, sizeof(response), "未知命令: %s\r\n", cmd);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

// 定时器回调函数
void demo_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "演示定时器触发");
}

void task_timer(uint32_t channel_id, const char *params) {
    char response[512];
    char cmd[32], arg[32];
    
    if (sscanf(params, "%31s %31s", cmd, arg) < 1) {
        snprintf(response, sizeof(response), "用法: timer <create|start|stop>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (strcmp(cmd, "create") == 0) {
        if (demo_timer != NULL) {
            xTimerDelete(demo_timer, pdMS_TO_TICKS(1000));
        }
        int period = strlen(arg) > 0 ? atoi(arg) : 1000; // 默认1秒
        demo_timer = xTimerCreate("DemoTimer", pdMS_TO_TICKS(period), pdTRUE, NULL, demo_timer_callback);
        if (demo_timer != NULL) {
            snprintf(response, sizeof(response), "定时器创建成功，周期: %d ms\r\n", period);
        } else {
            snprintf(response, sizeof(response), "定时器创建失败\r\n");
        }
    } else if (strcmp(cmd, "start") == 0) {
        if (demo_timer == NULL) {
            snprintf(response, sizeof(response), "错误: 定时器未创建\r\n");
        } else {
            if (xTimerStart(demo_timer, pdMS_TO_TICKS(1000)) == pdTRUE) {
                snprintf(response, sizeof(response), "定时器启动成功\r\n");
            } else {
                snprintf(response, sizeof(response), "定时器启动失败\r\n");
            }
        }
    } else if (strcmp(cmd, "stop") == 0) {
        if (demo_timer == NULL) {
            snprintf(response, sizeof(response), "错误: 定时器未创建\r\n");
        } else {
            if (xTimerStop(demo_timer, pdMS_TO_TICKS(1000)) == pdTRUE) {
                snprintf(response, sizeof(response), "定时器停止成功\r\n");
            } else {
                snprintf(response, sizeof(response), "定时器停止失败\r\n");
            }
        }
    } else {
        snprintf(response, sizeof(response), "未知命令: %s\r\n", cmd);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
} 