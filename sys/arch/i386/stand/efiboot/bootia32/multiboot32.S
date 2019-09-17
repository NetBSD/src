/*	$NetBSD: multiboot32.S,v 1.1.2.2 2019/09/17 19:32:00 martin Exp $ */

#include <machine/asm.h>
#include <machine/specialreg.h>

	.align	16
	.globl _C_LABEL(multiboot32)
_C_LABEL(multiboot32):
	.quad 0

	.globl _C_LABEL(multiboot32_size)
_C_LABEL(multiboot32_size):
	.long multiboot32_end - _C_LABEL(multiboot32_start)

	.text
	.p2align 4,,15

/*
 * multiboot32(entry 8(%esp), multiboot2_info 12(%esp), magic 16(%esp))
 */
ENTRY(multiboot32_start)
start:
        movl    16(%esp),%eax
        movl    12(%esp),%ebx
        movl    8(%esp),%edx
        jmp    *%edx
multiboot32_end:
