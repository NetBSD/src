/*	$NetBSD: x86_64.s,v 1.1 2022/10/14 19:41:18 ryo Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * assemble & link:
 *  as -o x86_64.o x86_64.s
 *  ld -o hello -e _start x86_64.o
 */

	/* ------------------------------------------------------------------ */

	/*
	 * This ELF section is used by the kernel to determine, among other
	 * things, the system call interface used by the binary.
	 *
	 * Normally, /usr/lib/crti.o is linked, but here it is written manually.
	 *
	 * SEE ALSO:
	 * - http://www.netbsd.org/docs/kernel/elf-notes.html
	 * - src/sys/sys/exec_elf.h
	 * - src/lib/csu/common/sysident.S
	 * - src/lib/csu/arch/x86_64/crti.S
	 */
	.section ".note.netbsd.ident", "a"
	.p2align 2
	.long	7		/* ELF_NOTE_NETBSD_NAMESZ */
	.long	4		/* ELF_NOTE_NETBSD_DESCSZ */
	.long	1		/* ELF_NOTE_TYPE_NETBSD_TAG */
	.ascii	"NetBSD\0\0"	/* ELF_NOTE_NETBSD_NAME */
	.long	999010000	/* __NetBSD_Version__ (sys/sys/param.h) */


	/* ------------------------------------------------------------------ */

	.section ".rodata"
message:
	.ascii "Hello, world!\n"
	.set MESSAGE_SIZE, . - message


	/* ------------------------------------------------------------------ */

	.section ".text"
	.p2align 2

	.global _start
	.type _start, %function
_start:
	/* write(STDOUT_FILENO, message, MESSAGE_SIZE) */
	movl	$1, %edi		/* %edi: fd = STDOUT_FILENO */
	leaq	message(%rip), %rsi	/* %rsi: buf = message */
	movl	$MESSAGE_SIZE, %edx	/* %edx: nbytes = MESSAGE_SIZE */

	movl	$4, %eax		/* SYS_write */
	movq	%rcx, %r10
	syscall

	/* exit(0) */
	xorl	%edi, %edi		/* %rdi: status = 0 */
	mov	$1, %eax		/* SYS_exit */
	movq	%rcx, %r10
	syscall

	.size _start, . - _start
