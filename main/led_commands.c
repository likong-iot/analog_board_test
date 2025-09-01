/**
 * @file led_commands.c
 * @brief LED命令处理函数实现
 */

#include "led_commands.h"
#include "led.h"
#include "shell.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "LED_CMD";

void task_led_control(uint32_t channel_id, const char *params)
{
    char response[1024];  // 增加缓冲区大小
    char cmd[32], state_str[32], times_str[32], interval_str[32];
    
    if (strlen(params) == 0) {
        // 分段发送帮助信息以避免缓冲区溢出
        snprintf(response, sizeof(response),
                "LED控制命令用法:\r\n"
                "led <1-4> on/off          - 控制单个LED开关\r\n"
                "led all on/off            - 控制所有LED开关\r\n"
                "led <1-4> toggle          - 切换单个LED状态\r\n"
                "led all toggle            - 切换所有LED状态\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        snprintf(response, sizeof(response),
                "led <1-4> blink <次数> [间隔ms] - LED闪烁\r\n"
                "led all blink <次数> [间隔ms]   - 所有LED闪烁\r\n"
                "led status                - 显示所有LED状态\r\n"
                "\r\n"
                "示例:\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        snprintf(response, sizeof(response),
                "led 1 on                  - 点亮LED1\r\n"
                "led all off               - 关闭所有LED\r\n"
                "led 2 toggle              - 切换LED2状态\r\n"
                "led 3 blink 5 200         - LED3闪烁5次，间隔200ms\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 解析命令参数
    int parsed = sscanf(params, "%31s %31s %31s %31s", cmd, state_str, times_str, interval_str);
    
    if (strcmp(cmd, "status") == 0) {
        // 显示所有LED状态
        snprintf(response, sizeof(response), "=== LED状态 ===\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
        for (led_num_t led = LED_1; led <= LED_4; led++) {
            led_state_t state;
            if (led_get_state(led, &state) == ESP_OK) {
                int gpio_num = led_get_gpio_num(led);
                snprintf(response, sizeof(response), "LED%d (GPIO%d): %s\r\n", 
                         led, gpio_num, state == LED_ON ? "点亮" : "熄灭");
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
            }
        }
        snprintf(response, sizeof(response), "==================\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    if (parsed < 2) {
        snprintf(response, sizeof(response), "错误: 参数不足\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    led_num_t led_num = LED_1;
    bool is_all = false;
    
    // 解析LED编号
    if (strcmp(cmd, "all") == 0) {
        is_all = true;
    } else {
        int led_number = atoi(cmd);
        if (led_number >= 1 && led_number <= 4) {
            led_num = (led_num_t)led_number;
        } else {
            ESP_LOGE(TAG, "无效的LED编号: %s", cmd);
            snprintf(response, sizeof(response), "错误: 无效的LED编号 '%s'，应为1-4或all\r\n", cmd);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 处理不同的命令
    if (strcmp(state_str, "on") == 0) {
        esp_err_t ret;
        if (is_all) {
            ret = led_set_all_state(LED_ON);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "所有LED已点亮\r\n");
            } else {
                snprintf(response, sizeof(response), "错误: 无法点亮所有LED\r\n");
            }
        } else {
            ret = led_set_state(led_num, LED_ON);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "LED%d已点亮\r\n", led_num);
            } else {
                snprintf(response, sizeof(response), "错误: 无法点亮LED%d\r\n", led_num);
            }
        }
    } else if (strcmp(state_str, "off") == 0) {
        esp_err_t ret;
        if (is_all) {
            ret = led_set_all_state(LED_OFF);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "所有LED已熄灭\r\n");
            } else {
                snprintf(response, sizeof(response), "错误: 无法熄灭所有LED\r\n");
            }
        } else {
            ret = led_set_state(led_num, LED_OFF);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "LED%d已熄灭\r\n", led_num);
            } else {
                snprintf(response, sizeof(response), "错误: 无法熄灭LED%d\r\n", led_num);
            }
        }
    } else if (strcmp(state_str, "toggle") == 0) {
        esp_err_t ret;
        if (is_all) {
            ret = led_toggle(LED_ALL);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "所有LED状态已切换\r\n");
            } else {
                snprintf(response, sizeof(response), "错误: 无法切换所有LED状态\r\n");
            }
        } else {
            ret = led_toggle(led_num);
            if (ret == ESP_OK) {
                led_state_t current_state;
                led_get_state(led_num, &current_state);
                snprintf(response, sizeof(response), "LED%d已切换为%s\r\n", 
                         led_num, current_state == LED_ON ? "点亮" : "熄灭");
            } else {
                snprintf(response, sizeof(response), "错误: 无法切换LED%d状态\r\n", led_num);
            }
        }
    } else if (strcmp(state_str, "blink") == 0) {
        if (parsed < 3) {
            snprintf(response, sizeof(response), "错误: blink命令需要指定闪烁次数\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        
        uint8_t times = (uint8_t)atoi(times_str);
        uint32_t interval_ms = (parsed >= 4) ? (uint32_t)atoi(interval_str) : 500; // 默认500ms间隔
        
        if (times == 0) {
            snprintf(response, sizeof(response), "错误: 闪烁次数必须大于0\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        
        esp_err_t ret;
        if (is_all) {
            snprintf(response, sizeof(response), "所有LED闪烁%d次，间隔%lums...\r\n", times, interval_ms);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            ret = led_blink(LED_ALL, times, interval_ms);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "所有LED闪烁完成\r\n");
            } else {
                snprintf(response, sizeof(response), "错误: LED闪烁失败\r\n");
            }
        } else {
            snprintf(response, sizeof(response), "LED%d闪烁%d次，间隔%lums...\r\n", led_num, times, interval_ms);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            ret = led_blink(led_num, times, interval_ms);
            if (ret == ESP_OK) {
                snprintf(response, sizeof(response), "LED%d闪烁完成\r\n", led_num);
            } else {
                snprintf(response, sizeof(response), "错误: LED%d闪烁失败\r\n", led_num);
            }
        }
    } else {
        snprintf(response, sizeof(response), "错误: 未知命令 '%s'，支持: on, off, toggle, blink\r\n", state_str);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}
