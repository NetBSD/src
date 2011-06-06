/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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
 *
 */
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: booke_cache.c,v 1.1.4.1 2011/06/06 09:06:25 jruoho Exp $");

#include <sys/param.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

static void inline
dcbf(vaddr_t va, vsize_t off)
{
	__asm volatile("dcbf\t%0,%1" : : "b" (va), "r" (off));
}

static void inline
dcbst(vaddr_t va, vsize_t off)
{
	__asm volatile("dcbst\t%0,%1" : : "b" (va), "r" (off));
}

static void inline
dcbi(vaddr_t va, vsize_t off)
{
	__asm volatile("dcbi\t%0,%1" : : "b" (va), "r" (off));
}

static void inline
dcbz(vaddr_t va, vsize_t off)
{
	__asm volatile("dcbz\t%0,%1" : : "b" (va), "r" (off));
}

static void inline
dcba(vaddr_t va, vsize_t off)
{
	__asm volatile("dcba\t%0,%1" : : "b" (va), "r" (off));
}

static void inline
icbi(vaddr_t va, vsize_t off)
{
	__asm volatile("icbi\t%0,%1" : : "b" (va), "r" (off));
}

static inline void
cache_op(vaddr_t va, vsize_t len, vsize_t line_size,
	void (*op)(vaddr_t, vsize_t))
{
	KASSERT(line_size > 0);

	if (len == 0)
		return;

	/* Make sure we flush all cache lines */
	len += va & (line_size - 1);
	va &= -line_size;

	for (vsize_t i = 0; i < len; i += line_size)
		(*op)(va, i);
	__asm volatile("mbar 0");
}

void
dcache_wb_page(vaddr_t va)
{
	cache_op(va, PAGE_SIZE, curcpu()->ci_ci.dcache_line_size, dcbst);
}

void
dcache_wbinv_page(vaddr_t va)
{
	cache_op(va, PAGE_SIZE, curcpu()->ci_ci.dcache_line_size, dcbf);
}

void
dcache_inv_page(vaddr_t va)
{
	cache_op(va, PAGE_SIZE, curcpu()->ci_ci.dcache_line_size, dcbi);
}

void
dcache_zero_page(vaddr_t va)
{
	cache_op(va, PAGE_SIZE, curcpu()->ci_ci.dcache_line_size, dcbz);
}

void
icache_inv_page(vaddr_t va)
{
	__asm("msync");
	cache_op(va, PAGE_SIZE, curcpu()->ci_ci.icache_line_size, icbi);
	__asm("msync");
	/* synchronizing instruction will be the rfi to user mode */
}

void
dcache_wb(vaddr_t va, vsize_t len)
{
	cache_op(va, len, curcpu()->ci_ci.dcache_line_size, dcbst);
}

void
dcache_wbinv(vaddr_t va, vsize_t len)
{
	cache_op(va, len, curcpu()->ci_ci.dcache_line_size, dcbf);
}

void
dcache_inv(vaddr_t va, vsize_t len)
{
	cache_op(va, len, curcpu()->ci_ci.dcache_line_size, dcbi);
}

void
icache_inv(vaddr_t va, vsize_t len)
{
	__asm volatile("msync");
	cache_op(va, len, curcpu()->ci_ci.icache_line_size, icbi);
	__asm volatile("msync");
}
