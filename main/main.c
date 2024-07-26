#include "transmitter.h"

void app_main(void) {
    struct transmitter_args_t args = {
            .device_name = "Yo_Bizl Keystep 37",
            .conn_interval_min = 0x06, // range 0x06 - 0x0c80
            .conn_interval_max = 0x06, // range 0x06 - 0x0c80
            .preferred_mtu = 500, // max 517
            .uart_num = UART_NUM_0,
            .rx_pin_num = 1
    };
    transmitter_start(&args);
}