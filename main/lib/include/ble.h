#include <stdint.h>

struct ble_midi_args_t {
    char *device_name;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
    uint16_t preferred_mtu;
    void (*conn_interval_change_callback)(uint16_t value);
    void (*mtu_change_callback)(uint16_t value);
};

void ble_midi_start(struct ble_midi_args_t *args);

void ble_notify(uint8_t *byte_buff, uint16_t length);