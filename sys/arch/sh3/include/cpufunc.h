/*	$NetBSD: cpufunc.h,v 1.7 2001/06/24 05:32:55 msaitoh Exp $	*/

/*
 * Copyright (c) 1993 Charles Hannum.
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
 *      This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH3 Version
 *
 * T.Horiuchi Brains Corp. 05/22/1998
 */

#ifndef _SH3_CPUFUNC_H_
#define	_SH3_CPUFUNC_H_

/*
 * Functions to provide access to SH3-specific instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sh3/mmureg.h>

#ifdef _KERNEL

void enable_ext_intr __P((void));
void disable_ext_intr __P((void));
static __inline void setPageDir __P((int));
static __inline void cache_clear __P((int));
static __inline void breakpoint __P((void));

static __inline void
tlbflush(void)
{
/* #define CACHE_FLUSH 0x80 */

#ifdef SH4
	SHREG_MMUCR = (SHREG_MMUCR | MMUCR_TF) & MMUCR_VALIDBITS;
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
#else
	SHREG_MMUCR |= MMUCR_TF;
#endif

/*   SHREG_CCR |= CACHE_FLUSH; */
}

static __inline void
cacheflush(void)
{
#if 1
	volatile int *p = (int *)ram_start;
	int i;
	int d;

	for(i = 0; i < 512; i++){
		d = *p;
		p += 8;
	}
#else
#define CACHE_FLUSH 0x809

	SHREG_CCR |= CACHE_FLUSH; 
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
	__asm __volatile("nop");
#endif
}

static __inline void
setPageDir(pagedir)
	int pagedir;
{

	PageDirReg = pagedir;
	tlbflush();
#ifdef SH4
	SHREG_TTB = pagedir;
#endif
}

/* XXXX ought to be in psl.h with spl() functions */

static __inline void
disable_intr(void)
{

	__asm __volatile("mov.l r0, @-r15");
	__asm __volatile("mov.l r1, @-r15");
	__asm __volatile("mov #0x10, r0");
	__asm __volatile("swap.b r0, r0");
	__asm __volatile("swap.w r0, r0");
	__asm __volatile("stc sr, r1");
	__asm __volatile("or r0, r1");
	__asm __volatile("ldc r1, sr");
	__asm __volatile("mov.l @r15+, r1");
	__asm __volatile("mov.l @r15+, r0");
}

static __inline void
enable_intr(void)
{

	__asm __volatile("mov.l r0, @-r15");
	__asm __volatile("mov.l r1, @-r15");
	__asm __volatile("mov #0x10, r0");
	__asm __volatile("swap.b r0, r0");
	__asm __volatile("swap.w r0, r0");
	__asm __volatile("not r0, r0");
	__asm __volatile("stc sr, r1");
	__asm __volatile("and r0, r1");
	__asm __volatile("ldc r1, sr");
	__asm __volatile("mov.l @r15+, r1");
	__asm __volatile("mov.l @r15+, r0");
}

static __inline void
cache_clear(va)
	int va;
{
#ifdef SH4
	__asm __volatile("ocbp @%0" :: "r"(va));
#endif
}

static __inline void
breakpoint()
{
	__asm __volatile ("trapa #0xc3");
}

#endif

#endif /* !_SH3_CPUFUNC_H_ */
