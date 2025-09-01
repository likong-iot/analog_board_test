#include "cmd.h"
#include "shell.h"
#include "cmd/cmd_basic.h"
#include "cmd/cmd_system.h"
#include "cmd/cmd_freertos.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "CMD";

// 命令初始化函数
void cmd_init(void) {
    // 注册基础命令
    cmd_register_task("help", task_help, "显示帮助信息");
    cmd_register_task("echo", task_echo, "回显文本");
    cmd_register_task("version", task_version, "显示版本信息");
    cmd_register_task("clear", task_clear, "清屏");
    cmd_register_task("test", task_test, "参数测试命令");
    cmd_register_task("kv", task_kv, "键值存储操作");
    cmd_register_task("buffer", task_buffer, "显示命令缓冲区");
    
    // 注册系统命令
    cmd_register_task("status", task_status, "显示系统状态");
    cmd_register_task("led", task_led, "控制LED (on/off)");
    cmd_register_task("tasks", task_tasks, "显示所有任务信息");
    cmd_register_task("heap", task_heap, "显示内存使用情况");
    cmd_register_task("uptime", task_uptime, "显示系统运行时间");
    cmd_register_task("cpu", task_cpu, "显示CPU使用率");
    cmd_register_task("reset", task_reset, "重启系统");
    cmd_register_task("delay", task_delay, "延时指定毫秒数");
    
    // 注册FreeRTOS命令
    cmd_register_task("queue", task_queue, "队列操作 (create/send/receive)");
    cmd_register_task("sem", task_sem, "信号量操作 (create/take/give)");
    cmd_register_task("timer", task_timer, "定时器操作 (create/start/stop)");

    ESP_LOGI(TAG, "命令系统初始化完成，已注册 %zu 个任务", (size_t)18);
}
