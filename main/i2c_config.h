/**
 * @file i2c_config.h
 * @brief I2C总线配置文件
 * 
 * 这个文件包含了项目中所有I2C总线的配置信息，包括引脚定义、时钟频率等。
 * 所有使用I2C的组件都应该包含这个文件来获取I2C配置。
 */

#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C总线硬件配置 */
#define I2C_MASTER_SCL_IO           32              /*!< I2C主机时钟线GPIO引脚 */
#define I2C_MASTER_SDA_IO           33              /*!< I2C主机数据线GPIO引脚 */
#define I2C_MASTER_NUM              I2C_NUM_0       /*!< I2C端口号 */
#define I2C_MASTER_FREQ_HZ          100000          /*!< I2C主机时钟频率 */

/* I2C通用配置 */
#define I2C_MASTER_TX_BUF_DISABLE   0               /*!< I2C主机不需要缓冲区 */
#define I2C_MASTER_RX_BUF_DISABLE   0               /*!< I2C主机不需要缓冲区 */
#define I2C_MASTER_TIMEOUT_MS       1000            /*!< I2C操作超时时间(毫秒) */

/* I2C操作标志 */
#define I2C_WRITE_BIT               I2C_MASTER_WRITE /*!< I2C主机写操作 */
#define I2C_READ_BIT                I2C_MASTER_READ  /*!< I2C主机读操作 */
#define I2C_ACK_CHECK_EN            0x1              /*!< I2C主机检查从机ACK */
#define I2C_ACK_CHECK_DIS           0x0              /*!< I2C主机不检查从机ACK */
#define I2C_ACK_VAL                 0x0              /*!< I2C ACK值 */
#define I2C_NACK_VAL                0x1              /*!< I2C NACK值 */

/* 已知的I2C设备地址和配置 */
#define TCA9535_I2C_ADDR            0x26            /*!< TCA9535 I/O扩展器地址 */
#define TCA9535_INT_GPIO            25              /*!< TCA9535中断引脚 */
#define ADS1115_I2C_ADDR            0x48            /*!< ADS1115 ADC地址 */

/**
 * @brief 初始化I2C主机
 * 
 * 这个函数应该在main函数中调用，只需要调用一次。
 * 所有使用I2C的组件都共享这个总线。
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_ERR_INVALID_ARG: 参数错误
 *         - ESP_FAIL: 初始化失败
 */
esp_err_t i2c_master_init(void);

/**
 * @brief 反初始化I2C主机
 * 
 * @return esp_err_t
 *         - ESP_OK: 反初始化成功
 *         - ESP_FAIL: 反初始化失败
 */
esp_err_t i2c_master_deinit(void);

/**
 * @brief 初始化ADS1115 ADC设备
 * 
 * @return esp_err_t
 *         - ESP_OK: 初始化成功
 *         - ESP_ERR_INVALID_ARG: 参数错误
 *         - ESP_FAIL: 初始化失败
 */
esp_err_t ads1115_init(void);

/**
 * @brief 获取ADS1115设备句柄
 * 
 * @return 指向ADS1115设备描述符的指针，如果未初始化则返回NULL
 */
void* ads1115_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_CONFIG_H */
