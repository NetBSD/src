/*	$NetBSD: fixup.c,v 1.10 2014/08/02 15:58:04 joerg Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

__KERNEL_RCSID(0, "$NetBSD: fixup.c,v 1.10 2014/08/02 15:58:04 joerg Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <powerpc/instr.h>
#include <powerpc/spr.h>
#include <powerpc/include/cpu.h>
#include <powerpc/include/oea/spr.h>

static inline void
fixup_jump(uint32_t *insnp, const struct powerpc_jump_fixup_info *jfi)
{
	union instr instr = { .i_int = *insnp };

	KASSERT(instr.i_any.i_opcd == OPC_B);

	instr.i_i.i_li = jfi->jfi_real - fixup_addr2offset(insnp);

	*insnp = instr.i_int;

	__asm(
		"dcbst	0,%0"	"\n\t"
		"sync"		"\n\t"
		"icbi	0,%0"	"\n\t"
		"sync"		"\n\t"
		"isync"
	    :: "b"(insnp));
}

void
powerpc_fixup_stubs(uint32_t *start, uint32_t *end,
	uint32_t *stub_start, uint32_t *stub_end)
{
	extern uint32_t __stub_start[], __stub_end[];
#ifdef DEBUG
	size_t fixups_done = 0;
	uint64_t cycles = 0;
#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601)
	    cycles = rtc_nanosecs() >> 7;
	else
#endif
	    cycles = mftb();
#endif

	if (stub_start == NULL) {
		stub_start = __stub_start;
		stub_end = __stub_end;
	}

	if (end > __stub_start)
		end = __stub_start;

	for (uint32_t *insnp = start; insnp < end; insnp++) {
		struct powerpc_jump_fixup_info fixup;
		union instr instr = { .i_int = *insnp };
		uint32_t *stub = insnp + instr.i_i.i_li;
		u_int opcode = instr.i_any.i_opcd;

		/*
		 * First we check to see if this is a jump and whether it is
		 * within the range we are interested in.
		 */
		if (opcode != OPC_B || stub < stub_start || stub_end <= stub)
			continue;

		fixup.jfi_stub = fixup_addr2offset(stub);
		fixup.jfi_real = 0;

		/*
		 * We know it's a jump, now we need to figure out where it goes.
		 */
		register_t fixreg[32];
		register_t ctr = 0;
		uint32_t valid_mask = (1 << 1);
#ifdef DIAGNOSTIC
		int r_lr = -1;
#endif
		for (; stub < stub_end && fixup.jfi_real == 0; stub++) {
			const union instr i = { .i_int = *stub };

			switch (i.i_any.i_opcd) {
			case OPC_integer_31: {
				const u_int rs = i.i_x.i_rs;
				const u_int ra = i.i_x.i_ra;
				const u_int rb = i.i_x.i_rb;
				switch (i.i_x.i_xo) {
				case OPC31_MFSPR: {
#ifdef DIAGNOSTIC
					const u_int spr = (rb << 5) | ra;
					KASSERT(spr == SPR_LR);
					r_lr = rs;
#endif
					valid_mask |= (1 << rs);
					break;
				}
				case OPC31_MTSPR: {
#ifdef DIAGNOSTIC
					const u_int spr = (rb << 5) | ra;
					KASSERT(valid_mask & (1 << rs));
					KASSERT(spr == SPR_CTR);
#endif
					ctr = fixreg[rs];
					break;
				}
				case OPC31_OR: {
#ifdef DIAGNOSTIC
					KASSERT(valid_mask & (1 << rs));
					KASSERT(valid_mask & (1 << rb));
#endif
					fixreg[ra] = fixreg[rs] | fixreg[rb];
					valid_mask |= 1 << ra;
					break;
				}
				default:
					panic("%s: %p: unexpected insn 0x%08x",
					    __func__, stub, i.i_int);
				}
				break;
			}
			case OPC_ADDI:
			case OPC_ADDIS: {
				const u_int rs = i.i_d.i_rs;
				const u_int ra = i.i_d.i_ra;
				register_t d = i.i_d.i_d << ((i.i_d.i_opcd & 1) * 16);
				if (ra) {
					KASSERT(valid_mask & (1 << ra));
					d += fixreg[ra];
				}
				fixreg[rs] = d;
				valid_mask |= (1 << rs);
				break;
			}
			case OPC_LWZ: {
				const u_int rs = i.i_d.i_rs;
				const u_int ra = i.i_d.i_ra;
				register_t addr = i.i_d.i_d;
				if (ra) {
					KASSERT(valid_mask & (1 << ra));
					addr += fixreg[ra];
				}
				fixreg[rs] = *(uint32_t *)addr;
				valid_mask |= (1 << rs);
				break;
			}
			case OPC_STW: {
				KASSERT((i.i_d.i_rs == r_lr || i.i_d.i_rs == 31) && i.i_d.i_ra == 1);
				break;
			}
			case OPC_STWU: {
				KASSERT(i.i_d.i_rs == 1 && i.i_d.i_ra == 1);
				KASSERT(i.i_d.i_d < 0);
				break;
			}
			case OPC_branch_19: {
				KASSERT(r_lr == -1 || i.i_int == 0x4e800421);
				KASSERT(r_lr != -1 || i.i_int == 0x4e800420);
				if (ctr == 0) {
					panic("%s: jump at %p to %p would "
					    "branch to 0", __func__, insnp,
					    insnp + instr.i_i.i_li);
				}
				fixup.jfi_real = fixup_addr2offset(ctr);
				break;
			}
			case OPC_RLWINM: {
				// LLVM emits these for bool operands.
				// Ignore them.
				break;
			}
			default: {
				panic("%s: %p: unexpected insn 0x%08x",
					    __func__, stub, i.i_int);
			}
			}
		}
		KASSERT(fixup.jfi_real != 0);
		/*
		 * Now we know the real destination to branch to.  Replace the
		 * old displacement with the new displacement.
		 */
#if 0
		printf("%s: %p: change from %#x to %#x\n",
		    __func__, insnp, fixup.jfi_stub << 2, fixup.jfi_real << 2);
#endif
		fixup_jump(insnp, &fixup);
#ifdef DEBUG
		fixups_done++;
#endif
	}

#ifdef DEBUG

#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601)
	    cycles = (rtc_nanosecs() >> 7) - cycles;
	else
#endif
	cycles = mftb() - cycles;

	printf("%s: %zu fixup%s done in %"PRIu64" cycles\n", __func__,
	    fixups_done, fixups_done == 1 ? "" : "s",
	    cycles);
#endif
}

void
cpu_fixup_stubs(void)
{
	extern uint32_t _ftext[];
	extern uint32_t _etext[];

	powerpc_fixup_stubs(_ftext, _etext, NULL, NULL);
}
