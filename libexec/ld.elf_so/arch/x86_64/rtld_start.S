/*	$NetBSD: rtld_start.S,v 1.1 2001/06/19 01:11:44 fvdl Exp $	*/

#include <machine/asm.h>

	.text
	.align	16
	.globl	.rtld_start
	.type	.rtld_start,@function
.rtld_start:
	subq	$16,%rsp		# make room of obj_main and exit proc
	movq	%rsp,%rdi		# stack pointer arg to _rtld
	pushq	%rbx			# save ps_strings

	call	_rtld@PLT		# _rtld(sp)

	popq	%rbx			# %rbx = ps_strings
	popq	%rdx			# %rdx = cleanup
	popq	%rcx			# %rcx = obj_main
	jmp	*%rax

	.align	4
	.globl	_rtld_bind_start
	.type	_rtld_bind_start,@function
_rtld_bind_start:	# (obj, reloff)
	pushfq				# save caller-saved registers
	pushq	%rax
	pushq	%rcx
	pushq	%rdx
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11

	call	_rtld_bind@PLT		# call the binder
	movq	%rax,80(%rsp)		# store function in return address

	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdi
	popq	%rsi
	popq	%rdx
	popq	%rcx
	popq	%rax
	popfq

	ret
