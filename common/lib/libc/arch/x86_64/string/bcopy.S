/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from locore.s.
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

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: bcopy.S,v 1.3 2009/11/21 19:52:54 dsl Exp $")
#endif

	/*
	 * (ov)bcopy (src,dst,cnt)
	 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
	 *
	 * Hacked about by dsl@netnsd.org
	 */

#ifdef MEMCOPY
ENTRY(memcpy)
#define NO_OVERLAP
#else
#ifdef MEMMOVE
ENTRY(memmove)
#else
ENTRY(bcopy)
#endif
#endif
	movq	%rdx,%rcx
#if defined(MEMCOPY) || defined(MEMMOVE)
	movq	%rdi,%rax	/* must return destination address */
#else
	xchgq	%rdi,%rsi	/* bcopy() has arg order reversed */
#endif

#if !defined(NO_OVERLAP)
	movq	%rdi,%r8
	subq	%rsi,%r8
#endif

	shrq	$3,%rcx		/* count for copy by words */
	jz	8f		/* j if less than 8 bytes */

	lea	-8(%rdi,%rdx),%r9	/* target address of last 8 */
	mov	-8(%rsi,%rdx),%r10	/* get last bytes */
#if !defined(NO_OVERLAP)
	cmpq	%rdx,%r8	/* overlapping? */
	jb	10f
#endif

/*
 * Non-overlaping, copy forwards.
 * Newer Intel cpus (Nehalem) will do 16byte read/write transfers
 * if %ecx is more than 76.
 * AMD might do something similar some day.
 */
	rep
	movsq
	mov	%r10,(%r9)	/* write last bytes */
	ret

#if !defined(NO_OVERLAP)
/* Must copy backwards.
 * Reverse copy is probably easy to code faster than 'rep movds'
 * since that requires (IIRC) an extra clock per iteration.
 * However I don't suppose anything cares that much!
 * The copy is aligned with the buffer start (more likely to
 * be a multiple of 8 than the end).
 */
10:
	lea	-8(%rsi,%rcx,8),%rsi
	lea	-8(%rdi,%rcx,8),%rdi
	std
	rep
	movsq
	cld
	mov	%r10,(%r9)	/* write last bytes */
	ret
#endif

/* Less than 8 bytes to copy, copy by bytes */
/* Intel Nehalem optimise 'rep movsb' for <= 7 bytes (9-15 clocks).
 * For long transfers it is 50+ !
 */
8:	mov	%rdx,%rcx

#if !defined(NO_OVERLAP)
	cmpq	%rdx,%r8	/* overlapping? */
	jb	81f
#endif

	/* nope, copy forwards. */
	rep
	movsb
	ret

#if !defined(NO_OVERLAP)
/* Must copy backwards */
81:
	lea	-1(%rsi,%rcx),%rsi
	lea	-1(%rdi,%rcx),%rdi
	std
	rep
	movsb
	cld
	ret
#endif
