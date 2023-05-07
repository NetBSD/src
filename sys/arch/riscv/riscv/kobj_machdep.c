/*	$NetBSD: kobj_machdep.c,v 1.5 2023/05/07 12:41:49 skrll Exp $	*/

/*-
 * Copyright (c) 2014,2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and by Nick Hudson.
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

__RCSID("$NetBSD: kobj_machdep.c,v 1.5 2023/05/07 12:41:49 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kobj_impl.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <riscv/locore.h>

struct hi20 {
	void *where;
	long addend;
	bool local;
} last_hi20;

static int
kobj_findhi20(kobj_t ko, uintptr_t relocbase, bool local, const Elf_Rela *lo12,
    Elf_Addr *hi20addr)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	/*
	 * Find the relocation section.
	 */
	for (size_t i = 0; i < ko->ko_nrela; i++) {
		rela = ko->ko_relatab[i].rela;
		if (rela == NULL) {
			continue;
		}
		relalim = rela + ko->ko_relatab[i].nrela;

		/* Check that the relocation is in this section */
		if (lo12 < rela || relalim < lo12)
			continue;

		/*
		 * Now find the HI20 relocation from the LO12 relocation
		 * address (hi20addr)
		 */
		for (; rela < relalim; rela++) {
			Elf_Addr * const where =
			    (Elf_Addr *)(relocbase + rela->r_offset);

			if (where != hi20addr) {
				continue;
			}
			/* Found the referenced HI20 relocation */
			uintptr_t symidx = ELF_R_SYM(rela->r_info);
#if 0
			if (symidx >= ko->ko_symcnt) {
				continue;
			}
#endif
			/*
			 * If this symbol doesn't is global and we're doing
			 * the local symbols at this point then we can't
			 * resolve it yet, so tell our caller.
			 */
			const Elf_Sym *sym = kobj_symbol(ko, symidx);
			if (local && ELF_ST_BIND(sym->st_info) != STB_LOCAL) {
				last_hi20.local = false;
				return 1;
			}

			last_hi20.local = true;

			Elf_Addr addr;
			const int error = kobj_sym_lookup(ko, symidx, &addr);
			if (error)
				return -1;

			long addend = rela->r_addend;	/* needs to be signed */
			addend -= (intptr_t)where;

			last_hi20.where = where;
			last_hi20.addend = addend;
			addend += addr;

			return 0;
		}
	}
	return -1;
}


int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data, bool isrela,
    bool local)
{
	if (!isrela) {
		panic("kobj_reloc: REL relocations not supported");
	}

	const Elf_Rela * const rela = (const Elf_Rela *)data;
	Elf_Addr * const where = (Elf_Addr *)(relocbase + rela->r_offset);
	long addend = rela->r_addend;	/* needs to be signed */
	uint32_t *wwhere = (uint32_t *)where;
	uint16_t * const hwhere = (uint16_t *)where;
	uint8_t * const bwhere = (uint8_t *)where;
	const u_int rtype = ELF_R_TYPE(rela->r_info);
	const u_int symidx = ELF_R_SYM(rela->r_info);
	const Elf_Sym *sym = kobj_symbol(ko, symidx);

	/*
	 * Check that we need to process this relocation in this pass.
	 * All relocations to local symols can be done in the first pass
	 * apart from PCREL_LO12 that reference a HI20 to a global symbol.
	 */
	switch (rtype) {
	case R_RISCV_PCREL_LO12_I:
	case R_RISCV_PCREL_LO12_S:
		if (addend != 0) {
			printf("oops\n");
		}
	case R_RISCV_PCREL_HI20:
	case R_RISCV_HI20:
		break;
	default:
		/* Don't do other relocations again */
		if (!local && ELF_ST_BIND(sym->st_info) == STB_LOCAL) {
			return 0;
		}
	}

	/* Relocation calculation */
	switch (rtype) {
	case R_RISCV_NONE:
	case R_RISCV_RELAX:
		return 0;

	case R_RISCV_BRANCH:
	case R_RISCV_JAL:
		// XXXNH eh? what's with the symidx test?'
		if (symidx == 0)
			break;
		/* FALLTHOUGH */

	case R_RISCV_CALL_PLT:
	case R_RISCV_CALL:
	case R_RISCV_PCREL_HI20:
	case R_RISCV_RVC_BRANCH:
	case R_RISCV_RVC_JUMP:
	case R_RISCV_32_PCREL:
		addend -= (intptr_t)where;		/* A -= P */
		/* FALLTHOUGH */

#ifdef _LP64
	case R_RISCV_64:	/* doubleword64 S + A */
	case R_RISCV_ADD64:
	case R_RISCV_SUB64:
#endif
	case R_RISCV_HI20:
	case R_RISCV_PCREL_LO12_I:
	case R_RISCV_PCREL_LO12_S:
	case R_RISCV_LO12_I:
	case R_RISCV_LO12_S:
	case R_RISCV_ADD32:
	case R_RISCV_ADD16:
	case R_RISCV_ADD8:
	case R_RISCV_SUB32:
	case R_RISCV_SUB16:
	case R_RISCV_SUB8:
	case R_RISCV_SUB6:
	case R_RISCV_SET32:
	case R_RISCV_SET16:
	case R_RISCV_SET8:
	case R_RISCV_SET6: {
		Elf_Addr addr;
		const int error = kobj_sym_lookup(ko, symidx, &addr);
		if (error) {
			/*
			 * This is a fatal error in the 2nd pass, i.e.
			 * local == false
			 */
			if (!local)
				return -1;
			return 0;
		}

		switch (rtype) {
		case R_RISCV_PCREL_HI20:
		case R_RISCV_HI20: {

			// XXXNH why is this without middle?
			last_hi20.addend = addend + addr;
			last_hi20.where = where;
			last_hi20.local = ELF_ST_BIND(sym->st_info) == STB_LOCAL;

			/*
			 * If this is the 2nd pass then and the symbol is
			 * local then it's already been fixed up.
			 */
			if (!local && last_hi20.local) {
//				last_hi20.global = false;
				return 0;
			}
			const uint32_t lobits = 12;
			const uint32_t middle = 1U << (lobits - 1);

			addend += addr + middle;
			break;
		    }
		case R_RISCV_PCREL_LO12_I:
		case R_RISCV_PCREL_LO12_S: {
			if (last_hi20.where != (void *)addr) {
				int err = kobj_findhi20(ko, relocbase, local,
				    rela, (Elf_Addr *)addr);
				if (err < 0)
					return -1;
				else if (err > 0) {
					KASSERT(local);
					return 0;
				}
			}
			/*
			 * In the second pass if the HI20 symbol was local
			 * it was already processed.
			 */
			if (!local && last_hi20.local) {
				return 0;
			}
			addend = addend + (last_hi20.addend & __BITS(11,0));
			break;
		    }
		default:
			addend += addr;				/* A += S */
		}
		break;
	    }

	default:
		printf("%s: unexpected relocation type %u\n\n", __func__, rtype);
		return -1;
	}

	/* Relocation memory update */
	switch (rtype) {
	case R_RISCV_64:	/* word64 S + A */
		*where = addend;
		break;

	case R_RISCV_32:	/* word32 S + A */
	case R_RISCV_SET32:	/* word32 S + A */
	case R_RISCV_32_PCREL:	/* word32 S + A - P */
		*wwhere = addend;
		break;

	case R_RISCV_SET16:	/* word16 S + A */
		*hwhere = addend;
		break;

	case R_RISCV_SET8:	/* word8 S + A */
		*bwhere = addend;
		break;

	case R_RISCV_SET6: {	/* word6 S + A */
		const uint8_t mask = __BITS(5, 0);

		*bwhere = (*bwhere & ~mask) | __SHIFTIN(addend, mask);
		break;
	    }

	case R_RISCV_ADD64:	/* word64 V + S + A */
		*where += addend;
		break;

	case R_RISCV_ADD32:	/* word32 V + S + A */
		*wwhere += addend;
		break;

	case R_RISCV_ADD16:	/* word16 V + S + A */
		*hwhere += addend;
		break;

	case R_RISCV_ADD8:	/* word8 V + S + A */
		*bwhere += addend;
		break;

	case R_RISCV_SUB64:	/* word64 V - S - A */
		*where -= addend;
		break;

	case R_RISCV_SUB32:	/* word32 V - S - A */
		*wwhere -= addend;
		break;

	case R_RISCV_SUB16:	/* word16 V - S - A */
		*hwhere -= addend;
		break;

	case R_RISCV_SUB8:	/* word8 V - S - A */
		*bwhere -= addend;
		break;

	case R_RISCV_SUB6:	/* word6 V - S - A */
		// XXXNH
		*bwhere -= addend;
		break;

	case R_RISCV_BRANCH: {
		/*
		 * B-type
		 *   31    | 30     25 | ... | 11     8 |    7     | ...
		 * imm[12] | imm[10:5] | ... | imm[4:1] | imm[11]  | ...
		 */

		const uint32_t immA = __SHIFTOUT(addend, __BIT(12));
		const uint32_t immB = __SHIFTOUT(addend, __BITS(10,  5));
		const uint32_t immC = __SHIFTOUT(addend, __BITS( 4,  1));
		const uint32_t immD = __SHIFTOUT(addend, __BIT(11));
		addend =
		    __SHIFTIN(immA, __BIT(31)) |
		    __SHIFTIN(immB, __BITS(30, 25)) |
		    __SHIFTIN(immC, __BITS(11, 8)) |
		    __SHIFTIN(immD, __BIT(7));
		const uint32_t mask = __BITS(31, 25) | __BITS(11, 7);

		*wwhere = (*wwhere & ~mask) | addend;
		break;
	    }

	case R_RISCV_JAL: {
		/*
		 * J-type
		 *   31    | 30     21 |   20    | 19      12 | ...
		 * imm[20] | imm[10:1] | imm[11] | imm[19:12] | ...
		 */

		const uint32_t immA = __SHIFTOUT(addend,  __BIT(20));
		const uint32_t immB = __SHIFTOUT(addend, __BITS(10,  1));
		const uint32_t immC = __SHIFTOUT(addend,  __BIT(11));
		const uint32_t immD = __SHIFTOUT(addend, __BITS(19, 12));
		addend =
		    __SHIFTIN(immA,  __BIT(31)) |
		    __SHIFTIN(immB, __BITS(30, 21)) |
		    __SHIFTIN(immC,  __BIT(20)) |
		    __SHIFTIN(immD, __BITS(19,12));
		/* FALLTHROUGH */
	    }

	case R_RISCV_HI20:		/* LUI/AUIPC (S + A - P) */
	case R_RISCV_PCREL_HI20: {	/* LUI/AUIPC (S + A - P) */
		/*
		 * U-Type
		 * 31      12 | ...
		 * imm[31:12] | ...
		 */
		const uint32_t mask = __BITS(31, 12);
		*wwhere = (addend & mask) | (*wwhere & ~mask);
		break;
	    }
	case R_RISCV_PCREL_LO12_I:
	case R_RISCV_LO12_I: {		/* low12 I-type instructions */
		/*
		 * I-Type
		 * 31     20 | ...
		 * imm[11:0] | ...
		 */

		*wwhere += ((addend) << 20);
		break;
	    }
	case R_RISCV_PCREL_LO12_S:
	case R_RISCV_LO12_S: {	/* low12 S-type instructions */
		/*
		 * S-type
		 *
		 * 31     25 | ... | 11     7 | ...
		 * imm[11:5] | ... | imm[4:0] | ...
		 *
		 */
		const uint32_t immA = __SHIFTOUT(addend, __BITS(11,  5));
		const uint32_t immB = __SHIFTOUT(addend, __BITS( 4,  0));
		const uint32_t mask = __BITS(31, 25) | __BITS(11, 7);
		addend =
		    __SHIFTIN(immA, __BITS(31, 25)) |
		    __SHIFTIN(immB, __BITS(11,  7));
		*wwhere = (*wwhere & ~mask) | addend;
		break;
	    }
	case R_RISCV_CALL:
	case R_RISCV_CALL_PLT: {
		/*
		 * U-type
		 *
		 * 31      12 | ...
		 * imm[31:12] | ...
		 *
		 *
		 * I-type
		 *
		 * 31     25 | ...
		 * imm[11:0] | ...
		 */
		// XXXNH better name...
		const int32_t check = (int32_t)addend >> 20;
		if (check == 0 || check == -1) {
			// This falls into the range for the JAL instruction.
			/*
			* J-type
			*   31    | 30     21 |   20    | 19      12 | ...
			* imm[20] | imm[10:1] | imm[11] | imm[19:12] | ...
			*/

			const uint32_t immA = __SHIFTOUT(addend,  __BIT(20));
			const uint32_t immB = __SHIFTOUT(addend, __BITS(10,  1));
			const uint32_t immC = __SHIFTOUT(addend,  __BIT(11));
			const uint32_t immD = __SHIFTOUT(addend, __BITS(19, 12));
			addend =
			    __SHIFTIN(immA,  __BIT(31)) |
			    __SHIFTIN(immB, __BITS(30, 21)) |
			    __SHIFTIN(immC,  __BIT(20)) |
			    __SHIFTIN(immD, __BITS(19,12));

			// Convert the auipc/jalr to a jal/nop
			wwhere[0] = addend | (wwhere[1] & 0xf80) | 0x6f;
			wwhere[1] = 0x00000013;	// nop (addi x0, x0, 0)
			printf("%s: %s where (%p) [0] -> %x, [1] -> %x\n", __func__,
			    "R_RISCV_CALL", wwhere, wwhere[0], wwhere[1]);
			break;
		}
		wwhere[0] = ((addend + 0x800) & 0xfffff000)
		    | (wwhere[0] & 0xfff);
		wwhere[1] = (addend << 20) | (wwhere[1] & 0x000fffff);
		printf("%s: %s where (%p) [0] -> %x, [1] -> %x\n", __func__,
		    "R_RISCV_CALL", wwhere, wwhere[0], wwhere[1]);

		break;
	    }
	case R_RISCV_RVC_BRANCH: {
		/*
		 * CB-type
		 *
		 * ... | 12         10 | ... | 6        5   3 2  | ...
		 * ... | offset[8|4:3] | ... | offset[7:6|2:1|5] | ...
		 */

		const uint16_t immA = __SHIFTOUT(addend, __BIT(8));
		const uint16_t immB = __SHIFTOUT(addend, __BITS(4, 3));
		const uint16_t immC = __SHIFTOUT(addend, __BITS(7, 6));
		const uint16_t immD = __SHIFTOUT(addend, __BITS(2, 1));
		const uint16_t immE = __SHIFTOUT(addend, __BIT(5));
		const uint16_t mask = __BITS(12, 10) | __BITS(6, 2);
		addend =
		    __SHIFTIN(immA, __BIT(12)) |
		    __SHIFTIN(immB, __BITS(11, 10)) |
		    __SHIFTIN(immC, __BITS( 6,  5)) |
		    __SHIFTIN(immD, __BITS( 4,  3)) |
		    __SHIFTIN(immE, __BIT(2));
		*hwhere = (*hwhere & ~mask) | addend;
		break;
	    }

	case R_RISCV_RVC_JUMP: {
		/*
		 * CJ-type
		 *
		 * ... | 12                          2 | ...
		 * ... | offset[11|4|9:8|10|6|7|3:1|5] | ...
		 */

		const uint16_t immA = __SHIFTOUT(addend, __BIT(11));
		const uint16_t immB = __SHIFTOUT(addend, __BIT(4));
		const uint16_t immC = __SHIFTOUT(addend, __BITS(9, 8));
		const uint16_t immD = __SHIFTOUT(addend, __BIT(10));
		const uint16_t immE = __SHIFTOUT(addend, __BIT(6));
		const uint16_t immF = __SHIFTOUT(addend, __BIT(7));
		const uint16_t immG = __SHIFTOUT(addend, __BITS(3, 1));
		const uint16_t immH = __SHIFTOUT(addend, __BIT(5));
		const uint16_t mask = __BITS(12, 2);
		addend =
		    __SHIFTIN(immA, __BIT(12)) |
		    __SHIFTIN(immB, __BIT(11)) |
		    __SHIFTIN(immC, __BITS(10,  9)) |
		    __SHIFTIN(immD, __BIT(8)) |
		    __SHIFTIN(immE, __BIT(7)) |
		    __SHIFTIN(immF, __BIT(6)) |
		    __SHIFTIN(immG, __BITS(5, 3)) |
		    __SHIFTIN(immH, __BIT(2));
		*hwhere = (*hwhere & ~mask) | addend;
		break;
	    }

	case R_RISCV_RVC_LUI: {
		/*
		 * CI-type
		 *
		 * ... | 12         | ... | 6           2 | ...
		 * ... | offset[17] | ... | offset[16:12] | ...
		 */

		const uint16_t immA = __SHIFTOUT(addend, __BIT(17));
		const uint16_t immB = __SHIFTOUT(addend, __BITS(16,12));
		const uint16_t mask = __BIT(12) | __BITS(6,  2);
		addend =
		    __SHIFTIN(immA, __BIT(12)) |
		    __SHIFTIN(immB, __BITS(6, 2));
		*hwhere = (*hwhere & ~mask) | addend;
		break;
	    }

	default:
		printf("Unexpected relocation type %d\n", rtype);
	}

	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{
	if (load)
		__asm("fence rw,rw; fence.i");

	return 0;
}
