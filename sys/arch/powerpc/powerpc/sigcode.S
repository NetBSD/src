/*	$NetBSD: sigcode.S,v 1.7.16.1 2014/08/20 00:03:20 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <machine/signal.h>

#ifndef SIGCODE_NAME
#include <sys/syscall.h>
#define	SIGCODE_NAME	sigcode
#define	ESIGCODE_NAME	esigcode
#define	SIGRETURN_NAME	SYS_compat_16___sigreturn14
#define	EXIT_NAME	SYS_exit
#endif
#ifndef SIGNAL_FRAMESIZE
#define SIGNAL_FRAMESIZE	0
#endif
#ifndef SIGCALLFRAMESIZE
#define	SIGCALLFRAMESIZE	16
#endif

/*
 * The following code gets copied to the top of the user stack on process
 * execution.  It does signal trampolining on signal delivery.
 *
 * On entry r1 points to a struct sigframe at bottom of current stack.
 * All other registers are unchanged.
 */
	.globl	_C_LABEL(SIGCODE_NAME),_C_LABEL(ESIGCODE_NAME)
_C_LABEL(SIGCODE_NAME):
	addi	%r1,%r1,-SIGCALLFRAMESIZE	/* reserved space for callee */
	blrl
	addi	%r3,%r1,SIGCALLFRAMESIZE+SIGNAL_FRAMESIZE
						/* compute &sf_sc */
	li	%r0,SIGRETURN_NAME
	sc					/* sigreturn(scp) */
	li	%r0,EXIT_NAME
	sc					/* exit(errno) */
_C_LABEL(ESIGCODE_NAME):
