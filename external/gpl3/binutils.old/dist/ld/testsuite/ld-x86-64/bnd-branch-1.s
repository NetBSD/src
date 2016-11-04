	.text
	.globl _start
_start:
bnd	jmp	foo1@plt
	call	foo2@plt
	jmp	foo3@plt
	call	foo4@plt
bnd	call	foo3@plt
	jmp	foo4@plt
