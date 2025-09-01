# ESP32 UART Shell 集成使用说明

## 🚀 概述
本项目已成功集成ESP32Shell组件，为UART1和UART2提供独立的命令行界面。

## 📌 硬件配置

### UART1
- **TX引脚**: GPIO22
- **RX引脚**: GPIO23  
- **波特率**: 115200
- **Shell通道ID**: 1
- **提示符**: `UART1> `

### UART2
- **TX引脚**: GPIO27
- **RX引脚**: GPIO26
- **波特率**: 115200  
- **Shell通道ID**: 2
- **提示符**: `UART2> `

## 🛠️ 项目结构

```
main/
├── analog_board_test_main.c    # 主程序文件
├── uart_driver.h              # UART驱动头文件
├── uart_driver.c              # UART驱动实现
└── CMakeLists.txt             # 构建配置
```

## 🔧 使用方法

### 1. 编译和烧录
```bash
idf.py build
idf.py flash monitor
```

### 2. 连接设备
- 使用USB转TTL模块连接ESP32的UART1或UART2
- 设置串口工具参数：115200, 8N1
- 推荐工具：PuTTY、SecureCRT、minicom等

### 3. 基本操作
连接成功后，您会看到对应的提示符：
```
UART1> 
```
或
```
UART2> 
```

输入 `help` 查看所有可用命令。

## 📋 可用命令

### 基础命令
- `help` - 显示帮助信息
- `echo <text>` - 回显文本
- `version` - 显示版本信息（带ASCII艺术）
- `clear` - 清屏
- `test <params>` - 参数测试

### 键值存储命令
- `kv set <key> <value>` - 设置键值对
- `kv get <key>` - 获取键值
- `kv del <key>` - 删除键值对
- `kv list` - 列出所有键值对
- `kv clear` - 清空所有键值对
- `kv count` - 显示键值对数量

### 宏命令
- `macro <name>` - 开始录制宏
- `endmacro` - 停止录制宏
- `exec macro` - 执行第一个宏
- `exec <name>` - 执行指定宏
- `buffer` - 宏缓冲区管理

### 系统命令
- `tasks` - 显示FreeRTOS任务信息
- `heap` - 显示内存使用情况
- `cpu` - 显示CPU使用率
- `uptime` - 显示系统运行时间
- `reset` - 系统重启
- `delay <ms>` - 延时测试

### FreeRTOS命令
- `queue` - 队列操作
- `sem` - 信号量操作  
- `timer` - 定时器操作

## 💡 使用示例

### 1. 基本测试
```
UART1> help
=== ESP32 Shell 命令列表 ===
...

UART1> echo Hello World
Echo: Hello World

UART1> version
立控esp32shell v1.0
```

### 2. 键值存储
```
UART1> kv set temperature 25
设置成功: temperature = 25

UART1> kv set humidity 60
设置成功: humidity = 60

UART1> kv list
=== 键值对列表 (2个) ===
temperature = 25
humidity = 60
==================

UART1> kv get temperature
temperature = 25
```

### 3. 宏录制和执行
```
UART1> macro test_macro
开始录制宏: test_macro

UART1> echo 这是第一条命令
已添加到宏: echo 这是第一条命令

UART1> kv set count 10
已添加到宏: kv set count 10

UART1> endmacro
停止录制宏

UART1> exec test_macro
宏 'test_macro' 执行完成
```

### 4. 系统监控
```
UART1> tasks
=== FreeRTOS 任务信息 ===
...

UART1> heap
=== 内存使用情况 ===
...

UART1> cpu
=== CPU 使用率 ===
...
```

## 🔍 调试功能

### 日志输出
系统会通过ESP_LOG输出详细的调试信息，包括：
- UART驱动初始化状态
- Shell实例创建状态
- 命令接收和执行日志
- 错误信息

### 多通道独立性
- UART1和UART2的Shell实例完全独立
- 各自维护独立的键值存储
- 各自维护独立的宏缓冲区
- 命令执行不会相互影响

## ⚠️ 注意事项

1. **硬件连接**：确保UART引脚连接正确，TX-RX交叉连接
2. **波特率设置**：串口工具必须设置为115200波特率
3. **命令格式**：命令参数用空格分隔，大小写敏感
4. **内存限制**：宏录制和键值存储有数量限制
5. **线程安全**：所有操作都是线程安全的，支持并发访问

## 🚀 扩展开发

如需添加自定义命令：

1. 在相应的cmd文件中添加命令实现
2. 在`cmd.c`的`cmd_init()`函数中注册新命令
3. 重新编译烧录

示例：
```c
// 在cmd_basic.c中添加
void task_my_command(uint32_t channel_id, const char *params) {
    // 命令实现
}

// 在cmd.c中注册
cmd_register_task("mycmd", task_my_command, "我的自定义命令");
```

## 📞 支持

如有问题，请检查：
1. 硬件连接是否正确
2. 串口工具配置是否正确
3. ESP_LOG输出的错误信息
4. ESP32Shell组件的README文档
