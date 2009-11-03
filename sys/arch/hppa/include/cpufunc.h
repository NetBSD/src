/*	$NetBSD: cpufunc.h,v 1.12 2009/11/03 05:07:26 snj Exp $	*/

/*	$OpenBSD: cpufunc.h,v 1.17 2000/05/15 17:22:40 mickey Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  (c) Copyright 1988 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */
/*
 * Copyright (c) 1990,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: c_support.s 1.8 94/12/14$
 *	Author: Bob Wheeler, University of Utah CSL
 */

#ifndef _HPPA_CPUFUNC_H_
#define _HPPA_CPUFUNC_H_

#include <machine/psl.h>
#include <machine/pte.h>

#define tlbbtop(b) ((b) >> (PGSHIFT - 5))
#define tlbptob(p) ((p) << (PGSHIFT - 5))

#define hptbtop(b) ((b) >> 17)

/* Get space register for an address */
static __inline register_t ldsid(vaddr_t p) {
	register_t ret;
	__asm volatile("ldsid (%1),%0" : "=r" (ret) : "r" (p));
	return ret;
}

#define mtctl(v,r) __asm volatile("mtctl %0,%1":: "r" (v), "i" (r))
#define mfctl(r,v) __asm volatile("mfctl %1,%0": "=r" (v): "i" (r))

#define mfcpu(r,v)	/* XXX for the lack of the mnemonics */		\
	__asm volatile("diag  %1\n\t"					\
			 "copy  %%r22, %0"				\
	: "=r" (v) : "i" ((0x1400 | ((r) << 21) | (22))) : "r22")

#define mtsp(v,r) __asm volatile("mtsp %0,%1":: "r" (v), "i" (r))
#define mfsp(r,v) __asm volatile("mfsp %1,%0": "=r" (v): "i" (r))

#define ssm(v,r) __asm volatile("ssm %1,%0": "=r" (r): "i" (v))
#define rsm(v,r) __asm volatile("rsm %1,%0": "=r" (r): "i" (v))

/* Move to system mask. Old value of system mask is returned. */
static __inline register_t mtsm(register_t mask) {
	register_t ret;
	__asm volatile(
	    "ssm 0,%0\n\t"
	    "mtsm %1": "=&r" (ret) : "r" (mask));
	return ret;
}

#define	fdce(sp,off) __asm volatile("fdce 0(%0,%1)":: "i" (sp), "r" (off))
#define	fice(sp,off) __asm volatile("fice 0(%0,%1)":: "i" (sp), "r" (off))
#define sync_caches() \
    __asm volatile("sync\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop":::"memory")

static __inline void
iitlba(u_int pg, pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("iitlba %0,(%%sr1, %1)":: "r" (pg), "r" (va));
}

static __inline void
idtlba(u_int pg, pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("idtlba %0,(%%sr1, %1)":: "r" (pg), "r" (va));
}

static __inline void
iitlbp(u_int prot, pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("iitlbp %0,(%%sr1, %1)":: "r" (prot), "r" (va));
}

static __inline void
idtlbp(u_int prot, pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("idtlbp %0,(%%sr1, %1)":: "r" (prot), "r" (va));
}

static __inline void
pitlb(pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("pitlb %%r0(%%sr1, %0)":: "r" (va));
}

static __inline void
pdtlb(pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("pdtlb %%r0(%%sr1, %0)":: "r" (va));
}

static __inline void
pitlbe(pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("pitlbe %%r0(%%sr1, %0)":: "r" (va));
}

static __inline void
pdtlbe(pa_space_t sp, vaddr_t va)
{
	mtsp(sp, 1);
	__asm volatile("pdtlbe %%r0(%%sr1, %0)":: "r" (va));
}

#ifdef _KERNEL
extern int (*cpu_hpt_init)(vaddr_t, vsize_t);

void ficache(pa_space_t, vaddr_t, vsize_t);
void fdcache(pa_space_t, vaddr_t, vsize_t);
void pdcache(pa_space_t, vaddr_t, vsize_t);
void fcacheall(void);
void ptlball(void);
hppa_hpa_t cpu_gethpa(int);

#define PCXL2_ACCEL_IO_START		0xf4000000
#define PCXL2_ACCEL_IO_END		(0xfc000000 - 1)
#define PCXL2_ACCEL_IO_ADDR2MASK(a)	(0x8 >> ((((a) >> 25) - 2) & 3))
void eaio_l2(int);

/*
 * These flush or purge the data cache for a item whose total 
 * size is <= the size of a data cache line, however they don't
 * check this constraint.
 */
static __inline void
fdcache_small(pa_space_t sp, vaddr_t va, vsize_t size)
{
	__asm volatile(
		"	mtsp	%0,%%sr1		\n"
		"	fdc	%%r0(%%sr1, %1)		\n"
		"	fdc	%2(%%sr1, %1)		\n"
		"	sync				\n"
		"	syncdma				\n"
		:
		: "r" (sp), "r" (va), "r" (size - 1));
}
static __inline void
pdcache_small(pa_space_t sp, vaddr_t va, vsize_t size)
{
	__asm volatile(
		"	mtsp	%0,%%sr1		\n"
		"	pdc	%%r0(%%sr1, %1)		\n"
		"	pdc	%2(%%sr1, %1)		\n"
		"	sync				\n"
		"	syncdma				\n"
		:
		: "r" (sp), "r" (va), "r" (size - 1));
}

#endif /* _KERNEL */

#endif /* _HPPA_CPUFUNC_H_ */
