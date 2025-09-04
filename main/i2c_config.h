/**
 * @file i2c_config.h
 * @brief I2C总线配置文件
 * 
 * 这个文件包含了项目中所有I2C总线的配置信息，包括引脚定义、时钟频率等。
 * 所有使用I2C的组件都应该包含这个文件来获取I2C配置。
 */

#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

// #include "driver/i2c.h"  // 移除旧I2C驱动，使用i2cdev库
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C总线硬件配置 */
#define I2C_MASTER_SCL_IO           32              /*!< I2C主机时钟线GPIO引脚 */
#define I2C_MASTER_SDA_IO           33              /*!< I2C主机数据线GPIO引脚 */
#define I2C_MASTER_NUM              0               /*!< I2C端口号 */
#define I2C_MASTER_FREQ_HZ          100000          /*!< I2C主机时钟频率 */

/* I2C通用配置 */
#define I2C_MASTER_TX_BUF_DISABLE   0               /*!< I2C主机不需要缓冲区 */
#define I2C_MASTER_RX_BUF_DISABLE   0               /*!< I2C主机不需要缓冲区 */
#define I2C_MASTER_TIMEOUT_MS       1000            /*!< I2C操作超时时间(毫秒) */

/* I2C操作标志 (兼容定义) */
#define I2C_WRITE_BIT               0                /*!< I2C主机写操作 */
#define I2C_READ_BIT                1                /*!< I2C主机读操作 */
#define I2C_ACK_CHECK_EN            0x1              /*!< I2C主机检查从机ACK */
#define I2C_ACK_CHECK_DIS           0x0              /*!< I2C主机不检查从机ACK */
#define I2C_ACK_VAL                 0x0              /*!< I2C ACK值 */
#define I2C_NACK_VAL                0x1              /*!< I2C NACK值 */

/* 已知的I2C设备地址和配置 */
#define TCA9535_I2C_ADDR            0x26            /*!< TCA9535 I/O扩展器地址 */
#define TCA9535_INT_GPIO            25              /*!< TCA9535中断引脚 */
#define ADS1115_I2C_ADDR            0x48            /*!< ADS1115 ADC地址 */

/* ADS1115电流测量配置 */
#define ADS1115_SHUNT_RESISTOR_OHMS 30.0f           /*!< 分流电阻值(欧姆) - 支持0-110mA电流测量 */
#define ADS1115_CHANNEL_COUNT       4               /*!< ADS1115通道数量 */
#define ADS1115_MAX_VOLTAGE_V       4.096f          /*!< ADS1115最大测量电压(伏特) - ±4.096V增益 */
#define ADS1115_MAX_CURRENT_MA      136.5f          /*!< 理论最大电流(毫安) - 4.096V/30Ω */

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

/**
 * @brief 读取指定通道的电流值
 * 
 * @param channel 通道号 (0-3)
 * @param current_ma 输出的电流值(毫安)
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: 读取失败
 */
esp_err_t ads1115_read_current(uint8_t channel, float *current_ma);

/**
 * @brief 读取指定通道的原始电压值
 * 
 * @param channel 通道号 (0-3)
 * @param voltage_v 输出的电压值(伏特)
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: 读取失败
 */
esp_err_t ads1115_read_voltage(uint8_t channel, float *voltage_v);

/**
 * @brief 读取所有4个通道的电流值
 * 
 * @param currents_ma 输出的4个通道电流值数组(毫安)
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: 读取失败
 */
esp_err_t ads1115_read_all_currents(float currents_ma[ADS1115_CHANNEL_COUNT]);

/**
 * @brief 读取所有通道的详细信息
 * 
 * @param channel_data 输出的通道数据数组，每个元素包含原始值、电压、电流
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_STATE: ADS1115未初始化
 *         - ESP_FAIL: 读取失败
 */
typedef struct {
    int16_t raw_value;                      /*!< 原始ADC值 */
    float voltage_v;                        /*!< 电压值(伏特) */
    float current_ma;                       /*!< 电流值(毫安) */
    esp_err_t status;                       /*!< 读取状态 */
} ads1115_channel_data_t;

esp_err_t ads1115_read_all_detailed(ads1115_channel_data_t channel_data[ADS1115_CHANNEL_COUNT]);

/**
 * @brief 获取ADS1115配置信息
 * 
 * @param config_info 输出的配置信息结构体
 * @return esp_err_t
 *         - ESP_OK: 获取成功
 *         - ESP_ERR_INVALID_STATE: ADS1115未初始化
 *         - ESP_FAIL: 获取失败
 */
typedef struct {
    uint8_t gain;                           /*!< 增益设置 */
    uint8_t data_rate;                      /*!< 数据速率 */
    uint8_t mode;                           /*!< 工作模式 */
    const char *gain_str;                   /*!< 增益字符串描述 */
    uint16_t rate_sps;                      /*!< 采样率(SPS) */
    const char *mode_str;                   /*!< 模式字符串描述 */
} ads1115_config_info_t;

esp_err_t ads1115_get_config_info(ads1115_config_info_t *config_info);

#ifdef __cplusplus
}
#endif

#endif /* I2C_CONFIG_H */
