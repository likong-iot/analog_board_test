#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// UART配置常量
#define UART1_TXD_PIN   22
#define UART1_RXD_PIN   23
#define UART2_TXD_PIN   27
#define UART2_RXD_PIN   26
#define UART_BAUD_RATE  56000
#define UART_BUF_SIZE   1024

/**
 * @brief 初始化UART1和UART2
 * @return true 成功, false 失败
 */
bool uart_driver_init(void);

/**
 * @brief UART发送数据
 * @param uart_num UART端口号
 * @param data 要发送的数据
 * @param length 数据长度
 * @return 实际发送的字节数
 */
int uart_send_data(uart_port_t uart_num, const uint8_t *data, size_t length);

/**
 * @brief UART接收数据
 * @param uart_num UART端口号
 * @param data 接收缓冲区
 * @param max_length 最大接收长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际接收的字节数
 */
int uart_receive_data(uart_port_t uart_num, uint8_t *data, size_t max_length, uint32_t timeout_ms);

/**
 * @brief UART1接收任务
 * @param arg 任务参数
 */
void uart1_rx_task(void *arg);

/**
 * @brief UART2接收任务
 * @param arg 任务参数
 */
void uart2_rx_task(void *arg);

/**
 * @brief UART1输出函数 (用于Shell)
 * @param channel_id 通道ID
 * @param data 数据
 * @param length 长度
 */
void uart1_output_func(uint32_t channel_id, const uint8_t *data, size_t length);

/**
 * @brief UART2输出函数 (用于Shell)
 * @param channel_id 通道ID
 * @param data 数据
 * @param length 长度
 */
void uart2_output_func(uint32_t channel_id, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H
