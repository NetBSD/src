        .text
	.globl	_start
_start:
        .word   datavar
        .word   rodatavar

        .section ".rodata", "a"
rodatavar:
        .word 0

        .section ".data", "aw"
datavar:
        .word 0
