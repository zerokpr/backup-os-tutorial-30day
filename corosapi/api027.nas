[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api027.nas"]

		GLOBAL	_api_getlang		; void api_getlang(void);

[SECTION .text]

_api_getlang:		; void api_getlang(void);
	MOV		EDX,27
	INT		0x40
	RET
