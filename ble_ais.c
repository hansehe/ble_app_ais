/* Attention! 
*  To maintain compliance with Nordic Semiconductor ASA�s Bluetooth profile 
*  qualification listings, this section of source code must not be modified.
*/

#include "ble_ais.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_ais       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_ais_t * p_ais, ble_evt_t * p_ble_evt)
{
    p_ais->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_ais       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_ais_t * p_ais, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_ais->conn_handle = BLE_CONN_HANDLE_INVALID;
}


// Define behaviour on BLE events
void ble_ais_on_ble_evt(ble_ais_t * p_ais, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_ais, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_ais, p_ble_evt);
            break;

        default:
            break;
    }
}



// Function for adding the data characteristic.
static uint32_t data_char_add(ble_ais_t * p_ais, const ble_ais_init_t * p_ais_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type = p_ais->uuid_type;
    ble_uuid.uuid = AIS_UUID_DATA_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = 20;
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = 20;
    attr_char_value.p_value      = NULL;
    
    return sd_ble_gatts_characteristic_add(p_ais->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_ais->data_char_handles);
}


// Function to initialize services and charateristics
uint32_t ble_ais_init(ble_ais_t * p_ais, const ble_ais_init_t * p_ais_init)
{
    if (p_ais == NULL || p_ais_init == NULL)
    {
        return NRF_ERROR_NULL;
    }
    
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_ais->conn_handle        = BLE_CONN_HANDLE_INVALID;
    p_ais->evt_write_handler 	= p_ais_init->evt_write_handler;

    // Add service
    ble_uuid128_t base_uuid = {AIS_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_ais->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
		
		ble_uuid.type = p_ais->uuid_type;
    ble_uuid.uuid = AIS_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_ais->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }


		// Add data charateristic
		err_code = data_char_add(p_ais, p_ais_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
		
		
    return NRF_SUCCESS;
}


// Define function to send data
uint32_t ble_ais_data_send(ble_ais_t * p_ais, uint8_t data[BLE_AIS_DATA_CHAR_LEN])
{
    ble_gatts_hvx_params_t params;
    uint16_t len = BLE_AIS_DATA_CHAR_LEN;
    
    memset(&params, 0, sizeof(params));
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_ais->data_char_handles.value_handle;
    params.p_data = data;
    params.p_len = &len;
    
    return sd_ble_gatts_hvx(p_ais->conn_handle, &params);
}
