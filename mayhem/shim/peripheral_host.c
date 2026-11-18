/* Builds upstream src/ble/peripheral.c on the host and exposes its OTA IAP state: reset per input, and
 * the erase event the BLE stack's TMOS would deliver after a CMD_IAP_ERASE write. */
#include "CH58xBLE_LIB.h"
#include "../../src/ble/peripheral.c"
#include "host.h"

void host_ota_reset(void)
{
	shim_reset_flash();
	ota_total_bytes = 0;
	ota_written_bytes = 0;
	ota_last_pct = 0xFF;
	memset(&iap_rec_data, 0, sizeof(iap_rec_data));
	OpParaDataLen = 0;
	OpAdd = 0;
	EraseAdd = 0;
	EraseBlockNum = 0;
	EraseBlockCnt = 0;
	VerifyStatus = 0;
	taskid = 1;
}

void host_ota_write(uint8_t index, uint8_t *data, uint8_t len)
{
	OTA_IAPWriteData(index, data, len);
	uint16_t ev = shim_take_events();
	while (ev & OTA_FLASH_ERASE_EVT)
		ev = peripheral_task(taskid, ev);
}
