/**
 * @file shell_encoding.c
 * @brief Shell编码系统实现
 * 
 * 提供统一的编码管理，支持多种编码格式的自动转换
 */

#include "shell_encoding.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "SHELL_ENCODING";
static const char *NVS_NAMESPACE = "shell_enc";
static const char *NVS_KEY_TYPE = "type";
static const char *NVS_KEY_CONFIG = "config";

// 全局编码配置
static shell_encoding_config_t g_encoding_config = {
    .type = SHELL_ENCODING_UTF8,
    .auto_detect = true,
    .fallback_to_ascii = true,
    .max_conversion_size = 4096
};

static bool g_initialized = false;
static nvs_handle_t g_nvs_handle = 0;

// 编码类型名称和描述
static const char* encoding_names[] = {
    "UTF-8",
    "GB2312", 
    "GBK",
    "ASCII"
};

static const char* encoding_descriptions[] = {
    "UTF-8 Unicode编码",
    "GB2312 简体中文编码",
    "GBK 中文扩展编码", 
    "ASCII 英文编码"
};

// UTF-8到GB2312的映射表（常用字符）
static const struct {
    const char *utf8;
    const char *gb2312;
} utf8_gb2312_map[] = {
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
    {"检", "\xBC\xEC"}, {"事", "\xCA\xC2"}, {"件", "\xBC\xFE"}, {"使", "\xCA\xB9"},
    {"用", "\xD3\xC3"}, {"自", "\xD7\xD4"}, {"错", "\xB4\xED"}, {"误", "\xCE\xF3"},
    {"无", "\xCE\xDE"}, {"法", "\xB7\xA8"}, {"创", "\xB4\xB4"}, {"建", "\xBD\xA8"},
    {"任", "\xC8\xCE"}, {"务", "\xCE\xF1"}, {"运", "\xD4\xCB"}, {"行", "\xD0\xD0"},
    {"隔", "\xB8\xF4"}, {"毫", "\xBA\xC1"}, {"秒", "\xC3\xEB"}, {"编", "\xB1\xE0"},
    {"码", "\xC2\xEB"}, {"配", "\xC5\xE4"}, {"置", "\xD6\xC3"}, {"字", "\xD7\xD6"},
    {"符", "\xB7\xFB"}, {"格", "\xB8\xF1"}, {"式", "\xCA\xBD"}, {"熄", "\xCF\xA8"},
    {"灭", "\xC3\xF0"}, {"已", "\xD2\xD1"}, {"连", "\xC1\xAC"}, {"接", "\xBD\xD3"},
    {"挂", "\xB9\xD2"}, {"载", "\xD4\xD8"}, {"状", "\xD7\xB4"}, {"态", "\xCC\xAC"},
    {"失", "\xCA\xA7"}, {"败", "\xB0\xDC"}, {"成", "\xB3\xC9"}, {"可", "\xBF\xC9"},
    {"等", "\xB5\xC8"}, {"命", "\xC3\xFC"}, {"令", "\xC1\xEE"}, {"帮", "\xB0\xEF"},
    {"助", "\xD6\xFA"}, {"信", "\xD0\xC5"}, {"息", "\xCF\xA2"}, {"系", "\xCF\xB5"},
    {"统", "\xCD\xB3"}, {"版", "\xB0\xE6"}, {"本", "\xB1\xBE"}, {"文", "\xCE\xC4"},
    {"目", "\xC4\xBF"}, {"操", "\xB2\xD9"}, {"作", "\xD7\xF7"}, {"参", "\xB2\xCE"},
    {"选", "\xD1\xA1"}, {"项", "\xCF\xEE"}, {"总", "\xD7\xDC"}, {"共", "\xB9\xB2"},
    {"个", "\xB8\xF6"}, {"提", "\xCC\xE1"}, {"查", "\xB2\xE9"}, {"看", "\xBF\xB4"},
    {"详", "\xCF\xEA"}, {"细", "\xCF\xB8"}, {"例", "\xC0\xFD"}, {"子", "\xD7\xD3"},
    {"回", "\xBB\xD8"}, {"入", "\xC8\xEB"}, {"清", "\xC7\xE5"}, {"屏", "\xC6\xC1"},
    {"所", "\xCB\xF9"}, {"有", "\xD3\xD0"}, {"控", "\xBF\xD8"}, {"制", "\xD6\xC6"},
    {"切", "\xC7\xD0"}, {"换", "\xBB\xBB"}, {"闪", "\xC9\xC1"}, {"烁", "\xCB\xB8"},
    {"重", "\xD6\xD8"}, {"启", "\xC6\xF4"}, {"宏", "\xBA\xEA"}, {"缓", "\xBB\xBA"},
    {"冲", "\xB3\xE5"}, {"区", "\xC7\xF8"}, {"管", "\xB9\xDC"}, {"理", "\xC0\xED"},
    {"键", "\xBC\xFC"}, {"值", "\xD6\xB5"}, {"储", "\xB4\xA2"}, {"存", "\xB4\xE6"},
    {"基", "\xBB\xF9"}, {"础", "\xB4\xA1"}, {"测", "\xB2\xE2"}, {"执", "\xD6\xB4"},
    {"队", "\xB6\xD3"}, {"列", "\xC1\xD0"}, {"号", "\xBA\xC5"}, {"量", "\xC1\xBF"},
    {"定", "\xB6\xA8"}, {"器", "\xC6\xF7"}, {"周", "\xD6\xDC"}, {"期", "\xC6\xDA"},
    {"工", "\xB9\xA4"}, {"移", "\xD2\xC6"}, {"名", "\xC3\xFB"}, {"复", "\xB8\xB4"},
    {"制", "\xD6\xC6"}, {"删", "\xC9\xBE"}, {"除", "\xB3\xFD"}, {"更", "\xB8\xFC"},
    {"新", "\xD0\xC2"}, {"递", "\xB5\xDD"}, {"归", "\xB9\xE9"}, {"非", "\xB7\xC7"},
    {"模", "\xC4\xA3"}, {"在", "\xD4\xDA"}, {"跳", "\xCC\xF8"}, {"转", "\xD7\xAA"},
    {"仅", "\xBD\xF6"}, {"延", "\xD1\xD3"}, {"立", "\xC1\xA2"}, {"硬", "\xD3\xB2"},
    {"固", "\xB9\xCC"}, {"艺", "\xD2\xD5"}, {"术", "\xCA\xF5"}, {"图", "\xCD\xBC"},
    {"通", "\xCD\xA8"}, {"道", "\xB5\xC0"}, {"长", "\xB3\xA4"}, {"度", "\xB6\xC8"},
    {"获", "\xBB\xF1"}, {"取", "\xC8\xA1"}, {"出", "\xB3\xF6"}, {"空", "\xBF\xD5"},
    {"足", "\xD7\xE3"}, {"找", "\xD5\xD2"}, {"实", "\xCA\xB5"}, {"需", "\xD0\xE8"},
    {"要", "\xD2\xAA"}, {"为", "\xCE\xAA"}, {"知", "\xD6\xAA"}, {"保", "\xB1\xA3"},
    {"各", "\xB8\xF7"}, {"正", "\xD5\xFD"}, {"录", "\xC2\xBC"}, {"中", "\xD6\xD0"},
    {"完", "\xCD\xEA"}, {"没", "\xC3\xBB"}, {"现", "\xCF\xD6"}, {"暂", "\xD4\xDD"},
    {"简", "\xBC\xF2"}, {"具", "\xBE\xDF"}, {"体", "\xCC\xC5"}, {"去", "\xC8\xA5"},
    {"带", "\xB4\xF8"}, {"指", "\xD6\xB8"}, {"条", "\xCC\xF8"}, {"件", "\xBC\xFE"},
    {"时", "\xCA\xB1"}, {"定", "\xB6\xA8"}, {"服", "\xB7\xFE"}, {"务", "\xCE\xF1"},
    {"戳", "\xB4\xC1"}, {"过", "\xB9\xFD"}, {"卡", "\xBF\xA1"}, {"载", "\xD4\xD8"},
    {"志", "\xD6\xBE"}, {"告", "\xB8\xE6"}, {"将", "\xBD\xAB"}, {"连", "\xC1\xAC"},
    {"日", "\xC8\xD5"}, {"警", "\xBE\xAF"}, {"次", "\xB4\xCE"}, {"长", "\xB3\xA4"},
    {"终", "\xD6\xD5"}, {"端", "\xB6\xCB"},
    {"设", "\xC9\xE8"}, {"请", "\xC7\xEB"}, {"备", "\xB1\xB8"}, {"生", "\xC9\xFA"},
    {"效", "\xD0\xA7"}, {"注", "\xD7\xA2"}, {"意", "\xD2\xE2"}, {"支", "\xD6\xA7"},
    {"常", "\xB3\xA3"}, {"议", "\xD2\xE9"}, {"串", "\xB4\xAE"}, {"口", "\xBF\xDA"},
    {"或", "\xBB\xF2"}, {"未", "\xCE\xB4"}, {"是", "\xCA\xC7"}, {"否", "\xB7\xF0"},
    {"正", "\xD5\xFD"}, {"确", "\xC8\xB7"}, {"错", "\xB4\xED"}, {"误", "\xCE\xF3"},
    {"成", "\xB3\xC9"}, {"功", "\xB9\xA6"},
    {"堆", "\xB6\xD1"}, {"最", "\xD7\xEE"}, {"小", "\xD0\xA1"},
    {"拟", "\xC4\xE3"}, {"板", "\xB0\xE5"}, {"会", "\xBB\xE1"}, {"话", "\xBB\xB0"}, {"结", "\xBD\xE1"}, {"束", "\xCA\xF8"}
};

static const size_t utf8_gb2312_map_size = sizeof(utf8_gb2312_map) / sizeof(utf8_gb2312_map[0]);

/**
 * @brief 从NVS加载编码配置
 */
static esp_err_t load_encoding_config_from_nvs(void)
{
    if (g_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t type_value = g_encoding_config.type;
    size_t config_size = sizeof(shell_encoding_config_t);
    
    // 尝试加载编码类型
    esp_err_t ret = nvs_get_u8(g_nvs_handle, NVS_KEY_TYPE, &type_value);
    if (ret == ESP_OK) {
        g_encoding_config.type = (shell_encoding_type_t)type_value;
        ESP_LOGI(TAG, "从NVS加载编码类型: %s", encoding_names[g_encoding_config.type]);
    }
    
    // 尝试加载完整配置
    ret = nvs_get_blob(g_nvs_handle, NVS_KEY_CONFIG, &g_encoding_config, &config_size);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "从NVS加载完整编码配置");
    }
    
    return ESP_OK;
}

/**
 * @brief 保存编码配置到NVS
 */
static esp_err_t save_encoding_config_to_nvs(void)
{
    if (g_nvs_handle == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 保存编码类型
    esp_err_t ret = nvs_set_u8(g_nvs_handle, NVS_KEY_TYPE, g_encoding_config.type);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "保存编码类型到NVS失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 保存完整配置
    ret = nvs_set_blob(g_nvs_handle, NVS_KEY_CONFIG, &g_encoding_config, sizeof(shell_encoding_config_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "保存编码配置到NVS失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 提交更改
    ret = nvs_commit(g_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "提交NVS更改失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "编码配置已保存到NVS");
    return ESP_OK;
}

/**
 * @brief UTF-8转GB2312
 */
static size_t utf8_to_gb2312(const char *utf8_str, char *gb2312_str, size_t gb2312_size)
{
    if (!utf8_str || !gb2312_str || gb2312_size == 0) {
        return 0;
    }
    
    size_t gb2312_len = 0;
    const char *p = utf8_str;
    
    while (*p && gb2312_len < gb2312_size - 1) {
        // 检查是否是UTF-8多字节字符
        if ((*p & 0x80) == 0) {
            // ASCII字符，直接复制
            gb2312_str[gb2312_len++] = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            // UTF-8双字节字符
            if (*(p + 1) && (*(p + 1) & 0xC0) == 0x80) {
                // 构造临时字符串用于查找
                char utf8_char[4] = {p[0], p[1], 0, 0};
                
                // 查找映射表
                bool found = false;
                for (size_t i = 0; i < utf8_gb2312_map_size; i++) {
                    if (strcmp(utf8_char, utf8_gb2312_map[i].utf8) == 0) {
                        const char *gb = utf8_gb2312_map[i].gb2312;
                        if (gb2312_len + 2 <= gb2312_size - 1) {
                            gb2312_str[gb2312_len++] = gb[0];
                            gb2312_str[gb2312_len++] = gb[1];
                            p += 2; // 跳过UTF-8双字节
                            found = true;
                            break;
                        }
                    }
                }
                
                if (!found) {
                    // 没有找到映射，使用问号替代
                    gb2312_str[gb2312_len++] = '?';
                    p += 2; // 跳过UTF-8双字节
                }
            } else {
                // 无效的UTF-8序列，跳过
                p++;
            }
        } else if ((*p & 0xF0) == 0xE0) {
            // UTF-8三字节字符
            if (*(p + 1) && (*(p + 1) & 0xC0) == 0x80 && 
                *(p + 2) && (*(p + 2) & 0xC0) == 0x80) {
                // 构造临时字符串用于查找
                char utf8_char[4] = {p[0], p[1], p[2], 0};
                
                // 查找映射表
                bool found = false;
                for (size_t i = 0; i < utf8_gb2312_map_size; i++) {
                    if (strcmp(utf8_char, utf8_gb2312_map[i].utf8) == 0) {
                        const char *gb = utf8_gb2312_map[i].gb2312;
                        if (gb2312_len + 2 <= gb2312_size - 1) {
                            gb2312_str[gb2312_len++] = gb[0];
                            gb2312_str[gb2312_len++] = gb[1];
                            p += 3; // 跳过UTF-8三字节
                            found = true;
                            break;
                        }
                    }
                }
                
                if (!found) {
                    // 没有找到映射，使用问号替代
                    gb2312_str[gb2312_len++] = '?';
                    p += 3; // 跳过UTF-8三字节
                }
            } else {
                // 无效的UTF-8序列，跳过
                p += 3;
            }
        } else {
            // 无效的UTF-8序列，跳过
            p++;
        }
    }
    
    gb2312_str[gb2312_len] = '\0';
    return gb2312_len;
}

/**
 * @brief GB2312转UTF-8
 */
static size_t gb2312_to_utf8(const char *gb2312_str, char *utf8_str, size_t utf8_size)
{
    if (!gb2312_str || !utf8_str || utf8_size == 0) {
        return 0;
    }
    
    size_t utf8_len = 0;
    const char *p = gb2312_str;
    
    while (*p && utf8_len < utf8_size - 1) {
        if ((*p & 0x80) && *(p + 1)) {
            // 可能是GB2312字符
            bool found = false;
            for (size_t i = 0; i < utf8_gb2312_map_size; i++) {
                const char *gb = utf8_gb2312_map[i].gb2312;
                if (gb[0] == p[0] && gb[1] == p[1]) {
                    const char *utf = utf8_gb2312_map[i].utf8;
                    size_t utf_char_len = strlen(utf);
                    if (utf8_len + utf_char_len <= utf8_size - 1) {
                        strcpy(utf8_str + utf8_len, utf);
                        utf8_len += utf_char_len;
                        p += 2;
                        found = true;
                        break;
                    }
                }
            }
            
            if (!found) {
                // 没有找到映射，使用问号替代
                if (utf8_len + 3 <= utf8_size - 1) {
                    strcpy(utf8_str + utf8_len, "?");
                    utf8_len += 1;
                    p += 2;
                }
            }
        } else {
            // ASCII字符
            utf8_str[utf8_len++] = *p++;
        }
    }
    
    utf8_str[utf8_len] = '\0';
    return utf8_len;
}

bool shell_encoding_init(const shell_encoding_config_t *config)
{
    if (g_initialized) {
        ESP_LOGW(TAG, "编码系统已经初始化");
        return true;
    }
    
    // 打开NVS命名空间
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &g_nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "打开NVS命名空间失败: %s", esp_err_to_name(ret));
        return false;
    }
    
    // 如果提供了配置，使用它；否则使用默认配置
    if (config) {
        memcpy(&g_encoding_config, config, sizeof(shell_encoding_config_t));
    }
    
    // 从NVS加载配置
    load_encoding_config_from_nvs();
    
    g_initialized = true;
    ESP_LOGI(TAG, "编码系统初始化成功，当前编码: %s", encoding_names[g_encoding_config.type]);
    
    return true;
}

void shell_encoding_deinit(void)
{
    if (!g_initialized) {
        return;
    }
    
    if (g_nvs_handle != 0) {
        nvs_close(g_nvs_handle);
        g_nvs_handle = 0;
    }
    
    g_initialized = false;
    ESP_LOGI(TAG, "编码系统已反初始化");
}

bool shell_encoding_set_global(shell_encoding_type_t encoding_type)
{
    if (!g_initialized) {
        ESP_LOGE(TAG, "编码系统未初始化");
        return false;
    }
    
    if (encoding_type >= SHELL_ENCODING_MAX) {
        ESP_LOGE(TAG, "无效的编码类型: %d", encoding_type);
        return false;
    }
    
    g_encoding_config.type = encoding_type;
    ESP_LOGI(TAG, "全局编码已设置为: %s", encoding_names[encoding_type]);
    
    // 保存到NVS
    save_encoding_config_to_nvs();
    
    return true;
}

shell_encoding_type_t shell_encoding_get_global(void)
{
    return g_encoding_config.type;
}

bool shell_encoding_get_config(shell_encoding_config_t *config)
{
    if (!g_initialized || !config) {
        return false;
    }
    
    memcpy(config, &g_encoding_config, sizeof(shell_encoding_config_t));
    return true;
}

shell_encoding_result_t shell_encoding_convert(
    const uint8_t *input, 
    size_t input_length,
    shell_encoding_type_t source_encoding,
    shell_encoding_type_t target_encoding)
{
    shell_encoding_result_t result = {0};
    
    if (!input || input_length == 0) {
        result.success = false;
        return result;
    }
    
    // 如果源编码和目标编码相同，直接返回
    if (source_encoding == target_encoding) {
        result.data = malloc(input_length);
        if (result.data) {
            memcpy(result.data, input, input_length);
            result.length = input_length;
            result.success = true;
            result.source = source_encoding;
            result.target = target_encoding;
        }
        return result;
    }
    
    // 分配转换缓冲区
    size_t max_output_size = input_length * 4; // 最坏情况：UTF-8可能占用4倍空间
    if (max_output_size > g_encoding_config.max_conversion_size) {
        max_output_size = g_encoding_config.max_conversion_size;
    }
    
    result.data = malloc(max_output_size);
    if (!result.data) {
        result.success = false;
        return result;
    }
    
    result.source = source_encoding;
    result.target = target_encoding;
    
    // 执行编码转换
    switch (source_encoding) {
        case SHELL_ENCODING_UTF8:
            if (target_encoding == SHELL_ENCODING_GB2312) {
                result.length = utf8_to_gb2312((const char*)input, (char*)result.data, max_output_size);
                result.success = (result.length > 0);
            } else if (target_encoding == SHELL_ENCODING_ASCII) {
                // UTF-8转ASCII：只保留ASCII字符
                size_t j = 0;
                for (size_t i = 0; i < input_length && j < max_output_size - 1; i++) {
                    if ((input[i] & 0x80) == 0) {
                        result.data[j++] = input[i];
                    }
                }
                result.data[j] = '\0';
                result.length = j;
                result.success = true;
            } else {
                result.success = false;
            }
            break;
            
        case SHELL_ENCODING_GB2312:
            if (target_encoding == SHELL_ENCODING_UTF8) {
                result.length = gb2312_to_utf8((const char*)input, (char*)result.data, max_output_size);
                result.success = (result.length > 0);
            } else if (target_encoding == SHELL_ENCODING_ASCII) {
                // GB2312转ASCII：只保留ASCII字符
                size_t j = 0;
                for (size_t i = 0; i < input_length && j < max_output_size - 1; i++) {
                    if ((input[i] & 0x80) == 0) {
                        result.data[j++] = input[i];
                    } else {
                        i++; // 跳过下一个字节（GB2312是双字节编码）
                    }
                }
                result.data[j] = '\0';
                result.length = j;
                result.success = true;
            } else {
                result.success = false;
            }
            break;
            
        case SHELL_ENCODING_ASCII:
            if (target_encoding == SHELL_ENCODING_UTF8 || target_encoding == SHELL_ENCODING_GB2312) {
                // ASCII转其他编码：直接复制
                memcpy(result.data, input, input_length);
                result.length = input_length;
                result.success = true;
            } else {
                result.success = false;
            }
            break;
            
        default:
            result.success = false;
            break;
    }
    
    if (!result.success) {
        free(result.data);
        result.data = NULL;
        result.length = 0;
    }
    
    return result;
}

void shell_encoding_free_result(shell_encoding_result_t *result)
{
    if (result && result->data) {
        free(result->data);
        result->data = NULL;
        result->length = 0;
        result->success = false;
    }
}

shell_encoding_type_t shell_encoding_detect(const uint8_t *data, size_t length)
{
    if (!data || length == 0) {
        return SHELL_ENCODING_ASCII;
    }
    
    // 简单的编码检测逻辑
    bool has_gb2312 = false;
    bool has_utf8 = false;
    
    for (size_t i = 0; i < length; i++) {
        if ((data[i] & 0x80) == 0) {
            // ASCII字符
            continue;
        } else if ((data[i] & 0xE0) == 0xC0) {
            // UTF-8双字节字符
            if (i + 1 < length && (data[i + 1] & 0xC0) == 0x80) {
                has_utf8 = true;
                i++; // 跳过下一个字节
            }
        } else if ((data[i] & 0xF0) == 0xE0) {
            // UTF-8三字节字符
            if (i + 2 < length && (data[i + 1] & 0xC0) == 0x80 && (data[i + 2] & 0xC0) == 0x80) {
                has_utf8 = true;
                i += 2; // 跳过下两个字节
            }
        } else if ((data[i] & 0x80) && i + 1 < length) {
            // 可能是GB2312双字节字符
            has_gb2312 = true;
            i++; // 跳过下一个字节
        }
    }
    
    if (has_utf8) {
        return SHELL_ENCODING_UTF8;
    } else if (has_gb2312) {
        return SHELL_ENCODING_GB2312;
    } else {
        return SHELL_ENCODING_ASCII;
    }
}

bool shell_encoding_contains_chinese(const char *str)
{
    if (!str) {
        return false;
    }
    
    while (*str) {
        if ((*str & 0x80) != 0) {
            return true;
        }
        str++;
    }
    
    return false;
}

const char* shell_encoding_get_name(shell_encoding_type_t encoding_type)
{
    if (encoding_type >= SHELL_ENCODING_MAX) {
        return "未知";
    }
    return encoding_names[encoding_type];
}

const char* shell_encoding_get_description(shell_encoding_type_t encoding_type)
{
    if (encoding_type >= SHELL_ENCODING_MAX) {
        return "未知编码类型";
    }
    return encoding_descriptions[encoding_type];
}
