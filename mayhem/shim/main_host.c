/* Builds upstream src/main.c on the host: renames main() and widens uint32_t for the
 * Image_Flag initializer, whose 32-bit pointer cast only constant-folds on the target. */
#include "CH58x_common.h"
#include "CH58x_sys.h"
#include "CH58xBLE_LIB.h"
#include "leddrv.h"
#include "button.h"
#include "bmlist.h"
#include "resource.h"
#include "animation.h"
#include "font.h"
#include "font3x5.h"
#include "auxbtn.h"
#include "game.h"
#include "flappy.h"
#include "pong.h"
#include "power.h"
#include "data.h"
#include "config.h"
#include "debug.h"
#include "ble/setup.h"
#include "ble/profile.h"
#include "ble/ota.h"
#include "usb/usb.h"
#include "legacyctrl.h"
#define main firmware_main
#define uint32_t uint64_t
#include "../../src/main.c"
#undef main
#undef uint32_t
#include "host.h"

static int bmlist_ready;

void host_reset(void)
{
	shim_reset_eeprom();
	cfg_fallback();
	streaming_enabled = 0;
	memset((void *)fb, 0, sizeof(fb));
	is_play_sequentially = 1;
	ota_display_active = 0;
	mode = 0;
	menu_cursor = 0;
	sw_state = SW_STOPPED;
	clock_active = 0;
	legacy_reset_auth();

	if (!bmlist_ready || bmlist_current() == NULL) {
		bmlist_init(LED_COLS * 4);
		bmlist_ready = 1;
		return;
	}
	clean_bmlist();
	bm_t *bm = bmlist_current();
	memset(bm->buf, 0, bm->width * sizeof(*bm->buf));
	bm->modes = 0;
	bm->is_flash = 0;
	bm->is_marquee = 0;
	bm->anim_step = 0;
	bm->brightness = 0;
	bm->timeout = 0;
}

void host_reload_bmlist(void) { reload_bmlist(); }

void host_animate(int steps)
{
	for (int i = 0; i < steps; i++) {
		common_tasks(common_taskid, ANI_NEXT_STEP);
		bm_t *bm = bmlist_current();
		if (bm->is_flash)
			ani_flash(bm, fb, i);
		if (bm->is_marquee)
			ani_marque(bm, fb, i);
	}
}

uint16_t host_fb(int col) { return fb[col]; }
int host_streaming_enabled(void) { return streaming_enabled; }

static bm_t *nth(int idx)
{
	bm_t *bm = bmlist_head();
	while (idx-- > 0)
		bm = bm->next;
	return bm;
}

int host_bm_count(void)
{
	bm_t *head = bmlist_head();
	if (!head)
		return 0;
	int n = 1;
	for (bm_t *bm = head->next; bm != head; bm = bm->next)
		n++;
	return n;
}
uint16_t host_bm_width(int idx) { return nth(idx)->width; }
uint16_t host_bm_col(int idx, int col) { return nth(idx)->buf[col]; }
