/**
 * @file key.c
 * @brief 按键驱动模块实现
 */

#include "key.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "KEY_DRIVER";

// 按键状态管理
static key_state_t current_key_state = KEY_STATE_RELEASED;
static key_state_t last_key_state = KEY_STATE_RELEASED;
static key_event_callback_t event_callback = NULL;
static TaskHandle_t key_task_handle = NULL;
static bool detection_running = false;
static SemaphoreHandle_t key_mutex = NULL;

/**
 * @brief 按键检测任务
 */
static void key_detection_task(void *arg)
{
    ESP_LOGI(TAG, "按键检测任务启动");
    
    uint32_t stable_count = 0;
    const uint32_t debounce_count = KEY_DEBOUNCE_MS / 10; // 10ms检测间隔
    
    while (detection_running) {
        // 读取GPIO电平
        int gpio_level = gpio_get_level(KEY_GPIO);
        key_state_t new_state = (gpio_level == KEY_PRESSED_LEVEL) ? KEY_STATE_PRESSED : KEY_STATE_RELEASED;
        
        if (xSemaphoreTake(key_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (new_state == current_key_state) {
                stable_count++;
            } else {
                stable_count = 0;
                current_key_state = new_state;
            }
            
            // 防抖处理：状态稳定足够长时间才认为是有效变化
            if (stable_count >= debounce_count && current_key_state != last_key_state) {
                uint32_t timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
                
                // 触发回调
                if (event_callback != NULL) {
                    key_event_t event = (current_key_state == KEY_STATE_PRESSED) ? 
                                       KEY_EVENT_PRESSED : KEY_EVENT_RELEASED;
                    event_callback(event, timestamp_ms);
                }
                
                last_key_state = current_key_state;
                ESP_LOGI(TAG, "按键状态变化: %s (时间戳: %lu)", 
                         current_key_state == KEY_STATE_PRESSED ? "按下" : "松开", 
                         timestamp_ms);
            }
            
            xSemaphoreGive(key_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms检测间隔
    }
    
    ESP_LOGI(TAG, "按键检测任务结束");
    key_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t key_init(void)
{
    // 创建互斥锁
    key_mutex = xSemaphoreCreateMutex();
    if (key_mutex == NULL) {
        ESP_LOGE(TAG, "创建按键互斥锁失败");
        return ESP_FAIL;
    }
    
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << KEY_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // 启用上拉，确保松开时为高电平
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "配置按键GPIO%d失败: %s", KEY_GPIO, esp_err_to_name(ret));
        vSemaphoreDelete(key_mutex);
        key_mutex = NULL;
        return ret;
    }
    
    // 初始化状态
    int initial_level = gpio_get_level(KEY_GPIO);
    current_key_state = (initial_level == KEY_PRESSED_LEVEL) ? KEY_STATE_PRESSED : KEY_STATE_RELEASED;
    last_key_state = current_key_state;
    
    ESP_LOGI(TAG, "按键驱动初始化成功 (GPIO%d, 初始状态: %s)", 
             KEY_GPIO, current_key_state == KEY_STATE_PRESSED ? "按下" : "松开");
    return ESP_OK;
}

esp_err_t key_deinit(void)
{
    // 停止检测
    key_stop_detection();
    
    // 删除互斥锁
    if (key_mutex != NULL) {
        vSemaphoreDelete(key_mutex);
        key_mutex = NULL;
    }
    
    // 重置GPIO
    gpio_reset_pin(KEY_GPIO);
    
    ESP_LOGI(TAG, "按键驱动反初始化完成");
    return ESP_OK;
}

esp_err_t key_get_state(key_state_t *state)
{
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(key_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        *state = current_key_state;
        xSemaphoreGive(key_mutex);
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t key_set_event_callback(key_event_callback_t callback)
{
    if (xSemaphoreTake(key_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        event_callback = callback;
        xSemaphoreGive(key_mutex);
        ESP_LOGI(TAG, "按键事件回调已%s", callback ? "设置" : "清除");
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t key_start_detection(void)
{
    if (detection_running) {
        ESP_LOGW(TAG, "按键检测已在运行");
        return ESP_OK;
    }
    
    if (key_mutex == NULL) {
        ESP_LOGE(TAG, "按键驱动未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    detection_running = true;
    
    BaseType_t ret = xTaskCreate(key_detection_task, "key_detect", 4096, NULL, 6, &key_task_handle);
    if (ret != pdPASS) {
        detection_running = false;
        ESP_LOGE(TAG, "创建按键检测任务失败");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "按键检测启动");
    return ESP_OK;
}

esp_err_t key_stop_detection(void)
{
    if (!detection_running) {
        ESP_LOGW(TAG, "按键检测未在运行");
        return ESP_OK;
    }
    
    detection_running = false;
    
    // 等待任务结束
    if (key_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(50)); // 等待任务自然结束
    }
    
    ESP_LOGI(TAG, "按键检测停止");
    return ESP_OK;
}
