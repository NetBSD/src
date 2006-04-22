/*	$NetBSD: sh_mmu.cpp,v 1.5.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sh3/sh_arch.h>
#include <sh3/sh_mmu.h>

#include <sh3/cpu/sh3.h>
#include <sh3/cpu/sh4.h>

//
// Get physical address from memory mapped TLB.
// SH3 version. SH4 can't do this method. because address/data array must be
// accessed from P2.
//
paddr_t
MemoryManager_SHMMU::searchPage(vaddr_t vaddr)
{
	uint32_t vpn, idx, s, dum, aae, dae, entry_idx, asid;
	paddr_t paddr = ~0;
	int way, kmode;

	vpn = vaddr & SH3_PAGE_MASK;
	// Windows CE uses VPN-only index-mode.
	idx = vaddr & SH3_MMU_VPN_MASK;

	kmode = SetKMode(1);
	// Get current ASID
	asid = _reg_read_4(SH3_PTEH) & SH3_PTEH_ASID_MASK;

	// to avoid another TLB access, disable external interrupt.
	s = suspendIntr();

	do {
		// load target address page to TLB
		dum = _reg_read_4(vaddr);
		_reg_write_4(vaddr, dum);

		for (way = 0; way < SH3_MMU_WAY; way++) {
			entry_idx = idx | (way << SH3_MMU_WAY_SHIFT);
			// inquire MMU address array.
			aae = _reg_read_4(SH3_MMUAA | entry_idx);

			if (!(aae & SH3_MMU_D_VALID) ||
			    ((aae & SH3_MMUAA_D_ASID_MASK) != asid) ||
			    (((aae | idx) & SH3_PAGE_MASK) != vpn))
				continue;

			// entry found.
			// inquire MMU data array to get its physical address.
			dae = _reg_read_4(SH3_MMUDA | entry_idx);
			paddr = (dae & SH3_PAGE_MASK) | (vaddr & ~SH3_PAGE_MASK);
			break;
		}
	} while (paddr == ~0);

	resumeIntr(s);
	SetKMode(kmode);

	return paddr;
}

void
MemoryManager_SHMMU::CacheDump()
{
	static const char *able[] = {"dis", "en" };
	int write_through_p0_u0_p3;
	int write_through_p1;
	uint32_t r;
	int kmode;

	DPRINTF_SETUP();

	kmode = SetKMode(1);
	switch (SHArchitecture::cpu_type()) {
	default:
		DPRINTF((TEXT("unknown architecture.\n")));
		SetKMode(kmode);
		return;
	case 3:
		r = _reg_read_4(SH3_CCR);
		DPRINTF((TEXT("cache %Sabled"),
		    able[(r & SH3_CCR_CE ? 1 : 0)]));
		if (r & SH3_CCR_RA)
			DPRINTF((TEXT(" ram-mode")));

		write_through_p0_u0_p3 = r & SH3_CCR_WT;
		write_through_p1 = !(r & SH3_CCR_CB);
		break;
	case 4:
		r = _reg_read_4(SH4_CCR);
		DPRINTF((TEXT("I-cache %Sabled"),
		    able[(r & SH4_CCR_ICE) ? 1 : 0]));
		if (r & SH4_CCR_IIX)
			DPRINTF((TEXT(" index-mode ")));
		DPRINTF((TEXT(" D-cache %Sabled"),
		    able[(r & SH4_CCR_OCE) ? 1 : 0]));
		if (r & SH4_CCR_OIX)
			DPRINTF((TEXT(" index-mode")));
		if (r & SH4_CCR_ORA)
			DPRINTF((TEXT(" ram-mode")));

		write_through_p0_u0_p3 = r & SH4_CCR_WT;
		write_through_p1 = !(r & SH4_CCR_CB);
		break;
	}
	DPRINTF((TEXT(".")));

	// Write-through/back
	DPRINTF((TEXT(" P0, U0, P3 write-%S P1 write-%S\n"),
	    write_through_p0_u0_p3 ? "through" : "back",
	    write_through_p1 ? "through" : "back"));

	SetKMode(kmode);
}

void
MemoryManager_SHMMU::MMUDump()
{
#define	ON(x, c)	((x) & (c) ? '|' : '.')
	uint32_t r, e, a;
	int i, kmode;

	DPRINTF_SETUP();

	kmode = SetKMode(1);
	DPRINTF((TEXT("MMU:\n")));
	switch (SHArchitecture::cpu_type()) {
	default:
		DPRINTF((TEXT("unknown architecture.\n")));
		SetKMode(kmode);
		return;
	case 3:
		r = _reg_read_4(SH3_MMUCR);
		if (!(r & SH3_MMUCR_AT))
			goto disabled;

		// MMU configuration.
		DPRINTF((TEXT("%s index-mode, %s virtual storage mode\n"),
		    r & SH3_MMUCR_IX
		    ? TEXT("ASID + VPN") : TEXT("VPN only"),
		    r & SH3_MMUCR_SV ? TEXT("single") : TEXT("multiple")));

		// Dump TLB.
		DPRINTF((TEXT("---TLB---\n")));
		DPRINTF((TEXT("   VPN    ASID    PFN     VDCG PR SZ\n")));
		for (i = 0; i < SH3_MMU_WAY; i++) {
			DPRINTF((TEXT(" [way %d]\n"), i));
			for (e = 0; e < SH3_MMU_ENTRY; e++) {
				// address/data array common offset.
				a = (e << SH3_MMU_VPN_SHIFT) |
				    (i << SH3_MMU_WAY_SHIFT);

				r = _reg_read_4(SH3_MMUAA | a);
				DPRINTF((TEXT("0x%08x %3d"),
				    r & SH3_MMUAA_D_VPN_MASK,
				    r & SH3_MMUAA_D_ASID_MASK));
				r = _reg_read_4(SH3_MMUDA | a);
				DPRINTF((TEXT(" 0x%08x %c%c%c%c  %d %dK\n"),
				    r & SH3_MMUDA_D_PPN_MASK,
				    ON(r, SH3_MMUDA_D_V),
				    ON(r, SH3_MMUDA_D_D),
				    ON(r, SH3_MMUDA_D_C),
				    ON(r, SH3_MMUDA_D_SH),
				    (r & SH3_MMUDA_D_PR_MASK) >>
				    SH3_MMUDA_D_PR_SHIFT,
				    r & SH3_MMUDA_D_SZ ? 4 : 1));
			}
		}

		break;
	case 4:
		r = _reg_read_4(SH4_MMUCR);
		if (!(r & SH4_MMUCR_AT))
			goto disabled;
		DPRINTF((TEXT("%s virtual storage mode,"),
		    r & SH3_MMUCR_SV ? TEXT("single") : TEXT("multiple")));
		DPRINTF((TEXT(" SQ access: (priviledge%S)"),
		    r & SH4_MMUCR_SQMD ? "" : "/user"));
		DPRINTF((TEXT("\n")));
#if sample_code
		//
		// Memory mapped TLB accessing program must run on P2.
		// This is sample code.
		//
		// Dump ITLB
		DPRINTF((TEXT("---ITLB---\n")));
		for (i = 0; i < 4; i++) {
			e = i << SH4_ITLB_E_SHIFT;
			r = _reg_read_4(SH4_ITLB_AA | e);
			DPRINTF((TEXT("%08x %3d _%c"),
			    r & SH4_ITLB_AA_VPN_MASK,
			    r & SH4_ITLB_AA_ASID_MASK,
			    ON(r, SH4_ITLB_AA_V)));
			r = _reg_read_4(SH4_ITLB_DA1 | e);
			DPRINTF((TEXT(" %08x %c%c_%c_ %1d"),
			    r & SH4_ITLB_DA1_PPN_MASK,
			    ON(r, SH4_ITLB_DA1_V),
			    ON(r, SH4_ITLB_DA1_C),
			    ON(r, SH4_ITLB_DA1_SH),
			    (r & SH4_ITLB_DA1_PR) >> SH4_UTLB_DA1_PR_SHIFT
			    ));
			r = _reg_read_4(SH4_ITLB_DA2 | e);
			DPRINTF((TEXT(" %c%d\n"),
			    ON(r, SH4_ITLB_DA2_TC),
			    r & SH4_ITLB_DA2_SA_MASK));
		}
		// Dump UTLB
		DPRINTF((TEXT("---UTLB---\n")));
		for (i = 0; i < 64; i++) {
			e = i << SH4_UTLB_E_SHIFT;
			r = _reg_read_4(SH4_UTLB_AA | e);
			DPRINTF((TEXT("%08x %3d %c%c"),
			    r & SH4_UTLB_AA_VPN_MASK,
			    ON(r, SH4_UTLB_AA_D),
			    ON(r, SH4_UTLB_AA_V),
			    r & SH4_UTLB_AA_ASID_MASK));
			r = _reg_read_4(SH4_UTLB_DA1 | e);
			DPRINTF((TEXT(" %08x %c%c%c%c%c %1d"),
			    r & SH4_UTLB_DA1_PPN_MASK,
			    ON(r, SH4_UTLB_DA1_V),
			    ON(r, SH4_UTLB_DA1_C),
			    ON(r, SH4_UTLB_DA1_D),
			    ON(r, SH4_UTLB_DA1_SH),
			    ON(r, SH4_UTLB_DA1_WT),
			    (r & SH4_UTLB_DA1_PR_MASK) >> SH4_UTLB_DA1_PR_SHIFT
			    ));
			r = _reg_read_4(SH4_UTLB_DA2 | e);
			DPRINTF((TEXT(" %c%d\n"),
			    ON(r, SH4_UTLB_DA2_TC),
			    r & SH4_UTLB_DA2_SA_MASK));
		}
#endif //sample_code
		break;
	}

	SetKMode(kmode);
	return;

 disabled:
	DPRINTF((TEXT("disabled.\n")));
	SetKMode(kmode);
#undef ON
}
