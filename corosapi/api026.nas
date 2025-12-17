[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api026.nas"]

		GLOBAL	_api_cmdline		; void api_cmdline(char *buf, maxsize);

[SECTION .text]

_api_cmdline:		; void api_cmdline(char *buf, int maxsize);
	PUSH	EBX
	MOV		EDX,26
	MOV		EBX,[ESP+8]		; buf
	MOV		ECX,[ESP+12]	; maxsize
	INT		0x40
	POP		EBX
	RET
