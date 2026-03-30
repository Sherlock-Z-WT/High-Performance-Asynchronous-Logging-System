#include <stdio.h>
#include "../include/log.h"
#include <pthread.h>

void* worker(void* arg){
    for(int i=0;i<1000;i++){
        log_write(LOG_INFO,"Thread %ld log %d",(long)arg,i);
    }
    return NULL;
}

int main() {
    log_config_t cfg = {
        .queue_size = 1024,
        .batch_size = 4096,
        .flush_interval_ms = 1000,
        .log_level = LOG_INFO,
        .log_file = "test.log",
        .enable_console = 1
    };
    
    if(log_init(&cfg) != 0){
        fprintf(stderr, "Failed to initialize log system\n");
        return 1;
    }
    
    pthread_t t1,t2;

    pthread_create(&t1,NULL,worker,(void*)1);
    pthread_create(&t2,NULL,worker,(void*)2);

    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

    log_stats_t stats = log_get_stats();
    printf("Total logs: %d\n", stats.total_logs);
    printf("Dropped logs: %d\n", stats.dropped_logs);
    printf("Queue length: %d\n", stats.queue_len);

    log_close();
    return 0;
}