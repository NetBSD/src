nop
	.section	.text.foo,"axG",@progbits,foo,comdat
foo:
	ret

	.section	.text.bar,"axG",@progbits,bar,comdat
	.align 2
bar:
	ret

	.section	.text._start,"ax",@progbits
	.globl	_start
_start:
	leaq	bar(%rip), %rsi
	testb	$1, %sil
	je	.L9
	mov $1, %eax
	mov $1, %ebx
	int $0x80
.L9:
	mov $1, %eax
	mov $0, %ebx
	int $0x80
