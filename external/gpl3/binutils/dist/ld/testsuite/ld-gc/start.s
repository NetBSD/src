.globl _start
_start:
	.long __start__foo
	.section	_foo,"aw",%progbits
foo:
	.long	1
