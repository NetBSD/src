/* Nios II opcode list for GAS, the GNU assembler.
   Copyright (C) 2012-2015 Free Software Foundation, Inc.
   Contributed by Nigel Gray (ngray@altera.com).
   Contributed by Mentor Graphics, Inc.

   This file is part of GAS, the GNU Assembler, and GDB, the GNU disassembler.

   GAS/GDB is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS/GDB is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS or GDB; see the file COPYING3.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

#ifndef _NIOS2_H_
#define _NIOS2_H_

#include "bfd.h"

/****************************************************************************
 * This file contains structures, bit masks and shift counts used
 * by the GNU toolchain to define the Nios II instruction set and
 * access various opcode fields.
 ****************************************************************************/

/* Instruction encoding formats.  */
enum iw_format_type {
  /* R1 formats */
  iw_i_type,
  iw_r_type,
  iw_j_type,
  iw_custom_type
};

/* Identify different overflow situations for error messages.  */
enum overflow_type
{
  call_target_overflow = 0,
  branch_target_overflow,
  address_offset_overflow,
  signed_immed16_overflow,
  unsigned_immed16_overflow,
  unsigned_immed5_overflow,
  custom_opcode_overflow,
  no_overflow
};

/* This structure holds information for a particular instruction. 

   The args field is a string describing the operands.  The following
   letters can appear in the args:
     c - a 5-bit control register index
     d - a 5-bit destination register index
     s - a 5-bit left source register index
     t - a 5-bit right source register index
     i - a 16-bit signed immediate
     u - a 16-bit unsigned immediate
     o - a 16-bit signed program counter relative offset
     j - a 5-bit unsigned immediate
     l - a 8-bit custom instruction constant
     m - a 26-bit unsigned immediate
   Literal ',', '(', and ')' characters may also appear in the args as
   delimiters.

   Note that the args describe the semantics and assembly-language syntax
   of the operands, not their encoding into the instruction word.

   The pinfo field is INSN_MACRO for a macro.  Otherwise, it is a collection
   of bits describing the instruction, notably any relevant hazard
   information.

   When assembling, the match field contains the opcode template, which
   is modified by the arguments to produce the actual opcode
   that is emitted.  If pinfo is INSN_MACRO, then this is 0.

   If pinfo is INSN_MACRO, the mask field stores the macro identifier.
   Otherwise this is a bit mask for the relevant portions of the opcode
   when disassembling.  If the actual opcode anded with the match field
   equals the opcode field, then we have found the correct instruction.  */

struct nios2_opcode
{
  const char *name;		/* The name of the instruction.  */
  const char *args;		/* A string describing the arguments for this 
				   instruction.  */
  const char *args_test;	/* Like args, but with an extra argument for 
				   the expected opcode.  */
  unsigned long num_args;	/* The number of arguments the instruction 
				   takes.  */
  unsigned size;		/* Size in bytes of the instruction.  */
  enum iw_format_type format;	/* Instruction format.  */
  unsigned long match;		/* The basic opcode for the instruction.  */
  unsigned long mask;		/* Mask for the opcode field of the 
				   instruction.  */
  unsigned long pinfo;		/* Is this a real instruction or instruction 
				   macro?  */
  enum overflow_type overflow_msg;  /* Used to generate informative 
				       message when fixup overflows.  */
};

/* This value is used in the nios2_opcode.pinfo field to indicate that the 
   instruction is a macro or pseudo-op.  This requires special treatment by 
   the assembler, and is used by the disassembler to determine whether to 
   check for a nop.  */
#define NIOS2_INSN_MACRO	0x80000000
#define NIOS2_INSN_MACRO_MOV	0x80000001
#define NIOS2_INSN_MACRO_MOVI	0x80000002
#define NIOS2_INSN_MACRO_MOVIA	0x80000004

#define NIOS2_INSN_RELAXABLE	0x40000000
#define NIOS2_INSN_UBRANCH	0x00000010
#define NIOS2_INSN_CBRANCH	0x00000020
#define NIOS2_INSN_CALL		0x00000040

#define NIOS2_INSN_OPTARG	0x00000080

/* Register attributes.  */
#define REG_NORMAL	(1<<0)	/* Normal registers.  */
#define REG_CONTROL	(1<<1)  /* Control registers.  */
#define REG_COPROCESSOR	(1<<2)  /* For custom instructions.  */

struct nios2_reg
{
  const char *name;
  const int index;
  unsigned long regtype;
};

/* Pull in the instruction field accessors, opcodes, and masks.  */
#include "nios2r1.h"

/* These are the data structures used to hold the instruction information.  */
extern const struct nios2_opcode nios2_r1_opcodes[];
extern const int nios2_num_r1_opcodes;
extern struct nios2_opcode *nios2_opcodes;
extern int nios2_num_opcodes;

/* These are the data structures used to hold the register information.  */
extern const struct nios2_reg nios2_builtin_regs[];
extern struct nios2_reg *nios2_regs;
extern const int nios2_num_builtin_regs;
extern int nios2_num_regs;

/* Return the opcode descriptor for a single instruction.  */
extern const struct nios2_opcode *
nios2_find_opcode_hash (unsigned long, unsigned long);

#endif /* _NIOS2_H */
