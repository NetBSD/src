	.text
	.align	4
	.globl	_start
_start:
	nop

	.section .merge1,"aMS",@progbits,1
A:	.string	"utter"

	.section .merge2,"aMS",@progbits,1
B:	.string "tasting"

	.section .merge3,"aM",@progbits,4
C:	.4byte	0x100
D:	.4byte	0x200

	.data
	.align	4
E:	.4byte	E
	.4byte	E + 0x1000
	.4byte	A
	.4byte	B
	.4byte	C
	.4byte	D
