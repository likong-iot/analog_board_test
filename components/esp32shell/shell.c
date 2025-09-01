#include "shell.h"
#include "cmd.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "SHELL";

// 全局任务列表
static cmd_task_t task_list[MAX_TASKS];
static size_t task_count = 0;
static SemaphoreHandle_t task_list_mutex = NULL;

// 全局shell实例列表
static shell_instance_t shell_instances[MAX_SHELL_INSTANCES];
static size_t shell_instance_count = 0;
static SemaphoreHandle_t shell_instances_mutex = NULL;

void cmd_system_init(void) {
  task_list_mutex = xSemaphoreCreateMutex();
  if (task_list_mutex == NULL) {
    ESP_LOGE(TAG, "创建任务列表互斥锁失败");
    return;
  }
  
  shell_instances_mutex = xSemaphoreCreateMutex();
  if (shell_instances_mutex == NULL) {
    ESP_LOGE(TAG, "创建shell实例互斥锁失败");
    vSemaphoreDelete(task_list_mutex);
    task_list_mutex = NULL;
    return;
  }
  
  // 初始化shell实例列表
  memset(shell_instances, 0, sizeof(shell_instances));
  shell_instance_count = 0;
  
  ESP_LOGI(TAG, "命令系统初始化完成");
}

// Shell系统初始化（包含命令系统初始化和命令注册）
void shell_system_init(void) {
  // 初始化命令系统
  cmd_system_init();
  
  // 初始化命令注册
  cmd_init();
  
  ESP_LOGI(TAG, "Shell系统初始化完成");
}

// Shell实例管理函数
shell_instance_t* shell_create_instance(const shell_config_t *config) {
  if (config == NULL) {
    ESP_LOGE(TAG, "配置参数为空");
    return NULL;
  }
  
  if (xSemaphoreTake(shell_instances_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    ESP_LOGE(TAG, "无法获取shell实例锁");
    return NULL;
  }
  
  if (shell_instance_count >= MAX_SHELL_INSTANCES) {
    ESP_LOGE(TAG, "shell实例数量已达上限");
    xSemaphoreGive(shell_instances_mutex);
    return NULL;
  }
  
  // 创建新实例
  shell_instance_t *instance = &shell_instances[shell_instance_count];
  
  // 复制配置
  memcpy(&instance->config, config, sizeof(shell_config_t));
  
  // 初始化命令缓冲区
  instance->cmd_buffer.head = 0;
  instance->cmd_buffer.tail = 0;
  instance->cmd_buffer.count = 0;
  instance->cmd_buffer.mutex = xSemaphoreCreateMutex();
  if (instance->cmd_buffer.mutex == NULL) {
    ESP_LOGE(TAG, "创建命令缓冲区互斥锁失败");
    xSemaphoreGive(shell_instances_mutex);
    return NULL;
  }
  
  // 初始化命令队列
  cmd_queue_init(&instance->cmd_queue);
  
  // 初始化键值存储
  kv_store_init(&instance->kv_store);
  
  // 初始化宏缓冲区
  macro_buffer_init(&instance->macro_buffer);
  
  instance->parser_task_handle = NULL;
  instance->executor_task_handle = NULL;
  instance->initialized = true;
  
  shell_instance_count++;
  
  ESP_LOGI(TAG, "创建shell实例成功，通道ID: %lu, 名称: %s", 
           config->channel_id, config->channel_name ? config->channel_name : "未知");
  xSemaphoreGive(shell_instances_mutex);
  
  return instance;
}

// 命令解析任务函数
static void shell_parser_task(void *arg) {
  shell_instance_t *instance = (shell_instance_t *)arg;
  uint32_t channel_id = instance->config.channel_id;
  const char *prompt = instance->config.prompt ? instance->config.prompt : "shell> ";
  const char *channel_name = instance->config.channel_name ? instance->config.channel_name : "未知";
  
  ESP_LOGI(TAG, "Shell解析任务启动，通道ID: %lu, 名称: %s", channel_id, channel_name);
  
  // 显示初始提示符
  if (instance->config.output_func) {
    instance->config.output_func(channel_id, (uint8_t *)prompt, strlen(prompt));
  }
  
  while (instance->initialized) {
    char all_commands[MAX_CMD_LENGTH * 4]; // 支持多个命令
    if (cmd_get_all_commands(&instance->cmd_buffer, all_commands, sizeof(all_commands))) {
      // 分割命令并加入队列
      char *command = strtok(all_commands, "\r\n");
      
      while (command != NULL) {
        // 跳过空命令
        if (strlen(command) > 0) {
          // 将命令加入队列
          cmd_queue_enqueue(&instance->cmd_queue, command);
        }
        
        // 获取下一个命令
        command = strtok(NULL, "\r\n");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  ESP_LOGI(TAG, "Shell解析任务退出，通道ID: %lu, 名称: %s", channel_id, channel_name);
  vTaskDelete(NULL);
}

// 命令执行任务函数
static void shell_executor_task(void *arg) {
  shell_instance_t *instance = (shell_instance_t *)arg;
  uint32_t channel_id = instance->config.channel_id;
  const char *prompt = instance->config.prompt ? instance->config.prompt : "shell> ";
  const char *channel_name = instance->config.channel_name ? instance->config.channel_name : "未知";
  
  ESP_LOGI(TAG, "Shell执行任务启动，通道ID: %lu, 名称: %s", channel_id, channel_name);
  
  while (instance->initialized) {
    char command[MAX_CMD_LENGTH];
    
    // 从命令队列中取出命令执行
    if (cmd_queue_dequeue(&instance->cmd_queue, command, MAX_CMD_LENGTH)) {
      // 显示输入的命令
      if (instance->config.output_func) {
        char display_cmd[MAX_CMD_LENGTH + 10];
        snprintf(display_cmd, sizeof(display_cmd), "%s\r\n", command);
        instance->config.output_func(channel_id, (uint8_t *)display_cmd, strlen(display_cmd));
      }
      
      // 执行命令
      cmd_execute(channel_id, command);
      
      // 显示新的提示符
      if (instance->config.output_func) {
        instance->config.output_func(channel_id, (uint8_t *)prompt, strlen(prompt));
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  ESP_LOGI(TAG, "Shell执行任务退出，通道ID: %lu, 名称: %s", channel_id, channel_name);
  vTaskDelete(NULL);
}

bool shell_start_instance(shell_instance_t *instance) {
  if (instance == NULL || !instance->initialized) {
    ESP_LOGE(TAG, "无效的shell实例");
    return false;
  }
  
  if (instance->parser_task_handle != NULL) {
    ESP_LOGW(TAG, "Shell实例已在运行");
    return true;
  }
  
  // 创建解析任务
  BaseType_t ret = xTaskCreate(shell_parser_task, 
                               "shell_parser", 
                               8192, 
                               instance, 
                               4, 
                               &instance->parser_task_handle);
  
  if (ret != pdPASS) {
    ESP_LOGE(TAG, "创建shell解析任务失败");
    return false;
  }
  
  // 创建执行任务
  ret = xTaskCreate(shell_executor_task, 
                    "shell_executor", 
                    8192, 
                    instance, 
                    3, 
                    &instance->executor_task_handle);
  
  if (ret != pdPASS) {
    ESP_LOGE(TAG, "创建shell执行任务失败");
    vTaskDelete(instance->parser_task_handle);
    instance->parser_task_handle = NULL;
    return false;
  }
  
  ESP_LOGI(TAG, "启动shell实例成功，通道ID: %lu, 名称: %s", 
           instance->config.channel_id, 
           instance->config.channel_name ? instance->config.channel_name : "未知");
  return true;
}

bool shell_stop_instance(shell_instance_t *instance) {
  if (instance == NULL || !instance->initialized) {
    return false;
  }
  
  if (instance->parser_task_handle != NULL || instance->executor_task_handle != NULL) {
    instance->initialized = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待任务退出
    
    if (instance->parser_task_handle != NULL) {
      vTaskDelete(instance->parser_task_handle);
      instance->parser_task_handle = NULL;
    }
    
    if (instance->executor_task_handle != NULL) {
      vTaskDelete(instance->executor_task_handle);
      instance->executor_task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "停止shell实例成功，通道ID: %lu, 名称: %s", 
             instance->config.channel_id,
             instance->config.channel_name ? instance->config.channel_name : "未知");
  }
  
  return true;
}

void shell_destroy_instance(shell_instance_t *instance) {
  if (instance == NULL) {
    return;
  }
  
  // 先停止实例
  shell_stop_instance(instance);
  
  // 清理资源
  if (instance->cmd_buffer.mutex != NULL) {
    vSemaphoreDelete(instance->cmd_buffer.mutex);
    instance->cmd_buffer.mutex = NULL;
  }
  
  // 清理命令队列
  cmd_queue_clear(&instance->cmd_queue);
  if (instance->cmd_queue.mutex != NULL) {
    vSemaphoreDelete(instance->cmd_queue.mutex);
    instance->cmd_queue.mutex = NULL;
  }
  
  // 清理键值存储
  kv_store_clear(&instance->kv_store);
  if (instance->kv_store.mutex != NULL) {
    vSemaphoreDelete(instance->kv_store.mutex);
    instance->kv_store.mutex = NULL;
  }
  
  // 清理宏缓冲区
  macro_buffer_clear(&instance->macro_buffer);
  if (instance->macro_buffer.temp_queue.mutex != NULL) {
    vSemaphoreDelete(instance->macro_buffer.temp_queue.mutex);
    instance->macro_buffer.temp_queue.mutex = NULL;
  }
  if (instance->macro_buffer.mutex != NULL) {
    vSemaphoreDelete(instance->macro_buffer.mutex);
    instance->macro_buffer.mutex = NULL;
  }
  
  // 从实例列表中移除
  if (xSemaphoreTake(shell_instances_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    for (size_t i = 0; i < shell_instance_count; i++) {
      if (&shell_instances[i] == instance) {
        // 将最后一个实例移到当前位置
        if (i < shell_instance_count - 1) {
          memcpy(&shell_instances[i], &shell_instances[shell_instance_count - 1], sizeof(shell_instance_t));
        }
        shell_instance_count--;
        break;
      }
    }
    xSemaphoreGive(shell_instances_mutex);
  }
  
  memset(instance, 0, sizeof(shell_instance_t));
  ESP_LOGI(TAG, "销毁shell实例成功");
}

// 便捷函数
shell_instance_t* shell_create_and_start(const shell_config_t *config) {
  shell_instance_t *instance = shell_create_instance(config);
  if (instance != NULL) {
    if (!shell_start_instance(instance)) {
      shell_destroy_instance(instance);
      return NULL;
    }
  }
  return instance;
}

void shell_stop_and_destroy(shell_instance_t *instance) {
  if (instance != NULL) {
    shell_stop_instance(instance);
    shell_destroy_instance(instance);
  }
}

// 命令缓冲区操作函数
void cmd_add_data(cmd_buffer_t *buffer, uint8_t *data, size_t length) {
  if (xSemaphoreTake(buffer->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }

  for (size_t i = 0; i < length; i++) {
    if (buffer->count < CMD_BUFFER_SIZE) {
      buffer->buffer[buffer->head] = data[i];
      buffer->head = (buffer->head + 1) % CMD_BUFFER_SIZE;
      buffer->count++;
    } else {
      // 缓冲区满，丢弃最旧的数据
      buffer->tail = (buffer->tail + 1) % CMD_BUFFER_SIZE;
      buffer->buffer[buffer->head] = data[i];
      buffer->head = (buffer->head + 1) % CMD_BUFFER_SIZE;
    }
  }

  xSemaphoreGive(buffer->mutex);
}

bool cmd_get_command(cmd_buffer_t *buffer, char *cmd, size_t max_length) {
  if (xSemaphoreTake(buffer->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }

  bool found_cmd = false;
  size_t cmd_len = 0;
  size_t temp_tail = buffer->tail;
  size_t temp_count = buffer->count;

  // 如果有数据，就尝试解析命令
  if (temp_count > 0) {
    // 查找换行符或回车符
    bool found_eol = false;
    size_t search_count = temp_count;
    size_t search_tail = temp_tail;
    
    while (search_count > 0 && cmd_len < max_length - 1) {
      char c = buffer->buffer[search_tail];
      search_tail = (search_tail + 1) % CMD_BUFFER_SIZE;
      search_count--;
      
      if (c == '\n' || c == '\r') {
        found_eol = true;
        break;
      }

      cmd[cmd_len++] = c;
    }

    if (cmd_len > 0) {
      found_cmd = true;
      cmd[cmd_len] = '\0';
      
      // 移除已处理的数据（包括换行符）
      size_t remove_count = cmd_len + (found_eol ? 1 : 0);
      while (remove_count > 0 && buffer->count > 0) {
        buffer->tail = (buffer->tail + 1) % CMD_BUFFER_SIZE;
        buffer->count--;
        remove_count--;
      }
      
      ESP_LOGI(TAG, "找到命令: '%s'", cmd);
    }
  }

  xSemaphoreGive(buffer->mutex);
  return found_cmd;
}

// 新增：获取所有可用命令（以换行符分隔）
bool cmd_get_all_commands(cmd_buffer_t *buffer, char *commands, size_t max_length) {
  if (xSemaphoreTake(buffer->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }

  bool found_commands = false;
  size_t commands_len = 0;
  size_t temp_tail = buffer->tail;
  size_t temp_count = buffer->count;

  // 如果有数据，就尝试解析所有命令
  if (temp_count > 0) {
    size_t search_count = temp_count;
    size_t search_tail = temp_tail;
    
    while (search_count > 0 && commands_len < max_length - 1) {
      char c = buffer->buffer[search_tail];
      search_tail = (search_tail + 1) % CMD_BUFFER_SIZE;
      search_count--;
      
      if (c == '\n' || c == '\r') {
        // 遇到换行符，检查是否有下一个命令
        if (search_count > 0) {
          // 还有更多数据，继续读取
          commands[commands_len++] = c;
        } else {
          // 没有更多数据，结束
          break;
        }
      } else {
        commands[commands_len++] = c;
      }
    }

    if (commands_len > 0) {
      found_commands = true;
      commands[commands_len] = '\0';
      
      // 移除已处理的所有数据
      size_t remove_count = commands_len;
      while (remove_count > 0 && buffer->count > 0) {
        buffer->tail = (buffer->tail + 1) % CMD_BUFFER_SIZE;
        buffer->count--;
        remove_count--;
      }
      
      ESP_LOGI(TAG, "找到多个命令: '%s'", commands);
    }
  }

  xSemaphoreGive(buffer->mutex);
  return found_commands;
}

// 命令队列操作函数实现
void cmd_queue_init(cmd_queue_t *queue) {
  queue->head = NULL;
  queue->tail = NULL;
  queue->count = 0;
  queue->mutex = xSemaphoreCreateMutex();
}

void cmd_queue_enqueue(cmd_queue_t *queue, const char *command) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  cmd_queue_item_t *item = malloc(sizeof(cmd_queue_item_t));
  if (item != NULL) {
    strncpy(item->command, command, MAX_CMD_LENGTH - 1);
    item->command[MAX_CMD_LENGTH - 1] = '\0';
    item->line_number = queue->count + 1;  // 自动分配行号
    item->next = NULL;
    
    if (queue->tail == NULL) {
      queue->head = item;
      queue->tail = item;
    } else {
      queue->tail->next = item;
      queue->tail = item;
    }
    queue->count++;
    
    ESP_LOGI(TAG, "命令已加入队列: '%s'", command);
  }
  
  xSemaphoreGive(queue->mutex);
}

bool cmd_queue_dequeue(cmd_queue_t *queue, char *command, size_t max_length) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  
  bool success = false;
  if (queue->head != NULL) {
    cmd_queue_item_t *item = queue->head;
    strncpy(command, item->command, max_length - 1);
    command[max_length - 1] = '\0';
    
    queue->head = item->next;
    if (queue->head == NULL) {
      queue->tail = NULL;
    }
    queue->count--;
    
    free(item);
    success = true;
    
    ESP_LOGI(TAG, "命令已从队列取出: '%s'", command);
  }
  
  xSemaphoreGive(queue->mutex);
  return success;
}

bool cmd_queue_peek(cmd_queue_t *queue, char *command, size_t max_length, size_t index) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  
  bool success = false;
  cmd_queue_item_t *current = queue->head;
  size_t current_index = 0;
  
  while (current != NULL && current_index < index) {
    current = current->next;
    current_index++;
  }
  
  if (current != NULL) {
    strncpy(command, current->command, max_length - 1);
    command[max_length - 1] = '\0';
    success = true;
  }
  
  xSemaphoreGive(queue->mutex);
  return success;
}

void cmd_queue_clear(cmd_queue_t *queue) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  while (queue->head != NULL) {
    cmd_queue_item_t *item = queue->head;
    queue->head = item->next;
    free(item);
  }
  queue->tail = NULL;
  queue->count = 0;
  
  xSemaphoreGive(queue->mutex);
}

size_t cmd_queue_size(cmd_queue_t *queue) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return 0;
  }
  
  size_t size = queue->count;
  xSemaphoreGive(queue->mutex);
  return size;
}

void cmd_queue_list(cmd_queue_t *queue, char *buffer, size_t buffer_size) {
  if (xSemaphoreTake(queue->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  size_t offset = 0;
  cmd_queue_item_t *current = queue->head;
  int index = 0;
  
  while (current != NULL && offset < buffer_size - 1) {
    int written = snprintf(buffer + offset, buffer_size - offset, 
                          "[%d] %s\r\n", current->line_number, current->command);
    if (written > 0 && offset + written < buffer_size) {
      offset += written;
    } else {
      break;
    }
    current = current->next;
    index++;
  }
  
  xSemaphoreGive(queue->mutex);
}

// 键值存储操作函数实现
void kv_store_init(kv_store_t *store) {
  store->head = NULL;
  store->count = 0;
  store->mutex = xSemaphoreCreateMutex();
}

bool kv_store_set(kv_store_t *store, const char *key, uint32_t value) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  
  bool success = false;
  
  // 查找是否已存在该键
  kv_pair_t *current = store->head;
  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      // 更新现有键值对
      current->value = value;
      success = true;
      ESP_LOGI(TAG, "更新键值对: %s = %lu", key, (unsigned long)value);
      break;
    }
    current = current->next;
  }
  
  if (!success) {
    // 创建新的键值对
    kv_pair_t *new_pair = malloc(sizeof(kv_pair_t));
    if (new_pair != NULL) {
      strncpy(new_pair->key, key, sizeof(new_pair->key) - 1);
      new_pair->key[sizeof(new_pair->key) - 1] = '\0';
      new_pair->value = value;
      new_pair->next = store->head;
      store->head = new_pair;
      store->count++;
      success = true;
      ESP_LOGI(TAG, "创建键值对: %s = %lu", key, (unsigned long)value);
    }
  }
  
  xSemaphoreGive(store->mutex);
  return success;
}

bool kv_store_get(kv_store_t *store, const char *key, uint32_t *value) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  
  bool found = false;
  kv_pair_t *current = store->head;
  
  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      *value = current->value;
      found = true;
      ESP_LOGI(TAG, "获取键值对: %s = %lu", key, (unsigned long)current->value);
      break;
    }
    current = current->next;
  }
  
  xSemaphoreGive(store->mutex);
  return found;
}

bool kv_store_delete(kv_store_t *store, const char *key) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return false;
  }
  
  bool deleted = false;
  kv_pair_t *current = store->head;
  kv_pair_t *prev = NULL;
  
  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      if (prev == NULL) {
        // 删除头节点
        store->head = current->next;
      } else {
        // 删除中间节点
        prev->next = current->next;
      }
      
      ESP_LOGI(TAG, "删除键值对: %s = %lu", key, (unsigned long)current->value);
      free(current);
      store->count--;
      deleted = true;
      break;
    }
    prev = current;
    current = current->next;
  }
  
  xSemaphoreGive(store->mutex);
  return deleted;
}

void kv_store_clear(kv_store_t *store) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  while (store->head != NULL) {
    kv_pair_t *temp = store->head;
    store->head = temp->next;
    free(temp);
  }
  store->count = 0;
  
  xSemaphoreGive(store->mutex);
  ESP_LOGI(TAG, "清空键值存储");
}

size_t kv_store_count(kv_store_t *store) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return 0;
  }
  
  size_t count = store->count;
  xSemaphoreGive(store->mutex);
  return count;
}

void kv_store_list(kv_store_t *store, char *buffer, size_t buffer_size) {
  if (xSemaphoreTake(store->mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return;
  }
  
  size_t offset = 0;
  kv_pair_t *current = store->head;
  
  while (current != NULL && offset < buffer_size - 1) {
    int written = snprintf(buffer + offset, buffer_size - offset, 
                          "%s = %lu\r\n", current->key, (unsigned long)current->value);
    if (written > 0 && offset + written < buffer_size) {
      offset += written;
    } else {
      break;
    }
    current = current->next;
  }
  
  xSemaphoreGive(store->mutex);
}

// 宏缓冲区操作函数实现
void macro_buffer_init(macro_buffer_t *macro) {
  macro->head = NULL;
  macro->count = 0;
  macro->recording = false;
  memset(macro->current_macro_name, 0, sizeof(macro->current_macro_name));
  cmd_queue_init(&macro->temp_queue);
  macro->mutex = xSemaphoreCreateMutex();
  macro->executing = false;
  macro->executing_channel_id = 0;
  macro->execution_mutex = xSemaphoreCreateMutex();
}

bool macro_buffer_start_recording(macro_buffer_t *macro, const char *macro_name) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  if (macro->recording) {
    xSemaphoreGive(macro->mutex);
    return false; // 已经在录制中
  }
  
  // 检查宏名称是否已存在
  if (macro_buffer_exists(macro, macro_name)) {
    xSemaphoreGive(macro->mutex);
    return false; // 宏名称已存在
  }
  
  macro->recording = true;
  strncpy(macro->current_macro_name, macro_name, sizeof(macro->current_macro_name) - 1);
  macro->current_macro_name[sizeof(macro->current_macro_name) - 1] = '\0';
  
  ESP_LOGI(TAG, "开始录制宏: %s", macro->current_macro_name);
  xSemaphoreGive(macro->mutex);
  return true;
}

bool macro_buffer_stop_recording(macro_buffer_t *macro) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  if (!macro->recording) {
    xSemaphoreGive(macro->mutex);
    return false; // 没有在录制
  }
  
  // 创建新的宏项
  macro_item_t *new_macro = (macro_item_t *)malloc(sizeof(macro_item_t));
  if (new_macro == NULL) {
    xSemaphoreGive(macro->mutex);
    return false;
  }
  
  strncpy(new_macro->name, macro->current_macro_name, sizeof(new_macro->name) - 1);
  new_macro->name[sizeof(new_macro->name) - 1] = '\0';
  
  // 复制命令队列
  cmd_queue_init(&new_macro->queue);
  char command[MAX_CMD_LENGTH];
  while (cmd_queue_dequeue(&macro->temp_queue, command, MAX_CMD_LENGTH)) {
    cmd_queue_enqueue(&new_macro->queue, command);
  }
  
  // 添加到宏列表
  new_macro->next = macro->head;
  macro->head = new_macro;
  macro->count++;
  
  macro->recording = false;
  memset(macro->current_macro_name, 0, sizeof(macro->current_macro_name));
  
  ESP_LOGI(TAG, "停止录制宏: %s", new_macro->name);
  xSemaphoreGive(macro->mutex);
  return true;
}

bool macro_buffer_add_command(macro_buffer_t *macro, const char *command) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  if (!macro->recording) {
    xSemaphoreGive(macro->mutex);
    return false; // 没有在录制
  }
  
  cmd_queue_enqueue(&macro->temp_queue, command);
  xSemaphoreGive(macro->mutex);
  return true;
}

bool macro_buffer_execute(macro_buffer_t *macro, uint32_t channel_id) {
  // 执行第一个宏（如果有的话）
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  if (macro->head == NULL) {
    xSemaphoreGive(macro->mutex);
    return false; // 没有宏
  }
  
  // 检查是否已经在执行
  if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    xSemaphoreGive(macro->mutex);
    return false;
  }
  
  if (macro->executing) {
    xSemaphoreGive(macro->execution_mutex);
    xSemaphoreGive(macro->mutex);
    return false; // 已经在执行
  }
  
  macro->executing = true;
  macro->executing_channel_id = channel_id;
  xSemaphoreGive(macro->execution_mutex);
  
  macro_item_t *first_macro = macro->head;
  char command[MAX_CMD_LENGTH];
  bool executed = false;
  size_t command_count = cmd_queue_size(&first_macro->queue);
  
  // 遍历所有命令并执行，但不删除
  for (size_t i = 0; i < command_count; i++) {
    // 检查是否被中断
    if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      if (!macro->executing) {
        xSemaphoreGive(macro->execution_mutex);
        ESP_LOGI(TAG, "宏执行被中断");
        break;
      }
      xSemaphoreGive(macro->execution_mutex);
    }
    
    if (cmd_queue_peek(&first_macro->queue, command, MAX_CMD_LENGTH, i)) {
      cmd_execute(channel_id, command);
      executed = true;
    }
  }
  
  // 清除执行状态
  if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    macro->executing = false;
    macro->executing_channel_id = 0;
    xSemaphoreGive(macro->execution_mutex);
  }
  
  if (executed) {
    ESP_LOGI(TAG, "执行宏: %s (执行了 %zu 条命令)", first_macro->name, command_count);
  }
  
  xSemaphoreGive(macro->mutex);
  return executed;
}

bool macro_buffer_execute_by_name(macro_buffer_t *macro, const char *macro_name, uint32_t channel_id) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  macro_item_t *current = macro->head;
  while (current != NULL) {
    if (strcmp(current->name, macro_name) == 0) {
      // 检查是否已经在执行
      if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        xSemaphoreGive(macro->mutex);
        return false;
      }
      
      if (macro->executing) {
        xSemaphoreGive(macro->execution_mutex);
        xSemaphoreGive(macro->mutex);
        return false; // 已经在执行
      }
      
      macro->executing = true;
      macro->executing_channel_id = channel_id;
      xSemaphoreGive(macro->execution_mutex);
      
      char command[MAX_CMD_LENGTH];
      bool executed = false;
      size_t command_count = cmd_queue_size(&current->queue);
      
      // 遍历所有命令并执行，但不删除
      for (size_t i = 0; i < command_count; i++) {
        // 检查是否被中断
        if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(1)) == pdTRUE) {
          if (!macro->executing) {
            xSemaphoreGive(macro->execution_mutex);
            ESP_LOGI(TAG, "宏执行被中断");
            break;
          }
          xSemaphoreGive(macro->execution_mutex);
        }
        
        if (cmd_queue_peek(&current->queue, command, MAX_CMD_LENGTH, i)) {
          cmd_execute(channel_id, command);
          executed = true;
        }
      }
      
      // 清除执行状态
      if (xSemaphoreTake(macro->execution_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        macro->executing = false;
        macro->executing_channel_id = 0;
        xSemaphoreGive(macro->execution_mutex);
      }
      
      if (executed) {
        ESP_LOGI(TAG, "执行宏: %s (执行了 %zu 条命令)", current->name, command_count);
      }
      
      xSemaphoreGive(macro->mutex);
      return executed;
    }
    current = current->next;
  }
  
  xSemaphoreGive(macro->mutex);
  return false; // 宏不存在
}

bool macro_buffer_delete(macro_buffer_t *macro, const char *macro_name) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  macro_item_t *current = macro->head;
  macro_item_t *prev = NULL;
  
  while (current != NULL) {
    if (strcmp(current->name, macro_name) == 0) {
      if (prev == NULL) {
        macro->head = current->next;
      } else {
        prev->next = current->next;
      }
      
      cmd_queue_clear(&current->queue);
      free(current);
      macro->count--;
      
      ESP_LOGI(TAG, "删除宏: %s", macro_name);
      xSemaphoreGive(macro->mutex);
      return true;
    }
    prev = current;
    current = current->next;
  }
  
  xSemaphoreGive(macro->mutex);
  return false; // 宏不存在
}

void macro_buffer_clear(macro_buffer_t *macro) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return;
  }
  
  macro_item_t *current = macro->head;
  while (current != NULL) {
    macro_item_t *next = current->next;
    cmd_queue_clear(&current->queue);
    free(current);
    current = next;
  }
  
  macro->head = NULL;
  macro->count = 0;
  macro->recording = false;
  memset(macro->current_macro_name, 0, sizeof(macro->current_macro_name));
  
  ESP_LOGI(TAG, "清空所有宏");
  xSemaphoreGive(macro->mutex);
}

size_t macro_buffer_count(macro_buffer_t *macro) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return 0;
  }
  
  size_t count = macro->count;
  xSemaphoreGive(macro->mutex);
  return count;
}

void macro_buffer_list(macro_buffer_t *macro, char *buffer, size_t buffer_size) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return;
  }
  
  size_t offset = 0;
  
  if (macro->recording) {
    offset += snprintf(buffer + offset, buffer_size - offset, 
                      "【正在录制】宏: %s\r\n", macro->current_macro_name);
  }
  
  if (macro->count > 0) {
    offset += snprintf(buffer + offset, buffer_size - offset, 
                      "已保存的宏 (%zu个):\r\n", macro->count);
    
    macro_item_t *current = macro->head;
    int index = 1;
    while (current != NULL && offset < buffer_size - 100) {
      size_t cmd_count = cmd_queue_size(&current->queue);
      offset += snprintf(buffer + offset, buffer_size - offset,
                        "%d. %s (包含 %zu 个命令)\r\n", index++, current->name, cmd_count);
      current = current->next;
    }
  } else if (!macro->recording) {
    offset += snprintf(buffer + offset, buffer_size - offset, "没有保存的宏\r\n");
  }
  
  xSemaphoreGive(macro->mutex);
}

bool macro_buffer_is_recording(macro_buffer_t *macro) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  bool recording = macro->recording;
  xSemaphoreGive(macro->mutex);
  return recording;
}

bool macro_buffer_exists(macro_buffer_t *macro, const char *macro_name) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  macro_item_t *current = macro->head;
  while (current != NULL) {
    if (strcmp(current->name, macro_name) == 0) {
      xSemaphoreGive(macro->mutex);
      return true;
    }
    current = current->next;
  }
  
  xSemaphoreGive(macro->mutex);
  return false;
}

void macro_buffer_get_commands(macro_buffer_t *macro, const char *macro_name, char *buffer, size_t buffer_size) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    snprintf(buffer, buffer_size, "错误: 无法访问宏缓冲区\r\n");
    return;
  }
  
  macro_item_t *current = macro->head;
  while (current != NULL) {
    if (strcmp(current->name, macro_name) == 0) {
      // 直接显示命令列表，不修改队列
      cmd_queue_list(&current->queue, buffer, buffer_size);
      xSemaphoreGive(macro->mutex);
      return;
    }
    current = current->next;
  }
  
  snprintf(buffer, buffer_size, "错误: 宏 '%s' 不存在\r\n", macro_name);
  xSemaphoreGive(macro->mutex);
}

bool macro_buffer_jump_if_not_zero(macro_buffer_t *macro, const char *macro_name, const char *key, int target_line, uint32_t channel_id) {
  if (xSemaphoreTake(macro->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }
  
  // 获取shell实例来访问kv_store
  shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
  if (instance == NULL) {
    xSemaphoreGive(macro->mutex);
    return false;
  }
  
  // 查找指定的宏
  macro_item_t *current = macro->head;
  while (current != NULL) {
    if (strcmp(current->name, macro_name) == 0) {
      // 获取键值
      uint32_t value = 0;
      bool key_exists = kv_store_get(&instance->kv_store, key, &value);
      
      if (!key_exists) {
        xSemaphoreGive(macro->mutex);
        return false; // 键不存在
      }
      
      // 如果值不等于0，执行跳转
      if (value != 0) {
        // 创建临时队列用于跳转执行
        cmd_queue_t temp_queue;
        cmd_queue_init(&temp_queue);
        
        // 复制命令到临时队列
        cmd_queue_item_t *item = current->queue.head;
        int current_line = 1;
        
        while (item != NULL) {
          if (current_line >= target_line) {
            cmd_queue_enqueue(&temp_queue, item->command);
          }
          item = item->next;
          current_line++;
        }
        
        // 执行临时队列中的命令
        char command[MAX_CMD_LENGTH];
        bool executed = false;
        size_t temp_count = cmd_queue_size(&temp_queue);
        
        for (size_t i = 0; i < temp_count; i++) {
          if (cmd_queue_peek(&temp_queue, command, MAX_CMD_LENGTH, i)) {
            cmd_execute(channel_id, command);
            executed = true;
          }
        }
        
        // 清理临时队列
        cmd_queue_clear(&temp_queue);
        
        if (executed) {
          ESP_LOGI(TAG, "宏 '%s' 跳转到第 %d 行执行完成", macro_name, target_line);
        }
        
        xSemaphoreGive(macro->mutex);
        return executed;
      } else {
        // 值等于0，不跳转
        ESP_LOGI(TAG, "宏 '%s' 键 '%s' 值为0，不执行跳转", macro_name, key);
        xSemaphoreGive(macro->mutex);
        return true; // 成功处理，只是不跳转
      }
    }
    current = current->next;
  }
  
  xSemaphoreGive(macro->mutex);
  return false; // 宏不存在
}

bool cmd_register_task(const char *cmd_name, task_func_t task_func, const char *description) {
  if (xSemaphoreTake(task_list_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }

  if (task_count >= MAX_TASKS) {
    ESP_LOGE(TAG, "任务列表已满，无法注册新任务");
    xSemaphoreGive(task_list_mutex);
    return false;
  }

  // 检查是否已存在同名任务
  for (size_t i = 0; i < task_count; i++) {
    if (strcmp(task_list[i].cmd_name, cmd_name) == 0) {
      ESP_LOGW(TAG, "任务 '%s' 已存在，将被覆盖", cmd_name);
      task_list[i].task_func = task_func;
      task_list[i].description = description;
      xSemaphoreGive(task_list_mutex);
      return true;
    }
  }

  // 添加新任务
  task_list[task_count].cmd_name = cmd_name;
  task_list[task_count].task_func = task_func;
  task_list[task_count].description = description;
  task_count++;

  ESP_LOGI(TAG, "注册任务: %s", cmd_name);
  xSemaphoreGive(task_list_mutex);
  return true;
}

void cmd_execute(uint32_t channel_id, const char *command) {
  if (command == NULL || strlen(command) == 0) {
    return;
  }

  // 获取shell实例
  shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
  if (instance == NULL) {
    return;
  }

  // 解析命令和参数
  char cmd_copy[MAX_CMD_LENGTH];
  strncpy(cmd_copy, command, MAX_CMD_LENGTH - 1);
  cmd_copy[MAX_CMD_LENGTH - 1] = '\0';

  char *cmd_name = strtok(cmd_copy, " \t");
  char *params = strtok(NULL, "");

  if (cmd_name == NULL) {
    return;
  }

  // 检查是否是宏开始命令
  if (strcmp(cmd_name, "macro") == 0) {
    if (params != NULL && strlen(params) > 0) {
      if (macro_buffer_start_recording(&instance->macro_buffer, params)) {
        char response[256];
        snprintf(response, sizeof(response), "开始录制宏: %s\r\n", params);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
      } else {
        char response[256];
        snprintf(response, sizeof(response), "错误: 已经在录制宏\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
      }
    } else {
      char response[256];
      snprintf(response, sizeof(response), "用法: macro <宏名称>\r\n");
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    return;
  }

  // 检查是否是宏结束命令
  if (strcmp(cmd_name, "endmacro") == 0) {
    if (macro_buffer_stop_recording(&instance->macro_buffer)) {
      char response[256];
      snprintf(response, sizeof(response), "停止录制宏\r\n");
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    } else {
      char response[256];
      snprintf(response, sizeof(response), "错误: 没有在录制宏\r\n");
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    return;
  }

    // 检查是否是宏执行命令
  if (strcmp(cmd_name, "exec") == 0) {
    if (params != NULL) {
      if (strcmp(params, "macro") == 0) {
        // 执行第一个宏
        if (macro_buffer_execute(&instance->macro_buffer, channel_id)) {
          char response[256];
          snprintf(response, sizeof(response), "宏执行完成\r\n");
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        } else {
          char response[256];
          snprintf(response, sizeof(response), "错误: 没有可执行的宏\r\n");
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
      } else {
        // 按名称执行宏
        if (macro_buffer_execute_by_name(&instance->macro_buffer, params, channel_id)) {
          char response[256];
          snprintf(response, sizeof(response), "宏 '%s' 执行完成\r\n", params);
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        } else {
          char response[256];
          snprintf(response, sizeof(response), "错误: 宏 '%s' 不存在\r\n", params);
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
            }
    } else {
      char response[256];
      snprintf(response, sizeof(response), "用法: exec macro 或 exec <宏名称>\r\n");
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    return;
  }

  // 检查是否是条件跳转命令
  if (strcmp(cmd_name, "jump") == 0) {
    if (params != NULL) {
      char macro_name[64], key[64];
      int target_line;
      
      if (sscanf(params, "%s %s %d", macro_name, key, &target_line) == 3) {
        if (macro_buffer_jump_if_not_zero(&instance->macro_buffer, macro_name, key, target_line, channel_id)) {
          char response[256];
          snprintf(response, sizeof(response), "条件跳转命令执行完成\r\n");
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        } else {
          char response[256];
          snprintf(response, sizeof(response), "错误: 条件跳转失败\r\n");
          cmd_output(channel_id, (uint8_t *)response, strlen(response));
        }
      } else {
        char response[256];
        snprintf(response, sizeof(response), "用法: jump <宏名称> <键名> <目标行号>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
      }
    } else {
      char response[256];
      snprintf(response, sizeof(response), "用法: jump <宏名称> <键名> <目标行号>\r\n");
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    return;
  }

  // 如果正在录制宏，将命令添加到宏缓冲区而不是执行
  if (macro_buffer_is_recording(&instance->macro_buffer)) {
    if (macro_buffer_add_command(&instance->macro_buffer, command)) {
      char response[256];
      snprintf(response, sizeof(response), "已添加到宏: %s\r\n", command);
      cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    return;
  }

  // 查找并执行任务
  bool found = false;
  for (size_t i = 0; i < task_count; i++) {
    if (strcmp(task_list[i].cmd_name, cmd_name) == 0) {
      ESP_LOGI(TAG, "执行任务: %s", cmd_name);
      task_list[i].task_func(channel_id, params ? params : "");
      found = true;
      break;
    }
  }

  if (!found) {
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "未知命令: %s\r\n", cmd_name);
    cmd_output(channel_id, (uint8_t *)error_msg, strlen(error_msg));
  }
}

const cmd_task_t *cmd_get_task_list(size_t *count) {
  if (count != NULL) {
    *count = task_count;
  }
  return task_list;
}

void cmd_show_prompt(uint32_t channel_id) {
  const char *prompt = "esp32shell> ";
  cmd_output(channel_id, (uint8_t *)prompt, strlen(prompt));
}

void cmd_show_command(uint32_t channel_id, const char *command) {
  char display_cmd[MAX_CMD_LENGTH + 10];
  snprintf(display_cmd, sizeof(display_cmd), "执行: %s\r\n", command);
  cmd_output(channel_id, (uint8_t *)display_cmd, strlen(display_cmd));
}

// 内部输出函数，通过I/O接口发送数据
void cmd_output(uint32_t channel_id, const uint8_t *data, size_t length) {
  // 从shell实例中查找对应的输出函数
  if (xSemaphoreTake(shell_instances_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    for (size_t i = 0; i < shell_instance_count; i++) {
      if (shell_instances[i].initialized && 
          shell_instances[i].config.channel_id == channel_id &&
          shell_instances[i].config.output_func != NULL) {
        shell_instances[i].config.output_func(channel_id, data, length);
        xSemaphoreGive(shell_instances_mutex);
        return;
      }
    }
    xSemaphoreGive(shell_instances_mutex);
  }
  
  ESP_LOGW(TAG, "未找到通道 %lu 的输出函数", (unsigned long)channel_id);
}

// 获取指定通道的shell实例
shell_instance_t* shell_get_instance_by_channel(uint32_t channel_id) {
  if (xSemaphoreTake(shell_instances_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    return NULL;
  }
  
  for (size_t i = 0; i < shell_instance_count; i++) {
    if (shell_instances[i].initialized && 
        shell_instances[i].config.channel_id == channel_id) {
      xSemaphoreGive(shell_instances_mutex);
      return &shell_instances[i];
    }
  }
  
  xSemaphoreGive(shell_instances_mutex);
  return NULL;
}

// 便捷函数：创建shell配置
shell_config_t create_shell_config(uint32_t channel_id,
                                    const char *channel_name,
                                    const char *prompt,
                                    shell_output_func_t output_func) {
  return (shell_config_t){.channel_id = channel_id,
                           .channel_name = channel_name,
                           .output_func = output_func,
                           .prompt = prompt,
                           .enabled = true,
                           .user_data = NULL};
}

// Shell实例缓冲区操作函数（供外部组件使用）
void shell_add_data_to_instance(uint32_t channel_id, const uint8_t *data, size_t length) {
  shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
  if (instance != NULL && instance->initialized) {
    cmd_add_data(&instance->cmd_buffer, (uint8_t *)data, length);
  } else {
    ESP_LOGW(TAG, "未找到通道 %lu 的shell实例", (unsigned long)channel_id);
  }
}

void* shell_get_buffer_from_instance(uint32_t channel_id) {
  shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
  if (instance != NULL && instance->initialized) {
    return &instance->cmd_buffer;
  } else {
    ESP_LOGW(TAG, "未找到通道 %lu 的shell实例", (unsigned long)channel_id);
    return NULL;
  }
}


