#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "ble.h"
#include "processor.h"

#define TIMESTAMP_HIGH(ts) 0x80 | ((ts >> 7) & 0x3f)

#define TIMESTAMP_LOW(ts) 0x80 | (ts & 0x7f)

#define NOTIFY(processor) do { \
    ble_notify(processor->buff, processor->buff_len); \
    processor->buff_len = 0; \
} while(0)

#define FLUSH_NOTIFY_IF_EXCEED(size, processor) \
    if (processor->buff_len + size > processor->buff_max) NOTIFY(processor)

#define SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor) \
    if (processor->buff_len == 0) do { \
        processor->buff[0] = TIMESTAMP_HIGH(timestamp); \
        processor->buff_len = 1; \
    } while (0)

static const char *TAG = "PROCESSOR";

void print_byte(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_status(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_1_of_1(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_1_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_sys_1_of_1(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_sys_1_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_sys_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_status_or_running_status(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_running_status_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_sysex_1_of_n(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void process_sysex_i_of_n(uint8_t byte, uint16_t timestamp, struct processor_t *processor);

void init_processor(struct processor_t *processor, uint16_t buff_max) {
    memset(processor, 0, sizeof(struct processor_t));
    free(processor->buff);
    processor->buff = malloc(sizeof(uint8_t) * buff_max);
    processor->buff_max = buff_max;
    processor->buff_len = 0;
    processor->process = process_status;
}

void flush_notify(struct processor_t *processor) {
    ESP_LOG_BUFFER_HEX(TAG, processor->buff, processor->buff_len);
    if (processor->buff_len > 0) {
        NOTIFY(processor);
    }
}

void template(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            return;
        case STATUS_NOTE_ON_PREF_4:
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            return;
        case STATUS_CC_PREF_4:
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            return;
        case STATUS_PITCH_BEND_PREF_4:
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            // sysex
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default: // unlikely
                            return;
                    }
                case 1:
                    ESP_LOGI(TAG, "rt message");
                    return;
                default: // unlikely
                    return;
            }
        default:
            ESP_LOGI(TAG, "data");
            return;
    }
}

void print_byte(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    ESP_LOGI(TAG, "wait_for_status");
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            ESP_LOGI(TAG, "STATUS_NOTE_OFF_PREF_4");
            return;
        case STATUS_NOTE_ON_PREF_4:
            ESP_LOGI(TAG, "STATUS_NOTE_ON_PREF_4");
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            ESP_LOGI(TAG, "STATUS_PKP_AFTERTOUCH_PREF_4");
            return;
        case STATUS_CC_PREF_4:
            ESP_LOGI(TAG, "STATUS_CC_PREF_4");
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            ESP_LOGI(TAG, "STATUS_PROGRAM_CHANGE_PREF_4");
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            ESP_LOGI(TAG, "STATUS_CP_AFTERTOUCH_PREF_4");
            return;
        case STATUS_PITCH_BEND_PREF_4:
            ESP_LOGI(TAG, "STATUS_PITCH_BEND_PREF_4");
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_EX_SUF_3");
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_MIDI_MTC_SUF_3");
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_SONG_POSITION_SUF_3");
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_SONG_SELECT_SUF_3");
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_UNDEFINED_1_SUF_3");
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_UNDEFINED_2_SUF_3");
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_TUNE_REQ_SUF_3");
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            ESP_LOGI(TAG, "STATUS_SYS_EOF_SUF_3");
                            return;
                        default:
                            return;
                    }
                case 1:
                    return;
                default:
                    return;
            }
        default:
            ESP_LOGI(TAG, "data");
            return;
    }
}

void process_status(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            return;
    }
}

void process_1_of_1(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(3, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
            processor->buff[processor->buff_len + 1] = processor->status;
            processor->buff[processor->buff_len + 2] = byte;
            processor->buff_len += 3;
            processor->process = process_status_or_running_status;
            return;
    }
}

void process_1_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            processor->first_data_byte = byte;
            processor->process = process_2_of_2;
            return;
    }
}

void process_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(4, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
            processor->buff[processor->buff_len + 1] = processor->status;
            processor->buff[processor->buff_len + 2] = processor->first_data_byte;
            processor->buff[processor->buff_len + 3] = byte;
            processor->buff_len += 4;
            processor->process = process_status_or_running_status;
            return;
    }
}

void process_sys_1_of_1(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(3, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
            processor->buff[processor->buff_len + 1] = processor->status;
            processor->buff[processor->buff_len + 2] = byte;
            processor->buff_len += 3;
            processor->process = process_status;
            return;
    }
}

void process_sys_1_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            processor->first_data_byte = byte;
            processor->process = process_sys_2_of_2;
            return;
    }
}

void process_sys_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(4, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
            processor->buff[processor->buff_len + 1] = processor->status;
            processor->buff[processor->buff_len + 2] = processor->first_data_byte;
            processor->buff[processor->buff_len + 3] = byte;
            processor->buff_len += 4;
            processor->process = process_status;
            return;
    }
}

void process_status_or_running_status(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            processor->first_data_byte = byte;
            processor->process = process_running_status_2_of_2;
            return;
    }
}

void process_running_status_2_of_2(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(3, processor);
            if (processor->buff_len == 0) {
                processor->buff[0] = TIMESTAMP_HIGH(timestamp);
                processor->buff[1] = TIMESTAMP_LOW(timestamp);
                processor->buff[2] = processor->status;
                processor->buff[3] = processor->first_data_byte;
                processor->buff[4] = byte;
                processor->buff_len = 5;
            } else {
                processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                processor->buff[processor->buff_len + 1] = processor->first_data_byte;
                processor->buff[processor->buff_len + 2] = byte;
                processor->buff_len += 3;
            }
            processor->process = process_status_or_running_status;
            return;
    }
}

void process_sysex_1_of_n(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            // sysex
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(3, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
            processor->buff[processor->buff_len + 1] = processor->status;
            processor->buff[processor->buff_len + 2] = byte;
            processor->buff_len += 3;
            processor->process = process_sysex_i_of_n;
            return;
    }
}

void process_sysex_i_of_n(uint8_t byte, uint16_t timestamp, struct processor_t *processor) {
    switch (byte >> 4) {
        case STATUS_NOTE_OFF_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_NOTE_ON_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PKP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_CC_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_PROGRAM_CHANGE_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_CP_AFTERTOUCH_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_1;
            return;
        case STATUS_PITCH_BEND_PREF_4:
            processor->timestamp = timestamp;
            processor->status = byte;
            processor->process = process_1_of_2;
            return;
        case STATUS_SYS_PREF_4:
            switch (byte & 0x8) {
                case 0:
                    switch (byte & 0x7) {
                        case STATUS_SYS_EX_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sysex_1_of_n;
                            return;
                        case STATUS_SYS_MIDI_MTC_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_SONG_POSITION_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_2;
                            return;
                        case STATUS_SYS_SONG_SELECT_SUF_3:
                            processor->timestamp = timestamp;
                            processor->status = byte;
                            processor->process = process_sys_1_of_1;
                            return;
                        case STATUS_SYS_UNDEFINED_1_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_UNDEFINED_2_SUF_3:
                            // ignore
                            return;
                        case STATUS_SYS_TUNE_REQ_SUF_3:
                            FLUSH_NOTIFY_IF_EXCEED(2, processor);
                            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            return;
                        case STATUS_SYS_EOF_SUF_3:
                            processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                            processor->buff[processor->buff_len + 1] = byte;
                            processor->buff_len += 2;
                            processor->process = process_status;
                            return;
                        default:
                            return;
                    }
                case 1: // rtm
                    FLUSH_NOTIFY_IF_EXCEED(2, processor);
                    SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
                    processor->buff[processor->buff_len] = TIMESTAMP_LOW(timestamp);
                    processor->buff[processor->buff_len + 1] = byte;
                    processor->buff_len += 2;
                    return;
                default:
                    return;
            }
        default:
            FLUSH_NOTIFY_IF_EXCEED(1, processor);
            SET_HIGH_TIMESTAMP_IF_EMPTY_BUF(timestamp, processor);
            processor->buff[processor->buff_len] = byte;
            processor->buff_len++;
            return;
    }
}