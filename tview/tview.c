#include <stdio.h>
#include <string.h>
#include "roastd.h"

#define INVALID		-0x7fffffff

int strtol(char *s, char **endp, int base);

char *skipspace(char *p);
char *skipchar2space(char *p);
int getnum(char **pp, int priority);
int parse_cmdline(char *cmdline, char *filename, int *w, int *h, int *t);
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang);
char *lineview(int win, int w, int y, int xskip, char *p, int tab, int lang);
int puttab(int x, int w, int xskip, char *s, int tab);
int ascii_char(char **p, int x, int w, int xskip, char *s, int tab);
int sjis_char(char **p, int x, int w, int xskip, char *s, int tab);
int euc_char(char **p, int x, int w, int xskip, char *s, int tab);

void RoaMain(void) {
	char winbuf[1024 * 757], txtbuf[240 * 1024];
	int w = 30, h = 10, t = 4, spd_x = 1, spd_y = 1;
	int win, i, j, lang = api_getlang(), xskip = 0;
	char s[30], *p, *q = 0;
	char filename[30];

	/* コマンドライン解析 */
	api_cmdline(s, 30);
	if (parse_cmdline(s, filename, &w, &h, &t) == -1) {
		api_putstr(" >tview file [-w30 -h10 -t4]\n");
		api_end();
	}

	/* ウィンドウの準備 */
	win = api_openwin(winbuf, w * 8 + 16, h * 16 + 37, -1, "tview");
	api_boxfillwin(win, 6, 27, w * 8 + 9, h * 16 + 30, 7);

	/* ファイルの読み込み */
	i = api_fopen(filename);
	if (i == 0) {
		api_putstr("file open error.\n");
		api_end();
	}
	j = api_fsize(i, 0);

	if (j >= 240 * 1024 - 1) j = 240 * 1024 - 2;
	txtbuf[0] = 0x0a;	// 番兵用の改行コード
	api_fread(txtbuf + 1, j, i);
	api_fclose(i);
	txtbuf[j+1] = 0;

	q = txtbuf + 1;
	
	for (p = txtbuf + 1; *p != 0; ++p) {
		if (*p != 0x0d) {
			*q = *p;
			++q;
		}
	}
	*q = 0;

	/* メイン */
	p = txtbuf + 1;
	while (1) {
		textview(win, w, h, xskip, p, t, lang);
		i = api_getkey(1);
		if (i == 'Q' || i == 'q') api_end();
		if ('A' <= i && i <= 'F') spd_x = 1 << (i - 'A');
		if ('a' <= i && i <= 'f') spd_y = 1 << (i - 'a');
		if (i == '<' && t > 1) t /= 2;
		if (i == '>' && t < 256) t *= 2;

		if (i == '4') {
			while (1) {
				xskip -= spd_x;
				if (xskip < 0) xskip = 0;
				if (api_getkey(0) != '4') break;
			}
		}
		if (i == '6') {
			while (1) {
				xskip += spd_x;
				if (api_getkey(0) != '6') break;
			}
		}

		if (i == '8') {
			while (1) {
				for (j = 0; j < spd_y; ++j) {
					if (p == txtbuf + 1) break;	// ファイル先頭に来たらそれ以上は進まない
					for (--p; p[-1] != 0x0a; --p);	// 1文字前前に0x0aがでるまでさかのぼる
				}
				if (api_getkey(0) != '8') break;
			}
		}

		if (i == '2') {
			while (1) {
				for (j = 0; j < spd_y; ++j) {
					for (q = p; *q != 0 && *q != 0x0a; ++q);
					if (*q == 0) break;
					p = q + 1;
				}
				if (api_getkey(0) != '2') break;
			}
		}
	}

	exit(1);
}

/**
 * スペース文字を読み飛ばす
 */
char *skipspace(char *p) {
	while (*p == ' ') ++p;
	return p;
}

/**
 * スペースが来るまで文字を読み飛ばす
 */
char *skipchar2space(char *p) {
	while (*p > ' ') ++p;
	return p;
}

/**
 * コマンドラインの文字列cmdlineを解析し、w, h, tにオプションの値を、filenameにファイル名を記録する
 * cmdline: コマンドライン文字列の先頭アドレス 
 * filename: 
 * w: コマンドラインで指定された幅を記録するメモリのアドレス
 * h: コマンドラインで指定された高さを記録するメモリのアドレス
 * t: タブ幅
 * 戻り値: コマンドが正常なら0、コマンドに誤りがあれば-1
 */
int parse_cmdline(char *cmdline, char *filename, int *w, int *h, int *t) {
	char *p = cmdline;
	char *q = 0;

	p = skipchar2space(p);	// スペースが来るまで読み飛ばす
	while (*p != 0) {
		p = skipspace(p);
		if (*p == '-') {
			if (p[1] == 'w') {
				*w = strtol(p + 2, &p, 0);
				if (*w < 20) *w = 20;
				if (*w > 126) *w = 126;
			} else if (p[1] == 'h') {
				*h = strtol(p + 2, &p, 0);
				if (*h < 1) *h = 1;
				if (*h > 45) *h = 45;
			} else if (p[1] == 't') {
				*t = strtol(p + 2, &p, 0);
				if (*t < 1) *t = 4;
			} else {
				return -1;
			}
		} else {	// ファイル名
			if (q != 0) return -1;
			q = p;
			p = skipchar2space(p);
			strncpy(filename, q, (p-q));
			*(filename + (q-p)) = 0x00;
		}
	}

	if (q == 0) return -1;

	return 0;
}

/**
 * p: テキストバッファの位置
 */
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang) {
	int i;
	api_boxfillwin(win, 8, 29, w * 8 + 7, h *  16 + 28, 7);
	for (i = 0; i < h; ++i) {
		p = lineview(win, w, i * 16 + 29, xskip, p, tab, lang);
	}
	api_refreshwin(win, 8, 29, w * 8 + 8, h *  16 + 29);
	return;
}

/**
 * 
 */
char *lineview(int win, int w, int y, int xskip, char *p, int tab, int lang) {
	int x = - xskip;
	char s[130];
	while (1) {
		if (*p == 0) break;
		if (*p == 0x0a) {
			++p;
			break;
		}

		// ASCII
		if (lang == 0) x = ascii_char(&p, x, w, xskip, s, tab);
		
		// SJIS
		if (lang == 1) x = sjis_char(&p, x, w, xskip, s, tab);

		// EUC
		if (lang == 2) x = euc_char(&p, x, w, xskip, s, tab);
	}

	if (x > w) x = w;
	if (x > 0) {
		s[x] = 0;
		api_putstrwin(win + 1, 8, y, 0, x, s);
	}

	return p;
}

/**
 * 
 */
int puttab(int x, int w, int xskip, char *s, int tab) {
	while (1) {
		if (0 <= x && x < w) s[x] = ' ';
		++x;
		if ((x + xskip) % tab == 0) break;
	}
	return x;
}


int ascii_char(char **p, int x, int w, int xskip, char *s, int tab) {
	if (**p == 0x09) {
		x = puttab(x, w, xskip, s, tab);
	} else {
		if (0 <= x && x < w) s[x] = **p;
		++x;
	}
	++*p;
	return x;
}

int sjis_char(char **p, int x, int w, int xskip, char *s, int tab) {
	if (**p == 0x09) {
		x = puttab(x, w, xskip, s, tab);
		++*p;
	} else if ((0x81 <= (**p) && (**p) <= 0x9f) || (0xe0 <= (**p) && (**p) <= 0xfc)) { // 全角文字
		if (x == -1) s[0] = '0';
		if (0 <= x && x < w - 1) {
			s[x] = **p;
			s[x + 1] = (*p)[1];
		}
		if (x == w - 1) s[x] = ' ';

		x += 2;
		*p += 2;
	} else {
		if (0 <= x && x < w) s[x] = **p;
		++x;
		++*p;
	}

	return x;
}

int euc_char(char **p, int x, int w, int xskip, char *s, int tab) {
	if (**p == 0x09) {
		x = puttab(x, w, xskip, s, tab);
		++*p;
	} else if (0xa1 <= (**p) && (**p) <= 0xfe) {	// 全角文字
		if (x == -1) s[0] = ' ';
		if (0 <= x && x < w - 1) {
			s[x] = **p;
			s[x + 1] = (*p)[1];
		}
		if (x == w - 1) s[x] = ' ';
		x += 2;
		*p += 2;
	} else {
		if (0 <= x && x < w) s[x] = **p;
		++x;
		++*p;
	}

	return x;
}