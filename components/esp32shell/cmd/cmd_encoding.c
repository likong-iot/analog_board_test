/**
 * @file cmd_encoding.c
 * @brief ESP32Shell编码配置命令实现
 */

#include "cmd_encoding.h"
#include "shell.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SHELL_ENCODING";
static const char *NVS_NAMESPACE = "shell_enc";
static const char *NVS_KEY_TYPE = "type";

static shell_encoding_type_t current_encoding = SHELL_ENCODING_GB2312;
static bool initialized = false;

/**
 * @brief UTF-8转GB2312的映射表项
 */
typedef struct {
    const char *utf8;
    const char *gb2312;
} utf8_to_gb2312_map_t;

// 完整的UTF-8到GB2312映射表 - 使用单字符映射策略以保证转换一致性
static const utf8_to_gb2312_map_t utf8_gb2312_map[] = {
    
    // 缺失的关键字符 (优先添加)
    {"设", "\xC9\xE8"}, {"推", "\xCD\xC6"}, {"荐", "\xBC\xF6"}, {"兼", "\xBC\xE6"},
    {"老", "\xC0\xCF"}, {"旧", "\xBE\xC9"}, {"代", "\xB4\xFA"}, {"标", "\xB1\xEA"},
    {"准", "\xD7\xBC"}, {"议", "\xD2\xE9"}, {"适", "\xCA\xCA"}, {"于", "\xD3\xDA"},
    {"只", "\xD6\xBB"}, {"后", "\xBA\xF3"}, {"生", "\xC9\xFA"}, {"效", "\xD0\xA7"},
    {"串", "\xB4\xAE"}, {"口", "\xBF\xDA"}, {"支", "\xD6\xA7"},
    {"注", "\xD7\xA2"}, {"意", "\xD2\xE2"},
    
    // 单个汉字 (关键字符)
    {"测", "\xB2\xE2"}, {"试", "\xCA\xD4"}, {"循", "\xD1\xAD"}, {"环", "\xBB\xB7"},
    {"按", "\xB0\xB4"}, {"键", "\xBC\xFC"}, {"下", "\xCF\xC2"}, {"松", "\xCB\xC9"},
    {"开", "\xBF\xAA"}, {"时", "\xCA\xB1"}, {"间", "\xBC\xE4"}, {"当", "\xB5\xB1"},
    {"前", "\xC7\xB0"}, {"拉", "\xC0\xAD"}, {"高", "\xB8\xDF"}, {"点", "\xB5\xE3"},
    {"亮", "\xC1\xC1"}, {"电", "\xB5\xE7"}, {"压", "\xD1\xB9"}, {"流", "\xC1\xF7"},
    {"数", "\xCA\xFD"}, {"据", "\xBE\xDD"}, {"启", "\xC6\xF4"}, {"动", "\xB6\xAF"},
    {"停", "\xCD\xA3"}, {"止", "\xD6\xB9"}, {"功", "\xB9\xA6"}, {"能", "\xC4\xDC"},
    {"记", "\xBC\xC7"}, {"录", "\xC2\xBC"}, {"内", "\xC4\xDA"}, {"存", "\xB4\xE6"},
    {"终", "\xD6\xD5"}, {"端", "\xB6\xCB"}, {"打", "\xB4\xF2"}, {"印", "\xD3\xA1"},
    {"持", "\xB3\xD6"}, {"续", "\xD0\xF8"}, {"显", "\xCF\xD4"}, {"示", "\xCA\xBE"},
    {"检", "\xBC\xEC"}, {"测", "\xB2\xE2"}, {"事", "\xCA\xC2"}, {"件", "\xBC\xFE"},
    {"使", "\xCA\xB9"}, {"用", "\xD3\xC3"}, {"自", "\xD7\xD4"}, {"错", "\xB4\xED"},
    {"误", "\xCE\xF3"}, {"无", "\xCE\xDE"}, {"法", "\xB7\xA8"}, {"创", "\xB4\xB4"},
    {"建", "\xBD\xA8"}, {"任", "\xC8\xCE"}, {"务", "\xCE\xF1"}, {"运", "\xD4\xCB"},
    {"行", "\xD0\xD0"}, {"隔", "\xB8\xF4"}, {"毫", "\xBA\xC1"}, {"秒", "\xC3\xEB"},
    {"编", "\xB1\xE0"}, {"码", "\xC2\xEB"}, {"配", "\xC5\xE4"}, {"置", "\xD6\xC3"},
    {"字", "\xD7\xD6"}, {"符", "\xB7\xFB"}, {"格", "\xB8\xF1"}, {"式", "\xCA\xBD"},
    {"熄", "\xCF\xA8"}, {"灭", "\xC3\xF0"}, {"已", "\xD2\xD1"}, {"连", "\xC1\xAC"},
    {"接", "\xBD\xD3"}, {"挂", "\xB9\xD2"}, {"载", "\xD4\xD8"}, {"状", "\xD7\xB4"},
    {"态", "\xCC\xAC"}, {"失", "\xCA\xA7"}, {"败", "\xB0\xDC"}, {"成", "\xB3\xC9"},
    {"可", "\xBF\xC9"}, {"等", "\xB5\xC8"}, {"命", "\xC3\xFC"}, {"令", "\xC1\xEE"},
    {"帮", "\xB0\xEF"}, {"助", "\xD6\xFA"}, {"信", "\xD0\xC5"}, {"息", "\xCF\xA2"},
    {"系", "\xCF\xB5"}, {"统", "\xCD\xB3"}, {"版", "\xB0\xE6"}, {"本", "\xB1\xBE"},
    {"文", "\xCE\xC4"}, {"目", "\xC4\xBF"}, {"操", "\xB2\xD9"}, {"作", "\xD7\xF7"},
    {"参", "\xB2\xCE"}, {"选", "\xD1\xA1"}, {"项", "\xCF\xEE"}, {"总", "\xD7\xDC"},
    {"共", "\xB9\xB2"}, {"个", "\xB8\xF6"}, {"提", "\xCC\xE1"}, {"示", "\xCA\xBE"},
    {"查", "\xB2\xE9"}, {"看", "\xBF\xB4"}, {"详", "\xCF\xEA"}, {"细", "\xCF\xB8"},
    {"用", "\xD3\xC3"}, {"法", "\xB7\xA8"}, {"例", "\xC0\xFD"}, {"子", "\xD7\xD3"},
    {"回", "\xBB\xD8"}, {"显", "\xCF\xD4"}, {"入", "\xC8\xEB"}, {"文", "\xCE\xC4"},
    {"本", "\xB1\xBE"}, {"清", "\xC7\xE5"}, {"屏", "\xC6\xC1"}, {"所", "\xCB\xF9"},
    {"有", "\xD3\xD0"}, {"控", "\xBF\xD8"}, {"制", "\xD6\xC6"}, {"切", "\xC7\xD0"},
    {"换", "\xBB\xBB"}, {"闪", "\xC9\xC1"}, {"烁", "\xCB\xB8"}, {"重", "\xD6\xD8"},
    {"启", "\xC6\xF4"}, {"宏", "\xBA\xEA"}, {"缓", "\xBB\xBA"}, {"冲", "\xB3\xE5"},
    {"区", "\xC7\xF8"}, {"管", "\xB9\xDC"}, {"理", "\xC0\xED"}, {"键", "\xBC\xFC"},
    {"值", "\xD6\xB5"}, {"储", "\xB4\xA2"}, {"存", "\xB4\xE6"}, {"基", "\xBB\xF9"},
    {"础", "\xB4\xA1"}, {"宏", "\xBA\xEA"}, {"测", "\xB2\xE2"}, {"执", "\xD6\xB4"},
    
    // 更多Shell命令相关字符
    {"队", "\xB6\xD3"}, {"列", "\xC1\xD0"}, {"信", "\xD0\xC5"}, {"号", "\xBA\xC5"},
    {"量", "\xC1\xBF"}, {"定", "\xB6\xA8"}, {"器", "\xC6\xF7"}, {"周", "\xD6\xDC"},
    {"期", "\xC6\xDA"}, {"工", "\xB9\xA4"}, {"移", "\xD2\xC6"}, {"名", "\xC3\xFB"},
    {"复", "\xB8\xB4"}, {"制", "\xD6\xC6"}, {"删", "\xC9\xBE"}, {"除", "\xB3\xFD"},
    {"更", "\xB8\xFC"}, {"新", "\xD0\xC2"}, {"递", "\xB5\xDD"}, {"归", "\xB9\xE9"},
    {"非", "\xB7\xC7"}, {"模", "\xC4\xA3"}, {"在", "\xD4\xDA"}, {"跳", "\xCC\xF8"},
    {"转", "\xD7\xAA"}, {"仅", "\xBD\xF6"}, {"延", "\xD1\xD3"}, {"立", "\xC1\xA2"},
    {"硬", "\xD3\xB2"}, {"固", "\xB9\xCC"}, {"艺", "\xD2\xD5"}, {"术", "\xCA\xF5"},
    {"图", "\xCD\xBC"}, {"通", "\xCD\xA8"}, {"道", "\xB5\xC0"}, {"长", "\xB3\xA4"},
    {"度", "\xB6\xC8"}, {"获", "\xBB\xF1"}, {"取", "\xC8\xA1"}, {"列", "\xC1\xD0"},
    {"出", "\xB3\xF6"}, {"空", "\xBF\xD5"}, {"足", "\xD7\xE3"}, {"找", "\xD5\xD2"},
    {"实", "\xCA\xB5"}, {"需", "\xD0\xE8"}, {"要", "\xD2\xAA"}, {"删", "\xC9\xBE"},
    {"为", "\xCE\xAA"}, {"清", "\xC7\xE5"}, {"知", "\xD6\xAA"}, {"保", "\xB1\xA3"},
    {"各", "\xB8\xF7"}, {"正", "\xD5\xFD"}, {"录", "\xC2\xBC"}, {"中", "\xD6\xD0"},
    {"完", "\xCD\xEA"}, {"没", "\xC3\xBB"}, {"可", "\xBF\xC9"}, {"现", "\xCF\xD6"},
    {"暂", "\xD4\xDD"}, {"简", "\xBC\xF2"}, {"要", "\xD2\xAA"}, {"具", "\xBE\xDF"},
    {"体", "\xCC\xE5"}, {"去", "\xC8\xA5"}, {"带", "\xB4\xF8"}, {"指", "\xD6\xB8"},
    {"条", "\xCC\xF5"}, {"件", "\xBC\xFE"}, {"仅", "\xBD\xF6"}, {"延", "\xD1\xD3"},
    {"时", "\xCA\xB1"}, {"指", "\xD6\xB8"}, {"定", "\xB6\xA8"}, {"服", "\xB7\xFE"},
    {"务", "\xCE\xF1"}, {"硬", "\xD3\xB2"}, {"艺", "\xD2\xD5"}, {"术", "\xCA\xF5"},
    
    // 测试相关单字
    {"戳", "\xB4\xC1"}, {"循", "\xD1\xAD"}, {"跳", "\xCC\xF8"}, {"过", "\xB9\xFD"},
    {"卡", "\xBF\xA1"}, {"挂", "\xB9\xD2"}, {"载", "\xD4\xD8"}, {"志", "\xD6\xBE"},
    {"告", "\xB8\xE6"}, {"将", "\xBD\xAB"}, {"连", "\xC1\xAC"}, {"拉", "\xC0\xAD"},
    {"亮", "\xC1\xC1"}, {"日", "\xC8\xD5"}, {"志", "\xD6\xBE"}, {"警", "\xBE\xAF"},
    {"次", "\xB4\xCE"}, {"长", "\xB3\xA4"}, {"终", "\xD6\xD5"}, {"端", "\xB6\xCB"},
    
    // 更多常用单字
    {"看", "\xBF\xB4"}, {"具", "\xBE\xDF"}, {"体", "\xCC\xE5"}, {"基", "\xBB\xF9"},
    {"础", "\xB4\xA1"}, {"的", "\xB5\xC4"}, {"入", "\xC8\xEB"}, {"本", "\xB1\xBE"},
    {"关", "\xB9\xD8"}, {"切", "\xC7\xD0"}, {"换", "\xBB\xBB"}, {"烁", "\xCB\xB8"},
    {"率", "\xC2\xCA"}, {"启", "\xC6\xF4"}, {"定", "\xB6\xA8"}, {"器", "\xC6\xF7"},
    {"空", "\xBF\xD5"}, {"移", "\xD2\xC6"}, {"重", "\xD6\xD8"}, {"名", "\xC3\xFB"},
    {"容", "\xC8\xDD"}, {"更", "\xB8\xFC"}, {"戳", "\xB4\xC1"}, {"找", "\xD5\xD2"},
    {"制", "\xD6\xC6"}, {"跳", "\xCC\xF8"}, {"转", "\xD7\xAA"}, {"条", "\xCC\xF5"},
    {"仅", "\xBD\xF6"}, {"化", "\xBB\xAF"}, {"止", "\xD6\xB9"}, {"格", "\xB8\xF1"},
    
    // 补充缺失的关键字符
    {"总", "\xD7\xDC"}, {"未", "\xCE\xB4"}, {"不", "\xB2\xBB"}, {"包", "\xB0\xFC"}, {"括", "\xC0\xA8"},
    
    // Help命令中的特殊字符
    {"【", "\xA1\xBE"}, {"】", "\xA1\xBF"}, {"？", "\xA3\xBF"}, {"！", "\xA3\xA1"}, {"（", "\xA3\xA8"}, {"）", "\xA3\xA9"},
    {"：", "\xA3\xBA"}, {"；", "\xA3\xBB"}, {"，", "\xA3\xAC"}, {"。", "\xA1\xA3"}, {"、", "\xA1\xA2"},
    
    // 补充缺失的字符（去重）
    {"表", "\xB1\xED"}, {"输", "\xCA\xE4"}, {"和", "\xBA\xCD"}, {"况", "\xBF\xF6"}, {"始", "\xCA\xBC"}, {"带", "\xB4\xF8"},
    {"计", "\xBC\xC6"}, {"数", "\xCA\xFD"}, {"目", "\xC4\xBF"}, {"录", "\xC2\xBC"}, {"总", "\xD7\xDC"}, {"个", "\xB8\xF6"},
    {"文", "\xCE\xC4"}, {"件", "\xBC\xFE"}, {"状", "\xD7\xB4"}, {"情", "\xC7\xE9"},
};

static const size_t utf8_gb2312_map_size = sizeof(utf8_gb2312_map) / sizeof(utf8_gb2312_map[0]);

esp_err_t shell_encoding_init(void)
{
    if (initialized) {
        return ESP_OK;
    }
    
    // 从NVS读取编码设置
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        uint8_t encoding_val = SHELL_ENCODING_GB2312;  // 默认GB2312
        size_t required_size = sizeof(encoding_val);
        ret = nvs_get_blob(nvs_handle, NVS_KEY_TYPE, &encoding_val, &required_size);
        if (ret == ESP_OK) {
            current_encoding = (shell_encoding_type_t)encoding_val;
        }
        nvs_close(nvs_handle);
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Shell编码配置初始化完成，当前编码: %s", shell_encoding_get_name(current_encoding));
    
    // 调试：打印"测"字的UTF-8编码
    const char *test_char = "测";
    ESP_LOGI(TAG, "测字UTF-8编码: 0x%02X 0x%02X 0x%02X", 
             (unsigned char)test_char[0], 
             (unsigned char)test_char[1], 
             (unsigned char)test_char[2]);
    return ESP_OK;
}

esp_err_t shell_encoding_set_type(shell_encoding_type_t encoding)
{
    if (encoding >= 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    current_encoding = encoding;
    
    // 保存到NVS
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        uint8_t encoding_val = (uint8_t)encoding;
        ret = nvs_set_blob(nvs_handle, NVS_KEY_TYPE, &encoding_val, sizeof(encoding_val));
        if (ret == ESP_OK) {
            ret = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    
    ESP_LOGI(TAG, "Shell编码类型已设置为: %s", shell_encoding_get_name(encoding));
    return ret;
}

shell_encoding_type_t shell_encoding_get_type(void)
{
    return current_encoding;
}

const char* shell_encoding_get_name(shell_encoding_type_t encoding)
{
    switch (encoding) {
        case SHELL_ENCODING_UTF8:
            return "UTF-8";
        case SHELL_ENCODING_GB2312:
            return "GB2312";
        default:
            return "Unknown";
    }
}

static esp_err_t convert_utf8_to_gb2312(const char *src, char *dest, size_t dest_size)
{
    if (src == NULL || dest == NULL || dest_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t src_len = strlen(src);
    size_t dest_pos = 0;
    size_t src_pos = 0;
    
    while (src_pos < src_len && dest_pos < dest_size - 1) {
        // 检查是否为ASCII字符（0x00-0x7F），直接复制
        if ((unsigned char)src[src_pos] <= 0x7F) {
            dest[dest_pos++] = src[src_pos++];
            continue;
        }
        
        bool found = false;
        
        // 特殊处理经常出现警告的字符
        if (src_pos + 2 < src_len && dest_pos + 2 < dest_size) {
            if ((unsigned char)src[src_pos] == 0xE6 && (unsigned char)src[src_pos+1] == 0x8B && (unsigned char)src[src_pos+2] == 0x9F) {
                // 拟
                dest[dest_pos++] = 0xC4;
                dest[dest_pos++] = 0xE3;
                src_pos += 3;
                found = true;
            } else if ((unsigned char)src[src_pos] == 0xE6 && (unsigned char)src[src_pos+1] == 0x9D && (unsigned char)src[src_pos+2] == 0xBF) {
                // 板
                dest[dest_pos++] = 0xB0;
                dest[dest_pos++] = 0xE5;
                src_pos += 3;
                found = true;
            } else if ((unsigned char)src[src_pos] == 0xE4 && (unsigned char)src[src_pos+1] == 0xBC && (unsigned char)src[src_pos+2] == 0x9A) {
                // 会
                dest[dest_pos++] = 0xBB;
                dest[dest_pos++] = 0xE1;
                src_pos += 3;
                found = true;
            } else if ((unsigned char)src[src_pos] == 0xE8 && (unsigned char)src[src_pos+1] == 0xAF && (unsigned char)src[src_pos+2] == 0x9D) {
                // 话
                dest[dest_pos++] = 0xBB;
                dest[dest_pos++] = 0xB0;
                src_pos += 3;
                found = true;
            } else if ((unsigned char)src[src_pos] == 0xE7 && (unsigned char)src[src_pos+1] == 0xBB && (unsigned char)src[src_pos+2] == 0x93) {
                // 结
                dest[dest_pos++] = 0xBD;
                dest[dest_pos++] = 0xE1;
                src_pos += 3;
                found = true;
            } else if ((unsigned char)src[src_pos] == 0xE6 && (unsigned char)src[src_pos+1] == 0x9D && (unsigned char)src[src_pos+2] == 0x9F) {
                // 束
                dest[dest_pos++] = 0xCA;
                dest[dest_pos++] = 0xF8;
                src_pos += 3;
                found = true;
            }
        }
        
        // 如果特殊处理没找到，再查找映射表
        if (!found) {
            // 对于非ASCII字符，查找UTF-8到GB2312的映射
            for (size_t i = 0; i < utf8_gb2312_map_size; i++) {
                size_t utf8_len = strlen(utf8_gb2312_map[i].utf8);
                if (src_pos + utf8_len <= src_len && 
                    memcmp(src + src_pos, utf8_gb2312_map[i].utf8, utf8_len) == 0) {
                    // 找到映射，复制GB2312字符
                    size_t gb2312_len = strlen(utf8_gb2312_map[i].gb2312);
                    if (dest_pos + gb2312_len < dest_size) {
                        memcpy(dest + dest_pos, utf8_gb2312_map[i].gb2312, gb2312_len);
                        dest_pos += gb2312_len;
                        src_pos += utf8_len;
                        found = true;
                        // ESP_LOGD(TAG, "找到映射[%zu]: %s -> GB2312(0x%02X 0x%02X)", 
                        //          i, utf8_gb2312_map[i].utf8,
                        //          (unsigned char)utf8_gb2312_map[i].gb2312[0],
                        //          (unsigned char)utf8_gb2312_map[i].gb2312[1]);
                        break;
                    }
                }
            }
        }
        
        if (!found) {
            // 未找到映射的非ASCII字符，限制日志输出频率避免看门狗超时
            static uint32_t last_warn_time = 0;
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if (current_time - last_warn_time > 3000) { // 每3秒最多警告一次
                if (src_pos + 2 < src_len) {
                    ESP_LOGW(TAG, "未找到映射的UTF-8字符: 0x%02X 0x%02X 0x%02X (位置: %zu)", 
                             (unsigned char)src[src_pos], 
                             (unsigned char)src[src_pos+1], 
                             (unsigned char)src[src_pos+2],
                             src_pos);
                } else {
                    ESP_LOGW(TAG, "未找到映射的字符: 0x%02X (位置: %zu)", (unsigned char)src[src_pos], src_pos);
                }
                last_warn_time = current_time;
            }
            
            // 尝试跳过UTF-8字符序列
            if ((unsigned char)src[src_pos] >= 0xE0 && (unsigned char)src[src_pos] <= 0xEF) {
                // 3字节UTF-8字符（大部分中文字符）
                if (src_pos + 2 < src_len) {
                    dest[dest_pos++] = '?';  // 用?替代未知字符
                    src_pos += 3;
                } else {
                    dest[dest_pos++] = src[src_pos++];
                }
            } else if ((unsigned char)src[src_pos] >= 0xC0 && (unsigned char)src[src_pos] <= 0xDF) {
                // 2字节UTF-8字符
                if (src_pos + 1 < src_len) {
                    dest[dest_pos++] = '?';
                    src_pos += 2;
                } else {
                    dest[dest_pos++] = src[src_pos++];
                }
            } else {
                // 其他情况，跳过一个字节
                dest[dest_pos++] = src[src_pos++];
            }
        }
    }
    
    dest[dest_pos] = '\0';
    return ESP_OK;
}

int shell_snprintf(char *dest, size_t dest_size, const char *format, ...)
{
    if (dest == NULL || dest_size == 0) {
        return -1;
    }
    
    // 首先使用UTF-8格式化字符串
    char temp_buffer[1024];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    va_end(args);
    
    if (ret < 0) {
        return ret;
    }
    
    // 根据当前编码设置进行转换
    if (current_encoding == SHELL_ENCODING_UTF8) {
        // 直接复制UTF-8字符串
        strncpy(dest, temp_buffer, dest_size - 1);
        dest[dest_size - 1] = '\0';
    } else {
        // 转换为GB2312
        ESP_LOGD(TAG, "转换为GB2312: %s", temp_buffer);
        esp_err_t conv_ret = convert_utf8_to_gb2312(temp_buffer, dest, dest_size);
        if (conv_ret != ESP_OK) {
            // 转换失败，使用原始UTF-8字符串
            ESP_LOGW(TAG, "转换失败，使用原始UTF-8");
            strncpy(dest, temp_buffer, dest_size - 1);
            dest[dest_size - 1] = '\0';
        } else {
            ESP_LOGD(TAG, "转换成功");
        }
    }
    
    return strlen(dest);
}

void task_shell_encoding(uint32_t channel_id, const char *params)
{
    char response[512];
    
    // 解析参数
    if (params == NULL || strlen(params) == 0) {
        // 显示帮助信息
        shell_snprintf(response, sizeof(response),
                "Shell编码配置命令用法:\r\n"
                "encoding status  - 显示当前编码设置\r\n"
                "encoding utf8    - 设置为UTF-8编码 (推荐)\r\n"
                "encoding gb2312  - 设置为GB2312编码 (兼容老旧串口工具)\r\n"
                "\r\n"
                "注意:\r\n"
                "- UTF-8: 现代标准，建议配置串口工具支持UTF-8\r\n"
                "- GB2312: 兼容模式，适用于只支持GB2312的串口工具\r\n"
                "- 设置后重启生效\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 去掉前导空格
    while (*params == ' ') params++;
    
    if (strncmp(params, "status", 6) == 0) {
        // 显示当前编码状态
        shell_encoding_type_t current = shell_encoding_get_type();
        shell_snprintf(response, sizeof(response),
                "当前Shell编码设置: %s\r\n"
                "编码说明:\r\n"
                "- UTF-8: 国际标准，支持所有字符\r\n"
                "- GB2312: 中文编码，兼容老旧工具\r\n"
                "\r\n"
                "建议: 将串口工具设置为UTF-8编码以获得最佳体验\r\n",
                shell_encoding_get_name(current));
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
    } else if (strncmp(params, "utf8", 4) == 0) {
        // 设置为UTF-8编码
        esp_err_t ret = shell_encoding_set_type(SHELL_ENCODING_UTF8);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Shell编码已设置为UTF-8");
            shell_snprintf(response, sizeof(response),
                    "Shell编码已设置为UTF-8\r\n"
                    "请重启设备使设置生效\r\n"
                    "\r\n"
                    "串口工具设置建议:\r\n"
                    "- PuTTY: Window → Translation → Character set = UTF-8\r\n"
                    "- SecureCRT: Terminal → Character Encoding = UTF-8\r\n"
                    "- 串口助手: 编码格式 → UTF-8\r\n");
        } else {
            ESP_LOGE(TAG, "设置UTF-8编码失败: %s", esp_err_to_name(ret));
            shell_snprintf(response, sizeof(response), "设置UTF-8编码失败: %s\r\n", esp_err_to_name(ret));
        }
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
    } else if (strncmp(params, "gb2312", 6) == 0) {
        // 设置为GB2312编码
        esp_err_t ret = shell_encoding_set_type(SHELL_ENCODING_GB2312);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Shell编码已设置为GB2312");
            shell_snprintf(response, sizeof(response),
                    "Shell编码已设置为GB2312\r\n"
                    "请重启设备使设置生效\r\n"
                    "\r\n"
                    "注意: GB2312编码仅支持项目中使用的常用中文字符\r\n"
                    "建议串口工具设置为GB2312编码\r\n");
        } else {
            ESP_LOGE(TAG, "设置GB2312编码失败: %s", esp_err_to_name(ret));
            shell_snprintf(response, sizeof(response), "设置GB2312编码失败: %s\r\n", esp_err_to_name(ret));
        }
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        
    } else {
        shell_snprintf(response, sizeof(response), "未知参数: %.20s\r\n使用 'encoding' 查看帮助\r\n", params);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
}
