/**
 * @file tca9535.h
 * @brief TCA9535 I/O扩展器驱动程序头文件
 * 
 * TCA9535是一个16位I/O扩展器，具有中断输出，通过I2C总线进行通信。
 * 它有两个8位的I/O端口（P0和P1），每个端口都可以独立配置为输入或输出。
 * 
 * 特性：
 * - 16个I/O引脚，分为两个8位端口（P0和P1）
 * - 每个I/O可独立配置为输入或输出
 * - 输入极性反转功能
 * - 中断输出功能
 * - I2C地址可配置（通过A0, A1, A2引脚）
 * 
 * @author ESP32开发团队
 * @date 2024
 */

#ifndef TCA9535_H
#define TCA9535_H

#include "esp_err.h"
#include "i2cdev.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCA9535寄存器地址枚举
 */
typedef enum {
    TCA9535_INPUT_REG0    = 0x00,  /*!< 输入端口0寄存器 */
    TCA9535_INPUT_REG1    = 0x01,  /*!< 输入端口1寄存器 */
    TCA9535_OUTPUT_REG0   = 0x02,  /*!< 输出端口0寄存器 */
    TCA9535_OUTPUT_REG1   = 0x03,  /*!< 输出端口1寄存器 */
    TCA9535_POLARITY_REG0 = 0x04,  /*!< 极性反转0寄存器 */
    TCA9535_POLARITY_REG1 = 0x05,  /*!< 极性反转1寄存器 */
    TCA9535_CONFIG_REG0   = 0x06,  /*!< 配置0寄存器 (1=输入, 0=输出) */
    TCA9535_CONFIG_REG1   = 0x07   /*!< 配置1寄存器 (1=输入, 0=输出) */
} tca9535_reg_t;

/**
 * @brief TCA9535位域结构体
 */
typedef struct {
    uint8_t bit0: 1;
    uint8_t bit1: 1;
    uint8_t bit2: 1;
    uint8_t bit3: 1;
    uint8_t bit4: 1;
    uint8_t bit5: 1;
    uint8_t bit6: 1;
    uint8_t bit7: 1;
} tca9535_port_bits_t;

/**
 * @brief TCA9535端口联合体
 * 可以按字节或按位访问
 */
typedef union {
    uint8_t byte;                    /*!< 按字节访问端口数据 */
    tca9535_port_bits_t bits;        /*!< 按位访问端口数据 */
} tca9535_port_t;

/**
 * @brief TCA9535寄存器结构体
 */
typedef struct {
    tca9535_port_t port0;            /*!< 端口0数据 */
    tca9535_port_t port1;            /*!< 端口1数据 */
} tca9535_ports_t;

/**
 * @brief TCA9535 16位寄存器联合体
 */
typedef union {
    uint16_t word;                   /*!< 按16位字访问寄存器数据 */
    tca9535_ports_t ports;           /*!< 按端口访问寄存器数据 */
} tca9535_register_t;

/**
 * @brief TCA9535配置结构体
 */
typedef struct {
    i2c_dev_t i2c_dev;              /*!< I2C设备描述符 */
} tca9535_config_t;

/**
 * @brief TCA9535设备句柄类型
 */
typedef struct tca9535_dev_s* tca9535_handle_t;

/**
 * @brief 创建TCA9535设备句柄
 * 
 * @param config 设备配置
 * @param handle 输出的设备句柄
 * @return esp_err_t
 *         - ESP_OK: 创建成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_ERR_NO_MEM: 内存不足
 */
esp_err_t tca9535_create(const tca9535_config_t *config, tca9535_handle_t *handle);

/**
 * @brief 删除TCA9535设备句柄
 * 
 * @param handle 设备句柄
 * @return esp_err_t
 *         - ESP_OK: 删除成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t tca9535_delete(tca9535_handle_t handle);

/**
 * @brief 读取单个寄存器
 * 
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param data 输出的数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_register(tca9535_handle_t handle, tca9535_reg_t reg, uint8_t *data);

/**
 * @brief 写入单个寄存器
 * 
 * @param handle 设备句柄
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_write_register(tca9535_handle_t handle, tca9535_reg_t reg, uint8_t data);

/**
 * @brief 读取16位寄存器对
 * 
 * @param handle 设备句柄
 * @param reg 寄存器起始地址
 * @param data 输出的寄存器数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_register_pair(tca9535_handle_t handle, tca9535_reg_t reg, tca9535_register_t *data);

/**
 * @brief 写入16位寄存器对
 * 
 * @param handle 设备句柄
 * @param reg 寄存器起始地址
 * @param data 要写入的寄存器数据
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_write_register_pair(tca9535_handle_t handle, tca9535_reg_t reg, const tca9535_register_t *data);

/**
 * @brief 读取输入端口
 * 
 * @param handle 设备句柄
 * @param data 输出的输入端口数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_input(tca9535_handle_t handle, tca9535_register_t *data);

/**
 * @brief 读取输出端口
 * 
 * @param handle 设备句柄
 * @param data 输出的输出端口数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_output(tca9535_handle_t handle, tca9535_register_t *data);

/**
 * @brief 写入输出端口
 * 
 * @param handle 设备句柄
 * @param data 要写入的输出端口数据
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_write_output(tca9535_handle_t handle, const tca9535_register_t *data);

/**
 * @brief 读取极性反转寄存器
 * 
 * @param handle 设备句柄
 * @param data 输出的极性反转数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_polarity(tca9535_handle_t handle, tca9535_register_t *data);

/**
 * @brief 写入极性反转寄存器
 * 
 * @param handle 设备句柄
 * @param data 要写入的极性反转数据
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_write_polarity(tca9535_handle_t handle, const tca9535_register_t *data);

/**
 * @brief 读取配置寄存器
 * 
 * @param handle 设备句柄
 * @param data 输出的配置数据
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_read_config(tca9535_handle_t handle, tca9535_register_t *data);

/**
 * @brief 写入配置寄存器
 * 
 * @param handle 设备句柄
 * @param data 要写入的配置数据 (1=输入, 0=输出)
 * @return esp_err_t
 *         - ESP_OK: 写入成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_write_config(tca9535_handle_t handle, const tca9535_register_t *data);

/**
 * @brief 设置单个引脚为输出并设置状态
 * 
 * @param handle 设备句柄
 * @param pin 引脚号 (0-15)
 * @param level 电平状态 (0=低电平, 1=高电平)
 * @return esp_err_t
 *         - ESP_OK: 设置成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_set_pin_output(tca9535_handle_t handle, uint8_t pin, uint8_t level);

/**
 * @brief 设置单个引脚为输入
 * 
 * @param handle 设备句柄
 * @param pin 引脚号 (0-15)
 * @return esp_err_t
 *         - ESP_OK: 设置成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_set_pin_input(tca9535_handle_t handle, uint8_t pin);

/**
 * @brief 读取单个引脚状态
 * 
 * @param handle 设备句柄
 * @param pin 引脚号 (0-15)
 * @param level 输出的电平状态
 * @return esp_err_t
 *         - ESP_OK: 读取成功
 *         - ESP_ERR_INVALID_ARG: 参数无效
 *         - ESP_FAIL: I2C通信失败
 */
esp_err_t tca9535_get_pin_level(tca9535_handle_t handle, uint8_t pin, uint8_t *level);

#ifdef __cplusplus
}
#endif

#endif /* TCA9535_H */
