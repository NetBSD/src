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
   using gcc 11.4.0 with flags: -g0 -O1 -S -fno-omit-frame-pointer.  */

	.file	"amd64-extended-prologue-analysis.c"
	.text
	.globl	bar
	.type	bar, @function
bar:
.LFB0:
	.cfi_startproc
	endbr64
	leal	(%rdi,%rdi), %eax
	ret
	.cfi_endproc
.LFE0:
	.size	bar, .-bar
	.globl	foo
	.type	foo, @function
foo:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$280, %rsp
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
	movl	%edi, %ebx
	movl	%esi, %r14d
	movl	%edx, %r13d
	movl	%ecx, %r12d
	movq	%fs:40, %rax
	movq	%rax, -56(%rbp)
	xorl	%eax, %eax
.L3:
	leal	(%rbx,%rax), %ecx
	movslq	%eax, %rdx
	movb	%cl, -320(%rbp,%rdx)
	addl	$1, %eax
	cmpl	$256, %eax
	jne	.L3
	movl	%ebx, %edi
	call	bar
	movl	%eax, %r15d
	movl	%r14d, %edi
	call	bar
	leal	(%r15,%rax), %r14d
	movl	%r13d, %edi
	call	bar
	addl	%eax, %r14d
	movl	%r12d, %edi
	call	bar
	addl	%r14d, %eax
	addl	%ebx, %eax
	movq	-56(%rbp), %rdx
	subq	%fs:40, %rdx
	jne	.L7
	addq	$280, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
.L7:
	.cfi_restore_state
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE1:
	.size	foo, .-foo
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	leal	(%rdi,%rdi), %ecx
	leal	2(%rdi), %edx
	leal	1(%rdi), %esi
	call	foo
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
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
