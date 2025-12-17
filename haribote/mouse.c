#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec) {
	/* 書き込み先のFIFOバッファを記憶 */
	mousefifo = fifo;
	mousedata0 = data0;

	/* マウス有効 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);

	mdec->phase = 0;	// マウスからのACK(0xfa)を待っている段階
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data) {
	if (mdec->phase == 0) {
		if (data == 0xfa) mdec->phase = 1;
		return 0;
	} else {

		if (mdec->phase == 1) {
			if ((data & 0xc8) == 0x08) {
				mdec->buf[0] = data;
				mdec->phase = 2;
			}
			return 0;
		} else if (mdec->phase == 2) {
			mdec->buf[1] = data;
			mdec->phase = 3;
			return 0;
		} else if (mdec->phase == 3) {
			mdec->buf[2] = data;
			mdec->phase = 1;

			mdec->btn = mdec->buf[0] & 0x07;
			mdec->x = mdec->buf[1];
			mdec->y = mdec->buf[2];

			if ((mdec->buf[0] & 0x10) != 0) mdec->x |= 0xffffff00;
			if ((mdec->buf[0] & 0x20) != 0) mdec->y |= 0xffffff00;

			mdec->y = - mdec->y;

			return 1;
		}
	}

	return -1;
}

/**
 * マウス割り込み
 */
void inthandler2c(int *esp)
{
	int data;
	io_out8(PIC1_OCW2, 0x64);	// IRQ-12受付完了をPICに通知
	io_out8(PIC0_OCW2, 0x62);	// IRQ-02受付完了をPICに通知
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, mousedata0 + data);
	return;
}
