/*	$NetBSD: sh3.s,v 1.1 2022/10/14 19:41:18 ryo Exp $	*/

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
 *  as -o sh3.o sh3.s
 *  ld -o hello -e __start sh3.o
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
	 * - src/lib/csu/arch/sh3/crti.S
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

	.global __start
	.type _start, %function
__start:
	/* write(STDOUT_FILENO, message, MESSAGE_SIZE) */
	mov	#1, r4			/* r4: fd = STDOUT_FILENO */
	mov.l	.Lmessage, r5		/* r5: buf = message */
	mov	#MESSAGE_SIZE, r6	/* r6: nbytes = MESSAGE_SIZE */
	mov	#4, r0			/* SYS_write */
	trapa	#0x80

	/* exit(0) */
	mov	#0, r4			/* r4: status = 0 */
	mov	#1, r0			/* SYS_exit */
	trapa	#0x80

	.p2align 2
.Lmessage:
	.long	message

	.size __start, . - __start
