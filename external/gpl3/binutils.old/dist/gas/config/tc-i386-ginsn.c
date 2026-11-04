/* tc-i386-ginsn.c -- Ginsn generation for the x86-64 ISA

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

/* This file contains the implementation of the ginsn creation for x86-64
   instructions.  */

/* DWARF register number for EFLAGS.  Used for pushf/popf insns.  */
#define GINSN_DW2_REGNUM_EFLAGS     49
/* DWARF register number for RSI.  Used as dummy value when RegIP/RegIZ.  */
#define GINSN_DW2_REGNUM_RSI_DUMMY  4

/* Identify the callee-saved registers in System V AMD64 ABI.  */

bool
x86_scfi_callee_saved_p (unsigned int dw2reg_num)
{
  if (dw2reg_num == 3 /* rbx.  */
      || dw2reg_num == REG_FP /* rbp.  */
      || dw2reg_num == REG_SP /* rsp.  */
      || (dw2reg_num >= 12 && dw2reg_num <= 15) /* r12 - r15.  */)
    return true;

  return false;
}

/* Check whether an instruction prefix which affects operation size
   accompanies.  For insns in the legacy space, setting REX.W takes precedence
   over the operand-size prefix (66H) when both are used.

   The current users of this API are in the handlers for PUSH, POP or other
   instructions which affect the stack pointer implicitly:  the operation size
   (16, 32, or 64 bits) determines the amount by which the stack pointer is
   incremented / decremented (2, 4 or 8).  */

static bool
ginsn_opsize_prefix_p (void)
{
  return (!(i.prefix[REX_PREFIX] & REX_W) && i.prefix[DATA_PREFIX]);
}

/* Get the DWARF register number for the given register entry.
   For specific byte/word/dword register accesses like al, cl, ah, ch, r8d,
   r20w etc., we need to identify the DWARF register number for the
   corresponding 8-byte GPR.

   This function is a hack - it relies on relative ordering of reg entries in
   the i386_regtab.  FIXME - it will be good to allow a more direct way to get
   this information.  */

static unsigned int
ginsn_dw2_regnum (const reg_entry *ireg)
{
  const reg_entry *temp = ireg;
  unsigned int dwarf_reg = Dw2Inval, idx = 0;

  /* ginsn creation is available for AMD64 abi only ATM.  Other flag_code
     are not expected.  */
  gas_assert (ireg && flag_code == CODE_64BIT);

  /* Watch out for RegIP, RegIZ.  These are expected to appear only with
     base/index addressing modes.  Although creating inaccurate data
     dependencies, using a dummy value (lets say volatile register rsi) will
     not hurt SCFI.  TBD_GINSN_GEN_NOT_SCFI.  */
  if (ireg->reg_num == RegIP || ireg->reg_num == RegIZ)
    return GINSN_DW2_REGNUM_RSI_DUMMY;

  dwarf_reg = ireg->dw2_regnum[object_64bit];

  if (dwarf_reg == Dw2Inval)
    {
      if (ireg <= &i386_regtab[3])
	/* For al, cl, dl, bl, bump over to axl, cxl, dxl, bxl respectively by
	   adding 8.  */
	temp = ireg + 8;
      else if (ireg <= &i386_regtab[7])
	/* For ah, ch, dh, bh, bump over to axl, cxl, dxl, bxl respectively by
	   adding 4.  */
	temp = ireg + 4;
      else
	{
	  /* The code relies on the relative ordering of the reg entries in
	     i386_regtab.  There are 32 register entries between axl-r31b,
	     ax-r31w etc.  The assertions here ensures the code does not
	     recurse indefinitely.  */
	  gas_assert ((temp - &i386_regtab[0]) >= 0);
	  idx = temp - &i386_regtab[0];
	  gas_assert (idx + 32 < i386_regtab_size - 1);

	  temp = temp + 32;
	}

      dwarf_reg = ginsn_dw2_regnum (temp);
    }

  /* Sanity check - failure may indicate state corruption, bad ginsn or
     perhaps the i386-reg table and the current function got out of sync.  */
  gas_assert (dwarf_reg < Dw2Inval);

  return dwarf_reg;
}

static ginsnS *
x86_ginsn_addsub_reg_mem (const symbolS *insn_end_sym)
{
  unsigned int dw2_regnum;
  unsigned int src1_dw2_regnum;
  ginsnS *ginsn = NULL;
  ginsnS * (*ginsn_func) (const symbolS *, bool,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_dst_type, unsigned int, offsetT);
  uint16_t opcode = i.tm.base_opcode;

  gas_assert (i.tm.opcode_space == SPACE_BASE
	      && (opcode == 0x1 || opcode == 0x29));
  ginsn_func = (opcode == 0x1) ? ginsn_new_add : ginsn_new_sub;

  /* op %reg, symbol or even other cases where destination involves indirect
     access are unnecessary for SCFI correctness.  TBD_GINSN_GEN_NOT_SCFI.  */
  if (i.mem_operands)
    return ginsn;

  /* Skip detection of 8/16/32-bit op size; 'add/sub reg, reg/mem' ops always
     make the dest reg untraceable for SCFI.  */

  /* op reg, reg/mem.  */
  src1_dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
  /* Of interest only when second opnd is not memory.  */
  if (i.reg_operands == 2)
    {
      dw2_regnum = ginsn_dw2_regnum (i.op[1].regs);
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_REG, src1_dw2_regnum, 0,
			  GINSN_SRC_REG, dw2_regnum, 0,
			  GINSN_DST_REG, dw2_regnum, 0);
      ginsn_set_where (ginsn);
    }

  return ginsn;
}

static ginsnS *
x86_ginsn_addsub_mem_reg (const symbolS *insn_end_sym)
{
  unsigned int dw2_regnum;
  unsigned int src1_dw2_regnum;
  const reg_entry *mem_reg;
  int32_t gdisp = 0;
  ginsnS *ginsn = NULL;
  ginsnS * (*ginsn_func) (const symbolS *, bool,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_dst_type, unsigned int, offsetT);
  uint16_t opcode = i.tm.base_opcode;

  gas_assert (i.tm.opcode_space == SPACE_BASE
	      && (opcode == 0x3 || opcode == 0x2b));
  ginsn_func = (opcode == 0x3) ? ginsn_new_add : ginsn_new_sub;

  /* op symbol, %reg.  */
  if (i.mem_operands && !i.base_reg && !i.index_reg)
    return ginsn;

  /* Skip detection of 8/16/32-bit op size; 'add/sub reg/mem, reg' ops always
     make the dest reg untraceable for SCFI.  */

  /* op reg/mem, %reg.  */
  dw2_regnum = ginsn_dw2_regnum (i.op[1].regs);

  if (i.reg_operands == 2)
    {
      src1_dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_REG, src1_dw2_regnum, 0,
			  GINSN_SRC_REG, dw2_regnum, 0,
			  GINSN_DST_REG, dw2_regnum, 0);
      ginsn_set_where (ginsn);
    }
  else if (i.mem_operands)
    {
      mem_reg = (i.base_reg) ? i.base_reg : i.index_reg;
      src1_dw2_regnum = ginsn_dw2_regnum (mem_reg);
      if (i.disp_operands == 1)
	gdisp = i.op[0].disps->X_add_number;
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_INDIRECT, src1_dw2_regnum, gdisp,
			  GINSN_SRC_REG, dw2_regnum, 0,
			  GINSN_DST_REG, dw2_regnum, 0);
      ginsn_set_where (ginsn);
    }

  return ginsn;
}

static ginsnS *
x86_ginsn_alu_imm (const symbolS *insn_end_sym)
{
  offsetT src_imm;
  unsigned int dw2_regnum;
  ginsnS *ginsn = NULL;
  enum ginsn_src_type src_type = GINSN_SRC_REG;
  enum ginsn_dst_type dst_type = GINSN_DST_REG;

  ginsnS * (*ginsn_func) (const symbolS *, bool,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_src_type, unsigned int, offsetT,
			  enum ginsn_dst_type, unsigned int, offsetT);

  /* FIXME - create ginsn where dest is REG_SP / REG_FP only ? */
  /* Map for insn.tm.extension_opcode
     000 ADD    100 AND
     001 OR     101 SUB
     010 ADC    110 XOR
     011 SBB    111 CMP  */

  /* add/sub/and imm, %reg only at this time for SCFI.
     Although all three ('and', 'or' , 'xor') make the destination reg
     untraceable, 'and' op is handled but not 'or' / 'xor' because we will look
     into supporting the DRAP pattern at some point.  Other opcodes ('adc',
     'sbb' and 'cmp') are not generated here either.  The ginsn representation
     does not have support for the latter three opcodes;  GINSN_TYPE_OTHER may
     be added for these after x86_ginsn_unhandled () invocation if the
     destination register is REG_SP or REG_FP.  */
  if (i.tm.extension_opcode == 5)
    ginsn_func = ginsn_new_sub;
  else if (i.tm.extension_opcode == 4)
    ginsn_func = ginsn_new_and;
  else if (i.tm.extension_opcode == 0)
    ginsn_func = ginsn_new_add;
  else
    return ginsn;

  /* TBD_GINSN_REPRESENTATION_LIMIT: There is no representation for when a
     symbol is used as an operand, like so:
	  addq    $simd_cmp_op+8, %rdx
     Skip generating any ginsn for this.  */
  if (i.imm_operands == 1
      && i.op[0].imms->X_op != O_constant)
    return ginsn;

  /* addq    $1, symbol
     addq    $1, -16(%rbp)
     These are not of interest for SCFI.  Also, TBD_GINSN_GEN_NOT_SCFI.  */
  if (i.mem_operands == 1)
    return ginsn;

  /* 8/16/32-bit op size makes the destination reg untraceable for SCFI.
     Deal with this via the x86_ginsn_unhandled () code path.  */
  if (i.suffix != QWORD_MNEM_SUFFIX)
    return ginsn;

  gas_assert (i.imm_operands == 1);
  src_imm = i.op[0].imms->X_add_number;
  /* The second operand may be a register or indirect access.  For SCFI, only
     the case when the second opnd is a register is interesting.  Revisit this
     if generating ginsns for a different gen mode TBD_GINSN_GEN_NOT_SCFI.  */
  if (i.reg_operands == 1)
    {
      dw2_regnum = ginsn_dw2_regnum (i.op[1].regs);
      /* For ginsn, keep the imm as second src operand.  */
      ginsn = ginsn_func (insn_end_sym, true,
			  src_type, dw2_regnum, 0,
			  GINSN_SRC_IMM, 0, src_imm,
			  dst_type, dw2_regnum, 0);

      ginsn_set_where (ginsn);
    }

  return ginsn;
}

/* Create ginsn(s) for MOV operations.

   The generated ginsns corresponding to mov with indirect access to memory
   (src or dest) suffer with loss of information: when both index and base
   registers are at play, only base register gets conveyed in ginsn.  Note
   this TBD_GINSN_GEN_NOT_SCFI.  */

static ginsnS *
x86_ginsn_move (const symbolS *insn_end_sym)
{
  ginsnS *ginsn = NULL;
  unsigned int dst_reg;
  unsigned int src_reg;
  offsetT src_disp = 0;
  offsetT dst_disp = 0;
  const reg_entry *dst = NULL;
  const reg_entry *src = NULL;
  uint16_t opcode = i.tm.base_opcode;
  enum ginsn_src_type src_type = GINSN_SRC_REG;
  enum ginsn_dst_type dst_type = GINSN_DST_REG;

  /* mov %reg, symbol or mov symbol, %reg.
     Not of interest for SCFI.  Also, TBD_GINSN_GEN_NOT_SCFI.  */
  if (i.mem_operands == 1 && !i.base_reg && !i.index_reg)
    return ginsn;

  /* 8/16/32-bit op size makes the destination reg untraceable for SCFI.
     Handle mov reg, reg only.  mov to or from a memory operand will make
     dest reg, when present, untraceable, irrespective of the op size.  */
  if (i.reg_operands == 2 && i.suffix != QWORD_MNEM_SUFFIX)
    return ginsn;

  gas_assert (i.tm.opcode_space == SPACE_BASE);
  if (opcode == 0x8b || opcode == 0x8a)
    {
      /* mov  disp(%reg), %reg.  */
      if (i.mem_operands)
	{
	  src = (i.base_reg) ? i.base_reg : i.index_reg;
	  if (i.disp_operands == 1)
	    src_disp = i.op[0].disps->X_add_number;
	  src_type = GINSN_SRC_INDIRECT;
	}
      else
	src = i.op[0].regs;

      dst = i.op[1].regs;
    }
  else if (opcode == 0x89 || opcode == 0x88)
    {
      /* mov %reg, disp(%reg).  */
      src = i.op[0].regs;
      if (i.mem_operands)
	{
	  dst = (i.base_reg) ? i.base_reg : i.index_reg;
	  if (i.disp_operands == 1)
	    dst_disp = i.op[1].disps->X_add_number;
	  dst_type = GINSN_DST_INDIRECT;
	}
      else
	dst = i.op[1].regs;
    }

  src_reg = ginsn_dw2_regnum (src);
  dst_reg = ginsn_dw2_regnum (dst);

  ginsn = ginsn_new_mov (insn_end_sym, true,
			 src_type, src_reg, src_disp,
			 dst_type, dst_reg, dst_disp);
  ginsn_set_where (ginsn);

  return ginsn;
}

/* Generate appropriate ginsn for lea.

   Unhandled sub-cases (marked with TBD_GINSN_GEN_NOT_SCFI) also suffer with
   some loss of information in the final ginsn chosen eventually (type
   GINSN_TYPE_OTHER).  But this is fine for now for GINSN_GEN_SCFI generation
   mode.  */

static ginsnS *
x86_ginsn_lea (const symbolS *insn_end_sym)
{
  offsetT src_disp = 0;
  ginsnS *ginsn = NULL;
  unsigned int src1_reg;
  const reg_entry *src1;
  offsetT index_scale;
  unsigned int dst_reg;
  bool index_regiz_p;

  if ((!i.base_reg) != (!i.index_reg || i.index_reg->reg_num == RegIZ))
    {
      /* lea disp(%base), %dst    or    lea disp(,%index,imm), %dst.
	 Either index_reg or base_reg exists, but not both.  Further, as per
	 above, the case when just %index exists but is equal to RegIZ is
	 excluded.  If not excluded, a GINSN_TYPE_MOV of %rsi
	 (GINSN_DW2_REGNUM_RSI_DUMMY) to %dst will be generated by this block.
	 Such a mov ginsn is imprecise; so, exclude now and generate
	 GINSN_TYPE_OTHER instead later via the x86_ginsn_unhandled ().
	 Excluding other cases is required due to
	 TBD_GINSN_REPRESENTATION_LIMIT.  */

      index_scale = i.log2_scale_factor;
      index_regiz_p = i.index_reg && i.index_reg->reg_num == RegIZ;
      src1 = i.base_reg ? i.base_reg : i.index_reg;
      src1_reg = ginsn_dw2_regnum (src1);
      dst_reg = ginsn_dw2_regnum (i.op[1].regs);
      /* It makes sense to represent a scale factor of 1 precisely here
	 (i.e., not using GINSN_TYPE_OTHER, but rather similar to the
	 base-without-index case).  A non-zero scale factor is still OK if
	 the index reg is zero reg.
	 However, skip from here the case when disp has a symbol instead.
	 TBD_GINSN_REPRESENTATION_LIMIT.  */
      if ((!index_scale || index_regiz_p)
	  && (!i.disp_operands || i.op[0].disps->X_op == O_constant))
	{
	  if (i.disp_operands)
	    src_disp = i.op[0].disps->X_add_number;

	  if (src_disp)
	    /* Generate an ADD ginsn.  */
	    ginsn = ginsn_new_add (insn_end_sym, true,
				   GINSN_SRC_REG, src1_reg, 0,
				   GINSN_SRC_IMM, 0, src_disp,
				   GINSN_DST_REG, dst_reg, 0);
	  else
	    /* Generate a MOV ginsn.  */
	    ginsn = ginsn_new_mov (insn_end_sym, true,
				   GINSN_SRC_REG, src1_reg, 0,
				   GINSN_DST_REG, dst_reg, 0);

	  ginsn_set_where (ginsn);
	}
    }
  /* Skip handling other cases here,
     - when (i.index_reg && i.base_reg) is true,
       e.g., lea disp(%base,%index,imm), %dst
       We do not have a ginsn representation for multiply.
     - or, when (!i.index_reg && !i.base_reg) is true,
       e.g., lea symbol, %dst
       Not a frequent pattern.  If %dst is a register of interest, the user is
       likely to use a MOV op anyway.
     Deal with these via the x86_ginsn_unhandled () code path to generate
     GINSN_TYPE_OTHER when necessary.  TBD_GINSN_GEN_NOT_SCFI.  */

  return ginsn;
}

static ginsnS *
x86_ginsn_jump (const symbolS *insn_end_sym, bool cond_p)
{
  ginsnS *ginsn = NULL;
  const symbolS *src_symbol;
  ginsnS * (*ginsn_func) (const symbolS *sym, bool real_p,
			  enum ginsn_src_type src_type, unsigned int src_reg,
			  const symbolS *src_ginsn_sym);

  gas_assert (i.disp_operands == 1);

  ginsn_func = cond_p ? ginsn_new_jump_cond : ginsn_new_jump;
  if (i.op[0].disps->X_op == O_symbol && !i.op[0].disps->X_add_number)
    {
      src_symbol = i.op[0].disps->X_add_symbol;
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_SYMBOL, 0, src_symbol);

      ginsn_set_where (ginsn);
    }
  else
    {
      /* A non-zero addend in jump/JCC target makes control-flow tracking
	 difficult.  Skip SCFI for now.  */
      as_bad (_("SCFI: `%s' insn with non-zero addend to sym not supported"),
	      cond_p ? "JCC" : "jmp");
      return ginsn;
    }

  return ginsn;
}

static ginsnS *
x86_ginsn_indirect_branch (const symbolS *insn_end_sym)
{
  ginsnS *ginsn = NULL;
  const reg_entry *mem_reg;
  unsigned int dw2_regnum;

  ginsnS * (*ginsn_func) (const symbolS *sym, bool real_p,
			  enum ginsn_src_type src_type, unsigned int src_reg,
			  const symbolS *src_ginsn_sym);

  /* Other cases are not expected.  */
  gas_assert (i.tm.extension_opcode == 4 || i.tm.extension_opcode == 2);

  if (i.tm.extension_opcode == 4)
    /* 0xFF /4 (jmp r/m).  */
    ginsn_func = ginsn_new_jump;
  else if (i.tm.extension_opcode == 2)
    /* 0xFF /2 (call r/m).  */
    ginsn_func = ginsn_new_call;
  else
    return ginsn;

  if (i.reg_operands)
    {
      dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_REG, dw2_regnum, NULL);
      ginsn_set_where (ginsn);
    }
  else if (i.mem_operands)
    {
      /* Handle jump/call near, absolute indirect, address.
	 E.g., jmp/call *imm(%rN),  jmp/call *sym(,%rN,imm)
	 or  jmp/call *sym(%rN) etc.  */
      mem_reg = i.base_reg ? i.base_reg : i.index_reg;
      /* Generate a ginsn, even if it is with TBD_GINSN_INFO_LOSS.  Otherwise,
	 the user gets the impression of missing functionality due to this
	 being a COFI and alerted for via the x86_ginsn_unhandled () workflow
	 as unhandled operation (which can be misleading for users).

	 Indirect branches make the code block ineligible for SCFI; Hence, an
	 approximate ginsn will not affect SCFI correctness:
	   - Use dummy register if no base or index
	   - Skip symbol information, if any.
	 Note this case of TBD_GINSN_GEN_NOT_SCFI.  */
      dw2_regnum = (mem_reg
		    ? ginsn_dw2_regnum (mem_reg)
		    : GINSN_DW2_REGNUM_RSI_DUMMY);
      ginsn = ginsn_func (insn_end_sym, true,
			  GINSN_SRC_REG, dw2_regnum, NULL);
      ginsn_set_where (ginsn);
    }

  return ginsn;
}

static ginsnS *
x86_ginsn_enter (const symbolS *insn_end_sym)
{
  ginsnS *ginsn = NULL;
  ginsnS *ginsn_next = NULL;
  ginsnS *ginsn_last = NULL;
  /* In 64-bit mode, the default stack update size is 8 bytes.  */
  int stack_opnd_size = 8;

  gas_assert (i.imm_operands == 2);

  /* For non-zero size operands, bail out as untraceable for SCFI.  */
  if (i.op[0].imms->X_op != O_constant || i.op[0].imms->X_add_symbol != 0
      || i.op[1].imms->X_op != O_constant || i.op[1].imms->X_add_symbol != 0)
    {
      as_bad ("SCFI: enter insn with non-zero operand not supported");
      return ginsn;
    }

  /* Check if this is a 16-bit op.  */
  if (ginsn_opsize_prefix_p ())
    stack_opnd_size = 2;

  /* If the nesting level is 0, the processor pushes the frame pointer from
     the BP/EBP/RBP register onto the stack, copies the current stack
     pointer from the SP/ESP/RSP register into the BP/EBP/RBP register, and
     loads the SP/ESP/RSP register with the current stack-pointer value
     minus the value in the size operand.  */
  ginsn = ginsn_new_sub (insn_end_sym, false,
			 GINSN_SRC_REG, REG_SP, 0,
			 GINSN_SRC_IMM, 0, stack_opnd_size,
			 GINSN_DST_REG, REG_SP, 0);
  ginsn_set_where (ginsn);
  ginsn_next = ginsn_new_store (insn_end_sym, false,
				GINSN_SRC_REG, REG_FP,
				GINSN_DST_INDIRECT, REG_SP, 0);
  ginsn_set_where (ginsn_next);
  gas_assert (!ginsn_link_next (ginsn, ginsn_next));
  ginsn_last = ginsn_new_mov (insn_end_sym, false,
			      GINSN_SRC_REG, REG_SP, 0,
			      GINSN_DST_REG, REG_FP, 0);
  ginsn_set_where (ginsn_last);
  gas_assert (!ginsn_link_next (ginsn_next, ginsn_last));

  return ginsn;
}

static ginsnS *
x86_ginsn_leave (const symbolS *insn_end_sym)
{
  ginsnS *ginsn = NULL;
  ginsnS *ginsn_next = NULL;
  ginsnS *ginsn_last = NULL;
  /* In 64-bit mode, the default stack update size is 8 bytes.  */
  int stack_opnd_size = 8;

  /* Check if this is a 16-bit op.  */
  if (ginsn_opsize_prefix_p ())
    stack_opnd_size = 2;

  /* The 'leave' instruction copies the contents of the RBP register
     into the RSP register to release all stack space allocated to the
     procedure.  */
  ginsn = ginsn_new_mov (insn_end_sym, false,
			 GINSN_SRC_REG, REG_FP, 0,
			 GINSN_DST_REG, REG_SP, 0);
  ginsn_set_where (ginsn);
  /* Then it restores the old value of the RBP register from the stack.  */
  ginsn_next = ginsn_new_load (insn_end_sym, false,
			       GINSN_SRC_INDIRECT, REG_SP, 0,
			       GINSN_DST_REG, REG_FP);
  ginsn_set_where (ginsn_next);
  gas_assert (!ginsn_link_next (ginsn, ginsn_next));
  ginsn_last = ginsn_new_add (insn_end_sym, false,
			      GINSN_SRC_REG, REG_SP, 0,
			      GINSN_SRC_IMM, 0, stack_opnd_size,
			      GINSN_DST_REG, REG_SP, 0);
  ginsn_set_where (ginsn_next);
  gas_assert (!ginsn_link_next (ginsn_next, ginsn_last));

  return ginsn;
}

/* Check if an instruction is whitelisted.

   Some instructions may appear with REG_SP or REG_FP as destination, because
   which they are deemed 'interesting' for SCFI.  Whitelist them here if they
   do not affect SCFI correctness.  */

static bool
x86_ginsn_safe_to_skip_p (void)
{
  bool skip_p = false;
  uint16_t opcode = i.tm.base_opcode;

  switch (opcode)
    {
    case 0x80:
    case 0x81:
    case 0x83:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* cmp imm, reg/rem.  */
      if (i.tm.extension_opcode == 7)
	skip_p = true;
      break;

    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* cmp imm/reg/mem, reg/rem.  */
      skip_p = true;
      break;

    case 0xf6:
    case 0xf7:
    case 0x84:
    case 0x85:
      /* test imm/reg/mem, reg/mem.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      skip_p = true;
      break;

    default:
      break;
    }

  return skip_p;
}

#define X86_GINSN_UNHANDLED_NONE        0
#define X86_GINSN_UNHANDLED_DEST_REG    1
#define X86_GINSN_UNHANDLED_CFG         2
#define X86_GINSN_UNHANDLED_STACKOP     3
#define X86_GINSN_UNHANDLED_UNEXPECTED  4

/* Check the input insn for its impact on the correctness of the synthesized
   CFI.  Returns an error code to the caller.  */

static int
x86_ginsn_unhandled (void)
{
  int err = X86_GINSN_UNHANDLED_NONE;
  const reg_entry *reg_op;
  unsigned int dw2_regnum;

  /* Keep an eye out for instructions affecting control flow.  */
  if (i.tm.opcode_modifier.jump)
    err = X86_GINSN_UNHANDLED_CFG;
  /* Also, for any instructions involving an implicit update to the stack
     pointer.  */
  else if (i.tm.opcode_modifier.operandconstraint == IMPLICIT_STACK_OP)
    err = X86_GINSN_UNHANDLED_STACKOP;
  /* Finally, also check if the missed instructions are affecting REG_SP or
     REG_FP.  The destination operand is the last at all stages of assembly
     (due to following AT&T syntax layout in the internal representation).  In
     case of Intel syntax input, this still remains true as swap_operands ()
     is done by now.
     PS: These checks do not involve index / base reg, as indirect memory
     accesses via REG_SP or REG_FP do not affect SCFI correctness.
     (Also note these instructions are candidates for other ginsn generation
     modes in future.  TBD_GINSN_GEN_NOT_SCFI.)  */
  else if (i.operands && i.reg_operands
	   && !(i.flags[i.operands - 1] & Operand_Mem))
    {
      reg_op = i.op[i.operands - 1].regs;
      if (reg_op)
	{
	  dw2_regnum = ginsn_dw2_regnum (reg_op);
	  if (dw2_regnum == REG_SP || dw2_regnum == REG_FP)
	    err = X86_GINSN_UNHANDLED_DEST_REG;
	}
      else
	/* Something unexpected.  Indicate to caller.  */
	err = X86_GINSN_UNHANDLED_UNEXPECTED;
    }

  return err;
}

/* Generate one or more generic GAS instructions, a.k.a, ginsns for the current
   machine instruction.

   Returns the head of linked list of ginsn(s) added, if success; Returns NULL
   if failure.

   The input ginsn_gen_mode GMODE determines the set of minimal necessary
   ginsns necessary for correctness of any passes applicable for that mode.
   For supporting the GINSN_GEN_SCFI generation mode, following is the list of
   machine instructions that must be translated into the corresponding ginsns
   to ensure correctness of SCFI:
     - All instructions affecting the two registers that could potentially
       be used as the base register for CFA tracking.  For SCFI, the base
       register for CFA tracking is limited to REG_SP and REG_FP only for
       now.
     - All change of flow instructions: conditional and unconditional branches,
       call and return from functions.
     - All instructions that can potentially be a register save / restore
       operation.
     - All instructions that perform stack manipulation implicitly: the CALL,
       RET, PUSH, POP, ENTER, and LEAVE instructions.

   The function currently supports GINSN_GEN_SCFI ginsn generation mode only.
   To support other generation modes will require work on this target-specific
   process of creation of ginsns:
     - Some of such places are tagged with TBD_GINSN_GEN_NOT_SCFI to serve as
       possible starting points.
     - Also note that ginsn representation may need enhancements.  Specifically,
       note some TBD_GINSN_INFO_LOSS and TBD_GINSN_REPRESENTATION_LIMIT markers.
   */

static ginsnS *
x86_ginsn_new (const symbolS *insn_end_sym, enum ginsn_gen_mode gmode)
{
  int err = 0;
  uint16_t opcode;
  unsigned int dw2_regnum;
  const reg_entry *mem_reg;
  ginsnS *ginsn = NULL;
  ginsnS *ginsn_next = NULL;
  /* In 64-bit mode, the default stack update size is 8 bytes.  */
  int stack_opnd_size = 8;

  /* Currently supports generation of selected ginsns, sufficient for
     the use-case of SCFI only.  */
  if (gmode != GINSN_GEN_SCFI)
    return ginsn;

  opcode = i.tm.base_opcode;

  /* Until it is clear how to handle APX NDD and other new opcodes, disallow
     them from SCFI.  */
  if (is_apx_rex2_encoding ()
      || (i.tm.opcode_modifier.evex && is_apx_evex_encoding ()))
    {
      as_bad (_("SCFI: unsupported APX op %#x may cause incorrect CFI"),
	      opcode);
      return ginsn;
    }

  switch (opcode)
    {

    /* Add opcodes 0x0/0x2 and sub opcodes 0x28/0x2a (with opcode_space
       SPACE_BASE) are 8-bit ops.  While they are relevant for SCFI
       correctness,  skip handling them here and use the x86_ginsn_unhandled
       code path to generate GINSN_TYPE_OTHER when necessary.  */

    case 0x1:  /* add reg, reg/mem.  */
    case 0x29: /* sub reg, reg/mem.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_addsub_reg_mem (insn_end_sym);
      break;

    case 0x3:  /* add reg/mem, reg.  */
    case 0x2b: /* sub reg/mem, reg.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_addsub_mem_reg (insn_end_sym);
      break;

    case 0xa0: /* push fs.  */
    case 0xa8: /* push gs.  */
      /* push fs / push gs have opcode_space == SPACE_0F.  */
      if (i.tm.opcode_space != SPACE_0F)
	break;
      dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn = ginsn_new_sub (insn_end_sym, false,
			     GINSN_SRC_REG, REG_SP, 0,
			     GINSN_SRC_IMM, 0, stack_opnd_size,
			     GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn);
      ginsn_next = ginsn_new_store (insn_end_sym, false,
				    GINSN_SRC_REG, dw2_regnum,
				    GINSN_DST_INDIRECT, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0xa1: /* pop fs.  */
    case 0xa9: /* pop gs.  */
      /* pop fs / pop gs have opcode_space == SPACE_0F.  */
      if (i.tm.opcode_space != SPACE_0F)
	break;
      dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn = ginsn_new_load (insn_end_sym, false,
			      GINSN_SRC_INDIRECT, REG_SP, 0,
			      GINSN_DST_REG, dw2_regnum);
      ginsn_set_where (ginsn);
      ginsn_next = ginsn_new_add (insn_end_sym, false,
				  GINSN_SRC_REG, REG_SP, 0,
				  GINSN_SRC_IMM, 0, stack_opnd_size,
				  GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0x50 ... 0x57:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* push reg.  */
      dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn = ginsn_new_sub (insn_end_sym, false,
			     GINSN_SRC_REG, REG_SP, 0,
			     GINSN_SRC_IMM, 0, stack_opnd_size,
			     GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn);
      ginsn_next = ginsn_new_store (insn_end_sym, false,
				    GINSN_SRC_REG, dw2_regnum,
				    GINSN_DST_INDIRECT, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0x58 ... 0x5f:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* pop reg.  */
      dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      ginsn = ginsn_new_load (insn_end_sym, false,
			      GINSN_SRC_INDIRECT, REG_SP, 0,
			      GINSN_DST_REG, dw2_regnum);
      ginsn_set_where (ginsn);
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn_next = ginsn_new_add (insn_end_sym, false,
				  GINSN_SRC_REG, REG_SP, 0,
				  GINSN_SRC_IMM, 0, stack_opnd_size,
				  GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0x6a: /* push imm8.  */
    case 0x68: /* push imm16/imm32.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      /* Skip getting the value of imm from machine instruction
	 because this is not important for SCFI.  */
      ginsn = ginsn_new_sub (insn_end_sym, false,
			     GINSN_SRC_REG, REG_SP, 0,
			     GINSN_SRC_IMM, 0, stack_opnd_size,
			     GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn);
      ginsn_next = ginsn_new_store (insn_end_sym, false,
				    GINSN_SRC_IMM, 0,
				    GINSN_DST_INDIRECT, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    /* PS: Opcodes 0x80 ... 0x8f with opcode_space SPACE_0F are present
       only after relaxation.  They do not need to be handled for ginsn
       creation.  */
    case 0x70 ... 0x7f:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_jump (insn_end_sym, true);
      break;

    case 0x80:
    case 0x81:
    case 0x83:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_alu_imm (insn_end_sym);
      break;

    case 0x8a: /* mov r/m8, r8.  */
    case 0x8b: /* mov r/m(16/32/64), r(16/32/64).  */
    case 0x88: /* mov r8, r/m8.  */
    case 0x89: /* mov r(16/32/64), r/m(16/32/64).  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_move (insn_end_sym);
      break;

    case 0x8d:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* lea disp(%base,%index,imm), %dst.  */
      ginsn = x86_ginsn_lea (insn_end_sym);
      break;

    case 0x8f:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* pop to reg/mem.  */
      if (i.mem_operands)
	{
	  mem_reg = (i.base_reg) ? i.base_reg : i.index_reg;
	  /* Use dummy register if no base or index.  Unlike other opcodes,
	     ginsns must be generated as this affect stack pointer.  */
	  dw2_regnum = (mem_reg
			? ginsn_dw2_regnum (mem_reg)
			: GINSN_DW2_REGNUM_RSI_DUMMY);
	}
      else
	dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
      ginsn = ginsn_new_load (insn_end_sym, false,
			      GINSN_SRC_INDIRECT, REG_SP, 0,
			      GINSN_DST_INDIRECT, dw2_regnum);
      ginsn_set_where (ginsn);
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn_next = ginsn_new_add (insn_end_sym, false,
				  GINSN_SRC_REG, REG_SP, 0,
				  GINSN_SRC_IMM, 0, stack_opnd_size,
				  GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0x9c:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* pushf / pushfq.  */
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      ginsn = ginsn_new_sub (insn_end_sym, false,
			     GINSN_SRC_REG, REG_SP, 0,
			     GINSN_SRC_IMM, 0, stack_opnd_size,
			     GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn);
      /* FIXME - hardcode the actual DWARF reg number value.  As for SCFI
	 correctness, although this behaves simply a placeholder value; its
	 just clearer if the value is correct.  */
      dw2_regnum = GINSN_DW2_REGNUM_EFLAGS;
      ginsn_next = ginsn_new_store (insn_end_sym, false,
				    GINSN_SRC_REG, dw2_regnum,
				    GINSN_DST_INDIRECT, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0x9d:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* popf / popfq.  */
      /* Check if operation size is 16-bit.  */
      if (ginsn_opsize_prefix_p ())
	stack_opnd_size = 2;
      /* FIXME - hardcode the actual DWARF reg number value.  As for SCFI
	 correctness, although this behaves simply a placeholder value; its
	 just clearer if the value is correct.  */
      dw2_regnum = GINSN_DW2_REGNUM_EFLAGS;
      ginsn = ginsn_new_load (insn_end_sym, false,
			      GINSN_SRC_INDIRECT, REG_SP, 0,
			      GINSN_DST_REG, dw2_regnum);
      ginsn_set_where (ginsn);
      ginsn_next = ginsn_new_add (insn_end_sym, false,
				  GINSN_SRC_REG, REG_SP, 0,
				  GINSN_SRC_IMM, 0, stack_opnd_size,
				  GINSN_DST_REG, REG_SP, 0);
      ginsn_set_where (ginsn_next);
      gas_assert (!ginsn_link_next (ginsn, ginsn_next));
      break;

    case 0xff:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* push from reg/mem.  */
      if (i.tm.extension_opcode == 6)
	{
	  /* Check if operation size is 16-bit.  */
	  if (ginsn_opsize_prefix_p ())
	    stack_opnd_size = 2;
	  ginsn = ginsn_new_sub (insn_end_sym, false,
				 GINSN_SRC_REG, REG_SP, 0,
				 GINSN_SRC_IMM, 0, stack_opnd_size,
				 GINSN_DST_REG, REG_SP, 0);
	  ginsn_set_where (ginsn);
	  if (i.mem_operands)
	    {
	      mem_reg = (i.base_reg) ? i.base_reg : i.index_reg;
	      /* Use dummy register if no base or index.  Unlike other opcodes,
		 ginsns must be generated as this affect stack pointer.  */
	      dw2_regnum = (mem_reg
			    ? ginsn_dw2_regnum (mem_reg)
			    : GINSN_DW2_REGNUM_RSI_DUMMY);
	    }
	  else
	    dw2_regnum = ginsn_dw2_regnum (i.op[0].regs);
	  ginsn_next = ginsn_new_store (insn_end_sym, false,
					GINSN_SRC_INDIRECT, dw2_regnum,
					GINSN_DST_INDIRECT, REG_SP, 0);
	  ginsn_set_where (ginsn_next);
	  gas_assert (!ginsn_link_next (ginsn, ginsn_next));
	}
      else if (i.tm.extension_opcode == 4 || i.tm.extension_opcode == 2)
	ginsn = x86_ginsn_indirect_branch (insn_end_sym);
      break;

    case 0xc2: /* ret imm16.  */
    case 0xc3: /* ret.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* Near ret.  */
      ginsn = ginsn_new_return (insn_end_sym, true);
      ginsn_set_where (ginsn);
      break;

    case 0xc8:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* enter.  */
      ginsn = x86_ginsn_enter (insn_end_sym);
      break;

    case 0xc9:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* leave.  */
      ginsn = x86_ginsn_leave (insn_end_sym);
      break;

    case 0xe0 ... 0xe2: /* loop / loope / loopne.  */
    case 0xe3:          /* jecxz / jrcxz.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      ginsn = x86_ginsn_jump (insn_end_sym, true);
      ginsn_set_where (ginsn);
      break;

    case 0xe8:
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* PS: SCFI machinery does not care about which func is being
	 called.  OK to skip that info.  */
      ginsn = ginsn_new_call (insn_end_sym, true,
			      GINSN_SRC_SYMBOL, 0, NULL);
      ginsn_set_where (ginsn);
      break;

    /* PS: opcode 0xe9 appears only after relaxation.  Skip here.  */
    case 0xeb:
      /* If opcode_space != SPACE_BASE, this is not a jmp insn.  Skip it
	 for GINSN_GEN_SCFI.  */
      if (i.tm.opcode_space != SPACE_BASE)
	break;
      /* Unconditional jmp.  */
      ginsn = x86_ginsn_jump (insn_end_sym, false);
      ginsn_set_where (ginsn);
      break;

    default:
      /* TBD_GINSN_GEN_NOT_SCFI: Skip all other opcodes uninteresting for
	 GINSN_GEN_SCFI mode.  */
      break;
    }

  if (!ginsn && !x86_ginsn_safe_to_skip_p ())
    {
      /* For all unhandled insns that are not whitelisted, check that they do
	 not impact SCFI correctness.  */
      err = x86_ginsn_unhandled ();
      switch (err)
	{
	case X86_GINSN_UNHANDLED_NONE:
	  break;
	case X86_GINSN_UNHANDLED_DEST_REG:
	  /* Not all writes to REG_FP are harmful in context of SCFI.  Simply
	     generate a GINSN_TYPE_OTHER with destination set to the
	     appropriate register.  The SCFI machinery will bail out if this
	     ginsn affects SCFI correctness.  */
	  dw2_regnum = ginsn_dw2_regnum (i.op[i.operands - 1].regs);
	  ginsn = ginsn_new_other (insn_end_sym, true,
				   GINSN_SRC_IMM, 0,
				   GINSN_SRC_IMM, 0,
				   GINSN_DST_REG, dw2_regnum);
	  ginsn_set_where (ginsn);
	  break;
	case X86_GINSN_UNHANDLED_CFG:
	case X86_GINSN_UNHANDLED_STACKOP:
	  as_bad (_("SCFI: unhandled op %#x may cause incorrect CFI"), opcode);
	  break;
	case X86_GINSN_UNHANDLED_UNEXPECTED:
	  as_bad (_("SCFI: unexpected op %#x may cause incorrect CFI"),
		  opcode);
	  break;
	default:
	  abort ();
	  break;
	}
    }

  return ginsn;
}
