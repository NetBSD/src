/* Print i386 instructions for GDB, the GNU debugger.
   Copyright (C) 1988-2015 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* 80386 instruction printer by Pace Willisson (pace@prep.ai.mit.edu)
   July 1988
    modified by John Hassey (hassey@dg-rtp.dg.com)
    x86-64 support added by Jan Hubicka (jh@suse.cz)
    VIA PadLock support by Michal Ludvig (mludvig@suse.cz).  */

/* The main tables describing the instructions is essentially a copy
   of the "Opcode Map" chapter (Appendix A) of the Intel 80386
   Programmers Manual.  Usually, there is a capital letter, followed
   by a small letter.  The capital letter tell the addressing mode,
   and the small letter tells about the operand size.  Refer to
   the Intel manual for details.  */

#include "sysdep.h"
#include "dis-asm.h"
#include "opintl.h"
#include "opcode/i386.h"
#include "libiberty.h"

#include <setjmp.h>

static int print_insn (bfd_vma, disassemble_info *);
static void dofloat (int);
static void OP_ST (int, int);
static void OP_STi (int, int);
static int putop (const char *, int);
static void oappend (const char *);
static void append_seg (void);
static void OP_indirE (int, int);
static void print_operand_value (char *, int, bfd_vma);
static void OP_E_register (int, int);
static void OP_E_memory (int, int);
static void print_displacement (char *, bfd_vma);
static void OP_E (int, int);
static void OP_G (int, int);
static bfd_vma get64 (void);
static bfd_signed_vma get32 (void);
static bfd_signed_vma get32s (void);
static int get16 (void);
static void set_op (bfd_vma, int);
static void OP_Skip_MODRM (int, int);
static void OP_REG (int, int);
static void OP_IMREG (int, int);
static void OP_I (int, int);
static void OP_I64 (int, int);
static void OP_sI (int, int);
static void OP_J (int, int);
static void OP_SEG (int, int);
static void OP_DIR (int, int);
static void OP_OFF (int, int);
static void OP_OFF64 (int, int);
static void ptr_reg (int, int);
static void OP_ESreg (int, int);
static void OP_DSreg (int, int);
static void OP_C (int, int);
static void OP_D (int, int);
static void OP_T (int, int);
static void OP_R (int, int);
static void OP_MMX (int, int);
static void OP_XMM (int, int);
static void OP_EM (int, int);
static void OP_EX (int, int);
static void OP_EMC (int,int);
static void OP_MXC (int,int);
static void OP_MS (int, int);
static void OP_XS (int, int);
static void OP_M (int, int);
static void OP_VEX (int, int);
static void OP_EX_Vex (int, int);
static void OP_EX_VexW (int, int);
static void OP_EX_VexImmW (int, int);
static void OP_XMM_Vex (int, int);
static void OP_XMM_VexW (int, int);
static void OP_Rounding (int, int);
static void OP_REG_VexI4 (int, int);
static void PCLMUL_Fixup (int, int);
static void VEXI4_Fixup (int, int);
static void VZERO_Fixup (int, int);
static void VCMP_Fixup (int, int);
static void VPCMP_Fixup (int, int);
static void OP_0f07 (int, int);
static void OP_Monitor (int, int);
static void OP_Mwait (int, int);
static void OP_Mwaitx (int, int);
static void NOP_Fixup1 (int, int);
static void NOP_Fixup2 (int, int);
static void OP_3DNowSuffix (int, int);
static void CMP_Fixup (int, int);
static void BadOp (void);
static void REP_Fixup (int, int);
static void BND_Fixup (int, int);
static void HLE_Fixup1 (int, int);
static void HLE_Fixup2 (int, int);
static void HLE_Fixup3 (int, int);
static void CMPXCHG8B_Fixup (int, int);
static void XMM_Fixup (int, int);
static void CRC32_Fixup (int, int);
static void FXSAVE_Fixup (int, int);
static void OP_LWPCB_E (int, int);
static void OP_LWP_E (int, int);
static void OP_Vex_2src_1 (int, int);
static void OP_Vex_2src_2 (int, int);

static void MOVBE_Fixup (int, int);

static void OP_Mask (int, int);

struct dis_private {
  /* Points to first byte not fetched.  */
  bfd_byte *max_fetched;
  bfd_byte the_buffer[MAX_MNEM_SIZE];
  bfd_vma insn_start;
  int orig_sizeflag;
  OPCODES_SIGJMP_BUF bailout;
};

enum address_mode
{
  mode_16bit,
  mode_32bit,
  mode_64bit
};

enum address_mode address_mode;

/* Flags for the prefixes for the current instruction.  See below.  */
static int prefixes;

/* REX prefix the current instruction.  See below.  */
static int rex;
/* Bits of REX we've already used.  */
static int rex_used;
/* REX bits in original REX prefix ignored.  */
static int rex_ignored;
/* Mark parts used in the REX prefix.  When we are testing for
   empty prefix (for 8bit register REX extension), just mask it
   out.  Otherwise test for REX bit is excuse for existence of REX
   only in case value is nonzero.  */
#define USED_REX(value)					\
  {							\
    if (value)						\
      {							\
	if ((rex & value))				\
	  rex_used |= (value) | REX_OPCODE;		\
      }							\
    else						\
      rex_used |= REX_OPCODE;				\
  }

/* Flags for prefixes which we somehow handled when printing the
   current instruction.  */
static int used_prefixes;

/* Flags stored in PREFIXES.  */
#define PREFIX_REPZ 1
#define PREFIX_REPNZ 2
#define PREFIX_LOCK 4
#define PREFIX_CS 8
#define PREFIX_SS 0x10
#define PREFIX_DS 0x20
#define PREFIX_ES 0x40
#define PREFIX_FS 0x80
#define PREFIX_GS 0x100
#define PREFIX_DATA 0x200
#define PREFIX_ADDR 0x400
#define PREFIX_FWAIT 0x800

/* Make sure that bytes from INFO->PRIVATE_DATA->BUFFER (inclusive)
   to ADDR (exclusive) are valid.  Returns 1 for success, longjmps
   on error.  */
#define FETCH_DATA(info, addr) \
  ((addr) <= ((struct dis_private *) (info->private_data))->max_fetched \
   ? 1 : fetch_data ((info), (addr)))

static int
fetch_data (struct disassemble_info *info, bfd_byte *addr)
{
  int status;
  struct dis_private *priv = (struct dis_private *) info->private_data;
  bfd_vma start = priv->insn_start + (priv->max_fetched - priv->the_buffer);

  if (addr <= priv->the_buffer + MAX_MNEM_SIZE)
    status = (*info->read_memory_func) (start,
					priv->max_fetched,
					addr - priv->max_fetched,
					info);
  else
    status = -1;
  if (status != 0)
    {
      /* If we did manage to read at least one byte, then
	 print_insn_i386 will do something sensible.  Otherwise, print
	 an error.  We do that here because this is where we know
	 STATUS.  */
      if (priv->max_fetched == priv->the_buffer)
	(*info->memory_error_func) (status, start, info);
      OPCODES_SIGLONGJMP (priv->bailout, 1);
    }
  else
    priv->max_fetched = addr;
  return 1;
}

/* Possible values for prefix requirement.  */
#define PREFIX_IGNORED_SHIFT	16
#define PREFIX_IGNORED_REPZ	(PREFIX_REPZ << PREFIX_IGNORED_SHIFT)
#define PREFIX_IGNORED_REPNZ	(PREFIX_REPNZ << PREFIX_IGNORED_SHIFT)
#define PREFIX_IGNORED_DATA	(PREFIX_DATA << PREFIX_IGNORED_SHIFT)
#define PREFIX_IGNORED_ADDR	(PREFIX_ADDR << PREFIX_IGNORED_SHIFT)
#define PREFIX_IGNORED_LOCK	(PREFIX_LOCK << PREFIX_IGNORED_SHIFT)

/* Opcode prefixes.  */
#define PREFIX_OPCODE		(PREFIX_REPZ \
				 | PREFIX_REPNZ \
				 | PREFIX_DATA)

/* Prefixes ignored.  */
#define PREFIX_IGNORED		(PREFIX_IGNORED_REPZ \
				 | PREFIX_IGNORED_REPNZ \
				 | PREFIX_IGNORED_DATA)

#define XX { NULL, 0 }
#define Bad_Opcode NULL, { { NULL, 0 } }, 0

#define Eb { OP_E, b_mode }
#define Ebnd { OP_E, bnd_mode }
#define EbS { OP_E, b_swap_mode }
#define Ev { OP_E, v_mode }
#define Ev_bnd { OP_E, v_bnd_mode }
#define EvS { OP_E, v_swap_mode }
#define Ed { OP_E, d_mode }
#define Edq { OP_E, dq_mode }
#define Edqw { OP_E, dqw_mode }
#define EdqwS { OP_E, dqw_swap_mode }
#define Edqb { OP_E, dqb_mode }
#define Edb { OP_E, db_mode }
#define Edw { OP_E, dw_mode }
#define Edqd { OP_E, dqd_mode }
#define Eq { OP_E, q_mode }
#define indirEv { OP_indirE, stack_v_mode }
#define indirEp { OP_indirE, f_mode }
#define stackEv { OP_E, stack_v_mode }
#define Em { OP_E, m_mode }
#define Ew { OP_E, w_mode }
#define M { OP_M, 0 }		/* lea, lgdt, etc. */
#define Ma { OP_M, a_mode }
#define Mb { OP_M, b_mode }
#define Md { OP_M, d_mode }
#define Mo { OP_M, o_mode }
#define Mp { OP_M, f_mode }		/* 32 or 48 bit memory operand for LDS, LES etc */
#define Mq { OP_M, q_mode }
#define Mx { OP_M, x_mode }
#define Mxmm { OP_M, xmm_mode }
#define Gb { OP_G, b_mode }
#define Gbnd { OP_G, bnd_mode }
#define Gv { OP_G, v_mode }
#define Gd { OP_G, d_mode }
#define Gdq { OP_G, dq_mode }
#define Gm { OP_G, m_mode }
#define Gw { OP_G, w_mode }
#define Rd { OP_R, d_mode }
#define Rdq { OP_R, dq_mode }
#define Rm { OP_R, m_mode }
#define Ib { OP_I, b_mode }
#define sIb { OP_sI, b_mode }	/* sign extened byte */
#define sIbT { OP_sI, b_T_mode } /* sign extened byte like 'T' */
#define Iv { OP_I, v_mode }
#define sIv { OP_sI, v_mode }
#define Iq { OP_I, q_mode }
#define Iv64 { OP_I64, v_mode }
#define Iw { OP_I, w_mode }
#define I1 { OP_I, const_1_mode }
#define Jb { OP_J, b_mode }
#define Jv { OP_J, v_mode }
#define Cm { OP_C, m_mode }
#define Dm { OP_D, m_mode }
#define Td { OP_T, d_mode }
#define Skip_MODRM { OP_Skip_MODRM, 0 }

#define RMeAX { OP_REG, eAX_reg }
#define RMeBX { OP_REG, eBX_reg }
#define RMeCX { OP_REG, eCX_reg }
#define RMeDX { OP_REG, eDX_reg }
#define RMeSP { OP_REG, eSP_reg }
#define RMeBP { OP_REG, eBP_reg }
#define RMeSI { OP_REG, eSI_reg }
#define RMeDI { OP_REG, eDI_reg }
#define RMrAX { OP_REG, rAX_reg }
#define RMrBX { OP_REG, rBX_reg }
#define RMrCX { OP_REG, rCX_reg }
#define RMrDX { OP_REG, rDX_reg }
#define RMrSP { OP_REG, rSP_reg }
#define RMrBP { OP_REG, rBP_reg }
#define RMrSI { OP_REG, rSI_reg }
#define RMrDI { OP_REG, rDI_reg }
#define RMAL { OP_REG, al_reg }
#define RMCL { OP_REG, cl_reg }
#define RMDL { OP_REG, dl_reg }
#define RMBL { OP_REG, bl_reg }
#define RMAH { OP_REG, ah_reg }
#define RMCH { OP_REG, ch_reg }
#define RMDH { OP_REG, dh_reg }
#define RMBH { OP_REG, bh_reg }
#define RMAX { OP_REG, ax_reg }
#define RMDX { OP_REG, dx_reg }

#define eAX { OP_IMREG, eAX_reg }
#define eBX { OP_IMREG, eBX_reg }
#define eCX { OP_IMREG, eCX_reg }
#define eDX { OP_IMREG, eDX_reg }
#define eSP { OP_IMREG, eSP_reg }
#define eBP { OP_IMREG, eBP_reg }
#define eSI { OP_IMREG, eSI_reg }
#define eDI { OP_IMREG, eDI_reg }
#define AL { OP_IMREG, al_reg }
#define CL { OP_IMREG, cl_reg }
#define DL { OP_IMREG, dl_reg }
#define BL { OP_IMREG, bl_reg }
#define AH { OP_IMREG, ah_reg }
#define CH { OP_IMREG, ch_reg }
#define DH { OP_IMREG, dh_reg }
#define BH { OP_IMREG, bh_reg }
#define AX { OP_IMREG, ax_reg }
#define DX { OP_IMREG, dx_reg }
#define zAX { OP_IMREG, z_mode_ax_reg }
#define indirDX { OP_IMREG, indir_dx_reg }

#define Sw { OP_SEG, w_mode }
#define Sv { OP_SEG, v_mode }
#define Ap { OP_DIR, 0 }
#define Ob { OP_OFF64, b_mode }
#define Ov { OP_OFF64, v_mode }
#define Xb { OP_DSreg, eSI_reg }
#define Xv { OP_DSreg, eSI_reg }
#define Xz { OP_DSreg, eSI_reg }
#define Yb { OP_ESreg, eDI_reg }
#define Yv { OP_ESreg, eDI_reg }
#define DSBX { OP_DSreg, eBX_reg }

#define es { OP_REG, es_reg }
#define ss { OP_REG, ss_reg }
#define cs { OP_REG, cs_reg }
#define ds { OP_REG, ds_reg }
#define fs { OP_REG, fs_reg }
#define gs { OP_REG, gs_reg }

#define MX { OP_MMX, 0 }
#define XM { OP_XMM, 0 }
#define XMScalar { OP_XMM, scalar_mode }
#define XMGatherQ { OP_XMM, vex_vsib_q_w_dq_mode }
#define XMM { OP_XMM, xmm_mode }
#define XMxmmq { OP_XMM, xmmq_mode }
#define EM { OP_EM, v_mode }
#define EMS { OP_EM, v_swap_mode }
#define EMd { OP_EM, d_mode }
#define EMx { OP_EM, x_mode }
#define EXw { OP_EX, w_mode }
#define EXd { OP_EX, d_mode }
#define EXdScalar { OP_EX, d_scalar_mode }
#define EXdS { OP_EX, d_swap_mode }
#define EXdScalarS { OP_EX, d_scalar_swap_mode }
#define EXq { OP_EX, q_mode }
#define EXqScalar { OP_EX, q_scalar_mode }
#define EXqScalarS { OP_EX, q_scalar_swap_mode }
#define EXqS { OP_EX, q_swap_mode }
#define EXx { OP_EX, x_mode }
#define EXxS { OP_EX, x_swap_mode }
#define EXxmm { OP_EX, xmm_mode }
#define EXymm { OP_EX, ymm_mode }
#define EXxmmq { OP_EX, xmmq_mode }
#define EXEvexHalfBcstXmmq { OP_EX, evex_half_bcst_xmmq_mode }
#define EXxmm_mb { OP_EX, xmm_mb_mode }
#define EXxmm_mw { OP_EX, xmm_mw_mode }
#define EXxmm_md { OP_EX, xmm_md_mode }
#define EXxmm_mq { OP_EX, xmm_mq_mode }
#define EXxmm_mdq { OP_EX, xmm_mdq_mode }
#define EXxmmdw { OP_EX, xmmdw_mode }
#define EXxmmqd { OP_EX, xmmqd_mode }
#define EXymmq { OP_EX, ymmq_mode }
#define EXVexWdq { OP_EX, vex_w_dq_mode }
#define EXVexWdqScalar { OP_EX, vex_scalar_w_dq_mode }
#define EXEvexXGscat { OP_EX, evex_x_gscat_mode }
#define EXEvexXNoBcst { OP_EX, evex_x_nobcst_mode }
#define MS { OP_MS, v_mode }
#define XS { OP_XS, v_mode }
#define EMCq { OP_EMC, q_mode }
#define MXC { OP_MXC, 0 }
#define OPSUF { OP_3DNowSuffix, 0 }
#define CMP { CMP_Fixup, 0 }
#define XMM0 { XMM_Fixup, 0 }
#define FXSAVE { FXSAVE_Fixup, 0 }
#define Vex_2src_1 { OP_Vex_2src_1, 0 }
#define Vex_2src_2 { OP_Vex_2src_2, 0 }

#define Vex { OP_VEX, vex_mode }
#define VexScalar { OP_VEX, vex_scalar_mode }
#define VexGatherQ { OP_VEX, vex_vsib_q_w_dq_mode }
#define Vex128 { OP_VEX, vex128_mode }
#define Vex256 { OP_VEX, vex256_mode }
#define VexGdq { OP_VEX, dq_mode }
#define VexI4 { VEXI4_Fixup, 0}
#define EXdVex { OP_EX_Vex, d_mode }
#define EXdVexS { OP_EX_Vex, d_swap_mode }
#define EXdVexScalarS { OP_EX_Vex, d_scalar_swap_mode }
#define EXqVex { OP_EX_Vex, q_mode }
#define EXqVexS { OP_EX_Vex, q_swap_mode }
#define EXqVexScalarS { OP_EX_Vex, q_scalar_swap_mode }
#define EXVexW { OP_EX_VexW, x_mode }
#define EXdVexW { OP_EX_VexW, d_mode }
#define EXqVexW { OP_EX_VexW, q_mode }
#define EXVexImmW { OP_EX_VexImmW, x_mode }
#define XMVex { OP_XMM_Vex, 0 }
#define XMVexScalar { OP_XMM_Vex, scalar_mode }
#define XMVexW { OP_XMM_VexW, 0 }
#define XMVexI4 { OP_REG_VexI4, x_mode }
#define PCLMUL { PCLMUL_Fixup, 0 }
#define VZERO { VZERO_Fixup, 0 }
#define VCMP { VCMP_Fixup, 0 }
#define VPCMP { VPCMP_Fixup, 0 }

#define EXxEVexR { OP_Rounding, evex_rounding_mode }
#define EXxEVexS { OP_Rounding, evex_sae_mode }

#define XMask { OP_Mask, mask_mode }
#define MaskG { OP_G, mask_mode }
#define MaskE { OP_E, mask_mode }
#define MaskBDE { OP_E, mask_bd_mode }
#define MaskR { OP_R, mask_mode }
#define MaskVex { OP_VEX, mask_mode }

#define MVexVSIBDWpX { OP_M, vex_vsib_d_w_dq_mode }
#define MVexVSIBDQWpX { OP_M, vex_vsib_d_w_d_mode }
#define MVexVSIBQWpX { OP_M, vex_vsib_q_w_dq_mode }
#define MVexVSIBQDWpX { OP_M, vex_vsib_q_w_d_mode }

/* Used handle "rep" prefix for string instructions.  */
#define Xbr { REP_Fixup, eSI_reg }
#define Xvr { REP_Fixup, eSI_reg }
#define Ybr { REP_Fixup, eDI_reg }
#define Yvr { REP_Fixup, eDI_reg }
#define Yzr { REP_Fixup, eDI_reg }
#define indirDXr { REP_Fixup, indir_dx_reg }
#define ALr { REP_Fixup, al_reg }
#define eAXr { REP_Fixup, eAX_reg }

/* Used handle HLE prefix for lockable instructions.  */
#define Ebh1 { HLE_Fixup1, b_mode }
#define Evh1 { HLE_Fixup1, v_mode }
#define Ebh2 { HLE_Fixup2, b_mode }
#define Evh2 { HLE_Fixup2, v_mode }
#define Ebh3 { HLE_Fixup3, b_mode }
#define Evh3 { HLE_Fixup3, v_mode }

#define BND { BND_Fixup, 0 }

#define cond_jump_flag { NULL, cond_jump_mode }
#define loop_jcxz_flag { NULL, loop_jcxz_mode }

/* bits in sizeflag */
#define SUFFIX_ALWAYS 4
#define AFLAG 2
#define DFLAG 1

enum
{
  /* byte operand */
  b_mode = 1,
  /* byte operand with operand swapped */
  b_swap_mode,
  /* byte operand, sign extend like 'T' suffix */
  b_T_mode,
  /* operand size depends on prefixes */
  v_mode,
  /* operand size depends on prefixes with operand swapped */
  v_swap_mode,
  /* word operand */
  w_mode,
  /* double word operand  */
  d_mode,
  /* double word operand with operand swapped */
  d_swap_mode,
  /* quad word operand */
  q_mode,
  /* quad word operand with operand swapped */
  q_swap_mode,
  /* ten-byte operand */
  t_mode,
  /* 16-byte XMM, 32-byte YMM or 64-byte ZMM operand.  In EVEX with
     broadcast enabled.  */
  x_mode,
  /* Similar to x_mode, but with different EVEX mem shifts.  */
  evex_x_gscat_mode,
  /* Similar to x_mode, but with disabled broadcast.  */
  evex_x_nobcst_mode,
  /* Similar to x_mode, but with operands swapped and disabled broadcast
     in EVEX.  */
  x_swap_mode,
  /* 16-byte XMM operand */
  xmm_mode,
  /* XMM, XMM or YMM register operand, or quad word, xmmword or ymmword
     memory operand (depending on vector length).  Broadcast isn't
     allowed.  */
  xmmq_mode,
  /* Same as xmmq_mode, but broadcast is allowed.  */
  evex_half_bcst_xmmq_mode,
  /* XMM register or byte memory operand */
  xmm_mb_mode,
  /* XMM register or word memory operand */
  xmm_mw_mode,
  /* XMM register or double word memory operand */
  xmm_md_mode,
  /* XMM register or quad word memory operand */
  xmm_mq_mode,
  /* XMM register or double/quad word memory operand, depending on
     VEX.W.  */
  xmm_mdq_mode,
  /* 16-byte XMM, word, double word or quad word operand.  */
  xmmdw_mode,
  /* 16-byte XMM, double word, quad word operand or xmm word operand.  */
  xmmqd_mode,
  /* 32-byte YMM operand */
  ymm_mode,
  /* quad word, ymmword or zmmword memory operand.  */
  ymmq_mode,
  /* 32-byte YMM or 16-byte word operand */
  ymmxmm_mode,
  /* d_mode in 32bit, q_mode in 64bit mode.  */
  m_mode,
  /* pair of v_mode operands */
  a_mode,
  cond_jump_mode,
  loop_jcxz_mode,
  v_bnd_mode,
  /* operand size depends on REX prefixes.  */
  dq_mode,
  /* registers like dq_mode, memory like w_mode.  */
  dqw_mode,
  dqw_swap_mode,
  bnd_mode,
  /* 4- or 6-byte pointer operand */
  f_mode,
  const_1_mode,
  /* v_mode for stack-related opcodes.  */
  stack_v_mode,
  /* non-quad operand size depends on prefixes */
  z_mode,
  /* 16-byte operand */
  o_mode,
  /* registers like dq_mode, memory like b_mode.  */
  dqb_mode,
  /* registers like d_mode, memory like b_mode.  */
  db_mode,
  /* registers like d_mode, memory like w_mode.  */
  dw_mode,
  /* registers like dq_mode, memory like d_mode.  */
  dqd_mode,
  /* normal vex mode */
  vex_mode,
  /* 128bit vex mode */
  vex128_mode,
  /* 256bit vex mode */
  vex256_mode,
  /* operand size depends on the VEX.W bit.  */
  vex_w_dq_mode,

  /* Similar to vex_w_dq_mode, with VSIB dword indices.  */
  vex_vsib_d_w_dq_mode,
  /* Similar to vex_vsib_d_w_dq_mode, with smaller memory.  */
  vex_vsib_d_w_d_mode,
  /* Similar to vex_w_dq_mode, with VSIB qword indices.  */
  vex_vsib_q_w_dq_mode,
  /* Similar to vex_vsib_q_w_dq_mode, with smaller memory.  */
  vex_vsib_q_w_d_mode,

  /* scalar, ignore vector length.  */
  scalar_mode,
  /* like d_mode, ignore vector length.  */
  d_scalar_mode,
  /* like d_swap_mode, ignore vector length.  */
  d_scalar_swap_mode,
  /* like q_mode, ignore vector length.  */
  q_scalar_mode,
  /* like q_swap_mode, ignore vector length.  */
  q_scalar_swap_mode,
  /* like vex_mode, ignore vector length.  */
  vex_scalar_mode,
  /* like vex_w_dq_mode, ignore vector length.  */
  vex_scalar_w_dq_mode,

  /* Static rounding.  */
  evex_rounding_mode,
  /* Supress all exceptions.  */
  evex_sae_mode,

  /* Mask register operand.  */
  mask_mode,
  /* Mask register operand.  */
  mask_bd_mode,

  es_reg,
  cs_reg,
  ss_reg,
  ds_reg,
  fs_reg,
  gs_reg,

  eAX_reg,
  eCX_reg,
  eDX_reg,
  eBX_reg,
  eSP_reg,
  eBP_reg,
  eSI_reg,
  eDI_reg,

  al_reg,
  cl_reg,
  dl_reg,
  bl_reg,
  ah_reg,
  ch_reg,
  dh_reg,
  bh_reg,

  ax_reg,
  cx_reg,
  dx_reg,
  bx_reg,
  sp_reg,
  bp_reg,
  si_reg,
  di_reg,

  rAX_reg,
  rCX_reg,
  rDX_reg,
  rBX_reg,
  rSP_reg,
  rBP_reg,
  rSI_reg,
  rDI_reg,

  z_mode_ax_reg,
  indir_dx_reg
};

enum
{
  FLOATCODE = 1,
  USE_REG_TABLE,
  USE_MOD_TABLE,
  USE_RM_TABLE,
  USE_PREFIX_TABLE,
  USE_X86_64_TABLE,
  USE_3BYTE_TABLE,
  USE_XOP_8F_TABLE,
  USE_VEX_C4_TABLE,
  USE_VEX_C5_TABLE,
  USE_VEX_LEN_TABLE,
  USE_VEX_W_TABLE,
  USE_EVEX_TABLE
};

#define FLOAT			NULL, { { NULL, FLOATCODE } }, 0

#define DIS386(T, I)		NULL, { { NULL, (T)}, { NULL,  (I) } }, 0
#define DIS386_PREFIX(T, I, P)		NULL, { { NULL, (T)}, { NULL,  (I) } }, P
#define REG_TABLE(I)		DIS386 (USE_REG_TABLE, (I))
#define MOD_TABLE(I)		DIS386 (USE_MOD_TABLE, (I))
#define RM_TABLE(I)		DIS386 (USE_RM_TABLE, (I))
#define PREFIX_TABLE(I)		DIS386 (USE_PREFIX_TABLE, (I))
#define X86_64_TABLE(I)		DIS386 (USE_X86_64_TABLE, (I))
#define THREE_BYTE_TABLE(I)	DIS386 (USE_3BYTE_TABLE, (I))
#define THREE_BYTE_TABLE_PREFIX(I, P)	DIS386_PREFIX (USE_3BYTE_TABLE, (I), P)
#define XOP_8F_TABLE(I)		DIS386 (USE_XOP_8F_TABLE, (I))
#define VEX_C4_TABLE(I)		DIS386 (USE_VEX_C4_TABLE, (I))
#define VEX_C5_TABLE(I)		DIS386 (USE_VEX_C5_TABLE, (I))
#define VEX_LEN_TABLE(I)	DIS386 (USE_VEX_LEN_TABLE, (I))
#define VEX_W_TABLE(I)		DIS386 (USE_VEX_W_TABLE, (I))
#define EVEX_TABLE(I)		DIS386 (USE_EVEX_TABLE, (I))

enum
{
  REG_80 = 0,
  REG_81,
  REG_82,
  REG_8F,
  REG_C0,
  REG_C1,
  REG_C6,
  REG_C7,
  REG_D0,
  REG_D1,
  REG_D2,
  REG_D3,
  REG_F6,
  REG_F7,
  REG_FE,
  REG_FF,
  REG_0F00,
  REG_0F01,
  REG_0F0D,
  REG_0F18,
  REG_0F71,
  REG_0F72,
  REG_0F73,
  REG_0FA6,
  REG_0FA7,
  REG_0FAE,
  REG_0FBA,
  REG_0FC7,
  REG_VEX_0F71,
  REG_VEX_0F72,
  REG_VEX_0F73,
  REG_VEX_0FAE,
  REG_VEX_0F38F3,
  REG_XOP_LWPCB,
  REG_XOP_LWP,
  REG_XOP_TBM_01,
  REG_XOP_TBM_02,

  REG_EVEX_0F71,
  REG_EVEX_0F72,
  REG_EVEX_0F73,
  REG_EVEX_0F38C6,
  REG_EVEX_0F38C7
};

enum
{
  MOD_8D = 0,
  MOD_C6_REG_7,
  MOD_C7_REG_7,
  MOD_FF_REG_3,
  MOD_FF_REG_5,
  MOD_0F01_REG_0,
  MOD_0F01_REG_1,
  MOD_0F01_REG_2,
  MOD_0F01_REG_3,
  MOD_0F01_REG_7,
  MOD_0F12_PREFIX_0,
  MOD_0F13,
  MOD_0F16_PREFIX_0,
  MOD_0F17,
  MOD_0F18_REG_0,
  MOD_0F18_REG_1,
  MOD_0F18_REG_2,
  MOD_0F18_REG_3,
  MOD_0F18_REG_4,
  MOD_0F18_REG_5,
  MOD_0F18_REG_6,
  MOD_0F18_REG_7,
  MOD_0F1A_PREFIX_0,
  MOD_0F1B_PREFIX_0,
  MOD_0F1B_PREFIX_1,
  MOD_0F24,
  MOD_0F26,
  MOD_0F2B_PREFIX_0,
  MOD_0F2B_PREFIX_1,
  MOD_0F2B_PREFIX_2,
  MOD_0F2B_PREFIX_3,
  MOD_0F51,
  MOD_0F71_REG_2,
  MOD_0F71_REG_4,
  MOD_0F71_REG_6,
  MOD_0F72_REG_2,
  MOD_0F72_REG_4,
  MOD_0F72_REG_6,
  MOD_0F73_REG_2,
  MOD_0F73_REG_3,
  MOD_0F73_REG_6,
  MOD_0F73_REG_7,
  MOD_0FAE_REG_0,
  MOD_0FAE_REG_1,
  MOD_0FAE_REG_2,
  MOD_0FAE_REG_3,
  MOD_0FAE_REG_4,
  MOD_0FAE_REG_5,
  MOD_0FAE_REG_6,
  MOD_0FAE_REG_7,
  MOD_0FB2,
  MOD_0FB4,
  MOD_0FB5,
  MOD_0FC7_REG_3,
  MOD_0FC7_REG_4,
  MOD_0FC7_REG_5,
  MOD_0FC7_REG_6,
  MOD_0FC7_REG_7,
  MOD_0FD7,
  MOD_0FE7_PREFIX_2,
  MOD_0FF0_PREFIX_3,
  MOD_0F382A_PREFIX_2,
  MOD_62_32BIT,
  MOD_C4_32BIT,
  MOD_C5_32BIT,
  MOD_VEX_0F12_PREFIX_0,
  MOD_VEX_0F13,
  MOD_VEX_0F16_PREFIX_0,
  MOD_VEX_0F17,
  MOD_VEX_0F2B,
  MOD_VEX_0F50,
  MOD_VEX_0F71_REG_2,
  MOD_VEX_0F71_REG_4,
  MOD_VEX_0F71_REG_6,
  MOD_VEX_0F72_REG_2,
  MOD_VEX_0F72_REG_4,
  MOD_VEX_0F72_REG_6,
  MOD_VEX_0F73_REG_2,
  MOD_VEX_0F73_REG_3,
  MOD_VEX_0F73_REG_6,
  MOD_VEX_0F73_REG_7,
  MOD_VEX_0FAE_REG_2,
  MOD_VEX_0FAE_REG_3,
  MOD_VEX_0FD7_PREFIX_2,
  MOD_VEX_0FE7_PREFIX_2,
  MOD_VEX_0FF0_PREFIX_3,
  MOD_VEX_0F381A_PREFIX_2,
  MOD_VEX_0F382A_PREFIX_2,
  MOD_VEX_0F382C_PREFIX_2,
  MOD_VEX_0F382D_PREFIX_2,
  MOD_VEX_0F382E_PREFIX_2,
  MOD_VEX_0F382F_PREFIX_2,
  MOD_VEX_0F385A_PREFIX_2,
  MOD_VEX_0F388C_PREFIX_2,
  MOD_VEX_0F388E_PREFIX_2,

  MOD_EVEX_0F10_PREFIX_1,
  MOD_EVEX_0F10_PREFIX_3,
  MOD_EVEX_0F11_PREFIX_1,
  MOD_EVEX_0F11_PREFIX_3,
  MOD_EVEX_0F12_PREFIX_0,
  MOD_EVEX_0F16_PREFIX_0,
  MOD_EVEX_0F38C6_REG_1,
  MOD_EVEX_0F38C6_REG_2,
  MOD_EVEX_0F38C6_REG_5,
  MOD_EVEX_0F38C6_REG_6,
  MOD_EVEX_0F38C7_REG_1,
  MOD_EVEX_0F38C7_REG_2,
  MOD_EVEX_0F38C7_REG_5,
  MOD_EVEX_0F38C7_REG_6
};

enum
{
  RM_C6_REG_7 = 0,
  RM_C7_REG_7,
  RM_0F01_REG_0,
  RM_0F01_REG_1,
  RM_0F01_REG_2,
  RM_0F01_REG_3,
  RM_0F01_REG_7,
  RM_0FAE_REG_5,
  RM_0FAE_REG_6,
  RM_0FAE_REG_7
};

enum
{
  PREFIX_90 = 0,
  PREFIX_0F10,
  PREFIX_0F11,
  PREFIX_0F12,
  PREFIX_0F16,
  PREFIX_0F1A,
  PREFIX_0F1B,
  PREFIX_0F2A,
  PREFIX_0F2B,
  PREFIX_0F2C,
  PREFIX_0F2D,
  PREFIX_0F2E,
  PREFIX_0F2F,
  PREFIX_0F51,
  PREFIX_0F52,
  PREFIX_0F53,
  PREFIX_0F58,
  PREFIX_0F59,
  PREFIX_0F5A,
  PREFIX_0F5B,
  PREFIX_0F5C,
  PREFIX_0F5D,
  PREFIX_0F5E,
  PREFIX_0F5F,
  PREFIX_0F60,
  PREFIX_0F61,
  PREFIX_0F62,
  PREFIX_0F6C,
  PREFIX_0F6D,
  PREFIX_0F6F,
  PREFIX_0F70,
  PREFIX_0F73_REG_3,
  PREFIX_0F73_REG_7,
  PREFIX_0F78,
  PREFIX_0F79,
  PREFIX_0F7C,
  PREFIX_0F7D,
  PREFIX_0F7E,
  PREFIX_0F7F,
  PREFIX_0FAE_REG_0,
  PREFIX_0FAE_REG_1,
  PREFIX_0FAE_REG_2,
  PREFIX_0FAE_REG_3,
  PREFIX_0FAE_REG_6,
  PREFIX_0FAE_REG_7,
  PREFIX_RM_0_0FAE_REG_7,
  PREFIX_0FB8,
  PREFIX_0FBC,
  PREFIX_0FBD,
  PREFIX_0FC2,
  PREFIX_0FC3,
  PREFIX_MOD_0_0FC7_REG_6,
  PREFIX_MOD_3_0FC7_REG_6,
  PREFIX_MOD_3_0FC7_REG_7,
  PREFIX_0FD0,
  PREFIX_0FD6,
  PREFIX_0FE6,
  PREFIX_0FE7,
  PREFIX_0FF0,
  PREFIX_0FF7,
  PREFIX_0F3810,
  PREFIX_0F3814,
  PREFIX_0F3815,
  PREFIX_0F3817,
  PREFIX_0F3820,
  PREFIX_0F3821,
  PREFIX_0F3822,
  PREFIX_0F3823,
  PREFIX_0F3824,
  PREFIX_0F3825,
  PREFIX_0F3828,
  PREFIX_0F3829,
  PREFIX_0F382A,
  PREFIX_0F382B,
  PREFIX_0F3830,
  PREFIX_0F3831,
  PREFIX_0F3832,
  PREFIX_0F3833,
  PREFIX_0F3834,
  PREFIX_0F3835,
  PREFIX_0F3837,
  PREFIX_0F3838,
  PREFIX_0F3839,
  PREFIX_0F383A,
  PREFIX_0F383B,
  PREFIX_0F383C,
  PREFIX_0F383D,
  PREFIX_0F383E,
  PREFIX_0F383F,
  PREFIX_0F3840,
  PREFIX_0F3841,
  PREFIX_0F3880,
  PREFIX_0F3881,
  PREFIX_0F3882,
  PREFIX_0F38C8,
  PREFIX_0F38C9,
  PREFIX_0F38CA,
  PREFIX_0F38CB,
  PREFIX_0F38CC,
  PREFIX_0F38CD,
  PREFIX_0F38DB,
  PREFIX_0F38DC,
  PREFIX_0F38DD,
  PREFIX_0F38DE,
  PREFIX_0F38DF,
  PREFIX_0F38F0,
  PREFIX_0F38F1,
  PREFIX_0F38F6,
  PREFIX_0F3A08,
  PREFIX_0F3A09,
  PREFIX_0F3A0A,
  PREFIX_0F3A0B,
  PREFIX_0F3A0C,
  PREFIX_0F3A0D,
  PREFIX_0F3A0E,
  PREFIX_0F3A14,
  PREFIX_0F3A15,
  PREFIX_0F3A16,
  PREFIX_0F3A17,
  PREFIX_0F3A20,
  PREFIX_0F3A21,
  PREFIX_0F3A22,
  PREFIX_0F3A40,
  PREFIX_0F3A41,
  PREFIX_0F3A42,
  PREFIX_0F3A44,
  PREFIX_0F3A60,
  PREFIX_0F3A61,
  PREFIX_0F3A62,
  PREFIX_0F3A63,
  PREFIX_0F3ACC,
  PREFIX_0F3ADF,
  PREFIX_VEX_0F10,
  PREFIX_VEX_0F11,
  PREFIX_VEX_0F12,
  PREFIX_VEX_0F16,
  PREFIX_VEX_0F2A,
  PREFIX_VEX_0F2C,
  PREFIX_VEX_0F2D,
  PREFIX_VEX_0F2E,
  PREFIX_VEX_0F2F,
  PREFIX_VEX_0F41,
  PREFIX_VEX_0F42,
  PREFIX_VEX_0F44,
  PREFIX_VEX_0F45,
  PREFIX_VEX_0F46,
  PREFIX_VEX_0F47,
  PREFIX_VEX_0F4A,
  PREFIX_VEX_0F4B,
  PREFIX_VEX_0F51,
  PREFIX_VEX_0F52,
  PREFIX_VEX_0F53,
  PREFIX_VEX_0F58,
  PREFIX_VEX_0F59,
  PREFIX_VEX_0F5A,
  PREFIX_VEX_0F5B,
  PREFIX_VEX_0F5C,
  PREFIX_VEX_0F5D,
  PREFIX_VEX_0F5E,
  PREFIX_VEX_0F5F,
  PREFIX_VEX_0F60,
  PREFIX_VEX_0F61,
  PREFIX_VEX_0F62,
  PREFIX_VEX_0F63,
  PREFIX_VEX_0F64,
  PREFIX_VEX_0F65,
  PREFIX_VEX_0F66,
  PREFIX_VEX_0F67,
  PREFIX_VEX_0F68,
  PREFIX_VEX_0F69,
  PREFIX_VEX_0F6A,
  PREFIX_VEX_0F6B,
  PREFIX_VEX_0F6C,
  PREFIX_VEX_0F6D,
  PREFIX_VEX_0F6E,
  PREFIX_VEX_0F6F,
  PREFIX_VEX_0F70,
  PREFIX_VEX_0F71_REG_2,
  PREFIX_VEX_0F71_REG_4,
  PREFIX_VEX_0F71_REG_6,
  PREFIX_VEX_0F72_REG_2,
  PREFIX_VEX_0F72_REG_4,
  PREFIX_VEX_0F72_REG_6,
  PREFIX_VEX_0F73_REG_2,
  PREFIX_VEX_0F73_REG_3,
  PREFIX_VEX_0F73_REG_6,
  PREFIX_VEX_0F73_REG_7,
  PREFIX_VEX_0F74,
  PREFIX_VEX_0F75,
  PREFIX_VEX_0F76,
  PREFIX_VEX_0F77,
  PREFIX_VEX_0F7C,
  PREFIX_VEX_0F7D,
  PREFIX_VEX_0F7E,
  PREFIX_VEX_0F7F,
  PREFIX_VEX_0F90,
  PREFIX_VEX_0F91,
  PREFIX_VEX_0F92,
  PREFIX_VEX_0F93,
  PREFIX_VEX_0F98,
  PREFIX_VEX_0F99,
  PREFIX_VEX_0FC2,
  PREFIX_VEX_0FC4,
  PREFIX_VEX_0FC5,
  PREFIX_VEX_0FD0,
  PREFIX_VEX_0FD1,
  PREFIX_VEX_0FD2,
  PREFIX_VEX_0FD3,
  PREFIX_VEX_0FD4,
  PREFIX_VEX_0FD5,
  PREFIX_VEX_0FD6,
  PREFIX_VEX_0FD7,
  PREFIX_VEX_0FD8,
  PREFIX_VEX_0FD9,
  PREFIX_VEX_0FDA,
  PREFIX_VEX_0FDB,
  PREFIX_VEX_0FDC,
  PREFIX_VEX_0FDD,
  PREFIX_VEX_0FDE,
  PREFIX_VEX_0FDF,
  PREFIX_VEX_0FE0,
  PREFIX_VEX_0FE1,
  PREFIX_VEX_0FE2,
  PREFIX_VEX_0FE3,
  PREFIX_VEX_0FE4,
  PREFIX_VEX_0FE5,
  PREFIX_VEX_0FE6,
  PREFIX_VEX_0FE7,
  PREFIX_VEX_0FE8,
  PREFIX_VEX_0FE9,
  PREFIX_VEX_0FEA,
  PREFIX_VEX_0FEB,
  PREFIX_VEX_0FEC,
  PREFIX_VEX_0FED,
  PREFIX_VEX_0FEE,
  PREFIX_VEX_0FEF,
  PREFIX_VEX_0FF0,
  PREFIX_VEX_0FF1,
  PREFIX_VEX_0FF2,
  PREFIX_VEX_0FF3,
  PREFIX_VEX_0FF4,
  PREFIX_VEX_0FF5,
  PREFIX_VEX_0FF6,
  PREFIX_VEX_0FF7,
  PREFIX_VEX_0FF8,
  PREFIX_VEX_0FF9,
  PREFIX_VEX_0FFA,
  PREFIX_VEX_0FFB,
  PREFIX_VEX_0FFC,
  PREFIX_VEX_0FFD,
  PREFIX_VEX_0FFE,
  PREFIX_VEX_0F3800,
  PREFIX_VEX_0F3801,
  PREFIX_VEX_0F3802,
  PREFIX_VEX_0F3803,
  PREFIX_VEX_0F3804,
  PREFIX_VEX_0F3805,
  PREFIX_VEX_0F3806,
  PREFIX_VEX_0F3807,
  PREFIX_VEX_0F3808,
  PREFIX_VEX_0F3809,
  PREFIX_VEX_0F380A,
  PREFIX_VEX_0F380B,
  PREFIX_VEX_0F380C,
  PREFIX_VEX_0F380D,
  PREFIX_VEX_0F380E,
  PREFIX_VEX_0F380F,
  PREFIX_VEX_0F3813,
  PREFIX_VEX_0F3816,
  PREFIX_VEX_0F3817,
  PREFIX_VEX_0F3818,
  PREFIX_VEX_0F3819,
  PREFIX_VEX_0F381A,
  PREFIX_VEX_0F381C,
  PREFIX_VEX_0F381D,
  PREFIX_VEX_0F381E,
  PREFIX_VEX_0F3820,
  PREFIX_VEX_0F3821,
  PREFIX_VEX_0F3822,
  PREFIX_VEX_0F3823,
  PREFIX_VEX_0F3824,
  PREFIX_VEX_0F3825,
  PREFIX_VEX_0F3828,
  PREFIX_VEX_0F3829,
  PREFIX_VEX_0F382A,
  PREFIX_VEX_0F382B,
  PREFIX_VEX_0F382C,
  PREFIX_VEX_0F382D,
  PREFIX_VEX_0F382E,
  PREFIX_VEX_0F382F,
  PREFIX_VEX_0F3830,
  PREFIX_VEX_0F3831,
  PREFIX_VEX_0F3832,
  PREFIX_VEX_0F3833,
  PREFIX_VEX_0F3834,
  PREFIX_VEX_0F3835,
  PREFIX_VEX_0F3836,
  PREFIX_VEX_0F3837,
  PREFIX_VEX_0F3838,
  PREFIX_VEX_0F3839,
  PREFIX_VEX_0F383A,
  PREFIX_VEX_0F383B,
  PREFIX_VEX_0F383C,
  PREFIX_VEX_0F383D,
  PREFIX_VEX_0F383E,
  PREFIX_VEX_0F383F,
  PREFIX_VEX_0F3840,
  PREFIX_VEX_0F3841,
  PREFIX_VEX_0F3845,
  PREFIX_VEX_0F3846,
  PREFIX_VEX_0F3847,
  PREFIX_VEX_0F3858,
  PREFIX_VEX_0F3859,
  PREFIX_VEX_0F385A,
  PREFIX_VEX_0F3878,
  PREFIX_VEX_0F3879,
  PREFIX_VEX_0F388C,
  PREFIX_VEX_0F388E,
  PREFIX_VEX_0F3890,
  PREFIX_VEX_0F3891,
  PREFIX_VEX_0F3892,
  PREFIX_VEX_0F3893,
  PREFIX_VEX_0F3896,
  PREFIX_VEX_0F3897,
  PREFIX_VEX_0F3898,
  PREFIX_VEX_0F3899,
  PREFIX_VEX_0F389A,
  PREFIX_VEX_0F389B,
  PREFIX_VEX_0F389C,
  PREFIX_VEX_0F389D,
  PREFIX_VEX_0F389E,
  PREFIX_VEX_0F389F,
  PREFIX_VEX_0F38A6,
  PREFIX_VEX_0F38A7,
  PREFIX_VEX_0F38A8,
  PREFIX_VEX_0F38A9,
  PREFIX_VEX_0F38AA,
  PREFIX_VEX_0F38AB,
  PREFIX_VEX_0F38AC,
  PREFIX_VEX_0F38AD,
  PREFIX_VEX_0F38AE,
  PREFIX_VEX_0F38AF,
  PREFIX_VEX_0F38B6,
  PREFIX_VEX_0F38B7,
  PREFIX_VEX_0F38B8,
  PREFIX_VEX_0F38B9,
  PREFIX_VEX_0F38BA,
  PREFIX_VEX_0F38BB,
  PREFIX_VEX_0F38BC,
  PREFIX_VEX_0F38BD,
  PREFIX_VEX_0F38BE,
  PREFIX_VEX_0F38BF,
  PREFIX_VEX_0F38DB,
  PREFIX_VEX_0F38DC,
  PREFIX_VEX_0F38DD,
  PREFIX_VEX_0F38DE,
  PREFIX_VEX_0F38DF,
  PREFIX_VEX_0F38F2,
  PREFIX_VEX_0F38F3_REG_1,
  PREFIX_VEX_0F38F3_REG_2,
  PREFIX_VEX_0F38F3_REG_3,
  PREFIX_VEX_0F38F5,
  PREFIX_VEX_0F38F6,
  PREFIX_VEX_0F38F7,
  PREFIX_VEX_0F3A00,
  PREFIX_VEX_0F3A01,
  PREFIX_VEX_0F3A02,
  PREFIX_VEX_0F3A04,
  PREFIX_VEX_0F3A05,
  PREFIX_VEX_0F3A06,
  PREFIX_VEX_0F3A08,
  PREFIX_VEX_0F3A09,
  PREFIX_VEX_0F3A0A,
  PREFIX_VEX_0F3A0B,
  PREFIX_VEX_0F3A0C,
  PREFIX_VEX_0F3A0D,
  PREFIX_VEX_0F3A0E,
  PREFIX_VEX_0F3A0F,
  PREFIX_VEX_0F3A14,
  PREFIX_VEX_0F3A15,
  PREFIX_VEX_0F3A16,
  PREFIX_VEX_0F3A17,
  PREFIX_VEX_0F3A18,
  PREFIX_VEX_0F3A19,
  PREFIX_VEX_0F3A1D,
  PREFIX_VEX_0F3A20,
  PREFIX_VEX_0F3A21,
  PREFIX_VEX_0F3A22,
  PREFIX_VEX_0F3A30,
  PREFIX_VEX_0F3A31,
  PREFIX_VEX_0F3A32,
  PREFIX_VEX_0F3A33,
  PREFIX_VEX_0F3A38,
  PREFIX_VEX_0F3A39,
  PREFIX_VEX_0F3A40,
  PREFIX_VEX_0F3A41,
  PREFIX_VEX_0F3A42,
  PREFIX_VEX_0F3A44,
  PREFIX_VEX_0F3A46,
  PREFIX_VEX_0F3A48,
  PREFIX_VEX_0F3A49,
  PREFIX_VEX_0F3A4A,
  PREFIX_VEX_0F3A4B,
  PREFIX_VEX_0F3A4C,
  PREFIX_VEX_0F3A5C,
  PREFIX_VEX_0F3A5D,
  PREFIX_VEX_0F3A5E,
  PREFIX_VEX_0F3A5F,
  PREFIX_VEX_0F3A60,
  PREFIX_VEX_0F3A61,
  PREFIX_VEX_0F3A62,
  PREFIX_VEX_0F3A63,
  PREFIX_VEX_0F3A68,
  PREFIX_VEX_0F3A69,
  PREFIX_VEX_0F3A6A,
  PREFIX_VEX_0F3A6B,
  PREFIX_VEX_0F3A6C,
  PREFIX_VEX_0F3A6D,
  PREFIX_VEX_0F3A6E,
  PREFIX_VEX_0F3A6F,
  PREFIX_VEX_0F3A78,
  PREFIX_VEX_0F3A79,
  PREFIX_VEX_0F3A7A,
  PREFIX_VEX_0F3A7B,
  PREFIX_VEX_0F3A7C,
  PREFIX_VEX_0F3A7D,
  PREFIX_VEX_0F3A7E,
  PREFIX_VEX_0F3A7F,
  PREFIX_VEX_0F3ADF,
  PREFIX_VEX_0F3AF0,

  PREFIX_EVEX_0F10,
  PREFIX_EVEX_0F11,
  PREFIX_EVEX_0F12,
  PREFIX_EVEX_0F13,
  PREFIX_EVEX_0F14,
  PREFIX_EVEX_0F15,
  PREFIX_EVEX_0F16,
  PREFIX_EVEX_0F17,
  PREFIX_EVEX_0F28,
  PREFIX_EVEX_0F29,
  PREFIX_EVEX_0F2A,
  PREFIX_EVEX_0F2B,
  PREFIX_EVEX_0F2C,
  PREFIX_EVEX_0F2D,
  PREFIX_EVEX_0F2E,
  PREFIX_EVEX_0F2F,
  PREFIX_EVEX_0F51,
  PREFIX_EVEX_0F54,
  PREFIX_EVEX_0F55,
  PREFIX_EVEX_0F56,
  PREFIX_EVEX_0F57,
  PREFIX_EVEX_0F58,
  PREFIX_EVEX_0F59,
  PREFIX_EVEX_0F5A,
  PREFIX_EVEX_0F5B,
  PREFIX_EVEX_0F5C,
  PREFIX_EVEX_0F5D,
  PREFIX_EVEX_0F5E,
  PREFIX_EVEX_0F5F,
  PREFIX_EVEX_0F60,
  PREFIX_EVEX_0F61,
  PREFIX_EVEX_0F62,
  PREFIX_EVEX_0F63,
  PREFIX_EVEX_0F64,
  PREFIX_EVEX_0F65,
  PREFIX_EVEX_0F66,
  PREFIX_EVEX_0F67,
  PREFIX_EVEX_0F68,
  PREFIX_EVEX_0F69,
  PREFIX_EVEX_0F6A,
  PREFIX_EVEX_0F6B,
  PREFIX_EVEX_0F6C,
  PREFIX_EVEX_0F6D,
  PREFIX_EVEX_0F6E,
  PREFIX_EVEX_0F6F,
  PREFIX_EVEX_0F70,
  PREFIX_EVEX_0F71_REG_2,
  PREFIX_EVEX_0F71_REG_4,
  PREFIX_EVEX_0F71_REG_6,
  PREFIX_EVEX_0F72_REG_0,
  PREFIX_EVEX_0F72_REG_1,
  PREFIX_EVEX_0F72_REG_2,
  PREFIX_EVEX_0F72_REG_4,
  PREFIX_EVEX_0F72_REG_6,
  PREFIX_EVEX_0F73_REG_2,
  PREFIX_EVEX_0F73_REG_3,
  PREFIX_EVEX_0F73_REG_6,
  PREFIX_EVEX_0F73_REG_7,
  PREFIX_EVEX_0F74,
  PREFIX_EVEX_0F75,
  PREFIX_EVEX_0F76,
  PREFIX_EVEX_0F78,
  PREFIX_EVEX_0F79,
  PREFIX_EVEX_0F7A,
  PREFIX_EVEX_0F7B,
  PREFIX_EVEX_0F7E,
  PREFIX_EVEX_0F7F,
  PREFIX_EVEX_0FC2,
  PREFIX_EVEX_0FC4,
  PREFIX_EVEX_0FC5,
  PREFIX_EVEX_0FC6,
  PREFIX_EVEX_0FD1,
  PREFIX_EVEX_0FD2,
  PREFIX_EVEX_0FD3,
  PREFIX_EVEX_0FD4,
  PREFIX_EVEX_0FD5,
  PREFIX_EVEX_0FD6,
  PREFIX_EVEX_0FD8,
  PREFIX_EVEX_0FD9,
  PREFIX_EVEX_0FDA,
  PREFIX_EVEX_0FDB,
  PREFIX_EVEX_0FDC,
  PREFIX_EVEX_0FDD,
  PREFIX_EVEX_0FDE,
  PREFIX_EVEX_0FDF,
  PREFIX_EVEX_0FE0,
  PREFIX_EVEX_0FE1,
  PREFIX_EVEX_0FE2,
  PREFIX_EVEX_0FE3,
  PREFIX_EVEX_0FE4,
  PREFIX_EVEX_0FE5,
  PREFIX_EVEX_0FE6,
  PREFIX_EVEX_0FE7,
  PREFIX_EVEX_0FE8,
  PREFIX_EVEX_0FE9,
  PREFIX_EVEX_0FEA,
  PREFIX_EVEX_0FEB,
  PREFIX_EVEX_0FEC,
  PREFIX_EVEX_0FED,
  PREFIX_EVEX_0FEE,
  PREFIX_EVEX_0FEF,
  PREFIX_EVEX_0FF1,
  PREFIX_EVEX_0FF2,
  PREFIX_EVEX_0FF3,
  PREFIX_EVEX_0FF4,
  PREFIX_EVEX_0FF5,
  PREFIX_EVEX_0FF6,
  PREFIX_EVEX_0FF8,
  PREFIX_EVEX_0FF9,
  PREFIX_EVEX_0FFA,
  PREFIX_EVEX_0FFB,
  PREFIX_EVEX_0FFC,
  PREFIX_EVEX_0FFD,
  PREFIX_EVEX_0FFE,
  PREFIX_EVEX_0F3800,
  PREFIX_EVEX_0F3804,
  PREFIX_EVEX_0F380B,
  PREFIX_EVEX_0F380C,
  PREFIX_EVEX_0F380D,
  PREFIX_EVEX_0F3810,
  PREFIX_EVEX_0F3811,
  PREFIX_EVEX_0F3812,
  PREFIX_EVEX_0F3813,
  PREFIX_EVEX_0F3814,
  PREFIX_EVEX_0F3815,
  PREFIX_EVEX_0F3816,
  PREFIX_EVEX_0F3818,
  PREFIX_EVEX_0F3819,
  PREFIX_EVEX_0F381A,
  PREFIX_EVEX_0F381B,
  PREFIX_EVEX_0F381C,
  PREFIX_EVEX_0F381D,
  PREFIX_EVEX_0F381E,
  PREFIX_EVEX_0F381F,
  PREFIX_EVEX_0F3820,
  PREFIX_EVEX_0F3821,
  PREFIX_EVEX_0F3822,
  PREFIX_EVEX_0F3823,
  PREFIX_EVEX_0F3824,
  PREFIX_EVEX_0F3825,
  PREFIX_EVEX_0F3826,
  PREFIX_EVEX_0F3827,
  PREFIX_EVEX_0F3828,
  PREFIX_EVEX_0F3829,
  PREFIX_EVEX_0F382A,
  PREFIX_EVEX_0F382B,
  PREFIX_EVEX_0F382C,
  PREFIX_EVEX_0F382D,
  PREFIX_EVEX_0F3830,
  PREFIX_EVEX_0F3831,
  PREFIX_EVEX_0F3832,
  PREFIX_EVEX_0F3833,
  PREFIX_EVEX_0F3834,
  PREFIX_EVEX_0F3835,
  PREFIX_EVEX_0F3836,
  PREFIX_EVEX_0F3837,
  PREFIX_EVEX_0F3838,
  PREFIX_EVEX_0F3839,
  PREFIX_EVEX_0F383A,
  PREFIX_EVEX_0F383B,
  PREFIX_EVEX_0F383C,
  PREFIX_EVEX_0F383D,
  PREFIX_EVEX_0F383E,
  PREFIX_EVEX_0F383F,
  PREFIX_EVEX_0F3840,
  PREFIX_EVEX_0F3842,
  PREFIX_EVEX_0F3843,
  PREFIX_EVEX_0F3844,
  PREFIX_EVEX_0F3845,
  PREFIX_EVEX_0F3846,
  PREFIX_EVEX_0F3847,
  PREFIX_EVEX_0F384C,
  PREFIX_EVEX_0F384D,
  PREFIX_EVEX_0F384E,
  PREFIX_EVEX_0F384F,
  PREFIX_EVEX_0F3858,
  PREFIX_EVEX_0F3859,
  PREFIX_EVEX_0F385A,
  PREFIX_EVEX_0F385B,
  PREFIX_EVEX_0F3864,
  PREFIX_EVEX_0F3865,
  PREFIX_EVEX_0F3866,
  PREFIX_EVEX_0F3875,
  PREFIX_EVEX_0F3876,
  PREFIX_EVEX_0F3877,
  PREFIX_EVEX_0F3878,
  PREFIX_EVEX_0F3879,
  PREFIX_EVEX_0F387A,
  PREFIX_EVEX_0F387B,
  PREFIX_EVEX_0F387C,
  PREFIX_EVEX_0F387D,
  PREFIX_EVEX_0F387E,
  PREFIX_EVEX_0F387F,
  PREFIX_EVEX_0F3883,
  PREFIX_EVEX_0F3888,
  PREFIX_EVEX_0F3889,
  PREFIX_EVEX_0F388A,
  PREFIX_EVEX_0F388B,
  PREFIX_EVEX_0F388D,
  PREFIX_EVEX_0F3890,
  PREFIX_EVEX_0F3891,
  PREFIX_EVEX_0F3892,
  PREFIX_EVEX_0F3893,
  PREFIX_EVEX_0F3896,
  PREFIX_EVEX_0F3897,
  PREFIX_EVEX_0F3898,
  PREFIX_EVEX_0F3899,
  PREFIX_EVEX_0F389A,
  PREFIX_EVEX_0F389B,
  PREFIX_EVEX_0F389C,
  PREFIX_EVEX_0F389D,
  PREFIX_EVEX_0F389E,
  PREFIX_EVEX_0F389F,
  PREFIX_EVEX_0F38A0,
  PREFIX_EVEX_0F38A1,
  PREFIX_EVEX_0F38A2,
  PREFIX_EVEX_0F38A3,
  PREFIX_EVEX_0F38A6,
  PREFIX_EVEX_0F38A7,
  PREFIX_EVEX_0F38A8,
  PREFIX_EVEX_0F38A9,
  PREFIX_EVEX_0F38AA,
  PREFIX_EVEX_0F38AB,
  PREFIX_EVEX_0F38AC,
  PREFIX_EVEX_0F38AD,
  PREFIX_EVEX_0F38AE,
  PREFIX_EVEX_0F38AF,
  PREFIX_EVEX_0F38B4,
  PREFIX_EVEX_0F38B5,
  PREFIX_EVEX_0F38B6,
  PREFIX_EVEX_0F38B7,
  PREFIX_EVEX_0F38B8,
  PREFIX_EVEX_0F38B9,
  PREFIX_EVEX_0F38BA,
  PREFIX_EVEX_0F38BB,
  PREFIX_EVEX_0F38BC,
  PREFIX_EVEX_0F38BD,
  PREFIX_EVEX_0F38BE,
  PREFIX_EVEX_0F38BF,
  PREFIX_EVEX_0F38C4,
  PREFIX_EVEX_0F38C6_REG_1,
  PREFIX_EVEX_0F38C6_REG_2,
  PREFIX_EVEX_0F38C6_REG_5,
  PREFIX_EVEX_0F38C6_REG_6,
  PREFIX_EVEX_0F38C7_REG_1,
  PREFIX_EVEX_0F38C7_REG_2,
  PREFIX_EVEX_0F38C7_REG_5,
  PREFIX_EVEX_0F38C7_REG_6,
  PREFIX_EVEX_0F38C8,
  PREFIX_EVEX_0F38CA,
  PREFIX_EVEX_0F38CB,
  PREFIX_EVEX_0F38CC,
  PREFIX_EVEX_0F38CD,

  PREFIX_EVEX_0F3A00,
  PREFIX_EVEX_0F3A01,
  PREFIX_EVEX_0F3A03,
  PREFIX_EVEX_0F3A04,
  PREFIX_EVEX_0F3A05,
  PREFIX_EVEX_0F3A08,
  PREFIX_EVEX_0F3A09,
  PREFIX_EVEX_0F3A0A,
  PREFIX_EVEX_0F3A0B,
  PREFIX_EVEX_0F3A0F,
  PREFIX_EVEX_0F3A14,
  PREFIX_EVEX_0F3A15,
  PREFIX_EVEX_0F3A16,
  PREFIX_EVEX_0F3A17,
  PREFIX_EVEX_0F3A18,
  PREFIX_EVEX_0F3A19,
  PREFIX_EVEX_0F3A1A,
  PREFIX_EVEX_0F3A1B,
  PREFIX_EVEX_0F3A1D,
  PREFIX_EVEX_0F3A1E,
  PREFIX_EVEX_0F3A1F,
  PREFIX_EVEX_0F3A20,
  PREFIX_EVEX_0F3A21,
  PREFIX_EVEX_0F3A22,
  PREFIX_EVEX_0F3A23,
  PREFIX_EVEX_0F3A25,
  PREFIX_EVEX_0F3A26,
  PREFIX_EVEX_0F3A27,
  PREFIX_EVEX_0F3A38,
  PREFIX_EVEX_0F3A39,
  PREFIX_EVEX_0F3A3A,
  PREFIX_EVEX_0F3A3B,
  PREFIX_EVEX_0F3A3E,
  PREFIX_EVEX_0F3A3F,
  PREFIX_EVEX_0F3A42,
  PREFIX_EVEX_0F3A43,
  PREFIX_EVEX_0F3A50,
  PREFIX_EVEX_0F3A51,
  PREFIX_EVEX_0F3A54,
  PREFIX_EVEX_0F3A55,
  PREFIX_EVEX_0F3A56,
  PREFIX_EVEX_0F3A57,
  PREFIX_EVEX_0F3A66,
  PREFIX_EVEX_0F3A67
};

enum
{
  X86_64_06 = 0,
  X86_64_07,
  X86_64_0D,
  X86_64_16,
  X86_64_17,
  X86_64_1E,
  X86_64_1F,
  X86_64_27,
  X86_64_2F,
  X86_64_37,
  X86_64_3F,
  X86_64_60,
  X86_64_61,
  X86_64_62,
  X86_64_63,
  X86_64_6D,
  X86_64_6F,
  X86_64_9A,
  X86_64_C4,
  X86_64_C5,
  X86_64_CE,
  X86_64_D4,
  X86_64_D5,
  X86_64_E8,
  X86_64_E9,
  X86_64_EA,
  X86_64_0F01_REG_0,
  X86_64_0F01_REG_1,
  X86_64_0F01_REG_2,
  X86_64_0F01_REG_3
};

enum
{
  THREE_BYTE_0F38 = 0,
  THREE_BYTE_0F3A,
  THREE_BYTE_0F7A
};

enum
{
  XOP_08 = 0,
  XOP_09,
  XOP_0A
};

enum
{
  VEX_0F = 0,
  VEX_0F38,
  VEX_0F3A
};

enum
{
  EVEX_0F = 0,
  EVEX_0F38,
  EVEX_0F3A
};

enum
{
  VEX_LEN_0F10_P_1 = 0,
  VEX_LEN_0F10_P_3,
  VEX_LEN_0F11_P_1,
  VEX_LEN_0F11_P_3,
  VEX_LEN_0F12_P_0_M_0,
  VEX_LEN_0F12_P_0_M_1,
  VEX_LEN_0F12_P_2,
  VEX_LEN_0F13_M_0,
  VEX_LEN_0F16_P_0_M_0,
  VEX_LEN_0F16_P_0_M_1,
  VEX_LEN_0F16_P_2,
  VEX_LEN_0F17_M_0,
  VEX_LEN_0F2A_P_1,
  VEX_LEN_0F2A_P_3,
  VEX_LEN_0F2C_P_1,
  VEX_LEN_0F2C_P_3,
  VEX_LEN_0F2D_P_1,
  VEX_LEN_0F2D_P_3,
  VEX_LEN_0F2E_P_0,
  VEX_LEN_0F2E_P_2,
  VEX_LEN_0F2F_P_0,
  VEX_LEN_0F2F_P_2,
  VEX_LEN_0F41_P_0,
  VEX_LEN_0F41_P_2,
  VEX_LEN_0F42_P_0,
  VEX_LEN_0F42_P_2,
  VEX_LEN_0F44_P_0,
  VEX_LEN_0F44_P_2,
  VEX_LEN_0F45_P_0,
  VEX_LEN_0F45_P_2,
  VEX_LEN_0F46_P_0,
  VEX_LEN_0F46_P_2,
  VEX_LEN_0F47_P_0,
  VEX_LEN_0F47_P_2,
  VEX_LEN_0F4A_P_0,
  VEX_LEN_0F4A_P_2,
  VEX_LEN_0F4B_P_0,
  VEX_LEN_0F4B_P_2,
  VEX_LEN_0F51_P_1,
  VEX_LEN_0F51_P_3,
  VEX_LEN_0F52_P_1,
  VEX_LEN_0F53_P_1,
  VEX_LEN_0F58_P_1,
  VEX_LEN_0F58_P_3,
  VEX_LEN_0F59_P_1,
  VEX_LEN_0F59_P_3,
  VEX_LEN_0F5A_P_1,
  VEX_LEN_0F5A_P_3,
  VEX_LEN_0F5C_P_1,
  VEX_LEN_0F5C_P_3,
  VEX_LEN_0F5D_P_1,
  VEX_LEN_0F5D_P_3,
  VEX_LEN_0F5E_P_1,
  VEX_LEN_0F5E_P_3,
  VEX_LEN_0F5F_P_1,
  VEX_LEN_0F5F_P_3,
  VEX_LEN_0F6E_P_2,
  VEX_LEN_0F7E_P_1,
  VEX_LEN_0F7E_P_2,
  VEX_LEN_0F90_P_0,
  VEX_LEN_0F90_P_2,
  VEX_LEN_0F91_P_0,
  VEX_LEN_0F91_P_2,
  VEX_LEN_0F92_P_0,
  VEX_LEN_0F92_P_2,
  VEX_LEN_0F92_P_3,
  VEX_LEN_0F93_P_0,
  VEX_LEN_0F93_P_2,
  VEX_LEN_0F93_P_3,
  VEX_LEN_0F98_P_0,
  VEX_LEN_0F98_P_2,
  VEX_LEN_0F99_P_0,
  VEX_LEN_0F99_P_2,
  VEX_LEN_0FAE_R_2_M_0,
  VEX_LEN_0FAE_R_3_M_0,
  VEX_LEN_0FC2_P_1,
  VEX_LEN_0FC2_P_3,
  VEX_LEN_0FC4_P_2,
  VEX_LEN_0FC5_P_2,
  VEX_LEN_0FD6_P_2,
  VEX_LEN_0FF7_P_2,
  VEX_LEN_0F3816_P_2,
  VEX_LEN_0F3819_P_2,
  VEX_LEN_0F381A_P_2_M_0,
  VEX_LEN_0F3836_P_2,
  VEX_LEN_0F3841_P_2,
  VEX_LEN_0F385A_P_2_M_0,
  VEX_LEN_0F38DB_P_2,
  VEX_LEN_0F38DC_P_2,
  VEX_LEN_0F38DD_P_2,
  VEX_LEN_0F38DE_P_2,
  VEX_LEN_0F38DF_P_2,
  VEX_LEN_0F38F2_P_0,
  VEX_LEN_0F38F3_R_1_P_0,
  VEX_LEN_0F38F3_R_2_P_0,
  VEX_LEN_0F38F3_R_3_P_0,
  VEX_LEN_0F38F5_P_0,
  VEX_LEN_0F38F5_P_1,
  VEX_LEN_0F38F5_P_3,
  VEX_LEN_0F38F6_P_3,
  VEX_LEN_0F38F7_P_0,
  VEX_LEN_0F38F7_P_1,
  VEX_LEN_0F38F7_P_2,
  VEX_LEN_0F38F7_P_3,
  VEX_LEN_0F3A00_P_2,
  VEX_LEN_0F3A01_P_2,
  VEX_LEN_0F3A06_P_2,
  VEX_LEN_0F3A0A_P_2,
  VEX_LEN_0F3A0B_P_2,
  VEX_LEN_0F3A14_P_2,
  VEX_LEN_0F3A15_P_2,
  VEX_LEN_0F3A16_P_2,
  VEX_LEN_0F3A17_P_2,
  VEX_LEN_0F3A18_P_2,
  VEX_LEN_0F3A19_P_2,
  VEX_LEN_0F3A20_P_2,
  VEX_LEN_0F3A21_P_2,
  VEX_LEN_0F3A22_P_2,
  VEX_LEN_0F3A30_P_2,
  VEX_LEN_0F3A31_P_2,
  VEX_LEN_0F3A32_P_2,
  VEX_LEN_0F3A33_P_2,
  VEX_LEN_0F3A38_P_2,
  VEX_LEN_0F3A39_P_2,
  VEX_LEN_0F3A41_P_2,
  VEX_LEN_0F3A44_P_2,
  VEX_LEN_0F3A46_P_2,
  VEX_LEN_0F3A60_P_2,
  VEX_LEN_0F3A61_P_2,
  VEX_LEN_0F3A62_P_2,
  VEX_LEN_0F3A63_P_2,
  VEX_LEN_0F3A6A_P_2,
  VEX_LEN_0F3A6B_P_2,
  VEX_LEN_0F3A6E_P_2,
  VEX_LEN_0F3A6F_P_2,
  VEX_LEN_0F3A7A_P_2,
  VEX_LEN_0F3A7B_P_2,
  VEX_LEN_0F3A7E_P_2,
  VEX_LEN_0F3A7F_P_2,
  VEX_LEN_0F3ADF_P_2,
  VEX_LEN_0F3AF0_P_3,
  VEX_LEN_0FXOP_08_CC,
  VEX_LEN_0FXOP_08_CD,
  VEX_LEN_0FXOP_08_CE,
  VEX_LEN_0FXOP_08_CF,
  VEX_LEN_0FXOP_08_EC,
  VEX_LEN_0FXOP_08_ED,
  VEX_LEN_0FXOP_08_EE,
  VEX_LEN_0FXOP_08_EF,
  VEX_LEN_0FXOP_09_80,
  VEX_LEN_0FXOP_09_81
};

enum
{
  VEX_W_0F10_P_0 = 0,
  VEX_W_0F10_P_1,
  VEX_W_0F10_P_2,
  VEX_W_0F10_P_3,
  VEX_W_0F11_P_0,
  VEX_W_0F11_P_1,
  VEX_W_0F11_P_2,
  VEX_W_0F11_P_3,
  VEX_W_0F12_P_0_M_0,
  VEX_W_0F12_P_0_M_1,
  VEX_W_0F12_P_1,
  VEX_W_0F12_P_2,
  VEX_W_0F12_P_3,
  VEX_W_0F13_M_0,
  VEX_W_0F14,
  VEX_W_0F15,
  VEX_W_0F16_P_0_M_0,
  VEX_W_0F16_P_0_M_1,
  VEX_W_0F16_P_1,
  VEX_W_0F16_P_2,
  VEX_W_0F17_M_0,
  VEX_W_0F28,
  VEX_W_0F29,
  VEX_W_0F2B_M_0,
  VEX_W_0F2E_P_0,
  VEX_W_0F2E_P_2,
  VEX_W_0F2F_P_0,
  VEX_W_0F2F_P_2,
  VEX_W_0F41_P_0_LEN_1,
  VEX_W_0F41_P_2_LEN_1,
  VEX_W_0F42_P_0_LEN_1,
  VEX_W_0F42_P_2_LEN_1,
  VEX_W_0F44_P_0_LEN_0,
  VEX_W_0F44_P_2_LEN_0,
  VEX_W_0F45_P_0_LEN_1,
  VEX_W_0F45_P_2_LEN_1,
  VEX_W_0F46_P_0_LEN_1,
  VEX_W_0F46_P_2_LEN_1,
  VEX_W_0F47_P_0_LEN_1,
  VEX_W_0F47_P_2_LEN_1,
  VEX_W_0F4A_P_0_LEN_1,
  VEX_W_0F4A_P_2_LEN_1,
  VEX_W_0F4B_P_0_LEN_1,
  VEX_W_0F4B_P_2_LEN_1,
  VEX_W_0F50_M_0,
  VEX_W_0F51_P_0,
  VEX_W_0F51_P_1,
  VEX_W_0F51_P_2,
  VEX_W_0F51_P_3,
  VEX_W_0F52_P_0,
  VEX_W_0F52_P_1,
  VEX_W_0F53_P_0,
  VEX_W_0F53_P_1,
  VEX_W_0F58_P_0,
  VEX_W_0F58_P_1,
  VEX_W_0F58_P_2,
  VEX_W_0F58_P_3,
  VEX_W_0F59_P_0,
  VEX_W_0F59_P_1,
  VEX_W_0F59_P_2,
  VEX_W_0F59_P_3,
  VEX_W_0F5A_P_0,
  VEX_W_0F5A_P_1,
  VEX_W_0F5A_P_3,
  VEX_W_0F5B_P_0,
  VEX_W_0F5B_P_1,
  VEX_W_0F5B_P_2,
  VEX_W_0F5C_P_0,
  VEX_W_0F5C_P_1,
  VEX_W_0F5C_P_2,
  VEX_W_0F5C_P_3,
  VEX_W_0F5D_P_0,
  VEX_W_0F5D_P_1,
  VEX_W_0F5D_P_2,
  VEX_W_0F5D_P_3,
  VEX_W_0F5E_P_0,
  VEX_W_0F5E_P_1,
  VEX_W_0F5E_P_2,
  VEX_W_0F5E_P_3,
  VEX_W_0F5F_P_0,
  VEX_W_0F5F_P_1,
  VEX_W_0F5F_P_2,
  VEX_W_0F5F_P_3,
  VEX_W_0F60_P_2,
  VEX_W_0F61_P_2,
  VEX_W_0F62_P_2,
  VEX_W_0F63_P_2,
  VEX_W_0F64_P_2,
  VEX_W_0F65_P_2,
  VEX_W_0F66_P_2,
  VEX_W_0F67_P_2,
  VEX_W_0F68_P_2,
  VEX_W_0F69_P_2,
  VEX_W_0F6A_P_2,
  VEX_W_0F6B_P_2,
  VEX_W_0F6C_P_2,
  VEX_W_0F6D_P_2,
  VEX_W_0F6F_P_1,
  VEX_W_0F6F_P_2,
  VEX_W_0F70_P_1,
  VEX_W_0F70_P_2,
  VEX_W_0F70_P_3,
  VEX_W_0F71_R_2_P_2,
  VEX_W_0F71_R_4_P_2,
  VEX_W_0F71_R_6_P_2,
  VEX_W_0F72_R_2_P_2,
  VEX_W_0F72_R_4_P_2,
  VEX_W_0F72_R_6_P_2,
  VEX_W_0F73_R_2_P_2,
  VEX_W_0F73_R_3_P_2,
  VEX_W_0F73_R_6_P_2,
  VEX_W_0F73_R_7_P_2,
  VEX_W_0F74_P_2,
  VEX_W_0F75_P_2,
  VEX_W_0F76_P_2,
  VEX_W_0F77_P_0,
  VEX_W_0F7C_P_2,
  VEX_W_0F7C_P_3,
  VEX_W_0F7D_P_2,
  VEX_W_0F7D_P_3,
  VEX_W_0F7E_P_1,
  VEX_W_0F7F_P_1,
  VEX_W_0F7F_P_2,
  VEX_W_0F90_P_0_LEN_0,
  VEX_W_0F90_P_2_LEN_0,
  VEX_W_0F91_P_0_LEN_0,
  VEX_W_0F91_P_2_LEN_0,
  VEX_W_0F92_P_0_LEN_0,
  VEX_W_0F92_P_2_LEN_0,
  VEX_W_0F92_P_3_LEN_0,
  VEX_W_0F93_P_0_LEN_0,
  VEX_W_0F93_P_2_LEN_0,
  VEX_W_0F93_P_3_LEN_0,
  VEX_W_0F98_P_0_LEN_0,
  VEX_W_0F98_P_2_LEN_0,
  VEX_W_0F99_P_0_LEN_0,
  VEX_W_0F99_P_2_LEN_0,
  VEX_W_0FAE_R_2_M_0,
  VEX_W_0FAE_R_3_M_0,
  VEX_W_0FC2_P_0,
  VEX_W_0FC2_P_1,
  VEX_W_0FC2_P_2,
  VEX_W_0FC2_P_3,
  VEX_W_0FC4_P_2,
  VEX_W_0FC5_P_2,
  VEX_W_0FD0_P_2,
  VEX_W_0FD0_P_3,
  VEX_W_0FD1_P_2,
  VEX_W_0FD2_P_2,
  VEX_W_0FD3_P_2,
  VEX_W_0FD4_P_2,
  VEX_W_0FD5_P_2,
  VEX_W_0FD6_P_2,
  VEX_W_0FD7_P_2_M_1,
  VEX_W_0FD8_P_2,
  VEX_W_0FD9_P_2,
  VEX_W_0FDA_P_2,
  VEX_W_0FDB_P_2,
  VEX_W_0FDC_P_2,
  VEX_W_0FDD_P_2,
  VEX_W_0FDE_P_2,
  VEX_W_0FDF_P_2,
  VEX_W_0FE0_P_2,
  VEX_W_0FE1_P_2,
  VEX_W_0FE2_P_2,
  VEX_W_0FE3_P_2,
  VEX_W_0FE4_P_2,
  VEX_W_0FE5_P_2,
  VEX_W_0FE6_P_1,
  VEX_W_0FE6_P_2,
  VEX_W_0FE6_P_3,
  VEX_W_0FE7_P_2_M_0,
  VEX_W_0FE8_P_2,
  VEX_W_0FE9_P_2,
  VEX_W_0FEA_P_2,
  VEX_W_0FEB_P_2,
  VEX_W_0FEC_P_2,
  VEX_W_0FED_P_2,
  VEX_W_0FEE_P_2,
  VEX_W_0FEF_P_2,
  VEX_W_0FF0_P_3_M_0,
  VEX_W_0FF1_P_2,
  VEX_W_0FF2_P_2,
  VEX_W_0FF3_P_2,
  VEX_W_0FF4_P_2,
  VEX_W_0FF5_P_2,
  VEX_W_0FF6_P_2,
  VEX_W_0FF7_P_2,
  VEX_W_0FF8_P_2,
  VEX_W_0FF9_P_2,
  VEX_W_0FFA_P_2,
  VEX_W_0FFB_P_2,
  VEX_W_0FFC_P_2,
  VEX_W_0FFD_P_2,
  VEX_W_0FFE_P_2,
  VEX_W_0F3800_P_2,
  VEX_W_0F3801_P_2,
  VEX_W_0F3802_P_2,
  VEX_W_0F3803_P_2,
  VEX_W_0F3804_P_2,
  VEX_W_0F3805_P_2,
  VEX_W_0F3806_P_2,
  VEX_W_0F3807_P_2,
  VEX_W_0F3808_P_2,
  VEX_W_0F3809_P_2,
  VEX_W_0F380A_P_2,
  VEX_W_0F380B_P_2,
  VEX_W_0F380C_P_2,
  VEX_W_0F380D_P_2,
  VEX_W_0F380E_P_2,
  VEX_W_0F380F_P_2,
  VEX_W_0F3816_P_2,
  VEX_W_0F3817_P_2,
  VEX_W_0F3818_P_2,
  VEX_W_0F3819_P_2,
  VEX_W_0F381A_P_2_M_0,
  VEX_W_0F381C_P_2,
  VEX_W_0F381D_P_2,
  VEX_W_0F381E_P_2,
  VEX_W_0F3820_P_2,
  VEX_W_0F3821_P_2,
  VEX_W_0F3822_P_2,
  VEX_W_0F3823_P_2,
  VEX_W_0F3824_P_2,
  VEX_W_0F3825_P_2,
  VEX_W_0F3828_P_2,
  VEX_W_0F3829_P_2,
  VEX_W_0F382A_P_2_M_0,
  VEX_W_0F382B_P_2,
  VEX_W_0F382C_P_2_M_0,
  VEX_W_0F382D_P_2_M_0,
  VEX_W_0F382E_P_2_M_0,
  VEX_W_0F382F_P_2_M_0,
  VEX_W_0F3830_P_2,
  VEX_W_0F3831_P_2,
  VEX_W_0F3832_P_2,
  VEX_W_0F3833_P_2,
  VEX_W_0F3834_P_2,
  VEX_W_0F3835_P_2,
  VEX_W_0F3836_P_2,
  VEX_W_0F3837_P_2,
  VEX_W_0F3838_P_2,
  VEX_W_0F3839_P_2,
  VEX_W_0F383A_P_2,
  VEX_W_0F383B_P_2,
  VEX_W_0F383C_P_2,
  VEX_W_0F383D_P_2,
  VEX_W_0F383E_P_2,
  VEX_W_0F383F_P_2,
  VEX_W_0F3840_P_2,
  VEX_W_0F3841_P_2,
  VEX_W_0F3846_P_2,
  VEX_W_0F3858_P_2,
  VEX_W_0F3859_P_2,
  VEX_W_0F385A_P_2_M_0,
  VEX_W_0F3878_P_2,
  VEX_W_0F3879_P_2,
  VEX_W_0F38DB_P_2,
  VEX_W_0F38DC_P_2,
  VEX_W_0F38DD_P_2,
  VEX_W_0F38DE_P_2,
  VEX_W_0F38DF_P_2,
  VEX_W_0F3A00_P_2,
  VEX_W_0F3A01_P_2,
  VEX_W_0F3A02_P_2,
  VEX_W_0F3A04_P_2,
  VEX_W_0F3A05_P_2,
  VEX_W_0F3A06_P_2,
  VEX_W_0F3A08_P_2,
  VEX_W_0F3A09_P_2,
  VEX_W_0F3A0A_P_2,
  VEX_W_0F3A0B_P_2,
  VEX_W_0F3A0C_P_2,
  VEX_W_0F3A0D_P_2,
  VEX_W_0F3A0E_P_2,
  VEX_W_0F3A0F_P_2,
  VEX_W_0F3A14_P_2,
  VEX_W_0F3A15_P_2,
  VEX_W_0F3A18_P_2,
  VEX_W_0F3A19_P_2,
  VEX_W_0F3A20_P_2,
  VEX_W_0F3A21_P_2,
  VEX_W_0F3A30_P_2_LEN_0,
  VEX_W_0F3A31_P_2_LEN_0,
  VEX_W_0F3A32_P_2_LEN_0,
  VEX_W_0F3A33_P_2_LEN_0,
  VEX_W_0F3A38_P_2,
  VEX_W_0F3A39_P_2,
  VEX_W_0F3A40_P_2,
  VEX_W_0F3A41_P_2,
  VEX_W_0F3A42_P_2,
  VEX_W_0F3A44_P_2,
  VEX_W_0F3A46_P_2,
  VEX_W_0F3A48_P_2,
  VEX_W_0F3A49_P_2,
  VEX_W_0F3A4A_P_2,
  VEX_W_0F3A4B_P_2,
  VEX_W_0F3A4C_P_2,
  VEX_W_0F3A60_P_2,
  VEX_W_0F3A61_P_2,
  VEX_W_0F3A62_P_2,
  VEX_W_0F3A63_P_2,
  VEX_W_0F3ADF_P_2,

  EVEX_W_0F10_P_0,
  EVEX_W_0F10_P_1_M_0,
  EVEX_W_0F10_P_1_M_1,
  EVEX_W_0F10_P_2,
  EVEX_W_0F10_P_3_M_0,
  EVEX_W_0F10_P_3_M_1,
  EVEX_W_0F11_P_0,
  EVEX_W_0F11_P_1_M_0,
  EVEX_W_0F11_P_1_M_1,
  EVEX_W_0F11_P_2,
  EVEX_W_0F11_P_3_M_0,
  EVEX_W_0F11_P_3_M_1,
  EVEX_W_0F12_P_0_M_0,
  EVEX_W_0F12_P_0_M_1,
  EVEX_W_0F12_P_1,
  EVEX_W_0F12_P_2,
  EVEX_W_0F12_P_3,
  EVEX_W_0F13_P_0,
  EVEX_W_0F13_P_2,
  EVEX_W_0F14_P_0,
  EVEX_W_0F14_P_2,
  EVEX_W_0F15_P_0,
  EVEX_W_0F15_P_2,
  EVEX_W_0F16_P_0_M_0,
  EVEX_W_0F16_P_0_M_1,
  EVEX_W_0F16_P_1,
  EVEX_W_0F16_P_2,
  EVEX_W_0F17_P_0,
  EVEX_W_0F17_P_2,
  EVEX_W_0F28_P_0,
  EVEX_W_0F28_P_2,
  EVEX_W_0F29_P_0,
  EVEX_W_0F29_P_2,
  EVEX_W_0F2A_P_1,
  EVEX_W_0F2A_P_3,
  EVEX_W_0F2B_P_0,
  EVEX_W_0F2B_P_2,
  EVEX_W_0F2E_P_0,
  EVEX_W_0F2E_P_2,
  EVEX_W_0F2F_P_0,
  EVEX_W_0F2F_P_2,
  EVEX_W_0F51_P_0,
  EVEX_W_0F51_P_1,
  EVEX_W_0F51_P_2,
  EVEX_W_0F51_P_3,
  EVEX_W_0F54_P_0,
  EVEX_W_0F54_P_2,
  EVEX_W_0F55_P_0,
  EVEX_W_0F55_P_2,
  EVEX_W_0F56_P_0,
  EVEX_W_0F56_P_2,
  EVEX_W_0F57_P_0,
  EVEX_W_0F57_P_2,
  EVEX_W_0F58_P_0,
  EVEX_W_0F58_P_1,
  EVEX_W_0F58_P_2,
  EVEX_W_0F58_P_3,
  EVEX_W_0F59_P_0,
  EVEX_W_0F59_P_1,
  EVEX_W_0F59_P_2,
  EVEX_W_0F59_P_3,
  EVEX_W_0F5A_P_0,
  EVEX_W_0F5A_P_1,
  EVEX_W_0F5A_P_2,
  EVEX_W_0F5A_P_3,
  EVEX_W_0F5B_P_0,
  EVEX_W_0F5B_P_1,
  EVEX_W_0F5B_P_2,
  EVEX_W_0F5C_P_0,
  EVEX_W_0F5C_P_1,
  EVEX_W_0F5C_P_2,
  EVEX_W_0F5C_P_3,
  EVEX_W_0F5D_P_0,
  EVEX_W_0F5D_P_1,
  EVEX_W_0F5D_P_2,
  EVEX_W_0F5D_P_3,
  EVEX_W_0F5E_P_0,
  EVEX_W_0F5E_P_1,
  EVEX_W_0F5E_P_2,
  EVEX_W_0F5E_P_3,
  EVEX_W_0F5F_P_0,
  EVEX_W_0F5F_P_1,
  EVEX_W_0F5F_P_2,
  EVEX_W_0F5F_P_3,
  EVEX_W_0F62_P_2,
  EVEX_W_0F66_P_2,
  EVEX_W_0F6A_P_2,
  EVEX_W_0F6B_P_2,
  EVEX_W_0F6C_P_2,
  EVEX_W_0F6D_P_2,
  EVEX_W_0F6E_P_2,
  EVEX_W_0F6F_P_1,
  EVEX_W_0F6F_P_2,
  EVEX_W_0F6F_P_3,
  EVEX_W_0F70_P_2,
  EVEX_W_0F72_R_2_P_2,
  EVEX_W_0F72_R_6_P_2,
  EVEX_W_0F73_R_2_P_2,
  EVEX_W_0F73_R_6_P_2,
  EVEX_W_0F76_P_2,
  EVEX_W_0F78_P_0,
  EVEX_W_0F78_P_2,
  EVEX_W_0F79_P_0,
  EVEX_W_0F79_P_2,
  EVEX_W_0F7A_P_1,
  EVEX_W_0F7A_P_2,
  EVEX_W_0F7A_P_3,
  EVEX_W_0F7B_P_1,
  EVEX_W_0F7B_P_2,
  EVEX_W_0F7B_P_3,
  EVEX_W_0F7E_P_1,
  EVEX_W_0F7E_P_2,
  EVEX_W_0F7F_P_1,
  EVEX_W_0F7F_P_2,
  EVEX_W_0F7F_P_3,
  EVEX_W_0FC2_P_0,
  EVEX_W_0FC2_P_1,
  EVEX_W_0FC2_P_2,
  EVEX_W_0FC2_P_3,
  EVEX_W_0FC6_P_0,
  EVEX_W_0FC6_P_2,
  EVEX_W_0FD2_P_2,
  EVEX_W_0FD3_P_2,
  EVEX_W_0FD4_P_2,
  EVEX_W_0FD6_P_2,
  EVEX_W_0FE6_P_1,
  EVEX_W_0FE6_P_2,
  EVEX_W_0FE6_P_3,
  EVEX_W_0FE7_P_2,
  EVEX_W_0FF2_P_2,
  EVEX_W_0FF3_P_2,
  EVEX_W_0FF4_P_2,
  EVEX_W_0FFA_P_2,
  EVEX_W_0FFB_P_2,
  EVEX_W_0FFE_P_2,
  EVEX_W_0F380C_P_2,
  EVEX_W_0F380D_P_2,
  EVEX_W_0F3810_P_1,
  EVEX_W_0F3810_P_2,
  EVEX_W_0F3811_P_1,
  EVEX_W_0F3811_P_2,
  EVEX_W_0F3812_P_1,
  EVEX_W_0F3812_P_2,
  EVEX_W_0F3813_P_1,
  EVEX_W_0F3813_P_2,
  EVEX_W_0F3814_P_1,
  EVEX_W_0F3815_P_1,
  EVEX_W_0F3818_P_2,
  EVEX_W_0F3819_P_2,
  EVEX_W_0F381A_P_2,
  EVEX_W_0F381B_P_2,
  EVEX_W_0F381E_P_2,
  EVEX_W_0F381F_P_2,
  EVEX_W_0F3820_P_1,
  EVEX_W_0F3821_P_1,
  EVEX_W_0F3822_P_1,
  EVEX_W_0F3823_P_1,
  EVEX_W_0F3824_P_1,
  EVEX_W_0F3825_P_1,
  EVEX_W_0F3825_P_2,
  EVEX_W_0F3826_P_1,
  EVEX_W_0F3826_P_2,
  EVEX_W_0F3828_P_1,
  EVEX_W_0F3828_P_2,
  EVEX_W_0F3829_P_1,
  EVEX_W_0F3829_P_2,
  EVEX_W_0F382A_P_1,
  EVEX_W_0F382A_P_2,
  EVEX_W_0F382B_P_2,
  EVEX_W_0F3830_P_1,
  EVEX_W_0F3831_P_1,
  EVEX_W_0F3832_P_1,
  EVEX_W_0F3833_P_1,
  EVEX_W_0F3834_P_1,
  EVEX_W_0F3835_P_1,
  EVEX_W_0F3835_P_2,
  EVEX_W_0F3837_P_2,
  EVEX_W_0F3838_P_1,
  EVEX_W_0F3839_P_1,
  EVEX_W_0F383A_P_1,
  EVEX_W_0F3840_P_2,
  EVEX_W_0F3858_P_2,
  EVEX_W_0F3859_P_2,
  EVEX_W_0F385A_P_2,
  EVEX_W_0F385B_P_2,
  EVEX_W_0F3866_P_2,
  EVEX_W_0F3875_P_2,
  EVEX_W_0F3878_P_2,
  EVEX_W_0F3879_P_2,
  EVEX_W_0F387A_P_2,
  EVEX_W_0F387B_P_2,
  EVEX_W_0F387D_P_2,
  EVEX_W_0F3883_P_2,
  EVEX_W_0F388D_P_2,
  EVEX_W_0F3891_P_2,
  EVEX_W_0F3893_P_2,
  EVEX_W_0F38A1_P_2,
  EVEX_W_0F38A3_P_2,
  EVEX_W_0F38C7_R_1_P_2,
  EVEX_W_0F38C7_R_2_P_2,
  EVEX_W_0F38C7_R_5_P_2,
  EVEX_W_0F38C7_R_6_P_2,

  EVEX_W_0F3A00_P_2,
  EVEX_W_0F3A01_P_2,
  EVEX_W_0F3A04_P_2,
  EVEX_W_0F3A05_P_2,
  EVEX_W_0F3A08_P_2,
  EVEX_W_0F3A09_P_2,
  EVEX_W_0F3A0A_P_2,
  EVEX_W_0F3A0B_P_2,
  EVEX_W_0F3A16_P_2,
  EVEX_W_0F3A18_P_2,
  EVEX_W_0F3A19_P_2,
  EVEX_W_0F3A1A_P_2,
  EVEX_W_0F3A1B_P_2,
  EVEX_W_0F3A1D_P_2,
  EVEX_W_0F3A21_P_2,
  EVEX_W_0F3A22_P_2,
  EVEX_W_0F3A23_P_2,
  EVEX_W_0F3A38_P_2,
  EVEX_W_0F3A39_P_2,
  EVEX_W_0F3A3A_P_2,
  EVEX_W_0F3A3B_P_2,
  EVEX_W_0F3A3E_P_2,
  EVEX_W_0F3A3F_P_2,
  EVEX_W_0F3A42_P_2,
  EVEX_W_0F3A43_P_2,
  EVEX_W_0F3A50_P_2,
  EVEX_W_0F3A51_P_2,
  EVEX_W_0F3A56_P_2,
  EVEX_W_0F3A57_P_2,
  EVEX_W_0F3A66_P_2,
  EVEX_W_0F3A67_P_2
};

typedef void (*op_rtn) (int bytemode, int sizeflag);

struct dis386 {
  const char *name;
  struct
    {
      op_rtn rtn;
      int bytemode;
    } op[MAX_OPERANDS];
  unsigned int prefix_requirement;
};

/* Upper case letters in the instruction names here are macros.
   'A' => print 'b' if no register operands or suffix_always is true
   'B' => print 'b' if suffix_always is true
   'C' => print 's' or 'l' ('w' or 'd' in Intel mode) depending on operand
	  size prefix
   'D' => print 'w' if no register operands or 'w', 'l' or 'q', if
	  suffix_always is true
   'E' => print 'e' if 32-bit form of jcxz
   'F' => print 'w' or 'l' depending on address size prefix (loop insns)
   'G' => print 'w' or 'l' depending on operand size prefix (i/o insns)
   'H' => print ",pt" or ",pn" branch hint
   'I' => honor following macro letter even in Intel mode (implemented only
	  for some of the macro letters)
   'J' => print 'l'
   'K' => print 'd' or 'q' if rex prefix is present.
   'L' => print 'l' if suffix_always is true
   'M' => print 'r' if intel_mnemonic is false.
   'N' => print 'n' if instruction has no wait "prefix"
   'O' => print 'd' or 'o' (or 'q' in Intel mode)
   'P' => print 'w', 'l' or 'q' if instruction has an operand size prefix,
	  or suffix_always is true.  print 'q' if rex prefix is present.
   'Q' => print 'w', 'l' or 'q' for memory operand or suffix_always
	  is true
   'R' => print 'w', 'l' or 'q' ('d' for 'l' and 'e' in Intel mode)
   'S' => print 'w', 'l' or 'q' if suffix_always is true
   'T' => print 'q' in 64bit mode if instruction has no operand size
	  prefix and behave as 'P' otherwise
   'U' => print 'q' in 64bit mode if instruction has no operand size
	  prefix and behave as 'Q' otherwise
   'V' => print 'q' in 64bit mode if instruction has no operand size
	  prefix and behave as 'S' otherwise
   'W' => print 'b', 'w' or 'l' ('d' in Intel mode)
   'X' => print 's', 'd' depending on data16 prefix (for XMM)
   'Y' => 'q' if instruction has an REX 64bit overwrite prefix and
	  suffix_always is true.
   'Z' => print 'q' in 64bit mode and behave as 'L' otherwise
   '!' => change condition from true to false or from false to true.
   '%' => add 1 upper case letter to the macro.
   '^' => print 'w' or 'l' depending on operand size prefix or
	  suffix_always is true (lcall/ljmp).
   '@' => print 'q' for Intel64 ISA, 'w' or 'q' for AMD64 ISA depending
	  on operand size prefix.

   2 upper case letter macros:
   "XY" => print 'x' or 'y' if suffix_always is true or no register
	   operands and no broadcast.
   "XZ" => print 'x', 'y', or 'z' if suffix_always is true or no
	   register operands and no broadcast.
   "XW" => print 's', 'd' depending on the VEX.W bit (for FMA)
   "LQ" => print 'l' ('d' in Intel mode) or 'q' for memory operand
	   or suffix_always is true
   "LB" => print "abs" in 64bit mode and behave as 'B' otherwise
   "LS" => print "abs" in 64bit mode and behave as 'S' otherwise
   "LV" => print "abs" for 64bit operand and behave as 'S' otherwise
   "LW" => print 'd', 'q' depending on the VEX.W bit
   "LP" => print 'w' or 'l' ('d' in Intel mode) if instruction has
	   an operand size prefix, or suffix_always is true.  print
	   'q' if rex prefix is present.

   Many of the above letters print nothing in Intel mode.  See "putop"
   for the details.

   Braces '{' and '}', and vertical bars '|', indicate alternative
   mnemonic strings for AT&T and Intel.  */

static const struct dis386 dis386[] = {
  /* 00 */
  { "addB",		{ Ebh1, Gb }, 0 },
  { "addS",		{ Evh1, Gv }, 0 },
  { "addB",		{ Gb, EbS }, 0 },
  { "addS",		{ Gv, EvS }, 0 },
  { "addB",		{ AL, Ib }, 0 },
  { "addS",		{ eAX, Iv }, 0 },
  { X86_64_TABLE (X86_64_06) },
  { X86_64_TABLE (X86_64_07) },
  /* 08 */
  { "orB",		{ Ebh1, Gb }, 0 },
  { "orS",		{ Evh1, Gv }, 0 },
  { "orB",		{ Gb, EbS }, 0 },
  { "orS",		{ Gv, EvS }, 0 },
  { "orB",		{ AL, Ib }, 0 },
  { "orS",		{ eAX, Iv }, 0 },
  { X86_64_TABLE (X86_64_0D) },
  { Bad_Opcode },	/* 0x0f extended opcode escape */
  /* 10 */
  { "adcB",		{ Ebh1, Gb }, 0 },
  { "adcS",		{ Evh1, Gv }, 0 },
  { "adcB",		{ Gb, EbS }, 0 },
  { "adcS",		{ Gv, EvS }, 0 },
  { "adcB",		{ AL, Ib }, 0 },
  { "adcS",		{ eAX, Iv }, 0 },
  { X86_64_TABLE (X86_64_16) },
  { X86_64_TABLE (X86_64_17) },
  /* 18 */
  { "sbbB",		{ Ebh1, Gb }, 0 },
  { "sbbS",		{ Evh1, Gv }, 0 },
  { "sbbB",		{ Gb, EbS }, 0 },
  { "sbbS",		{ Gv, EvS }, 0 },
  { "sbbB",		{ AL, Ib }, 0 },
  { "sbbS",		{ eAX, Iv }, 0 },
  { X86_64_TABLE (X86_64_1E) },
  { X86_64_TABLE (X86_64_1F) },
  /* 20 */
  { "andB",		{ Ebh1, Gb }, 0 },
  { "andS",		{ Evh1, Gv }, 0 },
  { "andB",		{ Gb, EbS }, 0 },
  { "andS",		{ Gv, EvS }, 0 },
  { "andB",		{ AL, Ib }, 0 },
  { "andS",		{ eAX, Iv }, 0 },
  { Bad_Opcode },	/* SEG ES prefix */
  { X86_64_TABLE (X86_64_27) },
  /* 28 */
  { "subB",		{ Ebh1, Gb }, 0 },
  { "subS",		{ Evh1, Gv }, 0 },
  { "subB",		{ Gb, EbS }, 0 },
  { "subS",		{ Gv, EvS }, 0 },
  { "subB",		{ AL, Ib }, 0 },
  { "subS",		{ eAX, Iv }, 0 },
  { Bad_Opcode },	/* SEG CS prefix */
  { X86_64_TABLE (X86_64_2F) },
  /* 30 */
  { "xorB",		{ Ebh1, Gb }, 0 },
  { "xorS",		{ Evh1, Gv }, 0 },
  { "xorB",		{ Gb, EbS }, 0 },
  { "xorS",		{ Gv, EvS }, 0 },
  { "xorB",		{ AL, Ib }, 0 },
  { "xorS",		{ eAX, Iv }, 0 },
  { Bad_Opcode },	/* SEG SS prefix */
  { X86_64_TABLE (X86_64_37) },
  /* 38 */
  { "cmpB",		{ Eb, Gb }, 0 },
  { "cmpS",		{ Ev, Gv }, 0 },
  { "cmpB",		{ Gb, EbS }, 0 },
  { "cmpS",		{ Gv, EvS }, 0 },
  { "cmpB",		{ AL, Ib }, 0 },
  { "cmpS",		{ eAX, Iv }, 0 },
  { Bad_Opcode },	/* SEG DS prefix */
  { X86_64_TABLE (X86_64_3F) },
  /* 40 */
  { "inc{S|}",		{ RMeAX }, 0 },
  { "inc{S|}",		{ RMeCX }, 0 },
  { "inc{S|}",		{ RMeDX }, 0 },
  { "inc{S|}",		{ RMeBX }, 0 },
  { "inc{S|}",		{ RMeSP }, 0 },
  { "inc{S|}",		{ RMeBP }, 0 },
  { "inc{S|}",		{ RMeSI }, 0 },
  { "inc{S|}",		{ RMeDI }, 0 },
  /* 48 */
  { "dec{S|}",		{ RMeAX }, 0 },
  { "dec{S|}",		{ RMeCX }, 0 },
  { "dec{S|}",		{ RMeDX }, 0 },
  { "dec{S|}",		{ RMeBX }, 0 },
  { "dec{S|}",		{ RMeSP }, 0 },
  { "dec{S|}",		{ RMeBP }, 0 },
  { "dec{S|}",		{ RMeSI }, 0 },
  { "dec{S|}",		{ RMeDI }, 0 },
  /* 50 */
  { "pushV",		{ RMrAX }, 0 },
  { "pushV",		{ RMrCX }, 0 },
  { "pushV",		{ RMrDX }, 0 },
  { "pushV",		{ RMrBX }, 0 },
  { "pushV",		{ RMrSP }, 0 },
  { "pushV",		{ RMrBP }, 0 },
  { "pushV",		{ RMrSI }, 0 },
  { "pushV",		{ RMrDI }, 0 },
  /* 58 */
  { "popV",		{ RMrAX }, 0 },
  { "popV",		{ RMrCX }, 0 },
  { "popV",		{ RMrDX }, 0 },
  { "popV",		{ RMrBX }, 0 },
  { "popV",		{ RMrSP }, 0 },
  { "popV",		{ RMrBP }, 0 },
  { "popV",		{ RMrSI }, 0 },
  { "popV",		{ RMrDI }, 0 },
  /* 60 */
  { X86_64_TABLE (X86_64_60) },
  { X86_64_TABLE (X86_64_61) },
  { X86_64_TABLE (X86_64_62) },
  { X86_64_TABLE (X86_64_63) },
  { Bad_Opcode },	/* seg fs */
  { Bad_Opcode },	/* seg gs */
  { Bad_Opcode },	/* op size prefix */
  { Bad_Opcode },	/* adr size prefix */
  /* 68 */
  { "pushT",		{ sIv }, 0 },
  { "imulS",		{ Gv, Ev, Iv }, 0 },
  { "pushT",		{ sIbT }, 0 },
  { "imulS",		{ Gv, Ev, sIb }, 0 },
  { "ins{b|}",		{ Ybr, indirDX }, 0 },
  { X86_64_TABLE (X86_64_6D) },
  { "outs{b|}",		{ indirDXr, Xb }, 0 },
  { X86_64_TABLE (X86_64_6F) },
  /* 70 */
  { "joH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jnoH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jbH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jaeH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jeH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jneH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jbeH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jaH",		{ Jb, BND, cond_jump_flag }, 0 },
  /* 78 */
  { "jsH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jnsH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jpH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jnpH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jlH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jgeH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jleH",		{ Jb, BND, cond_jump_flag }, 0 },
  { "jgH",		{ Jb, BND, cond_jump_flag }, 0 },
  /* 80 */
  { REG_TABLE (REG_80) },
  { REG_TABLE (REG_81) },
  { Bad_Opcode },
  { REG_TABLE (REG_82) },
  { "testB",		{ Eb, Gb }, 0 },
  { "testS",		{ Ev, Gv }, 0 },
  { "xchgB",		{ Ebh2, Gb }, 0 },
  { "xchgS",		{ Evh2, Gv }, 0 },
  /* 88 */
  { "movB",		{ Ebh3, Gb }, 0 },
  { "movS",		{ Evh3, Gv }, 0 },
  { "movB",		{ Gb, EbS }, 0 },
  { "movS",		{ Gv, EvS }, 0 },
  { "movD",		{ Sv, Sw }, 0 },
  { MOD_TABLE (MOD_8D) },
  { "movD",		{ Sw, Sv }, 0 },
  { REG_TABLE (REG_8F) },
  /* 90 */
  { PREFIX_TABLE (PREFIX_90) },
  { "xchgS",		{ RMeCX, eAX }, 0 },
  { "xchgS",		{ RMeDX, eAX }, 0 },
  { "xchgS",		{ RMeBX, eAX }, 0 },
  { "xchgS",		{ RMeSP, eAX }, 0 },
  { "xchgS",		{ RMeBP, eAX }, 0 },
  { "xchgS",		{ RMeSI, eAX }, 0 },
  { "xchgS",		{ RMeDI, eAX }, 0 },
  /* 98 */
  { "cW{t|}R",		{ XX }, 0 },
  { "cR{t|}O",		{ XX }, 0 },
  { X86_64_TABLE (X86_64_9A) },
  { Bad_Opcode },	/* fwait */
  { "pushfT",		{ XX }, 0 },
  { "popfT",		{ XX }, 0 },
  { "sahf",		{ XX }, 0 },
  { "lahf",		{ XX }, 0 },
  /* a0 */
  { "mov%LB",		{ AL, Ob }, 0 },
  { "mov%LS",		{ eAX, Ov }, 0 },
  { "mov%LB",		{ Ob, AL }, 0 },
  { "mov%LS",		{ Ov, eAX }, 0 },
  { "movs{b|}",		{ Ybr, Xb }, 0 },
  { "movs{R|}",		{ Yvr, Xv }, 0 },
  { "cmps{b|}",		{ Xb, Yb }, 0 },
  { "cmps{R|}",		{ Xv, Yv }, 0 },
  /* a8 */
  { "testB",		{ AL, Ib }, 0 },
  { "testS",		{ eAX, Iv }, 0 },
  { "stosB",		{ Ybr, AL }, 0 },
  { "stosS",		{ Yvr, eAX }, 0 },
  { "lodsB",		{ ALr, Xb }, 0 },
  { "lodsS",		{ eAXr, Xv }, 0 },
  { "scasB",		{ AL, Yb }, 0 },
  { "scasS",		{ eAX, Yv }, 0 },
  /* b0 */
  { "movB",		{ RMAL, Ib }, 0 },
  { "movB",		{ RMCL, Ib }, 0 },
  { "movB",		{ RMDL, Ib }, 0 },
  { "movB",		{ RMBL, Ib }, 0 },
  { "movB",		{ RMAH, Ib }, 0 },
  { "movB",		{ RMCH, Ib }, 0 },
  { "movB",		{ RMDH, Ib }, 0 },
  { "movB",		{ RMBH, Ib }, 0 },
  /* b8 */
  { "mov%LV",		{ RMeAX, Iv64 }, 0 },
  { "mov%LV",		{ RMeCX, Iv64 }, 0 },
  { "mov%LV",		{ RMeDX, Iv64 }, 0 },
  { "mov%LV",		{ RMeBX, Iv64 }, 0 },
  { "mov%LV",		{ RMeSP, Iv64 }, 0 },
  { "mov%LV",		{ RMeBP, Iv64 }, 0 },
  { "mov%LV",		{ RMeSI, Iv64 }, 0 },
  { "mov%LV",		{ RMeDI, Iv64 }, 0 },
  /* c0 */
  { REG_TABLE (REG_C0) },
  { REG_TABLE (REG_C1) },
  { "retT",		{ Iw, BND }, 0 },
  { "retT",		{ BND }, 0 },
  { X86_64_TABLE (X86_64_C4) },
  { X86_64_TABLE (X86_64_C5) },
  { REG_TABLE (REG_C6) },
  { REG_TABLE (REG_C7) },
  /* c8 */
  { "enterT",		{ Iw, Ib }, 0 },
  { "leaveT",		{ XX }, 0 },
  { "Jret{|f}P",	{ Iw }, 0 },
  { "Jret{|f}P",	{ XX }, 0 },
  { "int3",		{ XX }, 0 },
  { "int",		{ Ib }, 0 },
  { X86_64_TABLE (X86_64_CE) },
  { "iret%LP",		{ XX }, 0 },
  /* d0 */
  { REG_TABLE (REG_D0) },
  { REG_TABLE (REG_D1) },
  { REG_TABLE (REG_D2) },
  { REG_TABLE (REG_D3) },
  { X86_64_TABLE (X86_64_D4) },
  { X86_64_TABLE (X86_64_D5) },
  { Bad_Opcode },
  { "xlat",		{ DSBX }, 0 },
  /* d8 */
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  /* e0 */
  { "loopneFH",		{ Jb, XX, loop_jcxz_flag }, 0 },
  { "loopeFH",		{ Jb, XX, loop_jcxz_flag }, 0 },
  { "loopFH",		{ Jb, XX, loop_jcxz_flag }, 0 },
  { "jEcxzH",		{ Jb, XX, loop_jcxz_flag }, 0 },
  { "inB",		{ AL, Ib }, 0 },
  { "inG",		{ zAX, Ib }, 0 },
  { "outB",		{ Ib, AL }, 0 },
  { "outG",		{ Ib, zAX }, 0 },
  /* e8 */
  { X86_64_TABLE (X86_64_E8) },
  { X86_64_TABLE (X86_64_E9) },
  { X86_64_TABLE (X86_64_EA) },
  { "jmp",		{ Jb, BND }, 0 },
  { "inB",		{ AL, indirDX }, 0 },
  { "inG",		{ zAX, indirDX }, 0 },
  { "outB",		{ indirDX, AL }, 0 },
  { "outG",		{ indirDX, zAX }, 0 },
  /* f0 */
  { Bad_Opcode },	/* lock prefix */
  { "icebp",		{ XX }, 0 },
  { Bad_Opcode },	/* repne */
  { Bad_Opcode },	/* repz */
  { "hlt",		{ XX }, 0 },
  { "cmc",		{ XX }, 0 },
  { REG_TABLE (REG_F6) },
  { REG_TABLE (REG_F7) },
  /* f8 */
  { "clc",		{ XX }, 0 },
  { "stc",		{ XX }, 0 },
  { "cli",		{ XX }, 0 },
  { "sti",		{ XX }, 0 },
  { "cld",		{ XX }, 0 },
  { "std",		{ XX }, 0 },
  { REG_TABLE (REG_FE) },
  { REG_TABLE (REG_FF) },
};

static const struct dis386 dis386_twobyte[] = {
  /* 00 */
  { REG_TABLE (REG_0F00 ) },
  { REG_TABLE (REG_0F01 ) },
  { "larS",		{ Gv, Ew }, 0 },
  { "lslS",		{ Gv, Ew }, 0 },
  { Bad_Opcode },
  { "syscall",		{ XX }, 0 },
  { "clts",		{ XX }, 0 },
  { "sysret%LP",		{ XX }, 0 },
  /* 08 */
  { "invd",		{ XX }, 0 },
  { "wbinvd",		{ XX }, 0 },
  { Bad_Opcode },
  { "ud2",		{ XX }, 0 },
  { Bad_Opcode },
  { REG_TABLE (REG_0F0D) },
  { "femms",		{ XX }, 0 },
  { "",			{ MX, EM, OPSUF }, 0 }, /* See OP_3DNowSuffix.  */
  /* 10 */
  { PREFIX_TABLE (PREFIX_0F10) },
  { PREFIX_TABLE (PREFIX_0F11) },
  { PREFIX_TABLE (PREFIX_0F12) },
  { MOD_TABLE (MOD_0F13) },
  { "unpcklpX",		{ XM, EXx }, PREFIX_OPCODE },
  { "unpckhpX",		{ XM, EXx }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0F16) },
  { MOD_TABLE (MOD_0F17) },
  /* 18 */
  { REG_TABLE (REG_0F18) },
  { "nopQ",		{ Ev }, 0 },
  { PREFIX_TABLE (PREFIX_0F1A) },
  { PREFIX_TABLE (PREFIX_0F1B) },
  { "nopQ",		{ Ev }, 0 },
  { "nopQ",		{ Ev }, 0 },
  { "nopQ",		{ Ev }, 0 },
  { "nopQ",		{ Ev }, 0 },
  /* 20 */
  { "movZ",		{ Rm, Cm }, 0 },
  { "movZ",		{ Rm, Dm }, 0 },
  { "movZ",		{ Cm, Rm }, 0 },
  { "movZ",		{ Dm, Rm }, 0 },
  { MOD_TABLE (MOD_0F24) },
  { Bad_Opcode },
  { MOD_TABLE (MOD_0F26) },
  { Bad_Opcode },
  /* 28 */
  { "movapX",		{ XM, EXx }, PREFIX_OPCODE },
  { "movapX",		{ EXxS, XM }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0F2A) },
  { PREFIX_TABLE (PREFIX_0F2B) },
  { PREFIX_TABLE (PREFIX_0F2C) },
  { PREFIX_TABLE (PREFIX_0F2D) },
  { PREFIX_TABLE (PREFIX_0F2E) },
  { PREFIX_TABLE (PREFIX_0F2F) },
  /* 30 */
  { "wrmsr",		{ XX }, 0 },
  { "rdtsc",		{ XX }, 0 },
  { "rdmsr",		{ XX }, 0 },
  { "rdpmc",		{ XX }, 0 },
  { "sysenter",		{ XX }, 0 },
  { "sysexit",		{ XX }, 0 },
  { Bad_Opcode },
  { "getsec",		{ XX }, 0 },
  /* 38 */
  { THREE_BYTE_TABLE_PREFIX (THREE_BYTE_0F38, PREFIX_OPCODE) },
  { Bad_Opcode },
  { THREE_BYTE_TABLE_PREFIX (THREE_BYTE_0F3A, PREFIX_OPCODE) },
  { Bad_Opcode },
  { Bad_Opcode },
  { Bad_Opcode },
  { Bad_Opcode },
  { Bad_Opcode },
  /* 40 */
  { "cmovoS",		{ Gv, Ev }, 0 },
  { "cmovnoS",		{ Gv, Ev }, 0 },
  { "cmovbS",		{ Gv, Ev }, 0 },
  { "cmovaeS",		{ Gv, Ev }, 0 },
  { "cmoveS",		{ Gv, Ev }, 0 },
  { "cmovneS",		{ Gv, Ev }, 0 },
  { "cmovbeS",		{ Gv, Ev }, 0 },
  { "cmovaS",		{ Gv, Ev }, 0 },
  /* 48 */
  { "cmovsS",		{ Gv, Ev }, 0 },
  { "cmovnsS",		{ Gv, Ev }, 0 },
  { "cmovpS",		{ Gv, Ev }, 0 },
  { "cmovnpS",		{ Gv, Ev }, 0 },
  { "cmovlS",		{ Gv, Ev }, 0 },
  { "cmovgeS",		{ Gv, Ev }, 0 },
  { "cmovleS",		{ Gv, Ev }, 0 },
  { "cmovgS",		{ Gv, Ev }, 0 },
  /* 50 */
  { MOD_TABLE (MOD_0F51) },
  { PREFIX_TABLE (PREFIX_0F51) },
  { PREFIX_TABLE (PREFIX_0F52) },
  { PREFIX_TABLE (PREFIX_0F53) },
  { "andpX",		{ XM, EXx }, PREFIX_OPCODE },
  { "andnpX",		{ XM, EXx }, PREFIX_OPCODE },
  { "orpX",		{ XM, EXx }, PREFIX_OPCODE },
  { "xorpX",		{ XM, EXx }, PREFIX_OPCODE },
  /* 58 */
  { PREFIX_TABLE (PREFIX_0F58) },
  { PREFIX_TABLE (PREFIX_0F59) },
  { PREFIX_TABLE (PREFIX_0F5A) },
  { PREFIX_TABLE (PREFIX_0F5B) },
  { PREFIX_TABLE (PREFIX_0F5C) },
  { PREFIX_TABLE (PREFIX_0F5D) },
  { PREFIX_TABLE (PREFIX_0F5E) },
  { PREFIX_TABLE (PREFIX_0F5F) },
  /* 60 */
  { PREFIX_TABLE (PREFIX_0F60) },
  { PREFIX_TABLE (PREFIX_0F61) },
  { PREFIX_TABLE (PREFIX_0F62) },
  { "packsswb",		{ MX, EM }, PREFIX_OPCODE },
  { "pcmpgtb",		{ MX, EM }, PREFIX_OPCODE },
  { "pcmpgtw",		{ MX, EM }, PREFIX_OPCODE },
  { "pcmpgtd",		{ MX, EM }, PREFIX_OPCODE },
  { "packuswb",		{ MX, EM }, PREFIX_OPCODE },
  /* 68 */
  { "punpckhbw",	{ MX, EM }, PREFIX_OPCODE },
  { "punpckhwd",	{ MX, EM }, PREFIX_OPCODE },
  { "punpckhdq",	{ MX, EM }, PREFIX_OPCODE },
  { "packssdw",		{ MX, EM }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0F6C) },
  { PREFIX_TABLE (PREFIX_0F6D) },
  { "movK",		{ MX, Edq }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0F6F) },
  /* 70 */
  { PREFIX_TABLE (PREFIX_0F70) },
  { REG_TABLE (REG_0F71) },
  { REG_TABLE (REG_0F72) },
  { REG_TABLE (REG_0F73) },
  { "pcmpeqb",		{ MX, EM }, PREFIX_OPCODE },
  { "pcmpeqw",		{ MX, EM }, PREFIX_OPCODE },
  { "pcmpeqd",		{ MX, EM }, PREFIX_OPCODE },
  { "emms",		{ XX }, PREFIX_OPCODE },
  /* 78 */
  { PREFIX_TABLE (PREFIX_0F78) },
  { PREFIX_TABLE (PREFIX_0F79) },
  { THREE_BYTE_TABLE (THREE_BYTE_0F7A) },
  { Bad_Opcode },
  { PREFIX_TABLE (PREFIX_0F7C) },
  { PREFIX_TABLE (PREFIX_0F7D) },
  { PREFIX_TABLE (PREFIX_0F7E) },
  { PREFIX_TABLE (PREFIX_0F7F) },
  /* 80 */
  { "joH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jnoH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jbH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jaeH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jeH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jneH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jbeH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jaH",		{ Jv, BND, cond_jump_flag }, 0 },
  /* 88 */
  { "jsH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jnsH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jpH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jnpH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jlH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jgeH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jleH",		{ Jv, BND, cond_jump_flag }, 0 },
  { "jgH",		{ Jv, BND, cond_jump_flag }, 0 },
  /* 90 */
  { "seto",		{ Eb }, 0 },
  { "setno",		{ Eb }, 0 },
  { "setb",		{ Eb }, 0 },
  { "setae",		{ Eb }, 0 },
  { "sete",		{ Eb }, 0 },
  { "setne",		{ Eb }, 0 },
  { "setbe",		{ Eb }, 0 },
  { "seta",		{ Eb }, 0 },
  /* 98 */
  { "sets",		{ Eb }, 0 },
  { "setns",		{ Eb }, 0 },
  { "setp",		{ Eb }, 0 },
  { "setnp",		{ Eb }, 0 },
  { "setl",		{ Eb }, 0 },
  { "setge",		{ Eb }, 0 },
  { "setle",		{ Eb }, 0 },
  { "setg",		{ Eb }, 0 },
  /* a0 */
  { "pushT",		{ fs }, 0 },
  { "popT",		{ fs }, 0 },
  { "cpuid",		{ XX }, 0 },
  { "btS",		{ Ev, Gv }, 0 },
  { "shldS",		{ Ev, Gv, Ib }, 0 },
  { "shldS",		{ Ev, Gv, CL }, 0 },
  { REG_TABLE (REG_0FA6) },
  { REG_TABLE (REG_0FA7) },
  /* a8 */
  { "pushT",		{ gs }, 0 },
  { "popT",		{ gs }, 0 },
  { "rsm",		{ XX }, 0 },
  { "btsS",		{ Evh1, Gv }, 0 },
  { "shrdS",		{ Ev, Gv, Ib }, 0 },
  { "shrdS",		{ Ev, Gv, CL }, 0 },
  { REG_TABLE (REG_0FAE) },
  { "imulS",		{ Gv, Ev }, 0 },
  /* b0 */
  { "cmpxchgB",		{ Ebh1, Gb }, 0 },
  { "cmpxchgS",		{ Evh1, Gv }, 0 },
  { MOD_TABLE (MOD_0FB2) },
  { "btrS",		{ Evh1, Gv }, 0 },
  { MOD_TABLE (MOD_0FB4) },
  { MOD_TABLE (MOD_0FB5) },
  { "movz{bR|x}",	{ Gv, Eb }, 0 },
  { "movz{wR|x}",	{ Gv, Ew }, 0 }, /* yes, there really is movzww ! */
  /* b8 */
  { PREFIX_TABLE (PREFIX_0FB8) },
  { "ud1",		{ XX }, 0 },
  { REG_TABLE (REG_0FBA) },
  { "btcS",		{ Evh1, Gv }, 0 },
  { PREFIX_TABLE (PREFIX_0FBC) },
  { PREFIX_TABLE (PREFIX_0FBD) },
  { "movs{bR|x}",	{ Gv, Eb }, 0 },
  { "movs{wR|x}",	{ Gv, Ew }, 0 }, /* yes, there really is movsww ! */
  /* c0 */
  { "xaddB",		{ Ebh1, Gb }, 0 },
  { "xaddS",		{ Evh1, Gv }, 0 },
  { PREFIX_TABLE (PREFIX_0FC2) },
  { PREFIX_TABLE (PREFIX_0FC3) },
  { "pinsrw",		{ MX, Edqw, Ib }, PREFIX_OPCODE },
  { "pextrw",		{ Gdq, MS, Ib }, PREFIX_OPCODE },
  { "shufpX",		{ XM, EXx, Ib }, PREFIX_OPCODE },
  { REG_TABLE (REG_0FC7) },
  /* c8 */
  { "bswap",		{ RMeAX }, 0 },
  { "bswap",		{ RMeCX }, 0 },
  { "bswap",		{ RMeDX }, 0 },
  { "bswap",		{ RMeBX }, 0 },
  { "bswap",		{ RMeSP }, 0 },
  { "bswap",		{ RMeBP }, 0 },
  { "bswap",		{ RMeSI }, 0 },
  { "bswap",		{ RMeDI }, 0 },
  /* d0 */
  { PREFIX_TABLE (PREFIX_0FD0) },
  { "psrlw",		{ MX, EM }, PREFIX_OPCODE },
  { "psrld",		{ MX, EM }, PREFIX_OPCODE },
  { "psrlq",		{ MX, EM }, PREFIX_OPCODE },
  { "paddq",		{ MX, EM }, PREFIX_OPCODE },
  { "pmullw",		{ MX, EM }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0FD6) },
  { MOD_TABLE (MOD_0FD7) },
  /* d8 */
  { "psubusb",		{ MX, EM }, PREFIX_OPCODE },
  { "psubusw",		{ MX, EM }, PREFIX_OPCODE },
  { "pminub",		{ MX, EM }, PREFIX_OPCODE },
  { "pand",		{ MX, EM }, PREFIX_OPCODE },
  { "paddusb",		{ MX, EM }, PREFIX_OPCODE },
  { "paddusw",		{ MX, EM }, PREFIX_OPCODE },
  { "pmaxub",		{ MX, EM }, PREFIX_OPCODE },
  { "pandn",		{ MX, EM }, PREFIX_OPCODE },
  /* e0 */
  { "pavgb",		{ MX, EM }, PREFIX_OPCODE },
  { "psraw",		{ MX, EM }, PREFIX_OPCODE },
  { "psrad",		{ MX, EM }, PREFIX_OPCODE },
  { "pavgw",		{ MX, EM }, PREFIX_OPCODE },
  { "pmulhuw",		{ MX, EM }, PREFIX_OPCODE },
  { "pmulhw",		{ MX, EM }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0FE6) },
  { PREFIX_TABLE (PREFIX_0FE7) },
  /* e8 */
  { "psubsb",		{ MX, EM }, PREFIX_OPCODE },
  { "psubsw",		{ MX, EM }, PREFIX_OPCODE },
  { "pminsw",		{ MX, EM }, PREFIX_OPCODE },
  { "por",		{ MX, EM }, PREFIX_OPCODE },
  { "paddsb",		{ MX, EM }, PREFIX_OPCODE },
  { "paddsw",		{ MX, EM }, PREFIX_OPCODE },
  { "pmaxsw",		{ MX, EM }, PREFIX_OPCODE },
  { "pxor",		{ MX, EM }, PREFIX_OPCODE },
  /* f0 */
  { PREFIX_TABLE (PREFIX_0FF0) },
  { "psllw",		{ MX, EM }, PREFIX_OPCODE },
  { "pslld",		{ MX, EM }, PREFIX_OPCODE },
  { "psllq",		{ MX, EM }, PREFIX_OPCODE },
  { "pmuludq",		{ MX, EM }, PREFIX_OPCODE },
  { "pmaddwd",		{ MX, EM }, PREFIX_OPCODE },
  { "psadbw",		{ MX, EM }, PREFIX_OPCODE },
  { PREFIX_TABLE (PREFIX_0FF7) },
  /* f8 */
  { "psubb",		{ MX, EM }, PREFIX_OPCODE },
  { "psubw",		{ MX, EM }, PREFIX_OPCODE },
  { "psubd",		{ MX, EM }, PREFIX_OPCODE },
  { "psubq",		{ MX, EM }, PREFIX_OPCODE },
  { "paddb",		{ MX, EM }, PREFIX_OPCODE },
  { "paddw",		{ MX, EM }, PREFIX_OPCODE },
  { "paddd",		{ MX, EM }, PREFIX_OPCODE },
  { Bad_Opcode },
};

static const unsigned char onebyte_has_modrm[256] = {
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
  /*       -------------------------------        */
  /* 00 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 00 */
  /* 10 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 10 */
  /* 20 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 20 */
  /* 30 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 30 */
  /* 40 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40 */
  /* 50 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
  /* 60 */ 0,0,1,1,0,0,0,0,0,1,0,1,0,0,0,0, /* 60 */
  /* 70 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
  /* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 80 */
  /* 90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 90 */
  /* a0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* a0 */
  /* b0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* b0 */
  /* c0 */ 1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* c0 */
  /* d0 */ 1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1, /* d0 */
  /* e0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* e0 */
  /* f0 */ 0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1  /* f0 */
  /*       -------------------------------        */
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
};

static const unsigned char twobyte_has_modrm[256] = {
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
  /*       -------------------------------        */
  /* 00 */ 1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,1, /* 0f */
  /* 10 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 1f */
  /* 20 */ 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1, /* 2f */
  /* 30 */ 0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0, /* 3f */
  /* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 4f */
  /* 50 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 5f */
  /* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 6f */
  /* 70 */ 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1, /* 7f */
  /* 80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 8f */
  /* 90 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 9f */
  /* a0 */ 0,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1, /* af */
  /* b0 */ 1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1, /* bf */
  /* c0 */ 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0, /* cf */
  /* d0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* df */
  /* e0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* ef */
  /* f0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0  /* ff */
  /*       -------------------------------        */
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
};

static char obuf[100];
static char *obufp;
static char *mnemonicendp;
static char scratchbuf[100];
static unsigned char *start_codep;
static unsigned char *insn_codep;
static unsigned char *codep;
static unsigned char *end_codep;
static int last_lock_prefix;
static int last_repz_prefix;
static int last_repnz_prefix;
static int last_data_prefix;
static int last_addr_prefix;
static int last_rex_prefix;
static int last_seg_prefix;
static int fwait_prefix;
/* The active segment register prefix.  */
static int active_seg_prefix;
#define MAX_CODE_LENGTH 15
/* We can up to 14 prefixes since the maximum instruction length is
   15bytes.  */
static int all_prefixes[MAX_CODE_LENGTH - 1];
static disassemble_info *the_info;
static struct
  {
    int mod;
    int reg;
    int rm;
  }
modrm;
static unsigned char need_modrm;
static struct
  {
    int scale;
    int index;
    int base;
  }
sib;
static struct
  {
    int register_specifier;
    int length;
    int prefix;
    int w;
    int evex;
    int r;
    int v;
    int mask_register_specifier;
    int zeroing;
    int ll;
    int b;
  }
vex;
static unsigned char need_vex;
static unsigned char need_vex_reg;
static unsigned char vex_w_done;

struct op
  {
    const char *name;
    unsigned int len;
  };

/* If we are accessing mod/rm/reg without need_modrm set, then the
   values are stale.  Hitting this abort likely indicates that you
   need to update onebyte_has_modrm or twobyte_has_modrm.  */
#define MODRM_CHECK  if (!need_modrm) abort ()

static const char **names64;
static const char **names32;
static const char **names16;
static const char **names8;
static const char **names8rex;
static const char **names_seg;
static const char *index64;
static const char *index32;
static const char **index16;
static const char **names_bnd;

static const char *intel_names64[] = {
  "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};
static const char *intel_names32[] = {
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};
static const char *intel_names16[] = {
  "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};
static const char *intel_names8[] = {
  "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
};
static const char *intel_names8rex[] = {
  "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
  "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
};
static const char *intel_names_seg[] = {
  "es", "cs", "ss", "ds", "fs", "gs", "?", "?",
};
static const char *intel_index64 = "riz";
static const char *intel_index32 = "eiz";
static const char *intel_index16[] = {
  "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};

static const char *att_names64[] = {
  "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
  "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
};
static const char *att_names32[] = {
  "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
  "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"
};
static const char *att_names16[] = {
  "%ax", "%cx", "%dx", "%bx", "%sp", "%bp", "%si", "%di",
  "%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w"
};
static const char *att_names8[] = {
  "%al", "%cl", "%dl", "%bl", "%ah", "%ch", "%dh", "%bh",
};
static const char *att_names8rex[] = {
  "%al", "%cl", "%dl", "%bl", "%spl", "%bpl", "%sil", "%dil",
  "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"
};
static const char *att_names_seg[] = {
  "%es", "%cs", "%ss", "%ds", "%fs", "%gs", "%?", "%?",
};
static const char *att_index64 = "%riz";
static const char *att_index32 = "%eiz";
static const char *att_index16[] = {
  "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx"
};

static const char **names_mm;
static const char *intel_names_mm[] = {
  "mm0", "mm1", "mm2", "mm3",
  "mm4", "mm5", "mm6", "mm7"
};
static const char *att_names_mm[] = {
  "%mm0", "%mm1", "%mm2", "%mm3",
  "%mm4", "%mm5", "%mm6", "%mm7"
};

static const char *intel_names_bnd[] = {
  "bnd0", "bnd1", "bnd2", "bnd3"
};

static const char *att_names_bnd[] = {
  "%bnd0", "%bnd1", "%bnd2", "%bnd3"
};

static const char **names_xmm;
static const char *intel_names_xmm[] = {
  "xmm0", "xmm1", "xmm2", "xmm3",
  "xmm4", "xmm5", "xmm6", "xmm7",
  "xmm8", "xmm9", "xmm10", "xmm11",
  "xmm12", "xmm13", "xmm14", "xmm15",
  "xmm16", "xmm17", "xmm18", "xmm19",
  "xmm20", "xmm21", "xmm22", "xmm23",
  "xmm24", "xmm25", "xmm26", "xmm27",
  "xmm28", "xmm29", "xmm30", "xmm31"
};
static const char *att_names_xmm[] = {
  "%xmm0", "%xmm1", "%xmm2", "%xmm3",
  "%xmm4", "%xmm5", "%xmm6", "%xmm7",
  "%xmm8", "%xmm9", "%xmm10", "%xmm11",
  "%xmm12", "%xmm13", "%xmm14", "%xmm15",
  "%xmm16", "%xmm17", "%xmm18", "%xmm19",
  "%xmm20", "%xmm21", "%xmm22", "%xmm23",
  "%xmm24", "%xmm25", "%xmm26", "%xmm27",
  "%xmm28", "%xmm29", "%xmm30", "%xmm31"
};

static const char **names_ymm;
static const char *intel_names_ymm[] = {
  "ymm0", "ymm1", "ymm2", "ymm3",
  "ymm4", "ymm5", "ymm6", "ymm7",
  "ymm8", "ymm9", "ymm10", "ymm11",
  "ymm12", "ymm13", "ymm14", "ymm15",
  "ymm16", "ymm17", "ymm18", "ymm19",
  "ymm20", "ymm21", "ymm22", "ymm23",
  "ymm24", "ymm25", "ymm26", "ymm27",
  "ymm28", "ymm29", "ymm30", "ymm31"
};
static const char *att_names_ymm[] = {
  "%ymm0", "%ymm1", "%ymm2", "%ymm3",
  "%ymm4", "%ymm5", "%ymm6", "%ymm7",
  "%ymm8", "%ymm9", "%ymm10", "%ymm11",
  "%ymm12", "%ymm13", "%ymm14", "%ymm15",
  "%ymm16", "%ymm17", "%ymm18", "%ymm19",
  "%ymm20", "%ymm21", "%ymm22", "%ymm23",
  "%ymm24", "%ymm25", "%ymm26", "%ymm27",
  "%ymm28", "%ymm29", "%ymm30", "%ymm31"
};

static const char **names_zmm;
static const char *intel_names_zmm[] = {
  "zmm0", "zmm1", "zmm2", "zmm3",
  "zmm4", "zmm5", "zmm6", "zmm7",
  "zmm8", "zmm9", "zmm10", "zmm11",
  "zmm12", "zmm13", "zmm14", "zmm15",
  "zmm16", "zmm17", "zmm18", "zmm19",
  "zmm20", "zmm21", "zmm22", "zmm23",
  "zmm24", "zmm25", "zmm26", "zmm27",
  "zmm28", "zmm29", "zmm30", "zmm31"
};
static const char *att_names_zmm[] = {
  "%zmm0", "%zmm1", "%zmm2", "%zmm3",
  "%zmm4", "%zmm5", "%zmm6", "%zmm7",
  "%zmm8", "%zmm9", "%zmm10", "%zmm11",
  "%zmm12", "%zmm13", "%zmm14", "%zmm15",
  "%zmm16", "%zmm17", "%zmm18", "%zmm19",
  "%zmm20", "%zmm21", "%zmm22", "%zmm23",
  "%zmm24", "%zmm25", "%zmm26", "%zmm27",
  "%zmm28", "%zmm29", "%zmm30", "%zmm31"
};

static const char **names_mask;
static const char *intel_names_mask[] = {
  "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7"
};
static const char *att_names_mask[] = {
  "%k0", "%k1", "%k2", "%k3", "%k4", "%k5", "%k6", "%k7"
};

static const char *names_rounding[] =
{
  "{rn-sae}",
  "{rd-sae}",
  "{ru-sae}",
  "{rz-sae}"
};

static const struct dis386 reg_table[][8] = {
  /* REG_80 */
  {
    { "addA",	{ Ebh1, Ib }, 0 },
    { "orA",	{ Ebh1, Ib }, 0 },
    { "adcA",	{ Ebh1, Ib }, 0 },
    { "sbbA",	{ Ebh1, Ib }, 0 },
    { "andA",	{ Ebh1, Ib }, 0 },
    { "subA",	{ Ebh1, Ib }, 0 },
    { "xorA",	{ Ebh1, Ib }, 0 },
    { "cmpA",	{ Eb, Ib }, 0 },
  },
  /* REG_81 */
  {
    { "addQ",	{ Evh1, Iv }, 0 },
    { "orQ",	{ Evh1, Iv }, 0 },
    { "adcQ",	{ Evh1, Iv }, 0 },
    { "sbbQ",	{ Evh1, Iv }, 0 },
    { "andQ",	{ Evh1, Iv }, 0 },
    { "subQ",	{ Evh1, Iv }, 0 },
    { "xorQ",	{ Evh1, Iv }, 0 },
    { "cmpQ",	{ Ev, Iv }, 0 },
  },
  /* REG_82 */
  {
    { "addQ",	{ Evh1, sIb }, 0 },
    { "orQ",	{ Evh1, sIb }, 0 },
    { "adcQ",	{ Evh1, sIb }, 0 },
    { "sbbQ",	{ Evh1, sIb }, 0 },
    { "andQ",	{ Evh1, sIb }, 0 },
    { "subQ",	{ Evh1, sIb }, 0 },
    { "xorQ",	{ Evh1, sIb }, 0 },
    { "cmpQ",	{ Ev, sIb }, 0 },
  },
  /* REG_8F */
  {
    { "popU",	{ stackEv }, 0 },
    { XOP_8F_TABLE (XOP_09) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { XOP_8F_TABLE (XOP_09) },
  },
  /* REG_C0 */
  {
    { "rolA",	{ Eb, Ib }, 0 },
    { "rorA",	{ Eb, Ib }, 0 },
    { "rclA",	{ Eb, Ib }, 0 },
    { "rcrA",	{ Eb, Ib }, 0 },
    { "shlA",	{ Eb, Ib }, 0 },
    { "shrA",	{ Eb, Ib }, 0 },
    { Bad_Opcode },
    { "sarA",	{ Eb, Ib }, 0 },
  },
  /* REG_C1 */
  {
    { "rolQ",	{ Ev, Ib }, 0 },
    { "rorQ",	{ Ev, Ib }, 0 },
    { "rclQ",	{ Ev, Ib }, 0 },
    { "rcrQ",	{ Ev, Ib }, 0 },
    { "shlQ",	{ Ev, Ib }, 0 },
    { "shrQ",	{ Ev, Ib }, 0 },
    { Bad_Opcode },
    { "sarQ",	{ Ev, Ib }, 0 },
  },
  /* REG_C6 */
  {
    { "movA",	{ Ebh3, Ib }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_C6_REG_7) },
  },
  /* REG_C7 */
  {
    { "movQ",	{ Evh3, Iv }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_C7_REG_7) },
  },
  /* REG_D0 */
  {
    { "rolA",	{ Eb, I1 }, 0 },
    { "rorA",	{ Eb, I1 }, 0 },
    { "rclA",	{ Eb, I1 }, 0 },
    { "rcrA",	{ Eb, I1 }, 0 },
    { "shlA",	{ Eb, I1 }, 0 },
    { "shrA",	{ Eb, I1 }, 0 },
    { Bad_Opcode },
    { "sarA",	{ Eb, I1 }, 0 },
  },
  /* REG_D1 */
  {
    { "rolQ",	{ Ev, I1 }, 0 },
    { "rorQ",	{ Ev, I1 }, 0 },
    { "rclQ",	{ Ev, I1 }, 0 },
    { "rcrQ",	{ Ev, I1 }, 0 },
    { "shlQ",	{ Ev, I1 }, 0 },
    { "shrQ",	{ Ev, I1 }, 0 },
    { Bad_Opcode },
    { "sarQ",	{ Ev, I1 }, 0 },
  },
  /* REG_D2 */
  {
    { "rolA",	{ Eb, CL }, 0 },
    { "rorA",	{ Eb, CL }, 0 },
    { "rclA",	{ Eb, CL }, 0 },
    { "rcrA",	{ Eb, CL }, 0 },
    { "shlA",	{ Eb, CL }, 0 },
    { "shrA",	{ Eb, CL }, 0 },
    { Bad_Opcode },
    { "sarA",	{ Eb, CL }, 0 },
  },
  /* REG_D3 */
  {
    { "rolQ",	{ Ev, CL }, 0 },
    { "rorQ",	{ Ev, CL }, 0 },
    { "rclQ",	{ Ev, CL }, 0 },
    { "rcrQ",	{ Ev, CL }, 0 },
    { "shlQ",	{ Ev, CL }, 0 },
    { "shrQ",	{ Ev, CL }, 0 },
    { Bad_Opcode },
    { "sarQ",	{ Ev, CL }, 0 },
  },
  /* REG_F6 */
  {
    { "testA",	{ Eb, Ib }, 0 },
    { Bad_Opcode },
    { "notA",	{ Ebh1 }, 0 },
    { "negA",	{ Ebh1 }, 0 },
    { "mulA",	{ Eb }, 0 },	/* Don't print the implicit %al register,  */
    { "imulA",	{ Eb }, 0 },	/* to distinguish these opcodes from other */
    { "divA",	{ Eb }, 0 },	/* mul/imul opcodes.  Do the same for div  */
    { "idivA",	{ Eb }, 0 },	/* and idiv for consistency.		   */
  },
  /* REG_F7 */
  {
    { "testQ",	{ Ev, Iv }, 0 },
    { Bad_Opcode },
    { "notQ",	{ Evh1 }, 0 },
    { "negQ",	{ Evh1 }, 0 },
    { "mulQ",	{ Ev }, 0 },	/* Don't print the implicit register.  */
    { "imulQ",	{ Ev }, 0 },
    { "divQ",	{ Ev }, 0 },
    { "idivQ",	{ Ev }, 0 },
  },
  /* REG_FE */
  {
    { "incA",	{ Ebh1 }, 0 },
    { "decA",	{ Ebh1 }, 0 },
  },
  /* REG_FF */
  {
    { "incQ",	{ Evh1 }, 0 },
    { "decQ",	{ Evh1 }, 0 },
    { "call{T|}", { indirEv, BND }, 0 },
    { MOD_TABLE (MOD_FF_REG_3) },
    { "jmp{T|}", { indirEv, BND }, 0 },
    { MOD_TABLE (MOD_FF_REG_5) },
    { "pushU",	{ stackEv }, 0 },
    { Bad_Opcode },
  },
  /* REG_0F00 */
  {
    { "sldtD",	{ Sv }, 0 },
    { "strD",	{ Sv }, 0 },
    { "lldt",	{ Ew }, 0 },
    { "ltr",	{ Ew }, 0 },
    { "verr",	{ Ew }, 0 },
    { "verw",	{ Ew }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* REG_0F01 */
  {
    { MOD_TABLE (MOD_0F01_REG_0) },
    { MOD_TABLE (MOD_0F01_REG_1) },
    { MOD_TABLE (MOD_0F01_REG_2) },
    { MOD_TABLE (MOD_0F01_REG_3) },
    { "smswD",	{ Sv }, 0 },
    { Bad_Opcode },
    { "lmsw",	{ Ew }, 0 },
    { MOD_TABLE (MOD_0F01_REG_7) },
  },
  /* REG_0F0D */
  {
    { "prefetch",	{ Mb }, 0 },
    { "prefetchw",	{ Mb }, 0 },
    { "prefetchwt1",	{ Mb }, 0 },
    { "prefetch",	{ Mb }, 0 },
    { "prefetch",	{ Mb }, 0 },
    { "prefetch",	{ Mb }, 0 },
    { "prefetch",	{ Mb }, 0 },
    { "prefetch",	{ Mb }, 0 },
  },
  /* REG_0F18 */
  {
    { MOD_TABLE (MOD_0F18_REG_0) },
    { MOD_TABLE (MOD_0F18_REG_1) },
    { MOD_TABLE (MOD_0F18_REG_2) },
    { MOD_TABLE (MOD_0F18_REG_3) },
    { MOD_TABLE (MOD_0F18_REG_4) },
    { MOD_TABLE (MOD_0F18_REG_5) },
    { MOD_TABLE (MOD_0F18_REG_6) },
    { MOD_TABLE (MOD_0F18_REG_7) },
  },
  /* REG_0F71 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F71_REG_2) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F71_REG_4) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F71_REG_6) },
  },
  /* REG_0F72 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F72_REG_2) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F72_REG_4) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F72_REG_6) },
  },
  /* REG_0F73 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F73_REG_2) },
    { MOD_TABLE (MOD_0F73_REG_3) },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F73_REG_6) },
    { MOD_TABLE (MOD_0F73_REG_7) },
  },
  /* REG_0FA6 */
  {
    { "montmul",	{ { OP_0f07, 0 } }, 0 },
    { "xsha1",		{ { OP_0f07, 0 } }, 0 },
    { "xsha256",	{ { OP_0f07, 0 } }, 0 },
  },
  /* REG_0FA7 */
  {
    { "xstore-rng",	{ { OP_0f07, 0 } }, 0 },
    { "xcrypt-ecb",	{ { OP_0f07, 0 } }, 0 },
    { "xcrypt-cbc",	{ { OP_0f07, 0 } }, 0 },
    { "xcrypt-ctr",	{ { OP_0f07, 0 } }, 0 },
    { "xcrypt-cfb",	{ { OP_0f07, 0 } }, 0 },
    { "xcrypt-ofb",	{ { OP_0f07, 0 } }, 0 },
  },
  /* REG_0FAE */
  {
    { MOD_TABLE (MOD_0FAE_REG_0) },
    { MOD_TABLE (MOD_0FAE_REG_1) },
    { MOD_TABLE (MOD_0FAE_REG_2) },
    { MOD_TABLE (MOD_0FAE_REG_3) },
    { MOD_TABLE (MOD_0FAE_REG_4) },
    { MOD_TABLE (MOD_0FAE_REG_5) },
    { MOD_TABLE (MOD_0FAE_REG_6) },
    { MOD_TABLE (MOD_0FAE_REG_7) },
  },
  /* REG_0FBA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "btQ",	{ Ev, Ib }, 0 },
    { "btsQ",	{ Evh1, Ib }, 0 },
    { "btrQ",	{ Evh1, Ib }, 0 },
    { "btcQ",	{ Evh1, Ib }, 0 },
  },
  /* REG_0FC7 */
  {
    { Bad_Opcode },
    { "cmpxchg8b", { { CMPXCHG8B_Fixup, q_mode } }, 0 },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0FC7_REG_3) },
    { MOD_TABLE (MOD_0FC7_REG_4) },
    { MOD_TABLE (MOD_0FC7_REG_5) },
    { MOD_TABLE (MOD_0FC7_REG_6) },
    { MOD_TABLE (MOD_0FC7_REG_7) },
  },
  /* REG_VEX_0F71 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F71_REG_2) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F71_REG_4) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F71_REG_6) },
  },
  /* REG_VEX_0F72 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F72_REG_2) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F72_REG_4) },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F72_REG_6) },
  },
  /* REG_VEX_0F73 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F73_REG_2) },
    { MOD_TABLE (MOD_VEX_0F73_REG_3) },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F73_REG_6) },
    { MOD_TABLE (MOD_VEX_0F73_REG_7) },
  },
  /* REG_VEX_0FAE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0FAE_REG_2) },
    { MOD_TABLE (MOD_VEX_0FAE_REG_3) },
  },
  /* REG_VEX_0F38F3 */
  {
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38F3_REG_1) },
    { PREFIX_TABLE (PREFIX_VEX_0F38F3_REG_2) },
    { PREFIX_TABLE (PREFIX_VEX_0F38F3_REG_3) },
  },
  /* REG_XOP_LWPCB */
  {
    { "llwpcb", { { OP_LWPCB_E, 0 } }, 0 },
    { "slwpcb",	{ { OP_LWPCB_E, 0 } }, 0 },
  },
  /* REG_XOP_LWP */
  {
    { "lwpins", { { OP_LWP_E, 0 }, Ed, Iq }, 0 },
    { "lwpval",	{ { OP_LWP_E, 0 }, Ed, Iq }, 0 },
  },
  /* REG_XOP_TBM_01 */
  {
    { Bad_Opcode },
    { "blcfill",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "blsfill",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "blcs",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "tzmsk",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "blcic",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "blsic",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { "t1mskc",	{ { OP_LWP_E, 0 }, Ev }, 0 },
  },
  /* REG_XOP_TBM_02 */
  {
    { Bad_Opcode },
    { "blcmsk",	{ { OP_LWP_E, 0 }, Ev }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "blci",	{ { OP_LWP_E, 0 }, Ev }, 0 },
  },
#define NEED_REG_TABLE
#include "i386-dis-evex.h"
#undef NEED_REG_TABLE
};

static const struct dis386 prefix_table[][4] = {
  /* PREFIX_90 */
  {
    { "xchgS", { { NOP_Fixup1, eAX_reg }, { NOP_Fixup2, eAX_reg } }, 0 },
    { "pause", { XX }, 0 },
    { "xchgS", { { NOP_Fixup1, eAX_reg }, { NOP_Fixup2, eAX_reg } }, 0 },
    { NULL, { { NULL, 0 } }, PREFIX_IGNORED }
  },

  /* PREFIX_0F10 */
  {
    { "movups",	{ XM, EXx }, PREFIX_OPCODE },
    { "movss",	{ XM, EXd }, PREFIX_OPCODE },
    { "movupd",	{ XM, EXx }, PREFIX_OPCODE },
    { "movsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F11 */
  {
    { "movups",	{ EXxS, XM }, PREFIX_OPCODE },
    { "movss",	{ EXdS, XM }, PREFIX_OPCODE },
    { "movupd",	{ EXxS, XM }, PREFIX_OPCODE },
    { "movsd",	{ EXqS, XM }, PREFIX_OPCODE },
  },

  /* PREFIX_0F12 */
  {
    { MOD_TABLE (MOD_0F12_PREFIX_0) },
    { "movsldup", { XM, EXx }, PREFIX_OPCODE },
    { "movlpd",	{ XM, EXq }, PREFIX_OPCODE },
    { "movddup", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F16 */
  {
    { MOD_TABLE (MOD_0F16_PREFIX_0) },
    { "movshdup", { XM, EXx }, PREFIX_OPCODE },
    { "movhpd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F1A */
  {
    { MOD_TABLE (MOD_0F1A_PREFIX_0) },
    { "bndcl",  { Gbnd, Ev_bnd }, 0 },
    { "bndmov", { Gbnd, Ebnd }, 0 },
    { "bndcu",  { Gbnd, Ev_bnd }, 0 },
  },

  /* PREFIX_0F1B */
  {
    { MOD_TABLE (MOD_0F1B_PREFIX_0) },
    { MOD_TABLE (MOD_0F1B_PREFIX_1) },
    { "bndmov", { Ebnd, Gbnd }, 0 },
    { "bndcn",  { Gbnd, Ev_bnd }, 0 },
  },

  /* PREFIX_0F2A */
  {
    { "cvtpi2ps", { XM, EMCq }, PREFIX_OPCODE },
    { "cvtsi2ss%LQ", { XM, Ev }, PREFIX_OPCODE },
    { "cvtpi2pd", { XM, EMCq }, PREFIX_OPCODE },
    { "cvtsi2sd%LQ", { XM, Ev }, 0 },
  },

  /* PREFIX_0F2B */
  {
    { MOD_TABLE (MOD_0F2B_PREFIX_0) },
    { MOD_TABLE (MOD_0F2B_PREFIX_1) },
    { MOD_TABLE (MOD_0F2B_PREFIX_2) },
    { MOD_TABLE (MOD_0F2B_PREFIX_3) },
  },

  /* PREFIX_0F2C */
  {
    { "cvttps2pi", { MXC, EXq }, PREFIX_OPCODE },
    { "cvttss2siY", { Gv, EXd }, PREFIX_OPCODE },
    { "cvttpd2pi", { MXC, EXx }, PREFIX_OPCODE },
    { "cvttsd2siY", { Gv, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F2D */
  {
    { "cvtps2pi", { MXC, EXq }, PREFIX_OPCODE },
    { "cvtss2siY", { Gv, EXd }, PREFIX_OPCODE },
    { "cvtpd2pi", { MXC, EXx }, PREFIX_OPCODE },
    { "cvtsd2siY", { Gv, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F2E */
  {
    { "ucomiss",{ XM, EXd }, 0 },
    { Bad_Opcode },
    { "ucomisd",{ XM, EXq }, 0 },
  },

  /* PREFIX_0F2F */
  {
    { "comiss",	{ XM, EXd }, 0 },
    { Bad_Opcode },
    { "comisd",	{ XM, EXq }, 0 },
  },

  /* PREFIX_0F51 */
  {
    { "sqrtps", { XM, EXx }, PREFIX_OPCODE },
    { "sqrtss", { XM, EXd }, PREFIX_OPCODE },
    { "sqrtpd", { XM, EXx }, PREFIX_OPCODE },
    { "sqrtsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F52 */
  {
    { "rsqrtps",{ XM, EXx }, PREFIX_OPCODE },
    { "rsqrtss",{ XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F53 */
  {
    { "rcpps",	{ XM, EXx }, PREFIX_OPCODE },
    { "rcpss",	{ XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F58 */
  {
    { "addps", { XM, EXx }, PREFIX_OPCODE },
    { "addss", { XM, EXd }, PREFIX_OPCODE },
    { "addpd", { XM, EXx }, PREFIX_OPCODE },
    { "addsd", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F59 */
  {
    { "mulps",	{ XM, EXx }, PREFIX_OPCODE },
    { "mulss",	{ XM, EXd }, PREFIX_OPCODE },
    { "mulpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "mulsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5A */
  {
    { "cvtps2pd", { XM, EXq }, PREFIX_OPCODE },
    { "cvtss2sd", { XM, EXd }, PREFIX_OPCODE },
    { "cvtpd2ps", { XM, EXx }, PREFIX_OPCODE },
    { "cvtsd2ss", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5B */
  {
    { "cvtdq2ps", { XM, EXx }, PREFIX_OPCODE },
    { "cvttps2dq", { XM, EXx }, PREFIX_OPCODE },
    { "cvtps2dq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5C */
  {
    { "subps",	{ XM, EXx }, PREFIX_OPCODE },
    { "subss",	{ XM, EXd }, PREFIX_OPCODE },
    { "subpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "subsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5D */
  {
    { "minps",	{ XM, EXx }, PREFIX_OPCODE },
    { "minss",	{ XM, EXd }, PREFIX_OPCODE },
    { "minpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "minsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5E */
  {
    { "divps",	{ XM, EXx }, PREFIX_OPCODE },
    { "divss",	{ XM, EXd }, PREFIX_OPCODE },
    { "divpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "divsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F5F */
  {
    { "maxps",	{ XM, EXx }, PREFIX_OPCODE },
    { "maxss",	{ XM, EXd }, PREFIX_OPCODE },
    { "maxpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "maxsd",	{ XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F60 */
  {
    { "punpcklbw",{ MX, EMd }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "punpcklbw",{ MX, EMx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F61 */
  {
    { "punpcklwd",{ MX, EMd }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "punpcklwd",{ MX, EMx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F62 */
  {
    { "punpckldq",{ MX, EMd }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "punpckldq",{ MX, EMx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F6C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "punpcklqdq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F6D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "punpckhqdq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F6F */
  {
    { "movq",	{ MX, EM }, PREFIX_OPCODE },
    { "movdqu",	{ XM, EXx }, PREFIX_OPCODE },
    { "movdqa",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F70 */
  {
    { "pshufw",	{ MX, EM, Ib }, PREFIX_OPCODE },
    { "pshufhw",{ XM, EXx, Ib }, PREFIX_OPCODE },
    { "pshufd",	{ XM, EXx, Ib }, PREFIX_OPCODE },
    { "pshuflw",{ XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F73_REG_3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "psrldq",	{ XS, Ib }, 0 },
  },

  /* PREFIX_0F73_REG_7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pslldq",	{ XS, Ib }, 0 },
  },

  /* PREFIX_0F78 */
  {
    {"vmread",	{ Em, Gm }, 0 },
    { Bad_Opcode },
    {"extrq",	{ XS, Ib, Ib }, 0 },
    {"insertq",	{ XM, XS, Ib, Ib }, 0 },
  },

  /* PREFIX_0F79 */
  {
    {"vmwrite",	{ Gm, Em }, 0 },
    { Bad_Opcode },
    {"extrq",	{ XM, XS }, 0 },
    {"insertq",	{ XM, XS }, 0 },
  },

  /* PREFIX_0F7C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "haddpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "haddps",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F7D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "hsubpd",	{ XM, EXx }, PREFIX_OPCODE },
    { "hsubps",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F7E */
  {
    { "movK",	{ Edq, MX }, PREFIX_OPCODE },
    { "movq",	{ XM, EXq }, PREFIX_OPCODE },
    { "movK",	{ Edq, XM }, PREFIX_OPCODE },
  },

  /* PREFIX_0F7F */
  {
    { "movq",	{ EMS, MX }, PREFIX_OPCODE },
    { "movdqu",	{ EXxS, XM }, PREFIX_OPCODE },
    { "movdqa",	{ EXxS, XM }, PREFIX_OPCODE },
  },

  /* PREFIX_0FAE_REG_0 */
  {
    { Bad_Opcode },
    { "rdfsbase", { Ev }, 0 },
  },

  /* PREFIX_0FAE_REG_1 */
  {
    { Bad_Opcode },
    { "rdgsbase", { Ev }, 0 },
  },

  /* PREFIX_0FAE_REG_2 */
  {
    { Bad_Opcode },
    { "wrfsbase", { Ev }, 0 },
  },

  /* PREFIX_0FAE_REG_3 */
  {
    { Bad_Opcode },
    { "wrgsbase", { Ev }, 0 },
  },

  /* PREFIX_0FAE_REG_6 */
  {
    { "xsaveopt",      { FXSAVE }, 0 },
    { Bad_Opcode },
    { "clwb",	{ Mb }, 0 },
  },

  /* PREFIX_0FAE_REG_7 */
  {
    { "clflush",	{ Mb }, 0 },
    { Bad_Opcode },
    { "clflushopt",	{ Mb }, 0 },
  },

  /* PREFIX_RM_0_0FAE_REG_7 */
  {
    { "sfence",		{ Skip_MODRM }, 0 },
    { Bad_Opcode },
    { "pcommit",		{ Skip_MODRM }, 0 },
  },

  /* PREFIX_0FB8 */
  {
    { Bad_Opcode },
    { "popcntS", { Gv, Ev }, 0 },
  },

  /* PREFIX_0FBC */
  {
    { "bsfS",	{ Gv, Ev }, 0 },
    { "tzcntS",	{ Gv, Ev }, 0 },
    { "bsfS",	{ Gv, Ev }, 0 },
  },

  /* PREFIX_0FBD */
  {
    { "bsrS",	{ Gv, Ev }, 0 },
    { "lzcntS",	{ Gv, Ev }, 0 },
    { "bsrS",	{ Gv, Ev }, 0 },
  },

  /* PREFIX_0FC2 */
  {
    { "cmpps",	{ XM, EXx, CMP }, PREFIX_OPCODE },
    { "cmpss",	{ XM, EXd, CMP }, PREFIX_OPCODE },
    { "cmppd",	{ XM, EXx, CMP }, PREFIX_OPCODE },
    { "cmpsd",	{ XM, EXq, CMP }, PREFIX_OPCODE },
  },

  /* PREFIX_0FC3 */
  {
    { "movntiS", { Ma, Gv }, PREFIX_OPCODE },
  },

  /* PREFIX_MOD_0_0FC7_REG_6 */
  {
    { "vmptrld",{ Mq }, 0 },
    { "vmxon",	{ Mq }, 0 },
    { "vmclear",{ Mq }, 0 },
  },

  /* PREFIX_MOD_3_0FC7_REG_6 */
  {
    { "rdrand",	{ Ev }, 0 },
    { Bad_Opcode },
    { "rdrand",	{ Ev }, 0 }
  },

  /* PREFIX_MOD_3_0FC7_REG_7 */
  {
    { "rdseed",	{ Ev }, 0 },
    { Bad_Opcode },
    { "rdseed",	{ Ev }, 0 },
  },

  /* PREFIX_0FD0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "addsubpd", { XM, EXx }, 0 },
    { "addsubps", { XM, EXx }, 0 },
  },

  /* PREFIX_0FD6 */
  {
    { Bad_Opcode },
    { "movq2dq",{ XM, MS }, 0 },
    { "movq",	{ EXqS, XM }, 0 },
    { "movdq2q",{ MX, XS }, 0 },
  },

  /* PREFIX_0FE6 */
  {
    { Bad_Opcode },
    { "cvtdq2pd", { XM, EXq }, PREFIX_OPCODE },
    { "cvttpd2dq", { XM, EXx }, PREFIX_OPCODE },
    { "cvtpd2dq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0FE7 */
  {
    { "movntq",	{ Mq, MX }, PREFIX_OPCODE },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0FE7_PREFIX_2) },
  },

  /* PREFIX_0FF0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0FF0_PREFIX_3) },
  },

  /* PREFIX_0FF7 */
  {
    { "maskmovq", { MX, MS }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "maskmovdqu", { XM, XS }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3810 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pblendvb", { XM, EXx, XMM0 }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3814 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "blendvps", { XM, EXx, XMM0 }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3815 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "blendvpd", { XM, EXx, XMM0 }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3817 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "ptest",  { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3820 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxbw", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3821 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxbd", { XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3822 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxbq", { XM, EXw }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3823 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxwd", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3824 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxwq", { XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3825 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovsxdq", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3828 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmuldq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3829 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpeqq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F382A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_0F382A_PREFIX_2) },
  },

  /* PREFIX_0F382B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "packusdw", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3830 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxbw", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3831 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxbd", { XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3832 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxbq", { XM, EXw }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3833 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxwd", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3834 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxwq", { XM, EXd }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3835 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmovzxdq", { XM, EXq }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3837 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpgtq", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3838 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pminsb",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3839 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pminsd",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pminuw",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pminud",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmaxsb",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmaxsd",	{ XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmaxuw", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F383F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmaxud", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3840 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pmulld", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3841 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "phminposuw", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3880 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "invept",	{ Gm, Mo }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3881 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "invvpid", { Gm, Mo }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3882 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "invpcid", { Gm, M }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38C8 */
  {
    { "sha1nexte", { XM, EXxmm }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38C9 */
  {
    { "sha1msg1", { XM, EXxmm }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38CA */
  {
    { "sha1msg2", { XM, EXxmm }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38CB */
  {
    { "sha256rnds2", { XM, EXxmm, XMM0 }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38CC */
  {
    { "sha256msg1", { XM, EXxmm }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38CD */
  {
    { "sha256msg2", { XM, EXxmm }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38DB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aesimc", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38DC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aesenc", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38DD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aesenclast", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38DE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aesdec", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38DF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aesdeclast", { XM, EXx }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38F0 */
  {
    { "movbeS",	{ Gv, { MOVBE_Fixup, v_mode } }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "movbeS",	{ Gv, { MOVBE_Fixup, v_mode } }, PREFIX_OPCODE },
    { "crc32",	{ Gdq, { CRC32_Fixup, b_mode } }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38F1 */
  {
    { "movbeS",	{ { MOVBE_Fixup, v_mode }, Gv }, PREFIX_OPCODE },
    { Bad_Opcode },
    { "movbeS",	{ { MOVBE_Fixup, v_mode }, Gv }, PREFIX_OPCODE },
    { "crc32",	{ Gdq, { CRC32_Fixup, v_mode } }, PREFIX_OPCODE },
  },

  /* PREFIX_0F38F6 */
  {
    { Bad_Opcode },
    { "adoxS",	{ Gdq, Edq}, PREFIX_OPCODE },
    { "adcxS",	{ Gdq, Edq}, PREFIX_OPCODE },
    { Bad_Opcode },
  },

  /* PREFIX_0F3A08 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "roundps", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A09 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "roundpd", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A0A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "roundss", { XM, EXd, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A0B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "roundsd", { XM, EXq, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A0C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "blendps", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A0D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "blendpd", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A0E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pblendw", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A14 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pextrb",	{ Edqb, XM, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A15 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pextrw",	{ Edqw, XM, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A16 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pextrK",	{ Edq, XM, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A17 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "extractps", { Edqd, XM, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A20 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pinsrb",	{ XM, Edqb, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A21 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "insertps", { XM, EXd, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A22 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pinsrK",	{ XM, Edq, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A40 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "dpps",	{ XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A41 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "dppd",	{ XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A42 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "mpsadbw", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A44 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pclmulqdq", { XM, EXx, PCLMUL }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A60 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpestrm", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A61 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpestri", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A62 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpistrm", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3A63 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "pcmpistri", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3ACC */
  {
    { "sha1rnds4", { XM, EXxmm, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_0F3ADF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "aeskeygenassist", { XM, EXx, Ib }, PREFIX_OPCODE },
  },

  /* PREFIX_VEX_0F10 */
  {
    { VEX_W_TABLE (VEX_W_0F10_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F10_P_1) },
    { VEX_W_TABLE (VEX_W_0F10_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F10_P_3) },
  },

  /* PREFIX_VEX_0F11 */
  {
    { VEX_W_TABLE (VEX_W_0F11_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F11_P_1) },
    { VEX_W_TABLE (VEX_W_0F11_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F11_P_3) },
  },

  /* PREFIX_VEX_0F12 */
  {
    { MOD_TABLE (MOD_VEX_0F12_PREFIX_0) },
    { VEX_W_TABLE (VEX_W_0F12_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_0F12_P_2) },
    { VEX_W_TABLE (VEX_W_0F12_P_3) },
  },

  /* PREFIX_VEX_0F16 */
  {
    { MOD_TABLE (MOD_VEX_0F16_PREFIX_0) },
    { VEX_W_TABLE (VEX_W_0F16_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_0F16_P_2) },
  },

  /* PREFIX_VEX_0F2A */
  {
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2A_P_1) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2A_P_3) },
  },

  /* PREFIX_VEX_0F2C */
  {
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2C_P_1) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2C_P_3) },
  },

  /* PREFIX_VEX_0F2D */
  {
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2D_P_1) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2D_P_3) },
  },

  /* PREFIX_VEX_0F2E */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F2E_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2E_P_2) },
  },

  /* PREFIX_VEX_0F2F */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F2F_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F2F_P_2) },
  },

  /* PREFIX_VEX_0F41 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F41_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F41_P_2) },
  },

  /* PREFIX_VEX_0F42 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F42_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F42_P_2) },
  },

  /* PREFIX_VEX_0F44 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F44_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F44_P_2) },
  },

  /* PREFIX_VEX_0F45 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F45_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F45_P_2) },
  },

  /* PREFIX_VEX_0F46 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F46_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F46_P_2) },
  },

  /* PREFIX_VEX_0F47 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F47_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F47_P_2) },
  },

  /* PREFIX_VEX_0F4A */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F4A_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F4A_P_2) },
  },

  /* PREFIX_VEX_0F4B */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F4B_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F4B_P_2) },
  },

  /* PREFIX_VEX_0F51 */
  {
    { VEX_W_TABLE (VEX_W_0F51_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F51_P_1) },
    { VEX_W_TABLE (VEX_W_0F51_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F51_P_3) },
  },

  /* PREFIX_VEX_0F52 */
  {
    { VEX_W_TABLE (VEX_W_0F52_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F52_P_1) },
  },

  /* PREFIX_VEX_0F53 */
  {
    { VEX_W_TABLE (VEX_W_0F53_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F53_P_1) },
  },

  /* PREFIX_VEX_0F58 */
  {
    { VEX_W_TABLE (VEX_W_0F58_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F58_P_1) },
    { VEX_W_TABLE (VEX_W_0F58_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F58_P_3) },
  },

  /* PREFIX_VEX_0F59 */
  {
    { VEX_W_TABLE (VEX_W_0F59_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F59_P_1) },
    { VEX_W_TABLE (VEX_W_0F59_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F59_P_3) },
  },

  /* PREFIX_VEX_0F5A */
  {
    { VEX_W_TABLE (VEX_W_0F5A_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F5A_P_1) },
    { "vcvtpd2ps%XY", { XMM, EXx }, 0 },
    { VEX_LEN_TABLE (VEX_LEN_0F5A_P_3) },
  },

  /* PREFIX_VEX_0F5B */
  {
    { VEX_W_TABLE (VEX_W_0F5B_P_0) },
    { VEX_W_TABLE (VEX_W_0F5B_P_1) },
    { VEX_W_TABLE (VEX_W_0F5B_P_2) },
  },

  /* PREFIX_VEX_0F5C */
  {
    { VEX_W_TABLE (VEX_W_0F5C_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F5C_P_1) },
    { VEX_W_TABLE (VEX_W_0F5C_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F5C_P_3) },
  },

  /* PREFIX_VEX_0F5D */
  {
    { VEX_W_TABLE (VEX_W_0F5D_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F5D_P_1) },
    { VEX_W_TABLE (VEX_W_0F5D_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F5D_P_3) },
  },

  /* PREFIX_VEX_0F5E */
  {
    { VEX_W_TABLE (VEX_W_0F5E_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F5E_P_1) },
    { VEX_W_TABLE (VEX_W_0F5E_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F5E_P_3) },
  },

  /* PREFIX_VEX_0F5F */
  {
    { VEX_W_TABLE (VEX_W_0F5F_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F5F_P_1) },
    { VEX_W_TABLE (VEX_W_0F5F_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F5F_P_3) },
  },

  /* PREFIX_VEX_0F60 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F60_P_2) },
  },

  /* PREFIX_VEX_0F61 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F61_P_2) },
  },

  /* PREFIX_VEX_0F62 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F62_P_2) },
  },

  /* PREFIX_VEX_0F63 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F63_P_2) },
  },

  /* PREFIX_VEX_0F64 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F64_P_2) },
  },

  /* PREFIX_VEX_0F65 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F65_P_2) },
  },

  /* PREFIX_VEX_0F66 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F66_P_2) },
  },

  /* PREFIX_VEX_0F67 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F67_P_2) },
  },

  /* PREFIX_VEX_0F68 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F68_P_2) },
  },

  /* PREFIX_VEX_0F69 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F69_P_2) },
  },

  /* PREFIX_VEX_0F6A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F6A_P_2) },
  },

  /* PREFIX_VEX_0F6B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F6B_P_2) },
  },

  /* PREFIX_VEX_0F6C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F6C_P_2) },
  },

  /* PREFIX_VEX_0F6D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F6D_P_2) },
  },

  /* PREFIX_VEX_0F6E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F6E_P_2) },
  },

  /* PREFIX_VEX_0F6F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F6F_P_1) },
    { VEX_W_TABLE (VEX_W_0F6F_P_2) },
  },

  /* PREFIX_VEX_0F70 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F70_P_1) },
    { VEX_W_TABLE (VEX_W_0F70_P_2) },
    { VEX_W_TABLE (VEX_W_0F70_P_3) },
  },

  /* PREFIX_VEX_0F71_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F71_R_2_P_2) },
  },

  /* PREFIX_VEX_0F71_REG_4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F71_R_4_P_2) },
  },

  /* PREFIX_VEX_0F71_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F71_R_6_P_2) },
  },

  /* PREFIX_VEX_0F72_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F72_R_2_P_2) },
  },

  /* PREFIX_VEX_0F72_REG_4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F72_R_4_P_2) },
  },

  /* PREFIX_VEX_0F72_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F72_R_6_P_2) },
  },

  /* PREFIX_VEX_0F73_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F73_R_2_P_2) },
  },

  /* PREFIX_VEX_0F73_REG_3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F73_R_3_P_2) },
  },

  /* PREFIX_VEX_0F73_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F73_R_6_P_2) },
  },

  /* PREFIX_VEX_0F73_REG_7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F73_R_7_P_2) },
  },

  /* PREFIX_VEX_0F74 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F74_P_2) },
  },

  /* PREFIX_VEX_0F75 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F75_P_2) },
  },

  /* PREFIX_VEX_0F76 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F76_P_2) },
  },

  /* PREFIX_VEX_0F77 */
  {
    { VEX_W_TABLE (VEX_W_0F77_P_0) },
  },

  /* PREFIX_VEX_0F7C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F7C_P_2) },
    { VEX_W_TABLE (VEX_W_0F7C_P_3) },
  },

  /* PREFIX_VEX_0F7D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F7D_P_2) },
    { VEX_W_TABLE (VEX_W_0F7D_P_3) },
  },

  /* PREFIX_VEX_0F7E */
  {
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F7E_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_0F7E_P_2) },
  },

  /* PREFIX_VEX_0F7F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F7F_P_1) },
    { VEX_W_TABLE (VEX_W_0F7F_P_2) },
  },

  /* PREFIX_VEX_0F90 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F90_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F90_P_2) },
  },

  /* PREFIX_VEX_0F91 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F91_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F91_P_2) },
  },

  /* PREFIX_VEX_0F92 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F92_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F92_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F92_P_3) },
  },

  /* PREFIX_VEX_0F93 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F93_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F93_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F93_P_3) },
  },

  /* PREFIX_VEX_0F98 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F98_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F98_P_2) },
  },

  /* PREFIX_VEX_0F99 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F99_P_0) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F99_P_2) },
  },

  /* PREFIX_VEX_0FC2 */
  {
    { VEX_W_TABLE (VEX_W_0FC2_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0FC2_P_1) },
    { VEX_W_TABLE (VEX_W_0FC2_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0FC2_P_3) },
  },

  /* PREFIX_VEX_0FC4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FC4_P_2) },
  },

  /* PREFIX_VEX_0FC5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FC5_P_2) },
  },

  /* PREFIX_VEX_0FD0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD0_P_2) },
    { VEX_W_TABLE (VEX_W_0FD0_P_3) },
  },

  /* PREFIX_VEX_0FD1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD1_P_2) },
  },

  /* PREFIX_VEX_0FD2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD2_P_2) },
  },

  /* PREFIX_VEX_0FD3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD3_P_2) },
  },

  /* PREFIX_VEX_0FD4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD4_P_2) },
  },

  /* PREFIX_VEX_0FD5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD5_P_2) },
  },

  /* PREFIX_VEX_0FD6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FD6_P_2) },
  },

  /* PREFIX_VEX_0FD7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0FD7_PREFIX_2) },
  },

  /* PREFIX_VEX_0FD8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD8_P_2) },
  },

  /* PREFIX_VEX_0FD9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD9_P_2) },
  },

  /* PREFIX_VEX_0FDA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDA_P_2) },
  },

  /* PREFIX_VEX_0FDB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDB_P_2) },
  },

  /* PREFIX_VEX_0FDC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDC_P_2) },
  },

  /* PREFIX_VEX_0FDD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDD_P_2) },
  },

  /* PREFIX_VEX_0FDE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDE_P_2) },
  },

  /* PREFIX_VEX_0FDF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FDF_P_2) },
  },

  /* PREFIX_VEX_0FE0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE0_P_2) },
  },

  /* PREFIX_VEX_0FE1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE1_P_2) },
  },

  /* PREFIX_VEX_0FE2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE2_P_2) },
  },

  /* PREFIX_VEX_0FE3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE3_P_2) },
  },

  /* PREFIX_VEX_0FE4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE4_P_2) },
  },

  /* PREFIX_VEX_0FE5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE5_P_2) },
  },

  /* PREFIX_VEX_0FE6 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE6_P_1) },
    { VEX_W_TABLE (VEX_W_0FE6_P_2) },
    { VEX_W_TABLE (VEX_W_0FE6_P_3) },
  },

  /* PREFIX_VEX_0FE7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0FE7_PREFIX_2) },
  },

  /* PREFIX_VEX_0FE8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE8_P_2) },
  },

  /* PREFIX_VEX_0FE9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FE9_P_2) },
  },

  /* PREFIX_VEX_0FEA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FEA_P_2) },
  },

  /* PREFIX_VEX_0FEB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FEB_P_2) },
  },

  /* PREFIX_VEX_0FEC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FEC_P_2) },
  },

  /* PREFIX_VEX_0FED */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FED_P_2) },
  },

  /* PREFIX_VEX_0FEE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FEE_P_2) },
  },

  /* PREFIX_VEX_0FEF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FEF_P_2) },
  },

  /* PREFIX_VEX_0FF0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0FF0_PREFIX_3) },
  },

  /* PREFIX_VEX_0FF1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF1_P_2) },
  },

  /* PREFIX_VEX_0FF2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF2_P_2) },
  },

  /* PREFIX_VEX_0FF3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF3_P_2) },
  },

  /* PREFIX_VEX_0FF4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF4_P_2) },
  },

  /* PREFIX_VEX_0FF5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF5_P_2) },
  },

  /* PREFIX_VEX_0FF6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF6_P_2) },
  },

  /* PREFIX_VEX_0FF7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FF7_P_2) },
  },

  /* PREFIX_VEX_0FF8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF8_P_2) },
  },

  /* PREFIX_VEX_0FF9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FF9_P_2) },
  },

  /* PREFIX_VEX_0FFA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FFA_P_2) },
  },

  /* PREFIX_VEX_0FFB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FFB_P_2) },
  },

  /* PREFIX_VEX_0FFC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FFC_P_2) },
  },

  /* PREFIX_VEX_0FFD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FFD_P_2) },
  },

  /* PREFIX_VEX_0FFE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FFE_P_2) },
  },

  /* PREFIX_VEX_0F3800 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3800_P_2) },
  },

  /* PREFIX_VEX_0F3801 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3801_P_2) },
  },

  /* PREFIX_VEX_0F3802 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3802_P_2) },
  },

  /* PREFIX_VEX_0F3803 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3803_P_2) },
  },

  /* PREFIX_VEX_0F3804 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3804_P_2) },
  },

  /* PREFIX_VEX_0F3805 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3805_P_2) },
  },

  /* PREFIX_VEX_0F3806 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3806_P_2) },
  },

  /* PREFIX_VEX_0F3807 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3807_P_2) },
  },

  /* PREFIX_VEX_0F3808 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3808_P_2) },
  },

  /* PREFIX_VEX_0F3809 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3809_P_2) },
  },

  /* PREFIX_VEX_0F380A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380A_P_2) },
  },

  /* PREFIX_VEX_0F380B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380B_P_2) },
  },

  /* PREFIX_VEX_0F380C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380C_P_2) },
  },

  /* PREFIX_VEX_0F380D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380D_P_2) },
  },

  /* PREFIX_VEX_0F380E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380E_P_2) },
  },

  /* PREFIX_VEX_0F380F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F380F_P_2) },
  },

  /* PREFIX_VEX_0F3813 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vcvtph2ps", { XM, EXxmmq }, 0 },
  },

  /* PREFIX_VEX_0F3816 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3816_P_2) },
  },

  /* PREFIX_VEX_0F3817 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3817_P_2) },
  },

  /* PREFIX_VEX_0F3818 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3818_P_2) },
  },

  /* PREFIX_VEX_0F3819 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3819_P_2) },
  },

  /* PREFIX_VEX_0F381A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F381A_PREFIX_2) },
  },

  /* PREFIX_VEX_0F381C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F381C_P_2) },
  },

  /* PREFIX_VEX_0F381D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F381D_P_2) },
  },

  /* PREFIX_VEX_0F381E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F381E_P_2) },
  },

  /* PREFIX_VEX_0F3820 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3820_P_2) },
  },

  /* PREFIX_VEX_0F3821 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3821_P_2) },
  },

  /* PREFIX_VEX_0F3822 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3822_P_2) },
  },

  /* PREFIX_VEX_0F3823 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3823_P_2) },
  },

  /* PREFIX_VEX_0F3824 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3824_P_2) },
  },

  /* PREFIX_VEX_0F3825 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3825_P_2) },
  },

  /* PREFIX_VEX_0F3828 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3828_P_2) },
  },

  /* PREFIX_VEX_0F3829 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3829_P_2) },
  },

  /* PREFIX_VEX_0F382A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F382A_PREFIX_2) },
  },

  /* PREFIX_VEX_0F382B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F382B_P_2) },
  },

  /* PREFIX_VEX_0F382C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
     { MOD_TABLE (MOD_VEX_0F382C_PREFIX_2) },
  },

  /* PREFIX_VEX_0F382D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
     { MOD_TABLE (MOD_VEX_0F382D_PREFIX_2) },
  },

  /* PREFIX_VEX_0F382E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
     { MOD_TABLE (MOD_VEX_0F382E_PREFIX_2) },
  },

  /* PREFIX_VEX_0F382F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
     { MOD_TABLE (MOD_VEX_0F382F_PREFIX_2) },
  },

  /* PREFIX_VEX_0F3830 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3830_P_2) },
  },

  /* PREFIX_VEX_0F3831 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3831_P_2) },
  },

  /* PREFIX_VEX_0F3832 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3832_P_2) },
  },

  /* PREFIX_VEX_0F3833 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3833_P_2) },
  },

  /* PREFIX_VEX_0F3834 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3834_P_2) },
  },

  /* PREFIX_VEX_0F3835 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3835_P_2) },
  },

  /* PREFIX_VEX_0F3836 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3836_P_2) },
  },

  /* PREFIX_VEX_0F3837 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3837_P_2) },
  },

  /* PREFIX_VEX_0F3838 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3838_P_2) },
  },

  /* PREFIX_VEX_0F3839 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3839_P_2) },
  },

  /* PREFIX_VEX_0F383A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383A_P_2) },
  },

  /* PREFIX_VEX_0F383B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383B_P_2) },
  },

  /* PREFIX_VEX_0F383C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383C_P_2) },
  },

  /* PREFIX_VEX_0F383D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383D_P_2) },
  },

  /* PREFIX_VEX_0F383E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383E_P_2) },
  },

  /* PREFIX_VEX_0F383F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F383F_P_2) },
  },

  /* PREFIX_VEX_0F3840 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3840_P_2) },
  },

  /* PREFIX_VEX_0F3841 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3841_P_2) },
  },

  /* PREFIX_VEX_0F3845 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsrlv%LW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F3846 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3846_P_2) },
  },

  /* PREFIX_VEX_0F3847 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsllv%LW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F3858 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3858_P_2) },
  },

  /* PREFIX_VEX_0F3859 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3859_P_2) },
  },

  /* PREFIX_VEX_0F385A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F385A_PREFIX_2) },
  },

  /* PREFIX_VEX_0F3878 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3878_P_2) },
  },

  /* PREFIX_VEX_0F3879 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3879_P_2) },
  },

  /* PREFIX_VEX_0F388C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F388C_PREFIX_2) },
  },

  /* PREFIX_VEX_0F388E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_VEX_0F388E_PREFIX_2) },
  },

  /* PREFIX_VEX_0F3890 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpgatherd%LW", { XM, MVexVSIBDWpX, Vex }, 0 },
  },

  /* PREFIX_VEX_0F3891 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpgatherq%LW", { XMGatherQ, MVexVSIBQWpX, VexGatherQ }, 0 },
  },

  /* PREFIX_VEX_0F3892 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherdp%XW", { XM, MVexVSIBDWpX, Vex }, 0 },
  },

  /* PREFIX_VEX_0F3893 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherqp%XW", { XMGatherQ, MVexVSIBQWpX, VexGatherQ }, 0 },
  },

  /* PREFIX_VEX_0F3896 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F3897 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F3898 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F3899 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd132s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F389A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F389B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F389C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F389D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd132s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F389E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub132p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F389F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub132s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38A6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub213p%XW", { XM, Vex, EXx }, 0 },
    { Bad_Opcode },
  },

  /* PREFIX_VEX_0F38A7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd213p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38A8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd213p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38A9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd213s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38AA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38AB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38AC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd213p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38AD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd213s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38AE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub213p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38AF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub213s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38B6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38B7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38B8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38B9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd231s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38BA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38BB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub231s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38BC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38BD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd231s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38BE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub231p%XW", { XM, Vex, EXx }, 0 },
  },

  /* PREFIX_VEX_0F38BF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub231s%XW", { XMScalar, VexScalar, EXVexWdqScalar }, 0 },
  },

  /* PREFIX_VEX_0F38DB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38DB_P_2) },
  },

  /* PREFIX_VEX_0F38DC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38DC_P_2) },
  },

  /* PREFIX_VEX_0F38DD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38DD_P_2) },
  },

  /* PREFIX_VEX_0F38DE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38DE_P_2) },
  },

  /* PREFIX_VEX_0F38DF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38DF_P_2) },
  },

  /* PREFIX_VEX_0F38F2 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F2_P_0) },
  },

  /* PREFIX_VEX_0F38F3_REG_1 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F3_R_1_P_0) },
  },

  /* PREFIX_VEX_0F38F3_REG_2 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F3_R_2_P_0) },
  },

  /* PREFIX_VEX_0F38F3_REG_3 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F3_R_3_P_0) },
  },

  /* PREFIX_VEX_0F38F5 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F5_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F38F5_P_1) },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38F5_P_3) },
  },

  /* PREFIX_VEX_0F38F6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F38F6_P_3) },
  },

  /* PREFIX_VEX_0F38F7 */
  {
    { VEX_LEN_TABLE (VEX_LEN_0F38F7_P_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F38F7_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_0F38F7_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_0F38F7_P_3) },
  },

  /* PREFIX_VEX_0F3A00 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A00_P_2) },
  },

  /* PREFIX_VEX_0F3A01 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A01_P_2) },
  },

  /* PREFIX_VEX_0F3A02 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A02_P_2) },
  },

  /* PREFIX_VEX_0F3A04 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A04_P_2) },
  },

  /* PREFIX_VEX_0F3A05 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A05_P_2) },
  },

  /* PREFIX_VEX_0F3A06 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A06_P_2) },
  },

  /* PREFIX_VEX_0F3A08 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A08_P_2) },
  },

  /* PREFIX_VEX_0F3A09 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A09_P_2) },
  },

  /* PREFIX_VEX_0F3A0A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A0A_P_2) },
  },

  /* PREFIX_VEX_0F3A0B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A0B_P_2) },
  },

  /* PREFIX_VEX_0F3A0C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A0C_P_2) },
  },

  /* PREFIX_VEX_0F3A0D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A0D_P_2) },
  },

  /* PREFIX_VEX_0F3A0E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A0E_P_2) },
  },

  /* PREFIX_VEX_0F3A0F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A0F_P_2) },
  },

  /* PREFIX_VEX_0F3A14 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A14_P_2) },
  },

  /* PREFIX_VEX_0F3A15 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A15_P_2) },
  },

  /* PREFIX_VEX_0F3A16 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A16_P_2) },
  },

  /* PREFIX_VEX_0F3A17 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A17_P_2) },
  },

  /* PREFIX_VEX_0F3A18 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A18_P_2) },
  },

  /* PREFIX_VEX_0F3A19 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A19_P_2) },
  },

  /* PREFIX_VEX_0F3A1D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vcvtps2ph", { EXxmmq, XM, Ib }, 0 },
  },

  /* PREFIX_VEX_0F3A20 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A20_P_2) },
  },

  /* PREFIX_VEX_0F3A21 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A21_P_2) },
  },

  /* PREFIX_VEX_0F3A22 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A22_P_2) },
  },

  /* PREFIX_VEX_0F3A30 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A30_P_2) },
  },

  /* PREFIX_VEX_0F3A31 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A31_P_2) },
  },

  /* PREFIX_VEX_0F3A32 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A32_P_2) },
  },

  /* PREFIX_VEX_0F3A33 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A33_P_2) },
  },

  /* PREFIX_VEX_0F3A38 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A38_P_2) },
  },

  /* PREFIX_VEX_0F3A39 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A39_P_2) },
  },

  /* PREFIX_VEX_0F3A40 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A40_P_2) },
  },

  /* PREFIX_VEX_0F3A41 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A41_P_2) },
  },

  /* PREFIX_VEX_0F3A42 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A42_P_2) },
  },

  /* PREFIX_VEX_0F3A44 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A44_P_2) },
  },

  /* PREFIX_VEX_0F3A46 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A46_P_2) },
  },

  /* PREFIX_VEX_0F3A48 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A48_P_2) },
  },

  /* PREFIX_VEX_0F3A49 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A49_P_2) },
  },

  /* PREFIX_VEX_0F3A4A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A4A_P_2) },
  },

  /* PREFIX_VEX_0F3A4B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A4B_P_2) },
  },

  /* PREFIX_VEX_0F3A4C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A4C_P_2) },
  },

  /* PREFIX_VEX_0F3A5C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsubps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A5D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsubpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A5E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubaddps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A5F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubaddpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A60 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A60_P_2) },
    { Bad_Opcode },
  },

  /* PREFIX_VEX_0F3A61 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A61_P_2) },
  },

  /* PREFIX_VEX_0F3A62 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A62_P_2) },
  },

  /* PREFIX_VEX_0F3A63 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A63_P_2) },
  },

  /* PREFIX_VEX_0F3A68 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A69 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A6A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A6A_P_2) },
  },

  /* PREFIX_VEX_0F3A6B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A6B_P_2) },
  },

  /* PREFIX_VEX_0F3A6C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A6D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A6E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A6E_P_2) },
  },

  /* PREFIX_VEX_0F3A6F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A6F_P_2) },
  },

  /* PREFIX_VEX_0F3A78 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmaddps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A79 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmaddpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A7A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A7A_P_2) },
  },

  /* PREFIX_VEX_0F3A7B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A7B_P_2) },
  },

  /* PREFIX_VEX_0F3A7C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsubps", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { Bad_Opcode },
  },

  /* PREFIX_VEX_0F3A7D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsubpd", { XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
  },

  /* PREFIX_VEX_0F3A7E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A7E_P_2) },
  },

  /* PREFIX_VEX_0F3A7F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3A7F_P_2) },
  },

  /* PREFIX_VEX_0F3ADF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3ADF_P_2) },
  },

  /* PREFIX_VEX_0F3AF0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0F3AF0_P_3) },
  },

#define NEED_PREFIX_TABLE
#include "i386-dis-evex.h"
#undef NEED_PREFIX_TABLE
};

static const struct dis386 x86_64_table[][2] = {
  /* X86_64_06 */
  {
    { "pushP", { es }, 0 },
  },

  /* X86_64_07 */
  {
    { "popP", { es }, 0 },
  },

  /* X86_64_0D */
  {
    { "pushP", { cs }, 0 },
  },

  /* X86_64_16 */
  {
    { "pushP", { ss }, 0 },
  },

  /* X86_64_17 */
  {
    { "popP", { ss }, 0 },
  },

  /* X86_64_1E */
  {
    { "pushP", { ds }, 0 },
  },

  /* X86_64_1F */
  {
    { "popP", { ds }, 0 },
  },

  /* X86_64_27 */
  {
    { "daa", { XX }, 0 },
  },

  /* X86_64_2F */
  {
    { "das", { XX }, 0 },
  },

  /* X86_64_37 */
  {
    { "aaa", { XX }, 0 },
  },

  /* X86_64_3F */
  {
    { "aas", { XX }, 0 },
  },

  /* X86_64_60 */
  {
    { "pushaP", { XX }, 0 },
  },

  /* X86_64_61 */
  {
    { "popaP", { XX }, 0 },
  },

  /* X86_64_62 */
  {
    { MOD_TABLE (MOD_62_32BIT) },
    { EVEX_TABLE (EVEX_0F) },
  },

  /* X86_64_63 */
  {
    { "arpl", { Ew, Gw }, 0 },
    { "movs{lq|xd}", { Gv, Ed }, 0 },
  },

  /* X86_64_6D */
  {
    { "ins{R|}", { Yzr, indirDX }, 0 },
    { "ins{G|}", { Yzr, indirDX }, 0 },
  },

  /* X86_64_6F */
  {
    { "outs{R|}", { indirDXr, Xz }, 0 },
    { "outs{G|}", { indirDXr, Xz }, 0 },
  },

  /* X86_64_9A */
  {
    { "Jcall{T|}", { Ap }, 0 },
  },

  /* X86_64_C4 */
  {
    { MOD_TABLE (MOD_C4_32BIT) },
    { VEX_C4_TABLE (VEX_0F) },
  },

  /* X86_64_C5 */
  {
    { MOD_TABLE (MOD_C5_32BIT) },
    { VEX_C5_TABLE (VEX_0F) },
  },

  /* X86_64_CE */
  {
    { "into", { XX }, 0 },
  },

  /* X86_64_D4 */
  {
    { "aam", { Ib }, 0 },
  },

  /* X86_64_D5 */
  {
    { "aad", { Ib }, 0 },
  },

  /* X86_64_E8 */
  {
    { "callP",		{ Jv, BND }, 0 },
    { "call@",		{ Jv, BND }, 0 }
  },

  /* X86_64_E9 */
  {
    { "jmpP",		{ Jv, BND }, 0 },
    { "jmp@",		{ Jv, BND }, 0 }
  },

  /* X86_64_EA */
  {
    { "Jjmp{T|}", { Ap }, 0 },
  },

  /* X86_64_0F01_REG_0 */
  {
    { "sgdt{Q|IQ}", { M }, 0 },
    { "sgdt", { M }, 0 },
  },

  /* X86_64_0F01_REG_1 */
  {
    { "sidt{Q|IQ}", { M }, 0 },
    { "sidt", { M }, 0 },
  },

  /* X86_64_0F01_REG_2 */
  {
    { "lgdt{Q|Q}", { M }, 0 },
    { "lgdt", { M }, 0 },
  },

  /* X86_64_0F01_REG_3 */
  {
    { "lidt{Q|Q}", { M }, 0 },
    { "lidt", { M }, 0 },
  },
};

static const struct dis386 three_byte_table[][256] = {

  /* THREE_BYTE_0F38 */
  {
    /* 00 */
    { "pshufb",		{ MX, EM }, PREFIX_OPCODE },
    { "phaddw",		{ MX, EM }, PREFIX_OPCODE },
    { "phaddd",		{ MX, EM }, PREFIX_OPCODE },
    { "phaddsw",	{ MX, EM }, PREFIX_OPCODE },
    { "pmaddubsw",	{ MX, EM }, PREFIX_OPCODE },
    { "phsubw",		{ MX, EM }, PREFIX_OPCODE },
    { "phsubd",		{ MX, EM }, PREFIX_OPCODE },
    { "phsubsw",	{ MX, EM }, PREFIX_OPCODE },
    /* 08 */
    { "psignb",		{ MX, EM }, PREFIX_OPCODE },
    { "psignw",		{ MX, EM }, PREFIX_OPCODE },
    { "psignd",		{ MX, EM }, PREFIX_OPCODE },
    { "pmulhrsw",	{ MX, EM }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { PREFIX_TABLE (PREFIX_0F3810) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3814) },
    { PREFIX_TABLE (PREFIX_0F3815) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3817) },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "pabsb",		{ MX, EM }, PREFIX_OPCODE },
    { "pabsw",		{ MX, EM }, PREFIX_OPCODE },
    { "pabsd",		{ MX, EM }, PREFIX_OPCODE },
    { Bad_Opcode },
    /* 20 */
    { PREFIX_TABLE (PREFIX_0F3820) },
    { PREFIX_TABLE (PREFIX_0F3821) },
    { PREFIX_TABLE (PREFIX_0F3822) },
    { PREFIX_TABLE (PREFIX_0F3823) },
    { PREFIX_TABLE (PREFIX_0F3824) },
    { PREFIX_TABLE (PREFIX_0F3825) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { PREFIX_TABLE (PREFIX_0F3828) },
    { PREFIX_TABLE (PREFIX_0F3829) },
    { PREFIX_TABLE (PREFIX_0F382A) },
    { PREFIX_TABLE (PREFIX_0F382B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { PREFIX_TABLE (PREFIX_0F3830) },
    { PREFIX_TABLE (PREFIX_0F3831) },
    { PREFIX_TABLE (PREFIX_0F3832) },
    { PREFIX_TABLE (PREFIX_0F3833) },
    { PREFIX_TABLE (PREFIX_0F3834) },
    { PREFIX_TABLE (PREFIX_0F3835) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3837) },
    /* 38 */
    { PREFIX_TABLE (PREFIX_0F3838) },
    { PREFIX_TABLE (PREFIX_0F3839) },
    { PREFIX_TABLE (PREFIX_0F383A) },
    { PREFIX_TABLE (PREFIX_0F383B) },
    { PREFIX_TABLE (PREFIX_0F383C) },
    { PREFIX_TABLE (PREFIX_0F383D) },
    { PREFIX_TABLE (PREFIX_0F383E) },
    { PREFIX_TABLE (PREFIX_0F383F) },
    /* 40 */
    { PREFIX_TABLE (PREFIX_0F3840) },
    { PREFIX_TABLE (PREFIX_0F3841) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { PREFIX_TABLE (PREFIX_0F3880) },
    { PREFIX_TABLE (PREFIX_0F3881) },
    { PREFIX_TABLE (PREFIX_0F3882) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { PREFIX_TABLE (PREFIX_0F38C8) },
    { PREFIX_TABLE (PREFIX_0F38C9) },
    { PREFIX_TABLE (PREFIX_0F38CA) },
    { PREFIX_TABLE (PREFIX_0F38CB) },
    { PREFIX_TABLE (PREFIX_0F38CC) },
    { PREFIX_TABLE (PREFIX_0F38CD) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F38DB) },
    { PREFIX_TABLE (PREFIX_0F38DC) },
    { PREFIX_TABLE (PREFIX_0F38DD) },
    { PREFIX_TABLE (PREFIX_0F38DE) },
    { PREFIX_TABLE (PREFIX_0F38DF) },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { PREFIX_TABLE (PREFIX_0F38F0) },
    { PREFIX_TABLE (PREFIX_0F38F1) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F38F6) },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* THREE_BYTE_0F3A */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { PREFIX_TABLE (PREFIX_0F3A08) },
    { PREFIX_TABLE (PREFIX_0F3A09) },
    { PREFIX_TABLE (PREFIX_0F3A0A) },
    { PREFIX_TABLE (PREFIX_0F3A0B) },
    { PREFIX_TABLE (PREFIX_0F3A0C) },
    { PREFIX_TABLE (PREFIX_0F3A0D) },
    { PREFIX_TABLE (PREFIX_0F3A0E) },
    { "palignr",	{ MX, EM, Ib }, PREFIX_OPCODE },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3A14) },
    { PREFIX_TABLE (PREFIX_0F3A15) },
    { PREFIX_TABLE (PREFIX_0F3A16) },
    { PREFIX_TABLE (PREFIX_0F3A17) },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { PREFIX_TABLE (PREFIX_0F3A20) },
    { PREFIX_TABLE (PREFIX_0F3A21) },
    { PREFIX_TABLE (PREFIX_0F3A22) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { PREFIX_TABLE (PREFIX_0F3A40) },
    { PREFIX_TABLE (PREFIX_0F3A41) },
    { PREFIX_TABLE (PREFIX_0F3A42) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3A44) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { PREFIX_TABLE (PREFIX_0F3A60) },
    { PREFIX_TABLE (PREFIX_0F3A61) },
    { PREFIX_TABLE (PREFIX_0F3A62) },
    { PREFIX_TABLE (PREFIX_0F3A63) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3ACC) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F3ADF) },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },

  /* THREE_BYTE_0F7A */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { "ptest",		{ XX }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { "phaddbw",	{ XM, EXq }, PREFIX_OPCODE },
    { "phaddbd",	{ XM, EXq }, PREFIX_OPCODE },
    { "phaddbq",	{ XM, EXq }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { "phaddwd",	{ XM, EXq }, PREFIX_OPCODE },
    { "phaddwq",	{ XM, EXq }, PREFIX_OPCODE },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "phadddq",	{ XM, EXq }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { "phaddubw",	{ XM, EXq }, PREFIX_OPCODE },
    { "phaddubd",	{ XM, EXq }, PREFIX_OPCODE },
    { "phaddubq",	{ XM, EXq }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { "phadduwd",	{ XM, EXq }, PREFIX_OPCODE },
    { "phadduwq",	{ XM, EXq }, PREFIX_OPCODE },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "phaddudq",	{ XM, EXq }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { "phsubbw",	{ XM, EXq }, PREFIX_OPCODE },
    { "phsubbd",	{ XM, EXq }, PREFIX_OPCODE },
    { "phsubbq",	{ XM, EXq }, PREFIX_OPCODE },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
};

static const struct dis386 xop_table[][256] = {
  /* XOP_08 */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmacssww", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacsswd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacssdql", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmacssdd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacssdqh", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmacsww", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacswd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacsdql", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmacsdd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpmacsdqh", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpcmov", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { "vpperm", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmadcsswd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmadcswd", 	{ XMVexW, Vex, EXVexW, EXVexW, VexI4 }, 0 },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { "vprotb", 	{ XM, Vex_2src_1, Ib }, 0 },
    { "vprotw", 	{ XM, Vex_2src_1, Ib }, 0 },
    { "vprotd", 	{ XM, Vex_2src_1, Ib }, 0 },
    { "vprotq", 	{ XM, Vex_2src_1, Ib }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_CC) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_CD) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_CE) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_CF) },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_EC) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_ED) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_EE) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_08_EF) },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* XOP_09 */
  {
    /* 00 */
    { Bad_Opcode },
    { REG_TABLE (REG_XOP_TBM_01) },
    { REG_TABLE (REG_XOP_TBM_02) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { REG_TABLE (REG_XOP_LWPCB) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_09_80) },
    { VEX_LEN_TABLE (VEX_LEN_0FXOP_09_81) },
    { "vfrczss", 	{ XM, EXd }, 0 },
    { "vfrczsd", 	{ XM, EXq }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { "vprotb",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vprotw",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vprotd",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vprotq",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshlb",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshlw",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshld",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshlq",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    /* 98 */
    { "vpshab",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshaw",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshad",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { "vpshaq",		{ XM, Vex_2src_1, Vex_2src_2 }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { "vphaddbw",	{ XM, EXxmm }, 0 },
    { "vphaddbd",	{ XM, EXxmm }, 0 },
    { "vphaddbq",	{ XM, EXxmm }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vphaddwd",	{ XM, EXxmm }, 0 },
    { "vphaddwq",	{ XM, EXxmm }, 0 },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vphadddq",	{ XM, EXxmm }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { "vphaddubw",	{ XM, EXxmm }, 0 },
    { "vphaddubd",	{ XM, EXxmm }, 0 },
    { "vphaddubq",	{ XM, EXxmm }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vphadduwd",	{ XM, EXxmm }, 0 },
    { "vphadduwq",	{ XM, EXxmm }, 0 },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vphaddudq",	{ XM, EXxmm }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e0 */
    { Bad_Opcode },
    { "vphsubbw",	{ XM, EXxmm }, 0 },
    { "vphsubwd",	{ XM, EXxmm }, 0 },
    { "vphsubdq",	{ XM, EXxmm }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* XOP_0A */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { "bextr",	{ Gv, Ev, Iq }, 0 },
    { Bad_Opcode },
    { REG_TABLE (REG_XOP_LWP) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
};

static const struct dis386 vex_table[][256] = {
  /* VEX_0F */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { PREFIX_TABLE (PREFIX_VEX_0F10) },
    { PREFIX_TABLE (PREFIX_VEX_0F11) },
    { PREFIX_TABLE (PREFIX_VEX_0F12) },
    { MOD_TABLE (MOD_VEX_0F13) },
    { VEX_W_TABLE (VEX_W_0F14) },
    { VEX_W_TABLE (VEX_W_0F15) },
    { PREFIX_TABLE (PREFIX_VEX_0F16) },
    { MOD_TABLE (MOD_VEX_0F17) },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { VEX_W_TABLE (VEX_W_0F28) },
    { VEX_W_TABLE (VEX_W_0F29) },
    { PREFIX_TABLE (PREFIX_VEX_0F2A) },
    { MOD_TABLE (MOD_VEX_0F2B) },
    { PREFIX_TABLE (PREFIX_VEX_0F2C) },
    { PREFIX_TABLE (PREFIX_VEX_0F2D) },
    { PREFIX_TABLE (PREFIX_VEX_0F2E) },
    { PREFIX_TABLE (PREFIX_VEX_0F2F) },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F41) },
    { PREFIX_TABLE (PREFIX_VEX_0F42) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F44) },
    { PREFIX_TABLE (PREFIX_VEX_0F45) },
    { PREFIX_TABLE (PREFIX_VEX_0F46) },
    { PREFIX_TABLE (PREFIX_VEX_0F47) },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F4A) },
    { PREFIX_TABLE (PREFIX_VEX_0F4B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { MOD_TABLE (MOD_VEX_0F50) },
    { PREFIX_TABLE (PREFIX_VEX_0F51) },
    { PREFIX_TABLE (PREFIX_VEX_0F52) },
    { PREFIX_TABLE (PREFIX_VEX_0F53) },
    { "vandpX",		{ XM, Vex, EXx }, 0 },
    { "vandnpX",	{ XM, Vex, EXx }, 0 },
    { "vorpX",		{ XM, Vex, EXx }, 0 },
    { "vxorpX",		{ XM, Vex, EXx }, 0 },
    /* 58 */
    { PREFIX_TABLE (PREFIX_VEX_0F58) },
    { PREFIX_TABLE (PREFIX_VEX_0F59) },
    { PREFIX_TABLE (PREFIX_VEX_0F5A) },
    { PREFIX_TABLE (PREFIX_VEX_0F5B) },
    { PREFIX_TABLE (PREFIX_VEX_0F5C) },
    { PREFIX_TABLE (PREFIX_VEX_0F5D) },
    { PREFIX_TABLE (PREFIX_VEX_0F5E) },
    { PREFIX_TABLE (PREFIX_VEX_0F5F) },
    /* 60 */
    { PREFIX_TABLE (PREFIX_VEX_0F60) },
    { PREFIX_TABLE (PREFIX_VEX_0F61) },
    { PREFIX_TABLE (PREFIX_VEX_0F62) },
    { PREFIX_TABLE (PREFIX_VEX_0F63) },
    { PREFIX_TABLE (PREFIX_VEX_0F64) },
    { PREFIX_TABLE (PREFIX_VEX_0F65) },
    { PREFIX_TABLE (PREFIX_VEX_0F66) },
    { PREFIX_TABLE (PREFIX_VEX_0F67) },
    /* 68 */
    { PREFIX_TABLE (PREFIX_VEX_0F68) },
    { PREFIX_TABLE (PREFIX_VEX_0F69) },
    { PREFIX_TABLE (PREFIX_VEX_0F6A) },
    { PREFIX_TABLE (PREFIX_VEX_0F6B) },
    { PREFIX_TABLE (PREFIX_VEX_0F6C) },
    { PREFIX_TABLE (PREFIX_VEX_0F6D) },
    { PREFIX_TABLE (PREFIX_VEX_0F6E) },
    { PREFIX_TABLE (PREFIX_VEX_0F6F) },
    /* 70 */
    { PREFIX_TABLE (PREFIX_VEX_0F70) },
    { REG_TABLE (REG_VEX_0F71) },
    { REG_TABLE (REG_VEX_0F72) },
    { REG_TABLE (REG_VEX_0F73) },
    { PREFIX_TABLE (PREFIX_VEX_0F74) },
    { PREFIX_TABLE (PREFIX_VEX_0F75) },
    { PREFIX_TABLE (PREFIX_VEX_0F76) },
    { PREFIX_TABLE (PREFIX_VEX_0F77) },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F7C) },
    { PREFIX_TABLE (PREFIX_VEX_0F7D) },
    { PREFIX_TABLE (PREFIX_VEX_0F7E) },
    { PREFIX_TABLE (PREFIX_VEX_0F7F) },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { PREFIX_TABLE (PREFIX_VEX_0F90) },
    { PREFIX_TABLE (PREFIX_VEX_0F91) },
    { PREFIX_TABLE (PREFIX_VEX_0F92) },
    { PREFIX_TABLE (PREFIX_VEX_0F93) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { PREFIX_TABLE (PREFIX_VEX_0F98) },
    { PREFIX_TABLE (PREFIX_VEX_0F99) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { REG_TABLE (REG_VEX_0FAE) },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0FC2) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0FC4) },
    { PREFIX_TABLE (PREFIX_VEX_0FC5) },
    { "vshufpX",	{ XM, Vex, EXx, Ib }, 0 },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { PREFIX_TABLE (PREFIX_VEX_0FD0) },
    { PREFIX_TABLE (PREFIX_VEX_0FD1) },
    { PREFIX_TABLE (PREFIX_VEX_0FD2) },
    { PREFIX_TABLE (PREFIX_VEX_0FD3) },
    { PREFIX_TABLE (PREFIX_VEX_0FD4) },
    { PREFIX_TABLE (PREFIX_VEX_0FD5) },
    { PREFIX_TABLE (PREFIX_VEX_0FD6) },
    { PREFIX_TABLE (PREFIX_VEX_0FD7) },
    /* d8 */
    { PREFIX_TABLE (PREFIX_VEX_0FD8) },
    { PREFIX_TABLE (PREFIX_VEX_0FD9) },
    { PREFIX_TABLE (PREFIX_VEX_0FDA) },
    { PREFIX_TABLE (PREFIX_VEX_0FDB) },
    { PREFIX_TABLE (PREFIX_VEX_0FDC) },
    { PREFIX_TABLE (PREFIX_VEX_0FDD) },
    { PREFIX_TABLE (PREFIX_VEX_0FDE) },
    { PREFIX_TABLE (PREFIX_VEX_0FDF) },
    /* e0 */
    { PREFIX_TABLE (PREFIX_VEX_0FE0) },
    { PREFIX_TABLE (PREFIX_VEX_0FE1) },
    { PREFIX_TABLE (PREFIX_VEX_0FE2) },
    { PREFIX_TABLE (PREFIX_VEX_0FE3) },
    { PREFIX_TABLE (PREFIX_VEX_0FE4) },
    { PREFIX_TABLE (PREFIX_VEX_0FE5) },
    { PREFIX_TABLE (PREFIX_VEX_0FE6) },
    { PREFIX_TABLE (PREFIX_VEX_0FE7) },
    /* e8 */
    { PREFIX_TABLE (PREFIX_VEX_0FE8) },
    { PREFIX_TABLE (PREFIX_VEX_0FE9) },
    { PREFIX_TABLE (PREFIX_VEX_0FEA) },
    { PREFIX_TABLE (PREFIX_VEX_0FEB) },
    { PREFIX_TABLE (PREFIX_VEX_0FEC) },
    { PREFIX_TABLE (PREFIX_VEX_0FED) },
    { PREFIX_TABLE (PREFIX_VEX_0FEE) },
    { PREFIX_TABLE (PREFIX_VEX_0FEF) },
    /* f0 */
    { PREFIX_TABLE (PREFIX_VEX_0FF0) },
    { PREFIX_TABLE (PREFIX_VEX_0FF1) },
    { PREFIX_TABLE (PREFIX_VEX_0FF2) },
    { PREFIX_TABLE (PREFIX_VEX_0FF3) },
    { PREFIX_TABLE (PREFIX_VEX_0FF4) },
    { PREFIX_TABLE (PREFIX_VEX_0FF5) },
    { PREFIX_TABLE (PREFIX_VEX_0FF6) },
    { PREFIX_TABLE (PREFIX_VEX_0FF7) },
    /* f8 */
    { PREFIX_TABLE (PREFIX_VEX_0FF8) },
    { PREFIX_TABLE (PREFIX_VEX_0FF9) },
    { PREFIX_TABLE (PREFIX_VEX_0FFA) },
    { PREFIX_TABLE (PREFIX_VEX_0FFB) },
    { PREFIX_TABLE (PREFIX_VEX_0FFC) },
    { PREFIX_TABLE (PREFIX_VEX_0FFD) },
    { PREFIX_TABLE (PREFIX_VEX_0FFE) },
    { Bad_Opcode },
  },
  /* VEX_0F38 */
  {
    /* 00 */
    { PREFIX_TABLE (PREFIX_VEX_0F3800) },
    { PREFIX_TABLE (PREFIX_VEX_0F3801) },
    { PREFIX_TABLE (PREFIX_VEX_0F3802) },
    { PREFIX_TABLE (PREFIX_VEX_0F3803) },
    { PREFIX_TABLE (PREFIX_VEX_0F3804) },
    { PREFIX_TABLE (PREFIX_VEX_0F3805) },
    { PREFIX_TABLE (PREFIX_VEX_0F3806) },
    { PREFIX_TABLE (PREFIX_VEX_0F3807) },
    /* 08 */
    { PREFIX_TABLE (PREFIX_VEX_0F3808) },
    { PREFIX_TABLE (PREFIX_VEX_0F3809) },
    { PREFIX_TABLE (PREFIX_VEX_0F380A) },
    { PREFIX_TABLE (PREFIX_VEX_0F380B) },
    { PREFIX_TABLE (PREFIX_VEX_0F380C) },
    { PREFIX_TABLE (PREFIX_VEX_0F380D) },
    { PREFIX_TABLE (PREFIX_VEX_0F380E) },
    { PREFIX_TABLE (PREFIX_VEX_0F380F) },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3813) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3816) },
    { PREFIX_TABLE (PREFIX_VEX_0F3817) },
    /* 18 */
    { PREFIX_TABLE (PREFIX_VEX_0F3818) },
    { PREFIX_TABLE (PREFIX_VEX_0F3819) },
    { PREFIX_TABLE (PREFIX_VEX_0F381A) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F381C) },
    { PREFIX_TABLE (PREFIX_VEX_0F381D) },
    { PREFIX_TABLE (PREFIX_VEX_0F381E) },
    { Bad_Opcode },
    /* 20 */
    { PREFIX_TABLE (PREFIX_VEX_0F3820) },
    { PREFIX_TABLE (PREFIX_VEX_0F3821) },
    { PREFIX_TABLE (PREFIX_VEX_0F3822) },
    { PREFIX_TABLE (PREFIX_VEX_0F3823) },
    { PREFIX_TABLE (PREFIX_VEX_0F3824) },
    { PREFIX_TABLE (PREFIX_VEX_0F3825) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { PREFIX_TABLE (PREFIX_VEX_0F3828) },
    { PREFIX_TABLE (PREFIX_VEX_0F3829) },
    { PREFIX_TABLE (PREFIX_VEX_0F382A) },
    { PREFIX_TABLE (PREFIX_VEX_0F382B) },
    { PREFIX_TABLE (PREFIX_VEX_0F382C) },
    { PREFIX_TABLE (PREFIX_VEX_0F382D) },
    { PREFIX_TABLE (PREFIX_VEX_0F382E) },
    { PREFIX_TABLE (PREFIX_VEX_0F382F) },
    /* 30 */
    { PREFIX_TABLE (PREFIX_VEX_0F3830) },
    { PREFIX_TABLE (PREFIX_VEX_0F3831) },
    { PREFIX_TABLE (PREFIX_VEX_0F3832) },
    { PREFIX_TABLE (PREFIX_VEX_0F3833) },
    { PREFIX_TABLE (PREFIX_VEX_0F3834) },
    { PREFIX_TABLE (PREFIX_VEX_0F3835) },
    { PREFIX_TABLE (PREFIX_VEX_0F3836) },
    { PREFIX_TABLE (PREFIX_VEX_0F3837) },
    /* 38 */
    { PREFIX_TABLE (PREFIX_VEX_0F3838) },
    { PREFIX_TABLE (PREFIX_VEX_0F3839) },
    { PREFIX_TABLE (PREFIX_VEX_0F383A) },
    { PREFIX_TABLE (PREFIX_VEX_0F383B) },
    { PREFIX_TABLE (PREFIX_VEX_0F383C) },
    { PREFIX_TABLE (PREFIX_VEX_0F383D) },
    { PREFIX_TABLE (PREFIX_VEX_0F383E) },
    { PREFIX_TABLE (PREFIX_VEX_0F383F) },
    /* 40 */
    { PREFIX_TABLE (PREFIX_VEX_0F3840) },
    { PREFIX_TABLE (PREFIX_VEX_0F3841) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3845) },
    { PREFIX_TABLE (PREFIX_VEX_0F3846) },
    { PREFIX_TABLE (PREFIX_VEX_0F3847) },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { PREFIX_TABLE (PREFIX_VEX_0F3858) },
    { PREFIX_TABLE (PREFIX_VEX_0F3859) },
    { PREFIX_TABLE (PREFIX_VEX_0F385A) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { PREFIX_TABLE (PREFIX_VEX_0F3878) },
    { PREFIX_TABLE (PREFIX_VEX_0F3879) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F388C) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F388E) },
    { Bad_Opcode },
    /* 90 */
    { PREFIX_TABLE (PREFIX_VEX_0F3890) },
    { PREFIX_TABLE (PREFIX_VEX_0F3891) },
    { PREFIX_TABLE (PREFIX_VEX_0F3892) },
    { PREFIX_TABLE (PREFIX_VEX_0F3893) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3896) },
    { PREFIX_TABLE (PREFIX_VEX_0F3897) },
    /* 98 */
    { PREFIX_TABLE (PREFIX_VEX_0F3898) },
    { PREFIX_TABLE (PREFIX_VEX_0F3899) },
    { PREFIX_TABLE (PREFIX_VEX_0F389A) },
    { PREFIX_TABLE (PREFIX_VEX_0F389B) },
    { PREFIX_TABLE (PREFIX_VEX_0F389C) },
    { PREFIX_TABLE (PREFIX_VEX_0F389D) },
    { PREFIX_TABLE (PREFIX_VEX_0F389E) },
    { PREFIX_TABLE (PREFIX_VEX_0F389F) },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38A6) },
    { PREFIX_TABLE (PREFIX_VEX_0F38A7) },
    /* a8 */
    { PREFIX_TABLE (PREFIX_VEX_0F38A8) },
    { PREFIX_TABLE (PREFIX_VEX_0F38A9) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AA) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AB) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AC) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AD) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AE) },
    { PREFIX_TABLE (PREFIX_VEX_0F38AF) },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38B6) },
    { PREFIX_TABLE (PREFIX_VEX_0F38B7) },
    /* b8 */
    { PREFIX_TABLE (PREFIX_VEX_0F38B8) },
    { PREFIX_TABLE (PREFIX_VEX_0F38B9) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BA) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BB) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BC) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BD) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BE) },
    { PREFIX_TABLE (PREFIX_VEX_0F38BF) },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38DB) },
    { PREFIX_TABLE (PREFIX_VEX_0F38DC) },
    { PREFIX_TABLE (PREFIX_VEX_0F38DD) },
    { PREFIX_TABLE (PREFIX_VEX_0F38DE) },
    { PREFIX_TABLE (PREFIX_VEX_0F38DF) },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38F2) },
    { REG_TABLE (REG_VEX_0F38F3) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F38F5) },
    { PREFIX_TABLE (PREFIX_VEX_0F38F6) },
    { PREFIX_TABLE (PREFIX_VEX_0F38F7) },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* VEX_0F3A */
  {
    /* 00 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A00) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A01) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A02) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A04) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A05) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A06) },
    { Bad_Opcode },
    /* 08 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A08) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A09) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0A) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0B) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0C) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0D) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0E) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A0F) },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A14) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A15) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A16) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A17) },
    /* 18 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A18) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A19) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A1D) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A20) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A21) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A22) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A30) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A31) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A32) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A33) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A38) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A39) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A40) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A41) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A42) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A44) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A46) },
    { Bad_Opcode },
    /* 48 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A48) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A49) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A4A) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A4B) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A4C) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3A5C) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A5D) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A5E) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A5F) },
    /* 60 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A60) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A61) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A62) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A63) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A68) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A69) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6A) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6B) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6C) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6D) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6E) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A6F) },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { PREFIX_TABLE (PREFIX_VEX_0F3A78) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A79) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7A) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7B) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7C) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7D) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7E) },
    { PREFIX_TABLE (PREFIX_VEX_0F3A7F) },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* a8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* b8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* c8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* d8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F3ADF) },
    /* e0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* e8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f0 */
    { PREFIX_TABLE (PREFIX_VEX_0F3AF0) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* f8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
};

#define NEED_OPCODE_TABLE
#include "i386-dis-evex.h"
#undef NEED_OPCODE_TABLE
static const struct dis386 vex_len_table[][2] = {
  /* VEX_LEN_0F10_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F10_P_1) },
    { VEX_W_TABLE (VEX_W_0F10_P_1) },
  },

  /* VEX_LEN_0F10_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F10_P_3) },
    { VEX_W_TABLE (VEX_W_0F10_P_3) },
  },

  /* VEX_LEN_0F11_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F11_P_1) },
    { VEX_W_TABLE (VEX_W_0F11_P_1) },
  },

  /* VEX_LEN_0F11_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F11_P_3) },
    { VEX_W_TABLE (VEX_W_0F11_P_3) },
  },

  /* VEX_LEN_0F12_P_0_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0F12_P_0_M_0) },
  },

  /* VEX_LEN_0F12_P_0_M_1 */
  {
    { VEX_W_TABLE (VEX_W_0F12_P_0_M_1) },
  },

  /* VEX_LEN_0F12_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F12_P_2) },
  },

  /* VEX_LEN_0F13_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0F13_M_0) },
  },

  /* VEX_LEN_0F16_P_0_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0F16_P_0_M_0) },
  },

  /* VEX_LEN_0F16_P_0_M_1 */
  {
    { VEX_W_TABLE (VEX_W_0F16_P_0_M_1) },
  },

  /* VEX_LEN_0F16_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F16_P_2) },
  },

  /* VEX_LEN_0F17_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0F17_M_0) },
  },

  /* VEX_LEN_0F2A_P_1 */
  {
    { "vcvtsi2ss%LQ",	{ XMScalar, VexScalar, Ev }, 0 },
    { "vcvtsi2ss%LQ",	{ XMScalar, VexScalar, Ev }, 0 },
  },

  /* VEX_LEN_0F2A_P_3 */
  {
    { "vcvtsi2sd%LQ",	{ XMScalar, VexScalar, Ev }, 0 },
    { "vcvtsi2sd%LQ",	{ XMScalar, VexScalar, Ev }, 0 },
  },

  /* VEX_LEN_0F2C_P_1 */
  {
    { "vcvttss2siY",	{ Gv, EXdScalar }, 0 },
    { "vcvttss2siY",	{ Gv, EXdScalar }, 0 },
  },

  /* VEX_LEN_0F2C_P_3 */
  {
    { "vcvttsd2siY",	{ Gv, EXqScalar }, 0 },
    { "vcvttsd2siY",	{ Gv, EXqScalar }, 0 },
  },

  /* VEX_LEN_0F2D_P_1 */
  {
    { "vcvtss2siY",	{ Gv, EXdScalar }, 0 },
    { "vcvtss2siY",	{ Gv, EXdScalar }, 0 },
  },

  /* VEX_LEN_0F2D_P_3 */
  {
    { "vcvtsd2siY",	{ Gv, EXqScalar }, 0 },
    { "vcvtsd2siY",	{ Gv, EXqScalar }, 0 },
  },

  /* VEX_LEN_0F2E_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F2E_P_0) },
    { VEX_W_TABLE (VEX_W_0F2E_P_0) },
  },

  /* VEX_LEN_0F2E_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F2E_P_2) },
    { VEX_W_TABLE (VEX_W_0F2E_P_2) },
  },

  /* VEX_LEN_0F2F_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F2F_P_0) },
    { VEX_W_TABLE (VEX_W_0F2F_P_0) },
  },

  /* VEX_LEN_0F2F_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F2F_P_2) },
    { VEX_W_TABLE (VEX_W_0F2F_P_2) },
  },

  /* VEX_LEN_0F41_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F41_P_0_LEN_1) },
  },
  /* VEX_LEN_0F41_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F41_P_2_LEN_1) },
  },
  /* VEX_LEN_0F42_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F42_P_0_LEN_1) },
  },
  /* VEX_LEN_0F42_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F42_P_2_LEN_1) },
  },
  /* VEX_LEN_0F44_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F44_P_0_LEN_0) },
  },
  /* VEX_LEN_0F44_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F44_P_2_LEN_0) },
  },
  /* VEX_LEN_0F45_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F45_P_0_LEN_1) },
  },
  /* VEX_LEN_0F45_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F45_P_2_LEN_1) },
  },
  /* VEX_LEN_0F46_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F46_P_0_LEN_1) },
  },
  /* VEX_LEN_0F46_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F46_P_2_LEN_1) },
  },
  /* VEX_LEN_0F47_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F47_P_0_LEN_1) },
  },
  /* VEX_LEN_0F47_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F47_P_2_LEN_1) },
  },
  /* VEX_LEN_0F4A_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F4A_P_0_LEN_1) },
  },
  /* VEX_LEN_0F4A_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F4A_P_2_LEN_1) },
  },
  /* VEX_LEN_0F4B_P_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F4B_P_0_LEN_1) },
  },
  /* VEX_LEN_0F4B_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F4B_P_2_LEN_1) },
  },

  /* VEX_LEN_0F51_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F51_P_1) },
    { VEX_W_TABLE (VEX_W_0F51_P_1) },
  },

  /* VEX_LEN_0F51_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F51_P_3) },
    { VEX_W_TABLE (VEX_W_0F51_P_3) },
  },

  /* VEX_LEN_0F52_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F52_P_1) },
    { VEX_W_TABLE (VEX_W_0F52_P_1) },
  },

  /* VEX_LEN_0F53_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F53_P_1) },
    { VEX_W_TABLE (VEX_W_0F53_P_1) },
  },

  /* VEX_LEN_0F58_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F58_P_1) },
    { VEX_W_TABLE (VEX_W_0F58_P_1) },
  },

  /* VEX_LEN_0F58_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F58_P_3) },
    { VEX_W_TABLE (VEX_W_0F58_P_3) },
  },

  /* VEX_LEN_0F59_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F59_P_1) },
    { VEX_W_TABLE (VEX_W_0F59_P_1) },
  },

  /* VEX_LEN_0F59_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F59_P_3) },
    { VEX_W_TABLE (VEX_W_0F59_P_3) },
  },

  /* VEX_LEN_0F5A_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F5A_P_1) },
    { VEX_W_TABLE (VEX_W_0F5A_P_1) },
  },

  /* VEX_LEN_0F5A_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F5A_P_3) },
    { VEX_W_TABLE (VEX_W_0F5A_P_3) },
  },

  /* VEX_LEN_0F5C_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F5C_P_1) },
    { VEX_W_TABLE (VEX_W_0F5C_P_1) },
  },

  /* VEX_LEN_0F5C_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F5C_P_3) },
    { VEX_W_TABLE (VEX_W_0F5C_P_3) },
  },

  /* VEX_LEN_0F5D_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F5D_P_1) },
    { VEX_W_TABLE (VEX_W_0F5D_P_1) },
  },

  /* VEX_LEN_0F5D_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F5D_P_3) },
    { VEX_W_TABLE (VEX_W_0F5D_P_3) },
  },

  /* VEX_LEN_0F5E_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F5E_P_1) },
    { VEX_W_TABLE (VEX_W_0F5E_P_1) },
  },

  /* VEX_LEN_0F5E_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F5E_P_3) },
    { VEX_W_TABLE (VEX_W_0F5E_P_3) },
  },

  /* VEX_LEN_0F5F_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F5F_P_1) },
    { VEX_W_TABLE (VEX_W_0F5F_P_1) },
  },

  /* VEX_LEN_0F5F_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F5F_P_3) },
    { VEX_W_TABLE (VEX_W_0F5F_P_3) },
  },

  /* VEX_LEN_0F6E_P_2 */
  {
    { "vmovK",		{ XMScalar, Edq }, 0 },
    { "vmovK",		{ XMScalar, Edq }, 0 },
  },

  /* VEX_LEN_0F7E_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0F7E_P_1) },
    { VEX_W_TABLE (VEX_W_0F7E_P_1) },
  },

  /* VEX_LEN_0F7E_P_2 */
  {
    { "vmovK",		{ Edq, XMScalar }, 0 },
    { "vmovK",		{ Edq, XMScalar }, 0 },
  },

  /* VEX_LEN_0F90_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F90_P_0_LEN_0) },
  },

  /* VEX_LEN_0F90_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F90_P_2_LEN_0) },
  },

  /* VEX_LEN_0F91_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F91_P_0_LEN_0) },
  },

  /* VEX_LEN_0F91_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F91_P_2_LEN_0) },
  },

  /* VEX_LEN_0F92_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F92_P_0_LEN_0) },
  },

  /* VEX_LEN_0F92_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F92_P_2_LEN_0) },
  },

  /* VEX_LEN_0F92_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F92_P_3_LEN_0) },
  },

  /* VEX_LEN_0F93_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F93_P_0_LEN_0) },
  },

  /* VEX_LEN_0F93_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F93_P_2_LEN_0) },
  },

  /* VEX_LEN_0F93_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0F93_P_3_LEN_0) },
  },

  /* VEX_LEN_0F98_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F98_P_0_LEN_0) },
  },

  /* VEX_LEN_0F98_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F98_P_2_LEN_0) },
  },

  /* VEX_LEN_0F99_P_0 */
  {
    { VEX_W_TABLE (VEX_W_0F99_P_0_LEN_0) },
  },

  /* VEX_LEN_0F99_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F99_P_2_LEN_0) },
  },

  /* VEX_LEN_0FAE_R_2_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0FAE_R_2_M_0) },
  },

  /* VEX_LEN_0FAE_R_3_M_0 */
  {
    { VEX_W_TABLE (VEX_W_0FAE_R_3_M_0) },
  },

  /* VEX_LEN_0FC2_P_1 */
  {
    { VEX_W_TABLE (VEX_W_0FC2_P_1) },
    { VEX_W_TABLE (VEX_W_0FC2_P_1) },
  },

  /* VEX_LEN_0FC2_P_3 */
  {
    { VEX_W_TABLE (VEX_W_0FC2_P_3) },
    { VEX_W_TABLE (VEX_W_0FC2_P_3) },
  },

  /* VEX_LEN_0FC4_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0FC4_P_2) },
  },

  /* VEX_LEN_0FC5_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0FC5_P_2) },
  },

  /* VEX_LEN_0FD6_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0FD6_P_2) },
    { VEX_W_TABLE (VEX_W_0FD6_P_2) },
  },

  /* VEX_LEN_0FF7_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0FF7_P_2) },
  },

  /* VEX_LEN_0F3816_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3816_P_2) },
  },

  /* VEX_LEN_0F3819_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3819_P_2) },
  },

  /* VEX_LEN_0F381A_P_2_M_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F381A_P_2_M_0) },
  },

  /* VEX_LEN_0F3836_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3836_P_2) },
  },

  /* VEX_LEN_0F3841_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3841_P_2) },
  },

  /* VEX_LEN_0F385A_P_2_M_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F385A_P_2_M_0) },
  },

  /* VEX_LEN_0F38DB_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F38DB_P_2) },
  },

  /* VEX_LEN_0F38DC_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F38DC_P_2) },
  },

  /* VEX_LEN_0F38DD_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F38DD_P_2) },
  },

  /* VEX_LEN_0F38DE_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F38DE_P_2) },
  },

  /* VEX_LEN_0F38DF_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F38DF_P_2) },
  },

  /* VEX_LEN_0F38F2_P_0 */
  {
    { "andnS",		{ Gdq, VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F3_R_1_P_0 */
  {
    { "blsrS",		{ VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F3_R_2_P_0 */
  {
    { "blsmskS",	{ VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F3_R_3_P_0 */
  {
    { "blsiS",		{ VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F5_P_0 */
  {
    { "bzhiS",		{ Gdq, Edq, VexGdq }, 0 },
  },

  /* VEX_LEN_0F38F5_P_1 */
  {
    { "pextS",		{ Gdq, VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F5_P_3 */
  {
    { "pdepS",		{ Gdq, VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F6_P_3 */
  {
    { "mulxS",		{ Gdq, VexGdq, Edq }, 0 },
  },

  /* VEX_LEN_0F38F7_P_0 */
  {
    { "bextrS",		{ Gdq, Edq, VexGdq }, 0 },
  },

  /* VEX_LEN_0F38F7_P_1 */
  {
    { "sarxS",		{ Gdq, Edq, VexGdq }, 0 },
  },

  /* VEX_LEN_0F38F7_P_2 */
  {
    { "shlxS",		{ Gdq, Edq, VexGdq }, 0 },
  },

  /* VEX_LEN_0F38F7_P_3 */
  {
    { "shrxS",		{ Gdq, Edq, VexGdq }, 0 },
  },

  /* VEX_LEN_0F3A00_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A00_P_2) },
  },

  /* VEX_LEN_0F3A01_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A01_P_2) },
  },

  /* VEX_LEN_0F3A06_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A06_P_2) },
  },

  /* VEX_LEN_0F3A0A_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A0A_P_2) },
    { VEX_W_TABLE (VEX_W_0F3A0A_P_2) },
  },

  /* VEX_LEN_0F3A0B_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A0B_P_2) },
    { VEX_W_TABLE (VEX_W_0F3A0B_P_2) },
  },

  /* VEX_LEN_0F3A14_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A14_P_2) },
  },

  /* VEX_LEN_0F3A15_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A15_P_2) },
  },

  /* VEX_LEN_0F3A16_P_2  */
  {
    { "vpextrK",	{ Edq, XM, Ib }, 0 },
  },

  /* VEX_LEN_0F3A17_P_2 */
  {
    { "vextractps",	{ Edqd, XM, Ib }, 0 },
  },

  /* VEX_LEN_0F3A18_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A18_P_2) },
  },

  /* VEX_LEN_0F3A19_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A19_P_2) },
  },

  /* VEX_LEN_0F3A20_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A20_P_2) },
  },

  /* VEX_LEN_0F3A21_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A21_P_2) },
  },

  /* VEX_LEN_0F3A22_P_2 */
  {
    { "vpinsrK",	{ XM, Vex128, Edq, Ib }, 0 },
  },

  /* VEX_LEN_0F3A30_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A30_P_2_LEN_0) },
  },

  /* VEX_LEN_0F3A31_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A31_P_2_LEN_0) },
  },

  /* VEX_LEN_0F3A32_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A32_P_2_LEN_0) },
  },

  /* VEX_LEN_0F3A33_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A33_P_2_LEN_0) },
  },

  /* VEX_LEN_0F3A38_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A38_P_2) },
  },

  /* VEX_LEN_0F3A39_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A39_P_2) },
  },

  /* VEX_LEN_0F3A41_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A41_P_2) },
  },

  /* VEX_LEN_0F3A44_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A44_P_2) },
  },

  /* VEX_LEN_0F3A46_P_2 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3A46_P_2) },
  },

  /* VEX_LEN_0F3A60_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A60_P_2) },
  },

  /* VEX_LEN_0F3A61_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A61_P_2) },
  },

  /* VEX_LEN_0F3A62_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A62_P_2) },
  },

  /* VEX_LEN_0F3A63_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3A63_P_2) },
  },

  /* VEX_LEN_0F3A6A_P_2 */
  {
    { "vfmaddss",	{ XMVexW, Vex128, EXdVexW, EXdVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A6B_P_2 */
  {
    { "vfmaddsd",	{ XMVexW, Vex128, EXqVexW, EXqVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A6E_P_2 */
  {
    { "vfmsubss",	{ XMVexW, Vex128, EXdVexW, EXdVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A6F_P_2 */
  {
    { "vfmsubsd",	{ XMVexW, Vex128, EXqVexW, EXqVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A7A_P_2 */
  {
    { "vfnmaddss",	{ XMVexW, Vex128, EXdVexW, EXdVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A7B_P_2 */
  {
    { "vfnmaddsd",	{ XMVexW, Vex128, EXqVexW, EXqVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A7E_P_2 */
  {
    { "vfnmsubss",	{ XMVexW, Vex128, EXdVexW, EXdVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3A7F_P_2 */
  {
    { "vfnmsubsd",	{ XMVexW, Vex128, EXqVexW, EXqVexW, VexI4 }, 0 },
  },

  /* VEX_LEN_0F3ADF_P_2 */
  {
    { VEX_W_TABLE (VEX_W_0F3ADF_P_2) },
  },

  /* VEX_LEN_0F3AF0_P_3 */
  {
    { "rorxS",		{ Gdq, Edq, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_CC */
  {
     { "vpcomb",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_CD */
  {
     { "vpcomw",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_CE */
  {
     { "vpcomd",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_CF */
  {
     { "vpcomq",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_EC */
  {
     { "vpcomub",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_ED */
  {
     { "vpcomuw",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_EE */
  {
     { "vpcomud",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_08_EF */
  {
     { "vpcomuq",	{ XM, Vex128, EXx, Ib }, 0 },
  },

  /* VEX_LEN_0FXOP_09_80 */
  {
    { "vfrczps",	{ XM, EXxmm }, 0 },
    { "vfrczps",	{ XM, EXymmq }, 0 },
  },

  /* VEX_LEN_0FXOP_09_81 */
  {
    { "vfrczpd",	{ XM, EXxmm }, 0 },
    { "vfrczpd",	{ XM, EXymmq }, 0 },
  },
};

static const struct dis386 vex_w_table[][2] = {
  {
    /* VEX_W_0F10_P_0 */
    { "vmovups",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F10_P_1 */
    { "vmovss",		{ XMVexScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F10_P_2 */
    { "vmovupd",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F10_P_3 */
    { "vmovsd",		{ XMVexScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F11_P_0 */
    { "vmovups",	{ EXxS, XM }, 0 },
  },
  {
    /* VEX_W_0F11_P_1 */
    { "vmovss",		{ EXdVexScalarS, VexScalar, XMScalar }, 0 },
  },
  {
    /* VEX_W_0F11_P_2 */
    { "vmovupd",	{ EXxS, XM }, 0 },
  },
  {
    /* VEX_W_0F11_P_3 */
    { "vmovsd",		{ EXqVexScalarS, VexScalar, XMScalar }, 0 },
  },
  {
    /* VEX_W_0F12_P_0_M_0 */
    { "vmovlps",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F12_P_0_M_1 */
    { "vmovhlps",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F12_P_1 */
    { "vmovsldup",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F12_P_2 */
    { "vmovlpd",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F12_P_3 */
    { "vmovddup",	{ XM, EXymmq }, 0 },
  },
  {
    /* VEX_W_0F13_M_0 */
    { "vmovlpX",	{ EXq, XM }, 0 },
  },
  {
    /* VEX_W_0F14 */
    { "vunpcklpX",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F15 */
    { "vunpckhpX",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F16_P_0_M_0 */
    { "vmovhps",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F16_P_0_M_1 */
    { "vmovlhps",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F16_P_1 */
    { "vmovshdup",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F16_P_2 */
    { "vmovhpd",	{ XM, Vex128, EXq }, 0 },
  },
  {
    /* VEX_W_0F17_M_0 */
    { "vmovhpX",	{ EXq, XM }, 0 },
  },
  {
    /* VEX_W_0F28 */
    { "vmovapX",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F29 */
    { "vmovapX",	{ EXxS, XM }, 0 },
  },
  {
    /* VEX_W_0F2B_M_0 */
    { "vmovntpX",	{ Mx, XM }, 0 },
  },
  {
    /* VEX_W_0F2E_P_0 */
    { "vucomiss",	{ XMScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F2E_P_2 */
    { "vucomisd",	{ XMScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F2F_P_0 */
    { "vcomiss",	{ XMScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F2F_P_2 */
    { "vcomisd",	{ XMScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F41_P_0_LEN_1 */
    { "kandw",          { MaskG, MaskVex, MaskR }, 0 },
    { "kandq",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F41_P_2_LEN_1 */
    { "kandb",          { MaskG, MaskVex, MaskR }, 0 },
    { "kandd",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F42_P_0_LEN_1 */
    { "kandnw",         { MaskG, MaskVex, MaskR }, 0 },
    { "kandnq",         { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F42_P_2_LEN_1 */
    { "kandnb",         { MaskG, MaskVex, MaskR }, 0 },
    { "kandnd",         { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F44_P_0_LEN_0 */
    { "knotw",		{ MaskG, MaskR }, 0 },
    { "knotq",		{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0F44_P_2_LEN_0 */
    { "knotb",		{ MaskG, MaskR }, 0 },
    { "knotd",		{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0F45_P_0_LEN_1 */
    { "korw",           { MaskG, MaskVex, MaskR }, 0 },
    { "korq",           { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F45_P_2_LEN_1 */
    { "korb",           { MaskG, MaskVex, MaskR }, 0 },
    { "kord",           { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F46_P_0_LEN_1 */
    { "kxnorw",         { MaskG, MaskVex, MaskR }, 0 },
    { "kxnorq",         { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F46_P_2_LEN_1 */
    { "kxnorb",         { MaskG, MaskVex, MaskR }, 0 },
    { "kxnord",         { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F47_P_0_LEN_1 */
    { "kxorw",          { MaskG, MaskVex, MaskR }, 0 },
    { "kxorq",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F47_P_2_LEN_1 */
    { "kxorb",          { MaskG, MaskVex, MaskR }, 0 },
    { "kxord",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F4A_P_0_LEN_1 */
    { "kaddw",          { MaskG, MaskVex, MaskR }, 0 },
    { "kaddq",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F4A_P_2_LEN_1 */
    { "kaddb",          { MaskG, MaskVex, MaskR }, 0 },
    { "kaddd",          { MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F4B_P_0_LEN_1 */
    { "kunpckwd",	{ MaskG, MaskVex, MaskR }, 0 },
    { "kunpckdq",	{ MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F4B_P_2_LEN_1 */
    { "kunpckbw",	{ MaskG, MaskVex, MaskR }, 0 },
  },
  {
    /* VEX_W_0F50_M_0 */
    { "vmovmskpX",	{ Gdq, XS }, 0 },
  },
  {
    /* VEX_W_0F51_P_0 */
    { "vsqrtps",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F51_P_1 */
    { "vsqrtss",	{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F51_P_2  */
    { "vsqrtpd",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F51_P_3 */
    { "vsqrtsd",	{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F52_P_0 */
    { "vrsqrtps",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F52_P_1 */
    { "vrsqrtss",	{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F53_P_0  */
    { "vrcpps",		{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F53_P_1  */
    { "vrcpss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F58_P_0  */
    { "vaddps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F58_P_1  */
    { "vaddss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F58_P_2  */
    { "vaddpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F58_P_3  */
    { "vaddsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F59_P_0  */
    { "vmulps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F59_P_1  */
    { "vmulss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F59_P_2  */
    { "vmulpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F59_P_3  */
    { "vmulsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F5A_P_0  */
    { "vcvtps2pd",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F5A_P_1  */
    { "vcvtss2sd",	{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F5A_P_3  */
    { "vcvtsd2ss",	{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F5B_P_0  */
    { "vcvtdq2ps",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F5B_P_1  */
    { "vcvttps2dq",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F5B_P_2  */
    { "vcvtps2dq",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F5C_P_0  */
    { "vsubps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5C_P_1  */
    { "vsubss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F5C_P_2  */
    { "vsubpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5C_P_3  */
    { "vsubsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F5D_P_0  */
    { "vminps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5D_P_1  */
    { "vminss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F5D_P_2  */
    { "vminpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5D_P_3  */
    { "vminsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F5E_P_0  */
    { "vdivps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5E_P_1  */
    { "vdivss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F5E_P_2  */
    { "vdivpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5E_P_3  */
    { "vdivsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F5F_P_0  */
    { "vmaxps",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5F_P_1  */
    { "vmaxss",		{ XMScalar, VexScalar, EXdScalar }, 0 },
  },
  {
    /* VEX_W_0F5F_P_2  */
    { "vmaxpd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F5F_P_3  */
    { "vmaxsd",		{ XMScalar, VexScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F60_P_2  */
    { "vpunpcklbw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F61_P_2  */
    { "vpunpcklwd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F62_P_2  */
    { "vpunpckldq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F63_P_2  */
    { "vpacksswb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F64_P_2  */
    { "vpcmpgtb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F65_P_2  */
    { "vpcmpgtw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F66_P_2  */
    { "vpcmpgtd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F67_P_2  */
    { "vpackuswb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F68_P_2  */
    { "vpunpckhbw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F69_P_2  */
    { "vpunpckhwd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F6A_P_2  */
    { "vpunpckhdq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F6B_P_2  */
    { "vpackssdw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F6C_P_2  */
    { "vpunpcklqdq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F6D_P_2  */
    { "vpunpckhqdq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F6F_P_1  */
    { "vmovdqu",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F6F_P_2  */
    { "vmovdqa",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F70_P_1 */
    { "vpshufhw",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F70_P_2 */
    { "vpshufd",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F70_P_3 */
    { "vpshuflw",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F71_R_2_P_2  */
    { "vpsrlw",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F71_R_4_P_2  */
    { "vpsraw",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F71_R_6_P_2  */
    { "vpsllw",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F72_R_2_P_2  */
    { "vpsrld",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F72_R_4_P_2  */
    { "vpsrad",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F72_R_6_P_2  */
    { "vpslld",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F73_R_2_P_2  */
    { "vpsrlq",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F73_R_3_P_2  */
    { "vpsrldq",	{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F73_R_6_P_2  */
    { "vpsllq",		{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F73_R_7_P_2  */
    { "vpslldq",	{ Vex, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0F74_P_2 */
    { "vpcmpeqb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F75_P_2 */
    { "vpcmpeqw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F76_P_2 */
    { "vpcmpeqd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F77_P_0 */
    { "",		{ VZERO }, 0 },
  },
  {
    /* VEX_W_0F7C_P_2 */
    { "vhaddpd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F7C_P_3 */
    { "vhaddps",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F7D_P_2 */
    { "vhsubpd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F7D_P_3 */
    { "vhsubps",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F7E_P_1 */
    { "vmovq",		{ XMScalar, EXqScalar }, 0 },
  },
  {
    /* VEX_W_0F7F_P_1 */
    { "vmovdqu",	{ EXxS, XM }, 0 },
  },
  {
    /* VEX_W_0F7F_P_2 */
    { "vmovdqa",	{ EXxS, XM }, 0 },
  },
  {
    /* VEX_W_0F90_P_0_LEN_0 */
    { "kmovw",		{ MaskG, MaskE }, 0 },
    { "kmovq",		{ MaskG, MaskE }, 0 },
  },
  {
    /* VEX_W_0F90_P_2_LEN_0 */
    { "kmovb",		{ MaskG, MaskBDE }, 0 },
    { "kmovd",		{ MaskG, MaskBDE }, 0 },
  },
  {
    /* VEX_W_0F91_P_0_LEN_0 */
    { "kmovw",		{ Ew, MaskG }, 0 },
    { "kmovq",		{ Eq, MaskG }, 0 },
  },
  {
    /* VEX_W_0F91_P_2_LEN_0 */
    { "kmovb",		{ Eb, MaskG }, 0 },
    { "kmovd",		{ Ed, MaskG }, 0 },
  },
  {
    /* VEX_W_0F92_P_0_LEN_0 */
    { "kmovw",		{ MaskG, Rdq }, 0 },
  },
  {
    /* VEX_W_0F92_P_2_LEN_0 */
    { "kmovb",		{ MaskG, Rdq }, 0 },
  },
  {
    /* VEX_W_0F92_P_3_LEN_0 */
    { "kmovd",		{ MaskG, Rdq }, 0 },
    { "kmovq",		{ MaskG, Rdq }, 0 },
  },
  {
    /* VEX_W_0F93_P_0_LEN_0 */
    { "kmovw",		{ Gdq, MaskR }, 0 },
  },
  {
    /* VEX_W_0F93_P_2_LEN_0 */
    { "kmovb",		{ Gdq, MaskR }, 0 },
  },
  {
    /* VEX_W_0F93_P_3_LEN_0 */
    { "kmovd",		{ Gdq, MaskR }, 0 },
    { "kmovq",		{ Gdq, MaskR }, 0 },
  },
  {
    /* VEX_W_0F98_P_0_LEN_0 */
    { "kortestw",	{ MaskG, MaskR }, 0 },
    { "kortestq",	{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0F98_P_2_LEN_0 */
    { "kortestb",	{ MaskG, MaskR }, 0 },
    { "kortestd",	{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0F99_P_0_LEN_0 */
    { "ktestw",	{ MaskG, MaskR }, 0 },
    { "ktestq",	{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0F99_P_2_LEN_0 */
    { "ktestb",	{ MaskG, MaskR }, 0 },
    { "ktestd",	{ MaskG, MaskR }, 0 },
  },
  {
    /* VEX_W_0FAE_R_2_M_0 */
    { "vldmxcsr",	{ Md }, 0 },
  },
  {
    /* VEX_W_0FAE_R_3_M_0 */
    { "vstmxcsr",	{ Md }, 0 },
  },
  {
    /* VEX_W_0FC2_P_0 */
    { "vcmpps",		{ XM, Vex, EXx, VCMP }, 0 },
  },
  {
    /* VEX_W_0FC2_P_1 */
    { "vcmpss",		{ XMScalar, VexScalar, EXdScalar, VCMP }, 0 },
  },
  {
    /* VEX_W_0FC2_P_2 */
    { "vcmppd",		{ XM, Vex, EXx, VCMP }, 0 },
  },
  {
    /* VEX_W_0FC2_P_3 */
    { "vcmpsd",		{ XMScalar, VexScalar, EXqScalar, VCMP }, 0 },
  },
  {
    /* VEX_W_0FC4_P_2 */
    { "vpinsrw",	{ XM, Vex128, Edqw, Ib }, 0 },
  },
  {
    /* VEX_W_0FC5_P_2 */
    { "vpextrw",	{ Gdq, XS, Ib }, 0 },
  },
  {
    /* VEX_W_0FD0_P_2 */
    { "vaddsubpd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FD0_P_3 */
    { "vaddsubps",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FD1_P_2 */
    { "vpsrlw",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FD2_P_2 */
    { "vpsrld",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FD3_P_2 */
    { "vpsrlq",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FD4_P_2 */
    { "vpaddq",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FD5_P_2 */
    { "vpmullw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FD6_P_2 */
    { "vmovq",		{ EXqScalarS, XMScalar }, 0 },
  },
  {
    /* VEX_W_0FD7_P_2_M_1 */
    { "vpmovmskb",	{ Gdq, XS }, 0 },
  },
  {
    /* VEX_W_0FD8_P_2 */
    { "vpsubusb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FD9_P_2 */
    { "vpsubusw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDA_P_2 */
    { "vpminub",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDB_P_2 */
    { "vpand",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDC_P_2 */
    { "vpaddusb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDD_P_2 */
    { "vpaddusw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDE_P_2 */
    { "vpmaxub",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FDF_P_2 */
    { "vpandn",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE0_P_2  */
    { "vpavgb",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE1_P_2  */
    { "vpsraw",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FE2_P_2  */
    { "vpsrad",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FE3_P_2  */
    { "vpavgw",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE4_P_2  */
    { "vpmulhuw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE5_P_2  */
    { "vpmulhw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE6_P_1  */
    { "vcvtdq2pd",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0FE6_P_2  */
    { "vcvttpd2dq%XY",	{ XMM, EXx }, 0 },
  },
  {
    /* VEX_W_0FE6_P_3  */
    { "vcvtpd2dq%XY",	{ XMM, EXx }, 0 },
  },
  {
    /* VEX_W_0FE7_P_2_M_0 */
    { "vmovntdq",	{ Mx, XM }, 0 },
  },
  {
    /* VEX_W_0FE8_P_2  */
    { "vpsubsb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FE9_P_2  */
    { "vpsubsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FEA_P_2  */
    { "vpminsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FEB_P_2  */
    { "vpor",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FEC_P_2  */
    { "vpaddsb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FED_P_2  */
    { "vpaddsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FEE_P_2  */
    { "vpmaxsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FEF_P_2  */
    { "vpxor",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FF0_P_3_M_0 */
    { "vlddqu",		{ XM, M }, 0 },
  },
  {
    /* VEX_W_0FF1_P_2 */
    { "vpsllw",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FF2_P_2 */
    { "vpslld",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FF3_P_2 */
    { "vpsllq",		{ XM, Vex, EXxmm }, 0 },
  },
  {
    /* VEX_W_0FF4_P_2 */
    { "vpmuludq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FF5_P_2 */
    { "vpmaddwd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FF6_P_2 */
    { "vpsadbw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FF7_P_2 */
    { "vmaskmovdqu",	{ XM, XS }, 0 },
  },
  {
    /* VEX_W_0FF8_P_2 */
    { "vpsubb",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FF9_P_2 */
    { "vpsubw",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FFA_P_2 */
    { "vpsubd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FFB_P_2 */
    { "vpsubq",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FFC_P_2 */
    { "vpaddb",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FFD_P_2 */
    { "vpaddw",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0FFE_P_2 */
    { "vpaddd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3800_P_2  */
    { "vpshufb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3801_P_2  */
    { "vphaddw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3802_P_2  */
    { "vphaddd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3803_P_2  */
    { "vphaddsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3804_P_2  */
    { "vpmaddubsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3805_P_2  */
    { "vphsubw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3806_P_2  */
    { "vphsubd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3807_P_2  */
    { "vphsubsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3808_P_2  */
    { "vpsignb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3809_P_2  */
    { "vpsignw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F380A_P_2  */
    { "vpsignd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F380B_P_2  */
    { "vpmulhrsw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F380C_P_2  */
    { "vpermilps",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F380D_P_2  */
    { "vpermilpd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F380E_P_2  */
    { "vtestps",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F380F_P_2  */
    { "vtestpd",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F3816_P_2  */
    { "vpermps",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3817_P_2 */
    { "vptest",		{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F3818_P_2 */
    { "vbroadcastss",	{ XM, EXxmm_md }, 0 },
  },
  {
    /* VEX_W_0F3819_P_2 */
    { "vbroadcastsd",	{ XM, EXxmm_mq }, 0 },
  },
  {
    /* VEX_W_0F381A_P_2_M_0 */
    { "vbroadcastf128",	{ XM, Mxmm }, 0 },
  },
  {
    /* VEX_W_0F381C_P_2 */
    { "vpabsb",		{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F381D_P_2 */
    { "vpabsw",		{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F381E_P_2 */
    { "vpabsd",		{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F3820_P_2 */
    { "vpmovsxbw",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3821_P_2 */
    { "vpmovsxbd",	{ XM, EXxmmqd }, 0 },
  },
  {
    /* VEX_W_0F3822_P_2 */
    { "vpmovsxbq",	{ XM, EXxmmdw }, 0 },
  },
  {
    /* VEX_W_0F3823_P_2 */
    { "vpmovsxwd",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3824_P_2 */
    { "vpmovsxwq",	{ XM, EXxmmqd }, 0 },
  },
  {
    /* VEX_W_0F3825_P_2 */
    { "vpmovsxdq",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3828_P_2 */
    { "vpmuldq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3829_P_2 */
    { "vpcmpeqq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F382A_P_2_M_0 */
    { "vmovntdqa",	{ XM, Mx }, 0 },
  },
  {
    /* VEX_W_0F382B_P_2 */
    { "vpackusdw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F382C_P_2_M_0 */
    { "vmaskmovps",	{ XM, Vex, Mx }, 0 },
  },
  {
    /* VEX_W_0F382D_P_2_M_0 */
    { "vmaskmovpd",	{ XM, Vex, Mx }, 0 },
  },
  {
    /* VEX_W_0F382E_P_2_M_0 */
    { "vmaskmovps",	{ Mx, Vex, XM }, 0 },
  },
  {
    /* VEX_W_0F382F_P_2_M_0 */
    { "vmaskmovpd",	{ Mx, Vex, XM }, 0 },
  },
  {
    /* VEX_W_0F3830_P_2 */
    { "vpmovzxbw",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3831_P_2 */
    { "vpmovzxbd",	{ XM, EXxmmqd }, 0 },
  },
  {
    /* VEX_W_0F3832_P_2 */
    { "vpmovzxbq",	{ XM, EXxmmdw }, 0 },
  },
  {
    /* VEX_W_0F3833_P_2 */
    { "vpmovzxwd",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3834_P_2 */
    { "vpmovzxwq",	{ XM, EXxmmqd }, 0 },
  },
  {
    /* VEX_W_0F3835_P_2 */
    { "vpmovzxdq",	{ XM, EXxmmq }, 0 },
  },
  {
    /* VEX_W_0F3836_P_2  */
    { "vpermd",		{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3837_P_2 */
    { "vpcmpgtq",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3838_P_2 */
    { "vpminsb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3839_P_2 */
    { "vpminsd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383A_P_2 */
    { "vpminuw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383B_P_2 */
    { "vpminud",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383C_P_2 */
    { "vpmaxsb",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383D_P_2 */
    { "vpmaxsd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383E_P_2 */
    { "vpmaxuw",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F383F_P_2 */
    { "vpmaxud",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3840_P_2 */
    { "vpmulld",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3841_P_2 */
    { "vphminposuw",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F3846_P_2 */
    { "vpsravd",	{ XM, Vex, EXx }, 0 },
  },
  {
    /* VEX_W_0F3858_P_2 */
    { "vpbroadcastd", { XM, EXxmm_md }, 0 },
  },
  {
    /* VEX_W_0F3859_P_2 */
    { "vpbroadcastq",	{ XM, EXxmm_mq }, 0 },
  },
  {
    /* VEX_W_0F385A_P_2_M_0 */
    { "vbroadcasti128", { XM, Mxmm }, 0 },
  },
  {
    /* VEX_W_0F3878_P_2 */
    { "vpbroadcastb",	{ XM, EXxmm_mb }, 0 },
  },
  {
    /* VEX_W_0F3879_P_2 */
    { "vpbroadcastw",	{ XM, EXxmm_mw }, 0 },
  },
  {
    /* VEX_W_0F38DB_P_2 */
    { "vaesimc",	{ XM, EXx }, 0 },
  },
  {
    /* VEX_W_0F38DC_P_2 */
    { "vaesenc",	{ XM, Vex128, EXx }, 0 },
  },
  {
    /* VEX_W_0F38DD_P_2 */
    { "vaesenclast",	{ XM, Vex128, EXx }, 0 },
  },
  {
    /* VEX_W_0F38DE_P_2 */
    { "vaesdec",	{ XM, Vex128, EXx }, 0 },
  },
  {
    /* VEX_W_0F38DF_P_2 */
    { "vaesdeclast",	{ XM, Vex128, EXx }, 0 },
  },
  {
    /* VEX_W_0F3A00_P_2 */
    { Bad_Opcode },
    { "vpermq",		{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A01_P_2 */
    { Bad_Opcode },
    { "vpermpd",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A02_P_2 */
    { "vpblendd",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A04_P_2 */
    { "vpermilps",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A05_P_2 */
    { "vpermilpd",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A06_P_2 */
    { "vperm2f128",	{ XM, Vex256, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A08_P_2 */
    { "vroundps",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A09_P_2 */
    { "vroundpd",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0A_P_2 */
    { "vroundss",	{ XMScalar, VexScalar, EXdScalar, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0B_P_2 */
    { "vroundsd",	{ XMScalar, VexScalar, EXqScalar, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0C_P_2 */
    { "vblendps",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0D_P_2 */
    { "vblendpd",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0E_P_2 */
    { "vpblendw",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A0F_P_2 */
    { "vpalignr",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A14_P_2 */
    { "vpextrb",	{ Edqb, XM, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A15_P_2 */
    { "vpextrw",	{ Edqw, XM, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A18_P_2 */
    { "vinsertf128",	{ XM, Vex256, EXxmm, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A19_P_2 */
    { "vextractf128",	{ EXxmm, XM, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A20_P_2 */
    { "vpinsrb",	{ XM, Vex128, Edqb, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A21_P_2 */
    { "vinsertps",	{ XM, Vex128, EXd, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A30_P_2_LEN_0 */
    { "kshiftrb",	{ MaskG, MaskR, Ib }, 0 },
    { "kshiftrw",	{ MaskG, MaskR, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A31_P_2_LEN_0 */
    { "kshiftrd",	{ MaskG, MaskR, Ib }, 0 },
    { "kshiftrq",	{ MaskG, MaskR, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A32_P_2_LEN_0 */
    { "kshiftlb",	{ MaskG, MaskR, Ib }, 0 },
    { "kshiftlw",	{ MaskG, MaskR, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A33_P_2_LEN_0 */
    { "kshiftld",	{ MaskG, MaskR, Ib }, 0 },
    { "kshiftlq",	{ MaskG, MaskR, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A38_P_2 */
    { "vinserti128",	{ XM, Vex256, EXxmm, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A39_P_2 */
    { "vextracti128",	{ EXxmm, XM, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A40_P_2 */
    { "vdpps",		{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A41_P_2 */
    { "vdppd",		{ XM, Vex128, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A42_P_2 */
    { "vmpsadbw",	{ XM, Vex, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A44_P_2 */
    { "vpclmulqdq",	{ XM, Vex128, EXx, PCLMUL }, 0 },
  },
  {
    /* VEX_W_0F3A46_P_2 */
    { "vperm2i128",	{ XM, Vex256, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A48_P_2 */
    { "vpermil2ps",	{ XMVexW, Vex, EXVexImmW, EXVexImmW, EXVexImmW }, 0 },
    { "vpermil2ps",	{ XMVexW, Vex, EXVexImmW, EXVexImmW, EXVexImmW }, 0 },
  },
  {
    /* VEX_W_0F3A49_P_2 */
    { "vpermil2pd",	{ XMVexW, Vex, EXVexImmW, EXVexImmW, EXVexImmW }, 0 },
    { "vpermil2pd",	{ XMVexW, Vex, EXVexImmW, EXVexImmW, EXVexImmW }, 0 },
  },
  {
    /* VEX_W_0F3A4A_P_2 */
    { "vblendvps",	{ XM, Vex, EXx, XMVexI4 }, 0 },
  },
  {
    /* VEX_W_0F3A4B_P_2 */
    { "vblendvpd",	{ XM, Vex, EXx, XMVexI4 }, 0 },
  },
  {
    /* VEX_W_0F3A4C_P_2 */
    { "vpblendvb",	{ XM, Vex, EXx, XMVexI4 }, 0 },
  },
  {
    /* VEX_W_0F3A60_P_2 */
    { "vpcmpestrm",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A61_P_2 */
    { "vpcmpestri",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A62_P_2 */
    { "vpcmpistrm",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3A63_P_2 */
    { "vpcmpistri",	{ XM, EXx, Ib }, 0 },
  },
  {
    /* VEX_W_0F3ADF_P_2 */
    { "vaeskeygenassist", { XM, EXx, Ib }, 0 },
  },
#define NEED_VEX_W_TABLE
#include "i386-dis-evex.h"
#undef NEED_VEX_W_TABLE
};

static const struct dis386 mod_table[][2] = {
  {
    /* MOD_8D */
    { "leaS",		{ Gv, M }, 0 },
  },
  {
    /* MOD_C6_REG_7 */
    { Bad_Opcode },
    { RM_TABLE (RM_C6_REG_7) },
  },
  {
    /* MOD_C7_REG_7 */
    { Bad_Opcode },
    { RM_TABLE (RM_C7_REG_7) },
  },
  {
    /* MOD_FF_REG_3 */
    { "Jcall^", { indirEp }, 0 },
  },
  {
    /* MOD_FF_REG_5 */
    { "Jjmp^", { indirEp }, 0 },
  },
  {
    /* MOD_0F01_REG_0 */
    { X86_64_TABLE (X86_64_0F01_REG_0) },
    { RM_TABLE (RM_0F01_REG_0) },
  },
  {
    /* MOD_0F01_REG_1 */
    { X86_64_TABLE (X86_64_0F01_REG_1) },
    { RM_TABLE (RM_0F01_REG_1) },
  },
  {
    /* MOD_0F01_REG_2 */
    { X86_64_TABLE (X86_64_0F01_REG_2) },
    { RM_TABLE (RM_0F01_REG_2) },
  },
  {
    /* MOD_0F01_REG_3 */
    { X86_64_TABLE (X86_64_0F01_REG_3) },
    { RM_TABLE (RM_0F01_REG_3) },
  },
  {
    /* MOD_0F01_REG_7 */
    { "invlpg",		{ Mb }, 0 },
    { RM_TABLE (RM_0F01_REG_7) },
  },
  {
    /* MOD_0F12_PREFIX_0 */
    { "movlps",		{ XM, EXq }, PREFIX_OPCODE },
    { "movhlps",	{ XM, EXq }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F13 */
    { "movlpX",		{ EXq, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F16_PREFIX_0 */
    { "movhps",		{ XM, EXq }, 0 },
    { "movlhps",	{ XM, EXq }, 0 },
  },
  {
    /* MOD_0F17 */
    { "movhpX",		{ EXq, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F18_REG_0 */
    { "prefetchnta",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_1 */
    { "prefetcht0",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_2 */
    { "prefetcht1",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_3 */
    { "prefetcht2",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_4 */
    { "nop/reserved",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_5 */
    { "nop/reserved",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_6 */
    { "nop/reserved",	{ Mb }, 0 },
  },
  {
    /* MOD_0F18_REG_7 */
    { "nop/reserved",	{ Mb }, 0 },
  },
  {
    /* MOD_0F1A_PREFIX_0 */
    { "bndldx",		{ Gbnd, Ev_bnd }, 0 },
    { "nopQ",		{ Ev }, 0 },
  },
  {
    /* MOD_0F1B_PREFIX_0 */
    { "bndstx",		{ Ev_bnd, Gbnd }, 0 },
    { "nopQ",		{ Ev }, 0 },
  },
  {
    /* MOD_0F1B_PREFIX_1 */
    { "bndmk",		{ Gbnd, Ev_bnd }, 0 },
    { "nopQ",		{ Ev }, 0 },
  },
  {
    /* MOD_0F24 */
    { Bad_Opcode },
    { "movL",		{ Rd, Td }, 0 },
  },
  {
    /* MOD_0F26 */
    { Bad_Opcode },
    { "movL",		{ Td, Rd }, 0 },
  },
  {
    /* MOD_0F2B_PREFIX_0 */
    {"movntps",		{ Mx, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F2B_PREFIX_1 */
    {"movntss",		{ Md, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F2B_PREFIX_2 */
    {"movntpd",		{ Mx, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F2B_PREFIX_3 */
    {"movntsd",		{ Mq, XM }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F51 */
    { Bad_Opcode },
    { "movmskpX",	{ Gdq, XS }, PREFIX_OPCODE },
  },
  {
    /* MOD_0F71_REG_2 */
    { Bad_Opcode },
    { "psrlw",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F71_REG_4 */
    { Bad_Opcode },
    { "psraw",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F71_REG_6 */
    { Bad_Opcode },
    { "psllw",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F72_REG_2 */
    { Bad_Opcode },
    { "psrld",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F72_REG_4 */
    { Bad_Opcode },
    { "psrad",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F72_REG_6 */
    { Bad_Opcode },
    { "pslld",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F73_REG_2 */
    { Bad_Opcode },
    { "psrlq",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F73_REG_3 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F73_REG_3) },
  },
  {
    /* MOD_0F73_REG_6 */
    { Bad_Opcode },
    { "psllq",		{ MS, Ib }, 0 },
  },
  {
    /* MOD_0F73_REG_7 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_0F73_REG_7) },
  },
  {
    /* MOD_0FAE_REG_0 */
    { "fxsave",		{ FXSAVE }, 0 },
    { PREFIX_TABLE (PREFIX_0FAE_REG_0) },
  },
  {
    /* MOD_0FAE_REG_1 */
    { "fxrstor",	{ FXSAVE }, 0 },
    { PREFIX_TABLE (PREFIX_0FAE_REG_1) },
  },
  {
    /* MOD_0FAE_REG_2 */
    { "ldmxcsr",	{ Md }, 0 },
    { PREFIX_TABLE (PREFIX_0FAE_REG_2) },
  },
  {
    /* MOD_0FAE_REG_3 */
    { "stmxcsr",	{ Md }, 0 },
    { PREFIX_TABLE (PREFIX_0FAE_REG_3) },
  },
  {
    /* MOD_0FAE_REG_4 */
    { "xsave",		{ FXSAVE }, 0 },
  },
  {
    /* MOD_0FAE_REG_5 */
    { "xrstor",		{ FXSAVE }, 0 },
    { RM_TABLE (RM_0FAE_REG_5) },
  },
  {
    /* MOD_0FAE_REG_6 */
    { PREFIX_TABLE (PREFIX_0FAE_REG_6) },
    { RM_TABLE (RM_0FAE_REG_6) },
  },
  {
    /* MOD_0FAE_REG_7 */
    { PREFIX_TABLE (PREFIX_0FAE_REG_7) },
    { RM_TABLE (RM_0FAE_REG_7) },
  },
  {
    /* MOD_0FB2 */
    { "lssS",		{ Gv, Mp }, 0 },
  },
  {
    /* MOD_0FB4 */
    { "lfsS",		{ Gv, Mp }, 0 },
  },
  {
    /* MOD_0FB5 */
    { "lgsS",		{ Gv, Mp }, 0 },
  },
  {
    /* MOD_0FC7_REG_3 */
    { "xrstors",		{ FXSAVE }, 0 },
  },
  {
    /* MOD_0FC7_REG_4 */
    { "xsavec",		{ FXSAVE }, 0 },
  },
  {
    /* MOD_0FC7_REG_5 */
    { "xsaves",		{ FXSAVE }, 0 },
  },
  {
    /* MOD_0FC7_REG_6 */
    { PREFIX_TABLE (PREFIX_MOD_0_0FC7_REG_6) },
    { PREFIX_TABLE (PREFIX_MOD_3_0FC7_REG_6) }
  },
  {
    /* MOD_0FC7_REG_7 */
    { "vmptrst",	{ Mq }, 0 },
    { PREFIX_TABLE (PREFIX_MOD_3_0FC7_REG_7) }
  },
  {
    /* MOD_0FD7 */
    { Bad_Opcode },
    { "pmovmskb",	{ Gdq, MS }, 0 },
  },
  {
    /* MOD_0FE7_PREFIX_2 */
    { "movntdq",	{ Mx, XM }, 0 },
  },
  {
    /* MOD_0FF0_PREFIX_3 */
    { "lddqu",		{ XM, M }, 0 },
  },
  {
    /* MOD_0F382A_PREFIX_2 */
    { "movntdqa",	{ XM, Mx }, 0 },
  },
  {
    /* MOD_62_32BIT */
    { "bound{S|}",	{ Gv, Ma }, 0 },
    { EVEX_TABLE (EVEX_0F) },
  },
  {
    /* MOD_C4_32BIT */
    { "lesS",		{ Gv, Mp }, 0 },
    { VEX_C4_TABLE (VEX_0F) },
  },
  {
    /* MOD_C5_32BIT */
    { "ldsS",		{ Gv, Mp }, 0 },
    { VEX_C5_TABLE (VEX_0F) },
  },
  {
    /* MOD_VEX_0F12_PREFIX_0 */
    { VEX_LEN_TABLE (VEX_LEN_0F12_P_0_M_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F12_P_0_M_1) },
  },
  {
    /* MOD_VEX_0F13 */
    { VEX_LEN_TABLE (VEX_LEN_0F13_M_0) },
  },
  {
    /* MOD_VEX_0F16_PREFIX_0 */
    { VEX_LEN_TABLE (VEX_LEN_0F16_P_0_M_0) },
    { VEX_LEN_TABLE (VEX_LEN_0F16_P_0_M_1) },
  },
  {
    /* MOD_VEX_0F17 */
    { VEX_LEN_TABLE (VEX_LEN_0F17_M_0) },
  },
  {
    /* MOD_VEX_0F2B */
    { VEX_W_TABLE (VEX_W_0F2B_M_0) },
  },
  {
    /* MOD_VEX_0F50 */
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F50_M_0) },
  },
  {
    /* MOD_VEX_0F71_REG_2 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F71_REG_2) },
  },
  {
    /* MOD_VEX_0F71_REG_4 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F71_REG_4) },
  },
  {
    /* MOD_VEX_0F71_REG_6 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F71_REG_6) },
  },
  {
    /* MOD_VEX_0F72_REG_2 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F72_REG_2) },
  },
  {
    /* MOD_VEX_0F72_REG_4 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F72_REG_4) },
  },
  {
    /* MOD_VEX_0F72_REG_6 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F72_REG_6) },
  },
  {
    /* MOD_VEX_0F73_REG_2 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F73_REG_2) },
  },
  {
    /* MOD_VEX_0F73_REG_3 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F73_REG_3) },
  },
  {
    /* MOD_VEX_0F73_REG_6 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F73_REG_6) },
  },
  {
    /* MOD_VEX_0F73_REG_7 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_VEX_0F73_REG_7) },
  },
  {
    /* MOD_VEX_0FAE_REG_2 */
    { VEX_LEN_TABLE (VEX_LEN_0FAE_R_2_M_0) },
  },
  {
    /* MOD_VEX_0FAE_REG_3 */
    { VEX_LEN_TABLE (VEX_LEN_0FAE_R_3_M_0) },
  },
  {
    /* MOD_VEX_0FD7_PREFIX_2 */
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0FD7_P_2_M_1) },
  },
  {
    /* MOD_VEX_0FE7_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0FE7_P_2_M_0) },
  },
  {
    /* MOD_VEX_0FF0_PREFIX_3 */
    { VEX_W_TABLE (VEX_W_0FF0_P_3_M_0) },
  },
  {
    /* MOD_VEX_0F381A_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_0F381A_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F382A_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0F382A_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F382C_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0F382C_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F382D_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0F382D_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F382E_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0F382E_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F382F_PREFIX_2 */
    { VEX_W_TABLE (VEX_W_0F382F_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F385A_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_0F385A_P_2_M_0) },
  },
  {
    /* MOD_VEX_0F388C_PREFIX_2 */
    { "vpmaskmov%LW",	{ XM, Vex, Mx }, 0 },
  },
  {
    /* MOD_VEX_0F388E_PREFIX_2 */
    { "vpmaskmov%LW",	{ Mx, Vex, XM }, 0 },
  },
#define NEED_MOD_TABLE
#include "i386-dis-evex.h"
#undef NEED_MOD_TABLE
};

static const struct dis386 rm_table[][8] = {
  {
    /* RM_C6_REG_7 */
    { "xabort",		{ Skip_MODRM, Ib }, 0 },
  },
  {
    /* RM_C7_REG_7 */
    { "xbeginT",	{ Skip_MODRM, Jv }, 0 },
  },
  {
    /* RM_0F01_REG_0 */
    { Bad_Opcode },
    { "vmcall",		{ Skip_MODRM }, 0 },
    { "vmlaunch",	{ Skip_MODRM }, 0 },
    { "vmresume",	{ Skip_MODRM }, 0 },
    { "vmxoff",		{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0F01_REG_1 */
    { "monitor",	{ { OP_Monitor, 0 } }, 0 },
    { "mwait",		{ { OP_Mwait, 0 } }, 0 },
    { "clac",		{ Skip_MODRM }, 0 },
    { "stac",		{ Skip_MODRM }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "encls",		{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0F01_REG_2 */
    { "xgetbv",		{ Skip_MODRM }, 0 },
    { "xsetbv",		{ Skip_MODRM }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vmfunc",		{ Skip_MODRM }, 0 },
    { "xend",		{ Skip_MODRM }, 0 },
    { "xtest",		{ Skip_MODRM }, 0 },
    { "enclu",		{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0F01_REG_3 */
    { "vmrun",		{ Skip_MODRM }, 0 },
    { "vmmcall",	{ Skip_MODRM }, 0 },
    { "vmload",		{ Skip_MODRM }, 0 },
    { "vmsave",		{ Skip_MODRM }, 0 },
    { "stgi",		{ Skip_MODRM }, 0 },
    { "clgi",		{ Skip_MODRM }, 0 },
    { "skinit",		{ Skip_MODRM }, 0 },
    { "invlpga",	{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0F01_REG_7 */
    { "swapgs",		{ Skip_MODRM }, 0  },
    { "rdtscp",		{ Skip_MODRM }, 0  },
    { "monitorx",	{ { OP_Monitor, 0 } }, 0  },
    { "mwaitx",		{ { OP_Mwaitx,  0 } }, 0  },
    { "clzero",		{ Skip_MODRM }, 0  },
  },
  {
    /* RM_0FAE_REG_5 */
    { "lfence",		{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0FAE_REG_6 */
    { "mfence",		{ Skip_MODRM }, 0 },
  },
  {
    /* RM_0FAE_REG_7 */
    { PREFIX_TABLE (PREFIX_RM_0_0FAE_REG_7) },
  },
};

#define INTERNAL_DISASSEMBLER_ERROR _("<internal disassembler error>")

/* We use the high bit to indicate different name for the same
   prefix.  */
#define REP_PREFIX	(0xf3 | 0x100)
#define XACQUIRE_PREFIX	(0xf2 | 0x200)
#define XRELEASE_PREFIX	(0xf3 | 0x400)
#define BND_PREFIX	(0xf2 | 0x400)

static int
ckprefix (void)
{
  int newrex, i, length;
  rex = 0;
  rex_ignored = 0;
  prefixes = 0;
  used_prefixes = 0;
  rex_used = 0;
  last_lock_prefix = -1;
  last_repz_prefix = -1;
  last_repnz_prefix = -1;
  last_data_prefix = -1;
  last_addr_prefix = -1;
  last_rex_prefix = -1;
  last_seg_prefix = -1;
  fwait_prefix = -1;
  active_seg_prefix = 0;
  for (i = 0; i < (int) ARRAY_SIZE (all_prefixes); i++)
    all_prefixes[i] = 0;
  i = 0;
  length = 0;
  /* The maximum instruction length is 15bytes.  */
  while (length < MAX_CODE_LENGTH - 1)
    {
      FETCH_DATA (the_info, codep + 1);
      newrex = 0;
      switch (*codep)
	{
	/* REX prefixes family.  */
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4b:
	case 0x4c:
	case 0x4d:
	case 0x4e:
	case 0x4f:
	  if (address_mode == mode_64bit)
	    newrex = *codep;
	  else
	    return 1;
	  last_rex_prefix = i;
	  break;
	case 0xf3:
	  prefixes |= PREFIX_REPZ;
	  last_repz_prefix = i;
	  break;
	case 0xf2:
	  prefixes |= PREFIX_REPNZ;
	  last_repnz_prefix = i;
	  break;
	case 0xf0:
	  prefixes |= PREFIX_LOCK;
	  last_lock_prefix = i;
	  break;
	case 0x2e:
	  prefixes |= PREFIX_CS;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_CS;
	  break;
	case 0x36:
	  prefixes |= PREFIX_SS;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_SS;
	  break;
	case 0x3e:
	  prefixes |= PREFIX_DS;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_DS;
	  break;
	case 0x26:
	  prefixes |= PREFIX_ES;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_ES;
	  break;
	case 0x64:
	  prefixes |= PREFIX_FS;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_FS;
	  break;
	case 0x65:
	  prefixes |= PREFIX_GS;
	  last_seg_prefix = i;
	  active_seg_prefix = PREFIX_GS;
	  break;
	case 0x66:
	  prefixes |= PREFIX_DATA;
	  last_data_prefix = i;
	  break;
	case 0x67:
	  prefixes |= PREFIX_ADDR;
	  last_addr_prefix = i;
	  break;
	case FWAIT_OPCODE:
	  /* fwait is really an instruction.  If there are prefixes
	     before the fwait, they belong to the fwait, *not* to the
	     following instruction.  */
	  fwait_prefix = i;
	  if (prefixes || rex)
	    {
	      prefixes |= PREFIX_FWAIT;
	      codep++;
	      /* This ensures that the previous REX prefixes are noticed
		 as unused prefixes, as in the return case below.  */
	      rex_used = rex;
	      return 1;
	    }
	  prefixes = PREFIX_FWAIT;
	  break;
	default:
	  return 1;
	}
      /* Rex is ignored when followed by another prefix.  */
      if (rex)
	{
	  rex_used = rex;
	  return 1;
	}
      if (*codep != FWAIT_OPCODE)
	all_prefixes[i++] = *codep;
      rex = newrex;
      codep++;
      length++;
    }
  return 0;
}

/* Return the name of the prefix byte PREF, or NULL if PREF is not a
   prefix byte.  */

static const char *
prefix_name (int pref, int sizeflag)
{
  static const char *rexes [16] =
    {
      "rex",		/* 0x40 */
      "rex.B",		/* 0x41 */
      "rex.X",		/* 0x42 */
      "rex.XB",		/* 0x43 */
      "rex.R",		/* 0x44 */
      "rex.RB",		/* 0x45 */
      "rex.RX",		/* 0x46 */
      "rex.RXB",	/* 0x47 */
      "rex.W",		/* 0x48 */
      "rex.WB",		/* 0x49 */
      "rex.WX",		/* 0x4a */
      "rex.WXB",	/* 0x4b */
      "rex.WR",		/* 0x4c */
      "rex.WRB",	/* 0x4d */
      "rex.WRX",	/* 0x4e */
      "rex.WRXB",	/* 0x4f */
    };

  switch (pref)
    {
    /* REX prefixes family.  */
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b:
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
      return rexes [pref - 0x40];
    case 0xf3:
      return "repz";
    case 0xf2:
      return "repnz";
    case 0xf0:
      return "lock";
    case 0x2e:
      return "cs";
    case 0x36:
      return "ss";
    case 0x3e:
      return "ds";
    case 0x26:
      return "es";
    case 0x64:
      return "fs";
    case 0x65:
      return "gs";
    case 0x66:
      return (sizeflag & DFLAG) ? "data16" : "data32";
    case 0x67:
      if (address_mode == mode_64bit)
	return (sizeflag & AFLAG) ? "addr32" : "addr64";
      else
	return (sizeflag & AFLAG) ? "addr16" : "addr32";
    case FWAIT_OPCODE:
      return "fwait";
    case REP_PREFIX:
      return "rep";
    case XACQUIRE_PREFIX:
      return "xacquire";
    case XRELEASE_PREFIX:
      return "xrelease";
    case BND_PREFIX:
      return "bnd";
    default:
      return NULL;
    }
}

static char op_out[MAX_OPERANDS][100];
static int op_ad, op_index[MAX_OPERANDS];
static int two_source_ops;
static bfd_vma op_address[MAX_OPERANDS];
static bfd_vma op_riprel[MAX_OPERANDS];
static bfd_vma start_pc;

/*
 *   On the 386's of 1988, the maximum length of an instruction is 15 bytes.
 *   (see topic "Redundant prefixes" in the "Differences from 8086"
 *   section of the "Virtual 8086 Mode" chapter.)
 * 'pc' should be the address of this instruction, it will
 *   be used to print the target address if this is a relative jump or call
 * The function returns the length of this instruction in bytes.
 */

static char intel_syntax;
static char intel_mnemonic = !SYSV386_COMPAT;
static char open_char;
static char close_char;
static char separator_char;
static char scale_char;

enum x86_64_isa
{
  amd64 = 0,
  intel64
};

static enum x86_64_isa isa64;

/* Here for backwards compatibility.  When gdb stops using
   print_insn_i386_att and print_insn_i386_intel these functions can
   disappear, and print_insn_i386 be merged into print_insn.  */
int
print_insn_i386_att (bfd_vma pc, disassemble_info *info)
{
  intel_syntax = 0;

  return print_insn (pc, info);
}

int
print_insn_i386_intel (bfd_vma pc, disassemble_info *info)
{
  intel_syntax = 1;

  return print_insn (pc, info);
}

int
print_insn_i386 (bfd_vma pc, disassemble_info *info)
{
  intel_syntax = -1;

  return print_insn (pc, info);
}

void
print_i386_disassembler_options (FILE *stream)
{
  fprintf (stream, _("\n\
The following i386/x86-64 specific disassembler options are supported for use\n\
with the -M switch (multiple options should be separated by commas):\n"));

  fprintf (stream, _("  x86-64      Disassemble in 64bit mode\n"));
  fprintf (stream, _("  i386        Disassemble in 32bit mode\n"));
  fprintf (stream, _("  i8086       Disassemble in 16bit mode\n"));
  fprintf (stream, _("  att         Display instruction in AT&T syntax\n"));
  fprintf (stream, _("  intel       Display instruction in Intel syntax\n"));
  fprintf (stream, _("  att-mnemonic\n"
		     "              Display instruction in AT&T mnemonic\n"));
  fprintf (stream, _("  intel-mnemonic\n"
		     "              Display instruction in Intel mnemonic\n"));
  fprintf (stream, _("  addr64      Assume 64bit address size\n"));
  fprintf (stream, _("  addr32      Assume 32bit address size\n"));
  fprintf (stream, _("  addr16      Assume 16bit address size\n"));
  fprintf (stream, _("  data32      Assume 32bit data size\n"));
  fprintf (stream, _("  data16      Assume 16bit data size\n"));
  fprintf (stream, _("  suffix      Always display instruction suffix in AT&T syntax\n"));
  fprintf (stream, _("  amd64       Display instruction in AMD64 ISA\n"));
  fprintf (stream, _("  intel64     Display instruction in Intel64 ISA\n"));
}

/* Bad opcode.  */
static const struct dis386 bad_opcode = { "(bad)", { XX }, 0 };

/* Get a pointer to struct dis386 with a valid name.  */

static const struct dis386 *
get_valid_dis386 (const struct dis386 *dp, disassemble_info *info)
{
  int vindex, vex_table_index;

  if (dp->name != NULL)
    return dp;

  switch (dp->op[0].bytemode)
    {
    case USE_REG_TABLE:
      dp = &reg_table[dp->op[1].bytemode][modrm.reg];
      break;

    case USE_MOD_TABLE:
      vindex = modrm.mod == 0x3 ? 1 : 0;
      dp = &mod_table[dp->op[1].bytemode][vindex];
      break;

    case USE_RM_TABLE:
      dp = &rm_table[dp->op[1].bytemode][modrm.rm];
      break;

    case USE_PREFIX_TABLE:
      if (need_vex)
	{
	  /* The prefix in VEX is implicit.  */
	  switch (vex.prefix)
	    {
	    case 0:
	      vindex = 0;
	      break;
	    case REPE_PREFIX_OPCODE:
	      vindex = 1;
	      break;
	    case DATA_PREFIX_OPCODE:
	      vindex = 2;
	      break;
	    case REPNE_PREFIX_OPCODE:
	      vindex = 3;
	      break;
	    default:
	      abort ();
	      break;
	    }
	}
      else
	{
	  int last_prefix = -1;
	  int prefix = 0;
	  vindex = 0;
	  /* We check PREFIX_REPNZ and PREFIX_REPZ before PREFIX_DATA.
	     When there are multiple PREFIX_REPNZ and PREFIX_REPZ, the
	     last one wins.  */
	  if ((prefixes & (PREFIX_REPZ | PREFIX_REPNZ)) != 0)
	    {
	      if (last_repz_prefix > last_repnz_prefix)
		{
		  vindex = 1;
		  prefix = PREFIX_REPZ;
		  last_prefix = last_repz_prefix;
		}
	      else
		{
		  vindex = 3;
		  prefix = PREFIX_REPNZ;
		  last_prefix = last_repnz_prefix;
		}

	      /* Check if prefix should be ignored.  */
	      if ((((prefix_table[dp->op[1].bytemode][vindex].prefix_requirement
		     & PREFIX_IGNORED) >> PREFIX_IGNORED_SHIFT)
		   & prefix) != 0)
		vindex = 0;
	    }

	  if (vindex == 0 && (prefixes & PREFIX_DATA) != 0)
	    {
	      vindex = 2;
	      prefix = PREFIX_DATA;
	      last_prefix = last_data_prefix;
	    }

	  if (vindex != 0)
	    {
	      used_prefixes |= prefix;
	      all_prefixes[last_prefix] = 0;
	    }
	}
      dp = &prefix_table[dp->op[1].bytemode][vindex];
      break;

    case USE_X86_64_TABLE:
      vindex = address_mode == mode_64bit ? 1 : 0;
      dp = &x86_64_table[dp->op[1].bytemode][vindex];
      break;

    case USE_3BYTE_TABLE:
      FETCH_DATA (info, codep + 2);
      vindex = *codep++;
      dp = &three_byte_table[dp->op[1].bytemode][vindex];
      end_codep = codep;
      modrm.mod = (*codep >> 6) & 3;
      modrm.reg = (*codep >> 3) & 7;
      modrm.rm = *codep & 7;
      break;

    case USE_VEX_LEN_TABLE:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  vindex = 0;
	  break;
	case 256:
	  vindex = 1;
	  break;
	default:
	  abort ();
	  break;
	}

      dp = &vex_len_table[dp->op[1].bytemode][vindex];
      break;

    case USE_XOP_8F_TABLE:
      FETCH_DATA (info, codep + 3);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      rex = ~(*codep >> 5) & 0x7;

      /* VEX_TABLE_INDEX is the mmmmm part of the XOP byte 1 "RCB.mmmmm".  */
      switch ((*codep & 0x1f))
	{
	default:
	  dp = &bad_opcode;
	  return dp;
	case 0x8:
	  vex_table_index = XOP_08;
	  break;
	case 0x9:
	  vex_table_index = XOP_09;
	  break;
	case 0xa:
	  vex_table_index = XOP_0A;
	  break;
	}
      codep++;
      vex.w = *codep & 0x80;
      if (vex.w && address_mode == mode_64bit)
	rex |= REX_W;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit
	  && vex.register_specifier > 0x7)
	{
	  dp = &bad_opcode;
	  return dp;
	}

      vex.length = (*codep & 0x4) ? 256 : 128;
      switch ((*codep & 0x3))
	{
	case 0:
	  vex.prefix = 0;
	  break;
	case 1:
	  vex.prefix = DATA_PREFIX_OPCODE;
	  break;
	case 2:
	  vex.prefix = REPE_PREFIX_OPCODE;
	  break;
	case 3:
	  vex.prefix = REPNE_PREFIX_OPCODE;
	  break;
	}
      need_vex = 1;
      need_vex_reg = 1;
      codep++;
      vindex = *codep++;
      dp = &xop_table[vex_table_index][vindex];

      end_codep = codep;
      FETCH_DATA (info, codep + 1);
      modrm.mod = (*codep >> 6) & 3;
      modrm.reg = (*codep >> 3) & 7;
      modrm.rm = *codep & 7;
      break;

    case USE_VEX_C4_TABLE:
      /* VEX prefix.  */
      FETCH_DATA (info, codep + 3);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      rex = ~(*codep >> 5) & 0x7;
      switch ((*codep & 0x1f))
	{
	default:
	  dp = &bad_opcode;
	  return dp;
	case 0x1:
	  vex_table_index = VEX_0F;
	  break;
	case 0x2:
	  vex_table_index = VEX_0F38;
	  break;
	case 0x3:
	  vex_table_index = VEX_0F3A;
	  break;
	}
      codep++;
      vex.w = *codep & 0x80;
      if (vex.w && address_mode == mode_64bit)
	rex |= REX_W;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit
	  && vex.register_specifier > 0x7)
	{
	  dp = &bad_opcode;
	  return dp;
	}

      vex.length = (*codep & 0x4) ? 256 : 128;
      switch ((*codep & 0x3))
	{
	case 0:
	  vex.prefix = 0;
	  break;
	case 1:
	  vex.prefix = DATA_PREFIX_OPCODE;
	  break;
	case 2:
	  vex.prefix = REPE_PREFIX_OPCODE;
	  break;
	case 3:
	  vex.prefix = REPNE_PREFIX_OPCODE;
	  break;
	}
      need_vex = 1;
      need_vex_reg = 1;
      codep++;
      vindex = *codep++;
      dp = &vex_table[vex_table_index][vindex];
      end_codep = codep;
      /* There is no MODRM byte for VEX [82|77].  */
      if (vindex != 0x77 && vindex != 0x82)
	{
	  FETCH_DATA (info, codep + 1);
	  modrm.mod = (*codep >> 6) & 3;
	  modrm.reg = (*codep >> 3) & 7;
	  modrm.rm = *codep & 7;
	}
      break;

    case USE_VEX_C5_TABLE:
      /* VEX prefix.  */
      FETCH_DATA (info, codep + 2);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      rex = (*codep & 0x80) ? 0 : REX_R;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit
	  && vex.register_specifier > 0x7)
	{
	  dp = &bad_opcode;
	  return dp;
	}

      vex.w = 0;

      vex.length = (*codep & 0x4) ? 256 : 128;
      switch ((*codep & 0x3))
	{
	case 0:
	  vex.prefix = 0;
	  break;
	case 1:
	  vex.prefix = DATA_PREFIX_OPCODE;
	  break;
	case 2:
	  vex.prefix = REPE_PREFIX_OPCODE;
	  break;
	case 3:
	  vex.prefix = REPNE_PREFIX_OPCODE;
	  break;
	}
      need_vex = 1;
      need_vex_reg = 1;
      codep++;
      vindex = *codep++;
      dp = &vex_table[dp->op[1].bytemode][vindex];
      end_codep = codep;
      /* There is no MODRM byte for VEX [82|77].  */
      if (vindex != 0x77 && vindex != 0x82)
	{
	  FETCH_DATA (info, codep + 1);
	  modrm.mod = (*codep >> 6) & 3;
	  modrm.reg = (*codep >> 3) & 7;
	  modrm.rm = *codep & 7;
	}
      break;

    case USE_VEX_W_TABLE:
      if (!need_vex)
	abort ();

      dp = &vex_w_table[dp->op[1].bytemode][vex.w ? 1 : 0];
      break;

    case USE_EVEX_TABLE:
      two_source_ops = 0;
      /* EVEX prefix.  */
      vex.evex = 1;
      FETCH_DATA (info, codep + 4);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      /* The first byte after 0x62.  */
      rex = ~(*codep >> 5) & 0x7;
      vex.r = *codep & 0x10;
      switch ((*codep & 0xf))
	{
	default:
	  return &bad_opcode;
	case 0x1:
	  vex_table_index = EVEX_0F;
	  break;
	case 0x2:
	  vex_table_index = EVEX_0F38;
	  break;
	case 0x3:
	  vex_table_index = EVEX_0F3A;
	  break;
	}

      /* The second byte after 0x62.  */
      codep++;
      vex.w = *codep & 0x80;
      if (vex.w && address_mode == mode_64bit)
	rex |= REX_W;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit)
	{
	  /* In 16/32-bit mode silently ignore following bits.  */
	  rex &= ~REX_B;
	  vex.r = 1;
	  vex.v = 1;
	  vex.register_specifier &= 0x7;
	}

      /* The U bit.  */
      if (!(*codep & 0x4))
	return &bad_opcode;

      switch ((*codep & 0x3))
	{
	case 0:
	  vex.prefix = 0;
	  break;
	case 1:
	  vex.prefix = DATA_PREFIX_OPCODE;
	  break;
	case 2:
	  vex.prefix = REPE_PREFIX_OPCODE;
	  break;
	case 3:
	  vex.prefix = REPNE_PREFIX_OPCODE;
	  break;
	}

      /* The third byte after 0x62.  */
      codep++;

      /* Remember the static rounding bits.  */
      vex.ll = (*codep >> 5) & 3;
      vex.b = (*codep & 0x10) != 0;

      vex.v = *codep & 0x8;
      vex.mask_register_specifier = *codep & 0x7;
      vex.zeroing = *codep & 0x80;

      need_vex = 1;
      need_vex_reg = 1;
      codep++;
      vindex = *codep++;
      dp = &evex_table[vex_table_index][vindex];
      end_codep = codep;
      FETCH_DATA (info, codep + 1);
      modrm.mod = (*codep >> 6) & 3;
      modrm.reg = (*codep >> 3) & 7;
      modrm.rm = *codep & 7;

      /* Set vector length.  */
      if (modrm.mod == 3 && vex.b)
	vex.length = 512;
      else
	{
	  switch (vex.ll)
	    {
	    case 0x0:
	      vex.length = 128;
	      break;
	    case 0x1:
	      vex.length = 256;
	      break;
	    case 0x2:
	      vex.length = 512;
	      break;
	    default:
	      return &bad_opcode;
	    }
	}
      break;

    case 0:
      dp = &bad_opcode;
      break;

    default:
      abort ();
    }

  if (dp->name != NULL)
    return dp;
  else
    return get_valid_dis386 (dp, info);
}

static void
get_sib (disassemble_info *info, int sizeflag)
{
  /* If modrm.mod == 3, operand must be register.  */
  if (need_modrm
      && ((sizeflag & AFLAG) || address_mode == mode_64bit)
      && modrm.mod != 3
      && modrm.rm == 4)
    {
      FETCH_DATA (info, codep + 2);
      sib.index = (codep [1] >> 3) & 7;
      sib.scale = (codep [1] >> 6) & 3;
      sib.base = codep [1] & 7;
    }
}

static int
print_insn (bfd_vma pc, disassemble_info *info)
{
  const struct dis386 *dp;
  int i;
  char *op_txt[MAX_OPERANDS];
  int needcomma;
  int sizeflag, orig_sizeflag;
  const char *p;
  struct dis_private priv;
  int prefix_length;

  priv.orig_sizeflag = AFLAG | DFLAG;
  if ((info->mach & bfd_mach_i386_i386) != 0)
    address_mode = mode_32bit;
  else if (info->mach == bfd_mach_i386_i8086)
    {
      address_mode = mode_16bit;
      priv.orig_sizeflag = 0;
    }
  else
    address_mode = mode_64bit;

  if (intel_syntax == (char) -1)
    intel_syntax = (info->mach & bfd_mach_i386_intel_syntax) != 0;

  for (p = info->disassembler_options; p != NULL; )
    {
      if (CONST_STRNEQ (p, "amd64"))
	isa64 = amd64;
      else if (CONST_STRNEQ (p, "intel64"))
	isa64 = intel64;
      else if (CONST_STRNEQ (p, "x86-64"))
	{
	  address_mode = mode_64bit;
	  priv.orig_sizeflag = AFLAG | DFLAG;
	}
      else if (CONST_STRNEQ (p, "i386"))
	{
	  address_mode = mode_32bit;
	  priv.orig_sizeflag = AFLAG | DFLAG;
	}
      else if (CONST_STRNEQ (p, "i8086"))
	{
	  address_mode = mode_16bit;
	  priv.orig_sizeflag = 0;
	}
      else if (CONST_STRNEQ (p, "intel"))
	{
	  intel_syntax = 1;
	  if (CONST_STRNEQ (p + 5, "-mnemonic"))
	    intel_mnemonic = 1;
	}
      else if (CONST_STRNEQ (p, "att"))
	{
	  intel_syntax = 0;
	  if (CONST_STRNEQ (p + 3, "-mnemonic"))
	    intel_mnemonic = 0;
	}
      else if (CONST_STRNEQ (p, "addr"))
	{
	  if (address_mode == mode_64bit)
	    {
	      if (p[4] == '3' && p[5] == '2')
		priv.orig_sizeflag &= ~AFLAG;
	      else if (p[4] == '6' && p[5] == '4')
		priv.orig_sizeflag |= AFLAG;
	    }
	  else
	    {
	      if (p[4] == '1' && p[5] == '6')
		priv.orig_sizeflag &= ~AFLAG;
	      else if (p[4] == '3' && p[5] == '2')
		priv.orig_sizeflag |= AFLAG;
	    }
	}
      else if (CONST_STRNEQ (p, "data"))
	{
	  if (p[4] == '1' && p[5] == '6')
	    priv.orig_sizeflag &= ~DFLAG;
	  else if (p[4] == '3' && p[5] == '2')
	    priv.orig_sizeflag |= DFLAG;
	}
      else if (CONST_STRNEQ (p, "suffix"))
	priv.orig_sizeflag |= SUFFIX_ALWAYS;

      p = strchr (p, ',');
      if (p != NULL)
	p++;
    }

  if (intel_syntax)
    {
      names64 = intel_names64;
      names32 = intel_names32;
      names16 = intel_names16;
      names8 = intel_names8;
      names8rex = intel_names8rex;
      names_seg = intel_names_seg;
      names_mm = intel_names_mm;
      names_bnd = intel_names_bnd;
      names_xmm = intel_names_xmm;
      names_ymm = intel_names_ymm;
      names_zmm = intel_names_zmm;
      index64 = intel_index64;
      index32 = intel_index32;
      names_mask = intel_names_mask;
      index16 = intel_index16;
      open_char = '[';
      close_char = ']';
      separator_char = '+';
      scale_char = '*';
    }
  else
    {
      names64 = att_names64;
      names32 = att_names32;
      names16 = att_names16;
      names8 = att_names8;
      names8rex = att_names8rex;
      names_seg = att_names_seg;
      names_mm = att_names_mm;
      names_bnd = att_names_bnd;
      names_xmm = att_names_xmm;
      names_ymm = att_names_ymm;
      names_zmm = att_names_zmm;
      index64 = att_index64;
      index32 = att_index32;
      names_mask = att_names_mask;
      index16 = att_index16;
      open_char = '(';
      close_char =  ')';
      separator_char = ',';
      scale_char = ',';
    }

  /* The output looks better if we put 7 bytes on a line, since that
     puts most long word instructions on a single line.  Use 8 bytes
     for Intel L1OM.  */
  if ((info->mach & bfd_mach_l1om) != 0)
    info->bytes_per_line = 8;
  else
    info->bytes_per_line = 7;

  info->private_data = &priv;
  priv.max_fetched = priv.the_buffer;
  priv.insn_start = pc;

  obuf[0] = 0;
  for (i = 0; i < MAX_OPERANDS; ++i)
    {
      op_out[i][0] = 0;
      op_index[i] = -1;
    }

  the_info = info;
  start_pc = pc;
  start_codep = priv.the_buffer;
  codep = priv.the_buffer;

  if (OPCODES_SIGSETJMP (priv.bailout) != 0)
    {
      const char *name;

      /* Getting here means we tried for data but didn't get it.  That
	 means we have an incomplete instruction of some sort.  Just
	 print the first byte as a prefix or a .byte pseudo-op.  */
      if (codep > priv.the_buffer)
	{
	  name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
	  if (name != NULL)
	    (*info->fprintf_func) (info->stream, "%s", name);
	  else
	    {
	      /* Just print the first byte as a .byte instruction.  */
	      (*info->fprintf_func) (info->stream, ".byte 0x%x",
				     (unsigned int) priv.the_buffer[0]);
	    }

	  return 1;
	}

      return -1;
    }

  obufp = obuf;
  sizeflag = priv.orig_sizeflag;

  if (!ckprefix () || rex_used)
    {
      /* Too many prefixes or unused REX prefixes.  */
      for (i = 0;
	   i < (int) ARRAY_SIZE (all_prefixes) && all_prefixes[i];
	   i++)
	(*info->fprintf_func) (info->stream, "%s%s",
			       i == 0 ? "" : " ",
			       prefix_name (all_prefixes[i], sizeflag));
      return i;
    }

  insn_codep = codep;

  FETCH_DATA (info, codep + 1);
  two_source_ops = (*codep == 0x62) || (*codep == 0xc8);

  if (((prefixes & PREFIX_FWAIT)
       && ((*codep < 0xd8) || (*codep > 0xdf))))
    {
      /* Handle prefixes before fwait.  */
      for (i = 0; i < fwait_prefix && all_prefixes[i];
	   i++)
	(*info->fprintf_func) (info->stream, "%s ",
			       prefix_name (all_prefixes[i], sizeflag));
      (*info->fprintf_func) (info->stream, "fwait");
      return i + 1;
    }

  if (*codep == 0x0f)
    {
      unsigned char threebyte;
      FETCH_DATA (info, codep + 2);
      threebyte = *++codep;
      dp = &dis386_twobyte[threebyte];
      need_modrm = twobyte_has_modrm[*codep];
      codep++;
    }
  else
    {
      dp = &dis386[*codep];
      need_modrm = onebyte_has_modrm[*codep];
      codep++;
    }

  /* Save sizeflag for printing the extra prefixes later before updating
     it for mnemonic and operand processing.  The prefix names depend
     only on the address mode.  */
  orig_sizeflag = sizeflag;
  if (prefixes & PREFIX_ADDR)
    sizeflag ^= AFLAG;
  if ((prefixes & PREFIX_DATA))
    sizeflag ^= DFLAG;

  end_codep = codep;
  if (need_modrm)
    {
      FETCH_DATA (info, codep + 1);
      modrm.mod = (*codep >> 6) & 3;
      modrm.reg = (*codep >> 3) & 7;
      modrm.rm = *codep & 7;
    }

  need_vex = 0;
  need_vex_reg = 0;
  vex_w_done = 0;
  vex.evex = 0;

  if (dp->name == NULL && dp->op[0].bytemode == FLOATCODE)
    {
      get_sib (info, sizeflag);
      dofloat (sizeflag);
    }
  else
    {
      dp = get_valid_dis386 (dp, info);
      if (dp != NULL && putop (dp->name, sizeflag) == 0)
	{
	  get_sib (info, sizeflag);
	  for (i = 0; i < MAX_OPERANDS; ++i)
	    {
	      obufp = op_out[i];
	      op_ad = MAX_OPERANDS - 1 - i;
	      if (dp->op[i].rtn)
		(*dp->op[i].rtn) (dp->op[i].bytemode, sizeflag);
	      /* For EVEX instruction after the last operand masking
		 should be printed.  */
	      if (i == 0 && vex.evex)
		{
		  /* Don't print {%k0}.  */
		  if (vex.mask_register_specifier)
		    {
		      oappend ("{");
		      oappend (names_mask[vex.mask_register_specifier]);
		      oappend ("}");
		    }
		  if (vex.zeroing)
		    oappend ("{z}");
		}
	    }
	}
    }

  /* Check if the REX prefix is used.  */
  if (rex_ignored == 0 && (rex ^ rex_used) == 0 && last_rex_prefix >= 0)
    all_prefixes[last_rex_prefix] = 0;

  /* Check if the SEG prefix is used.  */
  if ((prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS | PREFIX_ES
		   | PREFIX_FS | PREFIX_GS)) != 0
      && (used_prefixes & active_seg_prefix) != 0)
    all_prefixes[last_seg_prefix] = 0;

  /* Check if the ADDR prefix is used.  */
  if ((prefixes & PREFIX_ADDR) != 0
      && (used_prefixes & PREFIX_ADDR) != 0)
    all_prefixes[last_addr_prefix] = 0;

  /* Check if the DATA prefix is used.  */
  if ((prefixes & PREFIX_DATA) != 0
      && (used_prefixes & PREFIX_DATA) != 0)
    all_prefixes[last_data_prefix] = 0;

  /* Print the extra prefixes.  */
  prefix_length = 0;
  for (i = 0; i < (int) ARRAY_SIZE (all_prefixes); i++)
    if (all_prefixes[i])
      {
	const char *name;
	name = prefix_name (all_prefixes[i], orig_sizeflag);
	if (name == NULL)
	  abort ();
	prefix_length += strlen (name) + 1;
	(*info->fprintf_func) (info->stream, "%s ", name);
      }

  /* If the mandatory PREFIX_REPZ/PREFIX_REPNZ/PREFIX_DATA prefix is
     unused, opcode is invalid.  Since the PREFIX_DATA prefix may be
     used by putop and MMX/SSE operand and may be overriden by the
     PREFIX_REPZ/PREFIX_REPNZ fix, we check the PREFIX_DATA prefix
     separately.  */
  if (dp->prefix_requirement == PREFIX_OPCODE
      && dp != &bad_opcode
      && (((prefixes
	    & (PREFIX_REPZ | PREFIX_REPNZ)) != 0
	   && (used_prefixes
	       & (PREFIX_REPZ | PREFIX_REPNZ)) == 0)
	  || ((((prefixes
		 & (PREFIX_REPZ | PREFIX_REPNZ | PREFIX_DATA))
		== PREFIX_DATA)
	       && (used_prefixes & PREFIX_DATA) == 0))))
    {
      (*info->fprintf_func) (info->stream, "(bad)");
      return end_codep - priv.the_buffer;
    }

  /* Check maximum code length.  */
  if ((codep - start_codep) > MAX_CODE_LENGTH)
    {
      (*info->fprintf_func) (info->stream, "(bad)");
      return MAX_CODE_LENGTH;
    }

  obufp = mnemonicendp;
  for (i = strlen (obuf) + prefix_length; i < 6; i++)
    oappend (" ");
  oappend (" ");
  (*info->fprintf_func) (info->stream, "%s", obuf);

  /* The enter and bound instructions are printed with operands in the same
     order as the intel book; everything else is printed in reverse order.  */
  if (intel_syntax || two_source_ops)
    {
      bfd_vma riprel;

      for (i = 0; i < MAX_OPERANDS; ++i)
	op_txt[i] = op_out[i];

      if (intel_syntax && dp && dp->op[2].rtn == OP_Rounding
          && dp->op[3].rtn == OP_E && dp->op[4].rtn == NULL)
	{
	  op_txt[2] = op_out[3];
	  op_txt[3] = op_out[2];
	}

      for (i = 0; i < (MAX_OPERANDS >> 1); ++i)
	{
	  op_ad = op_index[i];
	  op_index[i] = op_index[MAX_OPERANDS - 1 - i];
	  op_index[MAX_OPERANDS - 1 - i] = op_ad;
	  riprel = op_riprel[i];
	  op_riprel[i] = op_riprel [MAX_OPERANDS - 1 - i];
	  op_riprel[MAX_OPERANDS - 1 - i] = riprel;
	}
    }
  else
    {
      for (i = 0; i < MAX_OPERANDS; ++i)
	op_txt[MAX_OPERANDS - 1 - i] = op_out[i];
    }

  needcomma = 0;
  for (i = 0; i < MAX_OPERANDS; ++i)
    if (*op_txt[i])
      {
	if (needcomma)
	  (*info->fprintf_func) (info->stream, ",");
	if (op_index[i] != -1 && !op_riprel[i])
	  (*info->print_address_func) ((bfd_vma) op_address[op_index[i]], info);
	else
	  (*info->fprintf_func) (info->stream, "%s", op_txt[i]);
	needcomma = 1;
      }

  for (i = 0; i < MAX_OPERANDS; i++)
    if (op_index[i] != -1 && op_riprel[i])
      {
	(*info->fprintf_func) (info->stream, "        # ");
	(*info->print_address_func) ((bfd_vma) (start_pc + codep - start_codep
						+ op_address[op_index[i]]), info);
	break;
      }
  return codep - priv.the_buffer;
}

static const char *float_mem[] = {
  /* d8 */
  "fadd{s|}",
  "fmul{s|}",
  "fcom{s|}",
  "fcomp{s|}",
  "fsub{s|}",
  "fsubr{s|}",
  "fdiv{s|}",
  "fdivr{s|}",
  /* d9 */
  "fld{s|}",
  "(bad)",
  "fst{s|}",
  "fstp{s|}",
  "fldenvIC",
  "fldcw",
  "fNstenvIC",
  "fNstcw",
  /* da */
  "fiadd{l|}",
  "fimul{l|}",
  "ficom{l|}",
  "ficomp{l|}",
  "fisub{l|}",
  "fisubr{l|}",
  "fidiv{l|}",
  "fidivr{l|}",
  /* db */
  "fild{l|}",
  "fisttp{l|}",
  "fist{l|}",
  "fistp{l|}",
  "(bad)",
  "fld{t||t|}",
  "(bad)",
  "fstp{t||t|}",
  /* dc */
  "fadd{l|}",
  "fmul{l|}",
  "fcom{l|}",
  "fcomp{l|}",
  "fsub{l|}",
  "fsubr{l|}",
  "fdiv{l|}",
  "fdivr{l|}",
  /* dd */
  "fld{l|}",
  "fisttp{ll|}",
  "fst{l||}",
  "fstp{l|}",
  "frstorIC",
  "(bad)",
  "fNsaveIC",
  "fNstsw",
  /* de */
  "fiadd",
  "fimul",
  "ficom",
  "ficomp",
  "fisub",
  "fisubr",
  "fidiv",
  "fidivr",
  /* df */
  "fild",
  "fisttp",
  "fist",
  "fistp",
  "fbld",
  "fild{ll|}",
  "fbstp",
  "fistp{ll|}",
};

static const unsigned char float_mem_mode[] = {
  /* d8 */
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  /* d9 */
  d_mode,
  0,
  d_mode,
  d_mode,
  0,
  w_mode,
  0,
  w_mode,
  /* da */
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  /* db */
  d_mode,
  d_mode,
  d_mode,
  d_mode,
  0,
  t_mode,
  0,
  t_mode,
  /* dc */
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  /* dd */
  q_mode,
  q_mode,
  q_mode,
  q_mode,
  0,
  0,
  0,
  w_mode,
  /* de */
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  /* df */
  w_mode,
  w_mode,
  w_mode,
  w_mode,
  t_mode,
  q_mode,
  t_mode,
  q_mode
};

#define ST { OP_ST, 0 }
#define STi { OP_STi, 0 }

#define FGRPd9_2 NULL, { { NULL, 0 } }, 0
#define FGRPd9_4 NULL, { { NULL, 1 } }, 0
#define FGRPd9_5 NULL, { { NULL, 2 } }, 0
#define FGRPd9_6 NULL, { { NULL, 3 } }, 0
#define FGRPd9_7 NULL, { { NULL, 4 } }, 0
#define FGRPda_5 NULL, { { NULL, 5 } }, 0
#define FGRPdb_4 NULL, { { NULL, 6 } }, 0
#define FGRPde_3 NULL, { { NULL, 7 } }, 0
#define FGRPdf_4 NULL, { { NULL, 8 } }, 0

static const struct dis386 float_reg[][8] = {
  /* d8 */
  {
    { "fadd",	{ ST, STi }, 0 },
    { "fmul",	{ ST, STi }, 0 },
    { "fcom",	{ STi }, 0 },
    { "fcomp",	{ STi }, 0 },
    { "fsub",	{ ST, STi }, 0 },
    { "fsubr",	{ ST, STi }, 0 },
    { "fdiv",	{ ST, STi }, 0 },
    { "fdivr",	{ ST, STi }, 0 },
  },
  /* d9 */
  {
    { "fld",	{ STi }, 0 },
    { "fxch",	{ STi }, 0 },
    { FGRPd9_2 },
    { Bad_Opcode },
    { FGRPd9_4 },
    { FGRPd9_5 },
    { FGRPd9_6 },
    { FGRPd9_7 },
  },
  /* da */
  {
    { "fcmovb",	{ ST, STi }, 0 },
    { "fcmove",	{ ST, STi }, 0 },
    { "fcmovbe",{ ST, STi }, 0 },
    { "fcmovu",	{ ST, STi }, 0 },
    { Bad_Opcode },
    { FGRPda_5 },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* db */
  {
    { "fcmovnb",{ ST, STi }, 0 },
    { "fcmovne",{ ST, STi }, 0 },
    { "fcmovnbe",{ ST, STi }, 0 },
    { "fcmovnu",{ ST, STi }, 0 },
    { FGRPdb_4 },
    { "fucomi",	{ ST, STi }, 0 },
    { "fcomi",	{ ST, STi }, 0 },
    { Bad_Opcode },
  },
  /* dc */
  {
    { "fadd",	{ STi, ST }, 0 },
    { "fmul",	{ STi, ST }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { "fsub!M",	{ STi, ST }, 0 },
    { "fsubM",	{ STi, ST }, 0 },
    { "fdiv!M",	{ STi, ST }, 0 },
    { "fdivM",	{ STi, ST }, 0 },
  },
  /* dd */
  {
    { "ffree",	{ STi }, 0 },
    { Bad_Opcode },
    { "fst",	{ STi }, 0 },
    { "fstp",	{ STi }, 0 },
    { "fucom",	{ STi }, 0 },
    { "fucomp",	{ STi }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* de */
  {
    { "faddp",	{ STi, ST }, 0 },
    { "fmulp",	{ STi, ST }, 0 },
    { Bad_Opcode },
    { FGRPde_3 },
    { "fsub!Mp", { STi, ST }, 0 },
    { "fsubMp",	{ STi, ST }, 0 },
    { "fdiv!Mp", { STi, ST }, 0 },
    { "fdivMp",	{ STi, ST }, 0 },
  },
  /* df */
  {
    { "ffreep",	{ STi }, 0 },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { FGRPdf_4 },
    { "fucomip", { ST, STi }, 0 },
    { "fcomip", { ST, STi }, 0 },
    { Bad_Opcode },
  },
};

static char *fgrps[][8] = {
  /* d9_2  0 */
  {
    "fnop","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* d9_4  1 */
  {
    "fchs","fabs","(bad)","(bad)","ftst","fxam","(bad)","(bad)",
  },

  /* d9_5  2 */
  {
    "fld1","fldl2t","fldl2e","fldpi","fldlg2","fldln2","fldz","(bad)",
  },

  /* d9_6  3 */
  {
    "f2xm1","fyl2x","fptan","fpatan","fxtract","fprem1","fdecstp","fincstp",
  },

  /* d9_7  4 */
  {
    "fprem","fyl2xp1","fsqrt","fsincos","frndint","fscale","fsin","fcos",
  },

  /* da_5  5 */
  {
    "(bad)","fucompp","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* db_4  6 */
  {
    "fNeni(8087 only)","fNdisi(8087 only)","fNclex","fNinit",
    "fNsetpm(287 only)","frstpm(287 only)","(bad)","(bad)",
  },

  /* de_3  7 */
  {
    "(bad)","fcompp","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* df_4  8 */
  {
    "fNstsw","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },
};

static void
swap_operand (void)
{
  mnemonicendp[0] = '.';
  mnemonicendp[1] = 's';
  mnemonicendp += 2;
}

static void
OP_Skip_MODRM (int bytemode ATTRIBUTE_UNUSED,
	       int sizeflag ATTRIBUTE_UNUSED)
{
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
}

static void
dofloat (int sizeflag)
{
  const struct dis386 *dp;
  unsigned char floatop;

  floatop = codep[-1];

  if (modrm.mod != 3)
    {
      int fp_indx = (floatop - 0xd8) * 8 + modrm.reg;

      putop (float_mem[fp_indx], sizeflag);
      obufp = op_out[0];
      op_ad = 2;
      OP_E (float_mem_mode[fp_indx], sizeflag);
      return;
    }
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  dp = &float_reg[floatop - 0xd8][modrm.reg];
  if (dp->name == NULL)
    {
      putop (fgrps[dp->op[0].bytemode][modrm.rm], sizeflag);

      /* Instruction fnstsw is only one with strange arg.  */
      if (floatop == 0xdf && codep[-1] == 0xe0)
	strcpy (op_out[0], names16[0]);
    }
  else
    {
      putop (dp->name, sizeflag);

      obufp = op_out[0];
      op_ad = 2;
      if (dp->op[0].rtn)
	(*dp->op[0].rtn) (dp->op[0].bytemode, sizeflag);

      obufp = op_out[1];
      op_ad = 1;
      if (dp->op[1].rtn)
	(*dp->op[1].rtn) (dp->op[1].bytemode, sizeflag);
    }
}

/* Like oappend (below), but S is a string starting with '%'.
   In Intel syntax, the '%' is elided.  */
static void
oappend_maybe_intel (const char *s)
{
  oappend (s + intel_syntax);
}

static void
OP_ST (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  oappend_maybe_intel ("%st");
}

static void
OP_STi (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  sprintf (scratchbuf, "%%st(%d)", modrm.rm);
  oappend_maybe_intel (scratchbuf);
}

/* Capital letters in template are macros.  */
static int
putop (const char *in_template, int sizeflag)
{
  const char *p;
  int alt = 0;
  int cond = 1;
  unsigned int l = 0, len = 1;
  char last[4];

#define SAVE_LAST(c)			\
  if (l < len && l < sizeof (last))	\
    last[l++] = c;			\
  else					\
    abort ();

  for (p = in_template; *p; p++)
    {
      switch (*p)
	{
	default:
	  *obufp++ = *p;
	  break;
	case '%':
	  len++;
	  break;
	case '!':
	  cond = 0;
	  break;
	case '{':
	  alt = 0;
	  if (intel_syntax)
	    {
	      while (*++p != '|')
		if (*p == '}' || *p == '\0')
		  abort ();
	    }
	  /* Fall through.  */
	case 'I':
	  alt = 1;
	  continue;
	case '|':
	  while (*++p != '}')
	    {
	      if (*p == '\0')
		abort ();
	    }
	  break;
	case '}':
	  break;
	case 'A':
	  if (intel_syntax)
	    break;
	  if (modrm.mod != 3 || (sizeflag & SUFFIX_ALWAYS))
	    *obufp++ = 'b';
	  break;
	case 'B':
	  if (l == 0 && len == 1)
	    {
case_B:
	      if (intel_syntax)
		break;
	      if (sizeflag & SUFFIX_ALWAYS)
		*obufp++ = 'b';
	    }
	  else
	    {
	      if (l != 1
		  || len != 2
		  || last[0] != 'L')
		{
		  SAVE_LAST (*p);
		  break;
		}

	      if (address_mode == mode_64bit
		  && !(prefixes & PREFIX_ADDR))
		{
		  *obufp++ = 'a';
		  *obufp++ = 'b';
		  *obufp++ = 's';
		}

	      goto case_B;
	    }
	  break;
	case 'C':
	  if (intel_syntax && !alt)
	    break;
	  if ((prefixes & PREFIX_DATA) || (sizeflag & SUFFIX_ALWAYS))
	    {
	      if (sizeflag & DFLAG)
		*obufp++ = intel_syntax ? 'd' : 'l';
	      else
		*obufp++ = intel_syntax ? 'w' : 's';
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  break;
	case 'D':
	  if (intel_syntax || !(sizeflag & SUFFIX_ALWAYS))
	    break;
	  USED_REX (REX_W);
	  if (modrm.mod == 3)
	    {
	      if (rex & REX_W)
		*obufp++ = 'q';
	      else
		{
		  if (sizeflag & DFLAG)
		    *obufp++ = intel_syntax ? 'd' : 'l';
		  else
		    *obufp++ = 'w';
		  used_prefixes |= (prefixes & PREFIX_DATA);
		}
	    }
	  else
	    *obufp++ = 'w';
	  break;
	case 'E':		/* For jcxz/jecxz */
	  if (address_mode == mode_64bit)
	    {
	      if (sizeflag & AFLAG)
		*obufp++ = 'r';
	      else
		*obufp++ = 'e';
	    }
	  else
	    if (sizeflag & AFLAG)
	      *obufp++ = 'e';
	  used_prefixes |= (prefixes & PREFIX_ADDR);
	  break;
	case 'F':
	  if (intel_syntax)
	    break;
	  if ((prefixes & PREFIX_ADDR) || (sizeflag & SUFFIX_ALWAYS))
	    {
	      if (sizeflag & AFLAG)
		*obufp++ = address_mode == mode_64bit ? 'q' : 'l';
	      else
		*obufp++ = address_mode == mode_64bit ? 'l' : 'w';
	      used_prefixes |= (prefixes & PREFIX_ADDR);
	    }
	  break;
	case 'G':
	  if (intel_syntax || (obufp[-1] != 's' && !(sizeflag & SUFFIX_ALWAYS)))
	    break;
	  if ((rex & REX_W) || (sizeflag & DFLAG))
	    *obufp++ = 'l';
	  else
	    *obufp++ = 'w';
	  if (!(rex & REX_W))
	    used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 'H':
	  if (intel_syntax)
	    break;
	  if ((prefixes & (PREFIX_CS | PREFIX_DS)) == PREFIX_CS
	      || (prefixes & (PREFIX_CS | PREFIX_DS)) == PREFIX_DS)
	    {
	      used_prefixes |= prefixes & (PREFIX_CS | PREFIX_DS);
	      *obufp++ = ',';
	      *obufp++ = 'p';
	      if (prefixes & PREFIX_DS)
		*obufp++ = 't';
	      else
		*obufp++ = 'n';
	    }
	  break;
	case 'J':
	  if (intel_syntax)
	    break;
	  *obufp++ = 'l';
	  break;
	case 'K':
	  USED_REX (REX_W);
	  if (rex & REX_W)
	    *obufp++ = 'q';
	  else
	    *obufp++ = 'd';
	  break;
	case 'Z':
	  if (l != 0 || len != 1)
	    {
	      if (l != 1 || len != 2 || last[0] != 'X')
		{
		  SAVE_LAST (*p);
		  break;
		}
	      if (!need_vex || !vex.evex)
		abort ();
	      if (intel_syntax
		  || ((modrm.mod == 3 || vex.b) && !(sizeflag & SUFFIX_ALWAYS)))
		break;
	      switch (vex.length)
		{
		case 128:
		  *obufp++ = 'x';
		  break;
		case 256:
		  *obufp++ = 'y';
		  break;
		case 512:
		  *obufp++ = 'z';
		  break;
		default:
		  abort ();
		}
	      break;
	    }
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit && (sizeflag & SUFFIX_ALWAYS))
	    {
	      *obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	  goto case_L;
	case 'L':
	  if (l != 0 || len != 1)
	    {
	      SAVE_LAST (*p);
	      break;
	    }
case_L:
	  if (intel_syntax)
	    break;
	  if (sizeflag & SUFFIX_ALWAYS)
	    *obufp++ = 'l';
	  break;
	case 'M':
	  if (intel_mnemonic != cond)
	    *obufp++ = 'r';
	  break;
	case 'N':
	  if ((prefixes & PREFIX_FWAIT) == 0)
	    *obufp++ = 'n';
	  else
	    used_prefixes |= PREFIX_FWAIT;
	  break;
	case 'O':
	  USED_REX (REX_W);
	  if (rex & REX_W)
	    *obufp++ = 'o';
	  else if (intel_syntax && (sizeflag & DFLAG))
	    *obufp++ = 'q';
	  else
	    *obufp++ = 'd';
	  if (!(rex & REX_W))
	    used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 'T':
	  if (!intel_syntax
	      && address_mode == mode_64bit
	      && ((sizeflag & DFLAG) || (rex & REX_W)))
	    {
	      *obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	  goto case_P;
	case 'P':
	  if (l == 0 && len == 1)
	    {
case_P:
	      if (intel_syntax)
		{
		  if ((rex & REX_W) == 0
		      && (prefixes & PREFIX_DATA))
		    {
		      if ((sizeflag & DFLAG) == 0)
			*obufp++ = 'w';
		      used_prefixes |= (prefixes & PREFIX_DATA);
		    }
		  break;
		}
	      if ((prefixes & PREFIX_DATA)
		  || (rex & REX_W)
		  || (sizeflag & SUFFIX_ALWAYS))
		{
		  USED_REX (REX_W);
		  if (rex & REX_W)
		    *obufp++ = 'q';
		  else
		    {
		      if (sizeflag & DFLAG)
			*obufp++ = 'l';
		      else
			*obufp++ = 'w';
		      used_prefixes |= (prefixes & PREFIX_DATA);
		    }
		}
	    }
	  else
	    {
	      if (l != 1 || len != 2 || last[0] != 'L')
		{
		  SAVE_LAST (*p);
		  break;
		}

	      if ((prefixes & PREFIX_DATA)
		  || (rex & REX_W)
		  || (sizeflag & SUFFIX_ALWAYS))
		{
		  USED_REX (REX_W);
		  if (rex & REX_W)
		    *obufp++ = 'q';
		  else
		    {
		      if (sizeflag & DFLAG)
			*obufp++ = intel_syntax ? 'd' : 'l';
		      else
			*obufp++ = 'w';
		      used_prefixes |= (prefixes & PREFIX_DATA);
		    }
		}
	    }
	  break;
	case 'U':
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit
	      && ((sizeflag & DFLAG) || (rex & REX_W)))
	    {
	      if (modrm.mod != 3 || (sizeflag & SUFFIX_ALWAYS))
		*obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	  goto case_Q;
	case 'Q':
	  if (l == 0 && len == 1)
	    {
case_Q:
	      if (intel_syntax && !alt)
		break;
	      USED_REX (REX_W);
	      if (modrm.mod != 3 || (sizeflag & SUFFIX_ALWAYS))
		{
		  if (rex & REX_W)
		    *obufp++ = 'q';
		  else
		    {
		      if (sizeflag & DFLAG)
			*obufp++ = intel_syntax ? 'd' : 'l';
		      else
			*obufp++ = 'w';
		      used_prefixes |= (prefixes & PREFIX_DATA);
		    }
		}
	    }
	  else
	    {
	      if (l != 1 || len != 2 || last[0] != 'L')
		{
		  SAVE_LAST (*p);
		  break;
		}
	      if (intel_syntax
		  || (modrm.mod == 3 && !(sizeflag & SUFFIX_ALWAYS)))
		break;
	      if ((rex & REX_W))
		{
		  USED_REX (REX_W);
		  *obufp++ = 'q';
		}
	      else
		*obufp++ = 'l';
	    }
	  break;
	case 'R':
	  USED_REX (REX_W);
	  if (rex & REX_W)
	    *obufp++ = 'q';
	  else if (sizeflag & DFLAG)
	    {
	      if (intel_syntax)
		  *obufp++ = 'd';
	      else
		  *obufp++ = 'l';
	    }
	  else
	    *obufp++ = 'w';
	  if (intel_syntax && !p[1]
	      && ((rex & REX_W) || (sizeflag & DFLAG)))
	    *obufp++ = 'e';
	  if (!(rex & REX_W))
	    used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 'V':
	  if (l == 0 && len == 1)
	    {
	      if (intel_syntax)
		break;
	      if (address_mode == mode_64bit
		  && ((sizeflag & DFLAG) || (rex & REX_W)))
		{
		  if (sizeflag & SUFFIX_ALWAYS)
		    *obufp++ = 'q';
		  break;
		}
	    }
	  else
	    {
	      if (l != 1
		  || len != 2
		  || last[0] != 'L')
		{
		  SAVE_LAST (*p);
		  break;
		}

	      if (rex & REX_W)
		{
		  *obufp++ = 'a';
		  *obufp++ = 'b';
		  *obufp++ = 's';
		}
	    }
	  /* Fall through.  */
	  goto case_S;
	case 'S':
	  if (l == 0 && len == 1)
	    {
case_S:
	      if (intel_syntax)
		break;
	      if (sizeflag & SUFFIX_ALWAYS)
		{
		  if (rex & REX_W)
		    *obufp++ = 'q';
		  else
		    {
		      if (sizeflag & DFLAG)
			*obufp++ = 'l';
		      else
			*obufp++ = 'w';
		      used_prefixes |= (prefixes & PREFIX_DATA);
		    }
		}
	    }
	  else
	    {
	      if (l != 1
		  || len != 2
		  || last[0] != 'L')
		{
		  SAVE_LAST (*p);
		  break;
		}

	      if (address_mode == mode_64bit
		  && !(prefixes & PREFIX_ADDR))
		{
		  *obufp++ = 'a';
		  *obufp++ = 'b';
		  *obufp++ = 's';
		}

	      goto case_S;
	    }
	  break;
	case 'X':
	  if (l != 0 || len != 1)
	    {
	      SAVE_LAST (*p);
	      break;
	    }
	  if (need_vex && vex.prefix)
	    {
	      if (vex.prefix == DATA_PREFIX_OPCODE)
		*obufp++ = 'd';
	      else
		*obufp++ = 's';
	    }
	  else
	    {
	      if (prefixes & PREFIX_DATA)
		*obufp++ = 'd';
	      else
		*obufp++ = 's';
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  break;
	case 'Y':
	  if (l == 0 && len == 1)
	    {
	      if (intel_syntax || !(sizeflag & SUFFIX_ALWAYS))
		break;
	      if (rex & REX_W)
		{
		  USED_REX (REX_W);
		  *obufp++ = 'q';
		}
	      break;
	    }
	  else
	    {
	      if (l != 1 || len != 2 || last[0] != 'X')
		{
		  SAVE_LAST (*p);
		  break;
		}
	      if (!need_vex)
		abort ();
	      if (intel_syntax
		  || ((modrm.mod == 3 || vex.b) && !(sizeflag & SUFFIX_ALWAYS)))
		break;
	      switch (vex.length)
		{
		case 128:
		  *obufp++ = 'x';
		  break;
		case 256:
		  *obufp++ = 'y';
		  break;
		case 512:
		  if (!vex.evex)
		default:
		    abort ();
		}
	    }
	  break;
	case 'W':
	  if (l == 0 && len == 1)
	    {
	      /* operand size flag for cwtl, cbtw */
	      USED_REX (REX_W);
	      if (rex & REX_W)
		{
		  if (intel_syntax)
		    *obufp++ = 'd';
		  else
		    *obufp++ = 'l';
		}
	      else if (sizeflag & DFLAG)
		*obufp++ = 'w';
	      else
		*obufp++ = 'b';
	      if (!(rex & REX_W))
		used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  else
	    {
	      if (l != 1
		  || len != 2
		  || (last[0] != 'X'
		      && last[0] != 'L'))
		{
		  SAVE_LAST (*p);
		  break;
		}
	      if (!need_vex)
		abort ();
	      if (last[0] == 'X')
		*obufp++ = vex.w ? 'd': 's';
	      else
		*obufp++ = vex.w ? 'q': 'd';
	    }
	  break;
	case '^':
	  if (intel_syntax)
	    break;
	  if ((prefixes & PREFIX_DATA) || (sizeflag & SUFFIX_ALWAYS))
	    {
	      if (sizeflag & DFLAG)
		*obufp++ = 'l';
	      else
		*obufp++ = 'w';
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  break;
	case '@':
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit
	      && (isa64 == intel64
		  || ((sizeflag & DFLAG) || (rex & REX_W))))
	      *obufp++ = 'q';
	  else if ((prefixes & PREFIX_DATA))
	    {
	      if (!(sizeflag & DFLAG))
		*obufp++ = 'w';
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  break;
	}
      alt = 0;
    }
  *obufp = 0;
  mnemonicendp = obufp;
  return 0;
}

static void
oappend (const char *s)
{
  obufp = stpcpy (obufp, s);
}

static void
append_seg (void)
{
  /* Only print the active segment register.  */
  if (!active_seg_prefix)
    return;

  used_prefixes |= active_seg_prefix;
  switch (active_seg_prefix)
    {
    case PREFIX_CS:
      oappend_maybe_intel ("%cs:");
      break;
    case PREFIX_DS:
      oappend_maybe_intel ("%ds:");
      break;
    case PREFIX_SS:
      oappend_maybe_intel ("%ss:");
      break;
    case PREFIX_ES:
      oappend_maybe_intel ("%es:");
      break;
    case PREFIX_FS:
      oappend_maybe_intel ("%fs:");
      break;
    case PREFIX_GS:
      oappend_maybe_intel ("%gs:");
      break;
    default:
      break;
    }
}

static void
OP_indirE (int bytemode, int sizeflag)
{
  if (!intel_syntax)
    oappend ("*");
  OP_E (bytemode, sizeflag);
}

static void
print_operand_value (char *buf, int hex, bfd_vma disp)
{
  if (address_mode == mode_64bit)
    {
      if (hex)
	{
	  char tmp[30];
	  int i;
	  buf[0] = '0';
	  buf[1] = 'x';
	  sprintf_vma (tmp, disp);
	  for (i = 0; tmp[i] == '0' && tmp[i + 1]; i++);
	  strcpy (buf + 2, tmp + i);
	}
      else
	{
	  bfd_signed_vma v = disp;
	  char tmp[30];
	  int i;
	  if (v < 0)
	    {
	      *(buf++) = '-';
	      v = -disp;
	      /* Check for possible overflow on 0x8000000000000000.  */
	      if (v < 0)
		{
		  strcpy (buf, "9223372036854775808");
		  return;
		}
	    }
	  if (!v)
	    {
	      strcpy (buf, "0");
	      return;
	    }

	  i = 0;
	  tmp[29] = 0;
	  while (v)
	    {
	      tmp[28 - i] = (v % 10) + '0';
	      v /= 10;
	      i++;
	    }
	  strcpy (buf, tmp + 29 - i);
	}
    }
  else
    {
      if (hex)
	sprintf (buf, "0x%x", (unsigned int) disp);
      else
	sprintf (buf, "%d", (int) disp);
    }
}

/* Put DISP in BUF as signed hex number.  */

static void
print_displacement (char *buf, bfd_vma disp)
{
  bfd_signed_vma val = disp;
  char tmp[30];
  int i, j = 0;

  if (val < 0)
    {
      buf[j++] = '-';
      val = -disp;

      /* Check for possible overflow.  */
      if (val < 0)
	{
	  switch (address_mode)
	    {
	    case mode_64bit:
	      strcpy (buf + j, "0x8000000000000000");
	      break;
	    case mode_32bit:
	      strcpy (buf + j, "0x80000000");
	      break;
	    case mode_16bit:
	      strcpy (buf + j, "0x8000");
	      break;
	    }
	  return;
	}
    }

  buf[j++] = '0';
  buf[j++] = 'x';

  sprintf_vma (tmp, (bfd_vma) val);
  for (i = 0; tmp[i] == '0'; i++)
    continue;
  if (tmp[i] == '\0')
    i--;
  strcpy (buf + j, tmp + i);
}

static void
intel_operand_size (int bytemode, int sizeflag)
{
  if (vex.evex
      && vex.b
      && (bytemode == x_mode
	  || bytemode == evex_half_bcst_xmmq_mode))
    {
      if (vex.w)
	oappend ("QWORD PTR ");
      else
	oappend ("DWORD PTR ");
      return;
    }
  switch (bytemode)
    {
    case b_mode:
    case b_swap_mode:
    case dqb_mode:
    case db_mode:
      oappend ("BYTE PTR ");
      break;
    case w_mode:
    case dw_mode:
    case dqw_mode:
    case dqw_swap_mode:
      oappend ("WORD PTR ");
      break;
    case stack_v_mode:
      if (address_mode == mode_64bit && ((sizeflag & DFLAG) || (rex & REX_W)))
	{
	  oappend ("QWORD PTR ");
	  break;
	}
      /* FALLTHRU */
    case v_mode:
    case v_swap_mode:
    case dq_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	oappend ("QWORD PTR ");
      else
	{
	  if ((sizeflag & DFLAG) || bytemode == dq_mode)
	    oappend ("DWORD PTR ");
	  else
	    oappend ("WORD PTR ");
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case z_mode:
      if ((rex & REX_W) || (sizeflag & DFLAG))
	*obufp++ = 'D';
      oappend ("WORD PTR ");
      if (!(rex & REX_W))
	used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case a_mode:
      if (sizeflag & DFLAG)
	oappend ("QWORD PTR ");
      else
	oappend ("DWORD PTR ");
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case d_mode:
    case d_scalar_mode:
    case d_scalar_swap_mode:
    case d_swap_mode:
    case dqd_mode:
      oappend ("DWORD PTR ");
      break;
    case q_mode:
    case q_scalar_mode:
    case q_scalar_swap_mode:
    case q_swap_mode:
      oappend ("QWORD PTR ");
      break;
    case m_mode:
      if (address_mode == mode_64bit)
	oappend ("QWORD PTR ");
      else
	oappend ("DWORD PTR ");
      break;
    case f_mode:
      if (sizeflag & DFLAG)
	oappend ("FWORD PTR ");
      else
	oappend ("DWORD PTR ");
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case t_mode:
      oappend ("TBYTE PTR ");
      break;
    case x_mode:
    case x_swap_mode:
    case evex_x_gscat_mode:
    case evex_x_nobcst_mode:
      if (need_vex)
	{
	  switch (vex.length)
	    {
	    case 128:
	      oappend ("XMMWORD PTR ");
	      break;
	    case 256:
	      oappend ("YMMWORD PTR ");
	      break;
	    case 512:
	      oappend ("ZMMWORD PTR ");
	      break;
	    default:
	      abort ();
	    }
	}
      else
	oappend ("XMMWORD PTR ");
      break;
    case xmm_mode:
      oappend ("XMMWORD PTR ");
      break;
    case ymm_mode:
      oappend ("YMMWORD PTR ");
      break;
    case xmmq_mode:
    case evex_half_bcst_xmmq_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  oappend ("QWORD PTR ");
	  break;
	case 256:
	  oappend ("XMMWORD PTR ");
	  break;
	case 512:
	  oappend ("YMMWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmm_mb_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	case 256:
	case 512:
	  oappend ("BYTE PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmm_mw_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	case 256:
	case 512:
	  oappend ("WORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmm_md_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	case 256:
	case 512:
	  oappend ("DWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmm_mq_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	case 256:
	case 512:
	  oappend ("QWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmmdw_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  oappend ("WORD PTR ");
	  break;
	case 256:
	  oappend ("DWORD PTR ");
	  break;
	case 512:
	  oappend ("QWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case xmmqd_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  oappend ("DWORD PTR ");
	  break;
	case 256:
	  oappend ("QWORD PTR ");
	  break;
	case 512:
	  oappend ("XMMWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case ymmq_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  oappend ("QWORD PTR ");
	  break;
	case 256:
	  oappend ("YMMWORD PTR ");
	  break;
	case 512:
	  oappend ("ZMMWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case ymmxmm_mode:
      if (!need_vex)
	abort ();

      switch (vex.length)
	{
	case 128:
	case 256:
	  oappend ("XMMWORD PTR ");
	  break;
	default:
	  abort ();
	}
      break;
    case o_mode:
      oappend ("OWORD PTR ");
      break;
    case xmm_mdq_mode:
    case vex_w_dq_mode:
    case vex_scalar_w_dq_mode:
      if (!need_vex)
	abort ();

      if (vex.w)
	oappend ("QWORD PTR ");
      else
	oappend ("DWORD PTR ");
      break;
    case vex_vsib_d_w_dq_mode:
    case vex_vsib_q_w_dq_mode:
      if (!need_vex)
	abort ();

      if (!vex.evex)
	{
	  if (vex.w)
	    oappend ("QWORD PTR ");
	  else
	    oappend ("DWORD PTR ");
	}
      else
	{
	  switch (vex.length)
	    {
	    case 128:
	      oappend ("XMMWORD PTR ");
	      break;
	    case 256:
	      oappend ("YMMWORD PTR ");
	      break;
	    case 512:
	      oappend ("ZMMWORD PTR ");
	      break;
	    default:
	      abort ();
	    }
	}
      break;
    case vex_vsib_q_w_d_mode:
    case vex_vsib_d_w_d_mode:
      if (!need_vex || !vex.evex)
	abort ();

      switch (vex.length)
	{
	case 128:
	  oappend ("QWORD PTR ");
	  break;
	case 256:
	  oappend ("XMMWORD PTR ");
	  break;
	case 512:
	  oappend ("YMMWORD PTR ");
	  break;
	default:
	  abort ();
	}

      break;
    case mask_bd_mode:
      if (!need_vex || vex.length != 128)
	abort ();
      if (vex.w)
	oappend ("DWORD PTR ");
      else
	oappend ("BYTE PTR ");
      break;
    case mask_mode:
      if (!need_vex)
	abort ();
      if (vex.w)
	oappend ("QWORD PTR ");
      else
	oappend ("WORD PTR ");
      break;
    case v_bnd_mode:
    default:
      break;
    }
}

static void
OP_E_register (int bytemode, int sizeflag)
{
  int reg = modrm.rm;
  const char **names;

  USED_REX (REX_B);
  if ((rex & REX_B))
    reg += 8;

  if ((sizeflag & SUFFIX_ALWAYS)
      && (bytemode == b_swap_mode
	  || bytemode == v_swap_mode
	  || bytemode == dqw_swap_mode))
    swap_operand ();

  switch (bytemode)
    {
    case b_mode:
    case b_swap_mode:
      USED_REX (0);
      if (rex)
	names = names8rex;
      else
	names = names8;
      break;
    case w_mode:
      names = names16;
      break;
    case d_mode:
    case dw_mode:
    case db_mode:
      names = names32;
      break;
    case q_mode:
      names = names64;
      break;
    case m_mode:
    case v_bnd_mode:
      names = address_mode == mode_64bit ? names64 : names32;
      break;
    case bnd_mode:
      names = names_bnd;
      break;
    case stack_v_mode:
      if (address_mode == mode_64bit && ((sizeflag & DFLAG) || (rex & REX_W)))
	{
	  names = names64;
	  break;
	}
      bytemode = v_mode;
      /* FALLTHRU */
    case v_mode:
    case v_swap_mode:
    case dq_mode:
    case dqb_mode:
    case dqd_mode:
    case dqw_mode:
    case dqw_swap_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	names = names64;
      else
	{
	  if ((sizeflag & DFLAG)
	      || (bytemode != v_mode
		  && bytemode != v_swap_mode))
	    names = names32;
	  else
	    names = names16;
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case mask_bd_mode:
    case mask_mode:
      names = names_mask;
      break;
    case 0:
      return;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }
  oappend (names[reg]);
}

static void
OP_E_memory (int bytemode, int sizeflag)
{
  bfd_vma disp = 0;
  int add = (rex & REX_B) ? 8 : 0;
  int riprel = 0;
  int shift;

  if (vex.evex)
    {
      /* In EVEX, if operand doesn't allow broadcast, vex.b should be 0.  */
      if (vex.b
	  && bytemode != x_mode
	  && bytemode != xmmq_mode
	  && bytemode != evex_half_bcst_xmmq_mode)
	{
	  BadOp ();
	  return;
	}
      switch (bytemode)
	{
	case dqw_mode:
	case dw_mode:
	case dqw_swap_mode:
	  shift = 1;
	  break;
	case dqb_mode:
	case db_mode:
	  shift = 0;
	  break;
	case vex_vsib_d_w_dq_mode:
	case vex_vsib_d_w_d_mode:
	case vex_vsib_q_w_dq_mode:
	case vex_vsib_q_w_d_mode:
	case evex_x_gscat_mode:
	case xmm_mdq_mode:
	  shift = vex.w ? 3 : 2;
	  break;
	case x_mode:
	case evex_half_bcst_xmmq_mode:
	case xmmq_mode:
	  if (vex.b)
	    {
	      shift = vex.w ? 3 : 2;
	      break;
	    }
	  /* Fall through if vex.b == 0.  */
	case xmmqd_mode:
	case xmmdw_mode:
	case ymmq_mode:
	case evex_x_nobcst_mode:
	case x_swap_mode:
	  switch (vex.length)
	    {
	    case 128:
	      shift = 4;
	      break;
	    case 256:
	      shift = 5;
	      break;
	    case 512:
	      shift = 6;
	      break;
	    default:
	      abort ();
	    }
	  break;
	case ymm_mode:
	  shift = 5;
	  break;
	case xmm_mode:
	  shift = 4;
	  break;
	case xmm_mq_mode:
	case q_mode:
	case q_scalar_mode:
	case q_swap_mode:
	case q_scalar_swap_mode:
	  shift = 3;
	  break;
	case dqd_mode:
	case xmm_md_mode:
	case d_mode:
	case d_scalar_mode:
	case d_swap_mode:
	case d_scalar_swap_mode:
	  shift = 2;
	  break;
	case xmm_mw_mode:
	  shift = 1;
	  break;
	case xmm_mb_mode:
	  shift = 0;
	  break;
	default:
	  abort ();
	}
      /* Make necessary corrections to shift for modes that need it.
	 For these modes we currently have shift 4, 5 or 6 depending on
	 vex.length (it corresponds to xmmword, ymmword or zmmword
	 operand).  We might want to make it 3, 4 or 5 (e.g. for
	 xmmq_mode).  In case of broadcast enabled the corrections
	 aren't needed, as element size is always 32 or 64 bits.  */
      if (!vex.b
	  && (bytemode == xmmq_mode
	      || bytemode == evex_half_bcst_xmmq_mode))
	shift -= 1;
      else if (bytemode == xmmqd_mode)
	shift -= 2;
      else if (bytemode == xmmdw_mode)
	shift -= 3;
      else if (bytemode == ymmq_mode && vex.length == 128)
	shift -= 1;
    }
  else
    shift = 0;

  USED_REX (REX_B);
  if (intel_syntax)
    intel_operand_size (bytemode, sizeflag);
  append_seg ();

  if ((sizeflag & AFLAG) || address_mode == mode_64bit)
    {
      /* 32/64 bit address mode */
      int havedisp;
      int havesib;
      int havebase;
      int haveindex;
      int needindex;
      int base, rbase;
      int vindex = 0;
      int scale = 0;
      int addr32flag = !((sizeflag & AFLAG)
			 || bytemode == v_bnd_mode
			 || bytemode == bnd_mode);
      const char **indexes64 = names64;
      const char **indexes32 = names32;

      havesib = 0;
      havebase = 1;
      haveindex = 0;
      base = modrm.rm;

      if (base == 4)
	{
	  havesib = 1;
	  vindex = sib.index;
	  USED_REX (REX_X);
	  if (rex & REX_X)
	    vindex += 8;
	  switch (bytemode)
	    {
	    case vex_vsib_d_w_dq_mode:
	    case vex_vsib_d_w_d_mode:
	    case vex_vsib_q_w_dq_mode:
	    case vex_vsib_q_w_d_mode:
	      if (!need_vex)
		abort ();
	      if (vex.evex)
		{
		  if (!vex.v)
		    vindex += 16;
		}

	      haveindex = 1;
	      switch (vex.length)
		{
		case 128:
		  indexes64 = indexes32 = names_xmm;
		  break;
		case 256:
		  if (!vex.w
		      || bytemode == vex_vsib_q_w_dq_mode
		      || bytemode == vex_vsib_q_w_d_mode)
		    indexes64 = indexes32 = names_ymm;
		  else
		    indexes64 = indexes32 = names_xmm;
		  break;
		case 512:
		  if (!vex.w
		      || bytemode == vex_vsib_q_w_dq_mode
		      || bytemode == vex_vsib_q_w_d_mode)
		    indexes64 = indexes32 = names_zmm;
		  else
		    indexes64 = indexes32 = names_ymm;
		  break;
		default:
		  abort ();
		}
	      break;
	    default:
	      haveindex = vindex != 4;
	      break;
	    }
	  scale = sib.scale;
	  base = sib.base;
	  codep++;
	}
      rbase = base + add;

      switch (modrm.mod)
	{
	case 0:
	  if (base == 5)
	    {
	      havebase = 0;
	      if (address_mode == mode_64bit && !havesib)
		riprel = 1;
	      disp = get32s ();
	    }
	  break;
	case 1:
	  FETCH_DATA (the_info, codep + 1);
	  disp = *codep++;
	  if ((disp & 0x80) != 0)
	    disp -= 0x100;
	  if (vex.evex && shift > 0)
	    disp <<= shift;
	  break;
	case 2:
	  disp = get32s ();
	  break;
	}

      /* In 32bit mode, we need index register to tell [offset] from
	 [eiz*1 + offset].  */
      needindex = (havesib
		   && !havebase
		   && !haveindex
		   && address_mode == mode_32bit);
      havedisp = (havebase
		  || needindex
		  || (havesib && (haveindex || scale != 0)));

      if (!intel_syntax)
	if (modrm.mod != 0 || base == 5)
	  {
	    if (havedisp || riprel)
	      print_displacement (scratchbuf, disp);
	    else
	      print_operand_value (scratchbuf, 1, disp);
	    oappend (scratchbuf);
	    if (riprel)
	      {
		set_op (disp, 1);
		oappend (sizeflag & AFLAG ? "(%rip)" : "(%eip)");
	      }
	  }

      if ((havebase || haveindex || riprel)
	  && (bytemode != v_bnd_mode)
	  && (bytemode != bnd_mode))
	used_prefixes |= PREFIX_ADDR;

      if (havedisp || (intel_syntax && riprel))
	{
	  *obufp++ = open_char;
	  if (intel_syntax && riprel)
	    {
	      set_op (disp, 1);
	      oappend (sizeflag & AFLAG ? "rip" : "eip");
	    }
	  *obufp = '\0';
	  if (havebase)
	    oappend (address_mode == mode_64bit && !addr32flag
		     ? names64[rbase] : names32[rbase]);
	  if (havesib)
	    {
	      /* ESP/RSP won't allow index.  If base isn't ESP/RSP,
		 print index to tell base + index from base.  */
	      if (scale != 0
		  || needindex
		  || haveindex
		  || (havebase && base != ESP_REG_NUM))
		{
		  if (!intel_syntax || havebase)
		    {
		      *obufp++ = separator_char;
		      *obufp = '\0';
		    }
		  if (haveindex)
		    oappend (address_mode == mode_64bit && !addr32flag
			     ? indexes64[vindex] : indexes32[vindex]);
		  else
		    oappend (address_mode == mode_64bit && !addr32flag
			     ? index64 : index32);

		  *obufp++ = scale_char;
		  *obufp = '\0';
		  sprintf (scratchbuf, "%d", 1 << scale);
		  oappend (scratchbuf);
		}
	    }
	  if (intel_syntax
	      && (disp || modrm.mod != 0 || base == 5))
	    {
	      if (!havedisp || (bfd_signed_vma) disp >= 0)
		{
		  *obufp++ = '+';
		  *obufp = '\0';
		}
	      else if (modrm.mod != 1 && disp != -disp)
		{
		  *obufp++ = '-';
		  *obufp = '\0';
		  disp = - (bfd_signed_vma) disp;
		}

	      if (havedisp)
		print_displacement (scratchbuf, disp);
	      else
		print_operand_value (scratchbuf, 1, disp);
	      oappend (scratchbuf);
	    }

	  *obufp++ = close_char;
	  *obufp = '\0';
	}
      else if (intel_syntax)
	{
	  if (modrm.mod != 0 || base == 5)
	    {
	      if (!active_seg_prefix)
		{
		  oappend (names_seg[ds_reg - es_reg]);
		  oappend (":");
		}
	      print_operand_value (scratchbuf, 1, disp);
	      oappend (scratchbuf);
	    }
	}
    }
  else
    {
      /* 16 bit address mode */
      used_prefixes |= prefixes & PREFIX_ADDR;
      switch (modrm.mod)
	{
	case 0:
	  if (modrm.rm == 6)
	    {
	      disp = get16 ();
	      if ((disp & 0x8000) != 0)
		disp -= 0x10000;
	    }
	  break;
	case 1:
	  FETCH_DATA (the_info, codep + 1);
	  disp = *codep++;
	  if ((disp & 0x80) != 0)
	    disp -= 0x100;
	  break;
	case 2:
	  disp = get16 ();
	  if ((disp & 0x8000) != 0)
	    disp -= 0x10000;
	  break;
	}

      if (!intel_syntax)
	if (modrm.mod != 0 || modrm.rm == 6)
	  {
	    print_displacement (scratchbuf, disp);
	    oappend (scratchbuf);
	  }

      if (modrm.mod != 0 || modrm.rm != 6)
	{
	  *obufp++ = open_char;
	  *obufp = '\0';
	  oappend (index16[modrm.rm]);
	  if (intel_syntax
	      && (disp || modrm.mod != 0 || modrm.rm == 6))
	    {
	      if ((bfd_signed_vma) disp >= 0)
		{
		  *obufp++ = '+';
		  *obufp = '\0';
		}
	      else if (modrm.mod != 1)
		{
		  *obufp++ = '-';
		  *obufp = '\0';
		  disp = - (bfd_signed_vma) disp;
		}

	      print_displacement (scratchbuf, disp);
	      oappend (scratchbuf);
	    }

	  *obufp++ = close_char;
	  *obufp = '\0';
	}
      else if (intel_syntax)
	{
	  if (!active_seg_prefix)
	    {
	      oappend (names_seg[ds_reg - es_reg]);
	      oappend (":");
	    }
	  print_operand_value (scratchbuf, 1, disp & 0xffff);
	  oappend (scratchbuf);
	}
    }
  if (vex.evex && vex.b
      && (bytemode == x_mode
	  || bytemode == xmmq_mode
	  || bytemode == evex_half_bcst_xmmq_mode))
    {
      if (vex.w
	  || bytemode == xmmq_mode
	  || bytemode == evex_half_bcst_xmmq_mode)
	{
	  switch (vex.length)
	    {
	    case 128:
	      oappend ("{1to2}");
	      break;
	    case 256:
	      oappend ("{1to4}");
	      break;
	    case 512:
	      oappend ("{1to8}");
	      break;
	    default:
	      abort ();
	    }
	}
      else
	{
	  switch (vex.length)
	    {
	    case 128:
	      oappend ("{1to4}");
	      break;
	    case 256:
	      oappend ("{1to8}");
	      break;
	    case 512:
	      oappend ("{1to16}");
	      break;
	    default:
	      abort ();
	    }
	}
    }
}

static void
OP_E (int bytemode, int sizeflag)
{
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  if (modrm.mod == 3)
    OP_E_register (bytemode, sizeflag);
  else
    OP_E_memory (bytemode, sizeflag);
}

static void
OP_G (int bytemode, int sizeflag)
{
  int add = 0;
  USED_REX (REX_R);
  if (rex & REX_R)
    add += 8;
  switch (bytemode)
    {
    case b_mode:
      USED_REX (0);
      if (rex)
	oappend (names8rex[modrm.reg + add]);
      else
	oappend (names8[modrm.reg + add]);
      break;
    case w_mode:
      oappend (names16[modrm.reg + add]);
      break;
    case d_mode:
    case db_mode:
    case dw_mode:
      oappend (names32[modrm.reg + add]);
      break;
    case q_mode:
      oappend (names64[modrm.reg + add]);
      break;
    case bnd_mode:
      oappend (names_bnd[modrm.reg]);
      break;
    case v_mode:
    case dq_mode:
    case dqb_mode:
    case dqd_mode:
    case dqw_mode:
    case dqw_swap_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	oappend (names64[modrm.reg + add]);
      else
	{
	  if ((sizeflag & DFLAG) || bytemode != v_mode)
	    oappend (names32[modrm.reg + add]);
	  else
	    oappend (names16[modrm.reg + add]);
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case m_mode:
      if (address_mode == mode_64bit)
	oappend (names64[modrm.reg + add]);
      else
	oappend (names32[modrm.reg + add]);
      break;
    case mask_bd_mode:
    case mask_mode:
      oappend (names_mask[modrm.reg + add]);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
}

static bfd_vma
get64 (void)
{
  bfd_vma x;
#ifdef BFD64
  unsigned int a;
  unsigned int b;

  FETCH_DATA (the_info, codep + 8);
  a = *codep++ & 0xff;
  a |= (*codep++ & 0xff) << 8;
  a |= (*codep++ & 0xff) << 16;
  a |= (*codep++ & 0xff) << 24;
  b = *codep++ & 0xff;
  b |= (*codep++ & 0xff) << 8;
  b |= (*codep++ & 0xff) << 16;
  b |= (*codep++ & 0xff) << 24;
  x = a + ((bfd_vma) b << 32);
#else
  abort ();
  x = 0;
#endif
  return x;
}

static bfd_signed_vma
get32 (void)
{
  bfd_signed_vma x = 0;

  FETCH_DATA (the_info, codep + 4);
  x = *codep++ & (bfd_signed_vma) 0xff;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 8;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 16;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 24;
  return x;
}

static bfd_signed_vma
get32s (void)
{
  bfd_signed_vma x = 0;

  FETCH_DATA (the_info, codep + 4);
  x = *codep++ & (bfd_signed_vma) 0xff;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 8;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 16;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 24;

  x = (x ^ ((bfd_signed_vma) 1 << 31)) - ((bfd_signed_vma) 1 << 31);

  return x;
}

static int
get16 (void)
{
  int x = 0;

  FETCH_DATA (the_info, codep + 2);
  x = *codep++ & 0xff;
  x |= (*codep++ & 0xff) << 8;
  return x;
}

static void
set_op (bfd_vma op, int riprel)
{
  op_index[op_ad] = op_ad;
  if (address_mode == mode_64bit)
    {
      op_address[op_ad] = op;
      op_riprel[op_ad] = riprel;
    }
  else
    {
      /* Mask to get a 32-bit address.  */
      op_address[op_ad] = op & 0xffffffff;
      op_riprel[op_ad] = riprel & 0xffffffff;
    }
}

static void
OP_REG (int code, int sizeflag)
{
  const char *s;
  int add;

  switch (code)
    {
    case es_reg: case ss_reg: case cs_reg:
    case ds_reg: case fs_reg: case gs_reg:
      oappend (names_seg[code - es_reg]);
      return;
    }

  USED_REX (REX_B);
  if (rex & REX_B)
    add = 8;
  else
    add = 0;

  switch (code)
    {
    case ax_reg: case cx_reg: case dx_reg: case bx_reg:
    case sp_reg: case bp_reg: case si_reg: case di_reg:
      s = names16[code - ax_reg + add];
      break;
    case al_reg: case ah_reg: case cl_reg: case ch_reg:
    case dl_reg: case dh_reg: case bl_reg: case bh_reg:
      USED_REX (0);
      if (rex)
	s = names8rex[code - al_reg + add];
      else
	s = names8[code - al_reg];
      break;
    case rAX_reg: case rCX_reg: case rDX_reg: case rBX_reg:
    case rSP_reg: case rBP_reg: case rSI_reg: case rDI_reg:
      if (address_mode == mode_64bit
	  && ((sizeflag & DFLAG) || (rex & REX_W)))
	{
	  s = names64[code - rAX_reg + add];
	  break;
	}
      code += eAX_reg - rAX_reg;
      /* Fall through.  */
    case eAX_reg: case eCX_reg: case eDX_reg: case eBX_reg:
    case eSP_reg: case eBP_reg: case eSI_reg: case eDI_reg:
      USED_REX (REX_W);
      if (rex & REX_W)
	s = names64[code - eAX_reg + add];
      else
	{
	  if (sizeflag & DFLAG)
	    s = names32[code - eAX_reg + add];
	  else
	    s = names16[code - eAX_reg + add];
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    default:
      s = INTERNAL_DISASSEMBLER_ERROR;
      break;
    }
  oappend (s);
}

static void
OP_IMREG (int code, int sizeflag)
{
  const char *s;

  switch (code)
    {
    case indir_dx_reg:
      if (intel_syntax)
	s = "dx";
      else
	s = "(%dx)";
      break;
    case ax_reg: case cx_reg: case dx_reg: case bx_reg:
    case sp_reg: case bp_reg: case si_reg: case di_reg:
      s = names16[code - ax_reg];
      break;
    case es_reg: case ss_reg: case cs_reg:
    case ds_reg: case fs_reg: case gs_reg:
      s = names_seg[code - es_reg];
      break;
    case al_reg: case ah_reg: case cl_reg: case ch_reg:
    case dl_reg: case dh_reg: case bl_reg: case bh_reg:
      USED_REX (0);
      if (rex)
	s = names8rex[code - al_reg];
      else
	s = names8[code - al_reg];
      break;
    case eAX_reg: case eCX_reg: case eDX_reg: case eBX_reg:
    case eSP_reg: case eBP_reg: case eSI_reg: case eDI_reg:
      USED_REX (REX_W);
      if (rex & REX_W)
	s = names64[code - eAX_reg];
      else
	{
	  if (sizeflag & DFLAG)
	    s = names32[code - eAX_reg];
	  else
	    s = names16[code - eAX_reg];
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case z_mode_ax_reg:
      if ((rex & REX_W) || (sizeflag & DFLAG))
	s = *names32;
      else
	s = *names16;
      if (!(rex & REX_W))
	used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      s = INTERNAL_DISASSEMBLER_ERROR;
      break;
    }
  oappend (s);
}

static void
OP_I (int bytemode, int sizeflag)
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      mask = 0xff;
      break;
    case q_mode:
      if (address_mode == mode_64bit)
	{
	  op = get32s ();
	  break;
	}
      /* Fall through.  */
    case v_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	op = get32s ();
      else
	{
	  if (sizeflag & DFLAG)
	    {
	      op = get32 ();
	      mask = 0xffffffff;
	    }
	  else
	    {
	      op = get16 ();
	      mask = 0xfffff;
	    }
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case w_mode:
      mask = 0xfffff;
      op = get16 ();
      break;
    case const_1_mode:
      if (intel_syntax)
	oappend ("1");
      return;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  op &= mask;
  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend_maybe_intel (scratchbuf);
  scratchbuf[0] = '\0';
}

static void
OP_I64 (int bytemode, int sizeflag)
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  if (address_mode != mode_64bit)
    {
      OP_I (bytemode, sizeflag);
      return;
    }

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      mask = 0xff;
      break;
    case v_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	op = get64 ();
      else
	{
	  if (sizeflag & DFLAG)
	    {
	      op = get32 ();
	      mask = 0xffffffff;
	    }
	  else
	    {
	      op = get16 ();
	      mask = 0xfffff;
	    }
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    case w_mode:
      mask = 0xfffff;
      op = get16 ();
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  op &= mask;
  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend_maybe_intel (scratchbuf);
  scratchbuf[0] = '\0';
}

static void
OP_sI (int bytemode, int sizeflag)
{
  bfd_signed_vma op;

  switch (bytemode)
    {
    case b_mode:
    case b_T_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      if ((op & 0x80) != 0)
	op -= 0x100;
      if (bytemode == b_T_mode)
	{
	  if (address_mode != mode_64bit
	      || !((sizeflag & DFLAG) || (rex & REX_W)))
	    {
	      /* The operand-size prefix is overridden by a REX prefix.  */
	      if ((sizeflag & DFLAG) || (rex & REX_W))
		op &= 0xffffffff;
	      else
		op &= 0xffff;
	  }
	}
      else
	{
	  if (!(rex & REX_W))
	    {
	      if (sizeflag & DFLAG)
		op &= 0xffffffff;
	      else
		op &= 0xffff;
	    }
	}
      break;
    case v_mode:
      /* The operand-size prefix is overridden by a REX prefix.  */
      if ((sizeflag & DFLAG) || (rex & REX_W))
	op = get32s ();
      else
	op = get16 ();
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend_maybe_intel (scratchbuf);
}

static void
OP_J (int bytemode, int sizeflag)
{
  bfd_vma disp;
  bfd_vma mask = -1;
  bfd_vma segment = 0;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      disp = *codep++;
      if ((disp & 0x80) != 0)
	disp -= 0x100;
      break;
    case v_mode:
      if (isa64 == amd64)
	USED_REX (REX_W);
      if ((sizeflag & DFLAG)
	  || (address_mode == mode_64bit
	      && (isa64 != amd64 || (rex & REX_W))))
	disp = get32s ();
      else
	{
	  disp = get16 ();
	  if ((disp & 0x8000) != 0)
	    disp -= 0x10000;
	  /* In 16bit mode, address is wrapped around at 64k within
	     the same segment.  Otherwise, a data16 prefix on a jump
	     instruction means that the pc is masked to 16 bits after
	     the displacement is added!  */
	  mask = 0xffff;
	  if ((prefixes & PREFIX_DATA) == 0)
	    segment = ((start_pc + codep - start_codep)
		       & ~((bfd_vma) 0xffff));
	}
      if (address_mode != mode_64bit
	  || (isa64 == amd64 && !(rex & REX_W)))
	used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }
  disp = ((start_pc + (codep - start_codep) + disp) & mask) | segment;
  set_op (disp, 0);
  print_operand_value (scratchbuf, 1, disp);
  oappend (scratchbuf);
}

static void
OP_SEG (int bytemode, int sizeflag)
{
  if (bytemode == w_mode)
    oappend (names_seg[modrm.reg]);
  else
    OP_E (modrm.mod == 3 ? bytemode : w_mode, sizeflag);
}

static void
OP_DIR (int dummy ATTRIBUTE_UNUSED, int sizeflag)
{
  int seg, offset;

  if (sizeflag & DFLAG)
    {
      offset = get32 ();
      seg = get16 ();
    }
  else
    {
      offset = get16 ();
      seg = get16 ();
    }
  used_prefixes |= (prefixes & PREFIX_DATA);
  if (intel_syntax)
    sprintf (scratchbuf, "0x%x:0x%x", seg, offset);
  else
    sprintf (scratchbuf, "$0x%x,$0x%x", seg, offset);
  oappend (scratchbuf);
}

static void
OP_OFF (int bytemode, int sizeflag)
{
  bfd_vma off;

  if (intel_syntax && (sizeflag & SUFFIX_ALWAYS))
    intel_operand_size (bytemode, sizeflag);
  append_seg ();

  if ((sizeflag & AFLAG) || address_mode == mode_64bit)
    off = get32 ();
  else
    off = get16 ();

  if (intel_syntax)
    {
      if (!active_seg_prefix)
	{
	  oappend (names_seg[ds_reg - es_reg]);
	  oappend (":");
	}
    }
  print_operand_value (scratchbuf, 1, off);
  oappend (scratchbuf);
}

static void
OP_OFF64 (int bytemode, int sizeflag)
{
  bfd_vma off;

  if (address_mode != mode_64bit
      || (prefixes & PREFIX_ADDR))
    {
      OP_OFF (bytemode, sizeflag);
      return;
    }

  if (intel_syntax && (sizeflag & SUFFIX_ALWAYS))
    intel_operand_size (bytemode, sizeflag);
  append_seg ();

  off = get64 ();

  if (intel_syntax)
    {
      if (!active_seg_prefix)
	{
	  oappend (names_seg[ds_reg - es_reg]);
	  oappend (":");
	}
    }
  print_operand_value (scratchbuf, 1, off);
  oappend (scratchbuf);
}

static void
ptr_reg (int code, int sizeflag)
{
  const char *s;

  *obufp++ = open_char;
  used_prefixes |= (prefixes & PREFIX_ADDR);
  if (address_mode == mode_64bit)
    {
      if (!(sizeflag & AFLAG))
	s = names32[code - eAX_reg];
      else
	s = names64[code - eAX_reg];
    }
  else if (sizeflag & AFLAG)
    s = names32[code - eAX_reg];
  else
    s = names16[code - eAX_reg];
  oappend (s);
  *obufp++ = close_char;
  *obufp = 0;
}

static void
OP_ESreg (int code, int sizeflag)
{
  if (intel_syntax)
    {
      switch (codep[-1])
	{
	case 0x6d:	/* insw/insl */
	  intel_operand_size (z_mode, sizeflag);
	  break;
	case 0xa5:	/* movsw/movsl/movsq */
	case 0xa7:	/* cmpsw/cmpsl/cmpsq */
	case 0xab:	/* stosw/stosl */
	case 0xaf:	/* scasw/scasl */
	  intel_operand_size (v_mode, sizeflag);
	  break;
	default:
	  intel_operand_size (b_mode, sizeflag);
	}
    }
  oappend_maybe_intel ("%es:");
  ptr_reg (code, sizeflag);
}

static void
OP_DSreg (int code, int sizeflag)
{
  if (intel_syntax)
    {
      switch (codep[-1])
	{
	case 0x6f:	/* outsw/outsl */
	  intel_operand_size (z_mode, sizeflag);
	  break;
	case 0xa5:	/* movsw/movsl/movsq */
	case 0xa7:	/* cmpsw/cmpsl/cmpsq */
	case 0xad:	/* lodsw/lodsl/lodsq */
	  intel_operand_size (v_mode, sizeflag);
	  break;
	default:
	  intel_operand_size (b_mode, sizeflag);
	}
    }
  /* Set active_seg_prefix to PREFIX_DS if it is unset so that the
     default segment register DS is printed.  */
  if (!active_seg_prefix)
    active_seg_prefix = PREFIX_DS;
  append_seg ();
  ptr_reg (code, sizeflag);
}

static void
OP_C (int dummy ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  int add;
  if (rex & REX_R)
    {
      USED_REX (REX_R);
      add = 8;
    }
  else if (address_mode != mode_64bit && (prefixes & PREFIX_LOCK))
    {
      all_prefixes[last_lock_prefix] = 0;
      used_prefixes |= PREFIX_LOCK;
      add = 8;
    }
  else
    add = 0;
  sprintf (scratchbuf, "%%cr%d", modrm.reg + add);
  oappend_maybe_intel (scratchbuf);
}

static void
OP_D (int dummy ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  int add;
  USED_REX (REX_R);
  if (rex & REX_R)
    add = 8;
  else
    add = 0;
  if (intel_syntax)
    sprintf (scratchbuf, "db%d", modrm.reg + add);
  else
    sprintf (scratchbuf, "%%db%d", modrm.reg + add);
  oappend (scratchbuf);
}

static void
OP_T (int dummy ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  sprintf (scratchbuf, "%%tr%d", modrm.reg);
  oappend_maybe_intel (scratchbuf);
}

static void
OP_R (int bytemode, int sizeflag)
{
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  OP_E_register (bytemode, sizeflag);
}

static void
OP_MMX (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  int reg = modrm.reg;
  const char **names;

  used_prefixes |= (prefixes & PREFIX_DATA);
  if (prefixes & PREFIX_DATA)
    {
      names = names_xmm;
      USED_REX (REX_R);
      if (rex & REX_R)
	reg += 8;
    }
  else
    names = names_mm;
  oappend (names[reg]);
}

static void
OP_XMM (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  int reg = modrm.reg;
  const char **names;

  USED_REX (REX_R);
  if (rex & REX_R)
    reg += 8;
  if (vex.evex)
    {
      if (!vex.r)
	reg += 16;
    }

  if (need_vex
      && bytemode != xmm_mode
      && bytemode != xmmq_mode
      && bytemode != evex_half_bcst_xmmq_mode
      && bytemode != ymm_mode
      && bytemode != scalar_mode)
    {
      switch (vex.length)
	{
	case 128:
	  names = names_xmm;
	  break;
	case 256:
	  if (vex.w
	      || (bytemode != vex_vsib_q_w_dq_mode
		  && bytemode != vex_vsib_q_w_d_mode))
	    names = names_ymm;
	  else
	    names = names_xmm;
	  break;
	case 512:
	  names = names_zmm;
	  break;
	default:
	  abort ();
	}
    }
  else if (bytemode == xmmq_mode
	   || bytemode == evex_half_bcst_xmmq_mode)
    {
      switch (vex.length)
	{
	case 128:
	case 256:
	  names = names_xmm;
	  break;
	case 512:
	  names = names_ymm;
	  break;
	default:
	  abort ();
	}
    }
  else if (bytemode == ymm_mode)
    names = names_ymm;
  else
    names = names_xmm;
  oappend (names[reg]);
}

static void
OP_EM (int bytemode, int sizeflag)
{
  int reg;
  const char **names;

  if (modrm.mod != 3)
    {
      if (intel_syntax
	  && (bytemode == v_mode || bytemode == v_swap_mode))
	{
	  bytemode = (prefixes & PREFIX_DATA) ? x_mode : q_mode;
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      OP_E (bytemode, sizeflag);
      return;
    }

  if ((sizeflag & SUFFIX_ALWAYS) && bytemode == v_swap_mode)
    swap_operand ();

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  used_prefixes |= (prefixes & PREFIX_DATA);
  reg = modrm.rm;
  if (prefixes & PREFIX_DATA)
    {
      names = names_xmm;
      USED_REX (REX_B);
      if (rex & REX_B)
	reg += 8;
    }
  else
    names = names_mm;
  oappend (names[reg]);
}

/* cvt* are the only instructions in sse2 which have
   both SSE and MMX operands and also have 0x66 prefix
   in their opcode. 0x66 was originally used to differentiate
   between SSE and MMX instruction(operands). So we have to handle the
   cvt* separately using OP_EMC and OP_MXC */
static void
OP_EMC (int bytemode, int sizeflag)
{
  if (modrm.mod != 3)
    {
      if (intel_syntax && bytemode == v_mode)
	{
	  bytemode = (prefixes & PREFIX_DATA) ? x_mode : q_mode;
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      OP_E (bytemode, sizeflag);
      return;
    }

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  used_prefixes |= (prefixes & PREFIX_DATA);
  oappend (names_mm[modrm.rm]);
}

static void
OP_MXC (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  used_prefixes |= (prefixes & PREFIX_DATA);
  oappend (names_mm[modrm.reg]);
}

static void
OP_EX (int bytemode, int sizeflag)
{
  int reg;
  const char **names;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  if (modrm.mod != 3)
    {
      OP_E_memory (bytemode, sizeflag);
      return;
    }

  reg = modrm.rm;
  USED_REX (REX_B);
  if (rex & REX_B)
    reg += 8;
  if (vex.evex)
    {
      USED_REX (REX_X);
      if ((rex & REX_X))
	reg += 16;
    }

  if ((sizeflag & SUFFIX_ALWAYS)
      && (bytemode == x_swap_mode
	  || bytemode == d_swap_mode
	  || bytemode == dqw_swap_mode
	  || bytemode == d_scalar_swap_mode
	  || bytemode == q_swap_mode
	  || bytemode == q_scalar_swap_mode))
    swap_operand ();

  if (need_vex
      && bytemode != xmm_mode
      && bytemode != xmmdw_mode
      && bytemode != xmmqd_mode
      && bytemode != xmm_mb_mode
      && bytemode != xmm_mw_mode
      && bytemode != xmm_md_mode
      && bytemode != xmm_mq_mode
      && bytemode != xmm_mdq_mode
      && bytemode != xmmq_mode
      && bytemode != evex_half_bcst_xmmq_mode
      && bytemode != ymm_mode
      && bytemode != d_scalar_mode
      && bytemode != d_scalar_swap_mode
      && bytemode != q_scalar_mode
      && bytemode != q_scalar_swap_mode
      && bytemode != vex_scalar_w_dq_mode)
    {
      switch (vex.length)
	{
	case 128:
	  names = names_xmm;
	  break;
	case 256:
	  names = names_ymm;
	  break;
	case 512:
	  names = names_zmm;
	  break;
	default:
	  abort ();
	}
    }
  else if (bytemode == xmmq_mode
	   || bytemode == evex_half_bcst_xmmq_mode)
    {
      switch (vex.length)
	{
	case 128:
	case 256:
	  names = names_xmm;
	  break;
	case 512:
	  names = names_ymm;
	  break;
	default:
	  abort ();
	}
    }
  else if (bytemode == ymm_mode)
    names = names_ymm;
  else
    names = names_xmm;
  oappend (names[reg]);
}

static void
OP_MS (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    OP_EM (bytemode, sizeflag);
  else
    BadOp ();
}

static void
OP_XS (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    OP_EX (bytemode, sizeflag);
  else
    BadOp ();
}

static void
OP_M (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    /* bad bound,lea,lds,les,lfs,lgs,lss,cmpxchg8b,vmptrst modrm */
    BadOp ();
  else
    OP_E (bytemode, sizeflag);
}

static void
OP_0f07 (int bytemode, int sizeflag)
{
  if (modrm.mod != 3 || modrm.rm != 0)
    BadOp ();
  else
    OP_E (bytemode, sizeflag);
}

/* NOP is an alias of "xchg %ax,%ax" in 16bit mode, "xchg %eax,%eax" in
   32bit mode and "xchg %rax,%rax" in 64bit mode.  */

static void
NOP_Fixup1 (int bytemode, int sizeflag)
{
  if ((prefixes & PREFIX_DATA) != 0
      || (rex != 0
	  && rex != 0x48
	  && address_mode == mode_64bit))
    OP_REG (bytemode, sizeflag);
  else
    strcpy (obuf, "nop");
}

static void
NOP_Fixup2 (int bytemode, int sizeflag)
{
  if ((prefixes & PREFIX_DATA) != 0
      || (rex != 0
	  && rex != 0x48
	  && address_mode == mode_64bit))
    OP_IMREG (bytemode, sizeflag);
}

static const char *const Suffix3DNow[] = {
/* 00 */	NULL,		NULL,		NULL,		NULL,
/* 04 */	NULL,		NULL,		NULL,		NULL,
/* 08 */	NULL,		NULL,		NULL,		NULL,
/* 0C */	"pi2fw",	"pi2fd",	NULL,		NULL,
/* 10 */	NULL,		NULL,		NULL,		NULL,
/* 14 */	NULL,		NULL,		NULL,		NULL,
/* 18 */	NULL,		NULL,		NULL,		NULL,
/* 1C */	"pf2iw",	"pf2id",	NULL,		NULL,
/* 20 */	NULL,		NULL,		NULL,		NULL,
/* 24 */	NULL,		NULL,		NULL,		NULL,
/* 28 */	NULL,		NULL,		NULL,		NULL,
/* 2C */	NULL,		NULL,		NULL,		NULL,
/* 30 */	NULL,		NULL,		NULL,		NULL,
/* 34 */	NULL,		NULL,		NULL,		NULL,
/* 38 */	NULL,		NULL,		NULL,		NULL,
/* 3C */	NULL,		NULL,		NULL,		NULL,
/* 40 */	NULL,		NULL,		NULL,		NULL,
/* 44 */	NULL,		NULL,		NULL,		NULL,
/* 48 */	NULL,		NULL,		NULL,		NULL,
/* 4C */	NULL,		NULL,		NULL,		NULL,
/* 50 */	NULL,		NULL,		NULL,		NULL,
/* 54 */	NULL,		NULL,		NULL,		NULL,
/* 58 */	NULL,		NULL,		NULL,		NULL,
/* 5C */	NULL,		NULL,		NULL,		NULL,
/* 60 */	NULL,		NULL,		NULL,		NULL,
/* 64 */	NULL,		NULL,		NULL,		NULL,
/* 68 */	NULL,		NULL,		NULL,		NULL,
/* 6C */	NULL,		NULL,		NULL,		NULL,
/* 70 */	NULL,		NULL,		NULL,		NULL,
/* 74 */	NULL,		NULL,		NULL,		NULL,
/* 78 */	NULL,		NULL,		NULL,		NULL,
/* 7C */	NULL,		NULL,		NULL,		NULL,
/* 80 */	NULL,		NULL,		NULL,		NULL,
/* 84 */	NULL,		NULL,		NULL,		NULL,
/* 88 */	NULL,		NULL,		"pfnacc",	NULL,
/* 8C */	NULL,		NULL,		"pfpnacc",	NULL,
/* 90 */	"pfcmpge",	NULL,		NULL,		NULL,
/* 94 */	"pfmin",	NULL,		"pfrcp",	"pfrsqrt",
/* 98 */	NULL,		NULL,		"pfsub",	NULL,
/* 9C */	NULL,		NULL,		"pfadd",	NULL,
/* A0 */	"pfcmpgt",	NULL,		NULL,		NULL,
/* A4 */	"pfmax",	NULL,		"pfrcpit1",	"pfrsqit1",
/* A8 */	NULL,		NULL,		"pfsubr",	NULL,
/* AC */	NULL,		NULL,		"pfacc",	NULL,
/* B0 */	"pfcmpeq",	NULL,		NULL,		NULL,
/* B4 */	"pfmul",	NULL,		"pfrcpit2",	"pmulhrw",
/* B8 */	NULL,		NULL,		NULL,		"pswapd",
/* BC */	NULL,		NULL,		NULL,		"pavgusb",
/* C0 */	NULL,		NULL,		NULL,		NULL,
/* C4 */	NULL,		NULL,		NULL,		NULL,
/* C8 */	NULL,		NULL,		NULL,		NULL,
/* CC */	NULL,		NULL,		NULL,		NULL,
/* D0 */	NULL,		NULL,		NULL,		NULL,
/* D4 */	NULL,		NULL,		NULL,		NULL,
/* D8 */	NULL,		NULL,		NULL,		NULL,
/* DC */	NULL,		NULL,		NULL,		NULL,
/* E0 */	NULL,		NULL,		NULL,		NULL,
/* E4 */	NULL,		NULL,		NULL,		NULL,
/* E8 */	NULL,		NULL,		NULL,		NULL,
/* EC */	NULL,		NULL,		NULL,		NULL,
/* F0 */	NULL,		NULL,		NULL,		NULL,
/* F4 */	NULL,		NULL,		NULL,		NULL,
/* F8 */	NULL,		NULL,		NULL,		NULL,
/* FC */	NULL,		NULL,		NULL,		NULL,
};

static void
OP_3DNowSuffix (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  const char *mnemonic;

  FETCH_DATA (the_info, codep + 1);
  /* AMD 3DNow! instructions are specified by an opcode suffix in the
     place where an 8-bit immediate would normally go.  ie. the last
     byte of the instruction.  */
  obufp = mnemonicendp;
  mnemonic = Suffix3DNow[*codep++ & 0xff];
  if (mnemonic)
    oappend (mnemonic);
  else
    {
      /* Since a variable sized modrm/sib chunk is between the start
	 of the opcode (0x0f0f) and the opcode suffix, we need to do
	 all the modrm processing first, and don't know until now that
	 we have a bad opcode.  This necessitates some cleaning up.  */
      op_out[0][0] = '\0';
      op_out[1][0] = '\0';
      BadOp ();
    }
  mnemonicendp = obufp;
}

static struct op simd_cmp_op[] =
{
  { STRING_COMMA_LEN ("eq") },
  { STRING_COMMA_LEN ("lt") },
  { STRING_COMMA_LEN ("le") },
  { STRING_COMMA_LEN ("unord") },
  { STRING_COMMA_LEN ("neq") },
  { STRING_COMMA_LEN ("nlt") },
  { STRING_COMMA_LEN ("nle") },
  { STRING_COMMA_LEN ("ord") }
};

static void
CMP_Fixup (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned int cmp_type;

  FETCH_DATA (the_info, codep + 1);
  cmp_type = *codep++ & 0xff;
  if (cmp_type < ARRAY_SIZE (simd_cmp_op))
    {
      char suffix [3];
      char *p = mnemonicendp - 2;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = '\0';
      sprintf (p, "%s%s", simd_cmp_op[cmp_type].name, suffix);
      mnemonicendp += simd_cmp_op[cmp_type].len;
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, cmp_type);
      oappend_maybe_intel (scratchbuf);
      scratchbuf[0] = '\0';
    }
}

static void
OP_Mwaitx (int bytemode ATTRIBUTE_UNUSED,
	  int sizeflag ATTRIBUTE_UNUSED)
{
  /* mwaitx %eax,%ecx,%ebx */
  if (!intel_syntax)
    {
      const char **names = (address_mode == mode_64bit
			    ? names64 : names32);
      strcpy (op_out[0], names[0]);
      strcpy (op_out[1], names[1]);
      strcpy (op_out[2], names[3]);
      two_source_ops = 1;
    }
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
}

static void
OP_Mwait (int bytemode ATTRIBUTE_UNUSED,
	  int sizeflag ATTRIBUTE_UNUSED)
{
  /* mwait %eax,%ecx  */
  if (!intel_syntax)
    {
      const char **names = (address_mode == mode_64bit
			    ? names64 : names32);
      strcpy (op_out[0], names[0]);
      strcpy (op_out[1], names[1]);
      two_source_ops = 1;
    }
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
}

static void
OP_Monitor (int bytemode ATTRIBUTE_UNUSED,
	    int sizeflag ATTRIBUTE_UNUSED)
{
  /* monitor %eax,%ecx,%edx"  */
  if (!intel_syntax)
    {
      const char **op1_names;
      const char **names = (address_mode == mode_64bit
			    ? names64 : names32);

      if (!(prefixes & PREFIX_ADDR))
	op1_names = (address_mode == mode_16bit
		     ? names16 : names);
      else
	{
	  /* Remove "addr16/addr32".  */
	  all_prefixes[last_addr_prefix] = 0;
	  op1_names = (address_mode != mode_32bit
		       ? names32 : names16);
	  used_prefixes |= PREFIX_ADDR;
	}
      strcpy (op_out[0], op1_names[0]);
      strcpy (op_out[1], names[1]);
      strcpy (op_out[2], names[2]);
      two_source_ops = 1;
    }
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
}

static void
BadOp (void)
{
  /* Throw away prefixes and 1st. opcode byte.  */
  codep = insn_codep + 1;
  oappend ("(bad)");
}

static void
REP_Fixup (int bytemode, int sizeflag)
{
  /* The 0xf3 prefix should be displayed as "rep" for ins, outs, movs,
     lods and stos.  */
  if (prefixes & PREFIX_REPZ)
    all_prefixes[last_repz_prefix] = REP_PREFIX;

  switch (bytemode)
    {
    case al_reg:
    case eAX_reg:
    case indir_dx_reg:
      OP_IMREG (bytemode, sizeflag);
      break;
    case eDI_reg:
      OP_ESreg (bytemode, sizeflag);
      break;
    case eSI_reg:
      OP_DSreg (bytemode, sizeflag);
      break;
    default:
      abort ();
      break;
    }
}

/* For BND-prefixed instructions 0xF2 prefix should be displayed as
   "bnd".  */

static void
BND_Fixup (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  if (prefixes & PREFIX_REPNZ)
    all_prefixes[last_repnz_prefix] = BND_PREFIX;
}

/* Similar to OP_E.  But the 0xf2/0xf3 prefixes should be displayed as
   "xacquire"/"xrelease" for memory operand if there is a LOCK prefix.
 */

static void
HLE_Fixup1 (int bytemode, int sizeflag)
{
  if (modrm.mod != 3
      && (prefixes & PREFIX_LOCK) != 0)
    {
      if (prefixes & PREFIX_REPZ)
	all_prefixes[last_repz_prefix] = XRELEASE_PREFIX;
      if (prefixes & PREFIX_REPNZ)
	all_prefixes[last_repnz_prefix] = XACQUIRE_PREFIX;
    }

  OP_E (bytemode, sizeflag);
}

/* Similar to OP_E.  But the 0xf2/0xf3 prefixes should be displayed as
   "xacquire"/"xrelease" for memory operand.  No check for LOCK prefix.
 */

static void
HLE_Fixup2 (int bytemode, int sizeflag)
{
  if (modrm.mod != 3)
    {
      if (prefixes & PREFIX_REPZ)
	all_prefixes[last_repz_prefix] = XRELEASE_PREFIX;
      if (prefixes & PREFIX_REPNZ)
	all_prefixes[last_repnz_prefix] = XACQUIRE_PREFIX;
    }

  OP_E (bytemode, sizeflag);
}

/* Similar to OP_E.  But the 0xf3 prefixes should be displayed as
   "xrelease" for memory operand.  No check for LOCK prefix.   */

static void
HLE_Fixup3 (int bytemode, int sizeflag)
{
  if (modrm.mod != 3
      && last_repz_prefix > last_repnz_prefix
      && (prefixes & PREFIX_REPZ) != 0)
    all_prefixes[last_repz_prefix] = XRELEASE_PREFIX;

  OP_E (bytemode, sizeflag);
}

static void
CMPXCHG8B_Fixup (int bytemode, int sizeflag)
{
  USED_REX (REX_W);
  if (rex & REX_W)
    {
      /* Change cmpxchg8b to cmpxchg16b.  */
      char *p = mnemonicendp - 2;
      mnemonicendp = stpcpy (p, "16b");
      bytemode = o_mode;
    }
  else if ((prefixes & PREFIX_LOCK) != 0)
    {
      if (prefixes & PREFIX_REPZ)
	all_prefixes[last_repz_prefix] = XRELEASE_PREFIX;
      if (prefixes & PREFIX_REPNZ)
	all_prefixes[last_repnz_prefix] = XACQUIRE_PREFIX;
    }

  OP_M (bytemode, sizeflag);
}

static void
XMM_Fixup (int reg, int sizeflag ATTRIBUTE_UNUSED)
{
  const char **names;

  if (need_vex)
    {
      switch (vex.length)
	{
	case 128:
	  names = names_xmm;
	  break;
	case 256:
	  names = names_ymm;
	  break;
	default:
	  abort ();
	}
    }
  else
    names = names_xmm;
  oappend (names[reg]);
}

static void
CRC32_Fixup (int bytemode, int sizeflag)
{
  /* Add proper suffix to "crc32".  */
  char *p = mnemonicendp;

  switch (bytemode)
    {
    case b_mode:
      if (intel_syntax)
	goto skip;

      *p++ = 'b';
      break;
    case v_mode:
      if (intel_syntax)
	goto skip;

      USED_REX (REX_W);
      if (rex & REX_W)
	*p++ = 'q';
      else
	{
	  if (sizeflag & DFLAG)
	    *p++ = 'l';
	  else
	    *p++ = 'w';
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
  mnemonicendp = p;
  *p = '\0';

skip:
  if (modrm.mod == 3)
    {
      int add;

      /* Skip mod/rm byte.  */
      MODRM_CHECK;
      codep++;

      USED_REX (REX_B);
      add = (rex & REX_B) ? 8 : 0;
      if (bytemode == b_mode)
	{
	  USED_REX (0);
	  if (rex)
	    oappend (names8rex[modrm.rm + add]);
	  else
	    oappend (names8[modrm.rm + add]);
	}
      else
	{
	  USED_REX (REX_W);
	  if (rex & REX_W)
	    oappend (names64[modrm.rm + add]);
	  else if ((prefixes & PREFIX_DATA))
	    oappend (names16[modrm.rm + add]);
	  else
	    oappend (names32[modrm.rm + add]);
	}
    }
  else
    OP_E (bytemode, sizeflag);
}

static void
FXSAVE_Fixup (int bytemode, int sizeflag)
{
  /* Add proper suffix to "fxsave" and "fxrstor".  */
  USED_REX (REX_W);
  if (rex & REX_W)
    {
      char *p = mnemonicendp;
      *p++ = '6';
      *p++ = '4';
      *p = '\0';
      mnemonicendp = p;
    }
  OP_M (bytemode, sizeflag);
}

/* Display the destination register operand for instructions with
   VEX. */

static void
OP_VEX (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  int reg;
  const char **names;

  if (!need_vex)
    abort ();

  if (!need_vex_reg)
    return;

  reg = vex.register_specifier;
  if (vex.evex)
    {
      if (!vex.v)
	reg += 16;
    }

  if (bytemode == vex_scalar_mode)
    {
      oappend (names_xmm[reg]);
      return;
    }

  switch (vex.length)
    {
    case 128:
      switch (bytemode)
	{
	case vex_mode:
	case vex128_mode:
	case vex_vsib_q_w_dq_mode:
	case vex_vsib_q_w_d_mode:
	  names = names_xmm;
	  break;
	case dq_mode:
	  if (vex.w)
	    names = names64;
	  else
	    names = names32;
	  break;
	case mask_bd_mode:
	case mask_mode:
	  names = names_mask;
	  break;
	default:
	  abort ();
	  return;
	}
      break;
    case 256:
      switch (bytemode)
	{
	case vex_mode:
	case vex256_mode:
	  names = names_ymm;
	  break;
	case vex_vsib_q_w_dq_mode:
	case vex_vsib_q_w_d_mode:
	  names = vex.w ? names_ymm : names_xmm;
	  break;
	case mask_bd_mode:
	case mask_mode:
	  names = names_mask;
	  break;
	default:
	  abort ();
	  return;
	}
      break;
    case 512:
      names = names_zmm;
      break;
    default:
      abort ();
      break;
    }
  oappend (names[reg]);
}

/* Get the VEX immediate byte without moving codep.  */

static unsigned char
get_vex_imm8 (int sizeflag, int opnum)
{
  int bytes_before_imm = 0;

  if (modrm.mod != 3)
    {
      /* There are SIB/displacement bytes.  */
      if ((sizeflag & AFLAG) || address_mode == mode_64bit)
	{
	  /* 32/64 bit address mode */
	  int base = modrm.rm;

	  /* Check SIB byte.  */
	  if (base == 4)
	    {
	      FETCH_DATA (the_info, codep + 1);
	      base = *codep & 7;
	      /* When decoding the third source, don't increase
		 bytes_before_imm as this has already been incremented
		 by one in OP_E_memory while decoding the second
		 source operand.  */
	      if (opnum == 0)
		bytes_before_imm++;
	    }

	  /* Don't increase bytes_before_imm when decoding the third source,
	     it has already been incremented by OP_E_memory while decoding
	     the second source operand.  */
	  if (opnum == 0)
	    {
	      switch (modrm.mod)
		{
		  case 0:
		    /* When modrm.rm == 5 or modrm.rm == 4 and base in
		       SIB == 5, there is a 4 byte displacement.  */
		    if (base != 5)
		      /* No displacement. */
		      break;
		  case 2:
		    /* 4 byte displacement.  */
		    bytes_before_imm += 4;
		    break;
		  case 1:
		    /* 1 byte displacement.  */
		    bytes_before_imm++;
		    break;
		}
	    }
	}
      else
	{
	  /* 16 bit address mode */
	  /* Don't increase bytes_before_imm when decoding the third source,
	     it has already been incremented by OP_E_memory while decoding
	     the second source operand.  */
	  if (opnum == 0)
	    {
	      switch (modrm.mod)
		{
		case 0:
		  /* When modrm.rm == 6, there is a 2 byte displacement.  */
		  if (modrm.rm != 6)
		    /* No displacement. */
		    break;
		case 2:
		  /* 2 byte displacement.  */
		  bytes_before_imm += 2;
		  break;
		case 1:
		  /* 1 byte displacement: when decoding the third source,
		     don't increase bytes_before_imm as this has already
		     been incremented by one in OP_E_memory while decoding
		     the second source operand.  */
		  if (opnum == 0)
		    bytes_before_imm++;

		  break;
		}
	    }
	}
    }

  FETCH_DATA (the_info, codep + bytes_before_imm + 1);
  return codep [bytes_before_imm];
}

static void
OP_EX_VexReg (int bytemode, int sizeflag, int reg)
{
  const char **names;

  if (reg == -1 && modrm.mod != 3)
    {
      OP_E_memory (bytemode, sizeflag);
      return;
    }
  else
    {
      if (reg == -1)
	{
	  reg = modrm.rm;
	  USED_REX (REX_B);
	  if (rex & REX_B)
	    reg += 8;
	}
      else if (reg > 7 && address_mode != mode_64bit)
	BadOp ();
    }

  switch (vex.length)
    {
    case 128:
      names = names_xmm;
      break;
    case 256:
      names = names_ymm;
      break;
    default:
      abort ();
    }
  oappend (names[reg]);
}

static void
OP_EX_VexImmW (int bytemode, int sizeflag)
{
  int reg = -1;
  static unsigned char vex_imm8;

  if (vex_w_done == 0)
    {
      vex_w_done = 1;

      /* Skip mod/rm byte.  */
      MODRM_CHECK;
      codep++;

      vex_imm8 = get_vex_imm8 (sizeflag, 0);

      if (vex.w)
	  reg = vex_imm8 >> 4;

      OP_EX_VexReg (bytemode, sizeflag, reg);
    }
  else if (vex_w_done == 1)
    {
      vex_w_done = 2;

      if (!vex.w)
	  reg = vex_imm8 >> 4;

      OP_EX_VexReg (bytemode, sizeflag, reg);
    }
  else
    {
      /* Output the imm8 directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, vex_imm8 & 0xf);
      oappend_maybe_intel (scratchbuf);
      scratchbuf[0] = '\0';
      codep++;
    }
}

static void
OP_Vex_2src (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    {
      int reg = modrm.rm;
      USED_REX (REX_B);
      if (rex & REX_B)
	reg += 8;
      oappend (names_xmm[reg]);
    }
  else
    {
      if (intel_syntax
	  && (bytemode == v_mode || bytemode == v_swap_mode))
	{
	  bytemode = (prefixes & PREFIX_DATA) ? x_mode : q_mode;
	  used_prefixes |= (prefixes & PREFIX_DATA);
	}
      OP_E (bytemode, sizeflag);
    }
}

static void
OP_Vex_2src_1 (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    {
      /* Skip mod/rm byte.   */
      MODRM_CHECK;
      codep++;
    }

  if (vex.w)
    oappend (names_xmm[vex.register_specifier]);
  else
    OP_Vex_2src (bytemode, sizeflag);
}

static void
OP_Vex_2src_2 (int bytemode, int sizeflag)
{
  if (vex.w)
    OP_Vex_2src (bytemode, sizeflag);
  else
    oappend (names_xmm[vex.register_specifier]);
}

static void
OP_EX_VexW (int bytemode, int sizeflag)
{
  int reg = -1;

  if (!vex_w_done)
    {
      vex_w_done = 1;

      /* Skip mod/rm byte.  */
      MODRM_CHECK;
      codep++;

      if (vex.w)
	reg = get_vex_imm8 (sizeflag, 0) >> 4;
    }
  else
    {
      if (!vex.w)
	reg = get_vex_imm8 (sizeflag, 1) >> 4;
    }

  OP_EX_VexReg (bytemode, sizeflag, reg);
}

static void
VEXI4_Fixup (int bytemode ATTRIBUTE_UNUSED,
	     int sizeflag ATTRIBUTE_UNUSED)
{
  /* Skip the immediate byte and check for invalid bits.  */
  FETCH_DATA (the_info, codep + 1);
  if (*codep++ & 0xf)
    BadOp ();
}

static void
OP_REG_VexI4 (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  int reg;
  const char **names;

  FETCH_DATA (the_info, codep + 1);
  reg = *codep++;

  if (bytemode != x_mode)
    abort ();

  if (reg & 0xf)
      BadOp ();

  reg >>= 4;
  if (reg > 7 && address_mode != mode_64bit)
    BadOp ();

  switch (vex.length)
    {
    case 128:
      names = names_xmm;
      break;
    case 256:
      names = names_ymm;
      break;
    default:
      abort ();
    }
  oappend (names[reg]);
}

static void
OP_XMM_VexW (int bytemode, int sizeflag)
{
  /* Turn off the REX.W bit since it is used for swapping operands
     now.  */
  rex &= ~REX_W;
  OP_XMM (bytemode, sizeflag);
}

static void
OP_EX_Vex (int bytemode, int sizeflag)
{
  if (modrm.mod != 3)
    {
      if (vex.register_specifier != 0)
	BadOp ();
      need_vex_reg = 0;
    }
  OP_EX (bytemode, sizeflag);
}

static void
OP_XMM_Vex (int bytemode, int sizeflag)
{
  if (modrm.mod != 3)
    {
      if (vex.register_specifier != 0)
	BadOp ();
      need_vex_reg = 0;
    }
  OP_XMM (bytemode, sizeflag);
}

static void
VZERO_Fixup (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  switch (vex.length)
    {
    case 128:
      mnemonicendp = stpcpy (obuf, "vzeroupper");
      break;
    case 256:
      mnemonicendp = stpcpy (obuf, "vzeroall");
      break;
    default:
      abort ();
    }
}

static struct op vex_cmp_op[] =
{
  { STRING_COMMA_LEN ("eq") },
  { STRING_COMMA_LEN ("lt") },
  { STRING_COMMA_LEN ("le") },
  { STRING_COMMA_LEN ("unord") },
  { STRING_COMMA_LEN ("neq") },
  { STRING_COMMA_LEN ("nlt") },
  { STRING_COMMA_LEN ("nle") },
  { STRING_COMMA_LEN ("ord") },
  { STRING_COMMA_LEN ("eq_uq") },
  { STRING_COMMA_LEN ("nge") },
  { STRING_COMMA_LEN ("ngt") },
  { STRING_COMMA_LEN ("false") },
  { STRING_COMMA_LEN ("neq_oq") },
  { STRING_COMMA_LEN ("ge") },
  { STRING_COMMA_LEN ("gt") },
  { STRING_COMMA_LEN ("true") },
  { STRING_COMMA_LEN ("eq_os") },
  { STRING_COMMA_LEN ("lt_oq") },
  { STRING_COMMA_LEN ("le_oq") },
  { STRING_COMMA_LEN ("unord_s") },
  { STRING_COMMA_LEN ("neq_us") },
  { STRING_COMMA_LEN ("nlt_uq") },
  { STRING_COMMA_LEN ("nle_uq") },
  { STRING_COMMA_LEN ("ord_s") },
  { STRING_COMMA_LEN ("eq_us") },
  { STRING_COMMA_LEN ("nge_uq") },
  { STRING_COMMA_LEN ("ngt_uq") },
  { STRING_COMMA_LEN ("false_os") },
  { STRING_COMMA_LEN ("neq_os") },
  { STRING_COMMA_LEN ("ge_oq") },
  { STRING_COMMA_LEN ("gt_oq") },
  { STRING_COMMA_LEN ("true_us") },
};

static void
VCMP_Fixup (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned int cmp_type;

  FETCH_DATA (the_info, codep + 1);
  cmp_type = *codep++ & 0xff;
  if (cmp_type < ARRAY_SIZE (vex_cmp_op))
    {
      char suffix [3];
      char *p = mnemonicendp - 2;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = '\0';
      sprintf (p, "%s%s", vex_cmp_op[cmp_type].name, suffix);
      mnemonicendp += vex_cmp_op[cmp_type].len;
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, cmp_type);
      oappend_maybe_intel (scratchbuf);
      scratchbuf[0] = '\0';
    }
}

static void
VPCMP_Fixup (int bytemode ATTRIBUTE_UNUSED,
	     int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned int cmp_type;

  if (!vex.evex)
    abort ();

  FETCH_DATA (the_info, codep + 1);
  cmp_type = *codep++ & 0xff;
  /* There are aliases for immediates 0, 1, 2, 4, 5, 6.
     If it's the case, print suffix, otherwise - print the immediate.  */
  if (cmp_type < ARRAY_SIZE (simd_cmp_op)
      && cmp_type != 3
      && cmp_type != 7)
    {
      char suffix [3];
      char *p = mnemonicendp - 2;

      /* vpcmp* can have both one- and two-lettered suffix.  */
      if (p[0] == 'p')
	{
	  p++;
	  suffix[0] = p[0];
	  suffix[1] = '\0';
	}
      else
	{
	  suffix[0] = p[0];
	  suffix[1] = p[1];
	  suffix[2] = '\0';
	}

      sprintf (p, "%s%s", simd_cmp_op[cmp_type].name, suffix);
      mnemonicendp += simd_cmp_op[cmp_type].len;
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, cmp_type);
      oappend_maybe_intel (scratchbuf);
      scratchbuf[0] = '\0';
    }
}

static const struct op pclmul_op[] =
{
  { STRING_COMMA_LEN ("lql") },
  { STRING_COMMA_LEN ("hql") },
  { STRING_COMMA_LEN ("lqh") },
  { STRING_COMMA_LEN ("hqh") }
};

static void
PCLMUL_Fixup (int bytemode ATTRIBUTE_UNUSED,
	      int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned int pclmul_type;

  FETCH_DATA (the_info, codep + 1);
  pclmul_type = *codep++ & 0xff;
  switch (pclmul_type)
    {
    case 0x10:
      pclmul_type = 2;
      break;
    case 0x11:
      pclmul_type = 3;
      break;
    default:
      break;
    }
  if (pclmul_type < ARRAY_SIZE (pclmul_op))
    {
      char suffix [4];
      char *p = mnemonicendp - 3;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = p[2];
      suffix[3] = '\0';
      sprintf (p, "%s%s", pclmul_op[pclmul_type].name, suffix);
      mnemonicendp += pclmul_op[pclmul_type].len;
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, pclmul_type);
      oappend_maybe_intel (scratchbuf);
      scratchbuf[0] = '\0';
    }
}

static void
MOVBE_Fixup (int bytemode, int sizeflag)
{
  /* Add proper suffix to "movbe".  */
  char *p = mnemonicendp;

  switch (bytemode)
    {
    case v_mode:
      if (intel_syntax)
	goto skip;

      USED_REX (REX_W);
      if (sizeflag & SUFFIX_ALWAYS)
	{
	  if (rex & REX_W)
	    *p++ = 'q';
	  else
	    {
	      if (sizeflag & DFLAG)
		*p++ = 'l';
	      else
		*p++ = 'w';
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	}
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
  mnemonicendp = p;
  *p = '\0';

skip:
  OP_M (bytemode, sizeflag);
}

static void
OP_LWPCB_E (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  int reg;
  const char **names;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  if (vex.w)
    names = names64;
  else
    names = names32;

  reg = modrm.rm;
  USED_REX (REX_B);
  if (rex & REX_B)
    reg += 8;

  oappend (names[reg]);
}

static void
OP_LWP_E (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  const char **names;

  if (vex.w)
    names = names64;
  else
    names = names32;

  oappend (names[vex.register_specifier]);
}

static void
OP_Mask (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  if (!vex.evex
      || (bytemode != mask_mode && bytemode != mask_bd_mode))
    abort ();

  USED_REX (REX_R);
  if ((rex & REX_R) != 0 || !vex.r)
    {
      BadOp ();
      return;
    }

  oappend (names_mask [modrm.reg]);
}

static void
OP_Rounding (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  if (!vex.evex
      || (bytemode != evex_rounding_mode
	  && bytemode != evex_sae_mode))
    abort ();
  if (modrm.mod == 3 && vex.b)
    switch (bytemode)
      {
      case evex_rounding_mode:
	oappend (names_rounding[vex.ll]);
	break;
      case evex_sae_mode:
	oappend ("{sae}");
	break;
      default:
	break;
      }
}
