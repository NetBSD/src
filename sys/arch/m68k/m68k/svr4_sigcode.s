/*	$NetBSD: svr4_sigcode.s,v 1.2.2.2 2000/12/08 09:28:16 bouyer Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: locore.s 1.66 92/12/22$
 *
 *	@(#)locore.s	8.6 (Berkeley) 5/27/94
 */

#include <machine/asm.h>

#ifndef SVR4_SIGF_HANDLER
/*
 * This check is here so that it's possible to use the file as both
 * standalone and included into port's locore.s
 */
#include "assym.h"
#endif

/*
 * NOTICE: This is typically included in port's locore.s, like so:
 *
 *	#ifdef COMPAT_SVR4
 *	#include <m68k/m68k/svr4_sigcode.s>
 *	#endif
 */

	.data
	.align	2
GLOBAL(svr4_sigcode)
	movl	%sp@(SVR4_SIGF_HANDLER),%a0	| signal handler addr
	jsr	%a0@				| call signal handler
	lea	%sp@(SVR4_SIGF_UC),%a0		| ucontext to resume addr
	movl	%a0,%sp@-			| push pointer to ucontext
	movl	#SVR4_SETCONTEXT,%sp@-		| push context() subcode
	subql	#4,%sp				| padding for call frame layout
	movql	#SVR4_SYS_context,%d0		| setcontext(&sf.sf_uc)
	trap	#0				|  shouldn't return
	movl	%d0,%sp@(4)			|  so save `errno'
	moveq	#SVR4_SYS_exit,%d0		|  and exit hard
	trap	#0				| _exit(errno)
	.align	2
GLOBAL(svr4_esigcode)

