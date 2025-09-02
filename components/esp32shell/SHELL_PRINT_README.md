# Shell统一打印系统

## 概述

Shell统一打印系统是一个完整的编码管理和打印接口系统，为ESP32Shell提供统一的打印功能，支持多种编码格式的自动转换。

## 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    Shell统一打印系统架构                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  应用程序层                                                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   shell_print   │  │shell_print_line │  │shell_print_raw  │ │
│  │   shell_vprint  │  │shell_print_str  │  │shell_snprintf   │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
│                              │                                 │
│  编码管理层                                                    │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                shell_encoding                               │ │
│  │  - 编码类型管理 (UTF-8, GB2312, GBK, ASCII)                │ │
│  │  - 自动编码检测                                            │ │
│  │  - 编码转换 (UTF-8 ↔ GB2312 ↔ ASCII)                      │ │
│  │  - NVS配置持久化                                           │ │
│  └─────────────────────────────────────────────────────────────┘ │
│                              │                                 │
│  输出层                                                        │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                    cmd_output                               │ │
│  │  - 通道管理 (UART1, UART2, 等)                            │ │
│  │  - 数据输出到指定通道                                      │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 主要特性

### 1. 统一打印接口
- **shell_print()**: 基本格式化打印，自动编码转换
- **shell_vprint()**: va_list版本，支持复杂格式化
- **shell_print_with_encoding()**: 指定目标编码的打印
- **shell_print_string()**: 字符串打印
- **shell_print_line()**: 带换行的字符串打印
- **shell_print_raw()**: 原始数据打印，不进行编码转换

### 2. 编码管理
- **多编码支持**: UTF-8, GB2312, GBK, ASCII
- **自动编码检测**: 智能识别输入字符串的编码
- **编码转换**: 自动在源编码和目标编码间转换
- **配置持久化**: 编码设置保存到NVS，重启后保持

### 3. 缓冲区格式化
- **shell_snprintf()**: 格式化到缓冲区，不输出到终端
- **shell_vsnprintf()**: va_list版本的缓冲区格式化

## 使用方法

### 1. 系统初始化

```c
#include "shell_print.h"

// 使用默认配置初始化
if (!shell_print_init(NULL)) {
    ESP_LOGE(TAG, "Shell打印系统初始化失败");
    return;
}

// 或使用自定义配置初始化
shell_encoding_config_t config = {
    .type = SHELL_ENCODING_GB2312,
    .auto_detect = true,
    .fallback_to_ascii = true,
    .max_conversion_size = 4096
};

if (!shell_print_init(&config)) {
    ESP_LOGE(TAG, "Shell打印系统初始化失败");
    return;
}
```

### 2. 基本打印

```c
uint32_t channel_id = 1; // UART1通道

// 基本格式化打印
shell_print(channel_id, "数字: %d, 字符串: %s\r\n", 42, "测试");

// 字符串打印
shell_print_string(channel_id, "这是字符串\r\n");

// 带换行打印
shell_print_line(channel_id, "这是带换行的字符串");

// 原始数据打印
const uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
shell_print_raw(channel_id, data, sizeof(data));
```

### 3. 编码控制

```c
// 查询当前编码
shell_encoding_type_t current = shell_print_get_encoding();
ESP_LOGI(TAG, "当前编码: %s", shell_encoding_get_name(current));

// 切换编码
if (shell_print_set_encoding(SHELL_ENCODING_GB2312)) {
    ESP_LOGI(TAG, "编码已切换到GB2312");
}

// 强制使用指定编码打印
shell_print_with_encoding(channel_id, SHELL_ENCODING_UTF8, "UTF-8编码: 测试中文");
shell_print_with_encoding(channel_id, SHELL_ENCODING_GB2312, "GB2312编码: 测试中文");
```

### 4. 缓冲区格式化

```c
char buffer[256];

// 格式化到缓冲区
shell_snprintf(buffer, sizeof(buffer), 
               "格式化结果: 数字=%d, 字符串=%s", 100, "缓冲区测试");

// 打印缓冲区内容
shell_print(channel_id, "缓冲区: %s\r\n", buffer);
```

### 5. 文件内容打印到终端

```c
// 模拟文件内容（包含中文）
const char *file_lines[] = {
    "=== 测试文件 ===",
    "第1行：包含中文字符",
    "第2行：数字和符号：123, 456",
    "第3行：结束标记"
};

// 使用当前编码打印
for (int i = 0; i < 4; i++) {
    shell_print_line(channel_id, file_lines[i]);
}

// 使用指定编码打印
for (int i = 0; i < 4; i++) {
    shell_print_with_encoding(channel_id, SHELL_ENCODING_GB2312, "%s\r\n", file_lines[i]);
}
```

## 编码转换规则

### 1. UTF-8 ↔ GB2312
- 支持常用中文字符的转换
- 包含完整的字符映射表
- 转换失败时回退到ASCII

### 2. UTF-8 ↔ ASCII
- 保留所有ASCII字符
- 过滤掉非ASCII字符
- 确保输出兼容性

### 3. GB2312 ↔ ASCII
- 保留所有ASCII字符
- 过滤掉中文字符
- 适合纯英文环境

## 配置选项

### shell_encoding_config_t 结构

```c
typedef struct {
    shell_encoding_type_t type;     // 默认编码类型
    bool auto_detect;               // 是否自动检测编码
    bool fallback_to_ascii;         // 转换失败时是否回退到ASCII
    size_t max_conversion_size;     // 最大转换缓冲区大小
} shell_encoding_config_t;
```

### 编码类型

```c
typedef enum {
    SHELL_ENCODING_UTF8 = 0,        // UTF-8编码
    SHELL_ENCODING_GB2312 = 1,      // GB2312编码
    SHELL_ENCODING_GBK = 2,         // GBK编码
    SHELL_ENCODING_ASCII = 3,       // ASCII编码
    SHELL_ENCODING_MAX              // 编码类型数量
} shell_encoding_type_t;
```

## 错误处理

### 1. 初始化检查
```c
if (!shell_print_is_initialized()) {
    ESP_LOGE(TAG, "Shell打印系统未初始化");
    return;
}
```

### 2. 返回值检查
```c
int result = shell_print(channel_id, "测试打印");
if (result < 0) {
    ESP_LOGE(TAG, "打印失败");
} else {
    ESP_LOGI(TAG, "打印成功，字符数: %d", result);
}
```

### 3. 编码转换失败处理
```c
// 编码转换失败时会自动回退到原始数据
// 并在日志中输出警告信息
```

## 性能考虑

### 1. 内存使用
- 转换缓冲区大小可配置
- 默认最大转换大小：4KB
- 支持动态内存分配

### 2. 转换效率
- 使用查找表进行字符映射
- 避免重复转换相同内容
- 支持批量转换

### 3. 缓存策略
- 编码设置缓存到NVS
- 避免重复的编码检测
- 支持热切换编码

## 集成到现有项目

### 1. 在main函数中初始化
```c
// 在shell_system_init()之后添加
if (!shell_print_init(NULL)) {
    ESP_LOGE(TAG, "Shell打印系统初始化失败");
    return;
}
```

### 2. 替换现有打印调用
```c
// 旧方式
cmd_output(channel_id, (uint8_t*)response, strlen(response));

// 新方式
shell_print(channel_id, "%s", response);
```

### 3. 更新命令函数
```c
// 在命令处理函数中使用新的打印接口
void task_led_control(uint32_t channel_id, const char *params)
{
    shell_print(channel_id, "LED控制命令: %s\r\n", params);
    // ... 其他逻辑
}
```

## 注意事项

1. **初始化顺序**: 必须在shell_system_init()之后调用shell_print_init()
2. **编码一致性**: 建议在整个项目中使用统一的编码设置
3. **内存管理**: 编码转换结果会自动释放，无需手动管理
4. **线程安全**: 所有函数都是线程安全的
5. **错误处理**: 始终检查初始化状态和返回值

## 故障排除

### 1. 初始化失败
- 检查NVS是否已初始化
- 确认内存是否充足
- 查看日志中的具体错误信息

### 2. 编码转换失败
- 检查源字符串的编码格式
- 确认目标编码是否支持
- 查看字符映射表是否完整

### 3. 打印不显示
- 确认通道ID是否正确
- 检查shell实例是否已创建
- 验证输出函数是否正常工作

## 版本历史

- **v1.0**: 初始版本，支持基本编码转换
- **v1.1**: 添加NVS配置持久化
- **v1.2**: 优化编码检测算法
- **v1.3**: 添加GBK编码支持
- **v2.0**: 重构为统一打印系统
