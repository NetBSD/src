/*	$NetBSD: mips_fixup.c,v 1.10.10.1 2017/12/03 11:36:28 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mips_fixup.c,v 1.10.10.1 2017/12/03 11:36:28 jdolecek Exp $");

#include "opt_mips3_wired.h"
#include "opt_multiprocessor.h"
#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/cache.h>
#include <mips/mips3_pte.h>
#include <mips/regnum.h>
#include <mips/mips_opcode.h>

bool
mips_fixup_exceptions(mips_fixup_callback_t callback, void *arg)
{
#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	int32_t ebase = mipsNN_cp0_ebase_read();
	uint32_t *start;
	if (ebase == mips_options.mips_cpu_id
	    || (ebase & __BITS(31,30)) != __BIT(31)) {
		start = (uint32_t *)MIPS_KSEG0_START;
	} else {
		start = (uint32_t *)(intptr_t)(ebase & ~MIPS_EBASE_CPUNUM);
	}
#else
	uint32_t * const start = (uint32_t *)MIPS_KSEG0_START;
#endif
	uint32_t * const end = start + (5 * 128) / sizeof(uint32_t);
	const int32_t addr = (intptr_t)&cpu_info_store;
	const size_t size = sizeof(cpu_info_store);
	uint32_t new_insns[2];
	uint32_t *lui_insnp = NULL;
	int32_t lui_offset = 0;
	bool fixed = false;
	size_t lui_reg = 0;
#ifdef DEBUG_VERBOSE
	printf("%s: fixing %p..%p\n", __func__, start, end);
#endif
	/*
	 * If this was allocated so that bit 15 of the value/address is 1, then
	 * %hi will add 1 to the immediate (or 0x10000 to the value loaded)
	 * to compensate for using a negative offset for the lower half of
	 * the value.
	 */
	const int32_t upper_start = (addr + 32768) & ~0xffff;
	const int32_t upper_end = (addr + size - 1 + 32768) & ~0xffff;

#ifndef MIPS64_OCTEON
	KASSERT((addr & ~0xfff) == ((addr + size - 1) & ~0xfff));
#endif

	uint32_t lui_insn = 0;
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
			KASSERT(lui_reg == _R_K0 || lui_reg == _R_K1);
			if (upper_start == offset || upper_end == offset) {
				lui_insnp = insnp;
				lui_insn = insn;
				lui_offset = offset;
#ifdef DEBUG_VERBOSE
				printf(" (maybe)");
#endif
			} else {
				lui_insnp = NULL;
				lui_insn = 0;
				lui_offset = 0;
			}
#ifdef DEBUG_VERBOSE
			printf("\n");
#endif
		} else if (lui_insn != 0
			   && (INSN_LOAD_P(insn) || INSN_STORE_P(insn))) {
			size_t base = (insn >> 21) & 31;
#if defined(DIAGNOSTIC) || defined(DEBUG_VERBOSE)
			size_t rt = (insn >> 16) & 31;
#endif
			int32_t load_addr = lui_offset + (int16_t)insn;
			if (addr <= load_addr
			    && load_addr < addr + size
			    && base == lui_reg) {
				KASSERT(rt == _R_K0 || rt == _R_K1);
#ifdef DEBUG_VERBOSE
				printf("%s: %#x: insn %08x: %s r%zu, %%lo(%08x)(r%zu)\n",
				    __func__, (int32_t)(intptr_t)insnp,
				    insn,
				    INSN_LOAD_P(insn)
					? INSN_LW_P(insn) ? "lw" : "ld"
					: INSN_SW_P(insn) ? "sw" : "sd",
				    rt, load_addr, base);
#endif
				new_insns[0] = lui_insn;
				new_insns[1] = *insnp;
				if ((callback)(load_addr, new_insns, arg)) {
					if (lui_insnp) {
						*lui_insnp = new_insns[0];
						*insnp = new_insns[1];
					} else if (new_insns[1] == 0) {
						*insnp = new_insns[0];
					} else {
						*insnp = new_insns[1];
					}
					fixed = true;
				}
				lui_insnp = NULL;
			}
		} else if (INSN_LOAD_P(insn)) {
			/*
			 * If we are loading the register used in the LUI,
			 * then that LUI is meaningless now.
			 */
			size_t rt = (insn >> 16) & 31;
			if (lui_reg == rt)
				lui_insn = 0;
		}
	}

	if (fixed)
		mips_icache_sync_range((intptr_t)start,
		   sizeof(start[0]) * (end - start));

	return fixed;
}

#ifdef MIPS3_PLUS
bool
mips_fixup_zero_relative(int32_t load_addr, uint32_t new_insns[2], void *arg)
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
	KASSERT(MIPS_HAS_R4K_MMU);

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
	    INSN_LOAD_P(new_insns[0])
		? INSN_LW_P(new_insns[0]) ? "lw" : "ld"
		: INSN_LW_P(new_insns[0]) ? "sw" : "sd",
	    (new_insns[0] >> 16) & 31,
	    (int16_t)new_insns[0],
	    (new_insns[0] >> 21) & 31);
#endif
	/*
	 * Construct the TLB_LO entry needed to map cpu_info_store.
	 */

	/*
	 * Now allocate a TLB entry in the primary TLB for the mapping and
	 * enter the mapping into the TLB.
	 */
	TLBINFO_LOCK(ti);
	if (ci->ci_tlb_slot < 0) {
		uint32_t tlb_lo = MIPS3_PG_G|MIPS3_PG_V|MIPS3_PG_D
		    | mips3_paddr_to_tlbpfn(MIPS_KSEG0_TO_PHYS(trunc_page(load_addr)));
		struct tlbmask tlbmask = {
			.tlb_hi = -PAGE_SIZE | KERNEL_PID,
#if PGSHIFT & 1
			.tlb_lo1 = tlb_lo,
			.tlb_lo1 = tlb_lo + MIPS3_PG_NEXT,
#else
			.tlb_lo0 = 0,
			.tlb_lo1 = tlb_lo,
#endif
			.tlb_mask = -1,
		};
		ci->ci_tlb_slot = ti->ti_wired++;
		mips3_cp0_wired_write(ti->ti_wired);
		tlb_invalidate_addr(-PAGE_SIZE, KERNEL_PID);
		tlb_write_entry(ci->ci_tlb_slot, &tlbmask);
	}
	TLBINFO_UNLOCK(ti);

	return true;
}
#endif /* MIPS3_PLUS */

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

intptr_t
mips_fixup_addr(const uint32_t *stubp)
{
	/*
	 * Stubs typically look like:
	 *	lui	v0, %hi(sym)
	 *	lX	t9, %lo(sym)(v0)
	 *	[nop]
	 *	jr	t9
	 *	nop
	 *
	 * Or for loongson2 (
	 *	lui	v0, %hi(sym)
	 *	lX	t9, %lo(sym)(v0)
	 *	lui	at,0xcfff
	 *	ori	at,at,0xffff
	 *	and	t9,t9,at
	 *	jr	t9
	 *	move	at,at
	 *   or:
	 *	lui	v0, %hi(sym)
	 *	lX	t9, %lo(sym)(v0)
	 *	li	at, 0x3
	 *	dmtc0	at, $22
	 *	jr	t9
	 *	nop
	 */
	mips_reg_t regs[32];
	uint32_t used = 1 |__BIT(_R_A0)|__BIT(_R_A1)|__BIT(_R_A2)|__BIT(_R_A3);
	size_t n;
	const char *errstr = "mips";
	/*
	 * This is basically a small MIPS emulator for those instructions
	 * that might be in a stub routine.
	 */
	for (n = 0; n < 16; n++) {
		const InstFmt insn = { .word = stubp[n] };
		switch (insn.IType.op) {
		case OP_LUI:
			regs[insn.IType.rt] = (int16_t)insn.IType.imm << 16;
			used |= (1 << insn.IType.rt);
			break;
#ifdef _LP64
		case OP_LD:
			if ((used & (1 << insn.IType.rs)) == 0) {
				errstr = "LD";
				goto out;
			}
			regs[insn.IType.rt] = *(const int64_t *)
			    (regs[insn.IType.rs] + (int16_t)insn.IType.imm);
			used |= (1 << insn.IType.rt);
			break;
		case OP_SD:
			if (insn.IType.rt != _R_RA || insn.IType.rs != _R_SP) {
				errstr = "SD";
				goto out;
			}
			break;
#else
		case OP_LW:
			if ((used & (1 << insn.IType.rs)) == 0) {
				errstr = "LW";
				goto out;
			}
			regs[insn.IType.rt] = *(const int32_t *)
			    ((intptr_t)regs[insn.IType.rs]
			    + (int16_t)insn.IType.imm);
			used |= (1 << insn.IType.rt);
			break;
		case OP_SW:
			if (insn.IType.rt != _R_RA || insn.IType.rs != _R_SP) {
				errstr = "SW";
				goto out;
			}
			break;
#endif
		case OP_ORI:
			if ((used & (1 << insn.IType.rs)) == 0) {
				errstr = "ORI";
				goto out;
			}
			regs[insn.IType.rt] |= insn.IType.imm;
			used |= (1 << insn.IType.rt);
			break;
		case OP_COP0:
			switch (insn.RType.rs) {
			case OP_DMT:
				if (insn.RType.rd != 22) {
					errstr = "dmtc0 dst";
					goto out;
				}
				if ((used & (1 << insn.RType.rt)) == 0) {
					errstr = "dmtc0 src";
					goto out;
				}
				break;
			default:
				errstr = "COP0";
				goto out;
			}
			break;
		case OP_SPECIAL:
			switch (insn.RType.func) {
			case OP_JALR:
			case OP_JR:
				if ((used & (1 << insn.RType.rs)) == 0) {
					errstr = "JR";
					goto out;
				}
				if (stubp[n+1] != 0
				    && (stubp[n+1] & 0xfff0003c) != 0x0000003c
				    && stubp[n+1] != 0x00200825) {
					n++;
					errstr = "delay slot";
					goto out;
				}
				return regs[insn.RType.rs];
			case OP_AND:
				if ((used & (1 << insn.RType.rs)) == 0
				    || (used & (1 << insn.RType.rt)) == 0) {
					errstr = "AND";
					goto out;
				}
				regs[insn.RType.rd] =
				    regs[insn.RType.rs] & regs[insn.RType.rt];
				used |= (1 << insn.RType.rd);
				break;
#if !defined(__mips_o32)
			case OP_DSLL32:	/* force to 32-bits */
			case OP_DSRA32:	/* force to 32-bits */
				if (regs[insn.RType.rd] != regs[insn.RType.rt]
				    || (used & (1 << insn.RType.rt)) == 0
				    || regs[insn.RType.shamt] != 0) {
					errstr = "AND";
					goto out;
				}
				break;
#endif
			case OP_SLL:	/* nop */
				if (insn.RType.rd != _R_ZERO) {
					errstr = "NOP";
					goto out;
				}
				break;
			case OP_DSLL:
			default:
				errstr = "SPECIAL";
				goto out;
			}
			break;
		default:
			errstr = "mips";
			goto out;
		}
	}

  out:
	printf("%s: unexpected %s insn %#x at %p\n",
	    __func__, errstr,
	    stubp[n], &stubp[n]);
	return 0;
}

void
mips_fixup_stubs(uint32_t *start, uint32_t *end)
{
#ifdef DEBUG
	size_t fixups_done = 0;
	uint32_t cycles =
#if (MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	    (CPUISMIPS3 ? mips3_cp0_count_read() : 0);
#else
	    0;
#endif
#endif
	extern uint32_t __stub_start[], __stub_end[];

	KASSERT(MIPS_KSEG0_P(start));
	KASSERT(MIPS_KSEG0_P(end));
	KASSERT(MIPS_KSEG0_START == (((intptr_t)start >> 28) << 28));

	if (end > __stub_start)
		end = __stub_start;

	for (uint32_t *insnp = start; insnp < end; insnp++) {
		uint32_t insn = *insnp;
		uint32_t offset = insn & 0x03ffffff;
		uint32_t opcode = insn >> 26;
		const uint32_t * const stubp =
		    &((uint32_t *)(((intptr_t)insnp >> 28) << 28))[offset];

		/*
		 * First we check to see if this is a jump and whether it is
		 * within the range we are interested in.
		 */
		if ((opcode != OPCODE_J && opcode != OPCODE_JAL)
		    || stubp < __stub_start || __stub_end <= stubp)
			continue;

		const intptr_t real_addr = mips_fixup_addr(stubp);

		/*
		 * If the real_addr has been set yet, don't fix up.
		 */
		if (real_addr == 0) {
			continue;
		}
		/*
		 * Verify the real destination is in the same 256MB
		 * as the location of the jump instruction.
		 */
		KASSERT((real_addr >> 28) == ((intptr_t)insnp >> 28));

		/*
		 * Now fix it up.  Replace the old displacement to the stub
		 * with the real displacement.
		 */
		struct mips_jump_fixup_info fixup = {
		    .jfi_stub = fixup_addr2offset(stubp),
		    .jfi_real = fixup_addr2offset(real_addr),
		};

		fixup_mips_jump(insnp, &fixup);
#ifdef DEBUG
		fixups_done++;
#endif
	}

	if (sizeof(uint32_t [end - start]) > mips_cache_info.mci_picache_size)
		mips_icache_sync_all();
	else
		mips_icache_sync_range((intptr_t)start,
		    sizeof(uint32_t [end - start]));

#ifdef DEBUG
#if (MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
	if (CPUISMIPS3)
		cycles = mips3_cp0_count_read() - cycles;
#endif
	printf("%s: %zu fixup%s done in %u cycles\n", __func__,
	    fixups_done, fixups_done == 1 ? "" : "s",
	    cycles);
#endif
}

#define	__stub		__section(".stub")

void	mips_cpu_switch_resume(struct lwp *)		__stub;
tlb_asid_t
	tlb_get_asid(void)				__stub;
void	tlb_set_asid(uint32_t)				__stub;
void	tlb_invalidate_all(void)			__stub;
void	tlb_invalidate_globals(void)			__stub;
void	tlb_invalidate_asids(uint32_t, uint32_t)	__stub;
void	tlb_invalidate_addr(vaddr_t, tlb_asid_t)	__stub;
u_int	tlb_record_asids(u_long *, uint32_t)		__stub;
bool	tlb_update_addr(vaddr_t, tlb_asid_t, pt_entry_t, bool)
							__stub;
void	tlb_read_entry(size_t, struct tlbmask *)	__stub;
void	tlb_write_entry(size_t, const struct tlbmask *) __stub;

/*
 * wbflush isn't a stub since it gets overridden quite late
 * (after mips_vector_init returns).
 */
void	wbflush(void)					/*__stub*/;

void
mips_cpu_switch_resume(struct lwp *l)
{
	(*mips_locore_jumpvec.ljv_cpu_switch_resume)(l);
}

tlb_asid_t
tlb_get_asid(void)
{
	return (*mips_locore_jumpvec.ljv_tlb_get_asid)();
}

void
tlb_set_asid(uint32_t asid)
{
	(*mips_locore_jumpvec.ljv_tlb_set_asid)(asid);
}

void
tlb_invalidate_all(void)
{
	(*mips_locore_jumpvec.ljv_tlb_invalidate_all)();
}

void
tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	(*mips_locore_jumpvec.ljv_tlb_invalidate_addr)(va, asid);
}

void
tlb_invalidate_globals(void)
{
	(*mips_locore_jumpvec.ljv_tlb_invalidate_globals)();
}

void
tlb_invalidate_asids(uint32_t asid_lo, uint32_t asid_hi)
{
	(*mips_locore_jumpvec.ljv_tlb_invalidate_asids)(asid_lo, asid_hi);
}

u_int
tlb_record_asids(u_long *bitmap, tlb_asid_t asid_max)
{
	return (*mips_locore_jumpvec.ljv_tlb_record_asids)(bitmap, asid_max);
}

#if 0
bool
tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert)
{
	return (*mips_locore_jumpvec.ljv_tlb_update_addr)(va, asid, pte, insert);
}
#endif

void
tlb_read_entry(size_t tlbno, struct tlbmask *tlb)
{
	(*mips_locore_jumpvec.ljv_tlb_read_entry)(tlbno, tlb);
}

void
tlb_write_entry(size_t tlbno, const struct tlbmask *tlb)
{
	(*mips_locore_jumpvec.ljv_tlb_write_entry)(tlbno, tlb);
}

void
wbflush(void)
{
	(*mips_locoresw.lsw_wbflush)();
}

#ifndef LOCKDEBUG
void mutex_enter(kmutex_t *mtx)				__stub;
void mutex_exit(kmutex_t *mtx)				__stub;
void mutex_spin_enter(kmutex_t *mtx)			__stub;
void mutex_spin_exit(kmutex_t *mtx)			__stub;

void
mutex_enter(kmutex_t *mtx)
{

	(*mips_locore_atomicvec.lav_mutex_enter)(mtx);
}

void
mutex_exit(kmutex_t *mtx)
{

	(*mips_locore_atomicvec.lav_mutex_exit)(mtx);
}

void
mutex_spin_enter(kmutex_t *mtx)
{

	(*mips_locore_atomicvec.lav_mutex_spin_enter)(mtx);
}

void
mutex_spin_exit(kmutex_t *mtx)
{

	(*mips_locore_atomicvec.lav_mutex_spin_exit)(mtx);
}
#endif	/* !LOCKDEBUG */

u_int _atomic_cas_uint(volatile u_int *, u_int, u_int)		__stub;
u_long _atomic_cas_ulong(volatile u_long *, u_long, u_long)	__stub;

u_int
_atomic_cas_uint(volatile u_int *ptr, u_int old, u_int new)
{

	return (*mips_locore_atomicvec.lav_atomic_cas_uint)(ptr, old, new);
}

u_long
_atomic_cas_ulong(volatile u_long *ptr, u_long old, u_long new)
{

	return (*mips_locore_atomicvec.lav_atomic_cas_ulong)(ptr, old, new);
}

__strong_alias(atomic_cas_uint, _atomic_cas_uint)
__strong_alias(atomic_cas_uint_ni, _atomic_cas_uint)
__strong_alias(_atomic_cas_32, _atomic_cas_uint)
__strong_alias(_atomic_cas_32_ni, _atomic_cas_uint)
__strong_alias(atomic_cas_32, _atomic_cas_uint)
__strong_alias(atomic_cas_32_ni, _atomic_cas_uint)
__strong_alias(atomic_cas_ptr, _atomic_cas_ulong)
__strong_alias(atomic_cas_ptr_ni, _atomic_cas_ulong)
__strong_alias(atomic_cas_ulong, _atomic_cas_ulong)
__strong_alias(atomic_cas_ulong_ni, _atomic_cas_ulong)
#ifdef _LP64
__strong_alias(atomic_cas_64, _atomic_cas_ulong)
__strong_alias(atomic_cas_64_ni, _atomic_cas_ulong)
__strong_alias(_atomic_cas_64, _atomic_cas_ulong)
__strong_alias(_atomic_cas_64_ni, _atomic_cas_ulong)
#endif

int	ucas_uint(volatile u_int *, u_int, u_int, u_int *)	__stub;
int	ucas_ulong(volatile u_long *, u_long, u_long, u_long *)	__stub;

int
ucas_uint(volatile u_int *ptr, u_int old, u_int new, u_int *retp)
{

	return (*mips_locore_atomicvec.lav_ucas_uint)(ptr, old, new, retp);
}
__strong_alias(ucas_32, ucas_uint);
__strong_alias(ucas_int, ucas_uint);

int
ucas_ulong(volatile u_long *ptr, u_long old, u_long new, u_long *retp)
{

	return (*mips_locore_atomicvec.lav_ucas_ulong)(ptr, old, new, retp);
}
__strong_alias(ucas_ptr, ucas_ulong);
__strong_alias(ucas_long, ucas_ulong);
#ifdef _LP64
__strong_alias(ucas_64, ucas_ulong);
#endif
