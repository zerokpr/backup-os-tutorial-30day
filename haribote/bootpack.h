#define RoaMain HariMain

/* asmhead.nas */
struct BOOTINFO {
	char cyls; // ブートセクタはどこまでディスクを読んだのか
	char leds; // ブート時のキーボードのLEDの状態
	char vmode;	// ビデオモード 何ビットカラーか
	char reserve;
	short scrnx, scrny; // 画面解像度
	char *vram;
};
#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG 0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt();
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
int load_cr0(void);
void store_cr0(int cr0);
void write_mem8(int addr, int data);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void load_tr(int tr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_cons_putchar(void);
void asm_hrb_api(void);
void farjmp(int eip, int cs);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

/* graphic.c */
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int xsize, int ysize);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);

/* dsctbl.c */
void init_gdtidt(void);

struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT			0x0082
#define	AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1


#define PORT_KEYDAT 	0x0060

/* fifo.c */
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* keyboard.c */
#define PORT_KEYDAT 	0x0060
#define PORT_KEYSTA		0x0064
#define PORT_KEYCMD 	0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE	0x60
#define KBC_MODE		0x47

void init_keyboard(struct FIFO32 *fifo, int data0);
void wait_KBC_sendready(void);

/* mouse.c */
#define KEYCMD_SENDTO_MOUSE 	0xd4
#define MOUSECMD_ENABLE			0xf4
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);


/* memory.c */
unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

#define MEMMAN_ADDR		0x003c0000
#define MEMMAN_FREES	4090

struct FREEINFO {
	unsigned int addr, size;
};

struct MEMMAN {
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO	free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* window.c */

/**
 * ウィンドウを表す構造体
 */
struct WINDOW {
	unsigned char *buf;	// ウィンドウの画素データ
	int bxsize, bysize; // ウィンドウの大きさ（bxsize * bysize）
	int vx0, vy0;		// 画面上のどこにあるか
	int col_inv;		// 透明色番号
	int	height;			// ウィンドウの高さ
	int flags;			// ウィンドウに関する様々な設定情報
	struct WINDOWCTL *ctl;	// ウィンドウマネージャへのポインタ
	struct TASK* task;	// ウィンドウを持っているタスクへの参照。taskが終了される場合はウィンドウも閉じる
};

#define WINDOW_FLAGS_UNUSED		0
#define WINDOW_FLAGS_USED		1
#define MAX_WINDOWS		256

/**
 * ウィンドウを管理する構造体
 */
struct WINDOWCTL {
	unsigned char* vram;					// vramの先頭アドレス
	unsigned char* map;						// ダブルバッファリング用
	int xsize, ysize, top;					// 画面サイズ（BOOTINFOから取ってくるの面倒なのでここに保存しておく）
	struct WINDOW *windows[MAX_WINDOWS];	// ウィンドウ構造体へのポインタを描画順の大きい順に並べておく。非表示(height == -1)のウィンドウは含めない（画家のアルゴリズム用）
	struct WINDOW windows0[MAX_WINDOWS];	// ウィンドウの情報を実際に保存する配列
};

struct WINDOWCTL *windowctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct WINDOW *window_alloc(struct WINDOWCTL *ctl);
void window_setbuf(struct WINDOW *window, unsigned char *buf, int xsize, int ysize, int col_inv);
void window_updown(struct WINDOW *window, int height);
void window_refresh(struct WINDOW *window, int bx0, int by0, int bx1, int by1);
void window_slide(struct WINDOW *window, int vx0, int vy0);
void window_free(struct WINDOW *window);

void make_textbox8(struct WINDOW *window, int x0, int y0, int sx, int sy, int c);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void putfonts8_asc_window(struct WINDOW *window, int x, int y, int c, int b, char *s, int l);
void change_wtitle8(struct WINDOW *window, char act);

/* timer.c */
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);

#define MAX_TIMER	500

struct TIMER {
	struct TIMER *next;
	unsigned int timeout, flags;
	char should_be_canceled;	// キャンセルされるべきかのフラグ
	struct FIFO32 *fifo;
	int data;
};

struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;	// 先頭
	struct TIMER timers0[MAX_TIMER];
};

/* mttask.c */
#define MAX_TASK_PER_LV		100
#define MAX_TASKLEVELS	10
#define MAX_TASKS		1000	// 最大タスク数
#define TASK_GDT0		3		// TSSをGDTの何番から割り当てるか

#define TASK_FLAGS_UNUSED		0	// 未使用
#define TASK_FLAGS_ALLOCATED	1	// 確保済み
#define TASK_FLAGS_RUNNING		2	// 動作中

struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};

struct TASK {
	int selector, flags;	// selectorはGDTの番号
	int level;				// 
	int priority;			// タスクの優先度
	struct FIFO32 fifo;		// タスク
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];	// ローカルディスクリプタテーブル保存用
	struct CONSOLE *cons;
	int ds_base;
	int cons_stack;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	unsigned char langmode;
	unsigned char langbyte1;
};

struct TASKLEVEL {
	int running;	// 動作しているタスク数
	int now;		// 現在動作しているタスクの番号
	struct TASK *tasks[MAX_TASK_PER_LV];
};

struct TASKCTL {
	int now_lv;		// 現在動作しているレベル
	char lv_change;	// 次回タスクスイッチのときに、レベルを変えるべきかどうか
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
struct TASK *task_now(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);

extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;


/**
 * ファイルハンドル
 */
struct FILEHANDLE {
	char *buf;
	int size;
	int pos;
};

/* file.c */
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
char *file_loadfile2(int clustno, int *psize, int *fat);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);

/* console.c */
struct CONSOLE {
	struct WINDOW *window;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer;
};
void cons_newline(struct CONSOLE *cons);
void console_task_func(struct WINDOW *window, unsigned int memtotal);
void cons_putstr(struct CONSOLE *cons, char *s);
void cons_putstrn(struct CONSOLE *cons, char *s, int n);
int *inthandler0d(int *esp);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */
struct TASK *open_constask(struct WINDOW *window, unsigned int memtotal);
struct WINDOW *open_console(struct WINDOWCTL *winctl, unsigned int memtotal);
void close_constask(struct TASK *task);
void close_console(struct WINDOW *window);
void keywin_off(struct WINDOW *key_win);
void keywin_on(struct WINDOW *key_win);
void kill_console_task(struct TASK *task);

inline unsigned int is_keyboard_data(int data);
inline unsigned int is_mouse_data(int data);
inline unsigned int is_cons_termination_req(int data);
inline unsigned int is_nwcons_termination_req(int data);

/* jpeg.c */
struct DLL_STRPICENV {	/* 64KB */
	int work[64 * 1024 / 4];
};

struct RGB {
	unsigned char b, g, r, t;
};

int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);