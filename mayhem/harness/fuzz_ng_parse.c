/* BLE "next-gen" control characteristic: one command frame through upstream's ng_parse(). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"

uint8_t ng_parse(uint8_t *val, uint16_t len);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size == 0 || size > 0xFFFF)
		return -1;
	host_reset();
	uint8_t *buf = malloc(size);
	memcpy(buf, data, size);
	uint8_t rc = ng_parse(buf, (uint16_t)size);
	free(buf);
	return rc == 0 ? 0 : -1;
}
