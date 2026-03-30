#ifndef LOG_H
#define LOG_H
#include <stddef.h>

/**
 * 日志级别枚举
 */
typedef enum {
    LOG_DEBUG,  // 调试级别
    LOG_INFO,   // 信息级别
    LOG_ERROR   // 错误级别
} log_level_t;

/**
 * 日志输出函数类型
 * @param msg 日志消息
 */
typedef void (*log_output_func)(const char *msg);

/**
 * 日志配置结构体
 */
typedef struct{
    int queue_size;           // 日志队列大小
    int batch_size;           // 批处理大小
    int flush_interval_ms;    // 刷新间隔（毫秒）
    int log_level;            // 日志级别
    const char *log_file;     // 日志文件路径
    int enable_console;       // 是否启用控制台输出
}log_config_t;

/**
 * 日志统计信息结构体
 */
typedef struct{
    int total_logs;    // 总日志数
    int dropped_logs;  // 丢弃的日志数
    int queue_len;     // 当前队列长度
}log_stats_t;

/**
 * 初始化日志系统
 * @param cfg 日志配置
 * @return 0 成功，-1 失败
 */
int log_init(const log_config_t *cfg);

/**
 * 写入日志
 * @param level 日志级别
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void log_write(log_level_t level,const char *fmt,...);

/**
 * 设置日志级别
 * @param level 日志级别
 */
void log_set_level(log_level_t level);

/**
 * 获取日志统计信息
 * @return 日志统计信息
 */
log_stats_t log_get_stats(void);

/**
 * 关闭日志系统
 */
void log_close(void);

#endif