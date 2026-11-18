#ifndef MAYHEM_HOST_H
#define MAYHEM_HOST_H
#include <stddef.h>
#include <stdint.h>

void shim_reset_eeprom(void);
const uint8_t *shim_eeprom_data(void);
void shim_set_eeprom(const uint8_t *data, size_t len);
int shim_notify_len(void);
const uint8_t *shim_notify_buf(void);
void shim_notify_clear(void);
int shim_reset_requests(void);
void shim_reset_flash(void);
const uint8_t *shim_flash_data(void);
uint16_t shim_take_events(void);
int shim_ota_reply_len(void);
const uint8_t *shim_ota_reply(void);

/* from main_host.c: upstream main.c state, reset and driven the way its tasks do */
void host_reset(void);
void host_animate(int steps);
void host_reload_bmlist(void);
uint16_t host_fb(int col);
int host_streaming_enabled(void);
int host_bm_count(void);
uint16_t host_bm_width(int idx);
uint16_t host_bm_col(int idx, int col);

/* from peripheral_host.c: upstream src/ble/peripheral.c state, reset and driven as TMOS would */
void host_ota_reset(void);
void host_ota_write(uint8_t index, uint8_t *data, uint8_t len);
#endif
