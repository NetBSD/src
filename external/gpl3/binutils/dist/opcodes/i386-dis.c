/* Print i386 instructions for GDB, the GNU debugger.
   Copyright 1988, 1989, 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.

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

static int fetch_data (struct disassemble_info *, bfd_byte *);
static void ckprefix (void);
static const char *prefix_name (int, int);
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
static void OP_E_memory (int, int, int);
static void OP_E_extended (int, int, int);
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
static void OP_VEX_FMA (int, int);
static void OP_EX_Vex (int, int);
static void OP_EX_VexW (int, int);
static void OP_EX_VexImmW (int, int);
static void OP_XMM_Vex (int, int);
static void OP_XMM_VexW (int, int);
static void OP_REG_VexI4 (int, int);
static void PCLMUL_Fixup (int, int);
static void VEXI4_Fixup (int, int);
static void VZERO_Fixup (int, int);
static void VCMP_Fixup (int, int);
static void VPERMIL2_Fixup (int, int);
static void OP_0f07 (int, int);
static void OP_Monitor (int, int);
static void OP_Mwait (int, int);
static void NOP_Fixup1 (int, int);
static void NOP_Fixup2 (int, int);
static void OP_3DNowSuffix (int, int);
static void CMP_Fixup (int, int);
static void BadOp (void);
static void REP_Fixup (int, int);
static void CMPXCHG8B_Fixup (int, int);
static void XMM_Fixup (int, int);
static void CRC32_Fixup (int, int);
static void print_drex_arg (unsigned int, int, int);
static void OP_DREX4 (int, int);
static void OP_DREX3 (int, int);
static void OP_DREX_ICMP (int, int);
static void OP_DREX_FCMP (int, int);
static void MOVBE_Fixup (int, int);

struct dis_private {
  /* Points to first byte not fetched.  */
  bfd_byte *max_fetched;
  bfd_byte the_buffer[MAX_MNEM_SIZE];
  bfd_vma insn_start;
  int orig_sizeflag;
  jmp_buf bailout;
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
/* Original REX prefix.  */
static int rex_original;
/* REX bits in original REX prefix ignored.  It may not be the same
   as rex_original since some bits may not be ignored.  */
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

/* Special 'registers' for DREX handling */
#define DREX_REG_UNKNOWN	1000	/* not initialized */
#define DREX_REG_MEMORY         1001	/* use MODRM/SIB/OFFSET memory */

/* The DREX byte has the following fields:
   Bits 7-4 -- DREX.Dest, xmm destination register
   Bit 3    -- DREX.OC0, operand config bit defines operand order
   Bit 2    -- DREX.R, equivalent to REX_R bit, to extend ModRM register
   Bit 1    -- DREX.X, equivalent to REX_X bit, to extend SIB index field
   Bit 0    -- DREX.W, equivalent to REX_B bit, to extend ModRM r/m field,
	       SIB base field, or opcode reg field.  */
#define DREX_XMM(drex) ((drex >> 4) & 0xf)
#define DREX_OC0(drex) ((drex >> 3) & 0x1)

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
      longjmp (priv->bailout, 1);
    }
  else
    priv->max_fetched = addr;
  return 1;
}

#define XX { NULL, 0 }

#define Eb { OP_E, b_mode }
#define Ev { OP_E, v_mode }
#define Ed { OP_E, d_mode }
#define Edq { OP_E, dq_mode }
#define Edqw { OP_E, dqw_mode }
#define Edqb { OP_E, dqb_mode }
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
#define Gv { OP_G, v_mode }
#define Gd { OP_G, d_mode }
#define Gdq { OP_G, dq_mode }
#define Gm { OP_G, m_mode }
#define Gw { OP_G, w_mode }
#define Rd { OP_R, d_mode }
#define Rm { OP_R, m_mode }
#define Ib { OP_I, b_mode }
#define sIb { OP_sI, b_mode }	/* sign extened byte */
#define Iv { OP_I, v_mode }
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
#define XMM { OP_XMM, xmm_mode }
#define EM { OP_EM, v_mode }
#define EMd { OP_EM, d_mode }
#define EMx { OP_EM, x_mode }
#define EXw { OP_EX, w_mode }
#define EXd { OP_EX, d_mode }
#define EXq { OP_EX, q_mode }
#define EXx { OP_EX, x_mode }
#define EXxmm { OP_EX, xmm_mode }
#define EXxmmq { OP_EX, xmmq_mode }
#define EXymmq { OP_EX, ymmq_mode }
#define MS { OP_MS, v_mode }
#define XS { OP_XS, v_mode }
#define EMCq { OP_EMC, q_mode }
#define MXC { OP_MXC, 0 }
#define OPSUF { OP_3DNowSuffix, 0 }
#define CMP { CMP_Fixup, 0 }
#define XMM0 { XMM_Fixup, 0 }

#define Vex { OP_VEX, vex_mode }
#define Vex128 { OP_VEX, vex128_mode }
#define Vex256 { OP_VEX, vex256_mode }
#define VexI4 { VEXI4_Fixup, 0}
#define VexFMA { OP_VEX_FMA, vex_mode }
#define Vex128FMA { OP_VEX_FMA, vex128_mode }
#define EXdVex { OP_EX_Vex, d_mode }
#define EXqVex { OP_EX_Vex, q_mode }
#define EXVexW { OP_EX_VexW, x_mode }
#define EXdVexW { OP_EX_VexW, d_mode }
#define EXqVexW { OP_EX_VexW, q_mode }
#define EXVexImmW { OP_EX_VexImmW, x_mode }
#define XMVex { OP_XMM_Vex, 0 }
#define XMVexW { OP_XMM_VexW, 0 }
#define XMVexI4 { OP_REG_VexI4, x_mode }
#define PCLMUL { PCLMUL_Fixup, 0 }
#define VZERO { VZERO_Fixup, 0 }
#define VCMP { VCMP_Fixup, 0 }
#define VPERMIL2 { VPERMIL2_Fixup, 0 }

/* Used handle "rep" prefix for string instructions.  */
#define Xbr { REP_Fixup, eSI_reg }
#define Xvr { REP_Fixup, eSI_reg }
#define Ybr { REP_Fixup, eDI_reg }
#define Yvr { REP_Fixup, eDI_reg }
#define Yzr { REP_Fixup, eDI_reg }
#define indirDXr { REP_Fixup, indir_dx_reg }
#define ALr { REP_Fixup, al_reg }
#define eAXr { REP_Fixup, eAX_reg }

#define cond_jump_flag { NULL, cond_jump_mode }
#define loop_jcxz_flag { NULL, loop_jcxz_mode }

/* bits in sizeflag */
#define SUFFIX_ALWAYS 4
#define AFLAG 2
#define DFLAG 1

/* byte operand */
#define b_mode			1
/* operand size depends on prefixes */
#define v_mode			(b_mode + 1)
/* word operand */
#define w_mode			(v_mode + 1)
/* double word operand  */
#define d_mode			(w_mode + 1)
/* quad word operand */
#define q_mode			(d_mode + 1)
/* ten-byte operand */
#define t_mode			(q_mode + 1)
/* 16-byte XMM or 32-byte YMM operand */
#define x_mode			(t_mode + 1)
/* 16-byte XMM operand */
#define xmm_mode		(x_mode + 1)
/* 16-byte XMM or quad word operand */
#define xmmq_mode		(xmm_mode + 1)
/* 32-byte YMM or quad word operand */
#define ymmq_mode		(xmmq_mode + 1)
/* d_mode in 32bit, q_mode in 64bit mode.  */
#define m_mode			(ymmq_mode + 1)
/* pair of v_mode operands */
#define a_mode			(m_mode + 1)
#define cond_jump_mode		(a_mode + 1)
#define loop_jcxz_mode		(cond_jump_mode + 1)
/* operand size depends on REX prefixes.  */
#define dq_mode			(loop_jcxz_mode + 1)
/* registers like dq_mode, memory like w_mode.  */
#define dqw_mode		(dq_mode + 1)
/* 4- or 6-byte pointer operand */
#define f_mode			(dqw_mode + 1)
#define const_1_mode		(f_mode + 1)
/* v_mode for stack-related opcodes.  */
#define stack_v_mode		(const_1_mode + 1)
/* non-quad operand size depends on prefixes */
#define z_mode			(stack_v_mode + 1)
/* 16-byte operand */
#define o_mode			(z_mode + 1)
/* registers like dq_mode, memory like b_mode.  */
#define dqb_mode		(o_mode + 1)
/* registers like dq_mode, memory like d_mode.  */
#define dqd_mode		(dqb_mode + 1)
/* normal vex mode */
#define vex_mode		(dqd_mode + 1)
/* 128bit vex mode */
#define vex128_mode		(vex_mode + 1)
/* 256bit vex mode */
#define vex256_mode		(vex128_mode + 1)

#define es_reg			(vex256_mode + 1)
#define cs_reg			(es_reg + 1)
#define ss_reg			(cs_reg + 1)
#define ds_reg			(ss_reg + 1)
#define fs_reg			(ds_reg + 1)
#define gs_reg			(fs_reg + 1)

#define eAX_reg			(gs_reg + 1)
#define eCX_reg			(eAX_reg + 1)
#define eDX_reg			(eCX_reg + 1)
#define eBX_reg			(eDX_reg + 1)
#define eSP_reg			(eBX_reg + 1)
#define eBP_reg			(eSP_reg + 1)
#define eSI_reg			(eBP_reg + 1)
#define eDI_reg			(eSI_reg + 1)

#define al_reg			(eDI_reg + 1)
#define cl_reg			(al_reg + 1)
#define dl_reg			(cl_reg + 1)
#define bl_reg			(dl_reg + 1)
#define ah_reg			(bl_reg + 1)
#define ch_reg			(ah_reg + 1)
#define dh_reg			(ch_reg + 1)
#define bh_reg			(dh_reg + 1)

#define ax_reg			(bh_reg + 1)
#define cx_reg			(ax_reg + 1)
#define dx_reg			(cx_reg + 1)
#define bx_reg			(dx_reg + 1)
#define sp_reg			(bx_reg + 1)
#define bp_reg			(sp_reg + 1)
#define si_reg			(bp_reg + 1)
#define di_reg			(si_reg + 1)

#define rAX_reg			(di_reg + 1)
#define rCX_reg			(rAX_reg + 1)
#define rDX_reg			(rCX_reg + 1)
#define rBX_reg			(rDX_reg + 1)
#define rSP_reg			(rBX_reg + 1)
#define rBP_reg			(rSP_reg + 1)
#define rSI_reg			(rBP_reg + 1)
#define rDI_reg			(rSI_reg + 1)

#define z_mode_ax_reg		(rDI_reg + 1)
#define indir_dx_reg		(z_mode_ax_reg + 1)

#define MAX_BYTEMODE	indir_dx_reg

/* Flags that are OR'ed into the bytemode field to pass extra
   information.  */
#define DREX_OC1		0x10000	/* OC1 bit set */
#define DREX_NO_OC0		0x20000	/* OC0 bit not used */
#define DREX_MASK		0x40000	/* mask to delete */

#if MAX_BYTEMODE >= DREX_OC1
#error MAX_BYTEMODE must be less than DREX_OC1
#endif

#define FLOATCODE		1
#define USE_REG_TABLE		(FLOATCODE + 1)
#define USE_MOD_TABLE		(USE_REG_TABLE + 1)
#define USE_RM_TABLE		(USE_MOD_TABLE + 1)
#define USE_PREFIX_TABLE	(USE_RM_TABLE + 1)
#define USE_X86_64_TABLE	(USE_PREFIX_TABLE + 1)
#define USE_3BYTE_TABLE		(USE_X86_64_TABLE + 1)
#define USE_VEX_C4_TABLE	(USE_3BYTE_TABLE + 1)
#define USE_VEX_C5_TABLE	(USE_VEX_C4_TABLE + 1)
#define USE_VEX_LEN_TABLE	(USE_VEX_C5_TABLE + 1)

#define FLOAT			NULL, { { NULL, FLOATCODE } }

#define DIS386(T, I)		NULL, { { NULL, (T)}, { NULL,  (I) } }
#define REG_TABLE(I)		DIS386 (USE_REG_TABLE, (I))
#define MOD_TABLE(I)		DIS386 (USE_MOD_TABLE, (I))
#define RM_TABLE(I)		DIS386 (USE_RM_TABLE, (I))
#define PREFIX_TABLE(I)		DIS386 (USE_PREFIX_TABLE, (I))
#define X86_64_TABLE(I)		DIS386 (USE_X86_64_TABLE, (I))
#define THREE_BYTE_TABLE(I)	DIS386 (USE_3BYTE_TABLE, (I))
#define VEX_C4_TABLE(I)		DIS386 (USE_VEX_C4_TABLE, (I))
#define VEX_C5_TABLE(I)		DIS386 (USE_VEX_C5_TABLE, (I))
#define VEX_LEN_TABLE(I)	DIS386 (USE_VEX_LEN_TABLE, (I))

#define REG_80			0
#define REG_81			(REG_80 + 1)
#define REG_82			(REG_81 + 1)
#define REG_8F			(REG_82 + 1)
#define REG_C0			(REG_8F + 1)
#define REG_C1			(REG_C0 + 1)
#define REG_C6			(REG_C1 + 1)
#define REG_C7			(REG_C6 + 1)
#define REG_D0			(REG_C7 + 1)
#define REG_D1			(REG_D0 + 1)
#define REG_D2			(REG_D1 + 1)
#define REG_D3			(REG_D2 + 1)
#define REG_F6			(REG_D3 + 1)
#define REG_F7			(REG_F6 + 1)
#define REG_FE			(REG_F7 + 1)
#define REG_FF			(REG_FE + 1)
#define REG_0F00		(REG_FF + 1)
#define REG_0F01		(REG_0F00 + 1)
#define REG_0F0D		(REG_0F01 + 1)
#define REG_0F18		(REG_0F0D + 1)
#define REG_0F71		(REG_0F18 + 1)
#define REG_0F72		(REG_0F71 + 1)
#define REG_0F73		(REG_0F72 + 1)
#define REG_0FA6		(REG_0F73 + 1)
#define REG_0FA7		(REG_0FA6 + 1)
#define REG_0FAE		(REG_0FA7 + 1)
#define REG_0FBA		(REG_0FAE + 1)
#define REG_0FC7		(REG_0FBA + 1)
#define REG_VEX_71		(REG_0FC7 + 1)
#define REG_VEX_72		(REG_VEX_71 + 1)
#define REG_VEX_73		(REG_VEX_72 + 1)
#define REG_VEX_AE		(REG_VEX_73 + 1)

#define MOD_8D			0
#define MOD_0F01_REG_0		(MOD_8D + 1)
#define MOD_0F01_REG_1		(MOD_0F01_REG_0 + 1)
#define MOD_0F01_REG_2		(MOD_0F01_REG_1 + 1)
#define MOD_0F01_REG_3		(MOD_0F01_REG_2 + 1)
#define MOD_0F01_REG_7		(MOD_0F01_REG_3 + 1)
#define MOD_0F12_PREFIX_0	(MOD_0F01_REG_7 + 1)
#define MOD_0F13		(MOD_0F12_PREFIX_0 + 1)
#define MOD_0F16_PREFIX_0	(MOD_0F13 + 1)
#define MOD_0F17		(MOD_0F16_PREFIX_0 + 1)
#define MOD_0F18_REG_0		(MOD_0F17 + 1)
#define MOD_0F18_REG_1		(MOD_0F18_REG_0 + 1)
#define MOD_0F18_REG_2		(MOD_0F18_REG_1 + 1)
#define MOD_0F18_REG_3		(MOD_0F18_REG_2 + 1)
#define MOD_0F20		(MOD_0F18_REG_3 + 1)
#define MOD_0F21		(MOD_0F20 + 1)
#define MOD_0F22		(MOD_0F21 + 1)
#define MOD_0F23		(MOD_0F22 + 1)
#define MOD_0F24		(MOD_0F23 + 1)
#define MOD_0F26		(MOD_0F24 + 1)
#define MOD_0F2B_PREFIX_0	(MOD_0F26 + 1)
#define MOD_0F2B_PREFIX_1	(MOD_0F2B_PREFIX_0 + 1)
#define MOD_0F2B_PREFIX_2	(MOD_0F2B_PREFIX_1 + 1)
#define MOD_0F2B_PREFIX_3	(MOD_0F2B_PREFIX_2 + 1)
#define MOD_0F51		(MOD_0F2B_PREFIX_3 + 1)
#define MOD_0F71_REG_2		(MOD_0F51 + 1)
#define MOD_0F71_REG_4		(MOD_0F71_REG_2 + 1)
#define MOD_0F71_REG_6		(MOD_0F71_REG_4 + 1)
#define MOD_0F72_REG_2		(MOD_0F71_REG_6 + 1)
#define MOD_0F72_REG_4		(MOD_0F72_REG_2 + 1)
#define MOD_0F72_REG_6		(MOD_0F72_REG_4 + 1)
#define MOD_0F73_REG_2		(MOD_0F72_REG_6 + 1)
#define MOD_0F73_REG_3		(MOD_0F73_REG_2 + 1)
#define MOD_0F73_REG_6		(MOD_0F73_REG_3 + 1)
#define MOD_0F73_REG_7		(MOD_0F73_REG_6 + 1)
#define MOD_0FAE_REG_0		(MOD_0F73_REG_7 + 1)
#define MOD_0FAE_REG_1		(MOD_0FAE_REG_0 + 1)
#define MOD_0FAE_REG_2		(MOD_0FAE_REG_1 + 1)
#define MOD_0FAE_REG_3		(MOD_0FAE_REG_2 + 1)
#define MOD_0FAE_REG_4		(MOD_0FAE_REG_3 + 1)
#define MOD_0FAE_REG_5		(MOD_0FAE_REG_4 + 1)
#define MOD_0FAE_REG_6		(MOD_0FAE_REG_5 + 1)
#define MOD_0FAE_REG_7		(MOD_0FAE_REG_6 + 1)
#define MOD_0FB2		(MOD_0FAE_REG_7 + 1)
#define MOD_0FB4		(MOD_0FB2 + 1)
#define MOD_0FB5		(MOD_0FB4 + 1)
#define MOD_0FC7_REG_6		(MOD_0FB5 + 1)
#define MOD_0FC7_REG_7		(MOD_0FC7_REG_6 + 1)
#define MOD_0FD7		(MOD_0FC7_REG_7 + 1)
#define MOD_0FE7_PREFIX_2	(MOD_0FD7 + 1)
#define MOD_0FF0_PREFIX_3	(MOD_0FE7_PREFIX_2 + 1)
#define MOD_0F382A_PREFIX_2	(MOD_0FF0_PREFIX_3 + 1)
#define MOD_62_32BIT		(MOD_0F382A_PREFIX_2 + 1)
#define MOD_C4_32BIT		(MOD_62_32BIT + 1)
#define MOD_C5_32BIT		(MOD_C4_32BIT + 1)
#define MOD_VEX_12_PREFIX_0	(MOD_C5_32BIT + 1)
#define MOD_VEX_13		(MOD_VEX_12_PREFIX_0 + 1)
#define MOD_VEX_16_PREFIX_0	(MOD_VEX_13 + 1)
#define MOD_VEX_17		(MOD_VEX_16_PREFIX_0 + 1)
#define MOD_VEX_2B		(MOD_VEX_17 + 1)
#define MOD_VEX_51		(MOD_VEX_2B + 1)
#define MOD_VEX_71_REG_2	(MOD_VEX_51 + 1)
#define MOD_VEX_71_REG_4	(MOD_VEX_71_REG_2 + 1)
#define MOD_VEX_71_REG_6	(MOD_VEX_71_REG_4 + 1)
#define MOD_VEX_72_REG_2	(MOD_VEX_71_REG_6 + 1)
#define MOD_VEX_72_REG_4	(MOD_VEX_72_REG_2 + 1)
#define MOD_VEX_72_REG_6	(MOD_VEX_72_REG_4 + 1)
#define MOD_VEX_73_REG_2	(MOD_VEX_72_REG_6 + 1)
#define MOD_VEX_73_REG_3	(MOD_VEX_73_REG_2 + 1)
#define MOD_VEX_73_REG_6	(MOD_VEX_73_REG_3 + 1)
#define MOD_VEX_73_REG_7	(MOD_VEX_73_REG_6 + 1)
#define MOD_VEX_AE_REG_2	(MOD_VEX_73_REG_7 + 1)
#define MOD_VEX_AE_REG_3	(MOD_VEX_AE_REG_2 + 1)
#define MOD_VEX_D7_PREFIX_2	(MOD_VEX_AE_REG_3 + 1)
#define MOD_VEX_E7_PREFIX_2	(MOD_VEX_D7_PREFIX_2 + 1)
#define MOD_VEX_F0_PREFIX_3	(MOD_VEX_E7_PREFIX_2 + 1)
#define MOD_VEX_3818_PREFIX_2	(MOD_VEX_F0_PREFIX_3 + 1)
#define MOD_VEX_3819_PREFIX_2	(MOD_VEX_3818_PREFIX_2 + 1)
#define MOD_VEX_381A_PREFIX_2	(MOD_VEX_3819_PREFIX_2 + 1)
#define MOD_VEX_382A_PREFIX_2	(MOD_VEX_381A_PREFIX_2 + 1)
#define MOD_VEX_382C_PREFIX_2	(MOD_VEX_382A_PREFIX_2 + 1)
#define MOD_VEX_382D_PREFIX_2	(MOD_VEX_382C_PREFIX_2 + 1)
#define MOD_VEX_382E_PREFIX_2	(MOD_VEX_382D_PREFIX_2 + 1)
#define MOD_VEX_382F_PREFIX_2	(MOD_VEX_382E_PREFIX_2 + 1)

#define RM_0F01_REG_0		0
#define RM_0F01_REG_1		(RM_0F01_REG_0 + 1)
#define RM_0F01_REG_2		(RM_0F01_REG_1 + 1)
#define RM_0F01_REG_3		(RM_0F01_REG_2 + 1)
#define RM_0F01_REG_7		(RM_0F01_REG_3 + 1)
#define RM_0FAE_REG_5		(RM_0F01_REG_7 + 1)
#define RM_0FAE_REG_6		(RM_0FAE_REG_5 + 1)
#define RM_0FAE_REG_7		(RM_0FAE_REG_6 + 1)

#define PREFIX_90		0
#define PREFIX_0F10		(PREFIX_90 + 1)
#define PREFIX_0F11		(PREFIX_0F10 + 1)
#define PREFIX_0F12		(PREFIX_0F11 + 1)
#define PREFIX_0F16		(PREFIX_0F12 + 1)
#define PREFIX_0F2A		(PREFIX_0F16 + 1)
#define PREFIX_0F2B		(PREFIX_0F2A + 1)
#define PREFIX_0F2C		(PREFIX_0F2B + 1)
#define PREFIX_0F2D		(PREFIX_0F2C + 1)
#define PREFIX_0F2E		(PREFIX_0F2D + 1)
#define PREFIX_0F2F		(PREFIX_0F2E + 1)
#define PREFIX_0F51		(PREFIX_0F2F + 1)
#define PREFIX_0F52		(PREFIX_0F51 + 1)
#define PREFIX_0F53		(PREFIX_0F52 + 1)
#define PREFIX_0F58		(PREFIX_0F53 + 1)
#define PREFIX_0F59		(PREFIX_0F58 + 1)
#define PREFIX_0F5A		(PREFIX_0F59 + 1)
#define PREFIX_0F5B		(PREFIX_0F5A + 1)
#define PREFIX_0F5C		(PREFIX_0F5B + 1)
#define PREFIX_0F5D		(PREFIX_0F5C + 1)
#define PREFIX_0F5E		(PREFIX_0F5D + 1)
#define PREFIX_0F5F		(PREFIX_0F5E + 1)
#define PREFIX_0F60		(PREFIX_0F5F + 1)
#define PREFIX_0F61		(PREFIX_0F60 + 1)
#define PREFIX_0F62		(PREFIX_0F61 + 1)
#define PREFIX_0F6C		(PREFIX_0F62 + 1)
#define PREFIX_0F6D		(PREFIX_0F6C + 1)
#define PREFIX_0F6F		(PREFIX_0F6D + 1)
#define PREFIX_0F70		(PREFIX_0F6F + 1)
#define PREFIX_0F73_REG_3	(PREFIX_0F70 + 1)
#define PREFIX_0F73_REG_7	(PREFIX_0F73_REG_3 + 1)
#define PREFIX_0F78		(PREFIX_0F73_REG_7 + 1)
#define PREFIX_0F79		(PREFIX_0F78 + 1)
#define PREFIX_0F7C		(PREFIX_0F79 + 1)
#define PREFIX_0F7D		(PREFIX_0F7C + 1)
#define PREFIX_0F7E		(PREFIX_0F7D + 1)
#define PREFIX_0F7F		(PREFIX_0F7E + 1)
#define PREFIX_0FB8		(PREFIX_0F7F + 1)
#define PREFIX_0FBD		(PREFIX_0FB8 + 1)
#define PREFIX_0FC2		(PREFIX_0FBD + 1)
#define PREFIX_0FC3		(PREFIX_0FC2 + 1)
#define PREFIX_0FC7_REG_6	(PREFIX_0FC3 + 1)
#define PREFIX_0FD0		(PREFIX_0FC7_REG_6 + 1)
#define PREFIX_0FD6		(PREFIX_0FD0 + 1)
#define PREFIX_0FE6		(PREFIX_0FD6 + 1)
#define PREFIX_0FE7		(PREFIX_0FE6 + 1)
#define PREFIX_0FF0		(PREFIX_0FE7 + 1)
#define PREFIX_0FF7		(PREFIX_0FF0 + 1)
#define PREFIX_0F3810		(PREFIX_0FF7 + 1)
#define PREFIX_0F3814		(PREFIX_0F3810 + 1)
#define PREFIX_0F3815		(PREFIX_0F3814 + 1)
#define PREFIX_0F3817		(PREFIX_0F3815 + 1)
#define PREFIX_0F3820		(PREFIX_0F3817 + 1)
#define PREFIX_0F3821		(PREFIX_0F3820 + 1)
#define PREFIX_0F3822		(PREFIX_0F3821 + 1)
#define PREFIX_0F3823		(PREFIX_0F3822 + 1)
#define PREFIX_0F3824		(PREFIX_0F3823 + 1)
#define PREFIX_0F3825		(PREFIX_0F3824 + 1)
#define PREFIX_0F3828		(PREFIX_0F3825 + 1)
#define PREFIX_0F3829		(PREFIX_0F3828 + 1)
#define PREFIX_0F382A		(PREFIX_0F3829 + 1)
#define PREFIX_0F382B		(PREFIX_0F382A + 1)
#define PREFIX_0F3830		(PREFIX_0F382B + 1)
#define PREFIX_0F3831		(PREFIX_0F3830 + 1)
#define PREFIX_0F3832		(PREFIX_0F3831 + 1)
#define PREFIX_0F3833		(PREFIX_0F3832 + 1)
#define PREFIX_0F3834		(PREFIX_0F3833 + 1)
#define PREFIX_0F3835		(PREFIX_0F3834 + 1)
#define PREFIX_0F3837		(PREFIX_0F3835 + 1)
#define PREFIX_0F3838		(PREFIX_0F3837 + 1)
#define PREFIX_0F3839		(PREFIX_0F3838 + 1)
#define PREFIX_0F383A		(PREFIX_0F3839 + 1)
#define PREFIX_0F383B		(PREFIX_0F383A + 1)
#define PREFIX_0F383C		(PREFIX_0F383B + 1)
#define PREFIX_0F383D		(PREFIX_0F383C + 1)
#define PREFIX_0F383E		(PREFIX_0F383D + 1)
#define PREFIX_0F383F		(PREFIX_0F383E + 1)
#define PREFIX_0F3840		(PREFIX_0F383F + 1)
#define PREFIX_0F3841		(PREFIX_0F3840 + 1)
#define PREFIX_0F3880		(PREFIX_0F3841 + 1)
#define PREFIX_0F3881		(PREFIX_0F3880 + 1)
#define PREFIX_0F38DB		(PREFIX_0F3881 + 1)
#define PREFIX_0F38DC		(PREFIX_0F38DB + 1)
#define PREFIX_0F38DD		(PREFIX_0F38DC + 1)
#define PREFIX_0F38DE		(PREFIX_0F38DD + 1)
#define PREFIX_0F38DF		(PREFIX_0F38DE + 1)
#define PREFIX_0F38F0		(PREFIX_0F38DF + 1)
#define PREFIX_0F38F1		(PREFIX_0F38F0 + 1)
#define PREFIX_0F3A08		(PREFIX_0F38F1 + 1)
#define PREFIX_0F3A09		(PREFIX_0F3A08 + 1)
#define PREFIX_0F3A0A		(PREFIX_0F3A09 + 1)
#define PREFIX_0F3A0B		(PREFIX_0F3A0A + 1)
#define PREFIX_0F3A0C		(PREFIX_0F3A0B + 1)
#define PREFIX_0F3A0D		(PREFIX_0F3A0C + 1)
#define PREFIX_0F3A0E		(PREFIX_0F3A0D + 1)
#define PREFIX_0F3A14		(PREFIX_0F3A0E + 1)
#define PREFIX_0F3A15		(PREFIX_0F3A14 + 1)
#define PREFIX_0F3A16		(PREFIX_0F3A15 + 1)
#define PREFIX_0F3A17		(PREFIX_0F3A16 + 1)
#define PREFIX_0F3A20		(PREFIX_0F3A17 + 1)
#define PREFIX_0F3A21		(PREFIX_0F3A20 + 1)
#define PREFIX_0F3A22		(PREFIX_0F3A21 + 1)
#define PREFIX_0F3A40		(PREFIX_0F3A22 + 1)
#define PREFIX_0F3A41		(PREFIX_0F3A40 + 1)
#define PREFIX_0F3A42		(PREFIX_0F3A41 + 1)
#define PREFIX_0F3A44		(PREFIX_0F3A42 + 1)
#define PREFIX_0F3A60		(PREFIX_0F3A44 + 1)
#define PREFIX_0F3A61		(PREFIX_0F3A60 + 1)
#define PREFIX_0F3A62		(PREFIX_0F3A61 + 1)
#define PREFIX_0F3A63		(PREFIX_0F3A62 + 1)
#define PREFIX_0F3ADF		(PREFIX_0F3A63 + 1)
#define PREFIX_VEX_10		(PREFIX_0F3ADF + 1)
#define PREFIX_VEX_11		(PREFIX_VEX_10 + 1)
#define PREFIX_VEX_12		(PREFIX_VEX_11 + 1)
#define PREFIX_VEX_16		(PREFIX_VEX_12 + 1)
#define PREFIX_VEX_2A		(PREFIX_VEX_16 + 1)
#define PREFIX_VEX_2C		(PREFIX_VEX_2A + 1)
#define PREFIX_VEX_2D		(PREFIX_VEX_2C + 1)
#define PREFIX_VEX_2E		(PREFIX_VEX_2D + 1)
#define PREFIX_VEX_2F		(PREFIX_VEX_2E + 1)
#define PREFIX_VEX_51		(PREFIX_VEX_2F + 1)
#define PREFIX_VEX_52		(PREFIX_VEX_51 + 1)
#define PREFIX_VEX_53		(PREFIX_VEX_52 + 1)
#define PREFIX_VEX_58		(PREFIX_VEX_53 + 1)
#define PREFIX_VEX_59		(PREFIX_VEX_58 + 1)
#define PREFIX_VEX_5A		(PREFIX_VEX_59 + 1)
#define PREFIX_VEX_5B		(PREFIX_VEX_5A + 1)
#define PREFIX_VEX_5C		(PREFIX_VEX_5B + 1)
#define PREFIX_VEX_5D		(PREFIX_VEX_5C + 1)
#define PREFIX_VEX_5E		(PREFIX_VEX_5D + 1)
#define PREFIX_VEX_5F		(PREFIX_VEX_5E + 1)
#define PREFIX_VEX_60		(PREFIX_VEX_5F + 1)
#define PREFIX_VEX_61		(PREFIX_VEX_60 + 1)
#define PREFIX_VEX_62		(PREFIX_VEX_61 + 1)
#define PREFIX_VEX_63		(PREFIX_VEX_62 + 1)
#define PREFIX_VEX_64		(PREFIX_VEX_63 + 1)
#define PREFIX_VEX_65		(PREFIX_VEX_64 + 1)
#define PREFIX_VEX_66		(PREFIX_VEX_65 + 1)
#define PREFIX_VEX_67		(PREFIX_VEX_66 + 1)
#define PREFIX_VEX_68		(PREFIX_VEX_67 + 1)
#define PREFIX_VEX_69		(PREFIX_VEX_68 + 1)
#define PREFIX_VEX_6A		(PREFIX_VEX_69 + 1)
#define PREFIX_VEX_6B		(PREFIX_VEX_6A + 1)
#define PREFIX_VEX_6C		(PREFIX_VEX_6B + 1)
#define PREFIX_VEX_6D		(PREFIX_VEX_6C + 1)
#define PREFIX_VEX_6E		(PREFIX_VEX_6D + 1)
#define PREFIX_VEX_6F		(PREFIX_VEX_6E + 1)
#define PREFIX_VEX_70		(PREFIX_VEX_6F + 1)
#define PREFIX_VEX_71_REG_2	(PREFIX_VEX_70 + 1)
#define PREFIX_VEX_71_REG_4	(PREFIX_VEX_71_REG_2 + 1)
#define PREFIX_VEX_71_REG_6	(PREFIX_VEX_71_REG_4 + 1)
#define PREFIX_VEX_72_REG_2	(PREFIX_VEX_71_REG_6 + 1)
#define PREFIX_VEX_72_REG_4	(PREFIX_VEX_72_REG_2 + 1)
#define PREFIX_VEX_72_REG_6	(PREFIX_VEX_72_REG_4 + 1)
#define PREFIX_VEX_73_REG_2	(PREFIX_VEX_72_REG_6 + 1)
#define PREFIX_VEX_73_REG_3	(PREFIX_VEX_73_REG_2 + 1)
#define PREFIX_VEX_73_REG_6	(PREFIX_VEX_73_REG_3 + 1)
#define PREFIX_VEX_73_REG_7	(PREFIX_VEX_73_REG_6 + 1)
#define PREFIX_VEX_74		(PREFIX_VEX_73_REG_7 + 1)
#define PREFIX_VEX_75		(PREFIX_VEX_74 + 1)
#define PREFIX_VEX_76		(PREFIX_VEX_75 + 1)
#define PREFIX_VEX_77		(PREFIX_VEX_76 + 1)
#define PREFIX_VEX_7C		(PREFIX_VEX_77 + 1)
#define PREFIX_VEX_7D		(PREFIX_VEX_7C + 1)
#define PREFIX_VEX_7E		(PREFIX_VEX_7D + 1)
#define PREFIX_VEX_7F		(PREFIX_VEX_7E + 1)
#define PREFIX_VEX_C2		(PREFIX_VEX_7F + 1)
#define PREFIX_VEX_C4		(PREFIX_VEX_C2 + 1)
#define PREFIX_VEX_C5		(PREFIX_VEX_C4 + 1)
#define PREFIX_VEX_D0		(PREFIX_VEX_C5 + 1)
#define PREFIX_VEX_D1		(PREFIX_VEX_D0 + 1)
#define PREFIX_VEX_D2		(PREFIX_VEX_D1 + 1)
#define PREFIX_VEX_D3		(PREFIX_VEX_D2 + 1)
#define PREFIX_VEX_D4		(PREFIX_VEX_D3 + 1)
#define PREFIX_VEX_D5		(PREFIX_VEX_D4 + 1)
#define PREFIX_VEX_D6		(PREFIX_VEX_D5 + 1)
#define PREFIX_VEX_D7		(PREFIX_VEX_D6 + 1)
#define PREFIX_VEX_D8		(PREFIX_VEX_D7 + 1)
#define PREFIX_VEX_D9		(PREFIX_VEX_D8 + 1)
#define PREFIX_VEX_DA		(PREFIX_VEX_D9 + 1)
#define PREFIX_VEX_DB		(PREFIX_VEX_DA + 1)
#define PREFIX_VEX_DC		(PREFIX_VEX_DB + 1)
#define PREFIX_VEX_DD		(PREFIX_VEX_DC + 1)
#define PREFIX_VEX_DE		(PREFIX_VEX_DD + 1)
#define PREFIX_VEX_DF		(PREFIX_VEX_DE + 1)
#define PREFIX_VEX_E0		(PREFIX_VEX_DF + 1)
#define PREFIX_VEX_E1		(PREFIX_VEX_E0 + 1)
#define PREFIX_VEX_E2		(PREFIX_VEX_E1 + 1)
#define PREFIX_VEX_E3		(PREFIX_VEX_E2 + 1)
#define PREFIX_VEX_E4		(PREFIX_VEX_E3 + 1)
#define PREFIX_VEX_E5		(PREFIX_VEX_E4 + 1)
#define PREFIX_VEX_E6		(PREFIX_VEX_E5 + 1)
#define PREFIX_VEX_E7		(PREFIX_VEX_E6 + 1)
#define PREFIX_VEX_E8		(PREFIX_VEX_E7 + 1)
#define PREFIX_VEX_E9		(PREFIX_VEX_E8 + 1)
#define PREFIX_VEX_EA		(PREFIX_VEX_E9 + 1)
#define PREFIX_VEX_EB		(PREFIX_VEX_EA + 1)
#define PREFIX_VEX_EC		(PREFIX_VEX_EB + 1)
#define PREFIX_VEX_ED		(PREFIX_VEX_EC + 1)
#define PREFIX_VEX_EE		(PREFIX_VEX_ED + 1)
#define PREFIX_VEX_EF		(PREFIX_VEX_EE + 1)
#define PREFIX_VEX_F0		(PREFIX_VEX_EF + 1)
#define PREFIX_VEX_F1		(PREFIX_VEX_F0 + 1)
#define PREFIX_VEX_F2		(PREFIX_VEX_F1 + 1)
#define PREFIX_VEX_F3		(PREFIX_VEX_F2 + 1)
#define PREFIX_VEX_F4		(PREFIX_VEX_F3 + 1)
#define PREFIX_VEX_F5		(PREFIX_VEX_F4 + 1)
#define PREFIX_VEX_F6		(PREFIX_VEX_F5 + 1)
#define PREFIX_VEX_F7		(PREFIX_VEX_F6 + 1)
#define PREFIX_VEX_F8		(PREFIX_VEX_F7 + 1)
#define PREFIX_VEX_F9		(PREFIX_VEX_F8 + 1)
#define PREFIX_VEX_FA		(PREFIX_VEX_F9 + 1)
#define PREFIX_VEX_FB		(PREFIX_VEX_FA + 1)
#define PREFIX_VEX_FC		(PREFIX_VEX_FB + 1)
#define PREFIX_VEX_FD		(PREFIX_VEX_FC + 1)
#define PREFIX_VEX_FE		(PREFIX_VEX_FD + 1)
#define PREFIX_VEX_3800		(PREFIX_VEX_FE + 1)
#define PREFIX_VEX_3801		(PREFIX_VEX_3800 + 1)
#define PREFIX_VEX_3802		(PREFIX_VEX_3801 + 1)
#define PREFIX_VEX_3803		(PREFIX_VEX_3802 + 1)
#define PREFIX_VEX_3804		(PREFIX_VEX_3803 + 1)
#define PREFIX_VEX_3805		(PREFIX_VEX_3804 + 1)
#define PREFIX_VEX_3806		(PREFIX_VEX_3805 + 1)
#define PREFIX_VEX_3807		(PREFIX_VEX_3806 + 1)
#define PREFIX_VEX_3808		(PREFIX_VEX_3807 + 1)
#define PREFIX_VEX_3809		(PREFIX_VEX_3808 + 1)
#define PREFIX_VEX_380A		(PREFIX_VEX_3809 + 1)
#define PREFIX_VEX_380B		(PREFIX_VEX_380A + 1)
#define PREFIX_VEX_380C		(PREFIX_VEX_380B + 1)
#define PREFIX_VEX_380D		(PREFIX_VEX_380C + 1)
#define PREFIX_VEX_380E		(PREFIX_VEX_380D + 1)
#define PREFIX_VEX_380F		(PREFIX_VEX_380E + 1)
#define PREFIX_VEX_3817		(PREFIX_VEX_380F + 1)
#define PREFIX_VEX_3818		(PREFIX_VEX_3817 + 1)
#define PREFIX_VEX_3819		(PREFIX_VEX_3818 + 1)
#define PREFIX_VEX_381A		(PREFIX_VEX_3819 + 1)
#define PREFIX_VEX_381C		(PREFIX_VEX_381A + 1)
#define PREFIX_VEX_381D		(PREFIX_VEX_381C + 1)
#define PREFIX_VEX_381E		(PREFIX_VEX_381D + 1)
#define PREFIX_VEX_3820		(PREFIX_VEX_381E + 1)
#define PREFIX_VEX_3821		(PREFIX_VEX_3820 + 1)
#define PREFIX_VEX_3822		(PREFIX_VEX_3821 + 1)
#define PREFIX_VEX_3823		(PREFIX_VEX_3822 + 1)
#define PREFIX_VEX_3824		(PREFIX_VEX_3823 + 1)
#define PREFIX_VEX_3825		(PREFIX_VEX_3824 + 1)
#define PREFIX_VEX_3828		(PREFIX_VEX_3825 + 1)
#define PREFIX_VEX_3829		(PREFIX_VEX_3828 + 1)
#define PREFIX_VEX_382A		(PREFIX_VEX_3829 + 1)
#define PREFIX_VEX_382B		(PREFIX_VEX_382A + 1)
#define PREFIX_VEX_382C		(PREFIX_VEX_382B + 1)
#define PREFIX_VEX_382D		(PREFIX_VEX_382C + 1)
#define PREFIX_VEX_382E		(PREFIX_VEX_382D + 1)
#define PREFIX_VEX_382F		(PREFIX_VEX_382E + 1)
#define PREFIX_VEX_3830		(PREFIX_VEX_382F + 1)
#define PREFIX_VEX_3831		(PREFIX_VEX_3830 + 1)
#define PREFIX_VEX_3832		(PREFIX_VEX_3831 + 1)
#define PREFIX_VEX_3833		(PREFIX_VEX_3832 + 1)
#define PREFIX_VEX_3834		(PREFIX_VEX_3833 + 1)
#define PREFIX_VEX_3835		(PREFIX_VEX_3834 + 1)
#define PREFIX_VEX_3837		(PREFIX_VEX_3835 + 1)
#define PREFIX_VEX_3838		(PREFIX_VEX_3837 + 1)
#define PREFIX_VEX_3839		(PREFIX_VEX_3838 + 1)
#define PREFIX_VEX_383A		(PREFIX_VEX_3839 + 1)
#define PREFIX_VEX_383B		(PREFIX_VEX_383A + 1)
#define PREFIX_VEX_383C		(PREFIX_VEX_383B + 1)
#define PREFIX_VEX_383D		(PREFIX_VEX_383C + 1)
#define PREFIX_VEX_383E		(PREFIX_VEX_383D + 1)
#define PREFIX_VEX_383F		(PREFIX_VEX_383E + 1)
#define PREFIX_VEX_3840		(PREFIX_VEX_383F + 1)
#define PREFIX_VEX_3841		(PREFIX_VEX_3840 + 1)
#define PREFIX_VEX_38DB		(PREFIX_VEX_3841 + 1)
#define PREFIX_VEX_38DC		(PREFIX_VEX_38DB + 1)
#define PREFIX_VEX_38DD		(PREFIX_VEX_38DC + 1)
#define PREFIX_VEX_38DE		(PREFIX_VEX_38DD + 1)
#define PREFIX_VEX_38DF		(PREFIX_VEX_38DE + 1)
#define PREFIX_VEX_3A04		(PREFIX_VEX_38DF + 1)
#define PREFIX_VEX_3A05		(PREFIX_VEX_3A04 + 1)
#define PREFIX_VEX_3A06		(PREFIX_VEX_3A05 + 1)
#define PREFIX_VEX_3A08		(PREFIX_VEX_3A06 + 1)
#define PREFIX_VEX_3A09		(PREFIX_VEX_3A08 + 1)
#define PREFIX_VEX_3A0A		(PREFIX_VEX_3A09 + 1)
#define PREFIX_VEX_3A0B		(PREFIX_VEX_3A0A + 1)
#define PREFIX_VEX_3A0C		(PREFIX_VEX_3A0B + 1)
#define PREFIX_VEX_3A0D		(PREFIX_VEX_3A0C + 1)
#define PREFIX_VEX_3A0E		(PREFIX_VEX_3A0D + 1)
#define PREFIX_VEX_3A0F		(PREFIX_VEX_3A0E + 1)
#define PREFIX_VEX_3A14		(PREFIX_VEX_3A0F + 1)
#define PREFIX_VEX_3A15		(PREFIX_VEX_3A14 + 1)
#define PREFIX_VEX_3A16		(PREFIX_VEX_3A15 + 1)
#define PREFIX_VEX_3A17		(PREFIX_VEX_3A16 + 1)
#define PREFIX_VEX_3A18		(PREFIX_VEX_3A17 + 1)
#define PREFIX_VEX_3A19		(PREFIX_VEX_3A18 + 1)
#define PREFIX_VEX_3A20		(PREFIX_VEX_3A19 + 1)
#define PREFIX_VEX_3A21		(PREFIX_VEX_3A20 + 1)
#define PREFIX_VEX_3A22		(PREFIX_VEX_3A21 + 1)
#define PREFIX_VEX_3A40		(PREFIX_VEX_3A22 + 1)
#define PREFIX_VEX_3A41		(PREFIX_VEX_3A40 + 1)
#define PREFIX_VEX_3A42		(PREFIX_VEX_3A41 + 1)
#define PREFIX_VEX_3A48		(PREFIX_VEX_3A42 + 1)
#define PREFIX_VEX_3A49		(PREFIX_VEX_3A48 + 1)
#define PREFIX_VEX_3A4A		(PREFIX_VEX_3A49 + 1)
#define PREFIX_VEX_3A4B		(PREFIX_VEX_3A4A + 1)
#define PREFIX_VEX_3A4C		(PREFIX_VEX_3A4B + 1)
#define PREFIX_VEX_3A5C		(PREFIX_VEX_3A4C + 1)
#define PREFIX_VEX_3A5D		(PREFIX_VEX_3A5C + 1)
#define PREFIX_VEX_3A5E		(PREFIX_VEX_3A5D + 1)
#define PREFIX_VEX_3A5F		(PREFIX_VEX_3A5E + 1)
#define PREFIX_VEX_3A60		(PREFIX_VEX_3A5F + 1)
#define PREFIX_VEX_3A61		(PREFIX_VEX_3A60 + 1)
#define PREFIX_VEX_3A62		(PREFIX_VEX_3A61 + 1)
#define PREFIX_VEX_3A63		(PREFIX_VEX_3A62 + 1)
#define PREFIX_VEX_3A68		(PREFIX_VEX_3A63 + 1)
#define PREFIX_VEX_3A69		(PREFIX_VEX_3A68 + 1)
#define PREFIX_VEX_3A6A		(PREFIX_VEX_3A69 + 1)
#define PREFIX_VEX_3A6B		(PREFIX_VEX_3A6A + 1)
#define PREFIX_VEX_3A6C		(PREFIX_VEX_3A6B + 1)
#define PREFIX_VEX_3A6D		(PREFIX_VEX_3A6C + 1)
#define PREFIX_VEX_3A6E		(PREFIX_VEX_3A6D + 1)
#define PREFIX_VEX_3A6F		(PREFIX_VEX_3A6E + 1)
#define PREFIX_VEX_3A78		(PREFIX_VEX_3A6F + 1)
#define PREFIX_VEX_3A79		(PREFIX_VEX_3A78 + 1)
#define PREFIX_VEX_3A7A		(PREFIX_VEX_3A79 + 1)
#define PREFIX_VEX_3A7B		(PREFIX_VEX_3A7A + 1)
#define PREFIX_VEX_3A7C		(PREFIX_VEX_3A7B + 1)
#define PREFIX_VEX_3A7D		(PREFIX_VEX_3A7C + 1)
#define PREFIX_VEX_3A7E		(PREFIX_VEX_3A7D + 1)
#define PREFIX_VEX_3A7F		(PREFIX_VEX_3A7E + 1)
#define PREFIX_VEX_3ADF		(PREFIX_VEX_3A7F + 1)

#define X86_64_06		0
#define X86_64_07		(X86_64_06 + 1)
#define X86_64_0D		(X86_64_07 + 1)
#define X86_64_16		(X86_64_0D + 1)
#define X86_64_17		(X86_64_16 + 1)
#define X86_64_1E		(X86_64_17 + 1)
#define X86_64_1F		(X86_64_1E + 1)
#define X86_64_27		(X86_64_1F + 1)
#define X86_64_2F		(X86_64_27 + 1)
#define X86_64_37		(X86_64_2F + 1)
#define X86_64_3F		(X86_64_37 + 1)
#define X86_64_60		(X86_64_3F + 1)
#define X86_64_61		(X86_64_60 + 1)
#define X86_64_62		(X86_64_61 + 1)
#define X86_64_63		(X86_64_62 + 1)
#define X86_64_6D		(X86_64_63 + 1)
#define X86_64_6F		(X86_64_6D + 1)
#define X86_64_9A		(X86_64_6F + 1)
#define X86_64_C4		(X86_64_9A + 1)
#define X86_64_C5		(X86_64_C4 + 1)
#define X86_64_CE		(X86_64_C5 + 1)
#define X86_64_D4		(X86_64_CE + 1)
#define X86_64_D5		(X86_64_D4 + 1)
#define X86_64_EA		(X86_64_D5 + 1)
#define X86_64_0F01_REG_0	(X86_64_EA + 1)
#define X86_64_0F01_REG_1	(X86_64_0F01_REG_0 + 1)
#define X86_64_0F01_REG_2	(X86_64_0F01_REG_1 + 1)
#define X86_64_0F01_REG_3	(X86_64_0F01_REG_2 + 1)

#define THREE_BYTE_0F24		0
#define THREE_BYTE_0F25		(THREE_BYTE_0F24 + 1)
#define THREE_BYTE_0F38		(THREE_BYTE_0F25 + 1)
#define THREE_BYTE_0F3A		(THREE_BYTE_0F38 + 1)
#define THREE_BYTE_0F7A		(THREE_BYTE_0F3A + 1)
#define THREE_BYTE_0F7B		(THREE_BYTE_0F7A + 1)

#define VEX_0F			0
#define VEX_0F38		(VEX_0F + 1)
#define VEX_0F3A		(VEX_0F38 + 1)

#define VEX_LEN_10_P_1	0
#define VEX_LEN_10_P_3	(VEX_LEN_10_P_1 + 1)
#define VEX_LEN_11_P_1	(VEX_LEN_10_P_3 + 1)
#define VEX_LEN_11_P_3	(VEX_LEN_11_P_1 + 1)
#define VEX_LEN_12_P_0_M_0	(VEX_LEN_11_P_3 + 1)
#define VEX_LEN_12_P_0_M_1	(VEX_LEN_12_P_0_M_0 + 1)
#define VEX_LEN_12_P_2	(VEX_LEN_12_P_0_M_1 + 1)
#define VEX_LEN_13_M_0	(VEX_LEN_12_P_2 + 1)
#define VEX_LEN_16_P_0_M_0	(VEX_LEN_13_M_0 + 1)
#define VEX_LEN_16_P_0_M_1	(VEX_LEN_16_P_0_M_0 + 1)
#define VEX_LEN_16_P_2	(VEX_LEN_16_P_0_M_1 + 1)
#define VEX_LEN_17_M_0	(VEX_LEN_16_P_2 + 1)
#define VEX_LEN_2A_P_1	(VEX_LEN_17_M_0 + 1)
#define VEX_LEN_2A_P_3	(VEX_LEN_2A_P_1 + 1)
#define VEX_LEN_2B_M_0	(VEX_LEN_2A_P_3 + 1)
#define VEX_LEN_2C_P_1	(VEX_LEN_2B_M_0 + 1)
#define VEX_LEN_2C_P_3	(VEX_LEN_2C_P_1 + 1)
#define VEX_LEN_2D_P_1	(VEX_LEN_2C_P_3 + 1)
#define VEX_LEN_2D_P_3	(VEX_LEN_2D_P_1 + 1)
#define VEX_LEN_2E_P_0	(VEX_LEN_2D_P_3 + 1)
#define VEX_LEN_2E_P_2	(VEX_LEN_2E_P_0 + 1)
#define VEX_LEN_2F_P_0	(VEX_LEN_2E_P_2 + 1)
#define VEX_LEN_2F_P_2	(VEX_LEN_2F_P_0 + 1)
#define VEX_LEN_51_P_1	(VEX_LEN_2F_P_2 + 1)
#define VEX_LEN_51_P_3	(VEX_LEN_51_P_1 + 1)
#define VEX_LEN_52_P_1	(VEX_LEN_51_P_3 + 1)
#define VEX_LEN_53_P_1	(VEX_LEN_52_P_1 + 1)
#define VEX_LEN_58_P_1	(VEX_LEN_53_P_1 + 1)
#define VEX_LEN_58_P_3	(VEX_LEN_58_P_1 + 1)
#define VEX_LEN_59_P_1	(VEX_LEN_58_P_3 + 1)
#define VEX_LEN_59_P_3	(VEX_LEN_59_P_1 + 1)
#define VEX_LEN_5A_P_1	(VEX_LEN_59_P_3 + 1)
#define VEX_LEN_5A_P_3	(VEX_LEN_5A_P_1 + 1)
#define VEX_LEN_5C_P_1	(VEX_LEN_5A_P_3 + 1)
#define VEX_LEN_5C_P_3	(VEX_LEN_5C_P_1 + 1)
#define VEX_LEN_5D_P_1	(VEX_LEN_5C_P_3 + 1)
#define VEX_LEN_5D_P_3	(VEX_LEN_5D_P_1 + 1)
#define VEX_LEN_5E_P_1	(VEX_LEN_5D_P_3 + 1)
#define VEX_LEN_5E_P_3	(VEX_LEN_5E_P_1 + 1)
#define VEX_LEN_5F_P_1	(VEX_LEN_5E_P_3 + 1)
#define VEX_LEN_5F_P_3	(VEX_LEN_5F_P_1 + 1)
#define VEX_LEN_60_P_2	(VEX_LEN_5F_P_3 + 1)
#define VEX_LEN_61_P_2	(VEX_LEN_60_P_2 + 1)
#define VEX_LEN_62_P_2	(VEX_LEN_61_P_2 + 1)
#define VEX_LEN_63_P_2	(VEX_LEN_62_P_2 + 1)
#define VEX_LEN_64_P_2	(VEX_LEN_63_P_2 + 1)
#define VEX_LEN_65_P_2	(VEX_LEN_64_P_2 + 1)
#define VEX_LEN_66_P_2	(VEX_LEN_65_P_2 + 1)
#define VEX_LEN_67_P_2	(VEX_LEN_66_P_2 + 1)
#define VEX_LEN_68_P_2	(VEX_LEN_67_P_2 + 1)
#define VEX_LEN_69_P_2	(VEX_LEN_68_P_2 + 1)
#define VEX_LEN_6A_P_2	(VEX_LEN_69_P_2 + 1)
#define VEX_LEN_6B_P_2	(VEX_LEN_6A_P_2 + 1)
#define VEX_LEN_6C_P_2	(VEX_LEN_6B_P_2 + 1)
#define VEX_LEN_6D_P_2	(VEX_LEN_6C_P_2 + 1)
#define VEX_LEN_6E_P_2	(VEX_LEN_6D_P_2 + 1)
#define VEX_LEN_70_P_1	(VEX_LEN_6E_P_2 + 1)
#define VEX_LEN_70_P_2	(VEX_LEN_70_P_1 + 1)
#define VEX_LEN_70_P_3	(VEX_LEN_70_P_2 + 1)
#define VEX_LEN_71_R_2_P_2	(VEX_LEN_70_P_3 + 1)
#define VEX_LEN_71_R_4_P_2	(VEX_LEN_71_R_2_P_2 + 1)
#define VEX_LEN_71_R_6_P_2	(VEX_LEN_71_R_4_P_2 + 1)
#define VEX_LEN_72_R_2_P_2	(VEX_LEN_71_R_6_P_2 + 1)
#define VEX_LEN_72_R_4_P_2	(VEX_LEN_72_R_2_P_2 + 1)
#define VEX_LEN_72_R_6_P_2	(VEX_LEN_72_R_4_P_2 + 1)
#define VEX_LEN_73_R_2_P_2	(VEX_LEN_72_R_6_P_2 + 1)
#define VEX_LEN_73_R_3_P_2	(VEX_LEN_73_R_2_P_2 + 1)
#define VEX_LEN_73_R_6_P_2	(VEX_LEN_73_R_3_P_2 + 1)
#define VEX_LEN_73_R_7_P_2	(VEX_LEN_73_R_6_P_2 + 1)
#define VEX_LEN_74_P_2	(VEX_LEN_73_R_7_P_2 + 1)
#define VEX_LEN_75_P_2	(VEX_LEN_74_P_2 + 1)
#define VEX_LEN_76_P_2	(VEX_LEN_75_P_2 + 1)
#define VEX_LEN_7E_P_1	(VEX_LEN_76_P_2 + 1)
#define VEX_LEN_7E_P_2	(VEX_LEN_7E_P_1 + 1)
#define VEX_LEN_AE_R_2_M_0	(VEX_LEN_7E_P_2 + 1)
#define VEX_LEN_AE_R_3_M_0	(VEX_LEN_AE_R_2_M_0 + 1)
#define VEX_LEN_C2_P_1	(VEX_LEN_AE_R_3_M_0 + 1)
#define VEX_LEN_C2_P_3	(VEX_LEN_C2_P_1 + 1)
#define VEX_LEN_C4_P_2	(VEX_LEN_C2_P_3 + 1)
#define VEX_LEN_C5_P_2	(VEX_LEN_C4_P_2 + 1)
#define VEX_LEN_D1_P_2	(VEX_LEN_C5_P_2 + 1)
#define VEX_LEN_D2_P_2	(VEX_LEN_D1_P_2 + 1)
#define VEX_LEN_D3_P_2	(VEX_LEN_D2_P_2 + 1)
#define VEX_LEN_D4_P_2	(VEX_LEN_D3_P_2 + 1)
#define VEX_LEN_D5_P_2	(VEX_LEN_D4_P_2 + 1)
#define VEX_LEN_D6_P_2	(VEX_LEN_D5_P_2 + 1)
#define VEX_LEN_D7_P_2_M_1	(VEX_LEN_D6_P_2 + 1)
#define VEX_LEN_D8_P_2	(VEX_LEN_D7_P_2_M_1 + 1)
#define VEX_LEN_D9_P_2	(VEX_LEN_D8_P_2 + 1)
#define VEX_LEN_DA_P_2	(VEX_LEN_D9_P_2 + 1)
#define VEX_LEN_DB_P_2	(VEX_LEN_DA_P_2 + 1)
#define VEX_LEN_DC_P_2	(VEX_LEN_DB_P_2 + 1)
#define VEX_LEN_DD_P_2	(VEX_LEN_DC_P_2 + 1)
#define VEX_LEN_DE_P_2	(VEX_LEN_DD_P_2 + 1)
#define VEX_LEN_DF_P_2	(VEX_LEN_DE_P_2 + 1)
#define VEX_LEN_E0_P_2	(VEX_LEN_DF_P_2 + 1)
#define VEX_LEN_E1_P_2	(VEX_LEN_E0_P_2 + 1)
#define VEX_LEN_E2_P_2	(VEX_LEN_E1_P_2 + 1)
#define VEX_LEN_E3_P_2	(VEX_LEN_E2_P_2 + 1)
#define VEX_LEN_E4_P_2	(VEX_LEN_E3_P_2 + 1)
#define VEX_LEN_E5_P_2	(VEX_LEN_E4_P_2 + 1)
#define VEX_LEN_E7_P_2_M_0	(VEX_LEN_E5_P_2 + 1)
#define VEX_LEN_E8_P_2	(VEX_LEN_E7_P_2_M_0 + 1)
#define VEX_LEN_E9_P_2	(VEX_LEN_E8_P_2 + 1)
#define VEX_LEN_EA_P_2	(VEX_LEN_E9_P_2 + 1)
#define VEX_LEN_EB_P_2	(VEX_LEN_EA_P_2 + 1)
#define VEX_LEN_EC_P_2	(VEX_LEN_EB_P_2 + 1)
#define VEX_LEN_ED_P_2	(VEX_LEN_EC_P_2 + 1)
#define VEX_LEN_EE_P_2	(VEX_LEN_ED_P_2 + 1)
#define VEX_LEN_EF_P_2	(VEX_LEN_EE_P_2 + 1)
#define VEX_LEN_F1_P_2	(VEX_LEN_EF_P_2 + 1)
#define VEX_LEN_F2_P_2	(VEX_LEN_F1_P_2 + 1)
#define VEX_LEN_F3_P_2	(VEX_LEN_F2_P_2 + 1)
#define VEX_LEN_F4_P_2	(VEX_LEN_F3_P_2 + 1)
#define VEX_LEN_F5_P_2	(VEX_LEN_F4_P_2 + 1)
#define VEX_LEN_F6_P_2	(VEX_LEN_F5_P_2 + 1)
#define VEX_LEN_F7_P_2	(VEX_LEN_F6_P_2 + 1)
#define VEX_LEN_F8_P_2	(VEX_LEN_F7_P_2 + 1)
#define VEX_LEN_F9_P_2	(VEX_LEN_F8_P_2 + 1)
#define VEX_LEN_FA_P_2	(VEX_LEN_F9_P_2 + 1)
#define VEX_LEN_FB_P_2	(VEX_LEN_FA_P_2 + 1)
#define VEX_LEN_FC_P_2	(VEX_LEN_FB_P_2 + 1)
#define VEX_LEN_FD_P_2	(VEX_LEN_FC_P_2 + 1)
#define VEX_LEN_FE_P_2	(VEX_LEN_FD_P_2 + 1)
#define VEX_LEN_3800_P_2	(VEX_LEN_FE_P_2 + 1)
#define VEX_LEN_3801_P_2	(VEX_LEN_3800_P_2 + 1)
#define VEX_LEN_3802_P_2	(VEX_LEN_3801_P_2 + 1)
#define VEX_LEN_3803_P_2	(VEX_LEN_3802_P_2 + 1)
#define VEX_LEN_3804_P_2	(VEX_LEN_3803_P_2 + 1)
#define VEX_LEN_3805_P_2	(VEX_LEN_3804_P_2 + 1)
#define VEX_LEN_3806_P_2	(VEX_LEN_3805_P_2 + 1)
#define VEX_LEN_3807_P_2	(VEX_LEN_3806_P_2 + 1)
#define VEX_LEN_3808_P_2	(VEX_LEN_3807_P_2 + 1)
#define VEX_LEN_3809_P_2	(VEX_LEN_3808_P_2 + 1)
#define VEX_LEN_380A_P_2	(VEX_LEN_3809_P_2 + 1)
#define VEX_LEN_380B_P_2	(VEX_LEN_380A_P_2 + 1)
#define VEX_LEN_3819_P_2_M_0	(VEX_LEN_380B_P_2 + 1)
#define VEX_LEN_381A_P_2_M_0	(VEX_LEN_3819_P_2_M_0 + 1)
#define VEX_LEN_381C_P_2	(VEX_LEN_381A_P_2_M_0 + 1)
#define VEX_LEN_381D_P_2	(VEX_LEN_381C_P_2 + 1)
#define VEX_LEN_381E_P_2	(VEX_LEN_381D_P_2 + 1)
#define VEX_LEN_3820_P_2	(VEX_LEN_381E_P_2 + 1)
#define VEX_LEN_3821_P_2	(VEX_LEN_3820_P_2 + 1)
#define VEX_LEN_3822_P_2	(VEX_LEN_3821_P_2 + 1)
#define VEX_LEN_3823_P_2	(VEX_LEN_3822_P_2 + 1)
#define VEX_LEN_3824_P_2	(VEX_LEN_3823_P_2 + 1)
#define VEX_LEN_3825_P_2	(VEX_LEN_3824_P_2 + 1)
#define VEX_LEN_3828_P_2	(VEX_LEN_3825_P_2 + 1)
#define VEX_LEN_3829_P_2	(VEX_LEN_3828_P_2 + 1)
#define VEX_LEN_382A_P_2_M_0	(VEX_LEN_3829_P_2 + 1)
#define VEX_LEN_382B_P_2	(VEX_LEN_382A_P_2_M_0 + 1)
#define VEX_LEN_3830_P_2	(VEX_LEN_382B_P_2 + 1)
#define VEX_LEN_3831_P_2	(VEX_LEN_3830_P_2 + 1)
#define VEX_LEN_3832_P_2	(VEX_LEN_3831_P_2 + 1)
#define VEX_LEN_3833_P_2	(VEX_LEN_3832_P_2 + 1)
#define VEX_LEN_3834_P_2	(VEX_LEN_3833_P_2 + 1)
#define VEX_LEN_3835_P_2	(VEX_LEN_3834_P_2 + 1)
#define VEX_LEN_3837_P_2	(VEX_LEN_3835_P_2 + 1)
#define VEX_LEN_3838_P_2	(VEX_LEN_3837_P_2 + 1)
#define VEX_LEN_3839_P_2	(VEX_LEN_3838_P_2 + 1)
#define VEX_LEN_383A_P_2	(VEX_LEN_3839_P_2 + 1)
#define VEX_LEN_383B_P_2	(VEX_LEN_383A_P_2 + 1)
#define VEX_LEN_383C_P_2	(VEX_LEN_383B_P_2 + 1)
#define VEX_LEN_383D_P_2	(VEX_LEN_383C_P_2 + 1)
#define VEX_LEN_383E_P_2	(VEX_LEN_383D_P_2 + 1)
#define VEX_LEN_383F_P_2	(VEX_LEN_383E_P_2 + 1)
#define VEX_LEN_3840_P_2	(VEX_LEN_383F_P_2 + 1)
#define VEX_LEN_3841_P_2	(VEX_LEN_3840_P_2 + 1)
#define VEX_LEN_38DB_P_2	(VEX_LEN_3841_P_2 + 1)
#define VEX_LEN_38DC_P_2	(VEX_LEN_38DB_P_2 + 1)
#define VEX_LEN_38DD_P_2	(VEX_LEN_38DC_P_2 + 1)
#define VEX_LEN_38DE_P_2	(VEX_LEN_38DD_P_2 + 1)
#define VEX_LEN_38DF_P_2	(VEX_LEN_38DE_P_2 + 1)
#define VEX_LEN_3A06_P_2	(VEX_LEN_38DF_P_2 + 1)
#define VEX_LEN_3A0A_P_2	(VEX_LEN_3A06_P_2 + 1)
#define VEX_LEN_3A0B_P_2	(VEX_LEN_3A0A_P_2 + 1)
#define VEX_LEN_3A0E_P_2	(VEX_LEN_3A0B_P_2 + 1)
#define VEX_LEN_3A0F_P_2	(VEX_LEN_3A0E_P_2 + 1)
#define VEX_LEN_3A14_P_2	(VEX_LEN_3A0F_P_2 + 1)
#define VEX_LEN_3A15_P_2	(VEX_LEN_3A14_P_2 + 1)
#define VEX_LEN_3A16_P_2	(VEX_LEN_3A15_P_2 + 1)
#define VEX_LEN_3A17_P_2	(VEX_LEN_3A16_P_2 + 1)
#define VEX_LEN_3A18_P_2	(VEX_LEN_3A17_P_2 + 1)
#define VEX_LEN_3A19_P_2	(VEX_LEN_3A18_P_2 + 1)
#define VEX_LEN_3A20_P_2	(VEX_LEN_3A19_P_2 + 1)
#define VEX_LEN_3A21_P_2	(VEX_LEN_3A20_P_2 + 1)
#define VEX_LEN_3A22_P_2	(VEX_LEN_3A21_P_2 + 1)
#define VEX_LEN_3A41_P_2	(VEX_LEN_3A22_P_2 + 1)
#define VEX_LEN_3A42_P_2	(VEX_LEN_3A41_P_2 + 1)
#define VEX_LEN_3A4C_P_2	(VEX_LEN_3A42_P_2 + 1)
#define VEX_LEN_3A60_P_2	(VEX_LEN_3A4C_P_2 + 1)
#define VEX_LEN_3A61_P_2	(VEX_LEN_3A60_P_2 + 1)
#define VEX_LEN_3A62_P_2	(VEX_LEN_3A61_P_2 + 1)
#define VEX_LEN_3A63_P_2	(VEX_LEN_3A62_P_2 + 1)
#define VEX_LEN_3A6A_P_2	(VEX_LEN_3A63_P_2 + 1)
#define VEX_LEN_3A6B_P_2	(VEX_LEN_3A6A_P_2 + 1)
#define VEX_LEN_3A6E_P_2	(VEX_LEN_3A6B_P_2 + 1)
#define VEX_LEN_3A6F_P_2	(VEX_LEN_3A6E_P_2 + 1)
#define VEX_LEN_3A7A_P_2	(VEX_LEN_3A6F_P_2 + 1)
#define VEX_LEN_3A7B_P_2	(VEX_LEN_3A7A_P_2 + 1)
#define VEX_LEN_3A7E_P_2	(VEX_LEN_3A7B_P_2 + 1)
#define VEX_LEN_3A7F_P_2	(VEX_LEN_3A7E_P_2 + 1)
#define VEX_LEN_3ADF_P_2	(VEX_LEN_3A7F_P_2 + 1)

typedef void (*op_rtn) (int bytemode, int sizeflag);

struct dis386 {
  const char *name;
  struct
    {
      op_rtn rtn;
      int bytemode;
    } op[MAX_OPERANDS];
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
   'T' => print 'q' in 64bit mode and behave as 'P' otherwise
   'U' => print 'q' in 64bit mode and behave as 'Q' otherwise
   'V' => print 'q' in 64bit mode and behave as 'S' otherwise
   'W' => print 'b', 'w' or 'l' ('d' in Intel mode)
   'X' => print 's', 'd' depending on data16 prefix (for XMM)
   'Y' => 'q' if instruction has an REX 64bit overwrite prefix and
	  suffix_always is true.
   'Z' => print 'q' in 64bit mode and behave as 'L' otherwise
   '!' => change condition from true to false or from false to true.
   '%' => add 1 upper case letter to the macro.

   2 upper case letter macros:
   "XY" => print 'x' or 'y' if no register operands or suffix_always
	   is true.
   'LQ' => print 'l' ('d' in Intel mode) or 'q' for memory operand
	   or suffix_always is true

   Many of the above letters print nothing in Intel mode.  See "putop"
   for the details.

   Braces '{' and '}', and vertical bars '|', indicate alternative
   mnemonic strings for AT&T and Intel.  */

static const struct dis386 dis386[] = {
  /* 00 */
  { "addB",		{ Eb, Gb } },
  { "addS",		{ Ev, Gv } },
  { "addB",		{ Gb, Eb } },
  { "addS",		{ Gv, Ev } },
  { "addB",		{ AL, Ib } },
  { "addS",		{ eAX, Iv } },
  { X86_64_TABLE (X86_64_06) },
  { X86_64_TABLE (X86_64_07) },
  /* 08 */
  { "orB",		{ Eb, Gb } },
  { "orS",		{ Ev, Gv } },
  { "orB",		{ Gb, Eb } },
  { "orS",		{ Gv, Ev } },
  { "orB",		{ AL, Ib } },
  { "orS",		{ eAX, Iv } },
  { X86_64_TABLE (X86_64_0D) },
  { "(bad)",		{ XX } },	/* 0x0f extended opcode escape */
  /* 10 */
  { "adcB",		{ Eb, Gb } },
  { "adcS",		{ Ev, Gv } },
  { "adcB",		{ Gb, Eb } },
  { "adcS",		{ Gv, Ev } },
  { "adcB",		{ AL, Ib } },
  { "adcS",		{ eAX, Iv } },
  { X86_64_TABLE (X86_64_16) },
  { X86_64_TABLE (X86_64_17) },
  /* 18 */
  { "sbbB",		{ Eb, Gb } },
  { "sbbS",		{ Ev, Gv } },
  { "sbbB",		{ Gb, Eb } },
  { "sbbS",		{ Gv, Ev } },
  { "sbbB",		{ AL, Ib } },
  { "sbbS",		{ eAX, Iv } },
  { X86_64_TABLE (X86_64_1E) },
  { X86_64_TABLE (X86_64_1F) },
  /* 20 */
  { "andB",		{ Eb, Gb } },
  { "andS",		{ Ev, Gv } },
  { "andB",		{ Gb, Eb } },
  { "andS",		{ Gv, Ev } },
  { "andB",		{ AL, Ib } },
  { "andS",		{ eAX, Iv } },
  { "(bad)",		{ XX } },	/* SEG ES prefix */
  { X86_64_TABLE (X86_64_27) },
  /* 28 */
  { "subB",		{ Eb, Gb } },
  { "subS",		{ Ev, Gv } },
  { "subB",		{ Gb, Eb } },
  { "subS",		{ Gv, Ev } },
  { "subB",		{ AL, Ib } },
  { "subS",		{ eAX, Iv } },
  { "(bad)",		{ XX } },	/* SEG CS prefix */
  { X86_64_TABLE (X86_64_2F) },
  /* 30 */
  { "xorB",		{ Eb, Gb } },
  { "xorS",		{ Ev, Gv } },
  { "xorB",		{ Gb, Eb } },
  { "xorS",		{ Gv, Ev } },
  { "xorB",		{ AL, Ib } },
  { "xorS",		{ eAX, Iv } },
  { "(bad)",		{ XX } },	/* SEG SS prefix */
  { X86_64_TABLE (X86_64_37) },
  /* 38 */
  { "cmpB",		{ Eb, Gb } },
  { "cmpS",		{ Ev, Gv } },
  { "cmpB",		{ Gb, Eb } },
  { "cmpS",		{ Gv, Ev } },
  { "cmpB",		{ AL, Ib } },
  { "cmpS",		{ eAX, Iv } },
  { "(bad)",		{ XX } },	/* SEG DS prefix */
  { X86_64_TABLE (X86_64_3F) },
  /* 40 */
  { "inc{S|}",		{ RMeAX } },
  { "inc{S|}",		{ RMeCX } },
  { "inc{S|}",		{ RMeDX } },
  { "inc{S|}",		{ RMeBX } },
  { "inc{S|}",		{ RMeSP } },
  { "inc{S|}",		{ RMeBP } },
  { "inc{S|}",		{ RMeSI } },
  { "inc{S|}",		{ RMeDI } },
  /* 48 */
  { "dec{S|}",		{ RMeAX } },
  { "dec{S|}",		{ RMeCX } },
  { "dec{S|}",		{ RMeDX } },
  { "dec{S|}",		{ RMeBX } },
  { "dec{S|}",		{ RMeSP } },
  { "dec{S|}",		{ RMeBP } },
  { "dec{S|}",		{ RMeSI } },
  { "dec{S|}",		{ RMeDI } },
  /* 50 */
  { "pushV",		{ RMrAX } },
  { "pushV",		{ RMrCX } },
  { "pushV",		{ RMrDX } },
  { "pushV",		{ RMrBX } },
  { "pushV",		{ RMrSP } },
  { "pushV",		{ RMrBP } },
  { "pushV",		{ RMrSI } },
  { "pushV",		{ RMrDI } },
  /* 58 */
  { "popV",		{ RMrAX } },
  { "popV",		{ RMrCX } },
  { "popV",		{ RMrDX } },
  { "popV",		{ RMrBX } },
  { "popV",		{ RMrSP } },
  { "popV",		{ RMrBP } },
  { "popV",		{ RMrSI } },
  { "popV",		{ RMrDI } },
  /* 60 */
  { X86_64_TABLE (X86_64_60) },
  { X86_64_TABLE (X86_64_61) },
  { X86_64_TABLE (X86_64_62) },
  { X86_64_TABLE (X86_64_63) },
  { "(bad)",		{ XX } },	/* seg fs */
  { "(bad)",		{ XX } },	/* seg gs */
  { "(bad)",		{ XX } },	/* op size prefix */
  { "(bad)",		{ XX } },	/* adr size prefix */
  /* 68 */
  { "pushT",		{ Iq } },
  { "imulS",		{ Gv, Ev, Iv } },
  { "pushT",		{ sIb } },
  { "imulS",		{ Gv, Ev, sIb } },
  { "ins{b|}",		{ Ybr, indirDX } },
  { X86_64_TABLE (X86_64_6D) },
  { "outs{b|}",		{ indirDXr, Xb } },
  { X86_64_TABLE (X86_64_6F) },
  /* 70 */
  { "joH",		{ Jb, XX, cond_jump_flag } },
  { "jnoH",		{ Jb, XX, cond_jump_flag } },
  { "jbH",		{ Jb, XX, cond_jump_flag } },
  { "jaeH",		{ Jb, XX, cond_jump_flag } },
  { "jeH",		{ Jb, XX, cond_jump_flag } },
  { "jneH",		{ Jb, XX, cond_jump_flag } },
  { "jbeH",		{ Jb, XX, cond_jump_flag } },
  { "jaH",		{ Jb, XX, cond_jump_flag } },
  /* 78 */
  { "jsH",		{ Jb, XX, cond_jump_flag } },
  { "jnsH",		{ Jb, XX, cond_jump_flag } },
  { "jpH",		{ Jb, XX, cond_jump_flag } },
  { "jnpH",		{ Jb, XX, cond_jump_flag } },
  { "jlH",		{ Jb, XX, cond_jump_flag } },
  { "jgeH",		{ Jb, XX, cond_jump_flag } },
  { "jleH",		{ Jb, XX, cond_jump_flag } },
  { "jgH",		{ Jb, XX, cond_jump_flag } },
  /* 80 */
  { REG_TABLE (REG_80) },
  { REG_TABLE (REG_81) },
  { "(bad)",		{ XX } },
  { REG_TABLE (REG_82) },
  { "testB",		{ Eb, Gb } },
  { "testS",		{ Ev, Gv } },
  { "xchgB",		{ Eb, Gb } },
  { "xchgS",		{ Ev, Gv } },
  /* 88 */
  { "movB",		{ Eb, Gb } },
  { "movS",		{ Ev, Gv } },
  { "movB",		{ Gb, Eb } },
  { "movS",		{ Gv, Ev } },
  { "movD",		{ Sv, Sw } },
  { MOD_TABLE (MOD_8D) },
  { "movD",		{ Sw, Sv } },
  { REG_TABLE (REG_8F) },
  /* 90 */
  { PREFIX_TABLE (PREFIX_90) },
  { "xchgS",		{ RMeCX, eAX } },
  { "xchgS",		{ RMeDX, eAX } },
  { "xchgS",		{ RMeBX, eAX } },
  { "xchgS",		{ RMeSP, eAX } },
  { "xchgS",		{ RMeBP, eAX } },
  { "xchgS",		{ RMeSI, eAX } },
  { "xchgS",		{ RMeDI, eAX } },
  /* 98 */
  { "cW{t|}R",		{ XX } },
  { "cR{t|}O",		{ XX } },
  { X86_64_TABLE (X86_64_9A) },
  { "(bad)",		{ XX } },	/* fwait */
  { "pushfT",		{ XX } },
  { "popfT",		{ XX } },
  { "sahf",		{ XX } },
  { "lahf",		{ XX } },
  /* a0 */
  { "movB",		{ AL, Ob } },
  { "movS",		{ eAX, Ov } },
  { "movB",		{ Ob, AL } },
  { "movS",		{ Ov, eAX } },
  { "movs{b|}",		{ Ybr, Xb } },
  { "movs{R|}",		{ Yvr, Xv } },
  { "cmps{b|}",		{ Xb, Yb } },
  { "cmps{R|}",		{ Xv, Yv } },
  /* a8 */
  { "testB",		{ AL, Ib } },
  { "testS",		{ eAX, Iv } },
  { "stosB",		{ Ybr, AL } },
  { "stosS",		{ Yvr, eAX } },
  { "lodsB",		{ ALr, Xb } },
  { "lodsS",		{ eAXr, Xv } },
  { "scasB",		{ AL, Yb } },
  { "scasS",		{ eAX, Yv } },
  /* b0 */
  { "movB",		{ RMAL, Ib } },
  { "movB",		{ RMCL, Ib } },
  { "movB",		{ RMDL, Ib } },
  { "movB",		{ RMBL, Ib } },
  { "movB",		{ RMAH, Ib } },
  { "movB",		{ RMCH, Ib } },
  { "movB",		{ RMDH, Ib } },
  { "movB",		{ RMBH, Ib } },
  /* b8 */
  { "movS",		{ RMeAX, Iv64 } },
  { "movS",		{ RMeCX, Iv64 } },
  { "movS",		{ RMeDX, Iv64 } },
  { "movS",		{ RMeBX, Iv64 } },
  { "movS",		{ RMeSP, Iv64 } },
  { "movS",		{ RMeBP, Iv64 } },
  { "movS",		{ RMeSI, Iv64 } },
  { "movS",		{ RMeDI, Iv64 } },
  /* c0 */
  { REG_TABLE (REG_C0) },
  { REG_TABLE (REG_C1) },
  { "retT",		{ Iw } },
  { "retT",		{ XX } },
  { X86_64_TABLE (X86_64_C4) },
  { X86_64_TABLE (X86_64_C5) },
  { REG_TABLE (REG_C6) },
  { REG_TABLE (REG_C7) },
  /* c8 */
  { "enterT",		{ Iw, Ib } },
  { "leaveT",		{ XX } },
  { "Jret{|f}P",	{ Iw } },
  { "Jret{|f}P",	{ XX } },
  { "int3",		{ XX } },
  { "int",		{ Ib } },
  { X86_64_TABLE (X86_64_CE) },
  { "iretP",		{ XX } },
  /* d0 */
  { REG_TABLE (REG_D0) },
  { REG_TABLE (REG_D1) },
  { REG_TABLE (REG_D2) },
  { REG_TABLE (REG_D3) },
  { X86_64_TABLE (X86_64_D4) },
  { X86_64_TABLE (X86_64_D5) },
  { "(bad)",		{ XX } },
  { "xlat",		{ DSBX } },
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
  { "loopneFH",		{ Jb, XX, loop_jcxz_flag } },
  { "loopeFH",		{ Jb, XX, loop_jcxz_flag } },
  { "loopFH",		{ Jb, XX, loop_jcxz_flag } },
  { "jEcxzH",		{ Jb, XX, loop_jcxz_flag } },
  { "inB",		{ AL, Ib } },
  { "inG",		{ zAX, Ib } },
  { "outB",		{ Ib, AL } },
  { "outG",		{ Ib, zAX } },
  /* e8 */
  { "callT",		{ Jv } },
  { "jmpT",		{ Jv } },
  { X86_64_TABLE (X86_64_EA) },
  { "jmp",		{ Jb } },
  { "inB",		{ AL, indirDX } },
  { "inG",		{ zAX, indirDX } },
  { "outB",		{ indirDX, AL } },
  { "outG",		{ indirDX, zAX } },
  /* f0 */
  { "(bad)",		{ XX } },	/* lock prefix */
  { "icebp",		{ XX } },
  { "(bad)",		{ XX } },	/* repne */
  { "(bad)",		{ XX } },	/* repz */
  { "hlt",		{ XX } },
  { "cmc",		{ XX } },
  { REG_TABLE (REG_F6) },
  { REG_TABLE (REG_F7) },
  /* f8 */
  { "clc",		{ XX } },
  { "stc",		{ XX } },
  { "cli",		{ XX } },
  { "sti",		{ XX } },
  { "cld",		{ XX } },
  { "std",		{ XX } },
  { REG_TABLE (REG_FE) },
  { REG_TABLE (REG_FF) },
};

static const struct dis386 dis386_twobyte[] = {
  /* 00 */
  { REG_TABLE (REG_0F00 ) },
  { REG_TABLE (REG_0F01 ) },
  { "larS",		{ Gv, Ew } },
  { "lslS",		{ Gv, Ew } },
  { "(bad)",		{ XX } },
  { "syscall",		{ XX } },
  { "clts",		{ XX } },
  { "sysretP",		{ XX } },
  /* 08 */
  { "invd",		{ XX } },
  { "wbinvd",		{ XX } },
  { "(bad)",		{ XX } },
  { "ud2a",		{ XX } },
  { "(bad)",		{ XX } },
  { REG_TABLE (REG_0F0D) },
  { "femms",		{ XX } },
  { "",			{ MX, EM, OPSUF } }, /* See OP_3DNowSuffix.  */
  /* 10 */
  { PREFIX_TABLE (PREFIX_0F10) },
  { PREFIX_TABLE (PREFIX_0F11) },
  { PREFIX_TABLE (PREFIX_0F12) },
  { MOD_TABLE (MOD_0F13) },
  { "unpcklpX",		{ XM, EXx } },
  { "unpckhpX",		{ XM, EXx } },
  { PREFIX_TABLE (PREFIX_0F16) },
  { MOD_TABLE (MOD_0F17) },
  /* 18 */
  { REG_TABLE (REG_0F18) },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  { "nopQ",		{ Ev } },
  /* 20 */
  { MOD_TABLE (MOD_0F20) },
  { MOD_TABLE (MOD_0F21) },
  { MOD_TABLE (MOD_0F22) },
  { MOD_TABLE (MOD_0F23) },
  { MOD_TABLE (MOD_0F24) },
  { THREE_BYTE_TABLE (THREE_BYTE_0F25) },
  { MOD_TABLE (MOD_0F26) },
  { "(bad)",		{ XX } },
  /* 28 */
  { "movapX",		{ XM, EXx } },
  { "movapX",		{ EXx, XM } },
  { PREFIX_TABLE (PREFIX_0F2A) },
  { PREFIX_TABLE (PREFIX_0F2B) },
  { PREFIX_TABLE (PREFIX_0F2C) },
  { PREFIX_TABLE (PREFIX_0F2D) },
  { PREFIX_TABLE (PREFIX_0F2E) },
  { PREFIX_TABLE (PREFIX_0F2F) },
  /* 30 */
  { "wrmsr",		{ XX } },
  { "rdtsc",		{ XX } },
  { "rdmsr",		{ XX } },
  { "rdpmc",		{ XX } },
  { "sysenter",		{ XX } },
  { "sysexit",		{ XX } },
  { "(bad)",		{ XX } },
  { "getsec",		{ XX } },
  /* 38 */
  { THREE_BYTE_TABLE (THREE_BYTE_0F38) },
  { "(bad)",		{ XX } },
  { THREE_BYTE_TABLE (THREE_BYTE_0F3A) },
  { "(bad)",		{ XX } },
  { "(bad)",		{ XX } },
  { "(bad)",		{ XX } },
  { "(bad)",		{ XX } },
  { "(bad)",		{ XX } },
  /* 40 */
  { "cmovoS",		{ Gv, Ev } },
  { "cmovnoS",		{ Gv, Ev } },
  { "cmovbS",		{ Gv, Ev } },
  { "cmovaeS",		{ Gv, Ev } },
  { "cmoveS",		{ Gv, Ev } },
  { "cmovneS",		{ Gv, Ev } },
  { "cmovbeS",		{ Gv, Ev } },
  { "cmovaS",		{ Gv, Ev } },
  /* 48 */
  { "cmovsS",		{ Gv, Ev } },
  { "cmovnsS",		{ Gv, Ev } },
  { "cmovpS",		{ Gv, Ev } },
  { "cmovnpS",		{ Gv, Ev } },
  { "cmovlS",		{ Gv, Ev } },
  { "cmovgeS",		{ Gv, Ev } },
  { "cmovleS",		{ Gv, Ev } },
  { "cmovgS",		{ Gv, Ev } },
  /* 50 */
  { MOD_TABLE (MOD_0F51) },
  { PREFIX_TABLE (PREFIX_0F51) },
  { PREFIX_TABLE (PREFIX_0F52) },
  { PREFIX_TABLE (PREFIX_0F53) },
  { "andpX",		{ XM, EXx } },
  { "andnpX",		{ XM, EXx } },
  { "orpX",		{ XM, EXx } },
  { "xorpX",		{ XM, EXx } },
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
  { "packsswb",		{ MX, EM } },
  { "pcmpgtb",		{ MX, EM } },
  { "pcmpgtw",		{ MX, EM } },
  { "pcmpgtd",		{ MX, EM } },
  { "packuswb",		{ MX, EM } },
  /* 68 */
  { "punpckhbw",	{ MX, EM } },
  { "punpckhwd",	{ MX, EM } },
  { "punpckhdq",	{ MX, EM } },
  { "packssdw",		{ MX, EM } },
  { PREFIX_TABLE (PREFIX_0F6C) },
  { PREFIX_TABLE (PREFIX_0F6D) },
  { "movK",		{ MX, Edq } },
  { PREFIX_TABLE (PREFIX_0F6F) },
  /* 70 */
  { PREFIX_TABLE (PREFIX_0F70) },
  { REG_TABLE (REG_0F71) },
  { REG_TABLE (REG_0F72) },
  { REG_TABLE (REG_0F73) },
  { "pcmpeqb",		{ MX, EM } },
  { "pcmpeqw",		{ MX, EM } },
  { "pcmpeqd",		{ MX, EM } },
  { "emms",		{ XX } },
  /* 78 */
  { PREFIX_TABLE (PREFIX_0F78) },
  { PREFIX_TABLE (PREFIX_0F79) },
  { THREE_BYTE_TABLE (THREE_BYTE_0F7A) },
  { THREE_BYTE_TABLE (THREE_BYTE_0F7B) },
  { PREFIX_TABLE (PREFIX_0F7C) },
  { PREFIX_TABLE (PREFIX_0F7D) },
  { PREFIX_TABLE (PREFIX_0F7E) },
  { PREFIX_TABLE (PREFIX_0F7F) },
  /* 80 */
  { "joH",		{ Jv, XX, cond_jump_flag } },
  { "jnoH",		{ Jv, XX, cond_jump_flag } },
  { "jbH",		{ Jv, XX, cond_jump_flag } },
  { "jaeH",		{ Jv, XX, cond_jump_flag } },
  { "jeH",		{ Jv, XX, cond_jump_flag } },
  { "jneH",		{ Jv, XX, cond_jump_flag } },
  { "jbeH",		{ Jv, XX, cond_jump_flag } },
  { "jaH",		{ Jv, XX, cond_jump_flag } },
  /* 88 */
  { "jsH",		{ Jv, XX, cond_jump_flag } },
  { "jnsH",		{ Jv, XX, cond_jump_flag } },
  { "jpH",		{ Jv, XX, cond_jump_flag } },
  { "jnpH",		{ Jv, XX, cond_jump_flag } },
  { "jlH",		{ Jv, XX, cond_jump_flag } },
  { "jgeH",		{ Jv, XX, cond_jump_flag } },
  { "jleH",		{ Jv, XX, cond_jump_flag } },
  { "jgH",		{ Jv, XX, cond_jump_flag } },
  /* 90 */
  { "seto",		{ Eb } },
  { "setno",		{ Eb } },
  { "setb",		{ Eb } },
  { "setae",		{ Eb } },
  { "sete",		{ Eb } },
  { "setne",		{ Eb } },
  { "setbe",		{ Eb } },
  { "seta",		{ Eb } },
  /* 98 */
  { "sets",		{ Eb } },
  { "setns",		{ Eb } },
  { "setp",		{ Eb } },
  { "setnp",		{ Eb } },
  { "setl",		{ Eb } },
  { "setge",		{ Eb } },
  { "setle",		{ Eb } },
  { "setg",		{ Eb } },
  /* a0 */
  { "pushT",		{ fs } },
  { "popT",		{ fs } },
  { "cpuid",		{ XX } },
  { "btS",		{ Ev, Gv } },
  { "shldS",		{ Ev, Gv, Ib } },
  { "shldS",		{ Ev, Gv, CL } },
  { REG_TABLE (REG_0FA6) },
  { REG_TABLE (REG_0FA7) },
  /* a8 */
  { "pushT",		{ gs } },
  { "popT",		{ gs } },
  { "rsm",		{ XX } },
  { "btsS",		{ Ev, Gv } },
  { "shrdS",		{ Ev, Gv, Ib } },
  { "shrdS",		{ Ev, Gv, CL } },
  { REG_TABLE (REG_0FAE) },
  { "imulS",		{ Gv, Ev } },
  /* b0 */
  { "cmpxchgB",		{ Eb, Gb } },
  { "cmpxchgS",		{ Ev, Gv } },
  { MOD_TABLE (MOD_0FB2) },
  { "btrS",		{ Ev, Gv } },
  { MOD_TABLE (MOD_0FB4) },
  { MOD_TABLE (MOD_0FB5) },
  { "movz{bR|x}",	{ Gv, Eb } },
  { "movz{wR|x}",	{ Gv, Ew } }, /* yes, there really is movzww ! */
  /* b8 */
  { PREFIX_TABLE (PREFIX_0FB8) },
  { "ud2b",		{ XX } },
  { REG_TABLE (REG_0FBA) },
  { "btcS",		{ Ev, Gv } },
  { "bsfS",		{ Gv, Ev } },
  { PREFIX_TABLE (PREFIX_0FBD) },
  { "movs{bR|x}",	{ Gv, Eb } },
  { "movs{wR|x}",	{ Gv, Ew } }, /* yes, there really is movsww ! */
  /* c0 */
  { "xaddB",		{ Eb, Gb } },
  { "xaddS",		{ Ev, Gv } },
  { PREFIX_TABLE (PREFIX_0FC2) },
  { PREFIX_TABLE (PREFIX_0FC3) },
  { "pinsrw",		{ MX, Edqw, Ib } },
  { "pextrw",		{ Gdq, MS, Ib } },
  { "shufpX",		{ XM, EXx, Ib } },
  { REG_TABLE (REG_0FC7) },
  /* c8 */
  { "bswap",		{ RMeAX } },
  { "bswap",		{ RMeCX } },
  { "bswap",		{ RMeDX } },
  { "bswap",		{ RMeBX } },
  { "bswap",		{ RMeSP } },
  { "bswap",		{ RMeBP } },
  { "bswap",		{ RMeSI } },
  { "bswap",		{ RMeDI } },
  /* d0 */
  { PREFIX_TABLE (PREFIX_0FD0) },
  { "psrlw",		{ MX, EM } },
  { "psrld",		{ MX, EM } },
  { "psrlq",		{ MX, EM } },
  { "paddq",		{ MX, EM } },
  { "pmullw",		{ MX, EM } },
  { PREFIX_TABLE (PREFIX_0FD6) },
  { MOD_TABLE (MOD_0FD7) },
  /* d8 */
  { "psubusb",		{ MX, EM } },
  { "psubusw",		{ MX, EM } },
  { "pminub",		{ MX, EM } },
  { "pand",		{ MX, EM } },
  { "paddusb",		{ MX, EM } },
  { "paddusw",		{ MX, EM } },
  { "pmaxub",		{ MX, EM } },
  { "pandn",		{ MX, EM } },
  /* e0 */
  { "pavgb",		{ MX, EM } },
  { "psraw",		{ MX, EM } },
  { "psrad",		{ MX, EM } },
  { "pavgw",		{ MX, EM } },
  { "pmulhuw",		{ MX, EM } },
  { "pmulhw",		{ MX, EM } },
  { PREFIX_TABLE (PREFIX_0FE6) },
  { PREFIX_TABLE (PREFIX_0FE7) },
  /* e8 */
  { "psubsb",		{ MX, EM } },
  { "psubsw",		{ MX, EM } },
  { "pminsw",		{ MX, EM } },
  { "por",		{ MX, EM } },
  { "paddsb",		{ MX, EM } },
  { "paddsw",		{ MX, EM } },
  { "pmaxsw",		{ MX, EM } },
  { "pxor",		{ MX, EM } },
  /* f0 */
  { PREFIX_TABLE (PREFIX_0FF0) },
  { "psllw",		{ MX, EM } },
  { "pslld",		{ MX, EM } },
  { "psllq",		{ MX, EM } },
  { "pmuludq",		{ MX, EM } },
  { "pmaddwd",		{ MX, EM } },
  { "psadbw",		{ MX, EM } },
  { PREFIX_TABLE (PREFIX_0FF7) },
  /* f8 */
  { "psubb",		{ MX, EM } },
  { "psubw",		{ MX, EM } },
  { "psubd",		{ MX, EM } },
  { "psubq",		{ MX, EM } },
  { "paddb",		{ MX, EM } },
  { "paddw",		{ MX, EM } },
  { "paddd",		{ MX, EM } },
  { "(bad)",		{ XX } },
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
static char scratchbuf[100];
static unsigned char *start_codep;
static unsigned char *insn_codep;
static unsigned char *codep;
static const char *lock_prefix;
static const char *data_prefix;
static const char *addr_prefix;
static const char *repz_prefix;
static const char *repnz_prefix;
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
    int register_specifier;
    int length;
    int prefix;
    int w;
  }
vex;
static unsigned char need_vex;
static unsigned char need_vex_reg;
static unsigned char vex_w_done;

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

static const struct dis386 reg_table[][8] = {
  /* REG_80 */
  {
    { "addA",	{ Eb, Ib } },
    { "orA",	{ Eb, Ib } },
    { "adcA",	{ Eb, Ib } },
    { "sbbA",	{ Eb, Ib } },
    { "andA",	{ Eb, Ib } },
    { "subA",	{ Eb, Ib } },
    { "xorA",	{ Eb, Ib } },
    { "cmpA",	{ Eb, Ib } },
  },
  /* REG_81 */
  {
    { "addQ",	{ Ev, Iv } },
    { "orQ",	{ Ev, Iv } },
    { "adcQ",	{ Ev, Iv } },
    { "sbbQ",	{ Ev, Iv } },
    { "andQ",	{ Ev, Iv } },
    { "subQ",	{ Ev, Iv } },
    { "xorQ",	{ Ev, Iv } },
    { "cmpQ",	{ Ev, Iv } },
  },
  /* REG_82 */
  {
    { "addQ",	{ Ev, sIb } },
    { "orQ",	{ Ev, sIb } },
    { "adcQ",	{ Ev, sIb } },
    { "sbbQ",	{ Ev, sIb } },
    { "andQ",	{ Ev, sIb } },
    { "subQ",	{ Ev, sIb } },
    { "xorQ",	{ Ev, sIb } },
    { "cmpQ",	{ Ev, sIb } },
  },
  /* REG_8F */
  {
    { "popU",	{ stackEv } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* REG_C0 */
  {
    { "rolA",	{ Eb, Ib } },
    { "rorA",	{ Eb, Ib } },
    { "rclA",	{ Eb, Ib } },
    { "rcrA",	{ Eb, Ib } },
    { "shlA",	{ Eb, Ib } },
    { "shrA",	{ Eb, Ib } },
    { "(bad)",	{ XX } },
    { "sarA",	{ Eb, Ib } },
  },
  /* REG_C1 */
  {
    { "rolQ",	{ Ev, Ib } },
    { "rorQ",	{ Ev, Ib } },
    { "rclQ",	{ Ev, Ib } },
    { "rcrQ",	{ Ev, Ib } },
    { "shlQ",	{ Ev, Ib } },
    { "shrQ",	{ Ev, Ib } },
    { "(bad)",	{ XX } },
    { "sarQ",	{ Ev, Ib } },
  },
  /* REG_C6 */
  {
    { "movA",	{ Eb, Ib } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* REG_C7 */
  {
    { "movQ",	{ Ev, Iv } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",  { XX } },
  },
  /* REG_D0 */
  {
    { "rolA",	{ Eb, I1 } },
    { "rorA",	{ Eb, I1 } },
    { "rclA",	{ Eb, I1 } },
    { "rcrA",	{ Eb, I1 } },
    { "shlA",	{ Eb, I1 } },
    { "shrA",	{ Eb, I1 } },
    { "(bad)",	{ XX } },
    { "sarA",	{ Eb, I1 } },
  },
  /* REG_D1 */
  {
    { "rolQ",	{ Ev, I1 } },
    { "rorQ",	{ Ev, I1 } },
    { "rclQ",	{ Ev, I1 } },
    { "rcrQ",	{ Ev, I1 } },
    { "shlQ",	{ Ev, I1 } },
    { "shrQ",	{ Ev, I1 } },
    { "(bad)",	{ XX } },
    { "sarQ",	{ Ev, I1 } },
  },
  /* REG_D2 */
  {
    { "rolA",	{ Eb, CL } },
    { "rorA",	{ Eb, CL } },
    { "rclA",	{ Eb, CL } },
    { "rcrA",	{ Eb, CL } },
    { "shlA",	{ Eb, CL } },
    { "shrA",	{ Eb, CL } },
    { "(bad)",	{ XX } },
    { "sarA",	{ Eb, CL } },
  },
  /* REG_D3 */
  {
    { "rolQ",	{ Ev, CL } },
    { "rorQ",	{ Ev, CL } },
    { "rclQ",	{ Ev, CL } },
    { "rcrQ",	{ Ev, CL } },
    { "shlQ",	{ Ev, CL } },
    { "shrQ",	{ Ev, CL } },
    { "(bad)",	{ XX } },
    { "sarQ",	{ Ev, CL } },
  },
  /* REG_F6 */
  {
    { "testA",	{ Eb, Ib } },
    { "(bad)",	{ XX } },
    { "notA",	{ Eb } },
    { "negA",	{ Eb } },
    { "mulA",	{ Eb } },	/* Don't print the implicit %al register,  */
    { "imulA",	{ Eb } },	/* to distinguish these opcodes from other */
    { "divA",	{ Eb } },	/* mul/imul opcodes.  Do the same for div  */
    { "idivA",	{ Eb } },	/* and idiv for consistency.		   */
  },
  /* REG_F7 */
  {
    { "testQ",	{ Ev, Iv } },
    { "(bad)",	{ XX } },
    { "notQ",	{ Ev } },
    { "negQ",	{ Ev } },
    { "mulQ",	{ Ev } },	/* Don't print the implicit register.  */
    { "imulQ",	{ Ev } },
    { "divQ",	{ Ev } },
    { "idivQ",	{ Ev } },
  },
  /* REG_FE */
  {
    { "incA",	{ Eb } },
    { "decA",	{ Eb } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* REG_FF */
  {
    { "incQ",	{ Ev } },
    { "decQ",	{ Ev } },
    { "callT",	{ indirEv } },
    { "JcallT",	{ indirEp } },
    { "jmpT",	{ indirEv } },
    { "JjmpT",	{ indirEp } },
    { "pushU",	{ stackEv } },
    { "(bad)",	{ XX } },
  },
  /* REG_0F00 */
  {
    { "sldtD",	{ Sv } },
    { "strD",	{ Sv } },
    { "lldt",	{ Ew } },
    { "ltr",	{ Ew } },
    { "verr",	{ Ew } },
    { "verw",	{ Ew } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* REG_0F01 */
  {
    { MOD_TABLE (MOD_0F01_REG_0) },
    { MOD_TABLE (MOD_0F01_REG_1) },
    { MOD_TABLE (MOD_0F01_REG_2) },
    { MOD_TABLE (MOD_0F01_REG_3) },
    { "smswD",	{ Sv } },
    { "(bad)",	{ XX } },
    { "lmsw",	{ Ew } },
    { MOD_TABLE (MOD_0F01_REG_7) },
  },
  /* REG_0F0D */
  {
    { "prefetch",	{ Eb } },
    { "prefetchw",	{ Eb } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* REG_0F18 */
  {
    { MOD_TABLE (MOD_0F18_REG_0) },
    { MOD_TABLE (MOD_0F18_REG_1) },
    { MOD_TABLE (MOD_0F18_REG_2) },
    { MOD_TABLE (MOD_0F18_REG_3) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* REG_0F71 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F71_REG_2) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F71_REG_4) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F71_REG_6) },
    { "(bad)",	{ XX } },
  },
  /* REG_0F72 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F72_REG_2) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F72_REG_4) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F72_REG_6) },
    { "(bad)",	{ XX } },
  },
  /* REG_0F73 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F73_REG_2) },
    { MOD_TABLE (MOD_0F73_REG_3) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F73_REG_6) },
    { MOD_TABLE (MOD_0F73_REG_7) },
  },
  /* REG_0FA6 */
  {
    { "montmul",	{ { OP_0f07, 0 } } },
    { "xsha1",		{ { OP_0f07, 0 } } },
    { "xsha256",	{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
  },
  /* REG_0FA7 */
  {
    { "xstore-rng",	{ { OP_0f07, 0 } } },
    { "xcrypt-ecb",	{ { OP_0f07, 0 } } },
    { "xcrypt-cbc",	{ { OP_0f07, 0 } } },
    { "xcrypt-ctr",	{ { OP_0f07, 0 } } },
    { "xcrypt-cfb",	{ { OP_0f07, 0 } } },
    { "xcrypt-ofb",	{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
    { "(bad)",		{ { OP_0f07, 0 } } },
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
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "btQ",	{ Ev, Ib } },
    { "btsQ",	{ Ev, Ib } },
    { "btrQ",	{ Ev, Ib } },
    { "btcQ",	{ Ev, Ib } },
  },
  /* REG_0FC7 */
  {
    { "(bad)",	{ XX } },
    { "cmpxchg8b", { { CMPXCHG8B_Fixup, q_mode } } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0FC7_REG_6) },
    { MOD_TABLE (MOD_0FC7_REG_7) },
  },
  /* REG_VEX_71 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_71_REG_2) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_71_REG_4) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_71_REG_6) },
    { "(bad)",	{ XX } },
  },
  /* REG_VEX_72 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_72_REG_2) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_72_REG_4) },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_72_REG_6) },
    { "(bad)",	{ XX } },
  },
  /* REG_VEX_73 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_73_REG_2) },
    { MOD_TABLE (MOD_VEX_73_REG_3) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_73_REG_6) },
    { MOD_TABLE (MOD_VEX_73_REG_7) },
  },
  /* REG_VEX_AE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_AE_REG_2) },
    { MOD_TABLE (MOD_VEX_AE_REG_3) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
};

static const struct dis386 prefix_table[][4] = {
  /* PREFIX_90 */
  {
    { "xchgS", { { NOP_Fixup1, eAX_reg }, { NOP_Fixup2, eAX_reg } } },
    { "pause", { XX } },
    { "xchgS", { { NOP_Fixup1, eAX_reg }, { NOP_Fixup2, eAX_reg } } },
    { "(bad)", { XX } },
  },

  /* PREFIX_0F10 */
  {
    { "movups",	{ XM, EXx } },
    { "movss",	{ XM, EXd } },
    { "movupd",	{ XM, EXx } },
    { "movsd",	{ XM, EXq } },
  },

  /* PREFIX_0F11 */
  {
    { "movups",	{ EXx, XM } },
    { "movss",	{ EXd, XM } },
    { "movupd",	{ EXx, XM } },
    { "movsd",	{ EXq, XM } },
  },

  /* PREFIX_0F12 */
  {
    { MOD_TABLE (MOD_0F12_PREFIX_0) },
    { "movsldup", { XM, EXx } },
    { "movlpd",	{ XM, EXq } },
    { "movddup", { XM, EXq } },
  },

  /* PREFIX_0F16 */
  {
    { MOD_TABLE (MOD_0F16_PREFIX_0) },
    { "movshdup", { XM, EXx } },
    { "movhpd",	{ XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F2A */
  {
    { "cvtpi2ps", { XM, EMCq } },
    { "cvtsi2ss%LQ", { XM, Ev } },
    { "cvtpi2pd", { XM, EMCq } },
    { "cvtsi2sd%LQ", { XM, Ev } },
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
    { "cvttps2pi", { MXC, EXq } },
    { "cvttss2siY", { Gv, EXd } },
    { "cvttpd2pi", { MXC, EXx } },
    { "cvttsd2siY", { Gv, EXq } },
  },

  /* PREFIX_0F2D */
  {
    { "cvtps2pi", { MXC, EXq } },
    { "cvtss2siY", { Gv, EXd } },
    { "cvtpd2pi", { MXC, EXx } },
    { "cvtsd2siY", { Gv, EXq } },
  },

  /* PREFIX_0F2E */
  {
    { "ucomiss",{ XM, EXd } }, 
    { "(bad)",	{ XX } },
    { "ucomisd",{ XM, EXq } }, 
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F2F */
  {
    { "comiss",	{ XM, EXd } },
    { "(bad)",	{ XX } },
    { "comisd",	{ XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F51 */
  {
    { "sqrtps", { XM, EXx } },
    { "sqrtss", { XM, EXd } },
    { "sqrtpd", { XM, EXx } },
    { "sqrtsd",	{ XM, EXq } },
  },

  /* PREFIX_0F52 */
  {
    { "rsqrtps",{ XM, EXx } },
    { "rsqrtss",{ XM, EXd } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F53 */
  {
    { "rcpps",	{ XM, EXx } },
    { "rcpss",	{ XM, EXd } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F58 */
  {
    { "addps", { XM, EXx } },
    { "addss", { XM, EXd } },
    { "addpd", { XM, EXx } },
    { "addsd", { XM, EXq } },
  },

  /* PREFIX_0F59 */
  {
    { "mulps",	{ XM, EXx } },
    { "mulss",	{ XM, EXd } },
    { "mulpd",	{ XM, EXx } },
    { "mulsd",	{ XM, EXq } },
  },

  /* PREFIX_0F5A */
  {
    { "cvtps2pd", { XM, EXq } },
    { "cvtss2sd", { XM, EXd } },
    { "cvtpd2ps", { XM, EXx } },
    { "cvtsd2ss", { XM, EXq } },
  },

  /* PREFIX_0F5B */
  {
    { "cvtdq2ps", { XM, EXx } },
    { "cvttps2dq", { XM, EXx } },
    { "cvtps2dq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F5C */
  {
    { "subps",	{ XM, EXx } },
    { "subss",	{ XM, EXd } },
    { "subpd",	{ XM, EXx } },
    { "subsd",	{ XM, EXq } },
  },

  /* PREFIX_0F5D */
  {
    { "minps",	{ XM, EXx } },
    { "minss",	{ XM, EXd } },
    { "minpd",	{ XM, EXx } },
    { "minsd",	{ XM, EXq } },
  },

  /* PREFIX_0F5E */
  {
    { "divps",	{ XM, EXx } },
    { "divss",	{ XM, EXd } },
    { "divpd",	{ XM, EXx } },
    { "divsd",	{ XM, EXq } },
  },

  /* PREFIX_0F5F */
  {
    { "maxps",	{ XM, EXx } },
    { "maxss",	{ XM, EXd } },
    { "maxpd",	{ XM, EXx } },
    { "maxsd",	{ XM, EXq } },
  },

  /* PREFIX_0F60 */
  {
    { "punpcklbw",{ MX, EMd } },
    { "(bad)",	{ XX } },
    { "punpcklbw",{ MX, EMx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F61 */
  {
    { "punpcklwd",{ MX, EMd } },
    { "(bad)",	{ XX } },
    { "punpcklwd",{ MX, EMx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F62 */
  {
    { "punpckldq",{ MX, EMd } },
    { "(bad)",	{ XX } },
    { "punpckldq",{ MX, EMx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F6C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "punpcklqdq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F6D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "punpckhqdq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F6F */
  {
    { "movq",	{ MX, EM } },
    { "movdqu",	{ XM, EXx } },
    { "movdqa",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F70 */
  {
    { "pshufw",	{ MX, EM, Ib } },
    { "pshufhw",{ XM, EXx, Ib } },
    { "pshufd",	{ XM, EXx, Ib } },
    { "pshuflw",{ XM, EXx, Ib } },
  },

  /* PREFIX_0F73_REG_3 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "psrldq",	{ XS, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F73_REG_7 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pslldq",	{ XS, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F78 */
  {
    {"vmread",	{ Em, Gm } },
    {"(bad)",	{ XX } },
    {"extrq",	{ XS, Ib, Ib } },
    {"insertq",	{ XM, XS, Ib, Ib } },
  },

  /* PREFIX_0F79 */
  {
    {"vmwrite",	{ Gm, Em } },
    {"(bad)",	{ XX } },
    {"extrq",	{ XM, XS } },
    {"insertq",	{ XM, XS } },
  },

  /* PREFIX_0F7C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "haddpd",	{ XM, EXx } },
    { "haddps",	{ XM, EXx } },
  },

  /* PREFIX_0F7D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "hsubpd",	{ XM, EXx } },
    { "hsubps",	{ XM, EXx } },
  },

  /* PREFIX_0F7E */
  {
    { "movK",	{ Edq, MX } },
    { "movq",	{ XM, EXq } },
    { "movK",	{ Edq, XM } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F7F */
  {
    { "movq",	{ EM, MX } },
    { "movdqu",	{ EXx, XM } },
    { "movdqa",	{ EXx, XM } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0FB8 */
  {
    { "(bad)", { XX } },
    { "popcntS", { Gv, Ev } },
    { "(bad)", { XX } },
    { "(bad)", { XX } },
  },

  /* PREFIX_0FBD */
  {
    { "bsrS",	{ Gv, Ev } },
    { "lzcntS",	{ Gv, Ev } },
    { "bsrS",	{ Gv, Ev } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0FC2 */
  {
    { "cmpps",	{ XM, EXx, CMP } },
    { "cmpss",	{ XM, EXd, CMP } },
    { "cmppd",	{ XM, EXx, CMP } },
    { "cmpsd",	{ XM, EXq, CMP } },
  },

  /* PREFIX_0FC3 */
  {
    { "movntiS", { Ma, Gv } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0FC7_REG_6 */
  {
    { "vmptrld",{ Mq } },
    { "vmxon",	{ Mq } },
    { "vmclear",{ Mq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0FD0 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "addsubpd", { XM, EXx } },
    { "addsubps", { XM, EXx } },
  },

  /* PREFIX_0FD6 */
  {
    { "(bad)",	{ XX } },
    { "movq2dq",{ XM, MS } },
    { "movq",	{ EXq, XM } },
    { "movdq2q",{ MX, XS } },
  },

  /* PREFIX_0FE6 */
  {
    { "(bad)",	{ XX } },
    { "cvtdq2pd", { XM, EXq } },
    { "cvttpd2dq", { XM, EXx } },
    { "cvtpd2dq", { XM, EXx } },
  },

  /* PREFIX_0FE7 */
  {
    { "movntq",	{ Mq, MX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0FE7_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0FF0 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0FF0_PREFIX_3) },
  },

  /* PREFIX_0FF7 */
  {
    { "maskmovq", { MX, MS } },
    { "(bad)",	{ XX } },
    { "maskmovdqu", { XM, XS } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3810 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pblendvb", { XM, EXx, XMM0 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3814 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "blendvps", { XM, EXx, XMM0 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3815 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "blendvpd", { XM, EXx, XMM0 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3817 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "ptest",  { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3820 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxbw", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3821 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxbd", { XM, EXd } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3822 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxbq", { XM, EXw } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3823 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxwd", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3824 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxwq", { XM, EXd } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3825 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovsxdq", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3828 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmuldq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3829 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpeqq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F382A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_0F382A_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F382B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "packusdw", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3830 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxbw", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3831 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxbd", { XM, EXd } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3832 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxbq", { XM, EXw } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3833 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxwd", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3834 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxwq", { XM, EXd } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3835 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmovzxdq", { XM, EXq } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3837 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpgtq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3838 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pminsb",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3839 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pminsd",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pminuw",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pminud",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmaxsb",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmaxsd",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmaxuw", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F383F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmaxud", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3840 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pmulld", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3841 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "phminposuw", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3880 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "invept",	{ Gm, Mo } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3881 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "invvpid", { Gm, Mo } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38DB */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aesimc", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38DC */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aesenc", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38DD */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aesenclast", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38DE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aesdec", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38DF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aesdeclast", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F38F0 */
  {
    { "movbeS",	{ Gv, { MOVBE_Fixup, v_mode } } },
    { "(bad)",	{ XX } },
    { "movbeS",	{ Gv, { MOVBE_Fixup, v_mode } } },
    { "crc32",	{ Gdq, { CRC32_Fixup, b_mode } } },	
  },

  /* PREFIX_0F38F1 */
  {
    { "movbeS",	{ { MOVBE_Fixup, v_mode }, Gv } },
    { "(bad)",	{ XX } },
    { "movbeS",	{ { MOVBE_Fixup, v_mode }, Gv } },
    { "crc32",	{ Gdq, { CRC32_Fixup, v_mode } } },	
  },

  /* PREFIX_0F3A08 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "roundps", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A09 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "roundpd", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A0A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "roundss", { XM, EXd, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A0B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "roundsd", { XM, EXq, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A0C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "blendps", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A0D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "blendpd", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A0E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pblendw", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A14 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pextrb",	{ Edqb, XM, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A15 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pextrw",	{ Edqw, XM, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A16 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pextrK",	{ Edq, XM, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A17 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "extractps", { Edqd, XM, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A20 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pinsrb",	{ XM, Edqb, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A21 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "insertps", { XM, EXd, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A22 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pinsrK",	{ XM, Edq, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A40 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "dpps",	{ XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A41 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "dppd",	{ XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A42 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "mpsadbw", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A44 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pclmulqdq", { XM, EXx, PCLMUL } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A60 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpestrm", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A61 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpestri", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A62 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpistrm", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3A63 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "pcmpistri", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_0F3ADF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "aeskeygenassist", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_10 */
  {
    { "vmovups", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_10_P_1) },
    { "vmovupd", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_10_P_3) },
  },

  /* PREFIX_VEX_11 */
  {
    { "vmovups", { EXx, XM } },
    { VEX_LEN_TABLE (VEX_LEN_11_P_1) },
    { "vmovupd", { EXx, XM } },
    { VEX_LEN_TABLE (VEX_LEN_11_P_3) },
  },

  /* PREFIX_VEX_12 */
  {
    { MOD_TABLE (MOD_VEX_12_PREFIX_0) },
    { "vmovsldup", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_12_P_2) },
    { "vmovddup", { XM, EXymmq } },
  },

  /* PREFIX_VEX_16 */
  {
    { MOD_TABLE (MOD_VEX_16_PREFIX_0) },
    { "vmovshdup", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_16_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_2A */
  {
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2A_P_1) },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2A_P_3) },
  },

  /* PREFIX_VEX_2C */
  {
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2C_P_1) },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2C_P_3) },
  },

  /* PREFIX_VEX_2D */
  {
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2D_P_1) },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2D_P_3) },
  },

  /* PREFIX_VEX_2E */
  {
    { VEX_LEN_TABLE (VEX_LEN_2E_P_0) },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_2F */
  {
    { VEX_LEN_TABLE (VEX_LEN_2F_P_0) },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_2F_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_51 */
  {
    { "vsqrtps", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_51_P_1) },
    { "vsqrtpd", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_51_P_3) },
  },

  /* PREFIX_VEX_52 */
  {
    { "vrsqrtps", { XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_52_P_1) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_53 */
  {
    { "vrcpps",	{ XM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_53_P_1) },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_58 */
  {
    { "vaddps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_58_P_1) },
    { "vaddpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_58_P_3) },
  },

  /* PREFIX_VEX_59 */
  {
    { "vmulps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_59_P_1) },
    { "vmulpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_59_P_3) },
  },

  /* PREFIX_VEX_5A */
  {
    { "vcvtps2pd", { XM, EXxmmq } },
    { VEX_LEN_TABLE (VEX_LEN_5A_P_1) },
    { "vcvtpd2ps%XY", { XMM, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5A_P_3) },
  },

  /* PREFIX_VEX_5B */
  {
    { "vcvtdq2ps", { XM, EXx } },
    { "vcvttps2dq", { XM, EXx } },
    { "vcvtps2dq", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_5C */
  {
    { "vsubps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5C_P_1) },
    { "vsubpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5C_P_3) },
  },

  /* PREFIX_VEX_5D */
  {
    { "vminps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5D_P_1) },
    { "vminpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5D_P_3) },
  },

  /* PREFIX_VEX_5E */
  {
    { "vdivps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5E_P_1) },
    { "vdivpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5E_P_3) },
  },

  /* PREFIX_VEX_5F */
  {
    { "vmaxps",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5F_P_1) },
    { "vmaxpd",	{ XM, Vex, EXx } },
    { VEX_LEN_TABLE (VEX_LEN_5F_P_3) },
  },

  /* PREFIX_VEX_60 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_60_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_61 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_61_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_62 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_62_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_63 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_63_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_64 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_64_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_65 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_65_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_66 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_66_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_67 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_67_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_68 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_68_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_69 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_69_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_6A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_6B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_6C_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_6D_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_6E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_6F */
  {
    { "(bad)",	{ XX } },
    { "vmovdqu", { XM, EXx } },
    { "vmovdqa", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_70 */
  {
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_70_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_70_P_2) },
    { VEX_LEN_TABLE (VEX_LEN_70_P_3) },
  },

  /* PREFIX_VEX_71_REG_2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_71_R_2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_71_REG_4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_71_R_4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_71_REG_6 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_71_R_6_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_72_REG_2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_72_R_2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_72_REG_4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_72_R_4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_72_REG_6 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_72_R_6_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_73_REG_2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_73_R_2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_73_REG_3 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_73_R_3_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_73_REG_6 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_73_R_6_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_73_REG_7 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_73_R_7_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_74 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_74_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_75 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_75_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_76 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_76_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_77 */
  {
    { "",	{ VZERO } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_7C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vhaddpd", { XM, Vex, EXx } },
    { "vhaddps", { XM, Vex, EXx } },
  },

  /* PREFIX_VEX_7D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vhsubpd", { XM, Vex, EXx } },
    { "vhsubps", { XM, Vex, EXx } },
  },

  /* PREFIX_VEX_7E */
  {
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_7E_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_7E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_7F */
  {
    { "(bad)",	{ XX } },
    { "vmovdqu", { EXx, XM } },
    { "vmovdqa", { EXx, XM } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_C2 */
  {
    { "vcmpps",	{ XM, Vex, EXx, VCMP } },
    { VEX_LEN_TABLE (VEX_LEN_C2_P_1) },
    { "vcmppd",	{ XM, Vex, EXx, VCMP } },
    { VEX_LEN_TABLE (VEX_LEN_C2_P_3) },
  },

  /* PREFIX_VEX_C4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_C4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_C5 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_C5_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D0 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vaddsubpd", { XM, Vex, EXx } },
    { "vaddsubps", { XM, Vex, EXx } },
  },

  /* PREFIX_VEX_D1 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D1_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D3 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D3_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D5 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D5_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D6 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D6_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D7 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_D7_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D8 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D8_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_D9 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D9_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DA */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DA_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DB */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DB_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DC */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DC_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DD */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DD_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DE_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_DF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_DF_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E0 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E0_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E1 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E1_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E3 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E3_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E5 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E5_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E6 */
  {
    { "(bad)",	{ XX } },
    { "vcvtdq2pd", { XM, EXxmmq } },
    { "vcvttpd2dq%XY", { XMM, EXx } },
    { "vcvtpd2dq%XY", { XMM, EXx } },
  },

  /* PREFIX_VEX_E7 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_E7_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E8 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E8_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_E9 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_E9_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_EA */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_EA_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_EB */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_EB_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_EC */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_EC_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_ED */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_ED_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_EE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_EE_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_EF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_EF_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F0 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_F0_PREFIX_3) },
  },

  /* PREFIX_VEX_F1 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F1_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F2 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F2_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F3 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F3_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F4 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F4_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F5 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F5_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F6 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F6_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F7 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F7_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F8 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F8_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_F9 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_F9_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_FA */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_FA_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_FB */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_FB_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_FC */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_FC_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_FD */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_FD_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_FE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_FE_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3800 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3800_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3801 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3801_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3802 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3802_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3803 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3803_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3804 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3804_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3805 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3805_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3806 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3806_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3807 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3807_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3808 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3808_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3809 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3809_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_380A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_380B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermilps", { XM, Vex, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermilpd", { XM, Vex, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vtestps", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_380F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vtestpd", { XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3817 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vptest",	{ XM, EXx } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3818 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_3818_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3819 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_3819_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_381A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_381A_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_381C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_381C_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_381D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_381D_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_381E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_381E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3820 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3820_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3821 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3821_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3822 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3822_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3823 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3823_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3824 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3824_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3825 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3825_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3828 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3828_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3829 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3829_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { MOD_TABLE (MOD_VEX_382A_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_382B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
     { MOD_TABLE (MOD_VEX_382C_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
     { MOD_TABLE (MOD_VEX_382D_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
     { MOD_TABLE (MOD_VEX_382E_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_382F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
     { MOD_TABLE (MOD_VEX_382F_PREFIX_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3830 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3830_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3831 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3831_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3832 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3832_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3833 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3833_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3834 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3834_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3835 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3835_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3837 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3837_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3838 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3838_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3839 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3839_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383C_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383D_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_383F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_383F_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3840 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3840_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3841 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3841_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_38DB */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_38DB_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_38DC */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_38DC_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_38DD */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_38DD_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_38DE */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_38DE_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_38DF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_38DF_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A04 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermilps", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A05 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermilpd", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A06 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A06_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A08 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vroundps", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A09 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vroundpd", { XM, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A0A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A0B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vblendps", { XM, Vex, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vblendpd", { XM, Vex, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A0E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A0F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A0F_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A14 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A14_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A15 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A15_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A16 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A16_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A17 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A17_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A18 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A18_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A19 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A19_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A20 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A20_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A21 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A21_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A22 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A22_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A40 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vdpps",	{ XM, Vex, EXx, Ib } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A41 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A41_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A42 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A42_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A48 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermil2ps", { XMVexW, Vex, EXVexImmW, EXVexImmW, VPERMIL2 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A49 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vpermil2pd", { XMVexW, Vex, EXVexImmW, EXVexImmW, VPERMIL2 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A4A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vblendvps", { XM, Vex, EXx, XMVexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A4B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vblendvpd", { XM, Vex, EXx, XMVexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A4C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A4C_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A5C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmaddsubps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A5D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmaddsubpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A5E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmsubaddps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A5F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmsubaddpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A60 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A60_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A61 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A61_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A62 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A62_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A63 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A63_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A68 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmaddps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A69 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmaddpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A6A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A6B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmsubps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfmsubpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A6E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A6F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A6F_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A78 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfnmaddps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A79 */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfnmaddpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7A */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A7A_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7B */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A7B_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7C */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfnmsubps", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7D */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "vfnmsubpd", { XMVexW, VexFMA, EXVexW, EXVexW, VexI4 } },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7E */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A7E_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3A7F */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3A7F_P_2) },
    { "(bad)",	{ XX } },
  },

  /* PREFIX_VEX_3ADF */
  {
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_3ADF_P_2) },
    { "(bad)",	{ XX } },
  },
};

static const struct dis386 x86_64_table[][2] = {
  /* X86_64_06 */
  {
    { "push{T|}", { es } },
    { "(bad)", { XX } },
  },

  /* X86_64_07 */
  {
    { "pop{T|}", { es } },
    { "(bad)", { XX } },
  },

  /* X86_64_0D */
  {
    { "push{T|}", { cs } },
    { "(bad)", { XX } },
  },

  /* X86_64_16 */
  {
    { "push{T|}", { ss } },
    { "(bad)", { XX } },
  },

  /* X86_64_17 */
  {
    { "pop{T|}", { ss } },
    { "(bad)", { XX } },
  },

  /* X86_64_1E */
  {
    { "push{T|}", { ds } },
    { "(bad)", { XX } },
  },

  /* X86_64_1F */
  {
    { "pop{T|}", { ds } },
    { "(bad)", { XX } },
  },

  /* X86_64_27 */
  {
    { "daa", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_2F */
  {
    { "das", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_37 */
  {
    { "aaa", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_3F */
  {
    { "aas", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_60 */
  {
    { "pusha{P|}", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_61 */
  {
    { "popa{P|}", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_62 */
  {
    { MOD_TABLE (MOD_62_32BIT) },
    { "(bad)", { XX } },
  },

  /* X86_64_63 */
  {
    { "arpl", { Ew, Gw } },
    { "movs{lq|xd}", { Gv, Ed } },
  },

  /* X86_64_6D */
  {
    { "ins{R|}", { Yzr, indirDX } },
    { "ins{G|}", { Yzr, indirDX } },
  },

  /* X86_64_6F */
  {
    { "outs{R|}", { indirDXr, Xz } },
    { "outs{G|}", { indirDXr, Xz } },
  },

  /* X86_64_9A */
  {
    { "Jcall{T|}", { Ap } },
    { "(bad)", { XX } },
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
    { "into", { XX } },
    { "(bad)", { XX } },
  },

  /* X86_64_D4 */
  {
    { "aam", { sIb } },
    { "(bad)", { XX } },
  },

  /* X86_64_D5 */
  {
    { "aad", { sIb } },
    { "(bad)", { XX } },
  },

  /* X86_64_EA */
  {
    { "Jjmp{T|}", { Ap } },
    { "(bad)", { XX } },
  },

  /* X86_64_0F01_REG_0 */
  {
    { "sgdt{Q|IQ}", { M } },
    { "sgdt", { M } },
  },

  /* X86_64_0F01_REG_1 */
  {
    { "sidt{Q|IQ}", { M } },
    { "sidt", { M } },
  },

  /* X86_64_0F01_REG_2 */
  {
    { "lgdt{Q|Q}", { M } },
    { "lgdt", { M } },
  },

  /* X86_64_0F01_REG_3 */
  {
    { "lidt{Q|Q}", { M } },
    { "lidt", { M } },
  },
};

static const struct dis386 three_byte_table[][256] = {
  /* THREE_BYTE_0F24 */
  {
    /* 00 */
    { "fmaddps",	{ { OP_DREX4, q_mode } } },
    { "fmaddpd",	{ { OP_DREX4, q_mode } } },
    { "fmaddss",	{ { OP_DREX4, w_mode } } },
    { "fmaddsd",	{ { OP_DREX4, d_mode } } },
    { "fmaddps",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fmaddpd",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fmaddss",	{ { OP_DREX4, DREX_OC1 + w_mode } } },
    { "fmaddsd",	{ { OP_DREX4, DREX_OC1 + d_mode } } },
    /* 08 */
    { "fmsubps",	{ { OP_DREX4, q_mode } } },
    { "fmsubpd",	{ { OP_DREX4, q_mode } } },
    { "fmsubss",	{ { OP_DREX4, w_mode } } },
    { "fmsubsd",	{ { OP_DREX4, d_mode } } },
    { "fmsubps",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fmsubpd",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fmsubss",	{ { OP_DREX4, DREX_OC1 + w_mode } } },
    { "fmsubsd",	{ { OP_DREX4, DREX_OC1 + d_mode } } },
    /* 10 */
    { "fnmaddps",	{ { OP_DREX4, q_mode } } },
    { "fnmaddpd",	{ { OP_DREX4, q_mode } } },
    { "fnmaddss",	{ { OP_DREX4, w_mode } } },
    { "fnmaddsd",	{ { OP_DREX4, d_mode } } },
    { "fnmaddps",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fnmaddpd",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fnmaddss",	{ { OP_DREX4, DREX_OC1 + w_mode } } },
    { "fnmaddsd",	{ { OP_DREX4, DREX_OC1 + d_mode } } },
    /* 18 */
    { "fnmsubps",	{ { OP_DREX4, q_mode } } },
    { "fnmsubpd",	{ { OP_DREX4, q_mode } } },
    { "fnmsubss",	{ { OP_DREX4, w_mode } } },
    { "fnmsubsd",	{ { OP_DREX4, d_mode } } },
    { "fnmsubps",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fnmsubpd",	{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "fnmsubss",	{ { OP_DREX4, DREX_OC1 + w_mode } } },
    { "fnmsubsd",	{ { OP_DREX4, DREX_OC1 + d_mode } } },
    /* 20 */
    { "permps",		{ { OP_DREX4, q_mode } } },
    { "permpd",		{ { OP_DREX4, q_mode } } },
    { "pcmov",		{ { OP_DREX4, q_mode } } },
    { "pperm",		{ { OP_DREX4, q_mode } } },
    { "permps",		{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "permpd",		{ { OP_DREX4, DREX_OC1 + q_mode } } },
    { "pcmov",		{ { OP_DREX4, DREX_OC1 + w_mode } } },
    { "pperm",		{ { OP_DREX4, DREX_OC1 + d_mode } } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { "protb",		{ { OP_DREX3, q_mode } } },
    { "protw",		{ { OP_DREX3, q_mode } } },
    { "protd",		{ { OP_DREX3, q_mode } } },
    { "protq",		{ { OP_DREX3, q_mode } } },
    { "pshlb",		{ { OP_DREX3, q_mode } } },
    { "pshlw",		{ { OP_DREX3, q_mode } } },
    { "pshld",		{ { OP_DREX3, q_mode } } },
    { "pshlq",		{ { OP_DREX3, q_mode } } },
    /* 48 */
    { "pshab",		{ { OP_DREX3, q_mode } } },
    { "pshaw",		{ { OP_DREX3, q_mode } } },
    { "pshad",		{ { OP_DREX3, q_mode } } },
    { "pshaq",		{ { OP_DREX3, q_mode } } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmacssww",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacsswd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacssdql",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmacssdd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacssdqh",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmacsww",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacswd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacsdql",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmacsdd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "pmacsdqh",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmadcsswd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pmadcswd",	{ { OP_DREX4, DREX_OC1 + DREX_NO_OC0 + q_mode } } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* THREE_BYTE_0F25 */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 08 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 10 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "comps",		{ { OP_DREX3, q_mode }, { OP_DREX_FCMP, b_mode } } },
    { "compd",		{ { OP_DREX3, q_mode }, { OP_DREX_FCMP, b_mode } } },
    { "comss",		{ { OP_DREX3, w_mode }, { OP_DREX_FCMP, b_mode } } },
    { "comsd",		{ { OP_DREX3, d_mode }, { OP_DREX_FCMP, b_mode } } },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pcomb",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomw",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomd",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomq",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pcomub",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomuw",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomud",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    { "pcomuq",		{ { OP_DREX3, q_mode }, { OP_DREX_ICMP, b_mode } } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* THREE_BYTE_0F38 */
  {
    /* 00 */
    { "pshufb",		{ MX, EM } },
    { "phaddw",		{ MX, EM } },
    { "phaddd",		{ MX, EM } },
    { "phaddsw",	{ MX, EM } },
    { "pmaddubsw",	{ MX, EM } },
    { "phsubw",		{ MX, EM } },
    { "phsubd",		{ MX, EM } },
    { "phsubsw",	{ MX, EM } },
    /* 08 */
    { "psignb",		{ MX, EM } },
    { "psignw",		{ MX, EM } },
    { "psignd",		{ MX, EM } },
    { "pmulhrsw",	{ MX, EM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 10 */
    { PREFIX_TABLE (PREFIX_0F3810) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F3814) },
    { PREFIX_TABLE (PREFIX_0F3815) },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F3817) },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "pabsb",		{ MX, EM } },
    { "pabsw",		{ MX, EM } },
    { "pabsd",		{ MX, EM } },
    { "(bad)",		{ XX } },
    /* 20 */
    { PREFIX_TABLE (PREFIX_0F3820) },
    { PREFIX_TABLE (PREFIX_0F3821) },
    { PREFIX_TABLE (PREFIX_0F3822) },
    { PREFIX_TABLE (PREFIX_0F3823) },
    { PREFIX_TABLE (PREFIX_0F3824) },
    { PREFIX_TABLE (PREFIX_0F3825) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { PREFIX_TABLE (PREFIX_0F3828) },
    { PREFIX_TABLE (PREFIX_0F3829) },
    { PREFIX_TABLE (PREFIX_0F382A) },
    { PREFIX_TABLE (PREFIX_0F382B) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { PREFIX_TABLE (PREFIX_0F3830) },
    { PREFIX_TABLE (PREFIX_0F3831) },
    { PREFIX_TABLE (PREFIX_0F3832) },
    { PREFIX_TABLE (PREFIX_0F3833) },
    { PREFIX_TABLE (PREFIX_0F3834) },
    { PREFIX_TABLE (PREFIX_0F3835) },
    { "(bad)",		{ XX } },
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
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { PREFIX_TABLE (PREFIX_0F3880) },
    { PREFIX_TABLE (PREFIX_0F3881) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F38DB) },
    { PREFIX_TABLE (PREFIX_0F38DC) },
    { PREFIX_TABLE (PREFIX_0F38DD) },
    { PREFIX_TABLE (PREFIX_0F38DE) },
    { PREFIX_TABLE (PREFIX_0F38DF) },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { PREFIX_TABLE (PREFIX_0F38F0) },
    { PREFIX_TABLE (PREFIX_0F38F1) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* THREE_BYTE_0F3A */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 08 */
    { PREFIX_TABLE (PREFIX_0F3A08) },
    { PREFIX_TABLE (PREFIX_0F3A09) },
    { PREFIX_TABLE (PREFIX_0F3A0A) },
    { PREFIX_TABLE (PREFIX_0F3A0B) },
    { PREFIX_TABLE (PREFIX_0F3A0C) },
    { PREFIX_TABLE (PREFIX_0F3A0D) },
    { PREFIX_TABLE (PREFIX_0F3A0E) },
    { "palignr",	{ MX, EM, Ib } },
    /* 10 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F3A14) },
    { PREFIX_TABLE (PREFIX_0F3A15) },
    { PREFIX_TABLE (PREFIX_0F3A16) },
    { PREFIX_TABLE (PREFIX_0F3A17) },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { PREFIX_TABLE (PREFIX_0F3A20) },
    { PREFIX_TABLE (PREFIX_0F3A21) },
    { PREFIX_TABLE (PREFIX_0F3A22) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { PREFIX_TABLE (PREFIX_0F3A40) },
    { PREFIX_TABLE (PREFIX_0F3A41) },
    { PREFIX_TABLE (PREFIX_0F3A42) },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F3A44) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { PREFIX_TABLE (PREFIX_0F3A60) },
    { PREFIX_TABLE (PREFIX_0F3A61) },
    { PREFIX_TABLE (PREFIX_0F3A62) },
    { PREFIX_TABLE (PREFIX_0F3A63) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F3ADF) },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* THREE_BYTE_0F7A */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 08 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 10 */
    { "frczps",		{ XM, EXq } },
    { "frczpd",		{ XM, EXq } },
    { "frczss",		{ XM, EXq } },
    { "frczsd",		{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { "ptest",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { "cvtph2ps",	{ XM, EXd } },
    { "cvtps2ph",	{ EXd, XM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { "(bad)",		{ XX } },
    { "phaddbw",	{ XM, EXq } },
    { "phaddbd",	{ XM, EXq } },
    { "phaddbq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "phaddwd",	{ XM, EXq } },
    { "phaddwq",	{ XM, EXq } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "phadddq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "phaddubw",	{ XM, EXq } },
    { "phaddubd",	{ XM, EXq } },
    { "phaddubq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "phadduwd",	{ XM, EXq } },
    { "phadduwq",	{ XM, EXq } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "phaddudq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "phsubbw",	{ XM, EXq } },
    { "phsubbd",	{ XM, EXq } },
    { "phsubbq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* THREE_BYTE_0F7B */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 08 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 10 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { "protb",		{ XM, EXq, Ib } },
    { "protw",		{ XM, EXq, Ib } },
    { "protd",		{ XM, EXq, Ib } },
    { "protq",		{ XM, EXq, Ib } },
    { "pshlb",		{ XM, EXq, Ib } },
    { "pshlw",		{ XM, EXq, Ib } },
    { "pshld",		{ XM, EXq, Ib } },
    { "pshlq",		{ XM, EXq, Ib } },
    /* 48 */
    { "pshab",		{ XM, EXq, Ib } },
    { "pshaw",		{ XM, EXq, Ib } },
    { "pshad",		{ XM, EXq, Ib } },
    { "pshaq",		{ XM, EXq, Ib } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
};

static const struct dis386 vex_table[][256] = {
  /* VEX_0F */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 08 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 10 */
    { PREFIX_TABLE (PREFIX_VEX_10) },
    { PREFIX_TABLE (PREFIX_VEX_11) },
    { PREFIX_TABLE (PREFIX_VEX_12) },
    { MOD_TABLE (MOD_VEX_13) },
    { "vunpcklpX",	{ XM, Vex, EXx } },
    { "vunpckhpX",	{ XM, Vex, EXx } },
    { PREFIX_TABLE (PREFIX_VEX_16) },
    { MOD_TABLE (MOD_VEX_17) },
    /* 18 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "vmovapX",	{ XM, EXx } },
    { "vmovapX",	{ EXx, XM } },
    { PREFIX_TABLE (PREFIX_VEX_2A) },
    { MOD_TABLE (MOD_VEX_2B) },
    { PREFIX_TABLE (PREFIX_VEX_2C) },
    { PREFIX_TABLE (PREFIX_VEX_2D) },
    { PREFIX_TABLE (PREFIX_VEX_2E) },
    { PREFIX_TABLE (PREFIX_VEX_2F) },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { MOD_TABLE (MOD_VEX_51) },
    { PREFIX_TABLE (PREFIX_VEX_51) },
    { PREFIX_TABLE (PREFIX_VEX_52) },
    { PREFIX_TABLE (PREFIX_VEX_53) },
    { "vandpX",		{ XM, Vex, EXx } },
    { "vandnpX",	{ XM, Vex, EXx } },
    { "vorpX",		{ XM, Vex, EXx } },
    { "vxorpX",		{ XM, Vex, EXx } },
    /* 58 */
    { PREFIX_TABLE (PREFIX_VEX_58) },
    { PREFIX_TABLE (PREFIX_VEX_59) },
    { PREFIX_TABLE (PREFIX_VEX_5A) },
    { PREFIX_TABLE (PREFIX_VEX_5B) },
    { PREFIX_TABLE (PREFIX_VEX_5C) },
    { PREFIX_TABLE (PREFIX_VEX_5D) },
    { PREFIX_TABLE (PREFIX_VEX_5E) },
    { PREFIX_TABLE (PREFIX_VEX_5F) },
    /* 60 */
    { PREFIX_TABLE (PREFIX_VEX_60) },
    { PREFIX_TABLE (PREFIX_VEX_61) },
    { PREFIX_TABLE (PREFIX_VEX_62) },
    { PREFIX_TABLE (PREFIX_VEX_63) },
    { PREFIX_TABLE (PREFIX_VEX_64) },
    { PREFIX_TABLE (PREFIX_VEX_65) },
    { PREFIX_TABLE (PREFIX_VEX_66) },
    { PREFIX_TABLE (PREFIX_VEX_67) },
    /* 68 */
    { PREFIX_TABLE (PREFIX_VEX_68) },
    { PREFIX_TABLE (PREFIX_VEX_69) },
    { PREFIX_TABLE (PREFIX_VEX_6A) },
    { PREFIX_TABLE (PREFIX_VEX_6B) },
    { PREFIX_TABLE (PREFIX_VEX_6C) },
    { PREFIX_TABLE (PREFIX_VEX_6D) },
    { PREFIX_TABLE (PREFIX_VEX_6E) },
    { PREFIX_TABLE (PREFIX_VEX_6F) },
    /* 70 */
    { PREFIX_TABLE (PREFIX_VEX_70) },
    { REG_TABLE (REG_VEX_71) },
    { REG_TABLE (REG_VEX_72) },
    { REG_TABLE (REG_VEX_73) },
    { PREFIX_TABLE (PREFIX_VEX_74) },
    { PREFIX_TABLE (PREFIX_VEX_75) },
    { PREFIX_TABLE (PREFIX_VEX_76) },
    { PREFIX_TABLE (PREFIX_VEX_77) },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_7C) },
    { PREFIX_TABLE (PREFIX_VEX_7D) },
    { PREFIX_TABLE (PREFIX_VEX_7E) },
    { PREFIX_TABLE (PREFIX_VEX_7F) },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { REG_TABLE (REG_VEX_AE) },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_C2) },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_C4) },
    { PREFIX_TABLE (PREFIX_VEX_C5) },
    { "vshufpX",	{ XM, Vex, EXx, Ib } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { PREFIX_TABLE (PREFIX_VEX_D0) },
    { PREFIX_TABLE (PREFIX_VEX_D1) },
    { PREFIX_TABLE (PREFIX_VEX_D2) },
    { PREFIX_TABLE (PREFIX_VEX_D3) },
    { PREFIX_TABLE (PREFIX_VEX_D4) },
    { PREFIX_TABLE (PREFIX_VEX_D5) },
    { PREFIX_TABLE (PREFIX_VEX_D6) },
    { PREFIX_TABLE (PREFIX_VEX_D7) },
    /* d8 */
    { PREFIX_TABLE (PREFIX_VEX_D8) },
    { PREFIX_TABLE (PREFIX_VEX_D9) },
    { PREFIX_TABLE (PREFIX_VEX_DA) },
    { PREFIX_TABLE (PREFIX_VEX_DB) },
    { PREFIX_TABLE (PREFIX_VEX_DC) },
    { PREFIX_TABLE (PREFIX_VEX_DD) },
    { PREFIX_TABLE (PREFIX_VEX_DE) },
    { PREFIX_TABLE (PREFIX_VEX_DF) },
    /* e0 */
    { PREFIX_TABLE (PREFIX_VEX_E0) },
    { PREFIX_TABLE (PREFIX_VEX_E1) },
    { PREFIX_TABLE (PREFIX_VEX_E2) },
    { PREFIX_TABLE (PREFIX_VEX_E3) },
    { PREFIX_TABLE (PREFIX_VEX_E4) },
    { PREFIX_TABLE (PREFIX_VEX_E5) },
    { PREFIX_TABLE (PREFIX_VEX_E6) },
    { PREFIX_TABLE (PREFIX_VEX_E7) },
    /* e8 */
    { PREFIX_TABLE (PREFIX_VEX_E8) },
    { PREFIX_TABLE (PREFIX_VEX_E9) },
    { PREFIX_TABLE (PREFIX_VEX_EA) },
    { PREFIX_TABLE (PREFIX_VEX_EB) },
    { PREFIX_TABLE (PREFIX_VEX_EC) },
    { PREFIX_TABLE (PREFIX_VEX_ED) },
    { PREFIX_TABLE (PREFIX_VEX_EE) },
    { PREFIX_TABLE (PREFIX_VEX_EF) },
    /* f0 */
    { PREFIX_TABLE (PREFIX_VEX_F0) },
    { PREFIX_TABLE (PREFIX_VEX_F1) },
    { PREFIX_TABLE (PREFIX_VEX_F2) },
    { PREFIX_TABLE (PREFIX_VEX_F3) },
    { PREFIX_TABLE (PREFIX_VEX_F4) },
    { PREFIX_TABLE (PREFIX_VEX_F5) },
    { PREFIX_TABLE (PREFIX_VEX_F6) },
    { PREFIX_TABLE (PREFIX_VEX_F7) },
    /* f8 */
    { PREFIX_TABLE (PREFIX_VEX_F8) },
    { PREFIX_TABLE (PREFIX_VEX_F9) },
    { PREFIX_TABLE (PREFIX_VEX_FA) },
    { PREFIX_TABLE (PREFIX_VEX_FB) },
    { PREFIX_TABLE (PREFIX_VEX_FC) },
    { PREFIX_TABLE (PREFIX_VEX_FD) },
    { PREFIX_TABLE (PREFIX_VEX_FE) },
    { "(bad)",		{ XX } },
  },
  /* VEX_0F38 */
  {
    /* 00 */
    { PREFIX_TABLE (PREFIX_VEX_3800) },
    { PREFIX_TABLE (PREFIX_VEX_3801) },
    { PREFIX_TABLE (PREFIX_VEX_3802) },
    { PREFIX_TABLE (PREFIX_VEX_3803) },
    { PREFIX_TABLE (PREFIX_VEX_3804) },
    { PREFIX_TABLE (PREFIX_VEX_3805) },
    { PREFIX_TABLE (PREFIX_VEX_3806) },
    { PREFIX_TABLE (PREFIX_VEX_3807) },
    /* 08 */
    { PREFIX_TABLE (PREFIX_VEX_3808) },
    { PREFIX_TABLE (PREFIX_VEX_3809) },
    { PREFIX_TABLE (PREFIX_VEX_380A) },
    { PREFIX_TABLE (PREFIX_VEX_380B) },
    { PREFIX_TABLE (PREFIX_VEX_380C) },
    { PREFIX_TABLE (PREFIX_VEX_380D) },
    { PREFIX_TABLE (PREFIX_VEX_380E) },
    { PREFIX_TABLE (PREFIX_VEX_380F) },
    /* 10 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3817) },
    /* 18 */
    { PREFIX_TABLE (PREFIX_VEX_3818) },
    { PREFIX_TABLE (PREFIX_VEX_3819) },
    { PREFIX_TABLE (PREFIX_VEX_381A) },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_381C) },
    { PREFIX_TABLE (PREFIX_VEX_381D) },
    { PREFIX_TABLE (PREFIX_VEX_381E) },
    { "(bad)",		{ XX } },
    /* 20 */
    { PREFIX_TABLE (PREFIX_VEX_3820) },
    { PREFIX_TABLE (PREFIX_VEX_3821) },
    { PREFIX_TABLE (PREFIX_VEX_3822) },
    { PREFIX_TABLE (PREFIX_VEX_3823) },
    { PREFIX_TABLE (PREFIX_VEX_3824) },
    { PREFIX_TABLE (PREFIX_VEX_3825) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { PREFIX_TABLE (PREFIX_VEX_3828) },
    { PREFIX_TABLE (PREFIX_VEX_3829) },
    { PREFIX_TABLE (PREFIX_VEX_382A) },
    { PREFIX_TABLE (PREFIX_VEX_382B) },
    { PREFIX_TABLE (PREFIX_VEX_382C) },
    { PREFIX_TABLE (PREFIX_VEX_382D) },
    { PREFIX_TABLE (PREFIX_VEX_382E) },
    { PREFIX_TABLE (PREFIX_VEX_382F) },
    /* 30 */
    { PREFIX_TABLE (PREFIX_VEX_3830) },
    { PREFIX_TABLE (PREFIX_VEX_3831) },
    { PREFIX_TABLE (PREFIX_VEX_3832) },
    { PREFIX_TABLE (PREFIX_VEX_3833) },
    { PREFIX_TABLE (PREFIX_VEX_3834) },
    { PREFIX_TABLE (PREFIX_VEX_3835) },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3837) },
    /* 38 */
    { PREFIX_TABLE (PREFIX_VEX_3838) },
    { PREFIX_TABLE (PREFIX_VEX_3839) },
    { PREFIX_TABLE (PREFIX_VEX_383A) },
    { PREFIX_TABLE (PREFIX_VEX_383B) },
    { PREFIX_TABLE (PREFIX_VEX_383C) },
    { PREFIX_TABLE (PREFIX_VEX_383D) },
    { PREFIX_TABLE (PREFIX_VEX_383E) },
    { PREFIX_TABLE (PREFIX_VEX_383F) },
    /* 40 */
    { PREFIX_TABLE (PREFIX_VEX_3840) },
    { PREFIX_TABLE (PREFIX_VEX_3841) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 60 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_38DB) },
    { PREFIX_TABLE (PREFIX_VEX_38DC) },
    { PREFIX_TABLE (PREFIX_VEX_38DD) },
    { PREFIX_TABLE (PREFIX_VEX_38DE) },
    { PREFIX_TABLE (PREFIX_VEX_38DF) },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  /* VEX_0F3A */
  {
    /* 00 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3A04) },
    { PREFIX_TABLE (PREFIX_VEX_3A05) },
    { PREFIX_TABLE (PREFIX_VEX_3A06) },
    { "(bad)",		{ XX } },
    /* 08 */
    { PREFIX_TABLE (PREFIX_VEX_3A08) },
    { PREFIX_TABLE (PREFIX_VEX_3A09) },
    { PREFIX_TABLE (PREFIX_VEX_3A0A) },
    { PREFIX_TABLE (PREFIX_VEX_3A0B) },
    { PREFIX_TABLE (PREFIX_VEX_3A0C) },
    { PREFIX_TABLE (PREFIX_VEX_3A0D) },
    { PREFIX_TABLE (PREFIX_VEX_3A0E) },
    { PREFIX_TABLE (PREFIX_VEX_3A0F) },
    /* 10 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3A14) },
    { PREFIX_TABLE (PREFIX_VEX_3A15) },
    { PREFIX_TABLE (PREFIX_VEX_3A16) },
    { PREFIX_TABLE (PREFIX_VEX_3A17) },
    /* 18 */
    { PREFIX_TABLE (PREFIX_VEX_3A18) },
    { PREFIX_TABLE (PREFIX_VEX_3A19) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 20 */
    { PREFIX_TABLE (PREFIX_VEX_3A20) },
    { PREFIX_TABLE (PREFIX_VEX_3A21) },
    { PREFIX_TABLE (PREFIX_VEX_3A22) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 28 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 30 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 38 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 40 */
    { PREFIX_TABLE (PREFIX_VEX_3A40) },
    { PREFIX_TABLE (PREFIX_VEX_3A41) },
    { PREFIX_TABLE (PREFIX_VEX_3A42) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 48 */
    { PREFIX_TABLE (PREFIX_VEX_3A48) },
    { PREFIX_TABLE (PREFIX_VEX_3A49) },
    { PREFIX_TABLE (PREFIX_VEX_3A4A) },
    { PREFIX_TABLE (PREFIX_VEX_3A4B) },
    { PREFIX_TABLE (PREFIX_VEX_3A4C) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 50 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 58 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3A5C) },
    { PREFIX_TABLE (PREFIX_VEX_3A5D) },
    { PREFIX_TABLE (PREFIX_VEX_3A5E) },
    { PREFIX_TABLE (PREFIX_VEX_3A5F) },
    /* 60 */
    { PREFIX_TABLE (PREFIX_VEX_3A60) },
    { PREFIX_TABLE (PREFIX_VEX_3A61) },
    { PREFIX_TABLE (PREFIX_VEX_3A62) },
    { PREFIX_TABLE (PREFIX_VEX_3A63) },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 68 */
    { PREFIX_TABLE (PREFIX_VEX_3A68) },
    { PREFIX_TABLE (PREFIX_VEX_3A69) },
    { PREFIX_TABLE (PREFIX_VEX_3A6A) },
    { PREFIX_TABLE (PREFIX_VEX_3A6B) },
    { PREFIX_TABLE (PREFIX_VEX_3A6C) },
    { PREFIX_TABLE (PREFIX_VEX_3A6D) },
    { PREFIX_TABLE (PREFIX_VEX_3A6E) },
    { PREFIX_TABLE (PREFIX_VEX_3A6F) },
    /* 70 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 78 */
    { PREFIX_TABLE (PREFIX_VEX_3A78) },
    { PREFIX_TABLE (PREFIX_VEX_3A79) },
    { PREFIX_TABLE (PREFIX_VEX_3A7A) },
    { PREFIX_TABLE (PREFIX_VEX_3A7B) },
    { PREFIX_TABLE (PREFIX_VEX_3A7C) },
    { PREFIX_TABLE (PREFIX_VEX_3A7D) },
    { PREFIX_TABLE (PREFIX_VEX_3A7E) },
    { PREFIX_TABLE (PREFIX_VEX_3A7F) },
    /* 80 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 88 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 90 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* 98 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* a8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* b8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* c8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* d8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_3ADF) },
    /* e0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* e8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f0 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    /* f8 */
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
};

static const struct dis386 vex_len_table[][2] = {
  /* VEX_LEN_10_P_1 */
  {
    { "vmovss",		{ XMVex, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_10_P_3 */
  {
    { "vmovsd",		{ XMVex, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_11_P_1 */
  {
    { "vmovss",		{ EXdVex, Vex128, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_11_P_3 */
  {
    { "vmovsd",		{ EXqVex, Vex128, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_12_P_0_M_0 */
  {
    { "vmovlps",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_12_P_0_M_1 */
  {
    { "vmovhlps",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_12_P_2 */
  {
    { "vmovlpd",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_13_M_0 */
  {
    { "vmovlpX",	{ EXq, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_16_P_0_M_0 */
  {
    { "vmovhps",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_16_P_0_M_1 */
  {
    { "vmovlhps",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_16_P_2 */
  {
    { "vmovhpd",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_17_M_0 */
  {
    { "vmovhpX",	{ EXq, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2A_P_1 */
  {
    { "vcvtsi2ss%LQ",	{ XM, Vex128, Ev } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2A_P_3 */
  {
    { "vcvtsi2sd%LQ",	{ XM, Vex128, Ev } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2B_M_0 */
  {
    { "vmovntpX",	{ Mx, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2C_P_1 */
  {
    { "vcvttss2siY",	{ Gv, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2C_P_3 */
  {
    { "vcvttsd2siY",	{ Gv, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2D_P_1 */
  {
    { "vcvtss2siY",	{ Gv, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2D_P_3 */
  {
    { "vcvtsd2siY",	{ Gv, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2E_P_0 */
  {
    { "vucomiss",	{ XM, EXd } }, 
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2E_P_2 */
  {
    { "vucomisd",	{ XM, EXq } }, 
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2F_P_0 */
  {
    { "vcomiss",	{ XM, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_2F_P_2 */
  {
    { "vcomisd",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_51_P_1 */
  {
    { "vsqrtss",	{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_51_P_3 */
  {
    { "vsqrtsd",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_52_P_1 */
  {
    { "vrsqrtss",	{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_53_P_1 */
  {
    { "vrcpss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_58_P_1 */
  {
    { "vaddss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_58_P_3 */
  {
    { "vaddsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_59_P_1 */
  {
    { "vmulss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_59_P_3 */
  {
    { "vmulsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5A_P_1 */
  {
    { "vcvtss2sd",	{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5A_P_3 */
  {
    { "vcvtsd2ss",	{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5C_P_1 */
  {
    { "vsubss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5C_P_3 */
  {
    { "vsubsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5D_P_1 */
  {
    { "vminss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5D_P_3 */
  {
    { "vminsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5E_P_1 */
  {
    { "vdivss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5E_P_3 */
  {
    { "vdivsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5F_P_1 */
  {
    { "vmaxss",		{ XM, Vex128, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_5F_P_3 */
  {
    { "vmaxsd",		{ XM, Vex128, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_60_P_2 */
  {
    { "vpunpcklbw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_61_P_2 */
  {
    { "vpunpcklwd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_62_P_2 */
  {
    { "vpunpckldq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_63_P_2 */
  {
    { "vpacksswb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_64_P_2 */
  {
    { "vpcmpgtb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_65_P_2 */
  {
    { "vpcmpgtw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_66_P_2 */
  {
    { "vpcmpgtd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_67_P_2 */
  {
    { "vpackuswb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_68_P_2 */
  {
    { "vpunpckhbw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_69_P_2 */
  {
    { "vpunpckhwd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_6A_P_2 */
  {
    { "vpunpckhdq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_6B_P_2 */
  {
    { "vpackssdw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_6C_P_2 */
  {
    { "vpunpcklqdq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_6D_P_2 */
  {
    { "vpunpckhqdq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_6E_P_2 */
  {
    { "vmovK",		{ XM, Edq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_70_P_1 */
  {
    { "vpshufhw",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_70_P_2 */
  {
    { "vpshufd",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_70_P_3 */
  {
    { "vpshuflw",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_71_R_2_P_2 */
  {
    { "vpsrlw",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_71_R_4_P_2 */
  {
    { "vpsraw",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_71_R_6_P_2 */
  {
    { "vpsllw",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_72_R_2_P_2 */
  {
    { "vpsrld",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_72_R_4_P_2 */
  {
    { "vpsrad",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_72_R_6_P_2 */
  {
    { "vpslld",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_73_R_2_P_2 */
  {
    { "vpsrlq",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_73_R_3_P_2 */
  {
    { "vpsrldq",	{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_73_R_6_P_2 */
  {
    { "vpsllq",		{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_73_R_7_P_2 */
  {
    { "vpslldq",	{ Vex128, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_74_P_2 */
  {
    { "vpcmpeqb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_75_P_2 */
  {
    { "vpcmpeqw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_76_P_2 */
  {
    { "vpcmpeqd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_7E_P_1 */
  {
    { "vmovq",		{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_7E_P_2 */
  {
    { "vmovK",		{ Edq, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_AE_R_2_M0 */
  {
    { "vldmxcsr",	{ Md } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_AE_R_3_M0 */
  {
    { "vstmxcsr",	{ Md } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_C2_P_1 */
  {
    { "vcmpss",		{ XM, Vex128, EXd, VCMP } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_C2_P_3 */
  {
    { "vcmpsd",		{ XM, Vex128, EXq, VCMP } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_C4_P_2 */
  {
    { "vpinsrw",	{ XM, Vex128, Edqw, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_C5_P_2 */
  {
    { "vpextrw",	{ Gdq, XS, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D1_P_2 */
  {
    { "vpsrlw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D2_P_2 */
  {
    { "vpsrld",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D3_P_2 */
  {
    { "vpsrlq",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D4_P_2 */
  {
    { "vpaddq",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D5_P_2 */
  {
    { "vpmullw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D6_P_2 */
  {
    { "vmovq",		{ EXq, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D7_P_2_M_1 */
  {
    { "vpmovmskb",	{ Gdq, XS } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D8_P_2 */
  {
    { "vpsubusb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_D9_P_2 */
  {
    { "vpsubusw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DA_P_2 */
  {
    { "vpminub",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DB_P_2 */
  {
    { "vpand",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DC_P_2 */
  {
    { "vpaddusb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DD_P_2 */
  {
    { "vpaddusw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DE_P_2 */
  {
    { "vpmaxub",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_DF_P_2 */
  {
    { "vpandn",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E0_P_2 */
  {
    { "vpavgb",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E1_P_2 */
  {
    { "vpsraw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E2_P_2 */
  {
    { "vpsrad",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E3_P_2 */
  {
    { "vpavgw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E4_P_2 */
  {
    { "vpmulhuw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E5_P_2 */
  {
    { "vpmulhw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E7_P_2_M_0 */
  {
    { "vmovntdq",	{ Mx, XM } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E8_P_2 */
  {
    { "vpsubsb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_E9_P_2 */
  {
    { "vpsubsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_EA_P_2 */
  {
    { "vpminsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_EB_P_2 */
  {
    { "vpor",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_EC_P_2 */
  {
    { "vpaddsb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_ED_P_2 */
  {
    { "vpaddsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_EE_P_2 */
  {
    { "vpmaxsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_EF_P_2 */
  {
    { "vpxor",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F1_P_2 */
  {
    { "vpsllw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F2_P_2 */
  {
    { "vpslld",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F3_P_2 */
  {
    { "vpsllq",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F4_P_2 */
  {
    { "vpmuludq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F5_P_2 */
  {
    { "vpmaddwd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F6_P_2 */
  {
    { "vpsadbw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F7_P_2 */
  {
    { "vmaskmovdqu",	{ XM, XS } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F8_P_2 */
  {
    { "vpsubb",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_F9_P_2 */
  {
    { "vpsubw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_FA_P_2 */
  {
    { "vpsubd",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_FB_P_2 */
  {
    { "vpsubq",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_FC_P_2 */
  {
    { "vpaddb",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_FD_P_2 */
  {
    { "vpaddw",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_FE_P_2 */
  {
    { "vpaddd",		{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3800_P_2 */
  {
    { "vpshufb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3801_P_2 */
  {
    { "vphaddw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3802_P_2 */
  {
    { "vphaddd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3803_P_2 */
  {
    { "vphaddsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3804_P_2 */
  {
    { "vpmaddubsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3805_P_2 */
  {
    { "vphsubw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3806_P_2 */
  {
    { "vphsubd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3807_P_2 */
  {
    { "vphsubsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3808_P_2 */
  {
    { "vpsignb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3809_P_2 */
  {
    { "vpsignw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_380A_P_2 */
  {
    { "vpsignd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_380B_P_2 */
  {
    { "vpmulhrsw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3819_P_2_M_0 */
  {
    { "(bad)",		{ XX } },
    { "vbroadcastsd",	{ XM, Mq } },
  },

  /* VEX_LEN_381A_P_2_M_0 */
  {
    { "(bad)",		{ XX } },
    { "vbroadcastf128",	{ XM, Mxmm } },
  },

  /* VEX_LEN_381C_P_2 */
  {
    { "vpabsb",		{ XM, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_381D_P_2 */
  {
    { "vpabsw",		{ XM, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_381E_P_2 */
  {
    { "vpabsd",		{ XM, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3820_P_2 */
  {
    { "vpmovsxbw",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3821_P_2 */
  {
    { "vpmovsxbd",	{ XM, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3822_P_2 */
  {
    { "vpmovsxbq",	{ XM, EXw } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3823_P_2 */
  {
    { "vpmovsxwd",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3824_P_2 */
  {
    { "vpmovsxwq",	{ XM, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3825_P_2 */
  {
    { "vpmovsxdq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3828_P_2 */
  {
    { "vpmuldq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3829_P_2 */
  {
    { "vpcmpeqq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_382A_P_2_M_0 */
  {
    { "vmovntdqa",	{ XM, Mx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_382B_P_2 */
  {
    { "vpackusdw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3830_P_2 */
  {
    { "vpmovzxbw",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3831_P_2 */
  {
    { "vpmovzxbd",	{ XM, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3832_P_2 */
  {
    { "vpmovzxbq",	{ XM, EXw } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3833_P_2 */
  {
    { "vpmovzxwd",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3834_P_2 */
  {
    { "vpmovzxwq",	{ XM, EXd } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3835_P_2 */
  {
    { "vpmovzxdq",	{ XM, EXq } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3837_P_2 */
  {
    { "vpcmpgtq",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3838_P_2 */
  {
    { "vpminsb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3839_P_2 */
  {
    { "vpminsd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383A_P_2 */
  {
    { "vpminuw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383B_P_2 */
  {
    { "vpminud",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383C_P_2 */
  {
    { "vpmaxsb",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383D_P_2 */
  {
    { "vpmaxsd",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383E_P_2 */
  {
    { "vpmaxuw",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_383F_P_2 */
  {
    { "vpmaxud",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3840_P_2 */
  {
    { "vpmulld",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3841_P_2 */
  {
    { "vphminposuw",	{ XM, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_38DB_P_2 */
  {
    { "vaesimc",	{ XM, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_38DC_P_2 */
  {
    { "vaesenc",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_38DD_P_2 */
  {
    { "vaesenclast",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_38DE_P_2 */
  {
    { "vaesdec",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_38DF_P_2 */
  {
    { "vaesdeclast",	{ XM, Vex128, EXx } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A06_P_2 */
  {
    { "(bad)",		{ XX } },
    { "vperm2f128",	{ XM, Vex256, EXx, Ib } },
  },

  /* VEX_LEN_3A0A_P_2 */
  {
    { "vroundss",	{ XM, Vex128, EXd, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A0B_P_2 */
  {
    { "vroundsd",	{ XM, Vex128, EXq, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A0E_P_2 */
  {
    { "vpblendw",	{ XM, Vex128, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A0F_P_2 */
  {
    { "vpalignr",	{ XM, Vex128, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A14_P_2 */
  {
    { "vpextrb",	{ Edqb, XM, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A15_P_2 */
  {
    { "vpextrw",	{ Edqw, XM, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A16_P_2  */
  {
    { "vpextrK",	{ Edq, XM, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A17_P_2 */
  {
    { "vextractps",	{ Edqd, XM, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A18_P_2 */
  {
    { "(bad)",		{ XX } },
    { "vinsertf128",	{ XM, Vex256, EXxmm, Ib } },
  },

  /* VEX_LEN_3A19_P_2 */
  {
    { "(bad)",		{ XX } },
    { "vextractf128",	{ EXxmm, XM, Ib } },
  },

  /* VEX_LEN_3A20_P_2 */
  {
    { "vpinsrb",	{ XM, Vex128, Edqb, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A21_P_2 */
  {
    { "vinsertps",	{ XM, Vex128, EXd, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A22_P_2 */
  {
    { "vpinsrK",	{ XM, Vex128, Edq, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A41_P_2 */
  {
    { "vdppd",		{ XM, Vex128, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A42_P_2 */
  {
    { "vmpsadbw",	{ XM, Vex128, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A4C_P_2 */
  {
    { "vpblendvb",	{ XM, Vex128, EXx, XMVexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A60_P_2 */
  {
    { "vpcmpestrm",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A61_P_2 */
  {
    { "vpcmpestri",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A62_P_2 */
  {
    { "vpcmpistrm",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A63_P_2 */
  {
    { "vpcmpistri",	{ XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A6A_P_2 */
  {
    { "vfmaddss",	{ XMVexW, Vex128FMA, EXdVexW, EXdVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A6B_P_2 */
  {
    { "vfmaddsd",	{ XMVexW, Vex128FMA, EXqVexW, EXqVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A6E_P_2 */
  {
    { "vfmsubss",	{ XMVexW, Vex128FMA, EXdVexW, EXdVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A6F_P_2 */
  {
    { "vfmsubsd",	{ XMVexW, Vex128FMA, EXqVexW, EXqVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A7A_P_2 */
  {
    { "vfnmaddss",	{ XMVexW, Vex128FMA, EXdVexW, EXdVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A7B_P_2 */
  {
    { "vfnmaddsd",	{ XMVexW, Vex128FMA, EXqVexW, EXqVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A7E_P_2 */
  {
    { "vfnmsubss",	{ XMVexW, Vex128FMA, EXdVexW, EXdVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3A7F_P_2 */
  {
    { "vfnmsubsd",	{ XMVexW, Vex128FMA, EXqVexW, EXqVexW, VexI4 } },
    { "(bad)",		{ XX } },
  },

  /* VEX_LEN_3ADF_P_2 */
  {
    { "vaeskeygenassist", { XM, EXx, Ib } },
    { "(bad)",		{ XX } },
  },
};

static const struct dis386 mod_table[][2] = {
  {
    /* MOD_8D */
    { "leaS",		{ Gv, M } },
    { "(bad)",		{ XX } },
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
    { "invlpg",		{ Mb } },
    { RM_TABLE (RM_0F01_REG_7) },
  },
  {
    /* MOD_0F12_PREFIX_0 */
    { "movlps",		{ XM, EXq } },
    { "movhlps",	{ XM, EXq } },
  },
  {
    /* MOD_0F13 */
    { "movlpX",		{ EXq, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F16_PREFIX_0 */
    { "movhps",		{ XM, EXq } },
    { "movlhps",	{ XM, EXq } },
  },
  {
    /* MOD_0F17 */
    { "movhpX",		{ EXq, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F18_REG_0 */
    { "prefetchnta",	{ Mb } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F18_REG_1 */
    { "prefetcht0",	{ Mb } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F18_REG_2 */
    { "prefetcht1",	{ Mb } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F18_REG_3 */
    { "prefetcht2",	{ Mb } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F20 */
    { "(bad)",		{ XX } },
    { "movZ",		{ Rm, Cm } },
  },
  {
    /* MOD_0F21 */
    { "(bad)",		{ XX } },
    { "movZ",		{ Rm, Dm } },
  },
  {
    /* MOD_0F22 */
    { "(bad)",		{ XX } },
    { "movZ",		{ Cm, Rm } },
  },
  {
    /* MOD_0F23 */
    { "(bad)",		{ XX } },
    { "movZ",		{ Dm, Rm } },
  },
  {
    /* MOD_0F24 */
    { THREE_BYTE_TABLE (THREE_BYTE_0F24) },
    { "movL",		{ Rd, Td } },
  },
  {
    /* MOD_0F26 */
    { "(bad)",		{ XX } },
    { "movL",		{ Td, Rd } },
  },
  {
    /* MOD_0F2B_PREFIX_0 */
    {"movntps",		{ Mx, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F2B_PREFIX_1 */
    {"movntss",		{ Md, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F2B_PREFIX_2 */
    {"movntpd",		{ Mx, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F2B_PREFIX_3 */
    {"movntsd",		{ Mq, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F51 */
    { "(bad)",		{ XX } },
    { "movmskpX",	{ Gdq, XS } },
  },
  {
    /* MOD_0F71_REG_2 */
    { "(bad)",		{ XX } },
    { "psrlw",		{ MS, Ib } },
  },
  {
    /* MOD_0F71_REG_4 */
    { "(bad)",		{ XX } },
    { "psraw",		{ MS, Ib } },
  },
  {
    /* MOD_0F71_REG_6 */
    { "(bad)",		{ XX } },
    { "psllw",		{ MS, Ib } },
  },
  {
    /* MOD_0F72_REG_2 */
    { "(bad)",		{ XX } },
    { "psrld",		{ MS, Ib } },
  },
  {
    /* MOD_0F72_REG_4 */
    { "(bad)",		{ XX } },
    { "psrad",		{ MS, Ib } },
  },
  {
    /* MOD_0F72_REG_6 */
    { "(bad)",		{ XX } },
    { "pslld",		{ MS, Ib } },
  },
  {
    /* MOD_0F73_REG_2 */
    { "(bad)",		{ XX } },
    { "psrlq",		{ MS, Ib } },
  },
  {
    /* MOD_0F73_REG_3 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F73_REG_3) },
  },
  {
    /* MOD_0F73_REG_6 */
    { "(bad)",		{ XX } },
    { "psllq",		{ MS, Ib } },
  },
  {
    /* MOD_0F73_REG_7 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_0F73_REG_7) },
  },
  {
    /* MOD_0FAE_REG_0 */
    { "fxsave",		{ M } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FAE_REG_1 */
    { "fxrstor",	{ M } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FAE_REG_2 */
    { "ldmxcsr",	{ Md } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FAE_REG_3 */
    { "stmxcsr",	{ Md } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FAE_REG_4 */
    { "xsave",		{ M } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FAE_REG_5 */
    { "xrstor",		{ M } },
    { RM_TABLE (RM_0FAE_REG_5) },
  },
  {
    /* MOD_0FAE_REG_6 */
    { "xsaveopt",	{ M } },
    { RM_TABLE (RM_0FAE_REG_6) },
  },
  {
    /* MOD_0FAE_REG_7 */
    { "clflush",	{ Mb } },
    { RM_TABLE (RM_0FAE_REG_7) },
  },
  {
    /* MOD_0FB2 */
    { "lssS",		{ Gv, Mp } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FB4 */
    { "lfsS",		{ Gv, Mp } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FB5 */
    { "lgsS",		{ Gv, Mp } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FC7_REG_6 */
    { PREFIX_TABLE (PREFIX_0FC7_REG_6) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FC7_REG_7 */
    { "vmptrst",	{ Mq } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FD7 */
    { "(bad)",		{ XX } },
    { "pmovmskb",	{ Gdq, MS } },
  },
  {
    /* MOD_0FE7_PREFIX_2 */
    { "movntdq",	{ Mx, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0FF0_PREFIX_3 */
    { "lddqu",		{ XM, M } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_0F382A_PREFIX_2 */
    { "movntdqa",	{ XM, Mx } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_62_32BIT */
    { "bound{S|}",	{ Gv, Ma } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_C4_32BIT */
    { "lesS",		{ Gv, Mp } },
    { VEX_C4_TABLE (VEX_0F) },
  },
  {
    /* MOD_C5_32BIT */
    { "ldsS",		{ Gv, Mp } },
    { VEX_C5_TABLE (VEX_0F) },
  },
  {
    /* MOD_VEX_12_PREFIX_0 */
    { VEX_LEN_TABLE (VEX_LEN_12_P_0_M_0) },
    { VEX_LEN_TABLE (VEX_LEN_12_P_0_M_1) },
  },
  {
    /* MOD_VEX_13 */
    { VEX_LEN_TABLE (VEX_LEN_13_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_16_PREFIX_0 */
    { VEX_LEN_TABLE (VEX_LEN_16_P_0_M_0) },
    { VEX_LEN_TABLE (VEX_LEN_16_P_0_M_1) },
  },
  {
    /* MOD_VEX_17 */
    { VEX_LEN_TABLE (VEX_LEN_17_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_2B */
    { VEX_LEN_TABLE (VEX_LEN_2B_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_51 */
    { "(bad)",		{ XX } },
    { "vmovmskpX",	{ Gdq, XS } },
  },
  {
    /* MOD_VEX_71_REG_2 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_71_REG_2) },
  },
  {
    /* MOD_VEX_71_REG_4 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_71_REG_4) },
  },
  {
    /* MOD_VEX_71_REG_6 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_71_REG_6) },
  },
  {
    /* MOD_VEX_72_REG_2 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_72_REG_2) },
  },
  {
    /* MOD_VEX_72_REG_4 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_72_REG_4) },
  },
  {
    /* MOD_VEX_72_REG_6 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_72_REG_6) },
  },
  {
    /* MOD_VEX_73_REG_2 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_73_REG_2) },
  },
  {
    /* MOD_VEX_73_REG_3 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_73_REG_3) },
  },
  {
    /* MOD_VEX_73_REG_6 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_73_REG_6) },
  },
  {
    /* MOD_VEX_73_REG_7 */
    { "(bad)",		{ XX } },
    { PREFIX_TABLE (PREFIX_VEX_73_REG_7) },
  },
  {
    /* MOD_VEX_AE_REG_2 */
    { VEX_LEN_TABLE (VEX_LEN_AE_R_2_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_AE_REG_3 */
    { VEX_LEN_TABLE (VEX_LEN_AE_R_3_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_D7_PREFIX_2 */
    { "(bad)",		{ XX } },
    { VEX_LEN_TABLE (VEX_LEN_D7_P_2_M_1) },
  },
  {
    /* MOD_VEX_E7_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_E7_P_2_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_F0_PREFIX_3 */
    { "vlddqu",		{ XM, M } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_3818_PREFIX_2 */
    { "vbroadcastss",	{ XM, Md } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_3819_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_3819_P_2_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_381A_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_381A_P_2_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_382A_PREFIX_2 */
    { VEX_LEN_TABLE (VEX_LEN_382A_P_2_M_0) },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_382C_PREFIX_2 */
    { "vmaskmovps",	{ XM, Vex, Mx } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_382D_PREFIX_2 */
    { "vmaskmovpd",	{ XM, Vex, Mx } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_382E_PREFIX_2 */
    { "vmaskmovps",	{ Mx, Vex, XM } },
    { "(bad)",		{ XX } },
  },
  {
    /* MOD_VEX_382F_PREFIX_2 */
    { "vmaskmovpd",	{ Mx, Vex, XM } },
    { "(bad)",		{ XX } },
  },
};

static const struct dis386 rm_table[][8] = {
  {
    /* RM_0F01_REG_0 */
    { "(bad)",		{ XX } },
    { "vmcall",		{ Skip_MODRM } },
    { "vmlaunch",	{ Skip_MODRM } },
    { "vmresume",	{ Skip_MODRM } },
    { "vmxoff",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0F01_REG_1 */
    { "monitor",	{ { OP_Monitor, 0 } } },
    { "mwait",		{ { OP_Mwait, 0 } } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0F01_REG_2 */
    { "xgetbv",		{ Skip_MODRM } },
    { "xsetbv",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0F01_REG_3 */
    { "vmrun",		{ Skip_MODRM } },
    { "vmmcall",	{ Skip_MODRM } },
    { "vmload",		{ Skip_MODRM } },
    { "vmsave",		{ Skip_MODRM } },
    { "stgi",		{ Skip_MODRM } },
    { "clgi",		{ Skip_MODRM } },
    { "skinit",		{ Skip_MODRM } },
    { "invlpga",	{ Skip_MODRM } },
  },
  {
    /* RM_0F01_REG_7 */
    { "swapgs",		{ Skip_MODRM } },
    { "rdtscp",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0FAE_REG_5 */
    { "lfence",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0FAE_REG_6 */
    { "mfence",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
  {
    /* RM_0FAE_REG_7 */
    { "sfence",		{ Skip_MODRM } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
    { "(bad)",		{ XX } },
  },
};

#define INTERNAL_DISASSEMBLER_ERROR _("<internal disassembler error>")

static void
ckprefix (void)
{
  int newrex;
  rex = 0;
  rex_original = 0;
  rex_ignored = 0;
  prefixes = 0;
  used_prefixes = 0;
  rex_used = 0;
  while (1)
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
	      return;
	  break;
	case 0xf3:
	  prefixes |= PREFIX_REPZ;
	  break;
	case 0xf2:
	  prefixes |= PREFIX_REPNZ;
	  break;
	case 0xf0:
	  prefixes |= PREFIX_LOCK;
	  break;
	case 0x2e:
	  prefixes |= PREFIX_CS;
	  break;
	case 0x36:
	  prefixes |= PREFIX_SS;
	  break;
	case 0x3e:
	  prefixes |= PREFIX_DS;
	  break;
	case 0x26:
	  prefixes |= PREFIX_ES;
	  break;
	case 0x64:
	  prefixes |= PREFIX_FS;
	  break;
	case 0x65:
	  prefixes |= PREFIX_GS;
	  break;
	case 0x66:
	  prefixes |= PREFIX_DATA;
	  break;
	case 0x67:
	  prefixes |= PREFIX_ADDR;
	  break;
	case FWAIT_OPCODE:
	  /* fwait is really an instruction.  If there are prefixes
	     before the fwait, they belong to the fwait, *not* to the
	     following instruction.  */
	  if (prefixes || rex)
	    {
	      prefixes |= PREFIX_FWAIT;
	      codep++;
	      return;
	    }
	  prefixes = PREFIX_FWAIT;
	  break;
	default:
	  return;
	}
      /* Rex is ignored when followed by another prefix.  */
      if (rex)
	{
	  rex_used = rex;
	  return;
	}
      rex = newrex;
      rex_original = rex;
      codep++;
    }
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
}

/* Get a pointer to struct dis386 with a valid name.  */

static const struct dis386 *
get_valid_dis386 (const struct dis386 *dp, disassemble_info *info)
{
  int index, vex_table_index;

  if (dp->name != NULL)
    return dp;

  switch (dp->op[0].bytemode)
    {
    case USE_REG_TABLE:
      dp = &reg_table[dp->op[1].bytemode][modrm.reg];
      break;

    case USE_MOD_TABLE:
      index = modrm.mod == 0x3 ? 1 : 0;
      dp = &mod_table[dp->op[1].bytemode][index];
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
	      index = 0;
	      break;
	    case REPE_PREFIX_OPCODE:
	      index = 1;
	      break;
	    case DATA_PREFIX_OPCODE:
	      index = 2;
	      break;
	    case REPNE_PREFIX_OPCODE:
	      index = 3;
	      break;
	    default:
	      abort ();
	      break;
	    }
	}
      else 
	{
	  index = 0;
	  used_prefixes |= (prefixes & PREFIX_REPZ);
	  if (prefixes & PREFIX_REPZ)
	    {
	      index = 1;
	      repz_prefix = NULL;
	    }
	  else
	    {
	      /* We should check PREFIX_REPNZ and PREFIX_REPZ before
		 PREFIX_DATA.  */
	      used_prefixes |= (prefixes & PREFIX_REPNZ);
	      if (prefixes & PREFIX_REPNZ)
		{
		  index = 3;
		  repnz_prefix = NULL;
		}
	      else
		{
		  used_prefixes |= (prefixes & PREFIX_DATA);
		  if (prefixes & PREFIX_DATA)
		    {
		      index = 2;
		      data_prefix = NULL;
		    }
		}
	    }
	}
      dp = &prefix_table[dp->op[1].bytemode][index];
      break;

    case USE_X86_64_TABLE:
      index = address_mode == mode_64bit ? 1 : 0;
      dp = &x86_64_table[dp->op[1].bytemode][index];
      break;

    case USE_3BYTE_TABLE:
      FETCH_DATA (info, codep + 2);
      index = *codep++;
      dp = &three_byte_table[dp->op[1].bytemode][index];
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
	  index = 0;
	  break;
	case 256:
	  index = 1;
	  break;
	default:
	  abort ();
	  break;
	}

      dp = &vex_len_table[dp->op[1].bytemode][index];
      break;

    case USE_VEX_C4_TABLE:
      FETCH_DATA (info, codep + 3);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      rex = ~(*codep >> 5) & 0x7;
      switch ((*codep & 0x1f))
	{
	default:
	  BadOp ();
	case 0x1:
	  vex_table_index = 0;
	  break;
	case 0x2:
	  vex_table_index = 1;
	  break;
	case 0x3:
	  vex_table_index = 2;
	  break;
	}
      codep++;
      vex.w = *codep & 0x80;
      if (vex.w && address_mode == mode_64bit)
	rex |= REX_W;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit
	  && vex.register_specifier > 0x7)
	BadOp ();

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
      index = *codep++;
      dp = &vex_table[vex_table_index][index];
      /* There is no MODRM byte for VEX [82|77].  */
      if (index != 0x77 && index != 0x82)
	{
	  FETCH_DATA (info, codep + 1);
	  modrm.mod = (*codep >> 6) & 3;
	  modrm.reg = (*codep >> 3) & 7;
	  modrm.rm = *codep & 7;
	}
      break;

    case USE_VEX_C5_TABLE:
      FETCH_DATA (info, codep + 2);
      /* All bits in the REX prefix are ignored.  */
      rex_ignored = rex;
      rex = (*codep & 0x80) ? 0 : REX_R;

      vex.register_specifier = (~(*codep >> 3)) & 0xf;
      if (address_mode != mode_64bit
	  && vex.register_specifier > 0x7)
	BadOp ();

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
      index = *codep++;
      dp = &vex_table[dp->op[1].bytemode][index];
      /* There is no MODRM byte for VEX [82|77].  */
      if (index != 0x77 && index != 0x82)
	{
	  FETCH_DATA (info, codep + 1);
	  modrm.mod = (*codep >> 6) & 3;
	  modrm.reg = (*codep >> 3) & 7;
	  modrm.rm = *codep & 7;
	}
      break;

    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return NULL;
    }

  if (dp->name != NULL)
    return dp;
  else
    return get_valid_dis386 (dp, info);
}

static int
print_insn (bfd_vma pc, disassemble_info *info)
{
  const struct dis386 *dp;
  int i;
  char *op_txt[MAX_OPERANDS];
  int needcomma;
  int sizeflag;
  const char *p;
  struct dis_private priv;
  unsigned char op;
  char prefix_obuf[32];
  char *prefix_obufp;

  if (info->mach == bfd_mach_x86_64_intel_syntax
      || info->mach == bfd_mach_x86_64)
    address_mode = mode_64bit;
  else
    address_mode = mode_32bit;

  if (intel_syntax == (char) -1)
    intel_syntax = (info->mach == bfd_mach_i386_i386_intel_syntax
		    || info->mach == bfd_mach_x86_64_intel_syntax);

  if (info->mach == bfd_mach_i386_i386
      || info->mach == bfd_mach_x86_64
      || info->mach == bfd_mach_i386_i386_intel_syntax
      || info->mach == bfd_mach_x86_64_intel_syntax)
    priv.orig_sizeflag = AFLAG | DFLAG;
  else if (info->mach == bfd_mach_i386_i8086)
    priv.orig_sizeflag = 0;
  else
    abort ();

  for (p = info->disassembler_options; p != NULL; )
    {
      if (CONST_STRNEQ (p, "x86-64"))
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
      index64 = intel_index64;
      index32 = intel_index32;
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
      index64 = att_index64;
      index32 = att_index32;
      index16 = att_index16;
      open_char = '(';
      close_char =  ')';
      separator_char = ',';
      scale_char = ',';
    }

  /* The output looks better if we put 7 bytes on a line, since that
     puts most long word instructions on a single line.  */
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

  if (setjmp (priv.bailout) != 0)
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
  ckprefix ();

  insn_codep = codep;
  sizeflag = priv.orig_sizeflag;

  FETCH_DATA (info, codep + 1);
  two_source_ops = (*codep == 0x62) || (*codep == 0xc8);

  if (((prefixes & PREFIX_FWAIT)
       && ((*codep < 0xd8) || (*codep > 0xdf)))
      || (rex && rex_used))
    {
      const char *name;

      /* fwait not followed by floating point instruction, or rex followed
	 by other prefixes.  Print the first prefix.  */
      name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s", name);
      return 1;
    }

  op = 0;
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

  if ((prefixes & PREFIX_REPZ))
    {
      repz_prefix = "repz ";
      used_prefixes |= PREFIX_REPZ;
    }
  else
    repz_prefix = NULL;

  if ((prefixes & PREFIX_REPNZ))
    {
      repnz_prefix = "repnz ";
      used_prefixes |= PREFIX_REPNZ;
    }
  else
    repnz_prefix = NULL;

  if ((prefixes & PREFIX_LOCK))
    {
      lock_prefix = "lock ";
      used_prefixes |= PREFIX_LOCK;
    }
  else
    lock_prefix = NULL;

  addr_prefix = NULL;
  if (prefixes & PREFIX_ADDR)
    {
      sizeflag ^= AFLAG;
      if (dp->op[2].bytemode != loop_jcxz_mode || intel_syntax)
	{
	  if ((sizeflag & AFLAG) || address_mode == mode_64bit)
	    addr_prefix = "addr32 ";
	  else
	    addr_prefix = "addr16 ";
	  used_prefixes |= PREFIX_ADDR;
	}
    }

  data_prefix = NULL;
  if ((prefixes & PREFIX_DATA))
    {
      sizeflag ^= DFLAG;
      if (dp->op[2].bytemode == cond_jump_mode
	  && dp->op[0].bytemode == v_mode
	  && !intel_syntax)
	{
	  if (sizeflag & DFLAG)
	    data_prefix = "data32 ";
	  else
	    data_prefix = "data16 ";
	  used_prefixes |= PREFIX_DATA;
	}
    }

  if (need_modrm)
    {
      FETCH_DATA (info, codep + 1);
      modrm.mod = (*codep >> 6) & 3;
      modrm.reg = (*codep >> 3) & 7;
      modrm.rm = *codep & 7;
    }

  if (dp->name == NULL && dp->op[0].bytemode == FLOATCODE)
    {
      dofloat (sizeflag);
    }
  else
    {
      need_vex = 0;
      need_vex_reg = 0;
      vex_w_done = 0;
      dp = get_valid_dis386 (dp, info);
      if (dp != NULL && putop (dp->name, sizeflag) == 0)
        {
	  for (i = 0; i < MAX_OPERANDS; ++i)
	    {
	      obufp = op_out[i];
	      op_ad = MAX_OPERANDS - 1 - i;
	      if (dp->op[i].rtn)
		(*dp->op[i].rtn) (dp->op[i].bytemode, sizeflag);
	    }
	}
    }

  /* See if any prefixes were not used.  If so, print the first one
     separately.  If we don't do this, we'll wind up printing an
     instruction stream which does not precisely correspond to the
     bytes we are disassembling.  */
  if ((prefixes & ~used_prefixes) != 0)
    {
      const char *name;

      name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s", name);
      return 1;
    }
  if ((rex_original & ~rex_used) || rex_ignored)
    {
      const char *name;
      name = prefix_name (rex_original, priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s ", name);
    }

  prefix_obuf[0] = 0;
  prefix_obufp = prefix_obuf;
  if (lock_prefix)
    prefix_obufp = stpcpy (prefix_obufp, lock_prefix);
  if (repz_prefix)
    prefix_obufp = stpcpy (prefix_obufp, repz_prefix);
  if (repnz_prefix)
    prefix_obufp = stpcpy (prefix_obufp, repnz_prefix);
  if (addr_prefix)
    prefix_obufp = stpcpy (prefix_obufp, addr_prefix);
  if (data_prefix)
    prefix_obufp = stpcpy (prefix_obufp, data_prefix);

  if (prefix_obuf[0] != 0)
    (*info->fprintf_func) (info->stream, "%s", prefix_obuf);

  obufp = obuf + strlen (obuf);
  for (i = strlen (obuf) + strlen (prefix_obuf); i < 6; i++)
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

#define FGRPd9_2 NULL, { { NULL, 0 } }
#define FGRPd9_4 NULL, { { NULL, 1 } }
#define FGRPd9_5 NULL, { { NULL, 2 } }
#define FGRPd9_6 NULL, { { NULL, 3 } }
#define FGRPd9_7 NULL, { { NULL, 4 } }
#define FGRPda_5 NULL, { { NULL, 5 } }
#define FGRPdb_4 NULL, { { NULL, 6 } }
#define FGRPde_3 NULL, { { NULL, 7 } }
#define FGRPdf_4 NULL, { { NULL, 8 } }

static const struct dis386 float_reg[][8] = {
  /* d8 */
  {
    { "fadd",	{ ST, STi } },
    { "fmul",	{ ST, STi } },
    { "fcom",	{ STi } },
    { "fcomp",	{ STi } },
    { "fsub",	{ ST, STi } },
    { "fsubr",	{ ST, STi } },
    { "fdiv",	{ ST, STi } },
    { "fdivr",	{ ST, STi } },
  },
  /* d9 */
  {
    { "fld",	{ STi } },
    { "fxch",	{ STi } },
    { FGRPd9_2 },
    { "(bad)",	{ XX } },
    { FGRPd9_4 },
    { FGRPd9_5 },
    { FGRPd9_6 },
    { FGRPd9_7 },
  },
  /* da */
  {
    { "fcmovb",	{ ST, STi } },
    { "fcmove",	{ ST, STi } },
    { "fcmovbe",{ ST, STi } },
    { "fcmovu",	{ ST, STi } },
    { "(bad)",	{ XX } },
    { FGRPda_5 },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* db */
  {
    { "fcmovnb",{ ST, STi } },
    { "fcmovne",{ ST, STi } },
    { "fcmovnbe",{ ST, STi } },
    { "fcmovnu",{ ST, STi } },
    { FGRPdb_4 },
    { "fucomi",	{ ST, STi } },
    { "fcomi",	{ ST, STi } },
    { "(bad)",	{ XX } },
  },
  /* dc */
  {
    { "fadd",	{ STi, ST } },
    { "fmul",	{ STi, ST } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "fsub!M",	{ STi, ST } },
    { "fsubM",	{ STi, ST } },
    { "fdiv!M",	{ STi, ST } },
    { "fdivM",	{ STi, ST } },
  },
  /* dd */
  {
    { "ffree",	{ STi } },
    { "(bad)",	{ XX } },
    { "fst",	{ STi } },
    { "fstp",	{ STi } },
    { "fucom",	{ STi } },
    { "fucomp",	{ STi } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
  },
  /* de */
  {
    { "faddp",	{ STi, ST } },
    { "fmulp",	{ STi, ST } },
    { "(bad)",	{ XX } },
    { FGRPde_3 },
    { "fsub!Mp", { STi, ST } },
    { "fsubMp",	{ STi, ST } },
    { "fdiv!Mp", { STi, ST } },
    { "fdivMp",	{ STi, ST } },
  },
  /* df */
  {
    { "ffreep",	{ STi } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { "(bad)",	{ XX } },
    { FGRPdf_4 },
    { "fucomip", { ST, STi } },
    { "fcomip", { ST, STi } },
    { "(bad)",	{ XX } },
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
    "feni(287 only)","fdisi(287 only)","fNclex","fNinit",
    "fNsetpm(287 only)","(bad)","(bad)","(bad)",
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

static void
OP_ST (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  oappend ("%st" + intel_syntax);
}

static void
OP_STi (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  sprintf (scratchbuf, "%%st(%d)", modrm.rm);
  oappend (scratchbuf + intel_syntax);
}

/* Capital letters in template are macros.  */
static int
putop (const char *template, int sizeflag)
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

  for (p = template; *p; p++)
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
	  if (intel_syntax)
	    break;
	  if (sizeflag & SUFFIX_ALWAYS)
	    *obufp++ = 'b';
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
	      else if (sizeflag & DFLAG)
		*obufp++ = intel_syntax ? 'd' : 'l';
	      else
		*obufp++ = 'w';
	      used_prefixes |= (prefixes & PREFIX_DATA);
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
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit && (sizeflag & DFLAG))
	    {
	      *obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	case 'P':
	  if (intel_syntax)
	    break;
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
		}
	      used_prefixes |= (prefixes & PREFIX_DATA);
	    }
	  break;
	case 'U':
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit && (sizeflag & DFLAG))
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
		    }
		  used_prefixes |= (prefixes & PREFIX_DATA);
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
	  if (intel_syntax)
	    break;
	  if (address_mode == mode_64bit && (sizeflag & DFLAG))
	    {
	      if (sizeflag & SUFFIX_ALWAYS)
		*obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	case 'S':
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
	  else if (prefixes & PREFIX_DATA)
	    *obufp++ = 'd';
	  else
	    *obufp++ = 's';
	  used_prefixes |= (prefixes & PREFIX_DATA);
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
		  || (modrm.mod == 3 && !(sizeflag & SUFFIX_ALWAYS)))
		break;
	      switch (vex.length)
		{
		case 128:
		  *obufp++ = 'x';
		  break;
		case 256:
		  *obufp++ = 'y';
		  break;
		default:
		  abort ();
		}
	    }
	  break;
	  /* implicit operand size 'l' for i386 or 'q' for x86-64 */
	case 'W':
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
	  break;
	}
      alt = 0;
    }
  *obufp = 0;
  return 0;
}

static void
oappend (const char *s)
{
  strcpy (obufp, s);
  obufp += strlen (s);
}

static void
append_seg (void)
{
  if (prefixes & PREFIX_CS)
    {
      used_prefixes |= PREFIX_CS;
      oappend ("%cs:" + intel_syntax);
    }
  if (prefixes & PREFIX_DS)
    {
      used_prefixes |= PREFIX_DS;
      oappend ("%ds:" + intel_syntax);
    }
  if (prefixes & PREFIX_SS)
    {
      used_prefixes |= PREFIX_SS;
      oappend ("%ss:" + intel_syntax);
    }
  if (prefixes & PREFIX_ES)
    {
      used_prefixes |= PREFIX_ES;
      oappend ("%es:" + intel_syntax);
    }
  if (prefixes & PREFIX_FS)
    {
      used_prefixes |= PREFIX_FS;
      oappend ("%fs:" + intel_syntax);
    }
  if (prefixes & PREFIX_GS)
    {
      used_prefixes |= PREFIX_GS;
      oappend ("%gs:" + intel_syntax);
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
  switch (bytemode)
    {
    case b_mode:
    case dqb_mode:
      oappend ("BYTE PTR ");
      break;
    case w_mode:
    case dqw_mode:
      oappend ("WORD PTR ");
      break;
    case stack_v_mode:
      if (address_mode == mode_64bit && (sizeflag & DFLAG))
	{
	  oappend ("QWORD PTR ");
	  used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	}
      /* FALLTHRU */
    case v_mode:
    case dq_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	oappend ("QWORD PTR ");
      else if ((sizeflag & DFLAG) || bytemode == dq_mode)
	oappend ("DWORD PTR ");
      else
	oappend ("WORD PTR ");
      used_prefixes |= (prefixes & PREFIX_DATA);
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
    case dqd_mode:
      oappend ("DWORD PTR ");
      break;
    case q_mode:
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
    case xmmq_mode:
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
	default:
	  abort ();
	}
      break;
    case o_mode:
      oappend ("OWORD PTR ");
      break;
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

  switch (bytemode)
    {
    case b_mode:
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
      names = names32;
      break;
    case q_mode:
      names = names64;
      break;
    case m_mode:
      names = address_mode == mode_64bit ? names64 : names32;
      break;
    case stack_v_mode:
      if (address_mode == mode_64bit && (sizeflag & DFLAG))
	{
	  names = names64;
	  used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	}
      bytemode = v_mode;
      /* FALLTHRU */
    case v_mode:
    case dq_mode:
    case dqb_mode:
    case dqd_mode:
    case dqw_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	names = names64;
      else if ((sizeflag & DFLAG) || bytemode != v_mode)
	names = names32;
      else
	names = names16;
      used_prefixes |= (prefixes & PREFIX_DATA);
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
OP_E_memory (int bytemode, int sizeflag, int has_drex)
{
  bfd_vma disp = 0;
  int add = (rex & REX_B) ? 8 : 0;
  int riprel = 0;

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
      int index = 0;
      int scale = 0;

      havesib = 0;
      havebase = 1;
      haveindex = 0;
      base = modrm.rm;

      if (base == 4)
	{
	  havesib = 1;
	  FETCH_DATA (the_info, codep + 1);
	  index = (*codep >> 3) & 7;
	  scale = (*codep >> 6) & 3;
	  base = *codep & 7;
	  USED_REX (REX_X);
	  if (rex & REX_X)
	    index += 8;
	  haveindex = index != 4;
	  codep++;
	}
      rbase = base + add;

      /* If we have a DREX byte, skip it now 
	 (it has already been handled) */
      if (has_drex)
	{
	  FETCH_DATA (the_info, codep + 1);
	  codep++;
	}

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

      if (havebase || haveindex || riprel)
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
	    oappend (address_mode == mode_64bit && (sizeflag & AFLAG)
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
		    oappend (address_mode == mode_64bit 
			     && (sizeflag & AFLAG)
			     ? names64[index] : names32[index]);
		  else
		    oappend (address_mode == mode_64bit 
			     && (sizeflag & AFLAG)
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
	      else if (modrm.mod != 1)
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
	      if (prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
			      | PREFIX_ES | PREFIX_FS | PREFIX_GS))
		;
	      else
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
    { /* 16 bit address mode */
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
	  if (prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
			  | PREFIX_ES | PREFIX_FS | PREFIX_GS))
	    ;
	  else
	    {
	      oappend (names_seg[ds_reg - es_reg]);
	      oappend (":");
	    }
	  print_operand_value (scratchbuf, 1, disp & 0xffff);
	  oappend (scratchbuf);
	}
    }
}

static void
OP_E_extended (int bytemode, int sizeflag, int has_drex)
{
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  if (modrm.mod == 3)
    OP_E_register (bytemode, sizeflag);
  else
    OP_E_memory (bytemode, sizeflag, has_drex);
}

static void
OP_E (int bytemode, int sizeflag)
{
  OP_E_extended (bytemode, sizeflag, 0);
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
      oappend (names32[modrm.reg + add]);
      break;
    case q_mode:
      oappend (names64[modrm.reg + add]);
      break;
    case v_mode:
    case dq_mode:
    case dqb_mode:
    case dqd_mode:
    case dqw_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	oappend (names64[modrm.reg + add]);
      else if ((sizeflag & DFLAG) || bytemode != v_mode)
	oappend (names32[modrm.reg + add]);
      else
	oappend (names16[modrm.reg + add]);
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case m_mode:
      if (address_mode == mode_64bit)
	oappend (names64[modrm.reg + add]);
      else
	oappend (names32[modrm.reg + add]);
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
    case es_reg: case ss_reg: case cs_reg:
    case ds_reg: case fs_reg: case gs_reg:
      s = names_seg[code - es_reg + add];
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
      if (address_mode == mode_64bit && (sizeflag & DFLAG))
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
      else if (sizeflag & DFLAG)
	s = names32[code - eAX_reg + add];
      else
	s = names16[code - eAX_reg + add];
      used_prefixes |= (prefixes & PREFIX_DATA);
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
      else if (sizeflag & DFLAG)
	s = names32[code - eAX_reg];
      else
	s = names16[code - eAX_reg];
      used_prefixes |= (prefixes & PREFIX_DATA);
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
      else if (sizeflag & DFLAG)
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
  oappend (scratchbuf + intel_syntax);
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
      else if (sizeflag & DFLAG)
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
  oappend (scratchbuf + intel_syntax);
  scratchbuf[0] = '\0';
}

static void
OP_sI (int bytemode, int sizeflag)
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      if ((op & 0x80) != 0)
	op -= 0x100;
      mask = 0xffffffff;
      break;
    case v_mode:
      USED_REX (REX_W);
      if (rex & REX_W)
	op = get32s ();
      else if (sizeflag & DFLAG)
	{
	  op = get32s ();
	  mask = 0xffffffff;
	}
      else
	{
	  mask = 0xffffffff;
	  op = get16 ();
	  if ((op & 0x8000) != 0)
	    op -= 0x10000;
	}
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case w_mode:
      op = get16 ();
      mask = 0xffffffff;
      if ((op & 0x8000) != 0)
	op -= 0x10000;
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend (scratchbuf + intel_syntax);
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
      if ((sizeflag & DFLAG) || (rex & REX_W))
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
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }
  disp = ((start_pc + codep - start_codep + disp) & mask) | segment;
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
      if (!(prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
			| PREFIX_ES | PREFIX_FS | PREFIX_GS)))
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
      if (!(prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
			| PREFIX_ES | PREFIX_FS | PREFIX_GS)))
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
  oappend ("%es:" + intel_syntax);
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
  if ((prefixes
       & (PREFIX_CS
	  | PREFIX_DS
	  | PREFIX_SS
	  | PREFIX_ES
	  | PREFIX_FS
	  | PREFIX_GS)) == 0)
    prefixes |= PREFIX_DS;
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
      lock_prefix = NULL;
      used_prefixes |= PREFIX_LOCK;
      add = 8;
    }
  else
    add = 0;
  sprintf (scratchbuf, "%%cr%d", modrm.reg + add);
  oappend (scratchbuf + intel_syntax);
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
  oappend (scratchbuf + intel_syntax);
}

static void
OP_R (int bytemode, int sizeflag)
{
  if (modrm.mod == 3)
    OP_E (bytemode, sizeflag);
  else
    BadOp ();
}

static void
OP_MMX (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  used_prefixes |= (prefixes & PREFIX_DATA);
  if (prefixes & PREFIX_DATA)
    {
      int add;
      USED_REX (REX_R);
      if (rex & REX_R)
	add = 8;
      else
	add = 0;
      sprintf (scratchbuf, "%%xmm%d", modrm.reg + add);
    }
  else
    sprintf (scratchbuf, "%%mm%d", modrm.reg);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_XMM (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  int add;
  USED_REX (REX_R);
  if (rex & REX_R)
    add = 8;
  else
    add = 0;
  if (need_vex && bytemode != xmm_mode)
    {
      switch (vex.length)
	{
	case 128:
	  sprintf (scratchbuf, "%%xmm%d", modrm.reg + add);
	  break;
	case 256:
	  sprintf (scratchbuf, "%%ymm%d", modrm.reg + add);
	  break;
	default:
	  abort ();
	}
    }
  else
    sprintf (scratchbuf, "%%xmm%d", modrm.reg + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_EM (int bytemode, int sizeflag)
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
  if (prefixes & PREFIX_DATA)
    {
      int add;

      USED_REX (REX_B);
      if (rex & REX_B)
	add = 8;
      else
	add = 0;
      sprintf (scratchbuf, "%%xmm%d", modrm.rm + add);
    }
  else
    sprintf (scratchbuf, "%%mm%d", modrm.rm);
  oappend (scratchbuf + intel_syntax);
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
  sprintf (scratchbuf, "%%mm%d", modrm.rm);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_MXC (int bytemode ATTRIBUTE_UNUSED, int sizeflag ATTRIBUTE_UNUSED)
{
  used_prefixes |= (prefixes & PREFIX_DATA);
  sprintf (scratchbuf, "%%mm%d", modrm.reg);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_EX (int bytemode, int sizeflag)
{
  int add;
  if (modrm.mod != 3)
    {
      OP_E (bytemode, sizeflag);
      return;
    }
  USED_REX (REX_B);
  if (rex & REX_B)
    add = 8;
  else
    add = 0;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  if (need_vex
      && bytemode != xmm_mode
      && bytemode != xmmq_mode)
    {
      switch (vex.length)
	{
	case 128:
	  sprintf (scratchbuf, "%%xmm%d", modrm.rm + add);
	  break;
	case 256:
	  sprintf (scratchbuf, "%%ymm%d", modrm.rm + add);
	  break;
	default:
	  abort ();
	}
    }
  else
    sprintf (scratchbuf, "%%xmm%d", modrm.rm + add);
  oappend (scratchbuf + intel_syntax);
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
  obufp = obuf + strlen (obuf);
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
}

static const char *simd_cmp_op[] = {
  "eq",
  "lt",
  "le",
  "unord",
  "neq",
  "nlt",
  "nle",
  "ord"
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
      char *p = obuf + strlen (obuf) - 2;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = '\0';
      sprintf (p, "%s%s", simd_cmp_op[cmp_type], suffix);
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, cmp_type);
      oappend (scratchbuf + intel_syntax);
      scratchbuf[0] = '\0';
    }
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
	  addr_prefix = NULL;
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
    repz_prefix = "rep ";

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

static void
CMPXCHG8B_Fixup (int bytemode, int sizeflag)
{
  USED_REX (REX_W);
  if (rex & REX_W)
    {
      /* Change cmpxchg8b to cmpxchg16b.  */
      char *p = obuf + strlen (obuf) - 2;
      strcpy (p, "16b");
      bytemode = o_mode;
    }
  OP_M (bytemode, sizeflag);
}

static void
XMM_Fixup (int reg, int sizeflag ATTRIBUTE_UNUSED)
{
  if (need_vex)
    {
      switch (vex.length)
	{
	case 128:
	  sprintf (scratchbuf, "%%xmm%d", reg);
	  break;
	case 256:
	  sprintf (scratchbuf, "%%ymm%d", reg);
	  break;
	default:
	  abort ();
	}
    }
  else
    sprintf (scratchbuf, "%%xmm%d", reg);
  oappend (scratchbuf + intel_syntax);
}

static void
CRC32_Fixup (int bytemode, int sizeflag)
{
  /* Add proper suffix to "crc32".  */
  char *p = obuf + strlen (obuf);

  switch (bytemode)
    {
    case b_mode:
      if (intel_syntax)
	break;

      *p++ = 'b';
      break;
    case v_mode:
      if (intel_syntax)
	break;

      USED_REX (REX_W);
      if (rex & REX_W)
	*p++ = 'q';
      else if (sizeflag & DFLAG)
	*p++ = 'l';
      else
	*p++ = 'w';
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
  *p = '\0';

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

/* Print a DREX argument as either a register or memory operation.  */
static void
print_drex_arg (unsigned int reg, int bytemode, int sizeflag)
{
  if (reg == DREX_REG_UNKNOWN)
    BadOp ();

  else if (reg != DREX_REG_MEMORY)
    {
      sprintf (scratchbuf, "%%xmm%d", reg);
      oappend (scratchbuf + intel_syntax);
    }

  else
    OP_E_extended (bytemode, sizeflag, 1);
}

/* SSE5 instructions that have 4 arguments are encoded as:
   0f 24 <sub-opcode> <modrm> <optional-sib> <drex> <offset>.

   The <sub-opcode> byte has 1 bit (0x4) that is combined with 1 bit in
   the DREX field (0x8) to determine how the arguments are laid out.  
   The destination register must be the same register as one of the 
   inputs, and it is encoded in the DREX byte.  No REX prefix is used 
   for these instructions, since the DREX field contains the 3 extension
   bits provided by the REX prefix.

   The bytemode argument adds 2 extra bits for passing extra information:
	DREX_OC1	-- Set the OC1 bit to indicate dest == 1st arg
	DREX_NO_OC0	-- OC0 in DREX is invalid 
	(but pretend it is set).  */

static void
OP_DREX4 (int flag_bytemode, int sizeflag)
{
  unsigned int drex_byte;
  unsigned int regs[4];
  unsigned int modrm_regmem;
  unsigned int modrm_reg;
  unsigned int drex_reg;
  int bytemode;
  int rex_save = rex;
  int rex_used_save = rex_used;
  int has_sib = 0;
  int oc1 = (flag_bytemode & DREX_OC1) ? 2 : 0;
  int oc0;
  int i;

  bytemode = flag_bytemode & ~ DREX_MASK;

  for (i = 0; i < 4; i++)
    regs[i] = DREX_REG_UNKNOWN;

  /* Determine if we have a SIB byte in addition to MODRM before the 
     DREX byte.  */
  if (((sizeflag & AFLAG) || address_mode == mode_64bit)
      && (modrm.mod != 3)
      && (modrm.rm == 4))
    has_sib = 1;

  /* Get the DREX byte.  */
  FETCH_DATA (the_info, codep + 2 + has_sib);
  drex_byte = codep[has_sib+1];
  drex_reg = DREX_XMM (drex_byte);
  modrm_reg = modrm.reg + ((drex_byte & REX_R) ? 8 : 0);

  /* Is OC0 legal?  If not, hardwire oc0 == 1.  */
  if (flag_bytemode & DREX_NO_OC0)
    {
      oc0 = 1;
      if (DREX_OC0 (drex_byte))
	BadOp ();
    }
  else
    oc0 = DREX_OC0 (drex_byte);

  if (modrm.mod == 3)
    {			
      /* regmem == register  */
      modrm_regmem = modrm.rm + ((drex_byte & REX_B) ? 8 : 0);
      rex = rex_used = 0;
      /* skip modrm/drex since we don't call OP_E_extended  */
      codep += 2;
    }
  else
    {			
      /* regmem == memory, fill in appropriate REX bits  */
      modrm_regmem = DREX_REG_MEMORY;
      rex = drex_byte & (REX_B | REX_X | REX_R);
      if (rex)
	rex |= REX_OPCODE;
      rex_used = rex;
    }
  
  /* Based on the OC1/OC0 bits, lay out the arguments in the correct 
     order.  */
  switch (oc0 + oc1)
    {
    default:
      BadOp ();
      return;

    case 0:
      regs[0] = modrm_regmem;
      regs[1] = modrm_reg;
      regs[2] = drex_reg;
      regs[3] = drex_reg;
      break;

    case 1:
      regs[0] = modrm_reg;
      regs[1] = modrm_regmem;
      regs[2] = drex_reg;
      regs[3] = drex_reg;
      break;

    case 2:
      regs[0] = drex_reg;
      regs[1] = modrm_regmem;
      regs[2] = modrm_reg;
      regs[3] = drex_reg;
      break;

    case 3:
      regs[0] = drex_reg;
      regs[1] = modrm_reg;
      regs[2] = modrm_regmem;
      regs[3] = drex_reg;
      break;
    }

  /* Print out the arguments.  */
  for (i = 0; i < 4; i++)
    {
      int j = (intel_syntax) ? 3 - i : i;
      if (i > 0)
	{
	  *obufp++ = ',';
	  *obufp = '\0';
	}

      print_drex_arg (regs[j], bytemode, sizeflag);
    }

  rex = rex_save;
  rex_used = rex_used_save;
}

/* SSE5 instructions that have 3 arguments, and are encoded as:
   0f 24 <sub-opcode> <modrm> <optional-sib> <drex> <offset>	(or)
   0f 25 <sub-opcode> <modrm> <optional-sib> <drex> <offset> <cmp-byte>

   The DREX field has 1 bit (0x8) to determine how the arguments are 
   laid out. The destination register is encoded in the DREX byte.  
   No REX prefix is used for these instructions, since the DREX field 
   contains the 3 extension bits provided by the REX prefix.  */

static void
OP_DREX3 (int flag_bytemode, int sizeflag)
{
  unsigned int drex_byte;
  unsigned int regs[3];
  unsigned int modrm_regmem;
  unsigned int modrm_reg;
  unsigned int drex_reg;
  int bytemode;
  int rex_save = rex;
  int rex_used_save = rex_used;
  int has_sib = 0;
  int oc0;
  int i;

  bytemode = flag_bytemode & ~ DREX_MASK;

  for (i = 0; i < 3; i++)
    regs[i] = DREX_REG_UNKNOWN;

  /* Determine if we have a SIB byte in addition to MODRM before the 
     DREX byte.  */
  if (((sizeflag & AFLAG) || address_mode == mode_64bit)
      && (modrm.mod != 3)
      && (modrm.rm == 4))
    has_sib = 1;

  /* Get the DREX byte.  */
  FETCH_DATA (the_info, codep + 2 + has_sib);
  drex_byte = codep[has_sib+1];
  drex_reg = DREX_XMM (drex_byte);
  modrm_reg = modrm.reg + ((drex_byte & REX_R) ? 8 : 0);

  /* Is OC0 legal?  If not, hardwire oc0 == 0 */
  oc0 = DREX_OC0 (drex_byte);
  if ((flag_bytemode & DREX_NO_OC0) && oc0)
    BadOp ();

  if (modrm.mod == 3)
    {			
      /* regmem == register */
      modrm_regmem = modrm.rm + ((drex_byte & REX_B) ? 8 : 0);
      rex = rex_used = 0;
      /* skip modrm/drex since we don't call OP_E_extended.  */
      codep += 2;
    }
  else
    {			
      /* regmem == memory, fill in appropriate REX bits.  */
      modrm_regmem = DREX_REG_MEMORY;
      rex = drex_byte & (REX_B | REX_X | REX_R);
      if (rex)
	rex |= REX_OPCODE;
      rex_used = rex;
    }

  /* Based on the OC1/OC0 bits, lay out the arguments in the correct 
     order.  */
  switch (oc0)
    {
    default:
      BadOp ();
      return;

    case 0:
      regs[0] = modrm_regmem;
      regs[1] = modrm_reg;
      regs[2] = drex_reg;
      break;

    case 1:
      regs[0] = modrm_reg;
      regs[1] = modrm_regmem;
      regs[2] = drex_reg;
      break;
    }

  /* Print out the arguments.  */
  for (i = 0; i < 3; i++)
    {
      int j = (intel_syntax) ? 2 - i : i;
      if (i > 0)
	{
	  *obufp++ = ',';
	  *obufp = '\0';
	}

      print_drex_arg (regs[j], bytemode, sizeflag);
    }

  rex = rex_save;
  rex_used = rex_used_save;
}

/* Emit a floating point comparison for comp<xx> instructions.  */

static void
OP_DREX_FCMP (int bytemode ATTRIBUTE_UNUSED, 
	      int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned char byte;

  static const char *const cmp_test[] = {
    "eq",
    "lt",
    "le",
    "unord",
    "ne",
    "nlt",
    "nle",
    "ord",
    "ueq",
    "ult",
    "ule",
    "false",
    "une",
    "unlt",
    "unle",
    "true"
  };

  FETCH_DATA (the_info, codep + 1);
  byte = *codep & 0xff;

  if (byte >= ARRAY_SIZE (cmp_test)
      || obuf[0] != 'c'
      || obuf[1] != 'o'
      || obuf[2] != 'm')
    {
      /* The instruction isn't one we know about, so just append the 
	 extension byte as a numeric value.  */
      OP_I (b_mode, 0);
    }

  else
    {
      sprintf (scratchbuf, "com%s%s", cmp_test[byte], obuf+3);
      strcpy (obuf, scratchbuf);
      codep++;
    }
}

/* Emit an integer point comparison for pcom<xx> instructions, 
   rewriting the instruction to have the test inside of it.  */

static void
OP_DREX_ICMP (int bytemode ATTRIBUTE_UNUSED, 
	      int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned char byte;

  static const char *const cmp_test[] = {
    "lt",
    "le",
    "gt",
    "ge",
    "eq",
    "ne",
    "false",
    "true"
  };

  FETCH_DATA (the_info, codep + 1);
  byte = *codep & 0xff;

  if (byte >= ARRAY_SIZE (cmp_test)
      || obuf[0] != 'p'
      || obuf[1] != 'c'
      || obuf[2] != 'o'
      || obuf[3] != 'm')
    {
      /* The instruction isn't one we know about, so just print the 
	 comparison test byte as a numeric value.  */
      OP_I (b_mode, 0);
    }

  else
    {
      sprintf (scratchbuf, "pcom%s%s", cmp_test[byte], obuf+4);
      strcpy (obuf, scratchbuf);
      codep++;
    }
}

/* Display the destination register operand for instructions with
   VEX. */

static void
OP_VEX (int bytemode, int sizeflag ATTRIBUTE_UNUSED)
{
  if (!need_vex)
    abort ();

  if (!need_vex_reg)
    return;

  switch (vex.length)
    {
    case 128:
      switch (bytemode)
	{
	case vex_mode:
	case vex128_mode:
	  break;
	default:
	  abort ();
	  return;
	}

      sprintf (scratchbuf, "%%xmm%d", vex.register_specifier);
      break;
    case 256:
      switch (bytemode)
	{
	case vex_mode:
	case vex256_mode:
	  break;
	default:
	  abort ();
	  return;
	}

      sprintf (scratchbuf, "%%ymm%d", vex.register_specifier);
      break;
    default:
      abort ();
      break;
    }
  oappend (scratchbuf + intel_syntax);
}

/* Get the VEX immediate byte without moving codep.  */

static unsigned char
get_vex_imm8 (int sizeflag)
{
  int bytes_before_imm = 0;

  /* Skip mod/rm byte.   */
  MODRM_CHECK;
  codep++;

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
	      bytes_before_imm++;
	    }

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
      else
	{ /* 16 bit address mode */
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
	      /* 1 byte displacement.  */
	      bytes_before_imm++;
	      break;
	    }
	}
    }

  FETCH_DATA (the_info, codep + bytes_before_imm + 1);
  return codep [bytes_before_imm];
}

static void
OP_EX_VexReg (int bytemode, int sizeflag, int reg)
{
  if (reg == -1 && modrm.mod != 3)
    {
      OP_E_memory (bytemode, sizeflag, 0);
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
      sprintf (scratchbuf, "%%xmm%d", reg);
      break;
    case 256:
      sprintf (scratchbuf, "%%ymm%d", reg);
      break;
    default:
      abort ();
    }
  oappend (scratchbuf + intel_syntax);
}

static void
OP_EX_VexImmW (int bytemode, int sizeflag)
{
  int reg = -1;
  static unsigned char vex_imm8;

  if (!vex_w_done)
    {
      vex_imm8 = get_vex_imm8 (sizeflag);
      if (vex.w)
	reg = vex_imm8 >> 4;
      vex_w_done = 1;
    }
  else
    {
      if (!vex.w)
	reg = vex_imm8 >> 4;
    }

  OP_EX_VexReg (bytemode, sizeflag, reg);
}

static void
OP_EX_VexW (int bytemode, int sizeflag)
{
  int reg = -1;

  if (!vex_w_done)
    {
      vex_w_done = 1;
      if (vex.w)
	reg = vex.register_specifier;
    }
  else
    {
      if (!vex.w)
	reg = vex.register_specifier;
    }

  OP_EX_VexReg (bytemode, sizeflag, reg);
}

static void
OP_VEX_FMA (int bytemode, int sizeflag)
{
  int reg = get_vex_imm8 (sizeflag) >> 4;

  if (reg > 7 && address_mode != mode_64bit)
    BadOp ();

  switch (vex.length)
    {
    case 128:
      switch (bytemode)
	{
	case vex_mode:
	case vex128_mode:
	  break;
	default:
	  abort ();
	  return;
	}

      sprintf (scratchbuf, "%%xmm%d", reg);
      break;
    case 256:
      switch (bytemode)
	{
	case vex_mode:
	  break;
	default:
	  abort ();
	  return;
	}

      sprintf (scratchbuf, "%%ymm%d", reg);
      break;
    default:
      abort ();
    }
  oappend (scratchbuf + intel_syntax);
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
      sprintf (scratchbuf, "%%xmm%d", reg);
      break;
    case 256:
      sprintf (scratchbuf, "%%ymm%d", reg);
      break;
    default:
      abort ();
    }
  oappend (scratchbuf + intel_syntax);
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
      strcpy (obuf, "vzeroupper");
      break;
    case 256:
      strcpy (obuf, "vzeroall");
      break;
    default:
      abort ();
    }
}

static const char *vex_cmp_op[] = {
  "eq",
  "lt",
  "le",
  "unord",
  "neq",
  "nlt",
  "nle",
  "ord",
  "eq_uq",
  "nge",
  "ngt",
  "false",
  "neq_oq",
  "ge",
  "gt",
  "true",
  "eq_os",
  "lt_oq",
  "le_oq",
  "unord_s",
  "neq_us",
  "nlt_uq",
  "nle_uq",
  "ord_s",
  "eq_us",
  "nge_uq",
  "ngt_uq",
  "false_os",
  "neq_os",
  "ge_oq",
  "gt_oq",
  "true_us"
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
      char *p = obuf + strlen (obuf) - 2;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = '\0';
      sprintf (p, "%s%s", vex_cmp_op[cmp_type], suffix);
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, cmp_type);
      oappend (scratchbuf + intel_syntax);
      scratchbuf[0] = '\0';
    }
}

static const char *pclmul_op[] = {
  "lql",
  "hql",
  "lqh",
  "hqh"
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
      char *p = obuf + strlen (obuf) - 3;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = p[2];
      suffix[3] = '\0';
      sprintf (p, "%s%s", pclmul_op[pclmul_type], suffix);
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, pclmul_type);
      oappend (scratchbuf + intel_syntax);
      scratchbuf[0] = '\0';
    }
}

static const char *vpermil2_op[] = {
  "td",
  "td",
  "mo",
  "mz"
};

static void
VPERMIL2_Fixup (int bytemode ATTRIBUTE_UNUSED,
		int sizeflag ATTRIBUTE_UNUSED)
{
  unsigned int vpermil2_type;

  FETCH_DATA (the_info, codep + 1);
  vpermil2_type = *codep++ & 0xf;
  if (vpermil2_type < ARRAY_SIZE (vpermil2_op))
    {
      char suffix [4];
      char *p = obuf + strlen (obuf) - 3;
      suffix[0] = p[0];
      suffix[1] = p[1];
      suffix[2] = p[2];
      suffix[3] = '\0';
      sprintf (p, "%s%s", vpermil2_op[vpermil2_type], suffix);
    }
  else
    {
      /* We have a reserved extension byte.  Output it directly.  */
      scratchbuf[0] = '$';
      print_operand_value (scratchbuf + 1, 1, vpermil2_type);
      oappend (scratchbuf + intel_syntax);
      scratchbuf[0] = '\0';
    }
}

static void
MOVBE_Fixup (int bytemode, int sizeflag)
{
  /* Add proper suffix to "movbe".  */
  char *p = obuf + strlen (obuf);

  switch (bytemode)
    {
    case v_mode:
      if (intel_syntax)
	break;

      USED_REX (REX_W);
      if (sizeflag & SUFFIX_ALWAYS)
	{
	  if (rex & REX_W)
	    *p++ = 'q';
	  else if (sizeflag & DFLAG)
	    *p++ = 'l';
	  else
	    *p++ = 'w';
	}
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
  *p = '\0';

  OP_M (bytemode, sizeflag);
}
