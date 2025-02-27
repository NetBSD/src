/*	$NetBSD: h_execregs.S,v 1.1 2025/02/27 00:55:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#define	_LOCORE

#include <sys/syscall.h>

#include <machine/asm.h>

#include "execregs.h"

ENTRY(execregs_start)
	andl	$-0x4,%esp		/* align stack to 4-byte boundary */

	/* store registers to a buffer on stack */
	subl	$(NEXECREGS*4),%esp	/* space for NEXECREGS registers */
	movl	%edi,0*4(%esp)		/* order matches execregs.h */
	movl	%esi,1*4(%esp)
	movl	%ebp,2*4(%esp)
	movl	%edx,3*4(%esp)
	movl	%ecx,4*4(%esp)
	movl	%eax,5*4(%esp)

	/* call write(STDOUT_FILENO, regs, sizeof(regs)) */
	movl	%esp,%eax		/* eax := regs */
	pushl	$(NEXECREGS*4)		/* arg2 := sizeof(regs) */
	pushl	%eax			/* arg1 := regs */
	pushl	$0x1			/* arg0 := STDOUT_FILENO */
	call	execregs_write

	jb	2f			/* bail if write failed */
	cmpl	$(NEXECREGS*4),%eax	/* bail if wrote wrong # of bytes */
	jne	2f

	/* call exit(0) */
	pushl	$0			/* arg0 := 0 */
1:	call	execregs_exit
	hlt				/* paranoia */

2:	/* call exit(127) */
	pushl	$127			/* arg0 := 127 */
	jmp	1b
END(execregs_start)

ENTRY(execregs_write)
	movl	$SYS_write,%eax		/* syscall number */
	int	$0x80
	retl
END(execregs_write)

ENTRY(execregs_exit)
	movl	$SYS_exit,%eax		/* syscall number */
	int	$0x80
	hlt
END(execregs_exit)

/* main stub to simplify linking */
ENTRY(main)
	hlt
END(main)
