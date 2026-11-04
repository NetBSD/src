/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file is compiled from gdb.arch/amd64-extended-prologue-analysis.c
   using gcc 11.4.0 with flags: -g0 -O1 -S -fomit-frame-pointer
   -fno-asynchronous-unwind-tables.  */

	.file	"amd64-extended-prologue-analysis.c"
	.text
	.globl	bar
	.type	bar, @function
bar:
	endbr64
	leal	(%rdi,%rdi), %eax
	ret
	.size	bar, .-bar
	.globl	foo
	.type	foo, @function
foo:
	endbr64
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbp
	pushq	%rbx
	subq	$272, %rsp
	movl	%edi, %ebx
	movl	%esi, %r13d
	movl	%edx, %r12d
	movl	%ecx, %ebp
	movq	%fs:40, %rax
	movq	%rax, 264(%rsp)
	xorl	%eax, %eax
.L3:
	leal	(%rbx,%rax), %ecx
	movslq	%eax, %rdx
	movb	%cl, (%rsp,%rdx)
	addl	$1, %eax
	cmpl	$256, %eax
	jne	.L3
	movl	%ebx, %edi
	call	bar
	movl	%eax, %r14d
	movl	%r13d, %edi
	call	bar
	leal	(%r14,%rax), %r13d
	movl	%r12d, %edi
	call	bar
	addl	%eax, %r13d
	movl	%ebp, %edi
	call	bar
	addl	%r13d, %eax
	addl	%ebx, %eax
	movq	264(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L7
	addq	$272, %rsp
	popq	%rbx
	popq	%rbp
	popq	%r12
	popq	%r13
	popq	%r14
	ret
.L7:
	call	__stack_chk_fail@PLT
	.size	foo, .-foo
	.globl	main
	.type	main, @function
main:
	endbr64
	subq	$24, %rsp
	leal	(%rdi,%rdi), %ecx
	leal	2(%rdi), %edx
	leal	1(%rdi), %esi
	call	foo
	movl	%eax, 12(%rsp)
	movl	12(%rsp), %eax
	addq	$24, %rsp
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
