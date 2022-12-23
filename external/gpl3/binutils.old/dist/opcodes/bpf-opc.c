/* DO NOT EDIT!  -*- buffer-read-only: t -*- vi:set ro:  */
/* Instruction opcode table for bpf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2020 Free Software Foundation, Inc.

This file is part of the GNU Binutils and/or GDB, the GNU debugger.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "sysdep.h"
#include "ansidecl.h"
#include "bfd.h"
#include "symcat.h"
#include "bpf-desc.h"
#include "bpf-opc.h"
#include "libiberty.h"

/* -- opc.c */

/* -- asm.c */
/* The hash functions are recorded here to help keep assembler code out of
   the disassembler and vice versa.  */

static int asm_hash_insn_p        (const CGEN_INSN *);
static unsigned int asm_hash_insn (const char *);
static int dis_hash_insn_p        (const CGEN_INSN *);
static unsigned int dis_hash_insn (const char *, CGEN_INSN_INT);

/* Instruction formats.  */

#define F(f) & bpf_cgen_ifld_table[BPF_##f]
static const CGEN_IFMT ifmt_empty ATTRIBUTE_UNUSED = {
  0, 0, 0x0, { { 0 } }
};

static const CGEN_IFMT ifmt_addile ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_addrle ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_negle ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_addibe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_addrbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_negbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_endlele ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_endlebe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_lddwle ATTRIBUTE_UNUSED = {
  8, 128, 0xff, { { F (F_IMM64) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_DSTLE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_lddwbe ATTRIBUTE_UNUSED = {
  8, 128, 0xff, { { F (F_IMM64) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_SRCBE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ldabsw ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_REGS) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ldindwle ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_DSTLE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ldindwbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_SRCBE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ldxwle ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_DSTLE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ldxwbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_SRCBE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_stble ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_DSTLE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_stbbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_MODE) }, { F (F_OP_SIZE) }, { F (F_SRCBE) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_jeqile ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_jeqrle ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_SRCLE) }, { F (F_OP_CODE) }, { F (F_DSTLE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_jeqibe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_jeqrbe ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_DSTBE) }, { F (F_OP_CODE) }, { F (F_SRCBE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_ja ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_REGS) }, { F (F_OP_CODE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_call ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_REGS) }, { F (F_OP_CODE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

static const CGEN_IFMT ifmt_exit ATTRIBUTE_UNUSED = {
  8, 64, 0xff, { { F (F_IMM32) }, { F (F_OFFSET16) }, { F (F_REGS) }, { F (F_OP_CODE) }, { F (F_OP_SRC) }, { F (F_OP_CLASS) }, { 0 } }
};

#undef F

#define A(a) (1 << CGEN_INSN_##a)
#define OPERAND(op) BPF_OPERAND_##op
#define MNEM CGEN_SYNTAX_MNEMONIC /* syntax value for mnemonic */
#define OP(field) CGEN_SYNTAX_MAKE_FIELD (OPERAND (field))

/* The instruction table.  */

static const CGEN_OPCODE bpf_cgen_insn_opcode_table[MAX_INSNS] =
{
  /* Special null first entry.
     A `num' value of zero is thus invalid.
     Also, the special `invalid' insn resides here.  */
  { { 0, 0, 0, 0 }, {{0}}, 0, {0}},
/* add $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x7 }
  },
/* add $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xf }
  },
/* add32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x4 }
  },
/* add32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xc }
  },
/* sub $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x17 }
  },
/* sub $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x1f }
  },
/* sub32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x14 }
  },
/* sub32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x1c }
  },
/* mul $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x27 }
  },
/* mul $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x2f }
  },
/* mul32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x24 }
  },
/* mul32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x2c }
  },
/* div $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x37 }
  },
/* div $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x3f }
  },
/* div32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x34 }
  },
/* div32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x3c }
  },
/* or $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x47 }
  },
/* or $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x4f }
  },
/* or32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x44 }
  },
/* or32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x4c }
  },
/* and $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x57 }
  },
/* and $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x5f }
  },
/* and32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x54 }
  },
/* and32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x5c }
  },
/* lsh $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x67 }
  },
/* lsh $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x6f }
  },
/* lsh32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x64 }
  },
/* lsh32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x6c }
  },
/* rsh $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x77 }
  },
/* rsh $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x7f }
  },
/* rsh32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x74 }
  },
/* rsh32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x7c }
  },
/* mod $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x97 }
  },
/* mod $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x9f }
  },
/* mod32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0x94 }
  },
/* mod32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0x9c }
  },
/* xor $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xa7 }
  },
/* xor $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xaf }
  },
/* xor32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xa4 }
  },
/* xor32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xac }
  },
/* mov $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xb7 }
  },
/* mov $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xbf }
  },
/* mov32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xb4 }
  },
/* mov32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xbc }
  },
/* arsh $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xc7 }
  },
/* arsh $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xcf }
  },
/* arsh32 $dstle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), 0 } },
    & ifmt_addile, { 0xc4 }
  },
/* arsh32 $dstle,$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), 0 } },
    & ifmt_addrle, { 0xcc }
  },
/* neg $dstle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), 0 } },
    & ifmt_negle, { 0x8f }
  },
/* neg32 $dstle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), 0 } },
    & ifmt_negle, { 0x8c }
  },
/* add $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x7 }
  },
/* add $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xf }
  },
/* add32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x4 }
  },
/* add32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xc }
  },
/* sub $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x17 }
  },
/* sub $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x1f }
  },
/* sub32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x14 }
  },
/* sub32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x1c }
  },
/* mul $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x27 }
  },
/* mul $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x2f }
  },
/* mul32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x24 }
  },
/* mul32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x2c }
  },
/* div $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x37 }
  },
/* div $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x3f }
  },
/* div32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x34 }
  },
/* div32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x3c }
  },
/* or $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x47 }
  },
/* or $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x4f }
  },
/* or32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x44 }
  },
/* or32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x4c }
  },
/* and $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x57 }
  },
/* and $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x5f }
  },
/* and32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x54 }
  },
/* and32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x5c }
  },
/* lsh $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x67 }
  },
/* lsh $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x6f }
  },
/* lsh32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x64 }
  },
/* lsh32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x6c }
  },
/* rsh $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x77 }
  },
/* rsh $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x7f }
  },
/* rsh32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x74 }
  },
/* rsh32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x7c }
  },
/* mod $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x97 }
  },
/* mod $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x9f }
  },
/* mod32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0x94 }
  },
/* mod32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0x9c }
  },
/* xor $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xa7 }
  },
/* xor $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xaf }
  },
/* xor32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xa4 }
  },
/* xor32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xac }
  },
/* mov $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xb7 }
  },
/* mov $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xbf }
  },
/* mov32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xb4 }
  },
/* mov32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xbc }
  },
/* arsh $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xc7 }
  },
/* arsh $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xcf }
  },
/* arsh32 $dstbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), 0 } },
    & ifmt_addibe, { 0xc4 }
  },
/* arsh32 $dstbe,$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), 0 } },
    & ifmt_addrbe, { 0xcc }
  },
/* neg $dstbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), 0 } },
    & ifmt_negbe, { 0x8f }
  },
/* neg32 $dstbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), 0 } },
    & ifmt_negbe, { 0x8c }
  },
/* endle $dstle,$endsize */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (ENDSIZE), 0 } },
    & ifmt_endlele, { 0xd4 }
  },
/* endbe $dstle,$endsize */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (ENDSIZE), 0 } },
    & ifmt_endlele, { 0xdc }
  },
/* endle $dstbe,$endsize */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (ENDSIZE), 0 } },
    & ifmt_endlebe, { 0xd4 }
  },
/* endbe $dstbe,$endsize */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (ENDSIZE), 0 } },
    & ifmt_endlebe, { 0xdc }
  },
/* lddw $dstle,$imm64 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM64), 0 } },
    & ifmt_lddwle, { 0x18 }
  },
/* lddw $dstbe,$imm64 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM64), 0 } },
    & ifmt_lddwbe, { 0x18 }
  },
/* ldabsw $imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (IMM32), 0 } },
    & ifmt_ldabsw, { 0x20 }
  },
/* ldabsh $imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (IMM32), 0 } },
    & ifmt_ldabsw, { 0x28 }
  },
/* ldabsb $imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (IMM32), 0 } },
    & ifmt_ldabsw, { 0x30 }
  },
/* ldabsdw $imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (IMM32), 0 } },
    & ifmt_ldabsw, { 0x38 }
  },
/* ldindw $srcle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCLE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwle, { 0x40 }
  },
/* ldindh $srcle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCLE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwle, { 0x48 }
  },
/* ldindb $srcle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCLE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwle, { 0x50 }
  },
/* ldinddw $srcle,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCLE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwle, { 0x58 }
  },
/* ldindw $srcbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCBE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwbe, { 0x40 }
  },
/* ldindh $srcbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCBE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwbe, { 0x48 }
  },
/* ldindb $srcbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCBE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwbe, { 0x50 }
  },
/* ldinddw $srcbe,$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (SRCBE), ',', OP (IMM32), 0 } },
    & ifmt_ldindwbe, { 0x58 }
  },
/* ldxw $dstle,[$srcle+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', '[', OP (SRCLE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwle, { 0x61 }
  },
/* ldxh $dstle,[$srcle+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', '[', OP (SRCLE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwle, { 0x69 }
  },
/* ldxb $dstle,[$srcle+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', '[', OP (SRCLE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwle, { 0x71 }
  },
/* ldxdw $dstle,[$srcle+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', '[', OP (SRCLE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwle, { 0x79 }
  },
/* stxw [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0x63 }
  },
/* stxh [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0x6b }
  },
/* stxb [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0x73 }
  },
/* stxdw [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0x7b }
  },
/* ldxw $dstbe,[$srcbe+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', '[', OP (SRCBE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwbe, { 0x61 }
  },
/* ldxh $dstbe,[$srcbe+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', '[', OP (SRCBE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwbe, { 0x69 }
  },
/* ldxb $dstbe,[$srcbe+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', '[', OP (SRCBE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwbe, { 0x71 }
  },
/* ldxdw $dstbe,[$srcbe+$offset16] */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', '[', OP (SRCBE), '+', OP (OFFSET16), ']', 0 } },
    & ifmt_ldxwbe, { 0x79 }
  },
/* stxw [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0x63 }
  },
/* stxh [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0x6b }
  },
/* stxb [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0x73 }
  },
/* stxdw [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0x7b }
  },
/* stb [$dstle+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stble, { 0x72 }
  },
/* sth [$dstle+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stble, { 0x6a }
  },
/* stw [$dstle+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stble, { 0x62 }
  },
/* stdw [$dstle+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stble, { 0x7a }
  },
/* stb [$dstbe+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stbbe, { 0x72 }
  },
/* sth [$dstbe+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stbbe, { 0x6a }
  },
/* stw [$dstbe+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stbbe, { 0x62 }
  },
/* stdw [$dstbe+$offset16],$imm32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (IMM32), 0 } },
    & ifmt_stbbe, { 0x7a }
  },
/* jeq $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x15 }
  },
/* jeq $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x1d }
  },
/* jgt $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x25 }
  },
/* jgt $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x2d }
  },
/* jge $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x35 }
  },
/* jge $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x3d }
  },
/* jlt $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0xa5 }
  },
/* jlt $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0xad }
  },
/* jle $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0xb5 }
  },
/* jle $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0xbd }
  },
/* jset $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x45 }
  },
/* jset $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x4d }
  },
/* jne $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x55 }
  },
/* jne $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x5d }
  },
/* jsgt $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x65 }
  },
/* jsgt $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x6d }
  },
/* jsge $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0x75 }
  },
/* jsge $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0x7d }
  },
/* jslt $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0xc5 }
  },
/* jslt $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0xcd }
  },
/* jsle $dstle,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqile, { 0xd5 }
  },
/* jsle $dstle,$srcle,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTLE), ',', OP (SRCLE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrle, { 0xdd }
  },
/* jeq $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x15 }
  },
/* jeq $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x1d }
  },
/* jgt $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x25 }
  },
/* jgt $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x2d }
  },
/* jge $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x35 }
  },
/* jge $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x3d }
  },
/* jlt $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0xa5 }
  },
/* jlt $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0xad }
  },
/* jle $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0xb5 }
  },
/* jle $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0xbd }
  },
/* jset $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x45 }
  },
/* jset $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x4d }
  },
/* jne $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x55 }
  },
/* jne $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x5d }
  },
/* jsgt $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x65 }
  },
/* jsgt $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x6d }
  },
/* jsge $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0x75 }
  },
/* jsge $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0x7d }
  },
/* jslt $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0xc5 }
  },
/* jslt $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0xcd }
  },
/* jsle $dstbe,$imm32,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (IMM32), ',', OP (DISP16), 0 } },
    & ifmt_jeqibe, { 0xd5 }
  },
/* jsle $dstbe,$srcbe,$disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DSTBE), ',', OP (SRCBE), ',', OP (DISP16), 0 } },
    & ifmt_jeqrbe, { 0xdd }
  },
/* ja $disp16 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DISP16), 0 } },
    & ifmt_ja, { 0x5 }
  },
/* call $disp32 */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', OP (DISP32), 0 } },
    & ifmt_call, { 0x85 }
  },
/* exit */
  {
    { 0, 0, 0, 0 },
    { { MNEM, 0 } },
    & ifmt_exit, { 0x95 }
  },
/* xadddw [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0xdb }
  },
/* xaddw [$dstle+$offset16],$srcle */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTLE), '+', OP (OFFSET16), ']', ',', OP (SRCLE), 0 } },
    & ifmt_ldxwle, { 0xc3 }
  },
/* xadddw [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0xdb }
  },
/* xaddw [$dstbe+$offset16],$srcbe */
  {
    { 0, 0, 0, 0 },
    { { MNEM, ' ', '[', OP (DSTBE), '+', OP (OFFSET16), ']', ',', OP (SRCBE), 0 } },
    & ifmt_ldxwbe, { 0xc3 }
  },
};

#undef A
#undef OPERAND
#undef MNEM
#undef OP

/* Formats for ALIAS macro-insns.  */

#define F(f) & bpf_cgen_ifld_table[BPF_##f]
#undef F

/* Each non-simple macro entry points to an array of expansion possibilities.  */

#define A(a) (1 << CGEN_INSN_##a)
#define OPERAND(op) BPF_OPERAND_##op
#define MNEM CGEN_SYNTAX_MNEMONIC /* syntax value for mnemonic */
#define OP(field) CGEN_SYNTAX_MAKE_FIELD (OPERAND (field))

/* The macro instruction table.  */

static const CGEN_IBASE bpf_cgen_macro_insn_table[] =
{
};

/* The macro instruction opcode table.  */

static const CGEN_OPCODE bpf_cgen_macro_insn_opcode_table[] =
{
};

#undef A
#undef OPERAND
#undef MNEM
#undef OP

#ifndef CGEN_ASM_HASH_P
#define CGEN_ASM_HASH_P(insn) 1
#endif

#ifndef CGEN_DIS_HASH_P
#define CGEN_DIS_HASH_P(insn) 1
#endif

/* Return non-zero if INSN is to be added to the hash table.
   Targets are free to override CGEN_{ASM,DIS}_HASH_P in the .opc file.  */

static int
asm_hash_insn_p (const CGEN_INSN *insn ATTRIBUTE_UNUSED)
{
  return CGEN_ASM_HASH_P (insn);
}

static int
dis_hash_insn_p (const CGEN_INSN *insn)
{
  /* If building the hash table and the NO-DIS attribute is present,
     ignore.  */
  if (CGEN_INSN_ATTR_VALUE (insn, CGEN_INSN_NO_DIS))
    return 0;
  return CGEN_DIS_HASH_P (insn);
}

#ifndef CGEN_ASM_HASH
#define CGEN_ASM_HASH_SIZE 127
#ifdef CGEN_MNEMONIC_OPERANDS
#define CGEN_ASM_HASH(mnem) (*(unsigned char *) (mnem) % CGEN_ASM_HASH_SIZE)
#else
#define CGEN_ASM_HASH(mnem) (*(unsigned char *) (mnem) % CGEN_ASM_HASH_SIZE) /*FIXME*/
#endif
#endif

/* It doesn't make much sense to provide a default here,
   but while this is under development we do.
   BUFFER is a pointer to the bytes of the insn, target order.
   VALUE is the first base_insn_bitsize bits as an int in host order.  */

#ifndef CGEN_DIS_HASH
#define CGEN_DIS_HASH_SIZE 256
#define CGEN_DIS_HASH(buf, value) (*(unsigned char *) (buf))
#endif

/* The result is the hash value of the insn.
   Targets are free to override CGEN_{ASM,DIS}_HASH in the .opc file.  */

static unsigned int
asm_hash_insn (const char *mnem)
{
  return CGEN_ASM_HASH (mnem);
}

/* BUF is a pointer to the bytes of the insn, target order.
   VALUE is the first base_insn_bitsize bits as an int in host order.  */

static unsigned int
dis_hash_insn (const char *buf ATTRIBUTE_UNUSED,
		     CGEN_INSN_INT value ATTRIBUTE_UNUSED)
{
  return CGEN_DIS_HASH (buf, value);
}

/* Set the recorded length of the insn in the CGEN_FIELDS struct.  */

static void
set_fields_bitsize (CGEN_FIELDS *fields, int size)
{
  CGEN_FIELDS_BITSIZE (fields) = size;
}

/* Function to call before using the operand instance table.
   This plugs the opcode entries and macro instructions into the cpu table.  */

void
bpf_cgen_init_opcode_table (CGEN_CPU_DESC cd)
{
  int i;
  int num_macros = (sizeof (bpf_cgen_macro_insn_table) /
		    sizeof (bpf_cgen_macro_insn_table[0]));
  const CGEN_IBASE *ib = & bpf_cgen_macro_insn_table[0];
  const CGEN_OPCODE *oc = & bpf_cgen_macro_insn_opcode_table[0];
  CGEN_INSN *insns = xmalloc (num_macros * sizeof (CGEN_INSN));

  /* This test has been added to avoid a warning generated
     if memset is called with a third argument of value zero.  */
  if (num_macros >= 1)
    memset (insns, 0, num_macros * sizeof (CGEN_INSN));
  for (i = 0; i < num_macros; ++i)
    {
      insns[i].base = &ib[i];
      insns[i].opcode = &oc[i];
      bpf_cgen_build_insn_regex (& insns[i]);
    }
  cd->macro_insn_table.init_entries = insns;
  cd->macro_insn_table.entry_size = sizeof (CGEN_IBASE);
  cd->macro_insn_table.num_init_entries = num_macros;

  oc = & bpf_cgen_insn_opcode_table[0];
  insns = (CGEN_INSN *) cd->insn_table.init_entries;
  for (i = 0; i < MAX_INSNS; ++i)
    {
      insns[i].opcode = &oc[i];
      bpf_cgen_build_insn_regex (& insns[i]);
    }

  cd->sizeof_fields = sizeof (CGEN_FIELDS);
  cd->set_fields_bitsize = set_fields_bitsize;

  cd->asm_hash_p = asm_hash_insn_p;
  cd->asm_hash = asm_hash_insn;
  cd->asm_hash_size = CGEN_ASM_HASH_SIZE;

  cd->dis_hash_p = dis_hash_insn_p;
  cd->dis_hash = dis_hash_insn;
  cd->dis_hash_size = CGEN_DIS_HASH_SIZE;
}
