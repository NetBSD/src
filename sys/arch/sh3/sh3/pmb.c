/* $NetBSD: pmb.c,v 1.2 2020/08/04 02:09:57 uwe Exp $ */
/*
 * Copyright (c) 2020 Valery Ushakov
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmb.c,v 1.2 2020/08/04 02:09:57 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sh3/devreg.h>
#include <sh3/mmu_sh4.h>
#include <sh3/pmb.h>


static void st40_pmb_dump(int se_mode, int ub29);



void
st40_pmb_init(int product)
{
	uint32_t pascr = 0;	/* XXX: -Wuninitialized */
	int se_mode;
	int ub29;
	char bits[64];

	switch (product) {

	/* ST40-300 */
	case CPU_PRODUCT_STX7105: {
		pascr = _reg_read_4(ST40_PASCR);
		snprintb(bits, sizeof(bits), ST40_PASCR_BITS, pascr);
		printf("PMB: PASCR=%s\n", bits);

		se_mode = (pascr & ST40_PASCR_SE);
		ub29 = pascr & ST40_PASCR_UB_MASK;
		break;
	}

#if 0
	/* ST40-200, ST40-500 */
	case 0xdeadbabe: {
		uint32_t mmucr = _reg_read_4(SH4_MMUCR);

		se_mode = (mmucr & ST40_MMUCR_SE);
		ub29 = -1;
		break;
	}
#endif

	/* No PMB */
	default:
		return;
	}

	st40_pmb_dump(se_mode, ub29);
}


static void
st40_pmb_dump(int se_mode, int ub29)
{
	char bits[64] __unused;

	if (!se_mode) {
		printf("PMB: 29-bit mode\n");
		if (ub29 == -1)
			return;

		for (int area = 0; area < 8; ++area) {
			bool unbuffered = !!((uint32_t)ub29 & (1u << area));
			printf("PMB: area%d %s\n", area,
			       unbuffered ? "UB" : "--");
		}
		return;
	}

	printf("PMB: 32-bit mode\n");
	for (unsigned int i = 0; i < ST40_PMB_ENTRY; ++i) {
		uint32_t offset = (i << ST40_PMB_E_SHIFT);

		uint32_t addr = _reg_read_4(ST40_PMB_AA + offset);
#if 0
		snprintb(bits, sizeof(bits), ST40_PMB_AA_BITS, addr);
		printf("PMB[%02d] A=%s", i, bits);
#endif
		uint32_t data = _reg_read_4(ST40_PMB_DA + offset);
#if 0
		snprintb(bits, sizeof(bits), ST40_PMB_DA_BITS, data);
		printf(" D=%s\n", bits);
#endif
		if ((addr & ST40_PMB_AA_V) == 0)
			continue;

		uint32_t vpn = addr & ST40_PMB_AA_VPN_MASK;
		uint32_t ppn = data & ST40_PMB_DA_PPN_MASK;
		uint32_t szbits = data & ST40_PMB_DA_SZ_MASK;

		vpn >>= ST40_PMB_AA_VPN_SHIFT;
		ppn >>= ST40_PMB_DA_PPN_SHIFT;

		unsigned int sz = 0;
		switch (szbits) {
		case ST40_PMB_DA_SZ_16M:
			sz = 1;
			break;
		case ST40_PMB_DA_SZ_64M:
			sz = 4;
			break;
		case ST40_PMB_DA_SZ_128M:
			sz = 8;
			break;
		case ST40_PMB_DA_SZ_512M:
			sz = 32;
			break;
		}

		printf("PMB[%02d] = %02x..%02x -> %02x..%02x"
		       " %3uM %s %s %s\n",
		       i,
		       vpn, vpn + sz - 1,
		       ppn, ppn + sz - 1,
		       sz << 4,
		       data & ST40_PMB_DA_UB ? "UB" : "--",
		       data & ST40_PMB_DA_C  ?  "C" : "-",
		       data & ST40_PMB_DA_C  ?
		           (data & ST40_PMB_DA_WT ? "WT" : "CB") : "--");
	}
}
