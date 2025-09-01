/**
 * @file led.c
 * @brief LED控制模块实现
 */

#include "led.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LED";

// LED状态记录
static led_state_t led_states[4] = {LED_OFF, LED_OFF, LED_OFF, LED_OFF};

esp_err_t led_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // 配置LED1 GPIO
    gpio_config_t led1_config = {
        .pin_bit_mask = (1ULL << LED1_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret |= gpio_config(&led1_config);
    
    // 配置LED2 GPIO
    gpio_config_t led2_config = {
        .pin_bit_mask = (1ULL << LED2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret |= gpio_config(&led2_config);
    
    // 配置LED3 GPIO
    gpio_config_t led3_config = {
        .pin_bit_mask = (1ULL << LED3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret |= gpio_config(&led3_config);
    
    // 配置LED4 GPIO
    gpio_config_t led4_config = {
        .pin_bit_mask = (1ULL << LED4_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ret |= gpio_config(&led4_config);
    
    if (ret == ESP_OK) {
        // 初始化所有LED为关闭状态
        led_set_all_state(LED_OFF);
        ESP_LOGI(TAG, "LED模块初始化成功");
        ESP_LOGI(TAG, "LED1: GPIO%d, LED2: GPIO%d, LED3: GPIO%d, LED4: GPIO%d", 
                 LED1_GPIO, LED2_GPIO, LED3_GPIO, LED4_GPIO);
    } else {
        ESP_LOGE(TAG, "LED模块初始化失败");
    }
    
    return ret;
}

int led_get_gpio_num(led_num_t led_num)
{
    switch (led_num) {
        case LED_1: return LED1_GPIO;
        case LED_2: return LED2_GPIO;
        case LED_3: return LED3_GPIO;
        case LED_4: return LED4_GPIO;
        default: return -1;
    }
}

esp_err_t led_set_state(led_num_t led_num, led_state_t state)
{
    int gpio_num = led_get_gpio_num(led_num);
    if (gpio_num < 0) {
        ESP_LOGE(TAG, "无效的LED编号: %d", led_num);
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = gpio_set_level(gpio_num, state);
    if (ret == ESP_OK) {
        // 更新状态记录
        led_states[led_num - 1] = state;
        ESP_LOGD(TAG, "LED%d (GPIO%d) %s", led_num, gpio_num, 
                 state == LED_ON ? "点亮" : "熄灭");
    } else {
        ESP_LOGE(TAG, "设置LED%d状态失败", led_num);
    }
    
    return ret;
}

esp_err_t led_get_state(led_num_t led_num, led_state_t *state)
{
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (led_num < LED_1 || led_num > LED_4) {
        ESP_LOGE(TAG, "无效的LED编号: %d", led_num);
        return ESP_ERR_INVALID_ARG;
    }
    
    *state = led_states[led_num - 1];
    return ESP_OK;
}

esp_err_t led_set_all_state(led_state_t state)
{
    esp_err_t ret = ESP_OK;
    
    for (led_num_t led = LED_1; led <= LED_4; led++) {
        esp_err_t single_ret = led_set_state(led, state);
        if (single_ret != ESP_OK) {
            ret = single_ret;
        }
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "所有LED %s", state == LED_ON ? "点亮" : "熄灭");
    }
    
    return ret;
}

esp_err_t led_toggle(led_num_t led_num)
{
    if (led_num == LED_ALL) {
        // 切换所有LED
        esp_err_t ret = ESP_OK;
        for (led_num_t led = LED_1; led <= LED_4; led++) {
            led_state_t current_state = led_states[led - 1];
            led_state_t new_state = (current_state == LED_ON) ? LED_OFF : LED_ON;
            esp_err_t single_ret = led_set_state(led, new_state);
            if (single_ret != ESP_OK) {
                ret = single_ret;
            }
        }
        return ret;
    } else {
        // 切换单个LED
        if (led_num < LED_1 || led_num > LED_4) {
            ESP_LOGE(TAG, "无效的LED编号: %d", led_num);
            return ESP_ERR_INVALID_ARG;
        }
        
        led_state_t current_state = led_states[led_num - 1];
        led_state_t new_state = (current_state == LED_ON) ? LED_OFF : LED_ON;
        return led_set_state(led_num, new_state);
    }
}

esp_err_t led_blink(led_num_t led_num, uint8_t times, uint32_t interval_ms)
{
    if (times == 0 || interval_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = ESP_OK;
    
    if (led_num == LED_ALL) {
        // 所有LED闪烁
        for (uint8_t i = 0; i < times; i++) {
            ret |= led_set_all_state(LED_ON);
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
            ret |= led_set_all_state(LED_OFF);
            if (i < times - 1) {  // 最后一次不延时
                vTaskDelay(pdMS_TO_TICKS(interval_ms));
            }
        }
    } else {
        // 单个LED闪烁
        if (led_num < LED_1 || led_num > LED_4) {
            ESP_LOGE(TAG, "无效的LED编号: %d", led_num);
            return ESP_ERR_INVALID_ARG;
        }
        
        for (uint8_t i = 0; i < times; i++) {
            ret |= led_set_state(led_num, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
            ret |= led_set_state(led_num, LED_OFF);
            if (i < times - 1) {  // 最后一次不延时
                vTaskDelay(pdMS_TO_TICKS(interval_ms));
            }
        }
    }
    
    return ret;
}
