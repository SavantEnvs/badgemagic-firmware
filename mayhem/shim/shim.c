/* Host stand-ins for the CH582 vendor library, the BLE/USB stacks and the LED matrix driver.
 * Only hardware is replaced: every parser and state machine under test is upstream's own src/. */
#include "hw.h"
#include "host.h"
#include "CH58xBLE_LIB.h"
#include "ble/ota.h"

static uint8_t eeprom[EEPROM_MAX_SIZE];
static uint8_t notify_buf[16];
static int notify_len;
static unsigned cdc_bytes;
static uint16_t pending_events;

void shim_reset_eeprom(void)
{
	memset(eeprom, 0xFF, sizeof(eeprom));
}

uint32_t shim_eeprom(int op, uint32_t addr, void *buf, uint32_t len)
{
	if (addr > EEPROM_MAX_SIZE || len > EEPROM_MAX_SIZE - addr)
		return 1;
	switch (op) {
	case 0:
		memcpy(buf, eeprom + addr, len);
		break;
	case 1:
		memcpy(eeprom + addr, buf, len);
		break;
	default: {
		uint32_t first = addr & ~(uint32_t)(EEPROM_BLOCK_SIZE - 1);
		uint32_t last = (addr + len + EEPROM_BLOCK_SIZE - 1) & ~(uint32_t)(EEPROM_BLOCK_SIZE - 1);
		if (last > EEPROM_MAX_SIZE)
			last = EEPROM_MAX_SIZE;
		memset(eeprom + first, 0xFF, last - first);
	}
	}
	return 0;
}

static uint8_t flash[FLASH_ROM_MAX_SIZE];
static uint8_t ota_reply[32];
static int ota_reply_len;

void shim_reset_flash(void)
{
	memset(flash, 0xFF, sizeof(flash));
	/* the resident IAP bootloader: non-blank, as on a real device */
	memset(flash + IMAGE_IAP_START_ADD, 0x13, IMAGE_IAP_SIZE);
	ota_reply_len = 0;
	pending_events = 0;
}

uint8_t *shim_flash_at(uint32_t addr, uint32_t len)
{
	if (addr > FLASH_ROM_MAX_SIZE || len > FLASH_ROM_MAX_SIZE - addr)
		return NULL;
	return flash + addr;
}

uint32_t shim_flash_erase(uint32_t addr, uint32_t len)
{
	if (addr > FLASH_ROM_MAX_SIZE || len > FLASH_ROM_MAX_SIZE - addr)
		return 1;
	uint32_t first = addr & ~(uint32_t)(EEPROM_BLOCK_SIZE - 1);
	uint32_t last = (addr + len + EEPROM_BLOCK_SIZE - 1) & ~(uint32_t)(EEPROM_BLOCK_SIZE - 1);
	if (last > FLASH_ROM_MAX_SIZE)
		last = FLASH_ROM_MAX_SIZE;
	memset(flash + first, 0xFF, last - first);
	return 0;
}
const uint8_t *shim_flash_data(void) { return flash; }

bStatus_t OTAProfile_SendData(unsigned char id, unsigned char *p, unsigned char len)
{
	(void)id;
	ota_reply_len = len < sizeof(ota_reply) ? len : sizeof(ota_reply);
	memcpy(ota_reply, p, ota_reply_len);
	return SUCCESS;
}
bStatus_t OTAProfile_AddService(uint32_t s) { (void)s; return SUCCESS; }
bStatus_t OTAProfile_RegisterAppCBs(void *cbs) { (void)cbs; return SUCCESS; }
int shim_ota_reply_len(void) { return ota_reply_len; }
const uint8_t *shim_ota_reply(void) { return ota_reply; }

void SYS_DisableAllIrq(void *p) { (void)p; }
void mDelaymS(uint32_t ms) { (void)ms; }

bStatus_t GAPRole_PeripheralInit(void) { return SUCCESS; }
bStatus_t GAPRole_SetParameter(uint16_t a, uint16_t b, void *c) { (void)a; (void)b; (void)c; return SUCCESS; }
bStatus_t GAPRole_TerminateLink(uint16_t h) { (void)h; return SUCCESS; }
bStatus_t GAPRole_PeripheralConnParamUpdateReq(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint8_t f)
{
	(void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
	return SUCCESS;
}
bStatus_t GAPRole_PeripheralStartDevice(uint8_t t, gapBondCBs_t *b, gapRolesCBs_t *c) { (void)t; (void)b; (void)c; return SUCCESS; }
bStatus_t GAPRole_BroadcasterSetCB(gapRolesBroadcasterCBs_t *c) { (void)c; return SUCCESS; }
bStatus_t GGS_SetParameter(uint16_t a, uint16_t b, void *c) { (void)a; (void)b; (void)c; return SUCCESS; }
bStatus_t GGS_AddService(uint32_t s) { (void)s; return SUCCESS; }
bStatus_t GATTServApp_AddService(uint32_t s) { (void)s; return SUCCESS; }
bStatus_t GAP_SetParamValue(uint16_t a, uint16_t b) { (void)a; (void)b; return SUCCESS; }
bStatus_t GAPBondMgr_SetParameter(uint16_t a, uint16_t b, void *c) { (void)a; (void)b; (void)c; return SUCCESS; }

const uint8_t *shim_eeprom_data(void) { return eeprom; }

void shim_set_eeprom(const uint8_t *data, size_t len)
{
	shim_reset_eeprom();
	memcpy(eeprom, data, len < sizeof(eeprom) ? len : sizeof(eeprom));
}

int shim_notify_len(void) { return notify_len; }
const uint8_t *shim_notify_buf(void) { return notify_buf; }
void shim_notify_clear(void) { notify_len = 0; }

bStatus_t ng_notify(uint8_t *p, uint8_t len)
{
	notify_len = len < sizeof(notify_buf) ? len : sizeof(notify_buf);
	memcpy(notify_buf, p, notify_len);
	return SUCCESS;
}

int cdc_tx_poll(uint8_t *buf, int len, uint16_t timeout_ms)
{
	(void)buf; (void)timeout_ms;
	cdc_bytes += len;
	return len;
}
void cdc_onWrite(void (*cb)(uint8_t *, uint16_t)) { (void)cb; }
void hiddev_onWrite(void (*cb)(uint8_t *, uint16_t)) { (void)cb; }
void usb_start(void) {}

void tmos_memcpy(void *dst, const void *src, uint32_t len) { memcpy(dst, src, len); }
void tmos_memset(void *dst, uint8_t v, uint32_t len) { memset(dst, v, len); }
uint16_t tmos_rand(void) { return (uint16_t)rand(); }
tmosTaskID TMOS_ProcessEventRegister(uint16_t (*fn)(tmosTaskID, uint16_t)) { (void)fn; return 1; }
uint8_t tmos_start_task(tmosTaskID t, tmosEvents e, uint32_t d) { (void)t; (void)e; (void)d; return 0; }
uint8_t tmos_start_reload_task(tmosTaskID t, tmosEvents e, uint32_t d) { (void)t; (void)e; (void)d; return 0; }
uint8_t tmos_stop_task(tmosTaskID t, tmosEvents e) { (void)t; (void)e; return 0; }
uint8_t tmos_set_event(tmosTaskID t, tmosEvents e) { (void)t; pending_events |= e; return 0; }
uint16_t shim_take_events(void) { uint16_t e = pending_events; pending_events = 0; return e; }
uint8_t *tmos_msg_receive(tmosTaskID t) { (void)t; return NULL; }
uint8_t tmos_msg_deallocate(uint8_t *p) { (void)p; return 0; }
void TMOS_SystemProcess(void) {}
void tmos_clockInit(void) {}

static int reset_requests;
void SYS_ResetExecute(void) { reset_requests++; }
void shim_reset_jump(void) {}
int shim_reset_requests(void) { return reset_requests; }
void SetSysClock(int v) { (void)v; }
void DelayMs(uint32_t ms) { (void)ms; }
void RTC_InitTime(uint16_t y, uint16_t mo, uint16_t d, uint16_t h, uint16_t mi, uint16_t s)
{
	(void)y; (void)mo; (void)d; (void)h; (void)mi; (void)s;
}
void RTC_GetTime(uint16_t *y, uint16_t *mo, uint16_t *d, uint16_t *h, uint16_t *mi, uint16_t *s)
{
	*y = 2025; *mo = 1; *d = 1; *h = 0; *mi = 0; *s = 0;
}
void PFIC_EnableIRQ(int irq) { (void)irq; }
void LowPower_Shutdown(int m) { (void)m; }
void Calibration_LSI(int l) { (void)l; }
void PWR_PeriphWakeUpCfg(int a, int b, int c) { (void)a; (void)b; (void)c; }

int GPIOA_ReadPortPin(uint32_t p) { (void)p; return 1; }
int GPIOB_ReadPortPin(uint32_t p) { (void)p; return 1; }
void GPIOA_ModeCfg(uint32_t p, int m) { (void)p; (void)m; }
void GPIOB_ModeCfg(uint32_t p, int m) { (void)p; (void)m; }
void GPIOA_SetBits(uint32_t p) { (void)p; }
void GPIOA_ITModeCfg(uint32_t p, int m) { (void)p; (void)m; }
void TMR0_TimerInit(uint32_t t) { (void)t; }
void TMR3_TimerInit(uint32_t t) { (void)t; }
void TMR0_ITCfg(int a, int b) { (void)a; (void)b; }
void TMR3_ITCfg(int a, int b) { (void)a; (void)b; }
int TMR0_GetITFlag(int f) { (void)f; return 0; }
int TMR3_GetITFlag(int f) { (void)f; return 0; }
void TMR0_ClearITFlag(int f) { (void)f; }
void TMR3_ClearITFlag(int f) { (void)f; }
void UART1_DefInit(void) {}
void UART1_BaudRateCfg(uint32_t b) { (void)b; }
void ADC_ExtSingleChSampInit(int a, int b) { (void)a; (void)b; }
int16_t ADC_DataCalib_Rough(void) { return 0; }
void ADC_ChannelCfg(int c) { (void)c; }
uint16_t ADC_ExcutSingleConver(void) { return 2048; }

void led_init(void) {}
void leds_releaseall(void) {}
void led_write2dcol(int dcol, uint16_t c1, uint16_t c2) { (void)dcol; (void)c1; (void)c2; }
void led_write2row_raw(int row, int half, uint32_t val) { (void)row; (void)half; (void)val; }

void ble_hardwareInit(void) {}
int legacy_registerService(void) { return 0; }
int devInfo_registerService(void) { return 0; }
int batt_registerService(void) { return 0; }
int ng_registerService(void) { return 0; }
