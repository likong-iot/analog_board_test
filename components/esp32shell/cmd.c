#include "cmd.h"
#include "shell.h"
#include "cmd/cmd_basic.h"
#include "cmd/cmd_system.h"
#include "cmd/cmd_freertos.h"
#include "cmd/cmd_filesystem.h"
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
    
    // 注册文件系统命令
    cmd_register_task("pwd", task_pwd, "显示当前工作目录");
    cmd_register_task("cd", task_cd, "切换工作目录");
    cmd_register_task("ls", task_ls, "列出目录内容");
    cmd_register_task("mkdir", task_mkdir, "创建目录");
    cmd_register_task("rmdir", task_rmdir, "删除空目录");
    cmd_register_task("rm", task_rm, "删除文件");
    cmd_register_task("cp", task_cp, "复制文件");
    cmd_register_task("mv", task_mv, "移动/重命名文件");
    cmd_register_task("cat", task_cat, "显示文件内容");
    cmd_register_task("touch", task_touch, "创建空文件");
    cmd_register_task("du", task_du, "显示目录使用情况");
    cmd_register_task("find", task_find, "查找文件");

    ESP_LOGI(TAG, "命令系统初始化完成，已注册 %zu 个任务", (size_t)30);
}
