/*	$NetBSD: cache_sh4.c,v 1.10 2005/06/29 16:56:58 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache_sh4.c,v 1.10 2005/06/29 16:56:58 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/cache.h>
#include <sh3/cache_sh4.h>

#define	round_line(x)		(((x) + 31) & ~31)
#define	trunc_line(x)		((x) & ~31)

u_int32_t sh4_icache_mask;
u_int32_t sh4_dcache_mask;

void sh4_icache_sync_all(void);
void sh4_icache_sync_range(vaddr_t, vsize_t);
void sh4_icache_sync_range_index(vaddr_t, vsize_t);
void sh4_dcache_wbinv_all(void);
void sh4_dcache_wbinv_range(vaddr_t, vsize_t);
void sh4_dcache_wbinv_range_index(vaddr_t, vsize_t);
void sh4_dcache_inv_range(vaddr_t, vsize_t);
void sh4_dcache_wb_range(vaddr_t, vsize_t);

/* must be inlined. */
static __inline__ void cache_sh4_op_line_32(vaddr_t, vaddr_t, u_int32_t,
    u_int32_t);
static __inline__ void cache_sh4_op_8lines_32(vaddr_t, vaddr_t, u_int32_t,
    u_int32_t);

void
sh4_cache_config(void)
{
	jsize_t icache_size;
	size_t dcache_size;
	int ways;
	u_int32_t r;

        /* Determine cache size */
	switch (cpu_product) {
	default:
		/* FALLTHROUGH */
	case CPU_PRODUCT_7750:
	case CPU_PRODUCT_7750S:
	case CPU_PRODUCT_7751:
#if defined(SH4_CACHE_DISABLE_EMODE)
	case CPU_PRODUCT_7750R:
	case CPU_PRODUCT_7751R:
#endif
		icache_size = 8 * 1024;
		dcache_size = 16 * 1024;
		ways = 1;
		r = 0x0000090b;
		break;

#if !defined(SH4_CACHE_DISABLE_EMODE)
	case CPU_PRODUCT_7750R:
	case CPU_PRODUCT_7751R:
		icache_size = 16 * 1024;
		dcache_size = 32 * 1024;
		ways = 2;
		r = 0x8000090b;
		break;
#endif
	}
#if defined(SH4_ICACHE_DISABLE)
	r &= ~0x00000100;
#endif
#if defined(SH4_DCACHE_DISABLE)
	r &= ~0x00000001;
#endif
#if defined(SH4_DCACHE_WB_P0)
	r &= ~0x00000002;
#endif
#if defined(SH4_DCACHE_WB_P1)
	r |= 0x00000004;
#endif

	sh_cache_unified = 0;
	sh_cache_ways = ways;
	sh_cache_line_size = 32;
	sh_cache_size_dcache = dcache_size;
	if (sh_cache_ram_mode)
		sh_cache_size_dcache /= 2;
	sh_cache_size_icache = icache_size;

	sh4_icache_mask = (sh_cache_size_icache / sh_cache_line_size) - 1;
	sh4_icache_mask <<= CCIA_ENTRY_SHIFT;
	sh4_dcache_mask = (sh_cache_size_dcache / sh_cache_line_size) - 1;
	sh4_dcache_mask <<= CCDA_ENTRY_SHIFT;

	/*
	 * For now, P0, U0, P3 write-through P1 write-through
	 * XXX will be obsoleted.
	 */
	sh4_icache_sync_all();
	RUN_P2;
	_reg_write_4(SH4_CCR, r);
	RUN_P1;

	r = _reg_read_4(SH4_CCR);

	sh_cache_enable_icache = (r & SH4_CCR_ICE);
	sh_cache_enable_dcache = (r & SH4_CCR_OCE);
	sh_cache_write_through_p0_u0_p3 = (r & SH4_CCR_WT);
	sh_cache_write_through_p1 = !(r & SH4_CCR_CB);
	sh_cache_write_through = sh_cache_write_through_p0_u0_p3 &&
	    sh_cache_write_through_p1;
	sh_cache_ram_mode = (r & SH4_CCR_ORA);
	sh_cache_index_mode_icache = (r & SH4_CCR_IIX);
	sh_cache_index_mode_dcache = (r & SH4_CCR_OIX);

	sh_cache_ops._icache_sync_all		= sh4_icache_sync_all;
	sh_cache_ops._icache_sync_range		= sh4_icache_sync_range;
	sh_cache_ops._icache_sync_range_index	= sh4_icache_sync_range_index;

	sh_cache_ops._dcache_wbinv_all		= sh4_dcache_wbinv_all;
	sh_cache_ops._dcache_wbinv_range	= sh4_dcache_wbinv_range;
	sh_cache_ops._dcache_wbinv_range_index	= sh4_dcache_wbinv_range_index;
	sh_cache_ops._dcache_inv_range		= sh4_dcache_inv_range;
	sh_cache_ops._dcache_wb_range		= sh4_dcache_wb_range;
}

/*
 * cache_sh4_op_line_32: (index-operation)
 *
 *	Clear the specified bits on single 32-byte cache line.
 *
 */
static __inline__ void
cache_sh4_op_line_32(vaddr_t va, vaddr_t base, u_int32_t mask, u_int32_t bits)
{
	vaddr_t cca;

	cca = base | (va & mask);
	_reg_bclr_4(cca, bits);
}

/*
 * cache_sh4_op_8lines_32: (index-operation)
 *
 *	Clear the specified bits on 8 32-byte cache lines.
 *
 */
static __inline__ void
cache_sh4_op_8lines_32(vaddr_t va, vaddr_t base, u_int32_t mask, u_int32_t bits)
{
	__volatile__ u_int32_t *cca = (__volatile__ u_int32_t *)
	    (base | (va & mask));

	cca[ 0] &= ~bits;
	cca[ 8] &= ~bits;
	cca[16] &= ~bits;
	cca[24] &= ~bits;
	cca[32] &= ~bits;
	cca[40] &= ~bits;
	cca[48] &= ~bits;
	cca[56] &= ~bits;
}

void
sh4_icache_sync_all(void)
{
	vaddr_t va = 0;
	vaddr_t eva = sh_cache_size_icache;

	sh4_dcache_wbinv_all();

	RUN_P2;
	while (va < eva) {
		cache_sh4_op_8lines_32(va, SH4_CCIA, sh4_icache_mask, CCIA_V);
		va += 32 * 8;
	}
	RUN_P1;
}

void
sh4_icache_sync_range(vaddr_t va, vsize_t sz)
{
	vaddr_t ccia;
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	sh4_dcache_wbinv_range(va, (eva - va));

	RUN_P2;
	while (va < eva) {
		/* CCR.IIX has no effect on this entry specification */
		ccia = SH4_CCIA | CCIA_A | (va & sh4_icache_mask);
		_reg_write_4(ccia, va & CCIA_TAGADDR_MASK); /* V = 0 */
		va += 32;
	}
	RUN_P1;
}

void
sh4_icache_sync_range_index(vaddr_t va, vsize_t sz)
{
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	sh4_dcache_wbinv_range_index(va, eva - va);

	RUN_P2;
	while ((eva - va) >= (8 * 32)) {
		cache_sh4_op_8lines_32(va, SH4_CCIA, sh4_icache_mask, CCIA_V);
		va += 32 * 8;
	}

	while (va < eva) {
		cache_sh4_op_line_32(va, SH4_CCIA, sh4_icache_mask, CCIA_V);
		va += 32;
	}
	RUN_P1;
}

void
sh4_dcache_wbinv_all(void)
{
	vaddr_t va = 0;
	vaddr_t eva = sh_cache_size_dcache;

	RUN_P2;
	while (va < eva) {
		cache_sh4_op_8lines_32(va, SH4_CCDA, sh4_dcache_mask,
		    (CCDA_U | CCDA_V));
		va += 32 * 8;
	}
	RUN_P1;
}

void
sh4_dcache_wbinv_range(vaddr_t va, vsize_t sz)
{
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	while (va < eva) {
		__asm__ __volatile__("ocbp @%0" : : "r"(va));
		va += 32;
	}
}

void
sh4_dcache_wbinv_range_index(vaddr_t va, vsize_t sz)
{
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	RUN_P2;
	while ((eva - va) >= (8 * 32)) {
		cache_sh4_op_8lines_32(va, SH4_CCDA, sh4_dcache_mask,
		    (CCDA_U | CCDA_V));
		va += 32 * 8;
	}

	while (va < eva) {
		cache_sh4_op_line_32(va, SH4_CCDA, sh4_dcache_mask,
		    (CCDA_U | CCDA_V));
		va += 32;
	}
	RUN_P1;
}

void
sh4_dcache_inv_range(vaddr_t va, vsize_t sz)
{
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	while (va < eva) {
		__asm__ __volatile__("ocbi @%0" : : "r"(va));
		va += 32;
	}
}

void
sh4_dcache_wb_range(vaddr_t va, vsize_t sz)
{
	vaddr_t eva = round_line(va + sz);
	va = trunc_line(va);

	while (va < eva) {
		__asm__ __volatile__("ocbwb @%0" : : "r"(va));
		va += 32;
	}
}
