/*	$NetBSD: sigprocmask.S,v 1.1 1997/03/29 20:56:08 thorpej Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 * from: Header: sigprocmask.s,v 1.1 91/07/06 13:06:01 torek Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)sigprocmask.s	8.1 (Berkeley) 6/4/93"
#endif /* LIBC_SCCS and not lint */

#include "SYS.h"

/*
 * sigprocmask(int how, sigset_t *set, sigset_t *oset)
 */
ENTRY(sigprocmask)
	or.	4,4,4		# set == NULL?
	li	6,1		# how = SIG_BLOCK
	beq	1f
	lwz	4,0(4)		# if not, replace it in r4 with *set
	mr	6,3
1:
	mr	3,6		# ... using sigprocmask(SIG_BLOCK)
	li	0,SYS_sigprocmask
	sc
	bso	3f
	or.	5,5,5		# check to see of oset requested
	beq	2f		# if oset != NULL,
	stw	3,0(5)		#    *oset = oldmask
2:
	li	3,0
	blr			# in any case, return 0
3:
	b	cerror
