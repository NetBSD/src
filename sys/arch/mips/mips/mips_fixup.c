/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: mips_fixup.c,v 1.1.2.5 2010/03/01 19:26:01 matt Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/cache.h>
#include <mips/mips3_pte.h>

#define	INSN_LUI_P(insn)	(((insn) >> 26) == 017)
#define	INSN_LW_P(insn)		(((insn) >> 26) == 043)
#define	INSN_LD_P(insn)		(((insn) >> 26) == 067)

#define INSN_LOAD_P(insn)	(INSN_LD_P(insn) || INSN_LW_P(insn))

bool
mips_fixup_exceptions(mips_fixup_callback_t callback)
{
	uint32_t * const start = (uint32_t *)MIPS_KSEG0_START;
	uint32_t * const end = start + (5 * 128) / sizeof(uint32_t);
	const int32_t addr = (intptr_t)&cpu_info_store;
	const size_t size = sizeof(cpu_info_store);
	uint32_t new_insns[2];
	uint32_t *lui_insnp = NULL;
	bool fixed = false;
	size_t lui_reg = 0;
	/*
	 * If this was allocated so that bit 15 of the value/address is 1, then
	 * %hi will add 1 to the immediate (or 0x10000 to the value loaded)
	 * to compensate for using a negative offset for the lower half of
	 * the value.
	 */
	const int32_t upper_addr = (addr & ~0xffff) + ((addr << 1) & 0x10000);

	KASSERT((addr & ~0xfff) == ((addr + size - 1) & ~0xfff));

	for (uint32_t *insnp = start; insnp < end; insnp++) {
		const uint32_t insn = *insnp;
		if (INSN_LUI_P(insn)) {
			const int32_t offset = insn << 16;
			lui_reg = (insn >> 16) & 31;
#ifdef DEBUG_VERBOSE
			printf("%s: %#x: insn %08x: lui r%zu, %%hi(%#x)",
			    __func__, (int32_t)(intptr_t)insnp,
			    insn, lui_reg, offset);
#endif
			if (upper_addr == offset) {
				lui_insnp = insnp;
#ifdef DEBUG_VERBOSE
				printf(" (maybe)");
#endif
			} else {
				lui_insnp = NULL;
			}
#ifdef DEBUG_VERBOSE
			printf("\n");
#endif
		} else if (lui_insnp != NULL && INSN_LOAD_P(insn)) {
			size_t base = (insn >> 21) & 31;
			size_t rt = (insn >> 16) & 31;
			int32_t load_addr = upper_addr + (int16_t)insn;
			if (addr <= load_addr
			    && load_addr < addr + size
			    && base == lui_reg
			    && rt == lui_reg) {
#ifdef DEBUG_VERBOSE
				printf("%s: %#x: insn %08x: %s r%zu, %%lo(%08x)(r%zu)\n",
				    __func__, (int32_t)(intptr_t)insnp,
				    insn, INSN_LW_P(insn) ? "lw" : "ld",
				    rt, load_addr, base);
#endif
				new_insns[0] = *lui_insnp;
				new_insns[1] = *insnp;
				if ((callback)(load_addr, new_insns)) {
					*lui_insnp = new_insns[0];
					*insnp = new_insns[1];
					fixed = true;
				}
				lui_insnp = NULL;
			} else if (rt == lui_reg) {
				lui_insnp = NULL;
			}
		}
	}

	if (fixed)
		mips_icache_sync_range((vaddr_t)start, end - start);
		
	return fixed;
}

bool
mips_fixup_zero_relative(int32_t load_addr, uint32_t new_insns[2])
{
	struct cpu_info * const ci = curcpu();
	struct pmap_tlb_info * const ti = ci->ci_tlb_info;

	KASSERT(MIPS_KSEG0_P(load_addr));
	KASSERT(!MIPS_CACHE_VIRTUAL_ALIAS);
#ifdef MULTIPROCESSOR
	KASSERT(CPU_IS_PRIMARY(ci));
#endif
	KASSERT((intptr_t)ci <= load_addr);
	KASSERT(load_addr < (intptr_t)(ci + 1));

	/*
	 * Use the load instruction as a prototype and it make use $0
	 * as base and the new negative offset.  The second instruction
	 * is a NOP.
	 */
	new_insns[0] =
	    (new_insns[1] & (0xfc1f0000|PAGE_MASK)) | (0xffff & ~PAGE_MASK);
	new_insns[1] = 0;
#ifdef DEBUG_VERBOSE
	printf("%s: %08x: insn#1 %08x: %s r%u, %d(r%u)\n",
	    __func__, (int32_t)load_addr, new_insns[0],
	    INSN_LW_P(new_insns[0]) ? "lw" : "ld",
	    (new_insns[0] >> 16) & 31,
	    (int16_t)new_insns[0],
	    (new_insns[0] >> 21) & 31);
#endif
	/*
	 * Construct the TLB_LO entry needed to map cpu_info_store.
	 */
	const uint32_t tlb_lo = MIPS3_PG_G|MIPS3_PG_V|MIPS3_PG_D
	    | mips3_paddr_to_tlbpfn(MIPS_KSEG0_TO_PHYS(trunc_page(load_addr)));

	/*
	 * Now allocate a TLB entry in the primary TLB for the mapping and
	 * enter the mapping into the TLB.
	 */
	TLBINFO_LOCK(ti);
	if (ci->ci_tlb_slot < 0) {
		ci->ci_tlb_slot = ti->ti_wired++;
		mips3_cp0_wired_write(ti->ti_wired);
		tlb_enter(ci->ci_tlb_slot, -PAGE_SIZE, tlb_lo);
	}
	TLBINFO_UNLOCK(ti);

	return true;
}

#define OPCODE_J		002
#define OPCODE_JAL		003

static inline void
fixup_mips_jump(uint32_t *insnp, const struct mips_jump_fixup_info *jfi)
{
	uint32_t insn = *insnp;

	KASSERT((insn >> (26+1)) == (OPCODE_J >> 1));
	KASSERT((insn << 6) == (jfi->jfi_stub << 6));

	insn ^= (jfi->jfi_stub ^ jfi->jfi_real);

	KASSERT((insn << 6) == (jfi->jfi_real << 6));

#ifdef DEBUG
#if 0
	int32_t va = ((intptr_t) insnp >> 26) << 26;
	printf("%s: %08x: [%08x] %s %08x -> [%08x] %s %08x\n",
	    __func__, (int32_t)(intptr_t)insnp,
	    insn, opcode == OPCODE_J ? "j" : "jal",
	    va | (jfi->jfo_stub << 2),
	    *insnp, opcode == OPCODE_J ? "j" : "jal",
	    va | (jfi->jfi_real << 2));
#endif
#endif
	*insnp = insn;
}

void
mips_fixup_stubs(uint32_t *start, uint32_t *end,
	const struct mips_jump_fixup_info *fixups,
	size_t nfixups)
{
	const uint32_t min_offset = fixups[0].jfi_stub;
	const uint32_t max_offset = fixups[nfixups-1].jfi_stub;
#ifdef DEBUG
	size_t fixups_done = 0;
	uint32_t cycles = (CPUISMIPS3 ? mips3_cp0_count_read() : 0);
#endif

#ifdef DIAGNOGSTIC
	/*
	 * Verify the fixup list is sorted from low stub to high stub.
	 */
	for (const struct mips_jump_fixup_info *jfi = fixups + 1;
	     jfi < fixups + nfixups; jfi++) {
		KASSERT(jfi[-1].jfi_stub < jfi[0].jfi_stub);
	}
#endif

	for (uint32_t *insnp = start; insnp < end; insnp++) {
		uint32_t insn = *insnp;
		uint32_t offset = insn & 0x03ffffff;
		uint32_t opcode = insn >> 26;

		/*
		 * First we check to see if this is a jump and whether it is
		 * within the range we are interested in.
		 */
		if ((opcode != OPCODE_J && opcode != OPCODE_JAL)
		    || offset < min_offset || max_offset < offset)
			continue;

		/*
		 * We know it's a jump, but does it match one we want to
		 * fixup?
		 */
		for (const struct mips_jump_fixup_info *jfi = fixups;
		     jfi < fixups + nfixups; jfi++) {
			/*
			 * The caller has sorted the fixup list from lowest
			 * stub to highest stub so if the current offset is
			 * less than the this fixup's stub offset, we know
			 * can't match anything else in the fixup list.
			 */
			if (jfi->jfi_stub > offset)
				break;

			if (jfi->jfi_stub == offset) {
				/*
				 * Yes, we need to fix it up.  Replace the old
				 * displacement with the real displacement.
				 */
				fixup_mips_jump(insnp, jfi);
#ifdef DEBUG
				fixups_done++;
#endif
				break;
			}
		}
	}

	if (sizeof(uint32_t [end - start]) > mips_cache_info.mci_picache_size)
		mips_icache_sync_all();
	else
		mips_icache_sync_range((vaddr_t)start,
		    sizeof(uint32_t [end - start]));

#ifdef DEBUG
	if (CPUISMIPS3)
		cycles = mips3_cp0_count_read() - cycles;
	printf("%s: %zu fixup%s done in %u cycles\n", __func__,
	    fixups_done, fixups_done == 1 ? "" : "s",
	    cycles);
#endif
}

void mips_cpu_switch_resume(struct lwp *l) __section(".stub");

void
mips_cpu_switch_resume(struct lwp *l)
{
	(*mips_locoresw.lsw_cpu_switch_resume)(l);
}

void
fixup_mips_cpu_switch_resume(void)
{
	extern uint32_t __cpu_switchto_fixup[];

	struct mips_jump_fixup_info fixup = {
		fixup_addr2offset(mips_cpu_switch_resume),
		fixup_addr2offset(mips_locoresw.lsw_cpu_switch_resume)
	};

	fixup_mips_jump(__cpu_switchto_fixup, &fixup);
}
