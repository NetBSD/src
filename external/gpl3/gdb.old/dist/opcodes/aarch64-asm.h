/* aarch64-asm.h -- Header file for aarch64-asm.c and aarch64-asm-2.c.
   Copyright 2012  Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef OPCODES_AARCH64_ASM_H
#define OPCODES_AARCH64_ASM_H

#include "aarch64-opc.h"

/* Given OPCODE, return the opcode entry that OPCODE aliases to, e.g.
   given LSL, return UBFM.  */

const aarch64_opcode* aarch64_find_real_opcode (const aarch64_opcode *);

/* Switch-table-based high-level operand inserter.  */

const char* aarch64_insert_operand (const aarch64_operand *,
				    const aarch64_opnd_info *, aarch64_insn *,
				    const aarch64_inst *);

/* Operand inserters.  */

#define AARCH64_DECL_OPD_INSERTER(x)	\
  const char* aarch64_##x (const aarch64_operand *, const aarch64_opnd_info *, \
			   aarch64_insn *, const aarch64_inst *)

AARCH64_DECL_OPD_INSERTER (ins_regno);
AARCH64_DECL_OPD_INSERTER (ins_reglane);
AARCH64_DECL_OPD_INSERTER (ins_reglist);
AARCH64_DECL_OPD_INSERTER (ins_ldst_reglist);
AARCH64_DECL_OPD_INSERTER (ins_ldst_reglist_r);
AARCH64_DECL_OPD_INSERTER (ins_ldst_elemlist);
AARCH64_DECL_OPD_INSERTER (ins_advsimd_imm_shift);
AARCH64_DECL_OPD_INSERTER (ins_imm);
AARCH64_DECL_OPD_INSERTER (ins_imm_half);
AARCH64_DECL_OPD_INSERTER (ins_advsimd_imm_modified);
AARCH64_DECL_OPD_INSERTER (ins_fbits);
AARCH64_DECL_OPD_INSERTER (ins_aimm);
AARCH64_DECL_OPD_INSERTER (ins_limm);
AARCH64_DECL_OPD_INSERTER (ins_ft);
AARCH64_DECL_OPD_INSERTER (ins_addr_simple);
AARCH64_DECL_OPD_INSERTER (ins_addr_regoff);
AARCH64_DECL_OPD_INSERTER (ins_addr_simm);
AARCH64_DECL_OPD_INSERTER (ins_addr_uimm12);
AARCH64_DECL_OPD_INSERTER (ins_simd_addr_post);
AARCH64_DECL_OPD_INSERTER (ins_cond);
AARCH64_DECL_OPD_INSERTER (ins_sysreg);
AARCH64_DECL_OPD_INSERTER (ins_pstatefield);
AARCH64_DECL_OPD_INSERTER (ins_sysins_op);
AARCH64_DECL_OPD_INSERTER (ins_barrier);
AARCH64_DECL_OPD_INSERTER (ins_prfop);
AARCH64_DECL_OPD_INSERTER (ins_reg_extended);
AARCH64_DECL_OPD_INSERTER (ins_reg_shifted);

#undef AARCH64_DECL_OPD_INSERTER

#endif /* OPCODES_AARCH64_ASM_H */
