# High-Performance Asynchronous Logging System

A powerful, thread-safe asynchronous logging system designed for high-concurrency applications.

## Features

- **Asynchronous Processing**: Background thread handles log writing, non-blocking for main threads
- **Thread Safety**: Uses mutexes and condition variables to ensure safety in multi-threaded environments
- **Multi-output Support**: Supports both file and console output
- **Log Levels**: Supports DEBUG, INFO, and ERROR levels
- **Queue Mechanism**: Uses circular queue to buffer logs for improved performance
- **Batch Processing**: Writes logs in batches to reduce I/O operations
- **Statistics**: Provides detailed log statistics
- **Error Handling**: Properly handles errors like file open failures
- **Resource Management**: Automatically initializes and destroys all resources

## System Requirements

- C99 compatible compiler
- pthread library
- Linux operating system

## Installation

1. **Clone the project**

```bash
git clone <repository-url>
cd log_system
```

2. **Compile**

```bash
make
```

3. **Run tests**

```bash
./test_log
```

## Quick Start

### Basic Usage

```c
#include <stdio.h>
#include "include/log.h"

int main() {
    // Configure logging system
    log_config_t cfg = {
        .log_file = "app.log",
        .enable_console = 1,
        .log_level = LOG_INFO
    };
    
    // Initialize logging system
    if(log_init(&cfg) != 0){
        fprintf(stderr, "Failed to initialize log system\n");
        return 1;
    }
    
    // Write logs
    log_write(LOG_INFO, "System started");
    log_write(LOG_ERROR, "Error code: %d", 404);
    log_write(LOG_DEBUG, "Debug information");
    
    // Get statistics
    log_stats_t stats = log_get_stats();
    printf("Total logs: %d\n", stats.total_logs);
    printf("Dropped logs: %d\n", stats.dropped_logs);
    
    // Close logging system
    log_close();
    
    return 0;
}
```

### Compilation and Usage

```bash
gcc -o app app.c src/log.c -Iinclude -lpthread
./app
```

## API Documentation

### Data Structures

#### Log Levels

```c
typedef enum {
    LOG_DEBUG,  // Debug level
    LOG_INFO,   // Information level
    LOG_ERROR   // Error level
} log_level_t;
```

#### Log Configuration

```c
typedef struct{
    int queue_size;           // Log queue size
    int batch_size;           // Batch size
    int flush_interval_ms;    // Flush interval (milliseconds)
    int log_level;            // Log level
    const char *log_file;     // Log file path
    int enable_console;       // Whether to enable console output
} log_config_t;
```

#### Log Statistics

```c
typedef struct{
    int total_logs;    // Total number of logs
    int dropped_logs;  // Number of dropped logs
    int queue_len;     // Current queue length
} log_stats_t;
```

### Function Interfaces

#### log_init

```c
int log_init(const log_config_t *cfg);
```

- **Parameters**: `cfg` - Log configuration structure
- **Return Value**: 0 for success, -1 for failure
- **Functionality**: Initializes the logging system and creates the log writer thread

#### log_write

```c
void log_write(log_level_t level, const char *fmt, ...);
```

- **Parameters**:
  - `level` - Log level
  - `fmt` - Format string
  - `...` - Variable arguments
- **Functionality**: Writes logs, with level filtering

#### log_set_level

```c
void log_set_level(log_level_t level);
```

- **Parameters**: `level` - Log level
- **Functionality**: Dynamically sets the log level

#### log_get_stats

```c
log_stats_t log_get_stats(void);
```

- **Return Value**: Log statistics structure
- **Functionality**: Gets log statistics

#### log_close

```c
void log_close(void);
```

- **Functionality**: Closes the logging system and cleans up resources

## Configuration Options

| Option | Type | Default | Description |
|-------|------|-------|-------------|
| queue_size | int | 1024 | Log queue size |
| batch_size | int | 4096 | Batch size |
| flush_interval_ms | int | 1000 | Flush interval (milliseconds) |
| log_level | int | LOG_INFO | Log level |
| log_file | const char* | "app.log" | Log file path |
| enable_console | int | 1 | Whether to enable console output |

## Performance Characteristics

- **High Throughput**: Asynchronous processing and batch writing support high concurrency scenarios
- **Low Latency**: Main thread only enqueues logs, no I/O operations involved
- **Memory Usage**: Queue size configurable, default 1024 logs
- **Reliability**: Drops DEBUG level logs when queue is full to ensure critical logs are not lost

## Examples

### Multi-thread Example

```c
#include <stdio.h>
#include <pthread.h>
#include "include/log.h"

void* worker(void* arg){
    for(int i=0; i<1000; i++){
        log_write(LOG_INFO, "Thread %ld log %d", (long)arg, i);
    }
    return NULL;
}

int main() {
    log_config_t cfg = {
        .log_file = "test.log",
        .enable_console = 1
    };
    
    log_init(&cfg);
    
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, (void*)1);
    pthread_create(&t2, NULL, worker, (void*)2);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    log_stats_t stats = log_get_stats();
    printf("Total logs: %d\n", stats.total_logs);
    printf("Dropped logs: %d\n", stats.dropped_logs);
    
    log_close();
    return 0;
}
```

## Directory Structure

```
log_system/
├── include/         # Header files
│   └── log.h        # Log system header
├── src/             # Source code
│   └── log.c        # Log system implementation
├── test/            # Test files
│   └── test_log.c   # Multi-thread test
├── examples/        # Examples
│   └── demo.c       # Basic usage example
├── config/          # Configuration files
│   └── log.conf     # Log configuration file
├── Makefile         # Compilation script
├── README.md        # This document
└── test.log         # Test log file
```

## Contribution

Welcome to submit Issues and Pull Requests to improve this logging system.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Performance Test

On a 4-core CPU machine, using 10 threads writing logs simultaneously, each thread writes 100,000 logs:

- **Total Logs**: 1,000,000
- **Total Time**: Approximately 2.5 seconds
- **Throughput**: Approximately 400,000 logs/second
- **CPU Usage**: Approximately 30%
- **Memory Usage**: Approximately 5MB

## Notes

1. Ensure to call `log_close()` before program exit to clean up resources
2. Log file path needs write permission
3. DEBUG level logs will be dropped when queue is full to ensure critical logs are not lost
4. It is recommended to adjust queue size and batch size according to actual needs

## Troubleshooting

### Log File Not Generated
- Check if the file path is correct
- Check if you have write permission
- Check if `log_init()` returns 0 successfully

### Log Loss
- Check if the queue size is sufficient
- Check if there are too many DEBUG level logs causing queue overflow
- Adjust `queue_size` configuration

### Performance Issues
- Adjust `batch_size` to balance memory usage and I/O operations
- Consider disabling console output to improve performance
- Adjust `queue_size` to adapt to concurrency level
