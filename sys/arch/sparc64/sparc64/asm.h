/*	$NetBSD: asm.h,v 1.5 1999/05/30 18:57:27 eeh Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)asm.h	8.1 (Berkeley) 6/11/93
 */

/*
 * GCC __asm constructs for doing assembly stuff.
 */

/*
 * ``Routines'' to load and store from/to alternate address space.
 * The location can be a variable, the asi value (address space indicator)
 * must be a constant.
 *
 * N.B.: You can put as many special functions here as you like, since
 * they cost no kernel space or time if they are not used.
 *
 * These were static inline functions, but gcc screws up the constraints
 * on the address space identifiers (the "n"umeric value part) because
 * it inlines too late, so we have to use the funny valued-macro syntax.
 */
/* load byte from alternate address space */
#define	lduba(loc, asi) ({ \
	register int _lduba_v; \
	__asm __volatile("wr %2,%%g0,%%asi; lduba [%1]%%asi,%0" : "=r" (_lduba_v) : \
	    "r" ((long long)(loc)), "r" (asi)); \
	_lduba_v; \
})

/* load half-word from alternate address space */
#define	lduha(loc, asi) ({ \
	register int _lduha_v; \
	__asm __volatile("wr %2,%%g0,%%asi; lduha [%1]%%asi,%0" : "=r" (_lduha_v) : \
	    "r" ((long long)(loc)), "r" (asi)); \
	_lduha_v; \
})

/* load int from alternate address space */
#define	lda(loc, asi) ({ \
	register int _lda_v; \
	__asm __volatile("wr %2,%%g0,%%asi; lda [%1]%%asi,%0" : "=r" (_lda_v) : \
	    "r" ((int)(loc)), "r" (asi)); \
	_lda_v; \
})

#define	ldswa(loc, asi) ({ \
	register int _lda_v; \
	__asm __volatile("wr %2,%%g0,%%asi; ldswa [%1]%%asi,%0" : "=r" (_lda_v) : \
	    "r" ((int)(loc)), "r" (asi)); \
	_lda_v; \
})

/* store byte to alternate address space */
#define	stba(loc, asi, value) ({ \
	__asm __volatile("wr %2,%%g0,%%asi; stba %0,[%1]%%asi; membar #Sync" : : \
	    "r" ((int)(value)), "r" ((int)(loc)), "r" (asi)); \
})

/* store half-word to alternate address space */
#define	stha(loc, asi, value) ({ \
	__asm __volatile("wr %2,%%g0,%%asi; stha %0,[%1]%%asi; membar #Sync" : : \
	    "r" ((int)(value)), "r" ((int)(loc)), "r" (asi)); \
})

/* store int to alternate address space */
#define	sta(loc, asi, value) ({ \
	__asm __volatile("wr %2,%%g0,%%asi; sta %0,[%1]%%asi; membar #Sync" : : \
	    "r" ((int)(value)), "r" ((int)(loc)), "r" (asi)); \
})

/* load 64-bit int from alternate address space */
#define	ldda(loc, asi) ({ \
	register long long _lda_v; \
	__asm __volatile("wr %2,%%g0,%%asi; ldda [%1]%%asi,%0" : "=r" (_lda_v) : \
	    "r" ((int)(loc)), "r" (asi)); \
	_lda_v; \
})

/* store 64-bit int to alternate address space */
#define	stda(loc, asi, value) ({ \
	__asm __volatile("wr %2,%%g0,%%asi; stda %0,[%1]%%asi; membar #Sync" : : \
	    "r" ((long long)(value)), "r" ((int)(loc)), "r" (asi)); \
})

#ifdef __arch64__
/* native load 64-bit int from alternate address space w/64-bit compiler*/
#define	ldxa(loc, asi) ({ \
	register long _lda_v; \
	__asm __volatile("wr %2,%%g0,%%asi; ldxa [%1]%%asi,%0" : "=r" (_lda_v) : \
	    "r" ((long)(loc)), "r" (asi)); \
	_lda_v; \
})
#else
/* native load 64-bit int from alternate address space w/32-bit compiler*/
#define	ldxa(loc, asi) ({ \
	volatile register long _ldxa_tmp = 0; \
	volatile int64_t _ldxa_v; \
	volatile int64_t *_ldxa_a = &_ldxa_v; \
	__asm __volatile("wr %2,%%g0,%%asi; ldxa [%1]%%asi,%1; stx %1,[%3]; membar #Sync" : "=r" (_ldxa_tmp) : \
	    "r" ((long)(loc)), "r" (asi), "r" ((long)(_ldxa_a))); \
	_ldxa_v; \
})
#endif

#ifdef __arch64__
/* native store 64-bit int to alternate address space w/64-bit compiler*/
#define	stxa(loc, asi, value) ({ \
	__asm __volatile("wr %2,%%g0,%%asi; stxa %0,[%1]%%asi; membar #Sync" : : \
	    "r" ((long)(value)), "r" ((long)(loc)), "r" (asi)); \
})
#else
/* native store 64-bit int to alternate address space w/32-bit compiler*/
#define	stxa(loc, asi, value) ({ \
	int64_t _stxa_v; \
	int64_t *_stxa_a = &_stxa_v; \
	_stxa_v = value; \
	__asm __volatile("wr %2,%%g0,%%asi; ldx [%0],%3; stxa %3,[%1]%%asi; membar #Sync" : : \
	    "r" ((long)(_stxa_a)), "r" ((long)(loc)), "r" (asi), "r" ((long)(_stxa_v))); \
})
#endif

/* flush address from data cache */
#define flush(loc) ({ \
	__asm __volatile("flush %0" : : \
	     "r" ((long)(loc))); \
})

#define membar_sync() __asm __volatile("membar #Sync" : :)

#ifdef __arch64__
/* read 64-bit %tick register */
#define	tick() ({ \
	register long _tick_tmp; \
	__asm __volatile("rdpr %%tick, %0" : "=r" (_tick_tmp) :); \
	_tick_tmp; \
})
#else
/* native load 64-bit int from alternate address space w/32-bit compiler*/
#define	tick() ({ \
	volatile register long _tick_tmp = 0; \
	volatile int64_t _tick_v; \
	volatile int64_t *_tick_a = &_tick_v; \
	__asm __volatile("rdpr %%tick, %0; stx %0,[%1]; membar #StoreLoad" : "=r" (_tick_tmp) : \
	    "r" ((long)(_tick_a))); \
	_tick_v; \
})
#endif

/* atomic load/store of a byte in memory */
#define	ldstub(loc) ({ \
	int _v; \
	__asm __volatile("ldstub [%1],%0" : "=r" (_v) : "r" (loc) : "memory"); \
	_v; \
})

	
