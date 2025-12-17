#include "corosapi.h"

void RoaMain (void) {
	char *buf;
	int win;
	int key, x, y;

	api_initmalloc();
	buf = api_malloc(160 * 100);
	win = api_openwin(buf, 160, 100, -1, "walk");
	api_boxfillwin(win, 4, 24, 155, 95, 0);
	x = 76;
	y = 56;

	api_putstrwin(win, x, y, 3, 1, "*");

	while (1) {
		key = api_getkey(1);
		api_putstrwin(win, x, y, 0 /* 黒 */, 1, "*");
		if (key == '4' && x > 4) x -= 8;
		if (key == '6' && x < 148) x += 8;
		if (key == '8' && y > 24) y -= 8;
		if (key == '2' && y < 80) y += 8;
		if (key == 0x0a) break;
		api_putstrwin(win, x, y, 3, 1, "*");
	}

	api_closewin(win);
	api_end();
}
