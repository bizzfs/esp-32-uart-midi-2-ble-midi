#include <stdarg.h>

#include "esp_log.h"
#include "esp_peripheral.h"
#include "host/ble_hs.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "ble.h"
#include "gatt.h"
#include "uuids.h"

static uint8_t own_addr_type;
static uint16_t conn_handle;
static uint16_t itvl_min = 0x06;
static uint16_t itvl_max = 0x0c;
uint16_t preferred_mtu = 256;

void (*on_conn_interval_change)(uint16_t value);

void (*on_mtu_change)(uint16_t value);

static const char *TAG = "BLE";

static int gap_callback(struct ble_gap_event *event, void *args);

void ble_store_config_init(void);

static void advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields, rsp_fields;
    const char *name;
    int rc;

    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = &gatt_midi_svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    memset(&rsp_fields, 0, sizeof rsp_fields);
    name = ble_svc_gap_device_name();
    rsp_fields.name = (uint8_t *) name;
    rsp_fields.name_len = strlen(name);
    rsp_fields.name_is_complete = 1;
    rsp_fields.tx_pwr_lvl_is_present = 1;
    rsp_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement rsp data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_callback, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static int gap_callback(struct ble_gap_event *event, void *args) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status == 0) {
                conn_handle = event->connect.conn_handle;

                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);

                struct ble_gap_upd_params conn_params;
                conn_params.itvl_min = itvl_min; // x 1.25ms
                conn_params.itvl_max = itvl_max; // x 1.25ms
                conn_params.latency = 0x00; //number of skippable connection events
                conn_params.supervision_timeout = 0xA0; // x 6.25ms, time before peripheral will assume connection is dropped.
                ble_gap_update_params(event->connect.conn_handle, &conn_params);

                rc = ble_att_set_preferred_mtu(preferred_mtu);
            }
            if (event->connect.status != 0) {
                advertise();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
            conn_handle = 0;
            advertise();
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            MODLOG_DFLT(INFO, "connection updated; status=%d\n",
                        event->conn_update.status);
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            on_conn_interval_change(desc.conn_itvl);
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            MODLOG_DFLT(INFO, "advertise complete; reason=%d\n",
                        event->adv_complete.reason);
            advertise();
            return 0;

        case BLE_GAP_EVENT_ENC_CHANGE:
            /* Encryption has been enabled or disabled for this connection. */
            MODLOG_DFLT(INFO, "encryption change event; status=%d\n",
                        event->enc_change.status);
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            return 0;

        case BLE_GAP_EVENT_MTU:
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.channel_id,
                        event->mtu.value);
            on_mtu_change(event->mtu.value);
            return 0;

        case BLE_GAP_EVENT_REPEAT_PAIRING:
            /* We already have a bond with the peer, but it is attempting to
             * establish a new secure link.  This app sacrifices security for
             * convenience: just throw away the old bond and accept the new link.
             */

            /* Delete the old bond. */
            rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
            assert(rc == 0);
            ble_store_util_delete_peer(&desc.peer_id_addr);

            /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
             * continue with the pairing operation.
             */
            return BLE_GAP_REPEAT_PAIRING_RETRY;

        case BLE_GAP_EVENT_PASSKEY_ACTION:
            ESP_LOGI(TAG, "PASSKEY_ACTION_EVENT started");
            struct ble_sm_io pkey = {0};
            int key = 0;

            if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                pkey.action = event->passkey.params.action;
                pkey.passkey = 123456; // This is the passkey to be entered on peer
                ESP_LOGI(TAG, "Enter passkey %"
                PRIu32
                "on the peer side", pkey.passkey);
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
                ESP_LOGI(TAG, "Passkey on device's display: %"
                PRIu32, event->passkey.params.numcmp);
                ESP_LOGI(TAG, "Accept or reject the passkey through console in this format -> key Y or key N");
                pkey.action = event->passkey.params.action;
                if (scli_receive_key(&key)) {
                    pkey.numcmp_accept = key;
                } else {
                    pkey.numcmp_accept = 0;
                    ESP_LOGE(TAG, "Timeout! Rejecting the key");
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
                static uint8_t tem_oob[16] = {0};
                pkey.action = event->passkey.params.action;
                for (int i = 0; i < 16; i++) {
                    pkey.oob[i] = tem_oob[i];
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
            } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
                ESP_LOGI(TAG, "Enter the passkey through console in this format-> key 123456");
                pkey.action = event->passkey.params.action;
                if (scli_receive_key(&key)) {
                    pkey.passkey = key;
                } else {
                    pkey.passkey = 0;
                    ESP_LOGE(TAG, "Timeout! Passing 0 as the key");
                }
                rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
            }
            return 0;

        case BLE_GAP_EVENT_AUTHORIZE:
            MODLOG_DFLT(INFO, "authorize event: conn_handle=%d attr_handle=%d is_read=%d",
                        event->authorize.conn_handle,
                        event->authorize.attr_handle,
                        event->authorize.is_read);

            /* The default behaviour for the event is to reject authorize request */
            event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
            return 0;
    }

    return 0;
}

static void on_reset(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void on_sync(void) {
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");
    /* Begin advertising. */
    advertise();
}

static void nimble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_notify(uint8_t *byte_buff, uint16_t length) {
    struct os_mbuf *om;

    if (!conn_handle) {
        return;
    }

    om = ble_hs_mbuf_from_flat(byte_buff, length);
    ble_gatts_notify_custom(conn_handle, gatt_midi_chr_val_handle, om);
}


void ble_midi_start(struct ble_midi_args_t *args) {
    int rc;

    if (args->conn_interval_min) itvl_min = args->conn_interval_min;
    if (args->conn_interval_max) itvl_max = args->conn_interval_max;
    if (args->preferred_mtu) preferred_mtu = args->preferred_mtu;
    on_conn_interval_change = args->conn_interval_change_callback;
    on_mtu_change = args->mtu_change_callback;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
    ble_hs_cfg.sm_sc = 0;

    rc = gatt_midi_init();
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set(args->device_name);
    assert(rc == 0);

    ble_store_config_init();
    nimble_port_freertos_init(nimble_host_task);
}
