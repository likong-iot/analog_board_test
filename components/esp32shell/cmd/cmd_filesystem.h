#ifndef __CMD_FILESYSTEM_H__
#define __CMD_FILESYSTEM_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief pwd命令 - 显示当前工作目录
 * @param channel_id 通信通道ID
 * @param params 参数
 */
void task_pwd(uint32_t channel_id, const char *params);

/**
 * @brief cd命令 - 切换工作目录
 * @param channel_id 通信通道ID
 * @param params 参数 (目录路径)
 */
void task_cd(uint32_t channel_id, const char *params);

/**
 * @brief ls命令 - 列出目录内容
 * @param channel_id 通信通道ID
 * @param params 参数 (可选目录路径)
 */
void task_ls(uint32_t channel_id, const char *params);

/**
 * @brief mkdir命令 - 创建目录
 * @param channel_id 通信通道ID
 * @param params 参数 (目录名)
 */
void task_mkdir(uint32_t channel_id, const char *params);

/**
 * @brief rmdir命令 - 删除空目录
 * @param channel_id 通信通道ID
 * @param params 参数 (目录名)
 */
void task_rmdir(uint32_t channel_id, const char *params);

/**
 * @brief rm命令 - 删除文件
 * @param channel_id 通信通道ID
 * @param params 参数 (文件名)
 */
void task_rm(uint32_t channel_id, const char *params);

/**
 * @brief cp命令 - 复制文件
 * @param channel_id 通信通道ID
 * @param params 参数 (源文件 目标文件)
 */
void task_cp(uint32_t channel_id, const char *params);

/**
 * @brief mv命令 - 移动/重命名文件
 * @param channel_id 通信通道ID
 * @param params 参数 (源文件 目标文件)
 */
void task_mv(uint32_t channel_id, const char *params);

/**
 * @brief cat命令 - 显示文件内容
 * @param channel_id 通信通道ID
 * @param params 参数 (文件名 [编码])
 * @note 编码选项: utf8(不转换), gb2312(转换), auto(自动检测)
 * @note 不指定编码时，按原编码输出
 */
void task_cat(uint32_t channel_id, const char *params);

/**
 * @brief touch命令 - 创建空文件或更新时间戳
 * @param channel_id 通信通道ID
 * @param params 参数 (文件名)
 */
void task_touch(uint32_t channel_id, const char *params);

/**
 * @brief du命令 - 显示目录使用情况
 * @param channel_id 通信通道ID
 * @param params 参数 (可选目录路径)
 */
void task_du(uint32_t channel_id, const char *params);

/**
 * @brief find命令 - 查找文件
 * @param channel_id 通信通道ID
 * @param params 参数 (文件名模式)
 */
void task_find(uint32_t channel_id, const char *params);

/**
 * @brief 获取当前工作目录
 * @param channel_id 通道ID
 * @param cwd 输出缓冲区
 * @param size 缓冲区大小
 * @return true 成功, false 失败
 */
bool filesystem_get_cwd(uint32_t channel_id, char *cwd, size_t size);

/**
 * @brief 设置当前工作目录
 * @param channel_id 通道ID
 * @param path 新的工作目录路径
 * @return true 成功, false 失败
 */
bool filesystem_set_cwd(uint32_t channel_id, const char *path);

/**
 * @brief 规范化路径（处理相对路径、..、.等）
 * @param path 输入路径
 * @param resolved 输出规范化路径
 * @param size 输出缓冲区大小
 * @return true 成功, false 失败
 */
bool filesystem_resolve_path(const char *path, char *resolved, size_t size);

#ifdef __cplusplus
}
#endif

#endif // __CMD_FILESYSTEM_H__
