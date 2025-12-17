#include "bootpack.h"
#include<stdio.h>
#include<string.h>

void cons_newline(struct CONSOLE *cons) {
	int x, y;
	struct TASK *task = task_now();
	struct WINDOW *window = cons->window;
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16;
	} else {
		if (window != 0) {
			for (y = 28; y < 28 + 112; ++y) {
				for (x = 8; x < 8 + 240; ++x) {
					window->buf[x + y * window->bxsize] = window->buf[x + (y + 16) * window->bxsize];
				}
			}
			for (y = 28 + 112; y < 28 + 128; ++y) {
				for (x = 8; x < 8 + 240; ++x) {
					window->buf[x + y * window->bxsize] = COL8_000000;
				}
			}
			window_refresh(window, 8, 28, 8 + 240, 28 + 128);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x += 8;
	}
	return;
}

/**
 * コンソールに1文字を打ち込み表示する
 * move: カーソルを進めない場合は0
 */
void cons_putchar(struct CONSOLE *cons, int chr, char move) {
	char s[2];
	s[0] = chr;
	s[1] = '\0';

	if (s[0] == '\t') {	// タブ対応
		while (1) {
			if (cons->window != 0) {
				putfonts8_asc_window(cons->window, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons->cur_x = 8;
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) break;	// 32で割り切れたらbreak
		}
	} else if (s[0] == '\n') {
		cons->cur_x = 8;
		cons_newline(cons);
	} else if (s[0] == '\r') {	// 復帰
		// 無視
	} else {	// 通常文字
		if (cons->window != 0) {
			putfonts8_asc_window(cons->window, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}

		if (move != 0) {
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons->cur_x = 8;
				cons_newline(cons);
			}
		}
	}
}

/**
 * コンソールに文字列sを出力する（sに '\0' が出てくるまで）
 */
void cons_putstr(struct CONSOLE *cons, char *s) {
	while (*s != '\0') cons_putchar(cons, *(s++), 1);
	return;
}

/**
 * コンソールに文字列sをn文字出力する
 */
void cons_putstrn(struct CONSOLE *cons, char *s, int n) {
	int i;
	for (i = 0; i < n && s[i] != '\0'; ++i) cons_putchar(cons, s[i], 1);
	return;
}

/**
 * メモリの空き状態などを出力する
 */
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
	char s[128];
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	sprintf(s, "total memory: %dMB\nfree memory: %dKB\n\n", memtotal / (1024*1024), memman_total(memman) / 1024);
	cons_putstr(cons, s);
	return;
}

/**
 * スクリーンをクリアする
 */
void cmd_cls(struct CONSOLE *cons) {
	int x, y;
	struct WINDOW *window = cons->window;
	for (y = 28; y < 28 + 128; ++y) {
		for (x = 8; x < 8 + 240; ++x) {
			window->buf[x + y * window->bxsize] = COL8_000000;
		}
	}
	window_refresh(window, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28;
	return;
}

/**
 * ファイル出力
 */
void cmd_ls(struct CONSOLE *cons) {
	char s[30];
	int x, y;
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	for (x = 0; x < 224; ++x) {
		if (finfo[x].name[0] == 0x00) break;
		if (finfo[x].name[0] != 0xe5 && (finfo[x].type & 0x18) == 0) {
			sprintf(s, "filename.ext    %7d\n", finfo[x].size);
			for(y = 0; y < 8; ++y) s[y] = finfo[x].name[y];
			s[9] = finfo[x].ext[0];
			s[10] = finfo[x].ext[1];
			s[11] = finfo[x].ext[2];
			
			cons_putstr(cons, s);
		}
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
	int segsiz, datsiz, esp, dathrb, appsiz;
	struct FILEINFO *finfo;
	struct WINDOWCTL *windowctl;
	struct WINDOW *window;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *p, *q;
	struct TASK *task = task_now();

	int i;
	char name[18];	// コマンドの実行ファイル名

	for (i = 0; i < 13; ++i) {
		if (cmdline[i] <= ' ') break;
		name[i] = cmdline[i];
	}
	name[i] = '\0';

	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);

	if (finfo == 0 && name[i-1] != '.') {
		name[i] = '.';
		name[i+1] = 'H';
		name[i+2] = 'R';
		name[i+3] = 'B';
		name[i+4] = '\0';
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) { // ファイルが見つかった場合
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat);

		if (p == 0) {
			cons_putstr(cons, "Error: memory exhausted.\n");
			return 0;
		}
		
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz 	= *((int *) (p + 0x0000));
			esp		= *((int *) (p + 0x000c));
			datsiz	= *((int *) (p + 0x0010));
			dathrb	= *((int *) (p + 0x0014));

			q = (char *) memman_alloc_4k(memman, segsiz);
			if (q == 0) {
				cons_putstr(cons, "Error: memory exhausted.\n");
				return 0;
			}

			task->ds_base = (int) q;

			set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);

			for (i = 0; i < datsiz; ++i) q[esp + i] = p[dathrb + i];

			// アプリを起動する
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));

			/*-- アプリ終了後処理 --*/
			
			/* クローズしてないファイルをクローズ */
			for (i = 0; i < 8; ++i) {
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}

			/* ウィンドウ終了処理 */
			windowctl = (struct WINDOWCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_WINDOWS; ++i) {
				window = &(windowctl->windows0[i]);
				/* アプリが開きっぱなしになったウィンドウを発見 */
				if ((window->flags & 0x11) == 0x11 && window->task == task) {
					window_free(window);
				}
			}
			
			/* 不要なタイマーをすべてキャンセルする */
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr(cons, ".hrb file format error.\n");
		}
		
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}

	return 0;
}

/**
 * ぱっぱやー
 */
void cmd_roachan(struct CONSOLE *cons) {
	cons_putstr(cons, "pappaya~\n\n");
}

/**
 * コンソールを終了する。
 * 自身を終了させるのは難しいので、メインタスクに終了を要求する。
 * メインタスクのfifoに768～1023番のデータを送り、このデータから768を引いてwinctl->windows0を足したものが終了対象のウィンドウのアドレスになる
 */
void cmd_exit(struct CONSOLE *cons, int *fat) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct WINDOWCTL *winctl = (struct WINDOWCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	timer_cancel(cons->timer);
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();

	if (cons->window) {		// ウィンドウがある場合は768～1023
		fifo32_put(fifo, cons->window - winctl->windows0 + 768);
	} else {				// ウィンドウがない場合は1024～2023
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);
	}

	io_sti();
	while (1) task_sleep(task);
}

/**
 * 新しいコンソールを開いてそこからアプリを実行する
 */
void cmd_start(struct CONSOLE *cons, char *cmdline, unsigned int memtotal) {
	struct WINDOWCTL *winctl = (struct WINDOWCTL *) *((int *) 0x0fe4);
	struct WINDOW *window = open_console(winctl, memtotal);
	struct FIFO32 *fifo = &window->task->fifo;
	int i;

	window_slide(window, 32, 4);
	window_updown(window, winctl->top);

	for (i = 6; cmdline[i] != 0; ++i) fifo32_put(fifo, cmdline[i] + 256);
	fifo32_put(fifo, 10 + 256);		// 作ったコンソールにエンターキー送信

	cons_newline(cons);		// 元のコンソールで改行
	return;
}

/**
 * ウィンドウがないコンソールを作成する
 */
void cmd_ncst(struct CONSOLE *cons, char *cmdline, unsigned int memtotal) {
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;

	// コマンドラインに入力された文字列を新しいコンソールに入力する
	for (i = 5; cmdline[i] != 0; ++i) fifo32_put(fifo, cmdline[i] + 256);
	fifo32_put(fifo, 10 + 256);	// エンターキー

	cons_newline(cons);
	return;
}

/**
 * コマンド処理
 */
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
	if (strcmp(cmdline, "mem") == 0 && cons->window != 0) {					// memコマンド
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0 && cons->window != 0) {			// 画面リセットコマンド
		cmd_cls(cons);
	} else if (strcmp(cmdline, "ls") == 0 && cons->window != 0) {
		cmd_ls(cons);
	} else if (strcmp(cmdline, "roachan") == 0 && cons->window != 0) {
		cmd_roachan(cons);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {			// コマンドではなく、さらに空行でもない
			cons_putstr(cons, "Bad Command.\n\n");
		}
	}
}

/**
 * コンソールの言語モードを設定する
 */
void cmd_langmode(struct CONSOLE *cons, char *cmdline) {
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9] - '0';
	char s[30];
	if (mode <= 2) {
		task->langmode = mode;
		sprintf(s, "Current Langmode: ");
		if (mode == 0) strcpy(s, "ASCII");
		else if (mode == 1) strcpy(s+17, "Shift-JIS");
		else if (mode == 2) strcpy(s+17, "Japanese-EUC");
		cons_putstr(cons, s);
	} else {
		cons_putstr(cons, "mode number error\n");
	}
	cons_newline(cons);
	return;
}


/**
 * 
 */
void hrb_api_linewin(struct WINDOW *window, int x0, int y0, int x1, int y1, int col) {
	int i, x, y, dx, dy, len;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;

	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) dx = -1024;
		else dx = 1024;

		if (y0 <= y1) dy = ((y1 - y0 + 1) << 10) / len;
		else dy = ((y1 - y0 - 1) << 10) / len;
	} else {
		len = dy + 1;
		if (y0 > y1) dy = -1024;
		else dy = 1024;

		if (x0 <= x1) dx = ((x1 - x0 + 1) << 10) / len;
		else dx = ((x1 - x0 - 1) << 10) / len;
	}

	for (i = 0; i < len; ++i) {
		window->buf[(y >> 10) * window->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}


/**
 * to_sleep: 0以外ならキューにデータが入っていないとき実行中のタスクをスリープする。0ならすぐにリターンする
 */
void hrb_api_waitkey_inner(struct CONSOLE* cons, int to_sleep, int *ret_reg) {
	struct WINDOWCTL *windowctl = (struct WINDOWCTL *) *((int *) 0x0fe4);
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	struct TASK *task = task_now();
	int que_dat;
	
	while (1) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			if (to_sleep != 0) {
				task_sleep(task);
			} else {
				io_sti();
				ret_reg[7] = -1;
				return;
			}
		}

		que_dat = fifo32_get(&task->fifo);
		io_sti();
		if (que_dat <= 1) {		// カーソル用タイマ
			timer_init(cons->timer, &task->fifo, 1);
			timer_settime(cons->timer, 50);
		}
		
		if (que_dat == 2) {		// カーソルON
			cons->cur_c = COL8_FFFFFF;
		}

		if (que_dat == 3) {		// カーソルOFF
			cons->cur_c = -1;
		}

		if (que_dat == 4) {		// コンソールだけを閉じる
			timer_cancel(cons->timer);
			io_cli();
			fifo32_put(sys_fifo, cons->window - windowctl->windows0 + 2024);	// コンソールのみを閉じる要求
			cons->window = 0;
			io_sti();
		}

		if (que_dat >= 256) {
			ret_reg[7] = que_dat - 256;
			return;
		}
	}
	return;
}

/**
 * 全てのAPIの呼び出し口
 */
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	int ds_base = task->ds_base;
	struct WINDOWCTL *windowctl = (struct WINDOWCTL *) *((int *) 0x0fe4);
	struct WINDOW *window;

	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	int *reg = &eax + 1;	// 2回 PUSHAD しているので、EAXの次の番地が元のアプリで受け取るレジスタになる
	/**
	 * reg[0]: EDI, reg[1]: ESI, reg[2]: EBP, reg[3]: ESP
	 * reg[4]: EBX, reg[5]: EDX, reg[6]: ECX, reg[7]: EAX
	 */

	int i;

	if (edx == 1) cons_putchar(cons, eax & 0xff, 1);
	else if (edx == 2) cons_putstr(cons, (char *) ebx + ds_base);
	else if (edx == 3) cons_putstrn(cons, (char *) ebx + ds_base, ecx);
	else if (edx == 4) return &(task->tss.esp0);
	else if (edx == 5) {	// ウィンドウ作成
		window = window_alloc(windowctl);
		window->task = task;
		window->flags |= 0x10;	// 自動クローズ有効
		window_setbuf(window, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		window_slide(window, (windowctl->xsize - esi) / 2 & ~3, (windowctl->ysize - edi) / 2);
		window_updown(window, windowctl->top);
		reg[7] = (int) window;
	} else if (edx == 6) {		// 1文字表示API
		window = (struct WINDOW *) (ebx & 0xfffffffe);		// ebxの番号はwindowのポインタ
		putfonts8_asc(window->buf, window->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) window_refresh(window, esi, edi, esi + ecx * 8, edi + 16);
	} else if (edx == 7) {		// 矩形を描画する
		window = (struct WINDOW *) (ebx & 0xfffffffe);		// ebxの番号はwindowのポインタ
		boxfill8(window->buf, window->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) window_refresh(window, eax, ecx, esi + 1, edi + 1);
	} else if (edx == 8) {		// memmanの初期化
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {		// malloc
		ecx = (ecx + 0x0f) & 0xfffffff0;
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {		// free
		ecx = (ecx + 0x0f) & 0xfffffff0;
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {		// 点を描画する
		window = (struct WINDOW *) (ebx & 0xfffffffe);
		window->buf[window->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) window_refresh(window, esi, edi, esi + 1, edi + 1);
	} else if (edx == 12) {		// ウィンドウを更新する
		window = (struct WINDOW *) ebx;
		window_refresh(window, eax, ecx, esi, edi);
	} else if (edx == 13) {		// 線を引く
		window = (struct WINDOW *) (ebx & 0xfffffffe);
		hrb_api_linewin(window, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			/* XOR swap */
			if (eax > esi) {
				eax ^= esi;
				esi ^= eax;
				eax ^= esi;
			}
			if (ecx > edi) {
				ecx ^= edi;
				edi ^= ecx;
				ecx ^= edi;
			}
			window_refresh(window, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {		// ウィンドウを閉じる
		window_free((struct WINDOW *) ebx);
	} else if (edx == 15) {		// キー入力を待機する
		hrb_api_waitkey_inner(cons, eax, reg);
	} else if (edx == 16) {		// タイマの取得
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->should_be_canceled = 1;
	} else if (edx == 17) {		// タイマの送信データ設定(init)
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {		// タイマの時間設定
		timer_settime((struct TIMER *) ebx, eax);
	} else if (edx == 19) {		// タイマの解放
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {
		if (eax == 0) {		// 消音
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	} else if (edx == 21) {		// ファイルのオープン
		/* 空いているファイルの探索 */
		for (i = 0; i < 8; ++i) if (task->fhandle[i].buf == 0) break;

		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i >= 8) return 0;

		finfo = file_search((char *) ebx + ds_base,
			(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
		if (!finfo) return 0;

		reg[7] = (int) fh;
		fh->size = finfo->size;
		fh->pos = 0;
		fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);

	} else if (edx == 22) {		// ファイルのクローズ
		fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);	
		fh->buf = 0;
	} else if (edx == 23) {		// ファイルのシーク
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) fh->pos = ebx;
		else if (ecx == 1) fh->pos += ebx;
		else if (ecx == 2) fh->pos = fh->size + ebx;
		
		if (fh->pos < 0) fh->pos = 0;
		if (fh->pos > fh->size) fh->pos = fh->size;

	} else if (edx == 24) {		// ファイルサイズの取得
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) reg[7] = fh->size;
		else if (ecx == 1) reg[7] = fh->size;
		else if (ecx == 2) reg[7] = fh->pos - fh->size;

	} else if (edx == 25) {		// ファイルの読み込み
		fh = (struct FILEHANDLE *) eax;
		for (i = 0; i < ecx; ++i) {
			if (fh->pos == fh->size) break;
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			++(fh->pos);
		}
		reg[7] = i;
	} else if (edx == 26) {		// コマンドラインの取得
		i = 0;
		while (1) {
			*((char *) ebx + ds_base + i) = task->cmdline[i];
			if (task->cmdline[i] == 0) break;
			if (i >= ecx) break;
			++i;
		}
		reg[7] = i;
	} else if (edx == 27) {
		reg[7] = task->langmode;
	} else {
		cons_putstr(cons, "edx value wrong!");
	}

	return 0;
}

/**
 * コンソールの実行関数
 */
void console_task_func(struct WINDOW *window, unsigned int memtotal) {
	int i;
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	struct FILEHANDLE fhandle[8];

	int queue_data;
	char cmdline[30];	// コマンドライン用のバッファ

	struct CONSOLE cons;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	cons.window = window;
	task->cons = &cons;

	if (window != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}

	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	
	/* ---タスク構造体の初期化--- */
	/* ファイルハンドル初期化 */
	for (i = 0; i < 8; ++i) fhandle[i].buf = 0;
	task->fhandle = fhandle;
	task->fat = fat;
	task->cmdline = cmdline;


	/*
	  taskのlangmodeのデフォルト値を1に設定
	  日本語フォントファイルが読み込めない場合は0（後半に0xffが入っている）
	 */
	if (nihongo[4096] != 0xff) task->langmode = 1;
	else task->langmode = 0;
	task->langbyte1 = 0;

	// プロンプト表示
	cons_putchar(&cons, '>', 1);

	while (1) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			queue_data = fifo32_get(&task->fifo);
			io_sti();

			/* カーソル点滅用 */
			if (queue_data <= 1 && cons.window != 0) {
				if (queue_data != 0) {
					timer_init(cons.timer, &task->fifo, 0);
					if (cons.cur_c >= 0) cons.cur_c = COL8_FFFFFF;
				} else {
					timer_init(cons.timer, &task->fifo, 1);
					if (cons.cur_c >= 0) cons.cur_c = COL8_000000;
				}
				timer_settime(cons.timer, 50);
			}

			/* メインタスクからの情報受信用 */
			if (queue_data == 2) cons.cur_c = COL8_FFFFFF;	// カーソルオフ
			if (queue_data == 3) {	// カーソルオフ
				if (cons.window != 0) {
					boxfill8(
						window->buf,
						window->bxsize,
						COL8_000000,
						cons.cur_x,
						cons.cur_y,
						cons.cur_x + 7,
						cons.cur_y + 15
					);
				}
				cons.cur_c = -1;
			}
			if (queue_data == 4) cmd_exit(&cons, fat);

			/* キーボードデータ処理 */
			if (is_keyboard_data(queue_data)) {
				if (queue_data == 8 + 256) { 			// バックスペース処理
					if (cons.cur_x > 16) {
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (queue_data == 10 + 256) {	// エンターキー処理
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = '\0';
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);
					
					if (!cons.window) cmd_exit(&cons, fat); // ウィンドウがない場合はコマンド実行後すぐに終了する

					/* プロンプト表示 */
					cons_putchar(&cons, '>', 1);
				} else {	// 通常文字処理
					if (cons.cur_x < 240) {
						cmdline[cons.cur_x / 8 - 2] = queue_data - 256;
						cons_putchar(&cons, queue_data - 256, 1);
					}
				}

			}

			if (cons.window) {
				if (cons.cur_c >= 0) {
					boxfill8(window->buf, window->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				window_refresh(window, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}


/**
 * スタック例外ハンドラ
 */
int *inthandler0c (int *esp) {
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];

	cons_putstr(cons, "\nINT 0C :\n Stack Exception. \n");

	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr(cons, s);

	return &(task->tss.esp0);
}

/**
 * 一般保護例外ハンドラ
 */
int *inthandler0d(int *esp) {
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];

	cons_putstr(cons, "\nINT 0D: \n General Protected Exception.\n");
	sprintf(s, "CS=%04X,DS=%04X,EIP = %08X\n", esp[12], esp[9], esp[11]);
    cons_putstr(cons, s);
	return &(task->tss.esp0);
}
