[FORMAT "WCOFF"]	; オブジェクトファイル
[INSTRSET "i486p"]
[BITS 32]
[FILE "a_nask.nas"]	; ソースファイル名情報

	GLOBAL	_api_putchar		; void api_putchar(int c);
	GLOBAL	_api_end			; void api_end(void);
	GLOBAL	_api_openwin		; int api_openwin(char* buf, int xsiz, int ysiz, int col_inv, char *title);
	GLOBAL	_api_putstrwin		; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
	GLOBAL	_api_boxfillwin		; void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
	GLOBAL	_api_initmalloc		; void api_initmalloc(void);
	GLOBAL	_api_malloc			; char *api_malloc(int size);
	GLOBAL	_api_free			; void api_free(char *addr, int size);
	GLOBAL  _api_point			; void api_point(int win, int x, int y, int col);
	GLOBAL	_api_refreshwin		; void api_refreshwin(int win, int x0, int y0, int x1, int y1);
	GLOBAL	_api_linewin		; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
	GLOBAL	_api_closewin		; void api_closewin(int win);
	GLOBAL	_api_getkey			; int api_getkey(int mode);
	GLOBAL	_api_alloctimer		; int api_alloctimer(void);
	GLOBAL	_api_inittimer		; void api_inittimer(int timeraddr, int data);
	GLOBAL	_api_settimer		; void api_settimer(int timeraddr, int time);
	GLOBAL	_api_freetimer		; void api_freetimer(int timeraddr);
	GLOBAL	_api_beep			; void api_beep(int tone);

[SECTION .text]

_api_putchar:	; void api_putchar(int c);
	MOV		EDX,1
	MOV		AL,[ESP+4]	; AL = c;
	INT		0x40
	RET

_api_end:	; void api_end(void);
	MOV		EDX,4
	INT		0x40

_api_openwin:	; int api_openwin(char* buf, int xsiz, int ysiz, int col_inv, char *title);
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX,5
	MOV		EBX,[ESP+16]	; buf
	MOV		ESI,[ESP+20]	; xsiz
	MOV		EDI,[ESP+24]	; ysiz
	MOV		EAX,[ESP+28]	; col_inv
	MOV		ECX,[ESP+32]	; title
	INT		0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET

_api_putstrwin:		; void api_putstrwin(int win, int x, int y, int col, int len, char *str);
	PUSH	EDI
	PUSH	ESI
	PUSH	EBP
	PUSH	EBX
	MOV		EDX,6
	MOV		EBX,[ESP+20]	; win
	MOV		ESI,[ESP+24]	; x
	MOV		EDI,[ESP+28]	; y
	MOV		EAX,[ESP+32]	; col
	MOV		ECX,[ESP+36]	; len
	MOV		EBP,[ESP+40]	; str
	INT		0x40
	POP		EBX
	POP		EBP
	POP		ESI
	POP		EDI
	RET

_api_boxfillwin:		; void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
	PUSH	EDI
	PUSH	ESI
	PUSH	EBP
	PUSH	EBX
	MOV		EDX,7
	MOV		EBX,[ESP+20]	; win
	MOV		EAX,[ESP+24]	; x0
	MOV		ECX,[ESP+28]	; y0
	MOV		ESI,[ESP+32]	; x1
	MOV		EDI,[ESP+36]	; y1
	MOV		EBP,[ESP+40]	; col
	INT		0x40
	POP		EBX
	POP		EBP
	POP		ESI
	POP		EDI
	RET

_api_initmalloc:	; void api_initmalloc(void);
	PUSH	EBX
	MOV		EDX,8
	MOV		EBX,[CS:0x0020]		; hrbファイルではmalloc領域の番地が0x20に入っている
	MOV		EAX,EBX
	ADD		EAX,32*1024
	MOV		ECX,[CS:0x0000]
	SUB		ECX,EAX
	INT		0x40
	POP		EBX
	RET

_api_malloc:		; char *api_malloc(int size);
	PUSH	EBX
	MOV		EDX,9
	MOV		EBX,[CS:0x0020]		; hrbファイルではmalloc領域の番地が0x20に入っている
	MOV		ECX,[ESP+8]
	INT		0x40
	POP		EBX
	RET

_api_free:			; void api_free(char *addr, int size);
	PUSH	EBX
	MOV		EDX,10
	MOV		EBX,[CS:0x0020]		; hrbファイルではmalloc領域の番地が0x20に入っている
	MOV		EAX,[ESP+8]
	MOV		ECX,[ESP+12]
	INT		0x40
	POP		EBX
	RET

_api_point:			; void api_point(int win, int x, int y, int col);
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX,11
	MOV		EBX,[ESP+16]		; win
	MOV		ESI,[ESP+20]		; x
	MOV 	EDI,[ESP+24]		; y
	MOV		EAX,[ESP+28]		; col
	INT		0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET

_api_refreshwin:	; void api_refreshwin(int win, int x0, int y0, int x1, int y1);
	PUSH	EDX
	PUSH	EBX
	PUSH	EAX
	PUSH	ECX
	PUSH	ESI
	PUSH	EDI
	MOV		EDX,12
	MOV		EBX,[ESP+28]		; win
	MOV		EAX,[ESP+32]		; x0
	MOV		ECX,[ESP+36]		; y0
	MOV		ESI,[ESP+40]		; x1
	MOV		EDI,[ESP+44]		; y1
	INT		0x40
	POP		EDI
	POP		ESI
	POP		ECX
	POP		EAX
	POP		EBX
	POP		EDI
	RET

_api_linewin:		; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
	PUSH	EDX
	PUSH	EBX
	PUSH	EAX
	PUSH	ECX
	PUSH	ESI
	PUSH	EDI
	PUSH	EBP
	MOV		EDX,13
	MOV		EBX,[ESP+32]		; win
	MOV		EAX,[ESP+36]		; x0
	MOV		ECX,[ESP+40]		; y0
	MOV		ESI,[ESP+44]		; x1
	MOV		EDI,[ESP+48]		; y1
	MOV		EBP,[ESP+52]		; col
	INT		0x40
	POP		EBP
	POP		EDI
	POP		ESI
	POP		ECX
	POP		EAX
	POP		EBX
	POP		EDX
	RET

_api_closewin:		; void api_closewin(int win);
	PUSH	EBX
	MOV		EDX,14
	MOV		EBX,[ESP+8]		; win
	INT		0x40
	POP		EBX
	RET

_api_getkey:		; int api_getkey(int mode);
	MOV		EDX,15
	MOV		EAX,[ESP+4]		; mode
	INT		0x40
	RET

_api_alloctimer:	; int api_alloctimer(void);
	MOV		EDX,16
	INT		0x40
	RET

_api_inittimer:		; void api_inittimer(int timeraddr, int data);
	PUSH	EBX
	MOV		EDX,17
	MOV		EBX,[ESP+8]		; timeraddr
	MOV		EAX,[ESP+12]	; data
	INT		0x40
	POP		EBX
	RET

_api_settimer:		; void api_settimer(int timeraddr, int time);
	PUSH	EBX
	MOV		EDX,18
	MOV		EBX,[ESP+8]
	MOV		EAX,[ESP+12]
	INT		0x40
	POP		EBX
	RET

_api_freetimer:		; void api_freetimer(int timeraddr);
	PUSH	EBX
	MOV		EDX,19
	MOV		EBX,[ESP+8]
	INT		0x40
	POP		EBX
	RET

_api_beep:			; void api_beep(int tone);
	MOV		EDX,20
	MOV		EAX,[ESP+4]
	INT		0x40
	RET
