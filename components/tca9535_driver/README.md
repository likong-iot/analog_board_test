# TCA9535 I/O扩展器驱动组件

这是一个用于ESP-IDF 5.5的TCA9535 I/O扩展器驱动组件。TCA9535是一个16位I/O扩展器，具有中断输出，通过I2C总线进行通信。

## 特性

- ✅ 16个I/O引脚，分为两个8位端口（P0和P1）
- ✅ 每个I/O可独立配置为输入或输出
- ✅ 输入极性反转功能
- ✅ 面向对象的API设计
- ✅ 完整的错误处理
- ✅ 支持多个设备实例
- ✅ 符合ESP-IDF 5.5标准

## 硬件连接

```
ESP32     TCA9535
-----     -------
GPIO32 -> SCL
GPIO33 -> SDA
GND    -> GND
3.3V   -> VCC
```

## API参考

### 设备管理

```c
// 创建设备句柄
tca9535_config_t config = {
    .i2c_port = I2C_MASTER_NUM,
    .device_addr = TCA9535_I2C_ADDR,
    .timeout_ms = 1000
};
tca9535_handle_t handle;
tca9535_create(&config, &handle);

// 删除设备句柄
tca9535_delete(handle);
```

### 引脚操作

```c
// 设置引脚为输出并设置电平
tca9535_set_pin_output(handle, 0, 1);  // 引脚0输出高电平

// 设置引脚为输入
tca9535_set_pin_input(handle, 1);

// 读取引脚电平
uint8_t level;
tca9535_get_pin_level(handle, 1, &level);
```

### 寄存器操作

```c
// 批量配置
tca9535_register_t config_reg = {0};
config_reg.ports.port0.byte = 0xF0;  // 高4位输入，低4位输出
tca9535_write_config(handle, &config_reg);

// 批量输出
tca9535_register_t output_reg = {0};
output_reg.ports.port0.byte = 0x0F;  // 低4位输出高电平
tca9535_write_output(handle, &output_reg);

// 读取输入
tca9535_register_t input_reg;
tca9535_read_input(handle, &input_reg);
```

## 使用步骤

1. **初始化I2C总线**（在main函数中）：
   ```c
   #include "i2c_config.h"
   
   esp_err_t ret = i2c_master_init();
   ```

2. **创建TCA9535设备**：
   ```c
   #include "tca9535.h"
   
   tca9535_config_t config = {
       .i2c_port = I2C_MASTER_NUM,
       .device_addr = TCA9535_I2C_ADDR,
       .timeout_ms = 1000
   };
   
   tca9535_handle_t tca9535_handle;
   tca9535_create(&config, &tca9535_handle);
   ```

3. **配置和使用I/O引脚**：
   ```c
   // 简单使用
   tca9535_set_pin_output(tca9535_handle, 0, 1);
   tca9535_set_pin_input(tca9535_handle, 8);
   
   uint8_t level;
   tca9535_get_pin_level(tca9535_handle, 8, &level);
   ```

4. **清理资源**：
   ```c
   tca9535_delete(tca9535_handle);
   ```

## 寄存器说明

| 寄存器 | 地址 | 说明 |
|--------|------|------|
| INPUT_REG0/1 | 0x00/0x01 | 输入端口寄存器 |
| OUTPUT_REG0/1 | 0x02/0x03 | 输出端口寄存器 |
| POLARITY_REG0/1 | 0x04/0x05 | 极性反转寄存器 |
| CONFIG_REG0/1 | 0x06/0x07 | 配置寄存器 (1=输入, 0=输出) |

## 引脚编号

```
端口0: 引脚 0-7
端口1: 引脚 8-15
```

## 依赖

- ESP-IDF 5.5+
- main组件（提供I2C配置）
- driver组件（I2C驱动）
- esp_log组件（日志输出）

## 示例代码

详细的使用示例请参考 `tca9535_example.c` 文件。

## 错误代码

- `ESP_OK`: 操作成功
- `ESP_ERR_INVALID_ARG`: 参数无效
- `ESP_ERR_NO_MEM`: 内存不足
- `ESP_FAIL`: I2C通信失败

## 注意事项

1. 确保I2C总线已正确初始化
2. 检查设备I2C地址是否正确
3. 确保硬件连接正确，特别是上拉电阻
4. 多个设备共享I2C总线时注意地址冲突
5. 在配置输出前建议先读取当前状态

## 许可证

本组件遵循MIT许可证。
