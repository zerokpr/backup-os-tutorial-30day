#define RoaMain HariMain

/**
 * 
 */
void api_putchar(int c);

/**
 * 
 */
void api_putstr(char *buf);

/**
 * 
 */
void api_putnstr(char *buf, int n);

/**
 * hariapi終了関数
 * 呼び出さないとOS全体が落ちるので必ず呼び出す
 */
void api_end(void);

/**
 * 
 */
int api_openwin(char* buf, int xsiz, int ysiz, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);

/**
 * mallocを初期化する
 */
void api_initmalloc(void);

/**
 * 
 */
char *api_malloc(int size);

/**
 * addr番地からsizeバイトのメモリを解放する
 */
void api_free(char *addr, int size);

/**
 * 
 */
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_closewin(int win);
int api_getkey(int mode);
int api_alloctimer(void);
void api_inittimer(int timeraddr, int data);
void api_settimer(int timeraddr, int time);
void api_freetimer(int timeraddr);
void api_beep(int tone);

/**
 * 
 */
int api_fopen(char *fname);

/**
 * 
 */
void api_fclose(int fhandle);

/**
 * 
 */
void api_fseek(int fhandle, int offset, int mode);

/**
 * ファイルのサイズを読み取る
 */
int api_fsize(int fhandle, int mode);

/**
 * ファイルのデータを読み出す
 */
int api_fread(char *buf, int maxsize, int fhandle);


/**
 * コマンドライン引数を取得する
 */
int api_cmdline(char *buf, int maxsize);

