#ifndef SHELL_H
#define SHELL_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// 命令缓冲区大小
#define CMD_BUFFER_SIZE 2048
#define MAX_CMD_LENGTH 512
#define MAX_TASKS 32
#define MAX_SHELL_INSTANCES 4

// 命令缓冲区结构
typedef struct cmd_buffer {
  uint8_t buffer[CMD_BUFFER_SIZE];
  size_t head;
  size_t tail;
  size_t count;
  SemaphoreHandle_t mutex;
} cmd_buffer_t;

// 任务函数指针类型
typedef void (*task_func_t)(uint32_t channel_id, const char *params);

// 命令任务结构
typedef struct {
  const char *cmd_name;
  task_func_t task_func;
  const char *description;
} cmd_task_t;

// I/O函数指针类型
typedef void (*shell_output_func_t)(uint32_t channel_id, const uint8_t *data, size_t length);

// Shell配置结构体
typedef struct {
  uint32_t channel_id;                        // 通信通道ID（可以是UART号、网络端口等）
  const char *channel_name;                   // 通道名称（如"UART1", "TCP", "BLE"等）
  shell_output_func_t output_func;            // 输出函数指针
  const char *prompt;                         // 提示符
  bool enabled;                               // 是否启用
  void *user_data;                            // 用户自定义数据
} shell_config_t;

// 命令队列项结构体
typedef struct cmd_queue_item {
  char command[MAX_CMD_LENGTH];
  int line_number;                            // 行号
  struct cmd_queue_item *next;
} cmd_queue_item_t;

// 键值对结构体
typedef struct kv_pair {
  char key[64];
  uint32_t value;
  struct kv_pair *next;
} kv_pair_t;

// 键值存储结构体
typedef struct {
  kv_pair_t *head;
  size_t count;
  SemaphoreHandle_t mutex;
} kv_store_t;

// 命令队列结构体
typedef struct {
  cmd_queue_item_t *head;
  cmd_queue_item_t *tail;
  size_t count;
  SemaphoreHandle_t mutex;
} cmd_queue_t;

// 单个宏结构体
typedef struct macro_item {
  char name[64];                              // 宏名称
  cmd_queue_t queue;                          // 宏命令队列
  struct macro_item *next;                    // 下一个宏
} macro_item_t;

// 宏缓冲区结构体
typedef struct {
  macro_item_t *head;                         // 宏列表头
  size_t count;                               // 宏数量
  bool recording;                             // 是否正在录制
  char current_macro_name[64];                // 当前录制宏名称
  cmd_queue_t temp_queue;                     // 临时录制队列
  SemaphoreHandle_t mutex;                    // 互斥锁
  bool executing;                             // 是否正在执行宏
  uint32_t executing_channel_id;              // 正在执行的通道ID
  SemaphoreHandle_t execution_mutex;          // 执行互斥锁
} macro_buffer_t;

// Shell实例结构体
typedef struct {
  shell_config_t config;                      // 配置信息
  cmd_buffer_t cmd_buffer;                    // 命令缓冲区
  cmd_queue_t cmd_queue;                      // 命令队列
  kv_store_t kv_store;                        // 键值存储
  macro_buffer_t macro_buffer;                // 宏缓冲区
  TaskHandle_t parser_task_handle;            // 解析任务句柄
  TaskHandle_t executor_task_handle;          // 执行任务句柄
  bool initialized;                           // 是否已初始化
} shell_instance_t;

// 全局命令系统初始化
void cmd_system_init(void);

// Shell系统初始化（包含命令系统初始化和命令注册）
void shell_system_init(void);

// Shell实例管理函数
shell_instance_t* shell_create_instance(const shell_config_t *config);
bool shell_start_instance(shell_instance_t *instance);
bool shell_stop_instance(shell_instance_t *instance);
void shell_destroy_instance(shell_instance_t *instance);

// 命令缓冲区操作函数
void cmd_add_data(cmd_buffer_t *buffer, uint8_t *data, size_t length);
bool cmd_get_command(cmd_buffer_t *buffer, char *cmd, size_t max_length);
bool cmd_get_all_commands(cmd_buffer_t *buffer, char *commands, size_t max_length);

// 命令队列操作函数
void cmd_queue_init(cmd_queue_t *queue);
void cmd_queue_enqueue(cmd_queue_t *queue, const char *command);
bool cmd_queue_dequeue(cmd_queue_t *queue, char *command, size_t max_length);
bool cmd_queue_peek(cmd_queue_t *queue, char *command, size_t max_length, size_t index);
void cmd_queue_clear(cmd_queue_t *queue);
size_t cmd_queue_size(cmd_queue_t *queue);
void cmd_queue_list(cmd_queue_t *queue, char *buffer, size_t buffer_size);

// 键值存储操作函数
void kv_store_init(kv_store_t *store);
bool kv_store_set(kv_store_t *store, const char *key, uint32_t value);
bool kv_store_get(kv_store_t *store, const char *key, uint32_t *value);
bool kv_store_delete(kv_store_t *store, const char *key);
void kv_store_clear(kv_store_t *store);
size_t kv_store_count(kv_store_t *store);
void kv_store_list(kv_store_t *store, char *buffer, size_t buffer_size);

// 宏缓冲区操作函数
void macro_buffer_init(macro_buffer_t *macro);
bool macro_buffer_start_recording(macro_buffer_t *macro, const char *macro_name);
bool macro_buffer_stop_recording(macro_buffer_t *macro);
bool macro_buffer_add_command(macro_buffer_t *macro, const char *command);
bool macro_buffer_execute(macro_buffer_t *macro, uint32_t channel_id);
bool macro_buffer_execute_by_name(macro_buffer_t *macro, const char *macro_name, uint32_t channel_id);
bool macro_buffer_delete(macro_buffer_t *macro, const char *macro_name);
void macro_buffer_clear(macro_buffer_t *macro);
size_t macro_buffer_count(macro_buffer_t *macro);
void macro_buffer_list(macro_buffer_t *macro, char *buffer, size_t buffer_size);
bool macro_buffer_is_recording(macro_buffer_t *macro);
bool macro_buffer_exists(macro_buffer_t *macro, const char *macro_name);
void macro_buffer_get_commands(macro_buffer_t *macro, const char *macro_name, char *buffer, size_t buffer_size);

// 宏中断控制函数
bool macro_buffer_stop_execution(macro_buffer_t *macro, uint32_t channel_id);
bool macro_buffer_is_executing(macro_buffer_t *macro, uint32_t channel_id);

// 条件跳转函数
bool macro_buffer_jump_if_not_zero(macro_buffer_t *macro, const char *macro_name, const char *key, int target_line, uint32_t channel_id);

// 命令注册和执行函数
bool cmd_register_task(const char *cmd_name, task_func_t task_func, const char *description);
void cmd_execute(uint32_t channel_id, const char *command);

// 获取已注册的任务列表
const cmd_task_t *cmd_get_task_list(size_t *count);

// 提示符相关函数
void cmd_show_prompt(uint32_t channel_id);
void cmd_show_command(uint32_t channel_id, const char *command);

// 内部使用的输出函数（通过I/O接口调用）
void cmd_output(uint32_t channel_id, const uint8_t *data, size_t length);

// 便捷函数：创建并启动shell实例
shell_instance_t* shell_create_and_start(const shell_config_t *config);

// 便捷函数：停止并销毁shell实例
void shell_stop_and_destroy(shell_instance_t *instance);

// 便捷函数：创建shell配置
shell_config_t create_shell_config(uint32_t channel_id,
                                    const char *channel_name,
                                    const char *prompt,
                                    shell_output_func_t output_func);

// 获取指定通道的shell实例
shell_instance_t* shell_get_instance_by_channel(uint32_t channel_id);

// Shell实例缓冲区操作函数（供外部组件使用）
void shell_add_data_to_instance(uint32_t channel_id, const uint8_t *data, size_t length);
void* shell_get_buffer_from_instance(uint32_t channel_id);

#endif // SHELL_H
