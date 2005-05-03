/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
	/* .asciz "@(#)setjmp.s	8.1 (Berkeley) 6/4/93" */
	.asciz "$NetBSD: setjmp.S,v 1.7 2005/05/03 04:37:33 matt Exp $"
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "DEFS.h"
#include "assym.h"

ENTRY(setjmp, R6)
	movl	4(%ap),%r6		# construct sigcontext
	subl2	$12,%sp			# space for current struct sigstack
	pushl	%sp			# get current values
	pushl	$0			# no new values
	calls	$4,_C_LABEL(__sigaltstack14) # pop args plus signal stack value
	movl	(%sp)+,(%r6)+		# save onsigstack status of caller
	subl2	$SS_LEN,%sp
	pushl	%sp
	pushl	$0
	pushl	$SIG_BLOCK
	calls	$3,_C_LABEL(sigprocmask)# get signal mask
	movl	(%sp),(%r6)+		# save signal mask of caller
	movl	(%ap),%r0
	moval	4(%ap)[%r0],(%r6)+	# save sp of caller
	movl	12(%fp),(%r6)+		# save frame pointer of caller
	movl	8(%fp),(%r6)+		# save argument pointer of caller
	movl	16(%fp),(%r6)+		# save pc of caller
	movpsl	(%r6)			# save psl of caller
	movw	4(%fp),(%r6)
	clrl	%r0
	ret

ENTRY(longjmp, 0)
	movl	8(%ap),%r0		# return(v)
	movl	4(%ap),%r1		# fetch buffer
	tstl	12(%r1)
	beql	botch
loop:
	cmpl	12(%r1),%fp		# are we there yet?
	beql	done
	blssu	botch
	moval	20(%fp),%r2
	blbc	6(%fp),1f		# was %r0 saved?
	movl	%r0,(%r2)+
1:
	bbc	$1,6(%fp),2f		# was %r1 saved?
	movl	%r1,(%r2)
2:
	movab	loop,16(%fp)
	ret				# pop another frame

done:
	pushl	SC13_LEN+20(%r1)	# here we fake a sigcontext
	pushl	SC13_LEN+16(%r1)	# from a sigcontext13
	pushl	SC13_LEN+12(%r1)
	pushl	SC13_LEN+8(%r1)		# first do save registers
	pushl	SC13_LEN+4(%r1)
	pushl	SC13_LEN+0(%r1)
	pushl	$0			# fill out the sigset
	pushl	$0
	pushl	$0
	pushl	SC13_MASK(%r1)
	pushl	SC13_PS(%r1)		# now do everything else
	pushl	SC13_PC(%r1)
	pushl	SC13_AP(%r1)
	pushl	SC13_FP(%r1)
	pushl	SC13_AP(%r1)
	pushl	$0
	pushl	SC13_ONSTACK(%r1)
	movl	%sp,%r1
	pushl	%r0			# the return value
	pushl	%r1			# the sigcontext
	calls	$2,_C_LABEL(__longjmp14)
botch:
	calls	$0,_C_LABEL(longjmperror)
	halt
