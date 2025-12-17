#include "bootpack.h"

struct WINDOWCTL *windowctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
	struct WINDOWCTL *ctl;
	int i;

	ctl = (struct WINDOWCTL *) memman_alloc_4k(memman, sizeof (struct WINDOWCTL));
	if (ctl == 0) return ctl;

	ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	if (ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof (struct WINDOWCTL));
		return ctl;
	}

	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;	// 初期状態ではウィンドウがないため-1にしておく

	for (i = 0; i < MAX_WINDOWS; ++i) {
		ctl->windows0[i].flags = 0;	// 未使用マーク
		ctl->windows0[i].ctl = ctl;
	}
	
	return ctl;
}

struct WINDOW *window_alloc(struct WINDOWCTL *ctl) {
	struct WINDOW *window;
	int i;

	// 未使用のウィンドウバッファを探索
	for(i = 0; i < MAX_WINDOWS; ++i) {
		if (ctl->windows0[i].flags == WINDOW_FLAGS_UNUSED) {
			window = &(ctl->windows0[i]);
			window->flags = WINDOW_FLAGS_USED;	// 使用中マーク
			window->height = -1;			// 非表示
			window->task = 0;				// 自動で閉じる機能を使わない
			return window;
		}
	}

	return 0;
}

/**
 * ウィンドウオブジェクトに必要な情報をセットする（別にこれならいらなくない？）
 */
void window_setbuf(struct WINDOW *window, unsigned char *buf, int xsize, int ysize, int col_inv) {
	window->buf = buf;
	window->bxsize = xsize;
	window->bysize = ysize;
	window->col_inv = col_inv;
	return ;
}

/**
 * ウィンドウIDをメモしたバッファを更新
 */
void window_refreshmap(struct WINDOWCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
	unsigned char *buf, sid, *map = ctl->map;
	struct WINDOW *window;

	if (vx0 < 0) vx0 = 0;
	if (vy0 < 0) vy0 = 0;
	if (vx1 > ctl->xsize) vx1 = ctl->xsize;
	if (vy1 > ctl->ysize) vy1 = ctl->ysize;

	for (h = h0; h <= ctl->top; ++h) {
		window = ctl->windows[h];
		buf = window->buf;
		sid = window - ctl->windows0;	// 番地を引き算してそれをウィンドウの番号として使用する

		// vx0～vy1を使って、bx0～by1(ウィンドウ内部での座標)を逆算する
		bx0 = vx0 - window->vx0;
		by0 = vy0 - window->vy0;
		bx1 = vx1 - window->vx0;
		by1 = vy1 - window->vy0;
		if (bx0 < 0) bx0 = 0;
		if (by0 < 0) by0 = 0;
		if (bx1 > window->bxsize) bx1 = window->bxsize;
		if (by1 > window->bysize) by1 = window->bysize;

		/**
		 * 高速化のために透明部分があるウィンドウとないウィンドウでループを分けている
		 */
		if (window->col_inv == -1) {	// 透明部分がないループ
			if ((window->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 && 3) == 0) {	// 高速代入
				bx1 = (bx1 - bx0) / 4;
				sid4 = sid | sid << 8 | sid << 16 | sid << 24;
				for (by = by0; by < by1; ++by) {
					vy = window->vy0 + by;
					vx = window->vx0 + bx0;
					p = (int *) &map[vy * ctl->xsize + vx];
					for (bx = 0; bx < bx1; ++bx) {
						p[bx] = sid4;
					}
				}
			} else {	// 1バイトずつ代入する
				for (by = by0; by < by1; ++by) {
					vy = window->vy0 + by;
					for (bx = bx0; bx < bx1; ++bx) {
						vx = window->vx0 + bx;
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		} else {
			for (by = by0; by < by1; ++by) {
				vy = window->vy0 + by;
				for (bx = bx0; bx < bx1; ++bx) {
					vx = window->vx0 + bx;
					if (buf[by * window->bxsize + bx] != window->col_inv) {
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		}
		
	}
}

/**
 * vx0, vy0, vx1, vy1: 画面全体での描画更新矩形範囲
 * h0: 描き直し対象のウィンドウの高さの最小値（h0以上の高さのウィンドウが描き直される）
 */
void window_refreshsub(struct WINDOWCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
	int h;
	int bx, by;	// ウィンドウ内の相対座標を指すカウンター変数
	int vx, vy;	// 画面全体での座標を指すカウンター変数
	int bx0, by0, bx1, by1;	// forループ内で各ウィンドウの頂点を表す (bx0 <= x <= bx1, by0 <= y <= by1)
	int bx2, sid4, i, i1, *p, *q, *r;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct WINDOW *window;

	if (vx0 < 0) vx0 = 0;
	if (vy0 < 0) vy0 = 0;
	if (vx1 > ctl->xsize) vx1 = ctl->xsize;
	if (vy1 > ctl->ysize) vy1 = ctl->ysize;
	for (h = h0; h <= h1; ++h) {
		window = ctl->windows[h];
		buf = window->buf;
		sid = window - ctl->windows0;

		// vx0～vy1を使って、bx0～by1(ウィンドウ内部での座標)を逆算する
		bx0 = vx0 - window->vx0;
		by0 = vy0 - window->vy0;
		bx1 = vx1 - window->vx0;
		by1 = vy1 - window->vy0;
		if (bx0 < 0) bx0 = 0;
		if (by0 < 0) by0 = 0;
		if (bx1 > window->bxsize) bx1 = window->bxsize;
		if (by1 > window->bysize) by1 = window->bysize;

		if ((window->vx0 & 3) == 0) {	// 4バイト一括代入型
			i 	= (bx0 + 3) / 4;	// ceil( bx0 / 4 )
			i1 	= bx1 		/ 4;	// floor( bx1 / 4 )
			i1 	= i1 - i;
			sid4 = sid | sid << 8 | sid << 16 | sid << 24;
			
			for (by = by0; by < by1; ++by) {
				vy = window->vy0 + by;

				/* 前の端数を1バイトずつ代入する */
				for (bx = bx0; bx < bx1 && (bx & 3) != 0; ++bx) {
					vx = window->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * window->bxsize + bx];
					}
				}

				vx = window->vx0 + bx;
				p = (int *) &map[vy * ctl->xsize + vx];
				q = (int *) &vram[vy * ctl->xsize + vx];
				r = (int *) &buf[by * window->bxsize + bx];
				for (i = 0; i < i1; ++i) {
					if (p[i] == sid4) {	// ほとんどの場合でここの分岐になるため高速
						q[i] = r[i];
					} else {
						bx2 = bx + i * 4;
						vx = window->vx0 + bx2;
						if (map[vy * ctl->xsize + vx + 0] == sid) {
							vram[vy * ctl->xsize + vx + 0] = buf[by * window->bxsize + bx2 + 0];
						}
						if (map[vy * ctl->xsize + vx + 1] == sid) {
							vram[vy * ctl->xsize + vx + 1] = buf[by * window->bxsize + bx2 + 1];
						}
						if (map[vy * ctl->xsize + vx + 2] == sid) {
							vram[vy * ctl->xsize + vx + 2] = buf[by * window->bxsize + bx2 + 2];
						}
						if (map[vy * ctl->xsize + vx + 3] == sid) {
							vram[vy * ctl->xsize + vx + 3] = buf[by * window->bxsize + bx2 + 3];
						}
					}
				}

				/* 後ろの端数を1バイトずつ代入する */
				for (bx += i1 * 4; bx < bx1; ++bx) {
					vx = window->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * window->bxsize + bx];
					}
				}

			}
		} else {
			for (by = by0; by < by1; ++by) {
				vy = window->vy0 + by;
				for (bx = bx0; bx < bx1; ++bx) {
					vx = window->vx0 + bx;
					if (map[vy * ctl->xsize + vx] == sid) {
						vram[vy * ctl->xsize + vx] = buf[by * window->bxsize + bx];
					}
				}
			}
		}
	}
	return ;
}

/**
 * ウィンドウの高さを設定する
 */
void window_updown(struct WINDOW *window, int height) {
	int h, old = window->height;
	struct WINDOWCTL *ctl = window->ctl;

	// 指定した高さが高すぎる or 低すぎる場合は矯正する
	if (height > ctl->top + 1) height = ctl->top + 1;
	if (height < -1) height = -1;

	window->height = height;

	if (old > height) {
		if (height >= 0) {	// 表示される場合
			for (h = old; h > height; --h) {
				ctl->windows[h] = ctl->windows[h-1];
				ctl->windows[h]->height	= h;
			}
			ctl->windows[height] = window;
			window_refreshmap(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, height + 1);
			window_refreshsub(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, height + 1, old);
		} else {	// 非表示化
			if (ctl->top > old) {
				for (h = old; h < ctl->top; ++h) {
					ctl->windows[h] = ctl->windows[h+1];
					ctl->windows[h]->height = h;
				}
			}
			--(ctl->top);
			window_refreshmap(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, 0);
			window_refreshsub(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, 0, old - 1);
		}
	} else if (old < height) {	// 以前よりも高くなる
		if (old >= 0) {
			for (h = old; h < height; ++h) {
				ctl->windows[h] = ctl->windows[h+1];
				ctl->windows[h]->height = h;
			}
			ctl->windows[height] = window;
		} else { // old == -1
			for (h = ctl->top; h >= height; --h) {
				ctl->windows[h+1] = ctl->windows[h];
				ctl->windows[h+1]->height = h + 1;
			}
			ctl->windows[height] = window;
			++(ctl->top);
		}
		window_refreshmap(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, height);
		window_refreshsub(ctl, window->vx0, window->vy0, window->vx0 + window->bxsize, window->vy0 + window->bysize, height, height);
	}

	return ;
}

/**
 * ウィンドウ内の座標を指定し再描画
 */
void window_refresh(struct WINDOW *window, int bx0, int by0, int bx1, int by1) {
	struct WINDOWCTL *ctl = window->ctl;
	if (window->height >= 0) {
		window_refreshsub(
			ctl, 
			window->vx0 + bx0,
			window->vy0 + by0,
			window->vx0 + bx1,
			window->vy0 + by1,
			window->height,
			window->height
		);
	}
}

/**
 * ウィンドウを移動させる
 */
void window_slide(struct WINDOW *window, int vx0, int vy0) {
	struct WINDOWCTL *ctl = window->ctl;
	int old_vx0 = window->vx0, old_vy0 = window->vy0;
	window->vx0 = vx0;
	window->vy0 = vy0;
	if (window->height >= 0) {
		window_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + window->bxsize, old_vy0 + window->bysize, 0);
		window_refreshmap(ctl, vx0, vy0, vx0 + window->bxsize, vy0 + window->bysize, window->height);
		window_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + window->bxsize, old_vy0 + window->bysize, 0, window->height - 1);
		window_refreshsub(ctl, vx0, vy0, vx0 + window->bxsize, vy0 + window->bysize, window->height, window->height);
	}
	return;
}

/**
 * 使わないウィンドウを解放する
 */
void window_free(struct WINDOW *window) {
	if (window->height >= 0) window_updown(window, -1);
	window->flags = WINDOW_FLAGS_UNUSED;
	return;
}


void make_textbox8(struct WINDOW *window, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(window->buf, window->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(window->buf, window->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(window->buf, window->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(window->buf, window->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(window->buf, window->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(window->buf, window->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(window->buf, window->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(window->buf, window->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(window->buf, window->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
	return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act) {
	static char closebtn[14][16] = {
			"OOOOOOOOOOOOOOO@",
			"OQQQQQQQQQQQQQ$@",
			"OQQQQQQQQQQQQQ$@",
			"OQQQ@@QQQQ@@QQ$@",
			"OQQQQ@@QQ@@QQQ$@",
			"OQQQQQ@@@@QQQQ$@",
			"OQQQQQQ@@QQQQQ$@",
			"OQQQQQ@@@@QQQQ$@",
			"OQQQQ@@QQ@@QQQ$@",
			"OQQQ@@QQQQ@@QQ$@",
			"OQQQQQQQQQQQQQ$@",
			"OQQQQQQQQQQQQQ$@",
			"O$$$$$$$$$$$$$$@",
			"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c, tc, tbc;
	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	} else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
					c = COL8_000000;
			} else if (c == '$') {
					c = COL8_848484;
			} else if (c == 'Q') {
					c = COL8_C6C6C6;
			} else {
					c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

/**
 * windowのx, yを左上原点にして文字を入力する
 */
void putfonts8_asc_window(struct WINDOW *window, int x, int y, int fontcolor, int bcolor, char *s, int length) {
	struct TASK *task = task_now();
	boxfill8(window->buf, window->bxsize, bcolor, x, y, x + length * 8 - 1, y + 15);
	if (task->langmode != 0 && task->langbyte1 != 0) {
		putfonts8_asc(window->buf, window->bxsize, x, y, fontcolor, s);
		window_refresh(window, x - 8, y, x + length * 8, y + 16);
	} else {
		putfonts8_asc(window->buf, window->bxsize, x, y, fontcolor, s);
		window_refresh(window, x, y, x + length * 8, y + 16);
	}

	return;
}

/**
 * windowのタイトルの色を変える
 */
void change_wtitle8(struct WINDOW *window, char act) {
	int x, y, xsize = window->bxsize;
	char c, tc_new, tbc_new, tc_old, tbc_old, *buf = window->buf;

	if (act != 0) {
		tc_new = COL8_FFFFFF;
		tbc_new = COL8_000084;
		tc_old = COL8_C6C6C6;
		tbc_old = COL8_848484;
	} else {
		tc_new = COL8_C6C6C6;
		tbc_new = COL8_848484;
		tc_old = COL8_FFFFFF;
		tbc_old = COL8_000084;
	}

	for (y = 3; y <= 20; ++y) {
		for (x = 3; x <= xsize - 4; ++x) {
			c = buf[y * xsize + x];
			
			if (c == tc_old && x <= xsize - 30) {
				c = tc_new;
			} else if (c == tbc_old) {
				c = tbc_new;
			}
			
			buf[y * xsize + x] = c;
		}
	}
	window_refresh(window, 3, 3, xsize, 21);
	return;
}
