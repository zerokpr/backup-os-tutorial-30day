#include"corosapi.h"

void RoaMain (void) {
	char buf[150 * 50];
	int win;

	api_initmalloc();
	win = api_openwin(buf, 150, 50, -1, "hello");
	api_boxfillwin(win, 8, 36, 141, 43, 6);
	api_putstrwin(win, 28, 28, 0, 15, "Hello, Roachan!");

	while (api_getkey(1) != 0x0a);
	
	api_end();
}
