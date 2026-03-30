int main(){
    log_config_t cfg = {
        .log_file = "demo.log",
        .enable_console = 1,
    };
    log_init(&cfg);

    log_write(LOG_INFO,"system log");
    log_write(LOG_ERROR,"error code = %d",100);

    log_close();
}