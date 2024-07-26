#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/ans/ble_svc_ans.h"
#include "services/gatt/ble_svc_gatt.h"

#include "gatt.h"
#include "uuids.h"

static uint8_t gatt_midi_chr_val;
uint16_t gatt_midi_chr_val_handle;
static uint8_t gatt_midi_dsc_val;

static int gatt_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len, void *dst, uint16_t *len) {
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

static int gatt_svc_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *args) {
    const ble_uuid_t *uuid;
    int rc;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            uuid = ctxt->chr->uuid;
            if (attr_handle == gatt_midi_chr_val_handle) {
                rc = os_mbuf_append(ctxt->om, &gatt_midi_chr_val, sizeof(gatt_midi_chr_val));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            goto unknown;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            uuid = ctxt->chr->uuid;
            if (attr_handle == gatt_midi_chr_val_handle) {
                rc = gatt_write(ctxt->om, sizeof(gatt_midi_chr_val), sizeof(gatt_midi_chr_val), &gatt_midi_chr_val,
                                    NULL);
                ble_gatts_chr_updated(attr_handle);
                return rc;
            }
            goto unknown;

        case BLE_GATT_ACCESS_OP_READ_DSC:
            uuid = ctxt->dsc->uuid;
            if (ble_uuid_cmp(uuid, &gatt_midi_dsc_uuid.u) == 0) {
                rc = os_mbuf_append(ctxt->om, &gatt_midi_dsc_val, sizeof(gatt_midi_dsc_val));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            goto unknown;

        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            goto unknown;

        default:
            goto unknown;
    }

    unknown:
    /* Unknown characteristic/descriptor;
     * The NimBLE host should not have called this function;
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
        {
                /*** Service ***/
                .type = BLE_GATT_SVC_TYPE_PRIMARY,
                .uuid = &gatt_midi_svc_uuid.u,
                .characteristics = (struct ble_gatt_chr_def[])
                        {{
                                 /*** This characteristic can be subscribed to by writing 0x00 and 0x01 to the CCCD ***/
                                 .uuid = &gatt_midi_chr_uuid.u,
                                 .access_cb = gatt_svc_access,
                                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                                 .val_handle = &gatt_midi_chr_val_handle,
                                 .descriptors = (struct ble_gatt_dsc_def[])
                                         {{
                                                  .uuid = &gatt_midi_dsc_uuid.u,
                                                  .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE,
                                                  .access_cb = gatt_svc_access,
                                          },
                                          {
                                                  0, /* No more descriptors in this characteristic */
                                          }
                                         },
                         },
                         {
                                 0, /* No more characteristics in this service. */
                         }
                        },
        },

        {
                0, /* No more services. */
        },
};

int gatt_midi_init(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        return rc;
    }

    /* Setting a value for the read-only descriptor */
    gatt_midi_dsc_val = 0x99;

    return 0;
}