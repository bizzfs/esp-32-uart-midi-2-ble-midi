#include "driver/uart.h"

static uart_port_t uart_int_num;

void uart_start(uart_port_t uart_num, int rx_pin_num, QueueHandle_t *queue) {
    uart_int_num = uart_num;
    if (!uart_num) uart_num = UART_NUM_0;
    if (!rx_pin_num) rx_pin_num = UART_PIN_NO_CHANGE;

    uart_config_t uart_config = {
            .baud_rate = 31250,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_RTS,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, UART_PIN_NO_CHANGE, rx_pin_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_int_num, 4096, 8192, 10, queue, 0);
}
