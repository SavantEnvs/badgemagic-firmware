/* BLE OTA characteristic: the input is the sequence of GATT writes of one update session, each a length
 * byte followed by that many bytes, delivered to upstream's OTA_IAPWriteData() (the OTAProfile write
 * callback) with the code-flash model reset per input. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"

#define IAP_LEN 247 /* src/ble/ota.h: OTAProfile_WriteAttrCB stages each write in a buffer of this size */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	host_reset();
	host_ota_reset();

	for (size_t off = 0; off < size;) {
		size_t n = data[off++];
		if (n > IAP_LEN)
			return -1; /* cannot be a write: the profile's staging buffer holds IAP_LEN bytes */
		if (n > size - off)
			n = size - off;
		if (n == 0)
			continue; /* OTAProfile_WriteAttrCB (OTAprofile.c:341) never calls back for an empty write */
		uint8_t *pkt = malloc(n);
		memcpy(pkt, data + off, n);
		host_ota_write(0, pkt, (uint8_t)n);
		free(pkt);
		off += n;
	}
	return 0;
}
