/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	from: @(#)SYS.h	5.5 (Berkeley) 5/7/91
 *	$Id: SYS.h,v 1.1 1993/11/25 23:34:10 paulus Exp $
 */

#include <sys/syscall.h>

#ifdef PROF
#ifdef __GNUC__
#define	ENTRY(x)	.globl _/**/x; .even; _/**/x:; \
			.data; PROF/**/x:; .long 0; \
			.text; link a6,#0; lea PROF/**/x,a0; \
			jbsr mcount; unlk a6
#else
#define	ENTRY(x)	.globl _/**/x; .even; _/**/x:; \
			.data; PROF/**/x:; .long 0; \
			.text; lea PROF/**/x,a0; jbsr mcount
#endif
#else
#define	ENTRY(x)	.globl _/**/x; .even; _/**/x:
#endif	/* PROF */

#ifdef	PIC
/* With PIC code, we can't pass the error to cerror in d0,
   because the jump might go via the binder. */
#define	SYSCALL(x)	.even; err: movl d0,sp@-; jra cerror; ENTRY(x); \
			movl #SYS_/**/x,d0; trap #0; jcs err
#else
#define	SYSCALL(x)	.even; err: jra cerror; ENTRY(x); \
			movl #SYS_/**/x,d0; trap #0; jcs err
#endif

#define	RSYSCALL(x)	SYSCALL(x); rts
#define	PSEUDO(x,y)	ENTRY(x); movl #SYS_/**/y,d0; trap #0; rts

#define	ASMSTR		.asciz

	.globl	cerror
