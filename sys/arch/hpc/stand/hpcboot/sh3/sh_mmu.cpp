/*	$NetBSD: sh_mmu.cpp,v 1.1.6.1 2002/02/11 20:08:00 jdolecek Exp $	*/

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

BOOL
MemoryManager_SHMMU::init(void)
{

	_kmode = SetKMode(1);

	_asid = VOLATILE_REF(MMUPTEH) & MMUPTEH_ASID_MASK;
	DPRINTF((TEXT("ASID = %d\n"), _asid));

	return TRUE;
}

MemoryManager_SHMMU::~MemoryManager_SHMMU(void)
{

	SetKMode(_kmode);
}

// get physical address from memory mapped TLB.
paddr_t
MemoryManager_SHMMU::searchPage(vaddr_t vaddr)
{
	u_int32_t vpn, idx, s, dum, aae, dae, entry_idx;
	paddr_t paddr = ~0;
	int way;

	vpn = vaddr & SH3_PAGE_MASK;
	// Windows CE uses VPN-only index-mode.
	idx = vaddr & MMUAA_VPN_MASK;

	// to avoid another TLB access, disable external interrupt.
	s = suspendIntr();

	do {
		// load target address page to TLB
		dum = VOLATILE_REF(vaddr);
		VOLATILE_REF(vaddr) = dum;

		for (way = 0; way < MMU_WAY; way++) {
			entry_idx = idx | (way << MMUAA_WAY_SHIFT);
			// inquire MMU address array.
			aae = VOLATILE_REF(MMUAA | entry_idx);
						      
			if (!(aae & MMUAA_D_VALID) ||
			    ((aae & MMUAA_D_ASID_MASK) != _asid) ||
			    (((aae | idx) & SH3_PAGE_MASK) != vpn))
				continue;

			// entry found.
			// inquire MMU data array to get its physical address.
			dae = VOLATILE_REF(MMUDA | entry_idx);
			paddr = (dae & SH3_PAGE_MASK) | (vaddr & ~SH3_PAGE_MASK);
			break;
		}
	} while (paddr == ~0);

	resumeIntr(s);

	return paddr;
}

