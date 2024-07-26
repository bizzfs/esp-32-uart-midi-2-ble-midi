#include <stdbool.h>
#include <stdint.h>

struct midi_msg_t {
    uint8_t data[3];
    uint8_t len;
    uint16_t timestamp;
};

struct midi_parser_t {
    struct midi_msg_t *msg;
    uint8_t running_status;
    bool third_byte_flag;
};

struct midi_msg_t *midi_parse_byte(uint16_t timestamp, uint8_t byte, struct midi_parser_t *parser);
