#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "ble.h"
#include "parser.h"
#include "uart.h"

#include "processor.h"
#include "transmitter.h"

static const char *TAG = "TRANSMITTER";

QueueHandle_t uart_queue;
QueueHandle_t conn_tick_queue;
QueueHandle_t mtu_change_queue;
QueueSetHandle_t queue_set;

TaskHandle_t uart_task;
esp_timer_handle_t ms_timer;
esp_timer_handle_t conn_interval_timer;

uart_port_t uart_num;
int32_t timestamp;

void connect_callback(void);

void disconnect_callback(void);

void conn_interval_change_callback(uint16_t value);

void mtu_change_callback(uint16_t value);

static void ms_timer_callback(void *args);

static void conn_interval_timer_callback(void *args);

void transmitter_task(void *args);

void transmitter_start(struct transmitter_args_t *args) {
    uart_num = args->uart_num;

    struct ble_midi_args_t ble_midi_start_args = {
            .device_name = args->device_name,
            .conn_interval_min = args->conn_interval_min,
            .conn_interval_max = args->conn_interval_max,
            .preferred_mtu = args->preferred_mtu,
            .conn_interval_change_callback = &conn_interval_change_callback,
            .mtu_change_callback = &mtu_change_callback
    };
    ble_midi_start(&ble_midi_start_args);

    uart_start(uart_num, args->rx_pin_num, &uart_queue);

    conn_tick_queue = xQueueCreate(1, sizeof(uint8_t));
    mtu_change_queue = xQueueCreate(1, sizeof(uint16_t));
    queue_set = xQueueCreateSet(11);
    xQueueAddToSet(conn_tick_queue, queue_set);
    xQueueAddToSet(uart_queue, queue_set);
    xQueueAddToSet(mtu_change_queue, queue_set);

    const esp_timer_create_args_t ms_timer_args = {
            .callback = &ms_timer_callback,
    };
    ESP_ERROR_CHECK(esp_timer_create(&ms_timer_args, &ms_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(ms_timer, 1000));

    const esp_timer_create_args_t conn_interval_timer_args = {
            .callback = &conn_interval_timer_callback,
    };
    ESP_ERROR_CHECK(esp_timer_create(&conn_interval_timer_args, &conn_interval_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(conn_interval_timer, 15000));

    xTaskCreatePinnedToCore(transmitter_task, "transmitterTask", 4096, (void *) args->uart_num, 1, &uart_task, 1);
}

void conn_interval_change_callback(uint16_t value) {
    uint16_t interval_microsec = value * 1250;
    ESP_LOGE(TAG, "connection interval updated = %dms", interval_microsec / 1000);

    ESP_ERROR_CHECK(esp_timer_stop(conn_interval_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(conn_interval_timer, interval_microsec));
}

void mtu_change_callback(uint16_t value) {
    ESP_LOGE(TAG, "mtu updated = %d", value);
    xQueueGenericSend(mtu_change_queue, &value, 0, queueSEND_TO_BACK);
}

static void ms_timer_callback(void *args) {
    timestamp = esp_timer_get_time() / 1000;
}

static void conn_interval_timer_callback(void *args) {
    static const uint8_t tick = 1;
    xQueueGenericSend(conn_tick_queue, &tick, 0, queueSEND_TO_BACK);
}

void transmitter_task(void *args) {
    struct processor_t processor;
    init_processor(&processor, 514); // max buffer size default_mtu -3 (517 - 3)

    uart_event_t event;
    uint8_t tick;
    uint16_t mtu;
    QueueSetMemberHandle_t queue_member;

    for (;;) {
        queue_member = xQueueSelectFromSet(queue_set, portMAX_DELAY);
        if (queue_member == conn_tick_queue) {
            xQueueReceive(conn_tick_queue, &tick, 0);
            flush_notify(&processor);
        } else if (queue_member == uart_queue) {
            xQueueReceive(uart_queue, &event, 0);

            if (event.type != UART_DATA || event.size == 0) continue;

            uint8_t *ntf;
            ntf = (uint8_t *) malloc(sizeof(uint8_t) * event.size);
            memset(ntf, 0x00, event.size);
            uart_read_bytes(uart_num, ntf, event.size, portMAX_DELAY);
            for (uint16_t i = 0; i < event.size; i++) {
                processor.process(ntf[i], timestamp, &processor);
            }
            free(ntf);
        } else if (queue_member == mtu_change_queue) {
            xQueueReceive(mtu_change_queue, &mtu, 0);
            init_processor(&processor, mtu - 3);
        }
    }
    vTaskDelete(NULL);
}