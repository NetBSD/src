/*	$NetBSD: svr4_sigcode.s,v 1.3 2001/05/30 12:28:44 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */

#if defined(_KERNEL_OPT)
#include "opt_vm86.h"
#endif

#include "assym.h"

#include <machine/psl.h>
#include <machine/segments.h>
#include <machine/trap.h>
#include <compat/svr4/svr4_syscall.h>

/*
 * override user-land alignment before including asm.h
 */
#ifdef __ELF__
#define	ALIGN_DATA	.align	4
#define	ALIGN_TEXT	.align	4,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	16,0x90	/* 16-byte boundaries better for 486 */
#else
#define	ALIGN_DATA	.align	2
#define	ALIGN_TEXT	.align	2,0x90	/* 4-byte boundaries, NOP-filled */
#define	SUPERALIGN_TEXT	.align	4,0x90	/* 16-byte boundaries better for 486 */
#endif
#define _ALIGN_TEXT	ALIGN_TEXT
#include <machine/asm.h>

/*
 * XXX traditional CPP's evaluation semantics make this necessary.
 * XXX (__CONCAT() would be evaluated incorrectly)
 */
#ifdef __ELF__
#define	IDTVEC(name)	ALIGN_TEXT; .globl X/**/name; X/**/name:
#else
#define	IDTVEC(name)	ALIGN_TEXT; .globl _X/**/name; _X/**/name:
#endif


/*
 * These are used on interrupt or trap entry or exit.
 */
#define	INTRENTRY \
	pushl	%eax		; \
	pushl	%ecx		; \
	pushl	%edx		; \
	pushl	%ebx		; \
	pushl	%ebp		; \
	pushl	%esi		; \
	pushl	%edi		; \
	pushl	%ds		; \
	pushl	%es		; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	movl	%ax,%ds		; \
	movl	%ax,%es
#define	INTRFASTEXIT \
	popl	%es		; \
	popl	%ds		; \
	popl	%edi		; \
	popl	%esi		; \
	popl	%ebp		; \
	popl	%ebx		; \
	popl	%edx		; \
	popl	%ecx		; \
	popl	%eax		; \
	addl	$8,%esp		; \
	iret

/*
 * Signal trampoline; copied to top of user stack.
 */
/* LINTSTUB: Var: char svr4_sigcode[1], svr4_esigcode[1]; */
NENTRY(svr4_sigcode)
	call	SVR4_SIGF_HANDLER(%esp)
	leal	SVR4_SIGF_UC(%esp),%eax	# ucp (the call may have clobbered the
					# copy at SIGF_UCP(%esp))
#ifdef VM86
	testl	$PSL_VM,SVR4_UC_EFLAGS(%eax)
	jnz	1f
#endif
	movl	SVR4_UC_FS(%eax),%ecx
	movl	SVR4_UC_GS(%eax),%edx
	movl	%cx,%fs
	movl	%dx,%gs
1:	pushl	%eax
	pushl	$1			# setcontext(p) == syscontext(1, p) 
	pushl	%eax			# junk to fake return address
	movl	$SVR4_SYS_context,%eax
	int	$0x80	 		# enter kernel with args on stack
	movl	$SVR4_SYS_exit,%eax
	int	$0x80			# exit if sigreturn fails
	.globl	_C_LABEL(svr4_esigcode)
_C_LABEL(svr4_esigcode):

IDTVEC(svr4_fasttrap)
	pushl	$2		# size of instruction for restart
	pushl	$T_ASTFLT	# trap # for doing ASTs
	INTRENTRY
	call	_C_LABEL(svr4_fasttrap)
2:	/* Check for ASTs on exit to user mode. */
	cli
	cmpb	$0,_C_LABEL(astpending)
	je	1f
	/* Always returning to user mode here. */
	movb	$0,_C_LABEL(astpending)
	sti
	/* Pushed T_ASTFLT into tf_trapno on entry. */
	call	_C_LABEL(trap)
	jmp	2b
1:	INTRFASTEXIT
