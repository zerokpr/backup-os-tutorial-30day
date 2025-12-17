#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

void wait_KBC_sendready(void) {
	/* キーボードコントローラがデータ送信可能になるのを待つ */
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0) {
	/* キーボードコントローラの有効化 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);

	
	/* 書き込み先のFIFOバッファを記憶する */
	keyfifo = fifo;
	keydata0 = data0;

	return;
}

/**
 * キーボード割り込み
 */
void inthandler21(int *esp) {
	int data;

	data = io_in8(PORT_KEYDAT);
	io_out8(PIC0_OCW2, 0x61);	// IRQ-01受付完了をPICに通知

	fifo32_put(keyfifo, data + keydata0);

	return ;
}
