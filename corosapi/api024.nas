[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api024.nas"]

		GLOBAL	_api_fsize		; void api_fsize(int fhandle, int mode);

[SECTION .text]

_api_fsize:		; void api_fsize(int fhandle, int mode);
	MOV		EDX,24
	MOV		EAX,[ESP+4]		; fhandle
	MOV		ECX,[ESP+8]		; mode
	INT		0x40
	RET
