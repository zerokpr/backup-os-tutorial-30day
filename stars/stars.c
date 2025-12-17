#include "corosapi.h"
int rand(void);

void RoaMain (void) {
	char *buf;
	int win;
	int i;
	int x, y;

	api_initmalloc();
	buf = api_malloc(150 * 100);
	win = api_openwin(buf, 150, 100, -1, "stars");
	api_boxfillwin(win+1, 6, 26, 143, 93, 0);
	for (i = 0; i < 30; ++i) {
		x = (rand() % 137) + 6;
		y = (rand() % 124) + 26;
		api_point(win+1, x, y, 3);
	}
	api_refreshwin(win, 6, 26, 144, 94);
	api_end();
}
