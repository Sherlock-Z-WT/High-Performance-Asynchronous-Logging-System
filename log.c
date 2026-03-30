#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "log.h"

#define MAX_OUTPUTS 5    // 最大输出数量
#define QUEUE_SIZE 1024  // 日志队列大小
#define LOG_MSG_LEN 256  // 单条日志最大长度

/**
 * 日志队列结构体
 */
typedef struct{
    char messages[QUEUE_SIZE][LOG_MSG_LEN];  // 日志消息数组
    int head;  // 队列头指针
    int tail;  // 队列尾指针
}log_queue_t;

// 全局变量
static int total_logs = 0;          // 总日志数
static int dropped_logs = 0;        // 丢弃的日志数
static log_queue_t log_queue;       // 日志队列
static pthread_t writer_thread;      // 写日志线程
static int running = 1;              // 运行标志
static pthread_mutex_t queue_mutex;  // 队列互斥锁
static pthread_cond_t queue_cond;    // 队列条件变量
static FILE *log_fp = NULL;          // 日志文件指针
static log_output_func outputs[MAX_OUTPUTS];  // 输出函数数组
static int output_count = 0;         // 输出函数数量
static log_level_t current_level = LOG_INFO;  // 当前日志级别

/**
 * 文件输出函数
 * @param msg 日志消息
 */
static void file_output(const char *msg){
    if(log_fp){
        fwrite(msg, 1, strlen(msg), log_fp);
        fflush(log_fp);  // 确保数据写入磁盘
    }else{
        fprintf(stderr,"%s", msg);  // 失败时输出到标准错误
    }
}

/**
 * 控制台输出函数
 * @param msg 日志消息
 */
static void console_output(const char *msg){
    printf("%s", msg);  // 输出到标准输出
}

/**
 * 注册输出函数
 * @param func 输出函数
 */
static void register_output(log_output_func func){
    if(output_count < MAX_OUTPUTS){
        outputs[output_count++] = func;
    }
}

/**
 * 将日志加入队列
 * @param level 日志级别
 * @param msg 日志消息
 */
static void enqueue_log(log_level_t level, const char *msg){
    pthread_mutex_lock(&queue_mutex);  // 加锁

    total_logs++;  // 增加总日志数

    int next = (log_queue.tail + 1) % QUEUE_SIZE;  // 计算下一个位置

    // 队列满了
    if(next == log_queue.head){
        if(level == LOG_DEBUG){
            // 只丢弃调试级别日志
            dropped_logs++;
            pthread_mutex_unlock(&queue_mutex);  // 解锁
            return;
        }
    }

    // 复制消息到队列
    strncpy(log_queue.messages[log_queue.tail], msg, LOG_MSG_LEN - 1);
    log_queue.messages[log_queue.tail][LOG_MSG_LEN - 1] = '\0';  // 确保字符串结束
    log_queue.tail = next;  // 更新尾指针

    pthread_cond_signal(&queue_cond);  // 发送信号通知写线程
    pthread_mutex_unlock(&queue_mutex);  // 解锁
}

/**
 * 从队列中取出日志
 * @param out 输出缓冲区
 * @return 1 成功，0 失败
 */
static int dequeue_log(char *out){
    pthread_mutex_lock(&queue_mutex);  // 加锁
    if(log_queue.head == log_queue.tail){
        // 队列为空
        pthread_mutex_unlock(&queue_mutex);  // 解锁
        return 0;
    }
    // 复制消息到输出缓冲区
    strncpy(out, log_queue.messages[log_queue.head], LOG_MSG_LEN);
    log_queue.head = (log_queue.head + 1) % QUEUE_SIZE;  // 更新头指针
    pthread_mutex_unlock(&queue_mutex);  // 解锁
    return 1;
}

/**
 * 日志写入线程函数
 * @param arg 线程参数
 * @return 线程返回值
 */
static void* log_writer(void *arg){
    char msg[LOG_MSG_LEN];

    while(running){
        pthread_mutex_lock(&queue_mutex);  // 加锁

        // 等待队列非空
        while(log_queue.head == log_queue.tail && running){
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }

        // 检查是否需要退出
        if(!running){
            pthread_mutex_unlock(&queue_mutex);  // 解锁
            break;
        }

        int count = 0;
        // 批量处理日志
        while(log_queue.head != log_queue.tail && count < 100){
            strncpy(msg, log_queue.messages[log_queue.head], LOG_MSG_LEN);
            log_queue.head = (log_queue.head + 1) % QUEUE_SIZE;  // 更新头指针
            
            // 输出到所有注册的输出函数
            for(int i = 0; i < output_count; i++){
                outputs[i](msg);
            }
            count++;
        }
        
        pthread_mutex_unlock(&queue_mutex);  // 解锁
    }
    return NULL;
}

/**
 * 初始化日志系统
 * @param cfg 日志配置
 * @return 0 成功，-1 失败
 */
int log_init(const log_config_t *cfg){
    if(!cfg){
        fprintf(stderr, "log init failed: null config\n");
        return -1;
    }

    // 初始化互斥锁和条件变量
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    
    // 打开日志文件
    log_fp = fopen(cfg->log_file, "a");
    if(!log_fp){
        fprintf(stderr, "log init failed: cannot open log file\n");
        // 清理资源
        pthread_mutex_destroy(&queue_mutex);
        pthread_cond_destroy(&queue_cond);
        return -1;
    }

    // 初始化队列
    log_queue.head = log_queue.tail = 0;
    // 设置日志级别
    current_level = (log_level_t)cfg->log_level;
    
    // 创建写日志线程
    pthread_create(&writer_thread, NULL, log_writer, NULL);
    
    // 注册输出函数
    register_output(file_output);
    if(cfg->enable_console){
        register_output(console_output);
    }
    
    return 0;
}

/**
 * 写入日志
 * @param level 日志级别
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void log_write(log_level_t level, const char *fmt, ...){
    // 检查日志级别
    if(level < current_level){
        return;
    }

    char buffer[LOG_MSG_LEN];
    const char* level_str[] = {
        "DEBUG",
        "INFO",
        "ERROR"
    };

    // 获取当前时间
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    va_list args;
    va_start(args, fmt);

    // 格式化时间和级别
    int len = snprintf(buffer, LOG_MSG_LEN, 
        "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec,
        level_str[level]
    );

    // 格式化消息
    if(len < LOG_MSG_LEN - 1){
        vsnprintf(buffer + len, LOG_MSG_LEN - len, fmt, args);
    }

    va_end(args);
    
    // 添加换行符
    strncat(buffer, "\n", LOG_MSG_LEN - strlen(buffer) - 1);
    // 加入队列
    enqueue_log(level, buffer);
}

/**
 * 设置日志级别
 * @param level 日志级别
 */
void log_set_level(log_level_t level){
    current_level = level;
}

/**
 * 获取日志统计信息
 * @return 日志统计信息
 */
log_stats_t log_get_stats(void){
    log_stats_t stats;
    
    pthread_mutex_lock(&queue_mutex);  // 加锁
    
    stats.total_logs = total_logs;
    stats.dropped_logs = dropped_logs;
    stats.queue_len = (log_queue.tail - log_queue.head + QUEUE_SIZE) % QUEUE_SIZE;
    
    pthread_mutex_unlock(&queue_mutex);  // 解锁
    
    return stats;
}

/**
 * 关闭日志系统
 */
void log_close(void){
    running = 0;  // 设置退出标志
    pthread_cond_signal(&queue_cond);  // 唤醒写线程
    pthread_join(writer_thread, NULL);  // 等待写线程退出
    
    // 关闭日志文件
    if(log_fp){
        fclose(log_fp);
        log_fp = NULL;
    }
    
    // 清理资源
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
}
