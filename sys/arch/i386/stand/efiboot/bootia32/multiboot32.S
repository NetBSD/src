/*	$NetBSD: multiboot32.S,v 1.2 2019/09/24 00:47:46 manu Exp $ */

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
 * multiboot32(entry 4(%esp), multiboot2_info 8(%esp), magic 12(%esp))
 */
ENTRY(multiboot32_start)
start:
        movl    12(%esp),%eax
        movl    8(%esp),%ebx
        movl    4(%esp),%edx
        jmp    *%edx
multiboot32_end:
