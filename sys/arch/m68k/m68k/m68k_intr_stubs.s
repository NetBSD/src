/*	$NetBSD: m68k_intr_stubs.s,v 1.2 2024/01/16 01:16:46 thorpej Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      
 *	from: Utah $Hdr: locore.s 1.66 92/12/22$
 *	@(#)locore.s    8.6 (Berkeley) 5/27/94
 */     

#include <machine/asm.h>

#include "assym.h"

	.file   "m68k_intr_stubs.s"
	.text

#ifdef __ELF__
#define	INTRSTUB_ALIGN	.align 4
#else
#define	INTRSTUB_ALIGN	.align 2
#endif

/*
 * XXX Some platforms (e.g. news68k) have hardware-assisted ASTs, and
 * XXX thus don't need to branch to rei() after an interrupt.  Figure
 * XXX out a way to handle these platforms.  This works for now; the
 * XXX hardware-assist is just an optimization.
 */

/*
 * Vector stub for auto-vectored interrupts.  Calls the dispatch
 * routine with the frame BY VALUE (saves a few instructions).
 */
	INTRSTUB_ALIGN
ENTRY_NOPROFILE(intrstub_autovec)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(m68k_intr_autovec)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)

#ifdef __HAVE_M68K_INTR_VECTORED
/*
 * Vector stub for vectored interrupts.  Same stack situation as above.
 */
	INTRSTUB_ALIGN
ENTRY_NOPROFILE(intrstub_vectored)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(m68k_intr_vectored)
	INTERRUPT_RESTOREREG
	jra	_ASM_LABEL(rei)
#endif /* __HAVE_M68K_INTR_VECTORED */
