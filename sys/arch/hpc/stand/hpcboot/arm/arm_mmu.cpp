/* -*-C++-*-	$NetBSD: arm_mmu.cpp,v 1.2 2001/05/08 18:51:24 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <arm/arm_mmu.h>
#include <console.h>

MemoryManager_ArmMMU::MemoryManager_ArmMMU(Console *&cons,
    size_t pagesize)
	: MemoryManager(cons, pagesize)
{
	DPRINTF((TEXT("Use ARM software MMU.\n")));
}

MemoryManager_ArmMMU::~MemoryManager_ArmMMU(void)
{
	SetKMode(_kmode);
}

BOOL
MemoryManager_ArmMMU::init(void)
{
	u_int32_t reg;

	_kmode = SetKMode(1);
	// Check system mode
	if ((GetCPSR() & 0x1f) != 0x1f) {
		DPRINTF((TEXT("not System mode\n")));
		return FALSE;
	}
	// Domain access control.(full access)
	SetCop15Reg3(~0);

	// Get Translation table base.
	reg = GetCop15Reg2();
	_table_base =  reg & ARM_MMU_TABLEBASE_MASK;
	DPRINTF((TEXT("page directory address=0x%08x->0x%08x(0x%08x)\n"),
	    _table_base, readPhysical4(_table_base), reg));

	return TRUE;
}

paddr_t
MemoryManager_ArmMMU::searchPage(vaddr_t vaddr)
{
	paddr_t daddr, paddr = ~0;
	u_int32_t desc1, desc2;

	// set marker.
	memset(LPVOID(vaddr), 0xa5, _page_size);

	// PID virtual address mapping.
	DPRINTF((TEXT("Virtual Address 0x%08x"), vaddr));
	vaddr |= GetCop15Reg13();
	DPRINTF((TEXT("(+PID)-> 0x%08x\n"), vaddr));

	daddr = _table_base | ARM_MMU_TABLEINDEX(vaddr);
	desc1 = readPhysical4(daddr);
	DPRINTF((TEXT("1st level descriptor 0x%08x(addr 0x%08x)\n"),
	    desc1, daddr));
  
	switch(ARM_MMU_LEVEL1DESC_TRANSLATE_TYPE(desc1)) {
	default:
		DPRINTF((TEXT("1st level descriptor fault.\n")));
		break;
	case ARM_MMU_LEVEL1DESC_TRANSLATE_SECTION:
		paddr = ARM_MMU_SECTION_BASE(desc1) |
		    ARM_MMU_VADDR_SECTION_INDEX(vaddr);
		DPRINTF((TEXT("section Physical Address 0x%08x\n"), paddr));
		break;
	case ARM_MMU_LEVEL1DESC_TRANSLATE_PAGE:
		DPRINTF((TEXT("-> Level2 page descriptor.\n")));
		daddr = ARM_MMU_PTE_BASE(desc1) |
		    ARM_MMU_VADDR_PTE_INDEX(vaddr);
		desc2 = readPhysical4(daddr);
		DPRINTF((TEXT("2nd level descriptor 0x%08x(addr 0x%08x)\n"),
		    desc2, daddr));
		switch(desc2 & 0x3) {
		default:
			DPRINTF((TEXT("2nd level descriptor fault.\n")));
			break;
		case 2: // 4Kpage
			paddr =(desc2 & 0xfffff000) |(vaddr & 0x00000fff);
			break;
		case 1: // 64Kpage
			paddr =(desc2 & 0xffff0000) |(vaddr & 0x0000ffff);
			break;
		}
		break;
	}
      
	return paddr;
}
