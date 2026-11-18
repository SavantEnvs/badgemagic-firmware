/* Data-flash bitmap store: the input is the flash image (what a completed upload leaves behind),
 * decoded by the boot-time load_bmlist() and then played by the animation task. */
#include <stdint.h>
#include "host.h"

void load_bmlist(void);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	host_reset();
	shim_set_eeprom(data, size);
	load_bmlist();
	host_animate(24);
	return 0;
}
