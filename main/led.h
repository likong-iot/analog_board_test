/**
 * @file led.h
 * @brief LED控制模块头文件
 * 
 * 控制4个LED灯，分别连接到GPIO21、19、18、5
 * LED为上拉点亮方式（高电平点亮）
 */

#ifndef LED_H
#define LED_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* LED GPIO引脚定义 */
#define LED1_GPIO           21              /*!< LED1 GPIO引脚 */
#define LED2_GPIO           19              /*!< LED2 GPIO引脚 */
#define LED3_GPIO           18              /*!< LED3 GPIO引脚 */
#define LED4_GPIO           5               /*!< LED4 GPIO引脚 */

/* LED编号枚举 */
typedef enum {
    LED_1 = 1,                              /*!< LED1 */
    LED_2 = 2,                              /*!< LED2 */
    LED_3 = 3,                              /*!< LED3 */
    LED_4 = 4,                              /*!< LED4 */
    LED_ALL = 0                             /*!< 所有LED */
} led_num_t;

/* LED状态枚举 */
typedef enum {
    LED_OFF = 0,                            /*!< LED关闭 */
    LED_ON = 1                              /*!< LED开启 */
} led_state_t;

/**
 * @brief 初始化LED GPIO
 * 
 * 配置所有LED GPIO为输出模式，初始状态为关闭
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_FAIL: 初始化失败
 */
esp_err_t led_init(void);

/**
 * @brief 控制单个LED
 * 
 * @param led_num LED编号 (LED_1, LED_2, LED_3, LED_4)
 * @param state LED状态 (LED_ON, LED_OFF)
 * @return esp_err_t
 *         - ESP_OK: 控制成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t led_set_state(led_num_t led_num, led_state_t state);

/**
 * @brief 获取单个LED状态
 * 
 * @param led_num LED编号 (LED_1, LED_2, LED_3, LED_4)
 * @param state 输出的LED状态
 * @return esp_err_t
 *         - ESP_OK: 获取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t led_get_state(led_num_t led_num, led_state_t *state);

/**
 * @brief 控制所有LED
 * 
 * @param state LED状态 (LED_ON, LED_OFF)
 * @return esp_err_t
 *         - ESP_OK: 控制成功
 */
esp_err_t led_set_all_state(led_state_t state);

/**
 * @brief 切换LED状态
 * 
 * @param led_num LED编号 (LED_1, LED_2, LED_3, LED_4, LED_ALL)
 * @return esp_err_t
 *         - ESP_OK: 切换成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t led_toggle(led_num_t led_num);

/**
 * @brief LED闪烁
 * 
 * @param led_num LED编号 (LED_1, LED_2, LED_3, LED_4, LED_ALL)
 * @param times 闪烁次数
 * @param interval_ms 闪烁间隔(毫秒)
 * @return esp_err_t
 *         - ESP_OK: 闪烁成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t led_blink(led_num_t led_num, uint8_t times, uint32_t interval_ms);

/**
 * @brief 获取LED对应的GPIO引脚号
 * 
 * @param led_num LED编号
 * @return GPIO引脚号，如果LED编号无效则返回-1
 */
int led_get_gpio_num(led_num_t led_num);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
