/*	$NetBSD: cpufunc.c,v 1.1 2026/03/18 06:41:41 skrll Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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


/*-
 * Copyright (c) 2026 Rui-Xiang Guo
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLinIED WARRANTIES, INCLUDING, BUT NOT LIMITED
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

#include "opt_riscv_debug.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpufunc.c,v 1.1 2026/03/18 06:41:41 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <riscv/cpufunc.h>
#include <riscv/sbi.h>

#ifdef VERBOSE_INIT_RISCV
#define	VPRINTF(...)	printf(__VA_ARGS__)
#else
#define	VPRINTF(...)	__nothing
#endif

static void
cache_nullop(vaddr_t va, psize_t sz)
{
}

static void
scache_nullop(vaddr_t va, paddr_t pa, psize_t sz)
{
}

void (*cpu_dcache_wbinv_range)(vaddr_t, psize_t) = cache_nullop;
void (*cpu_dcache_inv_range)(vaddr_t, psize_t) = cache_nullop;
void (*cpu_dcache_wb_range)(vaddr_t, psize_t) = cache_nullop;

void (*cpu_sdcache_wbinv_range)(vaddr_t, paddr_t, psize_t) = scache_nullop;
void (*cpu_sdcache_inv_range)(vaddr_t, paddr_t, psize_t) = scache_nullop;
void (*cpu_sdcache_wb_range)(vaddr_t, paddr_t, psize_t) = scache_nullop;

u_int   riscv_dcache_align = CACHE_LINE_SIZE;
u_int   riscv_dcache_align_mask = CACHE_LINE_SIZE - 1;

static void
thead_dcache_wbinv_range(vaddr_t va, vsize_t sz)
{
	const u_int line_size = riscv_dcache_align;

	KASSERT(powerof2(line_size));
	KASSERT(sz != 0);

	const vaddr_t sva = rounddown2(va, line_size);
	const vaddr_t eva = roundup2(va + sz, line_size);

	for (vaddr_t addr = sva; addr < eva; addr += line_size) {
		asm volatile(
		    "mv a0, %0\n"
		    ".long 0x0275000b\n"	/* dcache.civa a0 */
		    :
		    : "r"(addr)
		    : "a0","memory");
	}

	asm volatile("fence rw, rw" ::: "memory");
}

static void
thead_dcache_inv_range(vaddr_t va, vsize_t sz)
{
	const u_int line_size =  riscv_dcache_align;

	KASSERT(powerof2(line_size));
	KASSERT(sz != 0);

	const vaddr_t sva = rounddown2(va, line_size);
	const vaddr_t eva = roundup2(va + sz, line_size);

	for (vaddr_t addr = sva; addr < eva; addr += line_size) {
		asm volatile(
		    "mv a0, %0\n"
		    ".long 0x0265000b\n"	/* dcache.iva a0 */
		    :
		    : "r"(addr)
		    : "a0", "memory");
	}

	asm volatile("fence rw, rw" ::: "memory");
}

static void
thead_dcache_wb_range(vaddr_t va, vsize_t sz)
{
	const u_int line_size = riscv_dcache_align;

	KASSERT(powerof2(line_size));
	KASSERT(sz != 0);

	const vaddr_t sva = rounddown2(va, line_size);
	const vaddr_t eva = roundup2(va + sz, line_size);

	for (vaddr_t addr = sva; addr < eva; addr += line_size) {
		asm volatile(
		    "mv a0, %0\n"
		    ".long 0x0245000b\n"	/* dcache.cva a0 */
		    :
		    : "r"(addr)
		    : "a0", "memory");
	}

	asm volatile("fence rw, rw" ::: "memory");
}


/*
 * This is only called on the BP.
 */
int
set_cpufuncs(void)
{
	const register_t mvendorid = sbi_get_mvendorid().value;

	switch (mvendorid) {
	case CPU_VENDOR_THEAD:
#ifdef _LP64
		/*
		 * Critical to do this before mucking around with any more
		 * mappings.
		 */
		if (csr_thead_sxstatus_read() & TX_SXSTATUS_MAEE) {
			VPRINTF("T-Head XMAE detected.\n");
			pmap_pte_xmae();
		}
		/*
		 * No fixups of the initial MMU tables.  We have to assume
		 * that those were set up correctly in locore.S.  The variables
		 * set above are for new mappings created now that the kernel
		 * is up and running.
		 */
#endif

		cpu_dcache_wbinv_range = thead_dcache_wbinv_range;
		cpu_dcache_inv_range = thead_dcache_inv_range;
		cpu_dcache_wb_range = thead_dcache_wb_range;


		break;
	default:
		break;
	}


	return 0;
}

