/* USB CDC/HID upload endpoint. legacy_usb_rx() keeps its reassembly state in function-local statics
 * that cannot be reset, so this is a file-input target: one process per input. The file is the
 * stream of 64-byte endpoint transfers of one session, read whole (the transfer length a header
 * declares can reach ~720 KB); as in the usb driver (ep_out / data_ep_buf) each transfer lands in
 * a fixed MAX_PACKET_SIZE endpoint buffer. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"

int legacy_usb_rx(uint8_t *buf, uint16_t len);

#define PACKET 64

int main(int argc, char **argv)
{
	if (argc < 2)
		return 2;
	FILE *f = fopen(argv[1], "rb");
	if (!f)
		return 2;
	size_t size = 0, cap = 4096;
	uint8_t *in = malloc(cap);
	for (;;) {
		if (size == cap)
			in = realloc(in, cap *= 2);
		size_t r = fread(in + size, 1, cap - size, f);
		if (r == 0)
			break;
		size += r;
	}
	fclose(f);
	if (size == 0)
		return 1;

	host_reset();
	uint8_t *ep = malloc(PACKET);
	for (size_t off = 0; off < size; off += PACKET) {
		size_t n = size - off < PACKET ? size - off : PACKET;
		memset(ep, 0, PACKET);
		memcpy(ep, in + off, n);
		legacy_usb_rx(ep, (uint16_t)n);
	}
	host_reload_bmlist();
	free(in);
	return 0;
}
