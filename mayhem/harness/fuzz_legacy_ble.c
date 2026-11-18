/* BLE legacy upload characteristic. The input is a 3-byte session preamble followed by the stream of
 * 16-byte GATT writes of one session, delivered to legacy_ble_rx():
 *   byte 0 bit 0 = 0: PIN gate already open (as after legacy_bypass_auth(), the image path);
 *   byte 0 bit 0 = 1: gate closed with PIN legacy_set_auth_code(bytes 1..2), so the unauthenticated
 *                     branch (wang rejection, PIN parse, wrong-code reply) is reachable too.
 * A shorter input just supplies zero preamble bytes. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"

int legacy_ble_rx(uint8_t *val, uint16_t len);
void legacy_bypass_auth(void);
void legacy_set_auth_code(uint16_t code);

#define PACKET 16
#define PREAMBLE 3

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint8_t pre[PREAMBLE] = {0};
	memcpy(pre, data, size < PREAMBLE ? size : PREAMBLE);
	const uint8_t *stream = size > PREAMBLE ? data + PREAMBLE : data + size;
	size_t stream_len = size > PREAMBLE ? size - PREAMBLE : 0;

	host_reset();
	legacy_bypass_auth();

	/* legacy_ble_rx keeps its packet counter in a function-local static, only touched once the gate
	 * is open; a "wang" header packet through the open gate always restarts it, so every input
	 * starts from the same state (header seen, c == 1) whichever gate mode follows. */
	uint8_t start[PACKET] = {'w', 'a', 'n', 'g'};
	legacy_ble_rx(start, PACKET);

	if (pre[0] & 1)
		legacy_set_auth_code((uint16_t)((pre[1] << 8) | pre[2]));

	for (size_t off = 0; off < stream_len; off += PACKET) {
		size_t n = stream_len - off < PACKET ? stream_len - off : PACKET;
		uint8_t *pkt = malloc(n);
		memcpy(pkt, stream + off, n);
		legacy_ble_rx(pkt, (uint16_t)n);
		free(pkt);
	}
	host_reload_bmlist();
	return 0;
}
