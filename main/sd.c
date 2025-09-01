#include "sd.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "soc/gpio_num.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "SD_CARD";

// 全局变量
static sdmmc_card_t *g_sd_card = NULL;
static bool g_sd_mounted = false;

esp_err_t sd_card_init(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "开始初始化SD卡...");
    
    // 检查是否已经挂载
    if (g_sd_mounted) {
        ESP_LOGW(TAG, "SD卡已经挂载");
        return ESP_OK;
    }
    
    // 1) 初始化 SPI 总线
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
        .flags = 0,
        .intr_flags = 0,
    };
    
    ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI总线初始化失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 2) 准备挂载配置
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,   // 不自动格式化
        .max_files = 10,                   // 最大同时打开文件数
        .allocation_unit_size = 16 * 1024, // 分配单元大小
    };
    
    // 3) SDSPI 设备配置
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = SD_SPI_HOST;
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.gpio_cd = SDSPI_SLOT_NO_CD;   // 无卡检测
    slot_config.gpio_wp = SDSPI_SLOT_NO_WP;   // 无写保护
    slot_config.gpio_int = SDSPI_SLOT_NO_INT; // 不使用中断
    
    // 4) 配置SDSPI主机
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 400;  // 降低频率提高稳定性
    
    // 5) 挂载SD卡
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &g_sd_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡挂载失败: %s", esp_err_to_name(ret));
        spi_bus_free(SD_SPI_HOST);
        return ret;
    }
    
    g_sd_mounted = true;
    ESP_LOGI(TAG, "SD卡成功挂载到 %s", SD_MOUNT_POINT);
    
    // 打印SD卡信息
    if (g_sd_card) {
        ESP_LOGI(TAG, "SD卡信息:");
        ESP_LOGI(TAG, "  名称: %s", g_sd_card->cid.name);
        ESP_LOGI(TAG, "  容量: %llu MB", ((uint64_t)g_sd_card->csd.capacity) * g_sd_card->csd.sector_size / (1024 * 1024));
        ESP_LOGI(TAG, "  扇区大小: %d bytes", g_sd_card->csd.sector_size);
        ESP_LOGI(TAG, "  最大频率: %d kHz", g_sd_card->max_freq_khz);
    }
    
    return ESP_OK;
}

esp_err_t sd_card_deinit(void) {
    if (!g_sd_mounted) {
        ESP_LOGW(TAG, "SD卡未挂载");
        return ESP_OK;
    }
    
    // 卸载SD卡
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, g_sd_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡卸载失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 释放SPI总线
    spi_bus_free(SD_SPI_HOST);
    
    g_sd_card = NULL;
    g_sd_mounted = false;
    
    ESP_LOGI(TAG, "SD卡已卸载");
    return ESP_OK;
}

bool sd_card_is_mounted(void) {
    return g_sd_mounted;
}

sdmmc_card_t* sd_card_get_info(void) {
    return g_sd_card;
}

void sd_list_directory(const char* path) {
    if (!g_sd_mounted) {
        ESP_LOGE(TAG, "SD卡未挂载");
        return;
    }
    
    DIR* dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "无法打开目录: %s", path);
        return;
    }
    
    ESP_LOGI(TAG, "目录内容 %s:", path);
    ESP_LOGI(TAG, "========================================");
    
    struct dirent* entry;
    char full_path[512];  // 增加缓冲区大小
    int file_count = 0, dir_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // 构建完整路径
        int ret = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (ret >= sizeof(full_path)) {
            ESP_LOGW(TAG, "路径太长，跳过: %s", entry->d_name);
            continue;
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ESP_LOGI(TAG, "[目录] %-20s", entry->d_name);
                dir_count++;
            } else {
                ESP_LOGI(TAG, "[文件] %-20s  %ld bytes", entry->d_name, st.st_size);
                file_count++;
            }
        } else {
            ESP_LOGI(TAG, "[未知] %-20s", entry->d_name);
        }
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "统计: %d个文件, %d个目录", file_count, dir_count);
    closedir(dir);
}

esp_err_t sd_card_test_basic(void) {
    if (!g_sd_mounted) {
        ESP_LOGE(TAG, "SD卡未挂载");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "开始SD卡基本功能测试...");
    
    // 简单的读写测试
    const char* test_file = SD_MOUNT_POINT "/test.txt";
    const char* test_content = "ESP32 SD Card Basic Test";
    
    // 写入测试
    FILE *f = fopen(test_file, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "SD卡写入测试失败");
        return ESP_FAIL;
    }
    
    fprintf(f, "%s", test_content);
    fclose(f);
    
    // 读取测试
    f = fopen(test_file, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "SD卡读取测试失败");
        return ESP_FAIL;
    }
    
    char buffer[128];
    if (fgets(buffer, sizeof(buffer), f) != NULL) {
        ESP_LOGI(TAG, "SD卡读写测试成功: %s", buffer);
    }
    fclose(f);
    
    // 删除测试文件
    remove(test_file);
    
    ESP_LOGI(TAG, "SD卡基本功能测试完成");
    return ESP_OK;
}
