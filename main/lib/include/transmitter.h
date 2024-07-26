#include <stdint.h>

#include "driver/uart.h"

struct transmitter_args_t {
    char *device_name;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
    uint16_t preferred_mtu;
    uart_port_t uart_num;
    int rx_pin_num;
};

void transmitter_start(struct transmitter_args_t *args);