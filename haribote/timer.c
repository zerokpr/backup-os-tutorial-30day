#include "bootpack.h"

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

#define TIMER_FLAGS_UNUSING		0	// 未使用状態
#define TIMER_FLAGS_ALLOC		1	// 確保した状態
#define TIMER_FLAGS_USING		2	// タイマ作動中

struct TIMERCTL timerctl;
extern struct TIMER *task_timer;

void init_pit(void) {
	int i;
	struct TIMER *sentinel;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	for (i = 0; i < MAX_TIMER; ++i) timerctl.timers0[i].flags = TIMER_FLAGS_UNUSING;

	/* 番兵セット */
	sentinel = timer_alloc();
	sentinel->next = 0;
	sentinel->timeout = 0xffffffff;
	sentinel->flags = TIMER_FLAGS_USING;

	timerctl.count = 0;
	timerctl.t0 = sentinel;
	timerctl.next = 0xffffffff;
	
	return;
}

/**
 * タイマオブジェクトを取得する
 */
struct TIMER *timer_alloc(void) {
	int i;
	for (i = 0; i < MAX_TIMER; ++i) {
		if (timerctl.timers0[i].flags == TIMER_FLAGS_UNUSING) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			timerctl.timers0[i].should_be_canceled = 0;
			return &timerctl.timers0[i];
		}
	}
	return 0;
}

/**
 * タイマオブジェクトを解放する
 */
void timer_free(struct TIMER *timer) {
	timer->flags = TIMER_FLAGS_UNUSING;
	return ;
}

/**
 * タイマ初期化
 */
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
	timer->fifo = fifo;
	timer->data = data;
	timer->next = 0;
	return ;
}

/**
 * タイマに時間を設定する
 */
void timer_settime(struct TIMER *timer, unsigned int timeout) {
	int eflags;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;	// オーバーフローする場合の対策が必要
	timer->flags = TIMER_FLAGS_USING;

	eflags = io_load_eflags();
	io_cli();

	/* 先頭に挿入する場合 */
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		timerctl.t0 = timer;
		timerctl.next = timer->timeout;
		timer->next = t;
		io_store_eflags(eflags);
		io_sti();
		return ;
	}

	while (1) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			s->next = timer;
			timer->next = t;
			io_store_eflags(eflags);
			io_sti();
			return;
		}
	}
	
	// 番兵がいるのでここには来ない

	io_sti();
	return ;
}

/**
 * 指定したタイマーをキャンセルする
 */
int timer_cancel(struct TIMER *timer) {
	int eflags;
	struct TIMER *t;
	eflags = io_load_eflags();
	io_cli();

	if (timer->flags == TIMER_FLAGS_USING) {	// 取り消し処理が必要か
		if (timer == timerctl.t0) {		// タイマーが先頭だった場合の取り消し処理
			t = timer->next;
			timerctl.t0 = t;
			timerctl.next = t->timeout;
		} else {
			t = timerctl.t0;
			while (1) {
				if (t->next == timer) break;
				t = t->next;
			}
			t->next = timer->next;
		}
		timer->flags = TIMER_FLAGS_ALLOC;
		io_store_eflags(eflags);
		return 1;
	}
	io_store_eflags(eflags);
	return 0;
}

/**
 * fifoが差すキューを持っているタイマーを全てキャンセルする
 */
void timer_cancelall(struct FIFO32 *fifo) {
	int eflags, i;
	struct TIMER *t;
	eflags = io_load_eflags();
	io_cli();	// 設定中にタイマの状態が変化しないようにする

	for(i = 0; i < MAX_TIMER; ++i) {
		t = &timerctl.timers0[i];
		if (t->flags != 0 && t->should_be_canceled && t->fifo == fifo) {
			timer_cancel(t);
			timer_free(t);
		}
	}

	io_store_eflags(eflags);
	return;
}

/**
 * タイマ割り込みハンドラ
 */
void inthandler20(int *esp) {
	char ts = 0;
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);		// IRQ-00受付完了をPICに通知
	++timerctl.count;

	if (timerctl.next > timerctl.count) return;

	timer = timerctl.t0;
	for (;timer->timeout <= timerctl.count; timer = timer->next) {
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) {
			fifo32_put(timer->fifo, timer->data);
		} else {	// タスクタイマの場合のみタスクスイッチのフラグを真にする
			ts = 1;
		}
	}

	timerctl.t0 = timer;
	timerctl.next = timer->timeout;

	// TODO: カウンタをリセットする処理を入れる

	/* 割り込み処理が終わったのでタスクスイッチ */
	if (ts != 0) {
		task_switch();
	}
	
	return ;
}
