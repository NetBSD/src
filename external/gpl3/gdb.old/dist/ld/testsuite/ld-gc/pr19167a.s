.globl _start
_start:
	.dc.a	__start__foo
	.section	_foo,"aw",%progbits
foo:
	.ascii "This is "
