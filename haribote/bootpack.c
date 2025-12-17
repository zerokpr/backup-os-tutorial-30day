#include<stdio.h>
#include<string.h>
#include "bootpack.h"

#define KEYCMD_LED	0xed

/**
 * キーボード、マウス、タイマからの割り込みで発生するデータを全て管理するキュー。
 * データの種類で以下のように使用する。
 * 
 * 0～1 	カーソル点滅用タイマ
 * 3		3秒タイマ
 * 10		10秒タイマ
 * 256～511	キーボード入力（KBCから読んだ値に256を足す）
 * 512～767	マウス入力（KBCから読んだ値に512を足す）
 */
struct FIFO32 interrupt_fifo;
extern struct TIMERCTL timerctl;


void set_roachan_screen(unsigned char *vram, int w, int h);

void RoaMain(void) {
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40]; // 画面に文字列を表示するためのバッファ

	int i, j, x, y; // カウンター変数
	struct WINDOW *window = 0, *window2; // ウィンドウループ変数
	struct FIFO32 keycmd;
	struct WINDOW *key_win = 0;	// キー入力先
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

	int queue_data;	// fifoでgetしたデータの格納先
	int interrupt_fifo_buff[256], keycmd_buf[32];	// fifo用バッファ
	int mx, my, move_mode_x = -1, move_mode_y = -1, move_mode_x_2 = 0, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	char mcursor[16][16];
	struct MOUSE_DEC mdec;
	struct TASK *main_task, *task;

	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	struct WINDOWCTL *windowctl;
	struct WINDOW *back_window, *mouse_window;
	unsigned char *buf_back, buf_mouse[256];
	static char keytable0[0x80] = {
			0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a,   0,   'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
			0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08,   0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a,   0,   'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};

	/* 日本語フォント読み込み用の変数 */
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];


	/* キューの初期化 */
	fifo32_init(&interrupt_fifo, 256, interrupt_fifo_buff, 0);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	*((int *) 0x0fec) = (int) &interrupt_fifo;

	init_gdtidt();
	init_pic();
	io_sti();	// GDTとIDTの初期化終わったので割り込み許可

	/* タイマー初期化 */
	init_pit();

	io_out8(PIC0_IMR, 0xf8); /* PIT, PIC1, キーボードを許可(11111000) */
	io_out8(PIC1_IMR, 0xef); /* マウスを許可(11101111) */

	init_keyboard(&interrupt_fifo, 256);

	init_palette();

	/* メモリマネージャ初期化 */
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	/* 日本語フォント読み込み */
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	} else {	// フォントファイルがない場合
		nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
		// フォントファイルがない場合は前半を半角文字で埋める
		for (i = 0; i < (1 << 12); ++i) nihongo[i] = hankaku[i];
		
		// フォントファイルがない場合は後半を0xffで覆いつくす
		for (i = (1 << 12); i < (1 << 12) + 32 * 94 * 47; ++i) nihongo[i] = 0xff;
	}
	*((int *) 0x0fe8) = (int) nihongo;
	memman_free_4k(memman, (int) fat, 4 * 2880);
	

	/* マウスカーソル位置の初期化 */
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	putblock8_8((char *)buf_mouse, binfo->scrnx, 16, 16, mx, my, (char *)mcursor, 16);
	enable_mouse(&interrupt_fifo, 512, &mdec);

	/* ウィンドウマネージャ設定 */
	windowctl = windowctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	back_window = window_alloc(windowctl);
	mouse_window = window_alloc(windowctl);
	*((int *) 0x0fe4) = (int) windowctl;

	/* タスク・ウィンドウ設定 */
	main_task = task_init(memman);
	interrupt_fifo.task = main_task;	// メインのタスクをfifoに指定する
	task_run(main_task, 1, 0);
	
	/* 背景設定 */
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	set_roachan_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	
	/* ウィンドウ設定 */
	key_win = open_console(windowctl, memtotal);
	window_setbuf(back_window, buf_back, binfo->scrnx, binfo->scrny, -1);	// 透明色なし
	window_setbuf(mouse_window, buf_mouse, 16, 16, 99); 					// 透明色番号は99

	window_slide(back_window, 0, 0);
	window_slide(key_win, 32, 4);
	window_slide(mouse_window, mx, my);
	window_updown(back_window, 0);
	window_updown(key_win, 1);
	window_updown(mouse_window, 2);

	/* 入力先ウィンドウ変数の初期化 */
	keywin_on(key_win);

	/* 最初にキーボード状態との食い違いがないように、設定しておくことにする */
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	/* メインループ */
	while (1) {
		window_refresh(back_window, 0, 0, back_window->bxsize, back_window->bysize);
	
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}

		io_cli();
		if (fifo32_status(&interrupt_fifo) == 0) {
			/* FIFOが空いたので、保留している描画があれば実行する */
			if (new_mx >= 0) {
				io_sti();
				window_slide(mouse_window, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				window_slide(window, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(main_task);
				io_sti();
			}
		} else {
			queue_data = fifo32_get(&interrupt_fifo);
			io_sti();

			/* 入力ウィンドウが閉じられた場合 */
			if (key_win != 0 && key_win->flags == 0) {
				if (windowctl->top == 1) {
					key_win = 0;
				} else {
					key_win = windowctl->windows[windowctl->top-1];
					keywin_on(key_win);
				}
			}

			/* キーボードデータの処理 */
			if (is_keyboard_data(queue_data)) {

				// 左シフトが押された場合
				if (queue_data == 256 + 0x2a) key_shift |= 1;

				// 右シフトが押された場合
				if (queue_data == 256 + 0x36) key_shift |= 2;

				// 左シフトが離された場合
				if (queue_data == 256 + 0xaa) key_shift &= ~1;
				
				// 右シフトが離された場合
				if (queue_data == 256 + 0xb6) key_shift &= ~2;
				
				// F11キー
				if (queue_data == 256 + 0x57) window_updown(windowctl->windows[1], windowctl->top - 1);

				if (queue_data == 256 + 0x3a) {		// CapsLock
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}

				if (queue_data == 256 + 0x45) {		// NumLock
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}

				if (queue_data == 256 + 0x46) {		// ScrollLock
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				
				if (queue_data == 256 + 0xfa) {		// リザーブ？
					keycmd_wait = -1;
				}

				if (queue_data == 256 + 0xfe) {		// リザーブ？
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}

				/* Shift + F1 が押されたらコンソールで実行しているタスクを終了する */
				if (queue_data == 256 + 0x3b && key_shift != 0 && key_win != 0) kill_console_task(key_win->task);

				/* Shift + F2 が押されたら新しくコンソールを作る */
				if (queue_data == 256 + 0x3c && key_shift != 0) {					
					// 新しく作ったコンソールは入力選択状態にしておく
					if (key_win) keywin_off(key_win);
					key_win = open_console(windowctl, memtotal);
					window_slide(key_win, 32, 4);
					window_updown(key_win, windowctl->top);
					keywin_on(key_win);
				}

				// キーコードを文字コードに変換
				if (queue_data < 0x80 + 256) {
					if (key_shift == 0) s[0] = keytable0[queue_data-256];
					else s[0] = keytable1[queue_data-256];
				} else {
					s[0] = 0;
				}

				
				if ('A' <= s[0] && s[0] <= 'Z') {
					if (
						((key_leds & 4) == 0 && key_shift == 0)			// Caps LockもShiftも押してないか
						|| ((key_leds & 4) != 0 && key_shift != 0)		// Caps LockもShiftも押されている
					) {
						s[0] += 'a' - 'A';
					}
				}

				if (s[0] != 0 && key_win != 0) {	// 通常文字、バックスペース、エンターキー
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}

				/* Tabキーでウィンドウ切り替え対応 */
				if (queue_data == 256 + 0x0f && key_win != 0) {
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) j = windowctl->top - 1;
					key_win = windowctl->windows[j];
					keywin_on(key_win);
				}

			/* マウスデータの処理 */
			} else if (is_mouse_data(queue_data)) {
				if (mouse_decode(&mdec, queue_data-512) != 0) {

					// マウスカーソルの移動
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;

					window_slide(mouse_window, mx, my);
					
					/* キーボードウィンドウを動かす */
					new_mx = mx;
					new_my = my;
					if ((mdec.btn & 0x01) != 0) {	// マウスの左ボタンがクリックされている場合
						if (move_mode_x < 0) {	// 通常モードの場合

							/* 手前のウィンドウから調べていく */
							for (j = windowctl->top-1; j > 0; --j) {
								window = windowctl->windows[j];
								x = mx - window->vx0;
								y = my - window->vy0;
								if (0 <= x && x < window->bxsize && 0 <= y && y < window->bysize) {
									if (window->buf[y * window->bxsize + x] != window->col_inv) {
										window_updown(window, windowctl->top-1);
										
										/* 入力ウィンドウとは別のウィンドウをクリックしたら入力ウィンドウを切り替える */
										if (window != key_win) {
											keywin_off(key_win);
											key_win = window;
											keywin_on(key_win);
										}

										/* ウィンドウタイトルをクリックした場合はウィンドウ移動モードへ */
										if (3 <= x && x < window->bxsize - 3 && 3 <= y && y < 21) {
											move_mode_x = mx;
											move_mode_y = my;
											move_mode_x_2 = window->vx0;
											new_wy = window->vy0;
										}

										/* ×ボタンをクリックしたらウィンドウを閉じる */
										if (window->bxsize - 21 <= x && x < window->bxsize - 5 && 5 <= y && y < 19) {
											if ((window->flags & 0x10) != 0) {	// アプリが作ったウィンドウか判定
												task = window->task;
												cons_putstr(task->cons, "\nBreak(mouse) :\n");
												io_cli();
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											} else {
												task = window->task;
												window_updown(window, -1);
												keywin_off(key_win);
												key_win = windowctl->windows[windowctl->top-1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {	// ウィンドウ移動モードの場合
							x = mx - move_mode_x;
							y = my - move_mode_y;
							
							new_wx = (move_mode_x_2 + x + 2) & ~3;
							new_wy += y;
							move_mode_y = my;
						}
					} else {	// マウスの左ボタンがクリックされていなかった場合
						move_mode_x = -1;
						move_mode_y = -1;
						
						if (new_wx != 0x7fffffff) {
							window_slide(window, new_wx, new_wy);	// 一度確定させる
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (is_cons_termination_req(queue_data)) {
				close_console(windowctl->windows0 + (queue_data - 768));
			} else if (is_nwcons_termination_req(queue_data)) {
				close_constask(taskctl->tasks0 + (queue_data - 1024));
			} else if (2024 <= queue_data && queue_data <= 2279) {	// コンソールだけを閉じる
				window2 = windowctl->windows0 + (queue_data - 2024);
				memman_free_4k(memman, (int) window2->buf, 265 * 165);
				window_free(window2);
			}
		}
	}
}

/**
 * コンソールのタスクを作成する
 * windowなしで作成したい場合は引数windowを0にしておく
 */
struct TASK *open_constask(struct WINDOW *window, unsigned int memtotal) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	struct TASK *task = task_alloc();

	/* tss初期化 */
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;	// 64 * 1024の一番下から8を引いた分。この後に;
	task->tss.eip = (int) &console_task_func;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int * ) (task->tss.esp + 4)) = (int) window;
	*((int * ) (task->tss.esp + 8)) = memtotal;
	
	/* タスク実行 */
	task_run(task, 2, 2);	// level=2, priority=2
	
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}


/**
 * 新しくコンソールを開く
 */
struct WINDOW *open_console(struct WINDOWCTL *winctl, unsigned int memtotal) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct WINDOW *window = window_alloc(winctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);

	window_setbuf(window, buf, 256, 165, -1);	// 透明色なし
	make_window8(buf, 256, 165, "yuduki roa console", 0);
	make_textbox8(window, 8, 28, 240, 128, COL8_000000);

	window->task = open_constask(window, memtotal);
	window->flags |= 0x20;	// カーソルあり

	return window;
}

void close_constask(struct TASK *task) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task->flags = 0;
	return ;
}

void close_console(struct WINDOW *window) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = window->task;
	memman_free_4k(memman, (int) window->buf, 256 * 165);
	window_free(window);
	close_constask(task);
	return;
}


/**
 * 指定した window のキーボード入力モードを OFF にする
 */
void keywin_off(struct WINDOW *key_win) {
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 3);	// コンソールのカーソルOFF
	}
	return;
}

/**
 * 指定した window のキーボード入力モードを ON にする
 */
void keywin_on(struct WINDOW *key_win) {
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) {
		fifo32_put(&key_win->task->fifo, 2);	// コンソールのカーソルON
	}
	return;
}

/**
 * コンソールのタスクを終了させる
 */
void kill_console_task(struct TASK *task) {
	if (task != 0 && task->tss.ss0 != 0) {
		cons_putstr(task->cons, "\nBreak(key) :\n");
		io_cli();
		task->tss.eax = (int) &(task->tss.esp0);
		task->tss.eip = (int) asm_end_app;
		io_sti();
		task_run(task, -1, 0);
	}
	return;
}

/**
 * fifoに来たデータがキーボードのデータであるか
 */
inline unsigned int is_keyboard_data(int data) {
	return 256 <= data && data <= 511;
}

/**
 * fifoに来たデータがマウスのデータであるか
 */
inline unsigned int is_mouse_data(int data) {
	return 512 <= data && data <= 767;
}

/**
 * fifoに来たデータがコンソールを終了させるためのリクエストであるか
 */
inline unsigned int is_cons_termination_req(int data) {
	return 768 <= data && data <= 1023;
}

/**
 * fifoに来たデータがウィンドウなしのコンソールを終了させるためのリクエストであるか
 */
inline unsigned int is_nwcons_termination_req(int data) {
	return 1024 <= data && data <= 2023;
}

void tss_init(struct TSS32 *tss) {
	tss->ldtr = 0;
	tss->iomap = 0x40000000;
	return;
}


unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	x &= 1; /* 偶数か奇数か */
	y &= 1;
	i = table[x + y * 2];	/* 中間色を作るための定数 */
	r = (r * 21) / 256;	/* これで 0～20 になる */
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;	/* これで 0～5 になる */
	g = (g + i) / 4;
	b = (b + i) / 4;
	return 16 + r + g * 6 + b * 36;
}


/**
 * 
 */
void set_roachan_screen(unsigned char *vram, int w, int h) {
	struct DLL_STRPICENV env;
	struct FILEINFO *finfo;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *filebuf;
	struct RGB *picbuf = (struct RGB *) memman_alloc_4k(memman, w * h * (sizeof (struct RGB)));
	int jpeginfo[8];
	int *fat;
	char s[30];

	int i; // counter

	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	finfo = file_search("roachan.jpg", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);

	filebuf = file_loadfile2(finfo->clustno, &finfo->size, fat);
	info_JPEG(&env, jpeginfo, finfo->size, filebuf);

	decode0_JPEG(&env, finfo->size, filebuf, 4, (char *) picbuf, 0);

	/**
	 * jpeginfo[2]: xsize
	 * jpeginfo[3]: ysize
	 */
	for (i = 0; i < jpeginfo[2] * jpeginfo[3]; ++i) {
		vram[i] = rgb2pal(
			picbuf[i].r,
			picbuf[i].g,
			picbuf[i].b,
			i % jpeginfo[2],
			i / jpeginfo[2]
		);
	}

	memman_free_4k(memman, (unsigned int) filebuf, finfo->size);
	memman_free_4k(memman, (unsigned int) picbuf, w * h * (sizeof (struct RGB)));
	memman_free_4k(memman, (unsigned int) fat, 4 * 2880);

	return;
}
