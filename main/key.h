/**
 * @file key.h
 * @brief 按键驱动模块头文件
 * 
 * 实现GPIO35按键检测功能：
 * - 按下：低电平
 * - 松开：高电平
 * - 状态变化检测和回调
 */

#ifndef KEY_H
#define KEY_H

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按键配置 */
#define KEY_GPIO            GPIO_NUM_35     /*!< 按键GPIO号 */
#define KEY_PRESSED_LEVEL   0               /*!< 按键按下时的电平 */
#define KEY_RELEASED_LEVEL  1               /*!< 按键松开时的电平 */
#define KEY_DEBOUNCE_MS     50              /*!< 按键防抖时间(毫秒) */

/* 按键状态枚举 */
typedef enum {
    KEY_STATE_RELEASED = 0,                 /*!< 按键松开状态 */
    KEY_STATE_PRESSED = 1                   /*!< 按键按下状态 */
} key_state_t;

/* 按键事件枚举 */
typedef enum {
    KEY_EVENT_PRESSED,                      /*!< 按键按下事件 */
    KEY_EVENT_RELEASED                      /*!< 按键松开事件 */
} key_event_t;

/**
 * @brief 按键事件回调函数类型
 * 
 * @param event 按键事件类型
 * @param timestamp_ms 事件发生的时间戳(毫秒)
 */
typedef void (*key_event_callback_t)(key_event_t event, uint32_t timestamp_ms);

/**
 * @brief 初始化按键驱动
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_FAIL: 初始化失败
 */
esp_err_t key_init(void);

/**
 * @brief 反初始化按键驱动
 * 
 * @return esp_err_t
 *         - ESP_OK: 反初始化成功
 *         - ESP_FAIL: 反初始化失败
 */
esp_err_t key_deinit(void);

/**
 * @brief 获取当前按键状态
 * 
 * @param state 输出按键状态
 * @return esp_err_t
 *         - ESP_OK: 获取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t key_get_state(key_state_t *state);

/**
 * @brief 设置按键事件回调函数
 * 
 * @param callback 回调函数指针，设为NULL则取消回调
 * @return esp_err_t
 *         - ESP_OK: 设置成功
 */
esp_err_t key_set_event_callback(key_event_callback_t callback);

/**
 * @brief 启动按键检测任务
 * 
 * @return esp_err_t
 *         - ESP_OK: 启动成功
 *         - ESP_FAIL: 启动失败
 */
esp_err_t key_start_detection(void);

/**
 * @brief 停止按键检测任务
 * 
 * @return esp_err_t
 *         - ESP_OK: 停止成功
 */
esp_err_t key_stop_detection(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_H */
