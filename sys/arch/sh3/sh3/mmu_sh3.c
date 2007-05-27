/*	$NetBSD: mmu_sh3.c,v 1.12 2007/05/27 12:21:24 uwe Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mmu_sh3.c,v 1.12 2007/05/27 12:21:24 uwe Exp $");

#include <sys/param.h>

#include <sh3/pte.h>
#include <sh3/mmu.h>
#include <sh3/mmu_sh3.h>


void
sh3_mmu_start()
{

	/* Zero clear all TLB entries */
	sh3_tlb_invalidate_all();

	/* Set current ASID to 0 - kernel */
	sh_tlb_set_asid(0);

	/* Flush TLB (TF) and enable address translation (AT) */
	_reg_write_4(SH3_MMUCR, SH3_MMUCR_TF | SH3_MMUCR_AT);
}


void
sh3_tlb_invalidate_all()
{
	uint32_t idx, a;
	int i, way;

	for (i = 0; i < SH3_MMU_ENTRY; ++i) {
		idx = i << SH3_MMU_VPN_SHIFT;

		for (way = 0; way < SH3_MMU_WAY; ++way) {
			a = idx | (way << SH3_MMU_WAY_SHIFT);

			_reg_write_4(SH3_MMUAA | a, 0);
			_reg_write_4(SH3_MMUDA | a, 0);
		}
	}
}


void
sh3_tlb_invalidate_asid(int asid)
{
	uint32_t idx, aa;
	int i, way;

	for (i = 0; i < SH3_MMU_ENTRY; ++i) {
		idx = i << SH3_MMU_VPN_SHIFT;

		for (way = 0; way < SH3_MMU_WAY; ++way) {
			aa = SH3_MMUAA | idx | (way << SH3_MMU_WAY_SHIFT);

			if ((_reg_read_4(aa) & SH3_MMUAA_D_ASID_MASK) == asid)
				_reg_write_4(aa, 0);
		}
	}
}


void
sh3_tlb_invalidate_addr(int asid, vaddr_t va)
{
	uint32_t match, idx, aa, entry;
	int way;

	/* What we are looking for in the address array */
	match = (va & SH3_MMUAA_D_VPN_MASK_4K) | asid;

	/* Where in the address array this VA is located - bits [16:12] */
	idx = va & SH3_MMU_VPN_MASK;

	/* Check each way - bits [9:8] */
	for (way = 0; way < SH3_MMU_WAY; ++way) {
		aa = SH3_MMUAA | idx | (way << SH3_MMU_WAY_SHIFT);

		entry = _reg_read_4(aa)
		    & (SH3_MMUAA_D_VPN_MASK_4K | SH3_MMUAA_D_ASID_MASK);

		if (entry == match) {
			_reg_write_4(aa, 0);
			break;
		}
	}
}


void
sh3_tlb_update(int asid, vaddr_t va, uint32_t pte)
{
	static unsigned int rc = 0;

	uint32_t match, idx, a, entry;
	uint32_t freea, matcha;
	uint32_t newa, newd;
	int way;
	int s;

	KDASSERT(asid < 256 && (pte & ~PGOFSET) != 0 && va != 0);


	if ((pte & PG_V) == 0) {
		sh3_tlb_invalidate_addr(asid, va);
		return;
	}


	/*
	 * Simple approach is to invalidate + ldtlb, but ldtlb uses
	 * MMUCR.RC to select the way to overwrite, and RC is only
	 * meaningful immediately after TLB exception, so ldtlb here
	 * would update some random way, e.g. a valid way even if
	 * there is an invalid way we could use instead.
	 *
	 * Nano-optimization: as invalidatation needs to loop over
	 * ways anyway, just loop over all of them, noting if there's
	 * either an existing entry for this VA that we can update or
	 * an invalid way we can use.
	 */

	/* What we are looking for in the address array */
	match = (va & SH3_MMUAA_D_VPN_MASK_4K) | asid;

	/* Where in the address array this VA is located - bits [16:12] */
	idx = va & SH3_MMU_VPN_MASK;

	newa = match | SH3_MMU_D_VALID;
	newd = pte & PG_HW_BITS;

	matcha = freea = ~0;


	s = splhigh();

	/* Check each way - bits [9:8] */
	for (way = 0; way < SH3_MMU_WAY; ++way) {
		a = idx | (way << SH3_MMU_WAY_SHIFT);

		entry = _reg_read_4(SH3_MMUAA | a);

		if ((entry & SH3_MMU_D_VALID) == 0)
			freea = a;

		entry &= (SH3_MMUAA_D_VPN_MASK_4K | SH3_MMUAA_D_ASID_MASK);
		if (entry == match) {
			matcha = a;
			break;
		}
	}

	if ((int)matcha >= 0)	/* there's an existing entry, update it */
		a = matcha;
	else if ((int)freea >= 0) /* there's an invalid way, overwrite it */
		a = freea;
	else {			/* no match, all ways are valid */
		a = idx | (rc << SH3_MMU_WAY_SHIFT);
		rc = (rc + 1) % SH3_MMU_WAY;
	}

	_reg_write_4(SH3_MMUAA | a, newa);
	_reg_write_4(SH3_MMUDA | a, newd);

	splx(s);
}
