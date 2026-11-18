/* Known-answer tests for the badge firmware's untrusted-input paths, built from upstream's own sources
 * with their normal flags: input bytes in, asserted values out (config fields, replies, flash, bitmaps). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "host.h"
#include "config.h"
#include "ble/ota.h"

uint8_t ng_parse(uint8_t *val, uint16_t len);
int legacy_ble_rx(uint8_t *val, uint16_t len);
int legacy_usb_rx(uint8_t *buf, uint16_t len);
void legacy_set_auth_code(uint16_t code);
void legacy_bypass_auth(void);
int cfg_readflash_def(badge_cfg_t *cfg);

static int run, failed;

#define CHECK(name, cond) do { \
	run++; \
	if (cond) { printf("PASS %s\n", name); } \
	else { failed++; printf("FAIL %s (%s:%d)\n", name, __FILE__, __LINE__); } \
} while (0)

static uint8_t ng(const uint8_t *frame, size_t n)
{
	uint8_t buf[64];
	memcpy(buf, frame, n);
	shim_notify_clear();
	uint8_t rc = ng_parse(buf, (uint16_t)n);
	return rc;
}

static int reply_is(uint8_t v) { return shim_notify_len() == 1 && shim_notify_buf()[0] == v; }

static void test_ng(void)
{
	host_reset();
	{ const uint8_t f[] = {1, 1, 1}; CHECK("ng reset_rx on: status", ng(f, sizeof(f)) == 0);
	  CHECK("ng reset_rx on: field", badge_cfg.reset_rx == 1); CHECK("ng reset_rx on: reply 0", reply_is(0)); }
	{ const uint8_t f[] = {1, 1, 0}; ng(f, sizeof(f)); CHECK("ng reset_rx off", badge_cfg.reset_rx == 0); }
	{ int before = shim_reset_requests(); const uint8_t f[] = {1, 2};
	  ng(f, sizeof(f)); CHECK("ng power_setting reset executes", shim_reset_requests() == before + 1); }
	{ const uint8_t f[] = {1, 7}; ng(f, sizeof(f)); CHECK("ng power_setting bad fn -> -1", reply_is(0xFF)); }
	{ const uint8_t f[] = {1}; ng(f, sizeof(f)); CHECK("ng power_setting empty -> -2", reply_is(0xFE)); }
	{ const uint8_t f[] = {0x20}; CHECK("ng unknown command", ng(f, sizeof(f)) == 0x18 && shim_notify_len() == 0); }
	{ CHECK("ng empty frame", ng_parse((uint8_t *)"", 0) == 0x18); }

	{ const uint8_t f[] = {4, 1, 'A', 'B', 'C'}; ng(f, sizeof(f));
	  CHECK("ng devname", memcmp(badge_cfg.ble_devname, "ABC\0\0\0\0\0", 8) == 0 && reply_is(0)); }
	{ const uint8_t f[] = {4, 0, 1}; ng(f, sizeof(f)); CHECK("ng always_on", badge_cfg.ble_always_on == 1); }
	{ const uint8_t f[] = {4, 2, 1}; ng(f, sizeof(f)); CHECK("ng ble_setting bad fn -> -1", reply_is(0xFF)); }

	{ const uint8_t f[] = {5, 8, 2, 3, 0xAA, 0x55}; ng(f, sizeof(f));
	  CHECK("ng splash fields", badge_cfg.splash_bm_w == 8 && badge_cfg.splash_bm_h == 2 && badge_cfg.splash_bm_fh == 3);
	  CHECK("ng splash bits", badge_cfg.splash_bm_bits[0] == 0xAA && badge_cfg.splash_bm_bits[1] == 0x55 && badge_cfg.splash_bm_bits[2] == 0);
	  CHECK("ng splash reply 0", reply_is(0)); }
	{ const uint8_t f[] = {5, 49, 2, 0, 1}; ng(f, sizeof(f)); CHECK("ng splash width -> -1", reply_is(0xFF)); }
	{ const uint8_t f[] = {5, 8, 45, 0, 1}; ng(f, sizeof(f)); CHECK("ng splash height -> -2", reply_is(0xFE)); }
	{ const uint8_t f[] = {5, 8}; ng(f, sizeof(f)); CHECK("ng splash short -> -4", reply_is(0xFC)); }

	{ const uint8_t f[] = {8, 1, 3}; ng(f, sizeof(f)); CHECK("ng brightness", badge_cfg.led_brightness == 3 && reply_is(0)); }
	{ const uint8_t f[] = {8, 1, 9}; ng(f, sizeof(f)); CHECK("ng brightness range -> -2", badge_cfg.led_brightness == 3 && reply_is(0xFE)); }
	{ const uint8_t f[] = {8, 0, 0x10, 0x00}; ng(f, sizeof(f)); CHECK("ng splash speed", badge_cfg.splash_speedT == 16 && reply_is(0)); }

	{ const uint8_t f[] = {3, 1, 2}; ng(f, sizeof(f)); CHECK("ng stream_bitmap outside streaming -> -1", reply_is(0xFF)); }
	{ const uint8_t f[] = {2, 0}; ng(f, sizeof(f)); CHECK("ng streaming on", host_streaming_enabled() == 1 && reply_is(0)); }
	{ const uint8_t f[] = {3, 0x34, 0x12, 0x78, 0x56}; ng(f, sizeof(f));
	  CHECK("ng stream_bitmap columns", host_fb(0) == 0x1234 && host_fb(1) == 0x5678 && host_fb(2) == 0); }
	{ const uint8_t f[] = {2, 1}; ng(f, sizeof(f)); CHECK("ng streaming off", host_streaming_enabled() == 0); }

	{ const uint8_t a[] = {4, 1, 'Z'}, b[] = {8, 1, 2}, s[] = {6};
	  ng(a, sizeof(a)); ng(b, sizeof(b)); ng(s, sizeof(s));
	  badge_cfg_t back;
	  CHECK("ng save_cfg reply 0", reply_is(0));
	  CHECK("ng save_cfg flash roundtrip", cfg_readflash_def(&back) == 0 && back.led_brightness == 2 && back.ble_devname[0] == 'Z' && back.ble_devname[1] == 0); }
	{ const uint8_t f[] = {7}; ng(f, sizeof(f));
	  CHECK("ng load_fallback_cfg", badge_cfg.led_brightness == 0 && memcmp(badge_cfg.ble_devname, "LED Badge Magic", 16) == 0 && reply_is(0)); }
}

#define ROWS 11
#define HDR 64
#define CHUNKS 16

static uint8_t image[HDR + ROWS * CHUNKS];

static void build_image(void)
{
	memset(image, 0, sizeof(image));
	memcpy(image, "wang\0\0", 6);
	image[8] = 0x04;
	image[16 + 1] = CHUNKS;
	image[38] = 233;
	image[39] = 11;
	image[HDR + 0] = 0xFF;
	image[HDR + 11 + 10] = 0x80;
}

static int bitmap_ok(void)
{
	if (host_bm_count() != 1 || host_bm_width(0) != (ROWS * CHUNKS * 8) / ROWS)
		return 0;
	for (int i = 0; i < 8; i++)
		if (host_bm_col(0, i) != 0x0001)
			return 0;
	return host_bm_col(0, 8) == 0x0400 && host_bm_col(0, 9) == 0 && host_bm_col(0, 127) == 0;
}

static void test_legacy_ble(void)
{
	host_reset();
	build_image();
	uint8_t pkt[16];

	legacy_set_auth_code(4321);
	memcpy(pkt, "wang\0\0\0\0\0\0\0\0\0\0\0\0", 16);
	CHECK("ble header before auth -> -4", legacy_ble_rx(pkt, 16) == -4);
	memcpy(pkt, "1234\0\0\0\0\0\0\0\0\0\0\0\0", 16);
	CHECK("ble wrong pin -> -5", legacy_ble_rx(pkt, 16) == -5);
	CHECK("ble short packet -> -1", legacy_ble_rx(pkt, 15) == -1);
	memcpy(pkt, "4321\0\0\0\0\0\0\0\0\0\0\0\0", 16);
	CHECK("ble right pin -> 0", legacy_ble_rx(pkt, 16) == 0);

	int ok = 1;
	for (size_t off = 0; off < sizeof(image); off += 16) {
		memcpy(pkt, image + off, 16);
		ok &= legacy_ble_rx(pkt, 16) == 0;
	}
	CHECK("ble upload packets accepted", ok);
	CHECK("ble upload stored header", memcmp(shim_eeprom_data(), "wang\0", 5) == 0 && shim_eeprom_data()[8] == 0x04);
	CHECK("ble upload stored bitmap bytes", shim_eeprom_data()[HDR] == 0xFF && shim_eeprom_data()[HDR + 21] == 0x80);
	host_reload_bmlist();
	CHECK("ble upload decodes to bitmap", bitmap_ok());
}

static void test_legacy_usb(void)
{
	host_reset();
	build_image();
	uint8_t pkt[64];
	int ok = 1;
	for (size_t off = 0; off < sizeof(image); off += 64) {
		size_t n = sizeof(image) - off < 64 ? sizeof(image) - off : 64;
		memset(pkt, 0, sizeof(pkt));
		memcpy(pkt, image + off, n);
		ok &= legacy_usb_rx(pkt, (uint16_t)n) == 0;
	}
	CHECK("usb upload packets accepted", ok);
	CHECK("usb upload stored header", memcmp(shim_eeprom_data(), "wang\0", 5) == 0);
	host_reload_bmlist();
	CHECK("usb upload decodes to bitmap", bitmap_ok());
	memset(pkt, 0, sizeof(pkt));
}

static const uint8_t *ota_reply_is(const uint8_t *want, int n)
{
	return shim_ota_reply_len() == n && memcmp(shim_ota_reply(), want, n) == 0 ? want : NULL;
}

static void ota(const uint8_t *frame, size_t n)
{
	uint8_t buf[IAP_LEN];
	memcpy(buf, frame, n);
	host_ota_write(0, buf, (uint8_t)n);
}

static void test_ota(void)
{
	host_reset();
	host_ota_reset();
	const uint8_t *flash = shim_flash_data();
	const uint8_t ok[] = {0, 0}, bad_range[] = {0xFF, 0}, bad_cmd[] = {0xFE, 0};

	{ const uint8_t f[] = {CMD_IAP_INFO, 0}; ota(f, sizeof(f));
	  const uint8_t want[] = {1, 0x00, 0x60, 0x03, 0x00, 0x00, 0x10, 0x82, 0};
	  CHECK("ota info reply", shim_ota_reply_len() == 20 && memcmp(shim_ota_reply(), want, sizeof(want)) == 0); }

	{ const uint8_t f[] = {CMD_IAP_PROM, 4, 0x10, 0, 'a', 'b', 'c', 'd'}; ota(f, sizeof(f));
	  CHECK("ota prom reply", ota_reply_is(ok, 2) != NULL);
	  CHECK("ota prom lands at slot B + 16*addr", memcmp(flash + IMAGE_B_START_ADD + 0x100, "abcd", 4) == 0); }
	{ const uint8_t f[] = {CMD_IAP_VERIFY, 4, 0x10, 0, 'a', 'b', 'c', 'd'}; ota(f, sizeof(f));
	  CHECK("ota verify match", ota_reply_is(ok, 2) != NULL); }
	{ const uint8_t f[] = {CMD_IAP_VERIFY, 4, 0x10, 0, 'a', 'b', 'c', 'X'}; ota(f, sizeof(f));
	  CHECK("ota verify mismatch", shim_ota_reply_len() == 2 && shim_ota_reply()[0] == 1); }
	{ const uint8_t f[] = {CMD_IAP_ERASE, 0, 0, 0, 2, 0}; ota(f, sizeof(f));
	  CHECK("ota erase reply", ota_reply_is(ok, 2) != NULL);
	  CHECK("ota erase clears the programmed bytes", memcmp(flash + IMAGE_B_START_ADD + 0x100, "\xff\xff\xff\xff", 4) == 0); }
	{ const uint8_t f[] = {CMD_IAP_ERASE, 0, 0, 0, 0, 1}; ota(f, sizeof(f));
	  CHECK("ota erase beyond slot B -> 0xFF", ota_reply_is(bad_range, 2) != NULL); }
	{ const uint8_t f[] = {0x99}; ota(f, sizeof(f));
	  CHECK("ota unknown command -> 0xFE", ota_reply_is(bad_cmd, 2) != NULL); }

	{ FLASH_ROM_ERASE(IMAGE_IAP_START_ADD, FLASH_BLOCK_SIZE);
	  int resets = shim_reset_requests();
	  const uint8_t f[] = {CMD_IAP_END, 0}; ota(f, sizeof(f));
	  CHECK("ota end aborts when the IAP region is blank", ota_reply_is(bad_cmd, 2) != NULL && shim_reset_requests() == resets); }
	{ host_ota_reset();
	  int resets = shim_reset_requests();
	  const uint8_t f[] = {CMD_IAP_END, 0}; ota(f, sizeof(f));
	  CHECK("ota end switches the boot flag to slot B", shim_eeprom_data()[OTA_DATAFLASH_ADD] == IMAGE_B_FLAG);
	  CHECK("ota end resets the chip", shim_reset_requests() == resets + 1); }
}

int main(void)
{
	test_ng();
	test_legacy_ble();
	test_legacy_usb();
	test_ota();
	printf("SUMMARY: %d tests run; %d failed\n", run, failed);
	return failed ? 1 : 0;
}
