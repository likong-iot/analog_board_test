#ifndef SD_H
#define SD_H

#include "esp_err.h"
#include "sdmmc_cmd.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// SD卡挂载点
#define SD_MOUNT_POINT "/sdcard"

// SPI引脚配置
#define SD_SPI_HOST   HSPI_HOST
#define PIN_NUM_MISO  4
#define PIN_NUM_MOSI  15
#define PIN_NUM_CLK   14
#define PIN_NUM_CS    13

/**
 * @brief SD卡初始化
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t sd_card_init(void);

/**
 * @brief SD卡反初始化
 * @return ESP_OK 成功, 其他值表示失败
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief 检查SD卡是否已挂载
 * @return true 已挂载, false 未挂载
 */
bool sd_card_is_mounted(void);

/**
 * @brief 获取SD卡信息
 * @return sdmmc_card_t* SD卡句柄，NULL表示未挂载
 */
sdmmc_card_t* sd_card_get_info(void);

/**
 * @brief 列出目录内容
 * @param path 目录路径
 */
void sd_list_directory(const char* path);

/**
 * @brief 测试SD卡基本功能
 * @return ESP_OK 测试成功, 其他值表示测试失败
 */
esp_err_t sd_card_test_basic(void);

#ifdef __cplusplus
}
#endif

#endif // SD_H
