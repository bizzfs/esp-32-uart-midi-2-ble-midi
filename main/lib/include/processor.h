#include <stdint.h>

typedef enum {
    STATUS_NOTE_OFF_PREF_4 = 0x8, // 2 data bytes
    STATUS_NOTE_ON_PREF_4, // 2 data bytes
    STATUS_PKP_AFTERTOUCH_PREF_4, // 2 data bytes
    STATUS_CC_PREF_4, // 2 data bytes
    STATUS_PROGRAM_CHANGE_PREF_4, // 1 data byte
    STATUS_CP_AFTERTOUCH_PREF_4, // 1 data byte
    STATUS_PITCH_BEND_PREF_4, // 2 data bytes
    STATUS_SYS_PREF_4,
} byte_prefix_4;

typedef enum {
    STATUS_SYS_EX_SUF_3, // 1+ data bytes //
    STATUS_SYS_MIDI_MTC_SUF_3, // 1 data byte
    STATUS_SYS_SONG_POSITION_SUF_3, // 2 data bytes
    STATUS_SYS_SONG_SELECT_SUF_3, // 1 data byte
    STATUS_SYS_UNDEFINED_1_SUF_3,
    STATUS_SYS_UNDEFINED_2_SUF_3,
    STATUS_SYS_TUNE_REQ_SUF_3, // 0 data bytes
    STATUS_SYS_EOF_SUF_3, // 0 data bytes
} byte_suffix_3;

struct processor_t;

struct processor_t {
    uint8_t *buff;
    uint16_t buff_len;
    uint16_t buff_max;
    uint16_t timestamp;
    uint8_t first_data_byte;
    uint8_t status;
    void (*process)(uint8_t byte, uint16_t timestamp, struct processor_t *processor);
};

void init_processor(struct processor_t *processor, uint16_t buff_max);

void flush_notify(struct processor_t *processor);