/* gen-sframe.h - Support for generating SFrame.
   Copyright (C) 2022-2026 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef GENSFRAME_H
#define GENSFRAME_H

/* Errors shouldn't be emitted either if SFrames are default-enabled, as
   we interpret default-enabled as "opportunistic SFrames".  Users don't
   want to be bothered by something preventing emission of SFrames in
   such a case.  */
#define sframe_as_bad(format, ...) \
  do {					       \
    if (flag_gen_sframe == GEN_SFRAME_ENABLED) \
      as_bad (format, __VA_ARGS__);            \
  } while (0)

/* The entity is not tracked.  */
#define SFRAME_FRE_ELEM_LOC_NONE	0
/* The location of the tracked entity is based on a register.  May or may not
   involve dereferencing.  */
#define SFRAME_FRE_ELEM_LOC_REG		1
/* The location of the tracked entity is based on the CFA.  In theory, may or
   may not involve dereferencing.  */
#define SFRAME_FRE_ELEM_LOC_STACK	2

/* An invalid register number.  */
#define SFRAME_FRE_REG_INVALID		((unsigned int)-1)

/* SFrame Frame Row Entry (FRE).

   A frame row entry is a slice of the frame and can be valid for a set of
   program instructions.  It keeps all information needed to retrieve the CFA
   and the Return Address (RA) if tracked.

   A frame row entry effectively stores accumulated information gathered by
   interpreting multiple CFI instructions.  More precisely, it is a
   self-sufficient record in its own right.  Only the subset of information
   necessary for unwinding is stored: Given a PC, how to retrieve the CFA and
   the RA.
*/

struct sframe_row_entry
{
  /* A linked list.  */
  struct sframe_row_entry *next;

  /* Start and end of the frame row entry.  */
  symbolS *pc_begin;
  symbolS *pc_end;

  /* A frame row entry is a merge candidate if new information can be updated
     on it.  */
  bool merge_candidate;

  /* Whether the return address is mangled with pauth code.  */
  bool mangled_ra_p;

  /* Whether RA is undefined.  */
  bool ra_undefined_p;

  /* Track CFA base (architectural) register ID.  */
  unsigned int cfa_base_reg;
  /* Offset from the CFA base register for recovering CFA.  */
  offsetT cfa_offset;
  /* Whether CFA recovery needs dereferencing.  This is tracked for
     SFRAME_FDE_TYPE_FLEX FDE type.  */
  bool cfa_deref_p;

  /* Track FP location.  Specify whether it is in register or memory.  */
  unsigned int fp_loc;
  unsigned int fp_reg;
  /* If the FP is stashed on stack, note the offset.  */
  offsetT fp_offset;
  /* Whether FP recovery needs dereferencing.  This is tracked for
     SFRAME_FDE_TYPE_FLEX FDE type.  */
  bool fp_deref_p;

  /* Track RA location.  Specify whether it is in register or memory.  */
  unsigned int ra_loc;
  unsigned int ra_reg;
  /* If RA is stashed on stack, note the offset.  */
  offsetT ra_offset;
  /* Whether RA recovery needs dereferencing.  This is tracked for FDE type
     SFRAME_FDE_TYPE_FLEX.  */
  bool ra_deref_p;
};

/* SFrame Function Description Entry.  */

struct sframe_func_entry
{
  /* A linked list.  */
  struct sframe_func_entry *next;

  /* Reference to the FDE created from CFI in dw2gencfi.  Some information
     like the start_address and the segment is made available via this
     member.  */
  const struct fde_entry *dw_fde;
  /* Whether the current FDE will use SFRAME_FDE_TYPE_FLEX representation.  */
  bool fde_flex_p;

  /* Reference to the first FRE for this function.  */
  struct sframe_row_entry *sframe_fres;

  unsigned int num_fres;
};

/* SFrame Function Description Entry Translation Context.  */

struct sframe_xlate_ctx
{
  /* Reference to the FDE created from CFI in dw2gencfi.  Information
     like the FDE start_address, end_address and the cfi insns are
     made available via this member.  */
  const struct fde_entry *dw_fde;

  /* List of FREs in the current FDE translation context, bounded by first_fre
     and last_fre.  */

  /* Keep track of the first FRE for the purpose of restoring state if
     necessary (for DW_CFA_restore).  */
  struct sframe_row_entry *first_fre;
  /* The last FRE in the list.  */
  struct sframe_row_entry *last_fre;

  /* The current FRE under construction.  */
  struct sframe_row_entry *cur_fre;
  /* Remember FRE for an eventual restore.  */
  struct sframe_row_entry *remember_fre;

  /* Whether the current FRE requires a more flexible frame encoding, hence
     needing SFRAME_FDE_TYPE_FLEX FDE type.  */
  bool flex_p;

  unsigned num_xlate_fres;
};

/* Error codes for SFrame translation context.  */
enum sframe_xlate_err
{
  /* Success.  */
  SFRAME_XLATE_OK = 0,
  /* Error.  */
  SFRAME_XLATE_ERROR = 1,
  /* Detailed error codes.  */
  SFRAME_XLATE_ERR_INVAL = -1,
  SFRAME_XLATE_ERR_NOTREPRESENTED = -2,
};

/* Callback to create the abi/arch identifier for SFrame section.  */

unsigned char
sframe_get_abi_arch_callback (const char *target_arch,
			      int big_endian_p);

/* SFrame version specific operations structure.  */

struct sframe_version_ops
{
  unsigned char format_version;    /* SFrame format version.  */
  /* set SFrame FRE info.  */
  unsigned char (*set_fre_info) (unsigned int, unsigned int, unsigned int,
				 bool);
  /* set SFrame Func info.  */
  unsigned char (*set_func_info) (unsigned int, unsigned int, unsigned int,
				  bool);
};

/* Generate SFrame stack trace info and prepare contents for the output.
   outout_sframe ()  is called at the end of file.  */

extern void output_sframe (segT sframe_seg);

#endif /* GENSFRAME_H */
