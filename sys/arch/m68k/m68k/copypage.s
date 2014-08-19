/*	$NetBSD: copypage.s,v 1.14.18.1 2014/08/20 00:03:11 tls Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by J.T. Conklin <jtc@NetBSD.org> and 
 * by Hiroshi Horitomo <horimoto@cs-aoi.cs.sist.ac.jp> 
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
 * Optimized functions for copying/clearing a whole page.
 */

#include "opt_m68k_arch.h"

#include <machine/asm.h>
#include "assym.h"

	.file	"copypage.s"
	.text

/*
 * copypage040(fromaddr, toaddr)
 *
 * Optimized version of bcopy for a single page-aligned PAGE_SIZE byte copy,
 * using instructions only available on the mc68040 and later.
 */
#if defined(M68040) || defined(M68060)
ENTRY(copypage040)
	movl	4(%sp),%a0		| source address
	movl	8(%sp),%a1		| destiniation address
	movw	#PAGE_SIZE/32-1,%d0	| number of 32 byte chunks - 1
.Lm16loop:
	.long	0xf6209000		| move16 (%a0)+,(%a1)+
	.long	0xf6209000		| move16 (%a0)+,(%a1)+
	dbf	%d0,.Lm16loop
	rts
#endif /* M68040 || M68060 */

/*
 * copypage(fromaddr, toaddr)
 *
 * Optimized version of bcopy for a single page-aligned PAGE_SIZE byte copy.
 */
ENTRY(copypage)
	movl	4(%sp),%a0		| source address
	movl	8(%sp),%a1		| destiniation address
#ifndef	__mc68010__
	movw	#PAGE_SIZE/32-1,%d0	| number of 32 byte chunks - 1
.Lmlloop:
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	movl	(%a0)+,(%a1)+
	dbf	%d0,.Lmlloop
#else	/* __mc68010__ */	
	movw	#PAGE_SIZE/4-1,%d0	| number of 4 byte chunks - 1
.Lmlloop:
	movl	(%a0)+,(%a1)+
	dbf	%d0,.Lmlloop		| use the 68010 loop mode
#endif	/* __mc68010__ */	
	rts

/*
 * zeropage(addr)
 *
 * Optimized version of bzero for a single page-aligned PAGE_SIZE byte zero.
 */
ENTRY(zeropage)
	movl	4(%sp),%a0		| dest address
#ifndef	__mc68010__
	movql	#PAGE_SIZE/256-1,%d0	| number of 256 byte chunks - 1
	movml	%d2-%d7,-(%sp)
	movql	#0,%d1
	movql	#0,%d2
	movql	#0,%d3
	movql	#0,%d4
	movql	#0,%d5
	movql	#0,%d6
	movql	#0,%d7
	movl	%d1,%a1
	lea	PAGE_SIZE(%a0),%a0
.Lzloop:
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	movml	%d1-%d7/%a1,-(%a0)
	dbf	%d0,.Lzloop
	movml	(%sp)+,%d2-%d7
#else	/* __mc68010__ */
	movw	#PAGE_SIZE/4-1,%d0	| number of 4 byte chunks - 1
.Lzloop:
	clrl	(%a0)+
	dbf	%d0,.Lzloop		| use the 68010 loop mode
#endif	/* __mc68010__ */
	rts
