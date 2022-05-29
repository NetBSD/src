/* $NetBSD: db_interface.c,v 1.18 2022/05/29 16:39:22 ryo Exp $ */

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.18 2022/05/29 16:39:22 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_ddb.h>
#include <uvm/uvm_prot.h>
#ifdef __HAVE_PMAP_PV_TRACK
#include <uvm/pmap/pmap_pvt.h>
#endif

#include <aarch64/armreg.h>
#include <aarch64/db_machdep.h>
#include <aarch64/locore.h>
#include <aarch64/machdep.h>
#include <aarch64/pmap.h>

#include <arm/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>

#include <dev/cons.h>

db_regs_t ddb_regs;

static bool
db_accessible_address(vaddr_t addr, bool readonly)
{
	register_t s;
	uint64_t par;
	int space;

	space = aarch64_addressspace(addr);
	if (space != AARCH64_ADDRSPACE_LOWER &&
	    space != AARCH64_ADDRSPACE_UPPER)
		return false;

	s = daif_disable(DAIF_I|DAIF_F);

	switch (aarch64_addressspace(addr)) {
	case AARCH64_ADDRSPACE_LOWER:
		if (readonly)
			reg_s1e0r_write(addr);
		else
			reg_s1e0w_write(addr);
		break;
	case AARCH64_ADDRSPACE_UPPER:
		if (readonly)
			reg_s1e1r_write(addr);
		else
			reg_s1e1w_write(addr);
		break;
	}
	isb();
	par = reg_par_el1_read();

	reg_daif_write(s);

	return ((par & PAR_F) == 0);
}

void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	vaddr_t lastpage = -1;
	const char *src;

	for (src = (const char *)addr; size > 0;) {
		const vaddr_t va = (vaddr_t)src;
		uintptr_t tmp;

		if (lastpage != atop(va) && !db_accessible_address(va, true)) {
			db_printf("address %p is invalid\n", src);
			memset(data, 0, size);	/* stubs are filled by zero */
			return;
		}
		lastpage = atop(va);

		if (aarch64_pan_enabled)
			reg_pan_write(0); /* disable PAN */

		tmp = (uintptr_t)src | (uintptr_t)data;
		if (size >= 8 && (tmp & 7) == 0) {
			*(uint64_t *)data = *(const uint64_t *)src;
			src += 8;
			data += 8;
			size -= 8;
		} else if (size >= 4 && (tmp & 3) == 0) {
			*(uint32_t *)data = *(const uint32_t *)src;
			src += 4;
			data += 4;
			size -= 4;
		} else if (size >= 2 && (tmp & 1) == 0) {
			*(uint16_t *)data = *(const uint16_t *)src;
			src += 2;
			data += 2;
			size -= 2;
		} else {
			*data++ = *src++;
			size--;
		}

		if (aarch64_pan_enabled)
			reg_pan_write(1); /* enable PAN */
	}
}

static void
db_write_text(vaddr_t addr, size_t size, const char *data)
{
	pt_entry_t *ptep, pte;
	size_t s;

	/*
	 * consider page boundary, and
	 * it works even if kernel_text is mapped with L2 or L3.
	 */
	if (atop(addr) != atop(addr + size - 1)) {
		s = PAGE_SIZE - (addr & PAGE_MASK);
		db_write_text(addr, s, data);
		addr += s;
		size -= s;
		data += s;
	}
	while (size > 0) {
		ptep = kvtopte(addr);
		KASSERT(ptep != NULL);

		/*
		 * change to writable.  it is required to keep execute permission.
		 * because if the block/page to which the target address belongs is
		 * the same as the block/page to which this function belongs, then
		 * if PROT_EXECUTE is dropped and TLB is invalidated, the program
		 * will stop...
		 */
		/* old pte is returned by pmap_kvattr */
		pte = pmap_kvattr(ptep, VM_PROT_EXECUTE|VM_PROT_READ|VM_PROT_WRITE);
		aarch64_tlbi_all();

		s = size;
		if (size > PAGE_SIZE)
			s = PAGE_SIZE;

		memcpy((void *)addr, data, s);
		cpu_icache_sync_range(addr, size);

		/* restore pte */
		*ptep = pte;
		aarch64_tlbi_all();

		addr += s;
		size -= s;
		data += s;
	}
}

void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	vaddr_t kernstart, datastart;
	vaddr_t lastpage = -1;
	char *dst;

	/* if readonly page, require changing attribute to write */
	extern char __kernel_text[], __data_start[];
	kernstart = trunc_page((vaddr_t)__kernel_text);
	datastart = trunc_page((vaddr_t)__data_start);
	if (kernstart <= addr && addr < datastart) {
		size_t s;

		s = datastart - addr;
		if (s > size)
			s = size;
		db_write_text(addr, s, data);
		addr += s;
		size -= s;
		data += s;
	}

	for (dst = (char *)addr; size > 0;) {
		const vaddr_t va = (vaddr_t)dst;
		uintptr_t tmp;

		if (lastpage != atop(va) && !db_accessible_address(va, false)) {
			db_printf("address %p is invalid\n", dst);
			return;
		}
		lastpage = atop(va);

		if (aarch64_pan_enabled)
			reg_pan_write(0); /* disable PAN */

		tmp = (uintptr_t)dst | (uintptr_t)data;
		if (size >= 8 && (tmp & 7) == 0) {
			*(uint64_t *)dst = *(const uint64_t *)data;
			dst += 8;
			data += 8;
			size -= 8;
		} else if (size >= 4 && (tmp & 3) == 0) {
			*(uint32_t *)dst = *(const uint32_t *)data;
			dst += 4;
			data += 4;
			size -= 4;
		} else if (size >= 2 && (tmp & 1) == 0) {
			*(uint16_t *)dst = *(const uint16_t *)data;
			dst += 2;
			data += 2;
			size -= 2;
		} else {
			*dst++ = *data++;
			size--;
		}

		if (aarch64_pan_enabled)
			reg_pan_write(1); /* enable PAN */
	}
}

/*
 * return register value of $X0..$X30, $SP or 0($XZR)
 */
static uint64_t
db_fetch_reg(unsigned int reg, db_regs_t *regs, bool use_sp)
{
	if (reg >= 32)
		panic("db_fetch_reg: botch");

	if (reg == 31) {
		/* $SP or $XZR */
		return use_sp ? regs->tf_sp : 0;
	}
	return regs->tf_reg[reg];
}

static inline uint64_t
SignExtend(int bitwidth, uint64_t imm, unsigned int multiply)
{
	const uint64_t signbit = ((uint64_t)1 << (bitwidth - 1));
	const uint64_t immmax = signbit << 1;

	if (imm & signbit)
		imm -= immmax;
	return imm * multiply;
}

db_addr_t
db_branch_taken(db_expr_t inst, db_addr_t pc, db_regs_t *regs)
{
	LE32TOH(inst);

#define INSN_FMT_RN(insn)		(((insn) >> 5) & 0x1f)
#define INSN_FMT_IMM26(insn)	((insn) & 0x03ffffff)
#define INSN_FMT_IMM19(insn)	(((insn) >> 5) & 0x7ffff)
#define INSN_FMT_IMM14(insn)	(((insn) >> 5) & 0x3fff)

	if ((inst & 0xfffffc1f) == 0xd65f0000 ||	/* ret xN */
	    (inst & 0xfffffc1f) == 0xd63f0000 ||	/* blr xN */
	    (inst & 0xfffffc1f) == 0xd61f0000) {	/* br xN */
		return db_fetch_reg(INSN_FMT_RN(inst), regs, false);
	}

	if ((inst & 0xfc000000) == 0x94000000 ||	/* bl imm */
	    (inst & 0xfc000000) == 0x14000000) {	/* b imm */
		return SignExtend(26, INSN_FMT_IMM26(inst), 4) + pc;
	}

	if ((inst & 0xff000010) == 0x54000000 ||	/* b.cond */
	    (inst & 0x7f000000) == 0x35000000 ||	/* cbnz */
	    (inst & 0x7f000000) == 0x34000000) {	/* cbz */
		return SignExtend(19, INSN_FMT_IMM19(inst), 4) + pc;
	}

	if ((inst & 0x7f000000) == 0x37000000 ||	/* tbnz */
	    (inst & 0x7f000000) == 0x36000000) {	/* tbz */
		return SignExtend(14, INSN_FMT_IMM14(inst), 4) + pc;
	}

	panic("branch_taken: botch");
}

bool
db_inst_unconditional_flow_transfer(db_expr_t inst)
{
	LE32TOH(inst);

	if ((inst & 0xfffffc1f) == 0xd65f0000 ||	/* ret xN */
	    (inst & 0xfc000000) == 0x94000000 ||	/* bl */
	    (inst & 0xfffffc1f) == 0xd63f0000 ||	/* blr */
	    (inst & 0xfc000000) == 0x14000000 ||	/* b imm */
	    (inst & 0xfffffc1f) == 0xd61f0000)		/* br */
		return true;

#define INSN_FMT_COND(insn)	((insn) & 0xf)
#define CONDITION_AL	14

	if ((inst & 0xff000010) == 0x54000000 &&	/* b.cond */
	    INSN_FMT_COND(inst) == CONDITION_AL)	/* always? */
		return true;

	return false;
}

void
db_pte_print(pt_entry_t pte, int level,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	if (pte == 0) {
		pr(" UNUSED\n");
		return;
	}

	pr(" %s", (pte & LX_VALID) ? "VALID" : "**INVALID**");

	if (level == 0 ||
	    (level == 1 && l1pde_is_table(pte)) ||
	    (level == 2 && l2pde_is_table(pte))) {

		/* L0/L1/L2 TABLE */
		if (level == 0 && (pte & LX_TYPE) != LX_TYPE_TBL)
			pr(" **ILLEGAL TYPE**"); /* L0 doesn't support block */
		else
			pr(" L%d-TABLE", level);

		pr(", PA=%lx", l0pde_pa(pte));

		if (pte & LX_TBL_NSTABLE)
			pr(", NSTABLE");
		if (pte & LX_TBL_APTABLE)
			pr(", APTABLE");
		if (pte & LX_TBL_UXNTABLE)
			pr(", UXNTABLE");
		if (pte & LX_TBL_PXNTABLE)
			pr(", PXNTABLE");

	} else if ((level == 1 && l1pde_is_block(pte)) ||
	    (level == 2 && l2pde_is_block(pte)) ||
	    level == 3) {

		/* L1/L2 BLOCK or L3 PAGE */
		switch (level) {
		case 1:
			pr(" L1(1G)-BLOCK");
			break;
		case 2:
			pr(" L2(2M)-BLOCK");
			break;
		case 3:
			pr(" %s", l3pte_is_page(pte) ?
			    "L3(4K)-PAGE" : "**ILLEGAL TYPE**");
			break;
		}

		pr(", PA=%lx", l3pte_pa(pte));

		pr(", %s", (pte & LX_BLKPAG_UXN) ? "UXN" : "UX");
		pr(", %s", (pte & LX_BLKPAG_PXN) ? "PXN" : "PX");

		if (pte & LX_BLKPAG_CONTIG)
			pr(", CONTIG");

		pr(", %s", (pte & LX_BLKPAG_NG) ? "nG" : "G");
		pr(", %s", (pte & LX_BLKPAG_AF) ?
		    "accessible" :
		    "**fault** ");

		switch (pte & LX_BLKPAG_SH) {
		case LX_BLKPAG_SH_NS:
			pr(", SH_NS");
			break;
		case LX_BLKPAG_SH_OS:
			pr(", SH_OS");
			break;
		case LX_BLKPAG_SH_IS:
			pr(", SH_IS");
			break;
		default:
			pr(", SH_??");
			break;
		}

		pr(", %s", (pte & LX_BLKPAG_AP_RO) ? "RO" : "RW");
		pr(", %s", (pte & LX_BLKPAG_APUSER) ? "EL0" : "EL1");
		pr(", %s", (pte & LX_BLKPAG_NS) ? "NS" : "secure");

		switch (pte & LX_BLKPAG_ATTR_MASK) {
		case LX_BLKPAG_ATTR_NORMAL_WB:
			pr(", WB");
			break;
		case LX_BLKPAG_ATTR_NORMAL_NC:
			pr(", NC");
			break;
		case LX_BLKPAG_ATTR_NORMAL_WT:
			pr(", WT");
			break;
		case LX_BLKPAG_ATTR_DEVICE_MEM:
			pr(", DEV");
			break;
		case LX_BLKPAG_ATTR_DEVICE_MEM_SO:
			pr(", DEV(SO)");
			break;
		default:
			pr(", ATTR(%lu)", __SHIFTOUT(pte, LX_BLKPAG_ATTR_INDX));
			break;
		}

		if (pte & LX_BLKPAG_OS_0)
			pr(", " PMAP_PTE_OS0);
		if (pte & LX_BLKPAG_OS_1)
			pr(", " PMAP_PTE_OS1);
		if (pte & LX_BLKPAG_OS_2)
			pr(", " PMAP_PTE_OS2);
		if (pte & LX_BLKPAG_OS_3)
			pr(", " PMAP_PTE_OS3);
	} else {
		pr(" **ILLEGAL TYPE**");
	}
	pr("\n");
}

void
db_pteinfo(vaddr_t va, void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct vm_page *pg;
	bool user;
	pd_entry_t *l0, *l1, *l2, *l3;
	pd_entry_t pde;
	pt_entry_t pte;
	uint64_t ttbr;
	paddr_t pa;
	unsigned int idx;

	switch (aarch64_addressspace(va)) {
	case AARCH64_ADDRSPACE_UPPER:
		user = false;
		ttbr = reg_ttbr1_el1_read();
		break;
	case AARCH64_ADDRSPACE_LOWER:
		user = true;
		ttbr = reg_ttbr0_el1_read();
		break;
	default:
		pr("illegal address space\n");
		return;
	}
	pa = ttbr & TTBR_BADDR;
	l0 = (pd_entry_t *)AARCH64_PA_TO_KVA(pa);

	/*
	 * traverse L0 -> L1 -> L2 -> L3 table
	 */
	pr("TTBR%d=%016"PRIx64", pa=%016"PRIxPADDR", va=%p",
	    user ? 0 : 1, ttbr, pa, l0);
	pr(", input-va=%016"PRIxVADDR
	    ", L0-index=%ld, L1-index=%ld, L2-index=%ld, L3-index=%ld\n",
	    va,
	    (va & L0_ADDR_BITS) >> L0_SHIFT,
	    (va & L1_ADDR_BITS) >> L1_SHIFT,
	    (va & L2_ADDR_BITS) >> L2_SHIFT,
	    (va & L3_ADDR_BITS) >> L3_SHIFT);

	idx = l0pde_index(va);
	pde = l0[idx];

	pr("L0[%3d]=%016"PRIx64":", idx, pde);
	db_pte_print(pde, 0, pr);

	if (!l0pde_valid(pde))
		return;

	l1 = (pd_entry_t *)AARCH64_PA_TO_KVA(l0pde_pa(pde));
	idx = l1pde_index(va);
	pde = l1[idx];

	pr(" L1[%3d]=%016"PRIx64":", idx, pde);
	db_pte_print(pde, 1, pr);

	if (!l1pde_valid(pde) || l1pde_is_block(pde))
		return;

	l2 = (pd_entry_t *)AARCH64_PA_TO_KVA(l1pde_pa(pde));
	idx = l2pde_index(va);
	pde = l2[idx];

	pr("  L2[%3d]=%016"PRIx64":", idx, pde);
	db_pte_print(pde, 2, pr);

	if (!l2pde_valid(pde) || l2pde_is_block(pde))
		return;

	l3 = (pd_entry_t *)AARCH64_PA_TO_KVA(l2pde_pa(pde));
	idx = l3pte_index(va);
	pte = l3[idx];

	pr("   L3[%3d]=%016"PRIx64":", idx, pte);
	db_pte_print(pte, 3, pr);

	pa = l3pte_pa(pte);
	pg = PHYS_TO_VM_PAGE(pa);

	if (pg != NULL) {
		uvm_page_printit(pg, false, pr);

		pmap_db_mdpg_print(pg, pr);
	} else {
#ifdef __HAVE_PMAP_PV_TRACK
		if (pmap_pv_tracked(pa))
			pr("PV tracked");
		else
			pr("No VM_PAGE or PV tracked");
#else
		pr("no VM_PAGE\n");
#endif
	}
}

static void
dump_ln_table(bool countmode, pd_entry_t *pdp, int level, int lnindex,
    vaddr_t va, void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct vm_page *pg;
	pd_entry_t pde;
	paddr_t pa;
	int i, n;
	const char *spaces[4] = { " ", "  ", "   ", "    " };
	const char *spc = spaces[level];

	pa = AARCH64_KVA_TO_PA((vaddr_t)pdp);
	pg = PHYS_TO_VM_PAGE(pa);

	if (pg == NULL) {
		pr("%sL%d: pa=%lx pg=NULL\n", spc, level, pa);
	} else {
		pr("%sL%d: pa=%lx pg=%p", spc, level, pa, pg);
	}

	for (i = n = 0; i < Ln_ENTRIES; i++) {
		db_read_bytes((db_addr_t)&pdp[i], sizeof(pdp[i]), (char *)&pde);
		if (lxpde_valid(pde)) {
			if (!countmode)
				pr("%sL%d[%3d] %3dth, va=%016lx, pte=%016lx:",
				    spc, level, i, n, va, pde);
			n++;

			if ((level != 0 && level != 3 && l1pde_is_block(pde)) ||
			    (level == 3 && l3pte_is_page(pde))) {
				if (!countmode)
					db_pte_print(pde, level, pr);
			} else if (level != 3 && l1pde_is_table(pde)) {
				if (!countmode)
					db_pte_print(pde, level, pr);
				pa = l0pde_pa(pde);
				dump_ln_table(countmode,
				    (pd_entry_t *)AARCH64_PA_TO_KVA(pa),
				    level + 1, i, va, pr);
			} else {
				if (!countmode)
					db_pte_print(pde, level, pr);
			}
		}

		switch (level) {
		case 0:
			va += L0_SIZE;
			break;
		case 1:
			va += L1_SIZE;
			break;
		case 2:
			va += L2_SIZE;
			break;
		case 3:
			va += L3_SIZE;
			break;
		}
	}

	if (level == 0)
		pr("L0 has %d entries\n", n);
	else
		pr("%sL%d[%3d] has %d L%d entries\n", spaces[level - 1],
		    level - 1, lnindex, n, level);
}

static void
db_dump_l0table(bool countmode, pd_entry_t *pdp, vaddr_t va_base,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	dump_ln_table(countmode, pdp, 0, 0, va_base, pr);
}

void
db_ttbrdump(bool countmode, vaddr_t va,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	struct pmap *pm, _pm;

	pm = (struct pmap *)va;
	db_read_bytes((db_addr_t)va, sizeof(_pm), (char *)&_pm);

	pr("pmap=%p\n", pm);
	pmap_db_pmap_print(&_pm, pr);

	db_dump_l0table(countmode, pmap_l0table(pm),
	    (pm == pmap_kernel()) ? 0xffff000000000000UL : 0, pr);
}
