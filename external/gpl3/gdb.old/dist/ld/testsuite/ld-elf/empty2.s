	.file "empty2.s"
	.equiv constant, 0x12345678
	.section .bss
bar:
	.text
	.type start,"function"
	.global start
start:
	.type _start,"function"
	.global _start
_start:
	.type __start,"function"
	.global __start
__start:
	.type main,"function"
	.global main
main:
	.type _main,"function"
	.global _main
_main:
	.long constant
