#include "cmd_filesystem.h"
#include "shell.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

static const char *TAG = "CMD_FS";

// 最大路径长度
#define MAX_PATH_LEN 512
// 安全路径长度(为拼接预留空间)
#define SAFE_PATH_LEN 400

bool filesystem_get_cwd(uint32_t channel_id, char *cwd, size_t size) {
    shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
    if (instance == NULL) {
        return false;
    }
    
    // 从shell实例的用户数据中获取当前工作目录
    if (instance->config.user_data != NULL) {
        strncpy(cwd, (char*)instance->config.user_data, size - 1);
        cwd[size - 1] = '\0';
        return true;
    } else {
        // 默认工作目录
        strncpy(cwd, "/sdcard", size - 1);
        cwd[size - 1] = '\0';
        return true;
    }
}

bool filesystem_set_cwd(uint32_t channel_id, const char *path) {
    shell_instance_t *instance = shell_get_instance_by_channel(channel_id);
    if (instance == NULL) {
        return false;
    }
    
    // 分配或重新分配用户数据内存
    if (instance->config.user_data == NULL) {
        instance->config.user_data = malloc(MAX_PATH_LEN);
        if (instance->config.user_data == NULL) {
            return false;
        }
    }
    
    strncpy((char*)instance->config.user_data, path, MAX_PATH_LEN - 1);
    ((char*)instance->config.user_data)[MAX_PATH_LEN - 1] = '\0';
    return true;
}

bool filesystem_resolve_path(const char *path, char *resolved, size_t size) {
    if (path == NULL || resolved == NULL || size == 0) {
        return false;
    }
    
    // 简单的路径解析实现
    if (path[0] == '/') {
        // 绝对路径
        strncpy(resolved, path, size - 1);
        resolved[size - 1] = '\0';
    } else {
        // 相对路径，需要与当前目录合并
        // 这里简化实现，实际应该处理 . 和 .. 
        if (strlen("/sdcard/") + strlen(path) + 1 < size) {
            snprintf(resolved, size, "/sdcard/%s", path);
        } else {
            return false;
        }
    }
    
    return true;
}

// 辅助函数：安全地构建路径
bool build_full_path(const char *cwd, const char *params, char *full_path, size_t path_size, char *response, size_t response_size, uint32_t channel_id) {
    if (params[0] == '/') {
        strncpy(full_path, params, path_size - 1);
        full_path[path_size - 1] = '\0';
    } else {
        size_t cwd_len = strlen(cwd);
        size_t params_len = strlen(params);
        if (cwd_len + params_len + 2 < path_size) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, params);
        } else {
            snprintf(response, response_size, "错误: 路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return false;
        }
    }
    return true;
}

void task_pwd(uint32_t channel_id, const char *params) {
    char cwd[MAX_PATH_LEN];
    char response[MAX_PATH_LEN + 50];
    
    if (filesystem_get_cwd(channel_id, cwd, sizeof(cwd))) {
        snprintf(response, sizeof(response), "%s\r\n", cwd);
    } else {
        snprintf(response, sizeof(response), "错误: 无法获取当前工作目录\r\n");
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_cd(uint32_t channel_id, const char *params) {
    char response[1024];
    char new_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        // 没有参数，回到默认目录
        strcpy(new_path, "/sdcard");
    } else {
        // 解析路径
        if (params[0] == '/') {
            // 绝对路径
            strncpy(new_path, params, sizeof(new_path) - 1);
            new_path[sizeof(new_path) - 1] = '\0';
        } else {
            // 相对路径
            char cwd[MAX_PATH_LEN];
            filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
            
            if (strcmp(params, "..") == 0) {
                // 返回上级目录
                char *last_slash = strrchr(cwd, '/');
                if (last_slash != NULL && last_slash != cwd) {
                    *last_slash = '\0';
                    strcpy(new_path, cwd);
                } else {
                    strcpy(new_path, "/sdcard");
                }
            } else if (strcmp(params, ".") == 0) {
                // 当前目录
                strcpy(new_path, cwd);
            } else {
                // 子目录 - 安全拼接路径
                size_t cwd_len = strlen(cwd);
                size_t params_len = strlen(params);
                if (cwd_len + params_len + 2 < sizeof(new_path)) {
                    strcpy(new_path, cwd);
                    strcat(new_path, "/");
                    strcat(new_path, params);
                } else {
                    snprintf(response, sizeof(response), "错误: 路径过长\r\n");
                    cmd_output(channel_id, (uint8_t *)response, strlen(response));
                    return;
                }
            }
        }
    }
    
    // 检查目录是否存在
    struct stat st;
    if (stat(new_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        if (filesystem_set_cwd(channel_id, new_path)) {
            snprintf(response, sizeof(response), "已切换到: %s\r\n", new_path);
            ESP_LOGI(TAG, "工作目录切换到: %s", new_path);
        } else {
            snprintf(response, sizeof(response), "错误: 无法设置工作目录\r\n");
            ESP_LOGE(TAG, "无法设置工作目录: %s", new_path);
        }
    } else {
        snprintf(response, sizeof(response), "错误: 目录不存在: %s\r\n", new_path);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_ls(uint32_t channel_id, const char *params) {
    char target_path[MAX_PATH_LEN];
    char response[1024];
    
    if (strlen(params) == 0) {
        // 没有参数，列出当前目录
        filesystem_get_cwd(channel_id, target_path, sizeof(target_path));
    } else {
        // 指定目录
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (!build_full_path(cwd, params, target_path, sizeof(target_path), response, sizeof(response), channel_id)) {
            return;
        }
    }
    
    DIR *dir = opendir(target_path);
    if (dir == NULL) {
        snprintf(response, sizeof(response), "错误: 无法打开目录: %s\r\n", target_path);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    snprintf(response, sizeof(response), "目录内容: %s\r\n", target_path);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    snprintf(response, sizeof(response), "----------------------------------------\r\n");
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    struct dirent *entry;
    int file_count = 0, dir_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LEN];
        if (strlen(target_path) <= SAFE_PATH_LEN && strlen(entry->d_name) <= 255) {
            strcpy(full_path, target_path);
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);
        } else {
            continue; // 跳过路径过长的项目
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                snprintf(response, sizeof(response), "d %-20s\r\n", entry->d_name);
                dir_count++;
            } else {
                snprintf(response, sizeof(response), "- %-20s %8ld bytes\r\n", 
                        entry->d_name, st.st_size);
                file_count++;
            }
        } else {
            snprintf(response, sizeof(response), "? %-20s\r\n", entry->d_name);
        }
        // 添加调试日志来检查文件名
        ESP_LOGD(TAG, "列出文件: %s (原始名称: %s)", full_path, entry->d_name);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
    }
    
    snprintf(response, sizeof(response), "----------------------------------------\r\n");
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    snprintf(response, sizeof(response), "总计: %d个文件, %d个目录\r\n", file_count, dir_count);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    closedir(dir);
}

void task_mkdir(uint32_t channel_id, const char *params) {
    char response[1024];
    char full_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: mkdir <目录名>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 构建完整路径
    if (params[0] == '/') {
        strncpy(full_path, params, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(params) <= SAFE_PATH_LEN) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, params);
        } else {
            snprintf(response, sizeof(response), "错误: 路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 创建目录
    if (mkdir(full_path, 0755) == 0) {
        snprintf(response, sizeof(response), "目录创建成功: %s\r\n", full_path);
        ESP_LOGI(TAG, "目录创建成功: %s", full_path);
    } else {
        snprintf(response, sizeof(response), "错误: 无法创建目录: %s (%s)\r\n", 
                full_path, strerror(errno));
        ESP_LOGE(TAG, "无法创建目录: %s (%s)", full_path, strerror(errno));
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

// 递归删除目录的辅助函数
static int remove_directory_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }
    
    struct dirent *entry;
    char full_path[MAX_PATH_LEN];
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        size_t path_len = strlen(path);
        size_t name_len = strlen(entry->d_name);
        if (path_len + name_len + 2 < sizeof(full_path)) {
            strcpy(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);
            
            struct stat st;
            if (stat(full_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    // 递归删除子目录
                    if (remove_directory_recursive(full_path) != 0) {
                        result = -1;
                        break;
                    }
                } else {
                    // 删除文件
                    if (remove(full_path) != 0) {
                        result = -1;
                        break;
                    }
                }
            }
        }
    }
    
    closedir(dir);
    
    if (result == 0) {
        // 删除空目录
        result = rmdir(path);
    }
    
    return result;
}

void task_rmdir(uint32_t channel_id, const char *params) {
    char response[1024];
    char full_path[MAX_PATH_LEN];
    bool recursive = false;
    const char *dir_name = params;
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: rmdir [-r] <目录名>\r\n");
        snprintf(response + strlen(response), sizeof(response) - strlen(response), "  -r  递归删除目录及其内容\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 检查是否有 -r 选项
    if (strncmp(params, "-r ", 3) == 0) {
        recursive = true;
        dir_name = params + 3;
        // 跳过多余的空格
        while (*dir_name == ' ') dir_name++;
    }
    
    if (strlen(dir_name) == 0) {
        snprintf(response, sizeof(response), "错误: 请指定要删除的目录名\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 构建完整路径
    if (dir_name[0] == '/') {
        strncpy(full_path, dir_name, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(dir_name) <= SAFE_PATH_LEN) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, dir_name);
        } else {
            snprintf(response, sizeof(response), "错误: 路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 删除目录
    int result;
    if (recursive) {
        result = remove_directory_recursive(full_path);
    } else {
        result = rmdir(full_path);
    }
    
    if (result == 0) {
        snprintf(response, sizeof(response), "目录删除成功: %s\r\n", full_path);
        ESP_LOGI(TAG, "目录删除成功: %s", full_path);
    } else {
        if (errno == ENOTEMPTY && !recursive) {
            snprintf(response, sizeof(response), "错误: 目录不为空，无法删除: %s\r\n提示: 使用 'rmdir -r %s' 递归删除\r\n", full_path, dir_name);
        } else {
            snprintf(response, sizeof(response), "错误: 无法删除目录: %s (%s)\r\n", 
                    full_path, strerror(errno));
        }
        ESP_LOGE(TAG, "无法删除目录: %s (%s)", full_path, strerror(errno));
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_rm(uint32_t channel_id, const char *params) {
    char response[1024];
    char full_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: rm <文件名>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 构建完整路径
    if (params[0] == '/') {
        strncpy(full_path, params, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(params) <= SAFE_PATH_LEN) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, params);
        } else {
            snprintf(response, sizeof(response), "错误: 路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 删除文件
    if (remove(full_path) == 0) {
        snprintf(response, sizeof(response), "文件删除成功: %s\r\n", full_path);
    } else {
        snprintf(response, sizeof(response), "错误: 无法删除文件: %s (%s)\r\n", 
                full_path, strerror(errno));
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_cp(uint32_t channel_id, const char *params) {
    char response[1024];
    char src_path[MAX_PATH_LEN], dst_path[MAX_PATH_LEN];
    char cwd[MAX_PATH_LEN];
    
    // 解析参数
    char *src = strtok((char*)params, " ");
    char *dst = strtok(NULL, " ");
    
    if (src == NULL || dst == NULL) {
        snprintf(response, sizeof(response), "用法: cp <源文件> <目标文件>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
    
    // 构建源文件路径
    if (src[0] == '/') {
        strcpy(src_path, src);
    } else {
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(src) <= SAFE_PATH_LEN) {
            strcpy(src_path, cwd);
            strcat(src_path, "/");
            strcat(src_path, src);
        } else {
            snprintf(response, sizeof(response), "错误: 源文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 构建目标文件路径
    if (dst[0] == '/') {
        strcpy(dst_path, dst);
    } else {
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(dst) <= SAFE_PATH_LEN) {
            strcpy(dst_path, cwd);
            strcat(dst_path, "/");
            strcat(dst_path, dst);
        } else {
            snprintf(response, sizeof(response), "错误: 目标文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 复制文件
    FILE *src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        snprintf(response, sizeof(response), "错误: 无法打开源文件: %s\r\n", src_path);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    FILE *dst_file = fopen(dst_path, "wb");
    if (dst_file == NULL) {
        snprintf(response, sizeof(response), "错误: 无法创建目标文件: %s\r\n", dst_path);
        fclose(src_file);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 复制数据
    char buffer[1024];
    size_t bytes_read, bytes_written;
    size_t total_bytes = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_read, dst_file);
        if (bytes_written != bytes_read) {
            snprintf(response, sizeof(response), "错误: 写入失败\r\n");
            fclose(src_file);
            fclose(dst_file);
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
        total_bytes += bytes_written;
    }
    
    fclose(src_file);
    fclose(dst_file);
    
    // 安全格式化 - 避免长路径导致的截断警告
    snprintf(response, sizeof(response), "文件复制成功 (%zu bytes)\r\n", total_bytes);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_mv(uint32_t channel_id, const char *params) {
    char response[1024];
    char src_path[MAX_PATH_LEN], dst_path[MAX_PATH_LEN];
    char cwd[MAX_PATH_LEN];
    
    // 解析参数
    char *src = strtok((char*)params, " ");
    char *dst = strtok(NULL, " ");
    
    if (src == NULL || dst == NULL) {
        snprintf(response, sizeof(response), "用法: mv <源文件> <目标文件>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
    
    // 构建路径
    if (src[0] == '/') {
        strcpy(src_path, src);
    } else {
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(src) <= SAFE_PATH_LEN) {
            strcpy(src_path, cwd);
            strcat(src_path, "/");
            strcat(src_path, src);
        } else {
            snprintf(response, sizeof(response), "错误: 源文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    if (dst[0] == '/') {
        strcpy(dst_path, dst);
    } else {
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(dst) <= SAFE_PATH_LEN) {
            strcpy(dst_path, cwd);
            strcat(dst_path, "/");
            strcat(dst_path, dst);
        } else {
            snprintf(response, sizeof(response), "错误: 目标文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 移动/重命名文件
    if (rename(src_path, dst_path) == 0) {
        snprintf(response, sizeof(response), "文件移动成功\r\n");
    } else {
        snprintf(response, sizeof(response), "错误: 无法移动文件 (%s)\r\n", strerror(errno));
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_cat(uint32_t channel_id, const char *params) {
    char response[1024];
    char full_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: cat <文件名>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 构建完整路径
    if (params[0] == '/') {
        strncpy(full_path, params, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(params) <= SAFE_PATH_LEN) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, params);
        } else {
            snprintf(response, sizeof(response), "错误: 文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        snprintf(response, sizeof(response), "错误: 无法打开文件: %s\r\n", full_path);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 读取并显示文件内容
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        cmd_output(channel_id, (uint8_t *)line, strlen(line));
    }
    
    fclose(file);
}

void task_touch(uint32_t channel_id, const char *params) {
    char response[1024];
    char full_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: touch <文件名>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 构建完整路径
    if (params[0] == '/') {
        strncpy(full_path, params, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';
    } else {
        char cwd[MAX_PATH_LEN];
        filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
        if (strlen(cwd) <= SAFE_PATH_LEN && strlen(params) <= SAFE_PATH_LEN) {
            strcpy(full_path, cwd);
            strcat(full_path, "/");
            strcat(full_path, params);
        } else {
            snprintf(response, sizeof(response), "错误: 文件路径过长\r\n");
            cmd_output(channel_id, (uint8_t *)response, strlen(response));
            return;
        }
    }
    
    // 创建空文件
    FILE *file = fopen(full_path, "a");
    if (file == NULL) {
        snprintf(response, sizeof(response), "错误: 无法创建文件: %s\r\n", full_path);
    } else {
        fclose(file);
        snprintf(response, sizeof(response), "文件创建/更新成功: %s\r\n", full_path);
    }
    
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_du(uint32_t channel_id, const char *params) {
    char response[1024];
    char target_path[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        filesystem_get_cwd(channel_id, target_path, sizeof(target_path));
    } else {
        if (params[0] == '/') {
            strncpy(target_path, params, sizeof(target_path) - 1);
            target_path[sizeof(target_path) - 1] = '\0';
        } else {
            char cwd[MAX_PATH_LEN];
            filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
            if (strlen(cwd) <= SAFE_PATH_LEN && strlen(params) <= SAFE_PATH_LEN) {
                strcpy(target_path, cwd);
                strcat(target_path, "/");
                strcat(target_path, params);
            } else {
                snprintf(response, sizeof(response), "错误: 路径过长\r\n");
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                return;
            }
        }
    }
    
    // 简单实现：只统计当前目录文件大小
    DIR *dir = opendir(target_path);
    if (dir == NULL) {
        snprintf(response, sizeof(response), "错误: 无法打开目录: %s\r\n", target_path);
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    struct dirent *entry;
    long total_size = 0;
    int file_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LEN];
        if (strlen(target_path) <= SAFE_PATH_LEN && strlen(entry->d_name) <= 255) {
            strcpy(full_path, target_path);
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);
        } else {
            continue; // 跳过路径过长的项目
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            total_size += st.st_size;
            file_count++;
        }
    }
    
    closedir(dir);
    
    snprintf(response, sizeof(response), "目录: %s\r\n", target_path);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    snprintf(response, sizeof(response), "文件数: %d\r\n", file_count);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    snprintf(response, sizeof(response), "总大小: %ld bytes (%.2f KB)\r\n", 
            total_size, (float)total_size / 1024.0);
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}

void task_find(uint32_t channel_id, const char *params) {
    char response[1024];
    char cwd[MAX_PATH_LEN];
    
    if (strlen(params) == 0) {
        snprintf(response, sizeof(response), "用法: find <文件名模式>\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    filesystem_get_cwd(channel_id, cwd, sizeof(cwd));
    
    DIR *dir = opendir(cwd);
    if (dir == NULL) {
        snprintf(response, sizeof(response), "错误: 无法打开当前目录\r\n");
        cmd_output(channel_id, (uint8_t *)response, strlen(response));
        return;
    }
    
    // 安全格式化查找消息
    if (strlen(cwd) + strlen(params) + 30 < sizeof(response)) {
        snprintf(response, sizeof(response), "在 %s 中查找: %s\r\n", cwd, params);
    } else {
        snprintf(response, sizeof(response), "正在查找...\r\n");
    }
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
    
    struct dirent *entry;
    int found_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // 简单的字符串匹配
        if (strstr(entry->d_name, params) != NULL) {
            // 检查输出字符串长度避免截断
            if (strlen(cwd) + strlen(entry->d_name) + 20 < sizeof(response)) {
                snprintf(response, sizeof(response), "找到: %s/%s\r\n", cwd, entry->d_name);
                cmd_output(channel_id, (uint8_t *)response, strlen(response));
                found_count++;
            }
        }
    }
    
    closedir(dir);
    
    if (found_count == 0) {
        snprintf(response, sizeof(response), "未找到匹配的文件\r\n");
    } else {
        snprintf(response, sizeof(response), "找到 %d 个匹配的文件\r\n", found_count);
    }
    cmd_output(channel_id, (uint8_t *)response, strlen(response));
}
