/* gen-sframe.c - Support for generating SFrame section.
   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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

#include "as.h"
#include "subsegs.h"
#include "sframe.h"
#include "sframe-internal.h"
#include "gen-sframe.h"
#include "dw2gencfi.h"

#ifdef support_sframe_p

#ifndef sizeof_member
# define sizeof_member(type, member)	(sizeof (((type *)0)->member))
#endif

/* SFrame FRE type selection optimization is an optimization for size.

   There are three flavors of SFrame FRE representation in the binary format:
     - sframe_frame_row_entry_addr1 where the FRE start address is 1 byte.
     - sframe_frame_row_entry_addr2 where the FRE start address is 2 bytes.
     - sframe_frame_row_entry_addr4 where the FRE start address is 4 bytes.

   Note that in the SFrame format, all SFrame FREs of a function use one
   single representation.  The SFrame FRE type itself is identified via the
   information in the SFrame FDE function info.

   Now, to select the minimum required one from the list above, one needs to
   make a decision based on the size (in bytes) of the function.

   As a result, for this optimization, some fragments (generated with a new
   type rs_sframe) for the SFrame section are fixed up later.

   This optimization (for size) is enabled by default.  */

#ifndef SFRAME_FRE_TYPE_SELECTION_OPT
# define SFRAME_FRE_TYPE_SELECTION_OPT 1
#endif

/* List of SFrame FDE entries.  */

static struct sframe_func_entry *all_sframe_fdes = NULL;

/* Tail of the list to add to.  */

static struct sframe_func_entry **last_sframe_fde = &all_sframe_fdes;

/* Emit a single byte into the current segment.  */

static inline void
out_one (int byte)
{
  FRAG_APPEND_1_CHAR (byte);
}

/* Emit a two-byte word into the current segment.  */

static inline void
out_two (int data)
{
  md_number_to_chars (frag_more (2), data, 2);
}

/* Emit a four byte word into the current segment.  */

static inline void
out_four (int data)
{
  md_number_to_chars (frag_more (4), data, 4);
}

/* Get the start address symbol from the DWARF FDE.  */

static symbolS*
get_dw_fde_start_addrS (const struct fde_entry *dw_fde)
{
  return dw_fde->start_address;
}

/* Get the start address symbol from the DWARF FDE.  */

static symbolS*
get_dw_fde_end_addrS (const struct fde_entry *dw_fde)
{
  return dw_fde->end_address;
}

/* Get whether PAUTH B key is used.  */
static bool
get_dw_fde_pauth_b_key_p (const struct fde_entry *dw_fde ATTRIBUTE_UNUSED)
{
#ifdef tc_fde_entry_extras
  return (dw_fde->pauth_key == AARCH64_PAUTH_KEY_B);
#else
  return false;
#endif
}

/* SFrame Frame Row Entry (FRE) related functions.  */

static void
sframe_fre_set_begin_addr (struct sframe_row_entry *fre, symbolS *beginS)
{
  fre->pc_begin = beginS;
}

static void
sframe_fre_set_end_addr (struct sframe_row_entry *fre, symbolS *endS)
{
  fre->pc_end = endS;
}

static void
sframe_fre_set_cfa_base_reg (struct sframe_row_entry *fre,
			     unsigned int cfa_base_reg)
{
  fre->cfa_base_reg = cfa_base_reg;
  fre->merge_candidate = false;
}

static offsetT
sframe_fre_get_cfa_offset (const struct sframe_row_entry * fre)
{
  offsetT offset = fre->cfa_offset;

  /* For s390x undo adjustment of CFA offset (to enable 8-bit offsets).  */
  if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG)
    offset = SFRAME_V2_S390X_CFA_OFFSET_DECODE (offset);

  return offset;
}

static void
sframe_fre_set_cfa_offset (struct sframe_row_entry *fre,
			   offsetT cfa_offset)
{
  /* For s390x adjust CFA offset to enable 8-bit offsets.  */
  if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG)
    cfa_offset = SFRAME_V2_S390X_CFA_OFFSET_ENCODE (cfa_offset);

  fre->cfa_offset = cfa_offset;
  fre->merge_candidate = false;
}

static void
sframe_fre_set_ra_track (struct sframe_row_entry *fre, offsetT ra_offset)
{
  fre->ra_loc = SFRAME_FRE_ELEM_LOC_STACK;
  fre->ra_offset = ra_offset;
  fre->merge_candidate = false;
}

static void
sframe_fre_set_bp_track (struct sframe_row_entry *fre, offsetT bp_offset)
{
  fre->bp_loc = SFRAME_FRE_ELEM_LOC_STACK;
  fre->bp_offset = bp_offset;
  fre->merge_candidate = false;
}

/* All stack offset values within an FRE are uniformly encoded in the same
   number of bytes.  The size of the stack offset values will, however, vary
   across FREs.  */

#define VALUE_8BIT  0x7f
#define VALUE_16BIT 0x7fff
#define VALUE_32BIT 0x7fffffff
#define VALUE_64BIT 0x7fffffffffffffff

/* Given a signed offset, return the size in bytes needed to represent it.  */

static unsigned int
get_offset_size_in_bytes (offsetT value)
{
  unsigned int size = 0;

  if (value <= VALUE_8BIT && value >= (offsetT) -VALUE_8BIT)
    size = 1;
  else if (value <= VALUE_16BIT && value >= (offsetT) -VALUE_16BIT)
    size = 2;
  else if (value <= VALUE_32BIT && value >= (offsetT) -VALUE_32BIT)
    size = 4;
  else if ((sizeof (offsetT) > 4) && (value <= (offsetT) VALUE_64BIT
				      && value >= (offsetT) -VALUE_64BIT))
    size = 8;

  return size;
}

#define SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_1B  0 /* SFRAME_FRE_OFFSET_1B.  */
#define SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_2B  1 /* SFRAME_FRE_OFFSET_2B.  */
#define SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_4B  2 /* SFRAME_FRE_OFFSET_4B.  */
#define SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_8B  3 /* Not supported in SFrame.  */
#define SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_MAX SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_8B

/* Helper struct for mapping offset size to output functions.  */

struct sframe_fre_offset_func_map
{
  unsigned int offset_size;
  void (*out_func)(int);
};

/* Given an OFFSET_SIZE, return the size in bytes needed to represent it.  */

static unsigned int
sframe_fre_offset_func_map_index (unsigned int offset_size)
{
  unsigned int idx = SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_MAX;

  switch (offset_size)
    {
      case SFRAME_FRE_OFFSET_1B:
	idx = SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_1B;
	break;
      case SFRAME_FRE_OFFSET_2B:
	idx = SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_2B;
	break;
      case SFRAME_FRE_OFFSET_4B:
	idx = SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_4B;
	break;
      default:
	/* Not supported in SFrame.  */
	break;
    }

  return idx;
}

/* Mapping from offset size to the output function to emit the value.  */

static const
struct sframe_fre_offset_func_map
fre_offset_func_map[SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_MAX+1] =
{
  { SFRAME_FRE_OFFSET_1B, out_one },
  { SFRAME_FRE_OFFSET_2B, out_two },
  { SFRAME_FRE_OFFSET_4B, out_four },
  { -1, NULL } /* Not Supported in SFrame.  */
};

/* SFrame version specific operations access.  */

static struct sframe_version_ops sframe_ver_ops;

/* SFrame (SFRAME_VERSION_1) set FRE info.  */

static unsigned char
sframe_v1_set_fre_info (unsigned int base_reg, unsigned int num_offsets,
			unsigned int offset_size, bool mangled_ra_p)
{
  unsigned char fre_info;
  fre_info = SFRAME_V1_FRE_INFO (base_reg, num_offsets, offset_size);
  fre_info = SFRAME_V1_FRE_INFO_UPDATE_MANGLED_RA_P (mangled_ra_p, fre_info);
  return fre_info;
}

/* SFrame (SFRAME_VERSION_1) set function info.  */
static unsigned char
sframe_v1_set_func_info (unsigned int fde_type, unsigned int fre_type,
			 unsigned int pauth_key)
{
  unsigned char func_info;
  func_info = SFRAME_V1_FUNC_INFO (fde_type, fre_type);
  func_info = SFRAME_V1_FUNC_INFO_UPDATE_PAUTH_KEY (pauth_key, func_info);
  return func_info;
}

/* SFrame version specific operations setup.  */

static void
sframe_set_version (uint32_t sframe_version ATTRIBUTE_UNUSED)
{
  sframe_ver_ops.format_version = SFRAME_VERSION_2;

  /* These operations remain the same for SFRAME_VERSION_2 as fre_info and
     func_info have not changed from SFRAME_VERSION_1.  */

  sframe_ver_ops.set_fre_info = sframe_v1_set_fre_info;

  sframe_ver_ops.set_func_info = sframe_v1_set_func_info;
}

/* SFrame set FRE info.  */

static unsigned char
sframe_set_fre_info (unsigned int base_reg, unsigned int num_offsets,
		     unsigned int offset_size, bool mangled_ra_p)
{
  return sframe_ver_ops.set_fre_info (base_reg, num_offsets,
				      offset_size, mangled_ra_p);
}

/* SFrame set func info. */

static unsigned char
sframe_set_func_info (unsigned int fde_type, unsigned int fre_type,
		      unsigned int pauth_key)
{
  return sframe_ver_ops.set_func_info (fde_type, fre_type, pauth_key);
}

/* Get the number of SFrame FDEs for the current file.  */

static unsigned int
get_num_sframe_fdes (void);

/* Get the number of SFrame frame row entries for the current file.  */

static unsigned int
get_num_sframe_fres (void);

/* Get CFA base register ID as represented in SFrame Frame Row Entry.  */

static unsigned int
get_fre_base_reg_id (struct sframe_row_entry *sframe_fre)
{
  unsigned int cfi_insn_cfa_base_reg = sframe_fre->cfa_base_reg;
  unsigned fre_base_reg = SFRAME_BASE_REG_SP;

  if (cfi_insn_cfa_base_reg == SFRAME_CFA_FP_REG)
    fre_base_reg = SFRAME_BASE_REG_FP;

  /* Only one bit is reserved in SFRAME_VERSION_1.  */
  gas_assert (fre_base_reg == SFRAME_BASE_REG_SP
	      || fre_base_reg == SFRAME_BASE_REG_FP);

  return fre_base_reg;
}

/* Get number of offsets necessary for the SFrame Frame Row Entry.  */

static unsigned int
get_fre_num_offsets (struct sframe_row_entry *sframe_fre)
{
  /* Atleast 1 must always be present (to recover CFA).  */
  unsigned int fre_num_offsets = 1;

  if (sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
    fre_num_offsets++;
  if (sframe_ra_tracking_p ()
      && (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_STACK
	  /* For s390x account padding RA offset, if FP without RA saved.  */
	  || (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
	      && sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)))
    fre_num_offsets++;
  return fre_num_offsets;
}

/* Get the minimum necessary offset size (in bytes) for this
   SFrame frame row entry.  */

static unsigned int
sframe_get_fre_offset_size (struct sframe_row_entry *sframe_fre)
{
  unsigned int max_offset_size = 0;
  unsigned int cfa_offset_size = 0;
  unsigned int bp_offset_size = 0;
  unsigned int ra_offset_size = 0;

  unsigned int fre_offset_size = 0;

  /* What size of offsets appear in this frame row entry.  */
  cfa_offset_size = get_offset_size_in_bytes (sframe_fre->cfa_offset);
  if (sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
    bp_offset_size = get_offset_size_in_bytes (sframe_fre->bp_offset);
  if (sframe_ra_tracking_p ())
    {
      if (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_STACK)
	ra_offset_size = get_offset_size_in_bytes (sframe_fre->ra_offset);
      /* For s390x account padding RA offset, if FP without RA saved.  */
      else if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
	       && sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	ra_offset_size = get_offset_size_in_bytes (SFRAME_FRE_RA_OFFSET_INVALID);
    }

  /* Get the maximum size needed to represent the offsets.  */
  max_offset_size = cfa_offset_size;
  if (bp_offset_size > max_offset_size)
    max_offset_size = bp_offset_size;
  if (ra_offset_size > max_offset_size)
    max_offset_size = ra_offset_size;

  gas_assert (max_offset_size);

  switch (max_offset_size)
    {
    case 1:
      fre_offset_size = SFRAME_FRE_OFFSET_1B;
      break;
    case 2:
      fre_offset_size = SFRAME_FRE_OFFSET_2B;
      break;
    case 4:
      fre_offset_size = SFRAME_FRE_OFFSET_4B;
      break;
    default:
      /* Offset of size 8 bytes is not supported in SFrame format
	 version 1.  */
      as_fatal (_("SFrame unsupported offset value\n"));
      break;
    }

  return fre_offset_size;
}

#if SFRAME_FRE_TYPE_SELECTION_OPT

/* Create a composite expression CEXP (for SFrame FRE start address) such that:

      exp = <val> OP_absent <width>, where,

    - <val> and <width> are themselves expressionS.
    - <val> stores the expression which when evaluated gives the value of the
      start address offset of the FRE.
    - <width> stores the expression when evaluated gives the number of bytes
      needed to encode the start address offset of the FRE.

   The use of OP_absent as the X_op_symbol helps identify this expression
   later when fragments are fixed up.  */

static void
create_fre_start_addr_exp (expressionS *cexp, symbolS *fre_pc_begin,
			   symbolS *fde_start_address,
			   symbolS *fde_end_address)
{
  expressionS val;
  expressionS width;

  /* val expression stores the FDE start address offset from the start PC
     of function.  */
  val.X_op = O_subtract;
  val.X_add_symbol = fre_pc_begin;
  val.X_op_symbol = fde_start_address;
  val.X_add_number = 0;

  /* width expressions stores the size of the function.  This is used later
     to determine the number of bytes to be used to encode the FRE start
     address of each FRE of the function.  */
  width.X_op = O_subtract;
  width.X_add_symbol = fde_end_address;
  width.X_op_symbol = fde_start_address;
  width.X_add_number = 0;

  cexp->X_op = O_absent;
  cexp->X_add_symbol = make_expr_symbol (&val);
  cexp->X_op_symbol = make_expr_symbol (&width);
  cexp->X_add_number = 0;
}

/* Create a composite expression CEXP (for SFrame FDE function info) such that:

      exp = <rest_of_func_info> OP_modulus <width>, where,

    - <rest_of_func_info> and <width> are themselves expressionS.
    - <rest_of_func_info> stores a constant expression where X_add_number is
    used to stash away the func_info.  The upper 4-bits of the func_info are copied
    back to the resulting byte by the fragment fixup logic.
    - <width> stores the expression when evaluated gives the size of the
    function in number of bytes.

   The use of OP_modulus as the X_op_symbol helps identify this expression
   later when fragments are fixed up.  */

static void
create_func_info_exp (expressionS *cexp, symbolS *dw_fde_end_addrS,
		      symbolS *dw_fde_start_addrS, uint8_t func_info)
{
  expressionS width;
  expressionS rest_of_func_info;

  width.X_op = O_subtract;
  width.X_add_symbol = dw_fde_end_addrS;
  width.X_op_symbol = dw_fde_start_addrS;
  width.X_add_number = 0;

  rest_of_func_info.X_op = O_constant;
  rest_of_func_info.X_add_number = func_info;

  cexp->X_op = O_modulus;
  cexp->X_add_symbol = make_expr_symbol (&rest_of_func_info);
  cexp->X_op_symbol = make_expr_symbol (&width);
  cexp->X_add_number = 0;
}

#endif

static struct sframe_row_entry*
sframe_row_entry_new (void)
{
  struct sframe_row_entry *fre = XCNEW (struct sframe_row_entry);
  /* Reset cfa_base_reg to -1.  A value of 0 will imply some valid register
     for the supported arches.  */
  fre->cfa_base_reg = SFRAME_FRE_BASE_REG_INVAL;
  fre->merge_candidate = true;
  /* Reset the mangled RA status bit to zero by default.  We will
     initialize it in sframe_row_entry_initialize () with the sticky
     bit if set.  */
  fre->mangled_ra_p = false;

  return fre;
}

static void
sframe_row_entry_free (struct sframe_row_entry *fre)
{
  while (fre)
    {
      struct sframe_row_entry *fre_next = fre->next;
      XDELETE (fre);
      fre = fre_next;
    }
}

/* Allocate an SFrame FDE.  */

static struct sframe_func_entry*
sframe_fde_alloc (void)
{
  return XCNEW (struct sframe_func_entry);
}

/* Free up the SFrame FDE.  */

static void
sframe_fde_free (struct sframe_func_entry *sframe_fde)
{
  sframe_row_entry_free (sframe_fde->sframe_fres);
  XDELETE (sframe_fde);
}

static void
output_sframe_row_entry (symbolS *fde_start_addr,
			 symbolS *fde_end_addr,
			 struct sframe_row_entry *sframe_fre)
{
  unsigned char fre_info;
  unsigned int fre_num_offsets;
  unsigned int fre_offset_size;
  unsigned int fre_base_reg;
  expressionS exp;
  unsigned int fre_addr_size;

  unsigned int idx = 0;
  unsigned int fre_write_offsets = 0;

  fre_addr_size = 4; /* 4 bytes by default.   FIXME tie it to fre_type? */

  /* SFrame FRE Start Address.  */
#if SFRAME_FRE_TYPE_SELECTION_OPT
  create_fre_start_addr_exp (&exp, sframe_fre->pc_begin, fde_start_addr,
			     fde_end_addr);
  frag_grow (fre_addr_size);
  frag_var (rs_sframe, fre_addr_size, 0, 0,
	    make_expr_symbol (&exp), 0, (char *) frag_now);
#else
  gas_assert (fde_end_addr);
  exp.X_op = O_subtract;
  exp.X_add_symbol = sframe_fre->pc_begin; /* to.  */
  exp.X_op_symbol = fde_start_addr; /* from.  */
  exp.X_add_number = 0;
  emit_expr (&exp, fre_addr_size);
#endif

  /* Create the fre_info using the CFA base register, number of offsets and max
     size of offset in this frame row entry.  */
  fre_base_reg = get_fre_base_reg_id (sframe_fre);
  fre_num_offsets = get_fre_num_offsets (sframe_fre);
  fre_offset_size = sframe_get_fre_offset_size (sframe_fre);
  fre_info = sframe_set_fre_info (fre_base_reg, fre_num_offsets,
				  fre_offset_size, sframe_fre->mangled_ra_p);
  out_one (fre_info);

  idx = sframe_fre_offset_func_map_index (fre_offset_size);
  gas_assert (idx < SFRAME_FRE_OFFSET_FUNC_MAP_INDEX_MAX);

  /* Write out the offsets in order - cfa, bp, ra.  */
  fre_offset_func_map[idx].out_func (sframe_fre->cfa_offset);
  fre_write_offsets++;

  if (sframe_ra_tracking_p ())
    {
      if (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_STACK)
	{
	  fre_offset_func_map[idx].out_func (sframe_fre->ra_offset);
	  fre_write_offsets++;
	}
      /* For s390x write padding RA offset, if FP without RA saved.  */
      else if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
	       && sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	{
	  fre_offset_func_map[idx].out_func (SFRAME_FRE_RA_OFFSET_INVALID);
	  fre_write_offsets++;
	}
    }
  if (sframe_fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
    {
      fre_offset_func_map[idx].out_func (sframe_fre->bp_offset);
      fre_write_offsets++;
    }

  /* Check if the expected number offsets have been written out
     in this FRE.  */
  gas_assert (fre_write_offsets == fre_num_offsets);
}

static void
output_sframe_funcdesc (symbolS *start_of_fre_section,
			symbolS *fre_symbol,
			struct sframe_func_entry *sframe_fde)
{
  expressionS exp;
  symbolS *dw_fde_start_addrS, *dw_fde_end_addrS;
  unsigned int pauth_key;

  dw_fde_start_addrS = get_dw_fde_start_addrS (sframe_fde->dw_fde);
  dw_fde_end_addrS = get_dw_fde_end_addrS (sframe_fde->dw_fde);

  /* Start address of the function.  gas always emits this value with encoding
     SFRAME_F_FDE_FUNC_START_PCREL.  See PR ld/32666.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = dw_fde_start_addrS; /* to location.  */
  exp.X_op_symbol = symbol_temp_new_now (); /* from location.  */
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_entry,
				  sfde_func_start_address));

  /* Size of the function in bytes.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = dw_fde_end_addrS;
  exp.X_op_symbol = dw_fde_start_addrS;
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_entry,
				  sfde_func_size));

  /* Offset to the first frame row entry.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = fre_symbol; /* Minuend.  */
  exp.X_op_symbol = start_of_fre_section; /* Subtrahend.  */
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_entry,
				  sfde_func_start_fre_off));

  /* Number of FREs.  */
  out_four (sframe_fde->num_fres);

  /* SFrame FDE function info.  */
  unsigned char func_info;
  pauth_key = (get_dw_fde_pauth_b_key_p (sframe_fde->dw_fde)
	       ? SFRAME_AARCH64_PAUTH_KEY_B : SFRAME_AARCH64_PAUTH_KEY_A);
  func_info = sframe_set_func_info (SFRAME_FDE_TYPE_PCINC,
				    SFRAME_FRE_TYPE_ADDR4,
				    pauth_key);
#if SFRAME_FRE_TYPE_SELECTION_OPT
  expressionS cexp;
  create_func_info_exp (&cexp, dw_fde_end_addrS, dw_fde_start_addrS,
			func_info);
  frag_grow (1); /* Size of func info is unsigned char.  */
  frag_var (rs_sframe, 1, 0, 0, make_expr_symbol (&cexp), 0,
	    (char *) frag_now);
#else
  out_one (func_info);
#endif
  out_one (0);
  out_two (0);
}

static void
output_sframe_internal (void)
{
  expressionS exp;
  unsigned int i = 0;

  symbolS *end_of_frame_hdr;
  symbolS *end_of_frame_section;
  symbolS *start_of_func_desc_section;
  symbolS *start_of_fre_section;
  struct sframe_func_entry *sframe_fde, *sframe_fde_next;
  struct sframe_row_entry *sframe_fre;
  unsigned char abi_arch = 0;
  int fixed_fp_offset = SFRAME_CFA_FIXED_FP_INVALID;
  int fixed_ra_offset = SFRAME_CFA_FIXED_RA_INVALID;

  /* The function descriptor entries as dumped by the assembler are not
     sorted on PCs.  Fix for PR ld/32666 requires setting of an additional
     flag in SFrame Version 2.  */
  unsigned char sframe_flags = SFRAME_F_FDE_FUNC_START_PCREL;

  unsigned int num_fdes = get_num_sframe_fdes ();
  unsigned int num_fres = get_num_sframe_fres ();
  symbolS **fde_fre_symbols = XNEWVEC (symbolS *, num_fdes);
  for (i = 0; i < num_fdes; i++)
    fde_fre_symbols[i] = symbol_temp_make ();

  end_of_frame_hdr = symbol_temp_make ();
  start_of_fre_section = symbol_temp_make ();
  start_of_func_desc_section = symbol_temp_make ();
  end_of_frame_section = symbol_temp_make ();

  /* Output the preamble of SFrame section.  */
  out_two (SFRAME_MAGIC);
  out_one (SFRAME_VERSION);
  /* gas must ensure emitted SFrame sections have at least the required flags
     set.  */
  gas_assert ((sframe_flags & SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS)
	      == SFRAME_V2_GNU_AS_LD_ENCODING_FLAGS);
  out_one (sframe_flags);
  /* abi/arch.  */
#ifdef sframe_get_abi_arch
  abi_arch = sframe_get_abi_arch ();
#endif
  gas_assert (abi_arch);
  out_one (abi_arch);

  /* Offset for the FP register from CFA.  Neither of the AMD64 or AAPCS64
     ABIs have a fixed offset for the FP register from the CFA.  This may be
     useful in future (but not without additional support in the toolchain)
     for specialized handling/encoding for cases where, for example,
     -fno-omit-frame-pointer is used.  */
  out_one (fixed_fp_offset);

  /* All ABIs participating in SFrame generation must define
     sframe_ra_tracking_p.
     When RA tracking (in FREs) is not needed (e.g., AMD64), SFrame assumes
     the RA is going to be at a fixed offset from CFA.  Check that the fixed RA
     offset is appropriately defined in all cases.  */
  if (!sframe_ra_tracking_p ())
    {
      fixed_ra_offset = sframe_cfa_ra_offset ();
      gas_assert (fixed_ra_offset != SFRAME_CFA_FIXED_RA_INVALID);
    }
  out_one (fixed_ra_offset);

  /* None of the AMD64, AARCH64, or s390x ABIs need the auxiliary header.
     When the need does arise to use this field, the appropriate backend
     must provide this information.  */
  out_one (0); /* Auxiliary SFrame header length.  */

  out_four (num_fdes); /* Number of FDEs.  */
  out_four (num_fres); /* Number of FREs.  */

  /* Size of FRE sub-section.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = end_of_frame_section;
  exp.X_op_symbol = start_of_fre_section;
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_header, sfh_fre_len));

  /* Offset of FDE sub-section.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = end_of_frame_hdr;
  exp.X_op_symbol = start_of_func_desc_section;
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_header, sfh_fdeoff));

  /* Offset of FRE sub-section.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = start_of_fre_section;
  exp.X_op_symbol = end_of_frame_hdr;
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_header, sfh_freoff));

  symbol_set_value_now (end_of_frame_hdr);
  symbol_set_value_now (start_of_func_desc_section);

  /* Output the SFrame function descriptor entries.  */
  i = 0;
  for (sframe_fde = all_sframe_fdes; sframe_fde; sframe_fde = sframe_fde->next)
    {
      output_sframe_funcdesc (start_of_fre_section,
			      fde_fre_symbols[i], sframe_fde);
      i++;
    }

  symbol_set_value_now (start_of_fre_section);

  /* Output the SFrame FREs.  */
  i = 0;
  sframe_fde = all_sframe_fdes;

  for (sframe_fde = all_sframe_fdes; sframe_fde; sframe_fde = sframe_fde_next)
    {
      symbol_set_value_now (fde_fre_symbols[i]);
      for (sframe_fre = sframe_fde->sframe_fres;
	   sframe_fre;
	   sframe_fre = sframe_fre->next)
	{
	  output_sframe_row_entry (get_dw_fde_start_addrS (sframe_fde->dw_fde),
				   get_dw_fde_end_addrS (sframe_fde->dw_fde),
				   sframe_fre);
	}
      i++;
      sframe_fde_next = sframe_fde->next;
      sframe_fde_free (sframe_fde);
    }
  all_sframe_fdes = NULL;
  last_sframe_fde = &all_sframe_fdes;

  symbol_set_value_now (end_of_frame_section);

  gas_assert (i == num_fdes);

  free (fde_fre_symbols);
  fde_fre_symbols = NULL;
}

static unsigned int
get_num_sframe_fdes (void)
{
  struct sframe_func_entry *sframe_fde;
  unsigned int total_fdes = 0;

  for (sframe_fde = all_sframe_fdes; sframe_fde ; sframe_fde = sframe_fde->next)
    total_fdes++;

  return total_fdes;
}

/* Get the total number of SFrame row entries across the FDEs.  */

static unsigned int
get_num_sframe_fres (void)
{
  struct sframe_func_entry *sframe_fde;
  unsigned int total_fres = 0;

  for (sframe_fde = all_sframe_fdes; sframe_fde ; sframe_fde = sframe_fde->next)
    total_fres += sframe_fde->num_fres;

  return total_fres;
}

/* SFrame translation context functions.  */

/* Allocate a new SFrame translation context.  */

static struct sframe_xlate_ctx*
sframe_xlate_ctx_alloc (void)
{
  struct sframe_xlate_ctx* xlate_ctx = XCNEW (struct sframe_xlate_ctx);
  return xlate_ctx;
}

/* Initialize the given SFrame translation context.  */

static void
sframe_xlate_ctx_init (struct sframe_xlate_ctx *xlate_ctx)
{
  xlate_ctx->dw_fde = NULL;
  xlate_ctx->first_fre = NULL;
  xlate_ctx->last_fre = NULL;
  xlate_ctx->cur_fre = NULL;
  xlate_ctx->remember_fre = NULL;
  xlate_ctx->num_xlate_fres = 0;
}

/* Cleanup the given SFrame translation context.  */

static void
sframe_xlate_ctx_cleanup (struct sframe_xlate_ctx *xlate_ctx)
{
  sframe_row_entry_free (xlate_ctx->first_fre);
  XDELETE (xlate_ctx->remember_fre);
  XDELETE (xlate_ctx->cur_fre);
}

/* Transfer the state from the SFrame translation context to the SFrame FDE.  */

static void
sframe_xlate_ctx_finalize (struct sframe_xlate_ctx *xlate_ctx,
			   struct sframe_func_entry *sframe_fde)
{
  sframe_fde->dw_fde = xlate_ctx->dw_fde;
  sframe_fde->sframe_fres = xlate_ctx->first_fre;
  sframe_fde->num_fres = xlate_ctx->num_xlate_fres;
}

/* Add the given FRE in the list of frame row entries in the given FDE
   translation context.  */

static void
sframe_xlate_ctx_add_fre (struct sframe_xlate_ctx *xlate_ctx,
			 struct sframe_row_entry *fre)
{
  gas_assert (xlate_ctx && fre);

  /* Add the frame row entry.  */
  if (!xlate_ctx->first_fre)
    xlate_ctx->first_fre = fre;
  else if (xlate_ctx->last_fre)
    xlate_ctx->last_fre->next = fre;

  xlate_ctx->last_fre = fre;

  /* Keep track of the total number of SFrame frame row entries.  */
  xlate_ctx->num_xlate_fres++;
}

/* A SFrame Frame Row Entry is self-sufficient in terms of stack tracing info
   for a given PC.  It contains information assimilated from multiple CFI
   instructions, and hence, a new SFrame FRE is initialized with the data from
   the previous known FRE, if any.

   Understandably, not all information (especially the instruction begin
   and end boundaries) needs to be relayed.  Hence, the caller of this API
   must set the pc_begin and pc_end as applicable.  */

static void
sframe_row_entry_initialize (struct sframe_row_entry *cur_fre,
			     struct sframe_row_entry *prev_fre)
{
  gas_assert (prev_fre);
  cur_fre->cfa_base_reg = prev_fre->cfa_base_reg;
  cur_fre->cfa_offset = prev_fre->cfa_offset;
  cur_fre->bp_loc = prev_fre->bp_loc;
  cur_fre->bp_offset = prev_fre->bp_offset;
  cur_fre->ra_loc = prev_fre->ra_loc;
  cur_fre->ra_offset = prev_fre->ra_offset;
  /* Treat RA mangling as a sticky bit.  It retains its value until another
     .cfi_negate_ra_state is seen.  */
  cur_fre->mangled_ra_p = prev_fre->mangled_ra_p;
}

/* Return SFrame register name for SP, FP, and RA, or NULL if other.  */

static const char *
sframe_register_name (unsigned int reg)
{
  if (reg == SFRAME_CFA_SP_REG)
    return "SP";
  else if (reg == SFRAME_CFA_FP_REG)
    return "FP";
  else if (reg == SFRAME_CFA_RA_REG)
    return "RA";
  else
    return NULL;
}

/* Translate DW_CFA_advance_loc into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_advance_loc (struct sframe_xlate_ctx *xlate_ctx,
			     struct cfi_insn_data *cfi_insn)
{
  struct sframe_row_entry *last_fre = xlate_ctx->last_fre;
  /* Get the scratchpad FRE currently being updated as the cfi_insn's
     get interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  if (cur_fre)
    {
      if (!cur_fre->merge_candidate)
	{
	  sframe_fre_set_end_addr (cur_fre, cfi_insn->u.ll.lab2);

	  sframe_xlate_ctx_add_fre (xlate_ctx, cur_fre);
	  last_fre = xlate_ctx->last_fre;

	  xlate_ctx->cur_fre = sframe_row_entry_new ();
	  cur_fre = xlate_ctx->cur_fre;

	  if (last_fre)
	    sframe_row_entry_initialize (cur_fre, last_fre);
	}
      else
	{
	  sframe_fre_set_end_addr (last_fre, cfi_insn->u.ll.lab2);
	  gas_assert (last_fre->merge_candidate == false);
	}
    }
  else
    {
      xlate_ctx->cur_fre = sframe_row_entry_new ();
      cur_fre = xlate_ctx->cur_fre;
    }

  gas_assert (cur_fre);
  sframe_fre_set_begin_addr (cur_fre, cfi_insn->u.ll.lab2);

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_def_cfa into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_def_cfa (struct sframe_xlate_ctx *xlate_ctx,
			 struct cfi_insn_data *cfi_insn)

{
  /* Get the scratchpad FRE.  This FRE will eventually get linked in.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  if (!cur_fre)
  {
    xlate_ctx->cur_fre = sframe_row_entry_new ();
    cur_fre = xlate_ctx->cur_fre;
    sframe_fre_set_begin_addr (cur_fre,
			       get_dw_fde_start_addrS (xlate_ctx->dw_fde));
  }
  /* Define the current CFA rule to use the provided register and
     offset.  However, if the register is not FP/SP, skip creating
     SFrame stack trace info for the function.  */
  if (cfi_insn->u.ri.reg != SFRAME_CFA_SP_REG
      && cfi_insn->u.ri.reg != SFRAME_CFA_FP_REG)
    {
      as_warn (_("no SFrame FDE emitted; "
		 "non-SP/FP register %u in .cfi_def_cfa"),
	       cfi_insn->u.ri.reg);
      return SFRAME_XLATE_ERR_NOTREPRESENTED; /* Not represented.  */
    }
  sframe_fre_set_cfa_base_reg (cur_fre, cfi_insn->u.ri.reg);
  sframe_fre_set_cfa_offset (cur_fre, cfi_insn->u.ri.offset);
  cur_fre->merge_candidate = false;

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_def_cfa_register into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_def_cfa_register (struct sframe_xlate_ctx *xlate_ctx,
				  struct cfi_insn_data *cfi_insn)
{
  struct sframe_row_entry *last_fre = xlate_ctx->last_fre;
  /* Get the scratchpad FRE.  This FRE will eventually get linked in.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);
  /* Define the current CFA rule to use the provided register (but to
     keep the old offset).  However, if the register is not FP/SP,
     skip creating SFrame stack trace info for the function.  */
  if (cfi_insn->u.r != SFRAME_CFA_SP_REG
      && cfi_insn->u.r != SFRAME_CFA_FP_REG)
    {
      as_warn (_("no SFrame FDE emitted; "
		 "non-SP/FP register %u in .cfi_def_cfa_register"),
	       cfi_insn->u.r);
      return SFRAME_XLATE_ERR_NOTREPRESENTED; /* Not represented.  */
    }
  sframe_fre_set_cfa_base_reg (cur_fre, cfi_insn->u.r);
  if (last_fre)
    sframe_fre_set_cfa_offset (cur_fre, sframe_fre_get_cfa_offset (last_fre));

  cur_fre->merge_candidate = false;

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_def_cfa_offset into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_def_cfa_offset (struct sframe_xlate_ctx *xlate_ctx,
				struct cfi_insn_data *cfi_insn)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);
  /*  Define the current CFA rule to use the provided offset (but to keep
      the old register).  However, if the old register is not FP/SP,
      skip creating SFrame stack trace info for the function.  */
  if ((cur_fre->cfa_base_reg == SFRAME_CFA_FP_REG)
      || (cur_fre->cfa_base_reg == SFRAME_CFA_SP_REG))
    {
      sframe_fre_set_cfa_offset (cur_fre, cfi_insn->u.i);
      cur_fre->merge_candidate = false;
    }
  else
    {
      /* No CFA base register in effect.  Non-SP/FP CFA base register should
	 not occur, as sframe_xlate_do_def_cfa[_register] would detect this.  */
      as_warn (_("no SFrame FDE emitted; "
		 ".cfi_def_cfa_offset without CFA base register in effect"));
      return SFRAME_XLATE_ERR_NOTREPRESENTED;
    }

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_offset into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_offset (struct sframe_xlate_ctx *xlate_ctx,
			struct cfi_insn_data *cfi_insn)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);
  /* Change the rule for the register indicated by the register number to
     be the specified offset.  */
  /* Ignore SP reg, as it can be recovered from the CFA tracking info.  */
  if (cfi_insn->u.ri.reg == SFRAME_CFA_FP_REG)
    {
      gas_assert (!cur_fre->base_reg);
      sframe_fre_set_bp_track (cur_fre, cfi_insn->u.ri.offset);
      cur_fre->merge_candidate = false;
    }
  else if (sframe_ra_tracking_p ()
	   && cfi_insn->u.ri.reg == SFRAME_CFA_RA_REG)
    {
      sframe_fre_set_ra_track (cur_fre, cfi_insn->u.ri.offset);
      cur_fre->merge_candidate = false;
    }
  /* This is used to track changes to non-rsp registers, skip all others
     except FP / RA for now.  */
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_val_offset into SFrame context.
   Return SFRAME_XLATE_OK if success.

   When CFI_ESC_P is true, the CFI_INSN is hand-crafted using CFI_escape
   data.  See sframe_xlate_do_escape_val_offset.  */

static int
sframe_xlate_do_val_offset (const struct sframe_xlate_ctx *xlate_ctx ATTRIBUTE_UNUSED,
			    const struct cfi_insn_data *cfi_insn,
			    bool cfi_esc_p)
{
  /* Previous value of register is CFA + offset.  However, if the specified
     register is not interesting (SP, FP, or RA reg), the current
     DW_CFA_val_offset instruction can be safely skipped without sacrificing
     the asynchronicity of stack trace information.  */
  if (cfi_insn->u.ri.reg == SFRAME_CFA_FP_REG
      || (sframe_ra_tracking_p () && cfi_insn->u.ri.reg == SFRAME_CFA_RA_REG)
      /* Ignore SP reg, if offset matches assumed default rule.  */
      || (cfi_insn->u.ri.reg == SFRAME_CFA_SP_REG
	  && ((sframe_get_abi_arch () != SFRAME_ABI_S390X_ENDIAN_BIG
	       && cfi_insn->u.ri.offset != 0)
	      || (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
		  && cfi_insn->u.ri.offset != SFRAME_S390X_SP_VAL_OFFSET))))
    {
      as_warn (_("no SFrame FDE emitted; %s with %s reg %u"),
	       cfi_esc_p ? ".cfi_escape DW_CFA_val_offset" : ".cfi_val_offset",
	       sframe_register_name (cfi_insn->u.ri.reg), cfi_insn->u.ri.reg);
      return SFRAME_XLATE_ERR_NOTREPRESENTED; /* Not represented.  */
    }

  /* Safe to skip.  */
  return SFRAME_XLATE_OK;
}

/* S390-specific translate DW_CFA_register into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
s390_sframe_xlate_do_register (struct sframe_xlate_ctx *xlate_ctx,
			       struct cfi_insn_data *cfi_insn)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);

  /* Change the rule for the register indicated by the register number to
     be the specified register.  Encode the register number as offset by
     shifting it to the left by one and setting the least-significant bit
     (LSB).  The LSB can be used to differentiate offsets from register
     numbers, as offsets from CFA are always a multiple of -8 on s390x.  */
  if (cfi_insn->u.rr.reg1 == SFRAME_CFA_FP_REG)
    sframe_fre_set_bp_track (cur_fre,
			     SFRAME_V2_S390X_OFFSET_ENCODE_REGNUM (cfi_insn->u.rr.reg2));
  else if (sframe_ra_tracking_p ()
	   && cfi_insn->u.rr.reg1 == SFRAME_CFA_RA_REG)
    sframe_fre_set_ra_track (cur_fre,
			     SFRAME_V2_S390X_OFFSET_ENCODE_REGNUM (cfi_insn->u.rr.reg2));

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_register into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_register (struct sframe_xlate_ctx *xlate_ctx ATTRIBUTE_UNUSED,
			  struct cfi_insn_data *cfi_insn)
{
  /* Conditionally invoke S390-specific implementation.  */
  if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG)
    return s390_sframe_xlate_do_register (xlate_ctx, cfi_insn);

  /* Previous value of register1 is register2.  However, if the specified
     register1 is not interesting (FP or RA reg), the current DW_CFA_register
     instruction can be safely skipped without sacrificing the asynchronicity of
     stack trace information.  */
  if (cfi_insn->u.rr.reg1 == SFRAME_CFA_FP_REG
      || (sframe_ra_tracking_p () && cfi_insn->u.rr.reg1 == SFRAME_CFA_RA_REG)
      /* Ignore SP reg, as it can be recovered from the CFA tracking info.  */
      )
    {
      as_warn (_("no SFrame FDE emitted; %s register %u in .cfi_register"),
	       sframe_register_name (cfi_insn->u.rr.reg1), cfi_insn->u.rr.reg1);
      return SFRAME_XLATE_ERR_NOTREPRESENTED;  /* Not represented.  */
    }

  /* Safe to skip.  */
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_remember_state into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_remember_state (struct sframe_xlate_ctx *xlate_ctx)
{
  struct sframe_row_entry *last_fre = xlate_ctx->last_fre;

  /* If there is no FRE state to remember, nothing to do here.  Return
     early with non-zero error code, this will cause no SFrame stack trace
     info for the function involved.  */
  if (!last_fre)
    {
      as_warn (_("no SFrame FDE emitted; "
		 ".cfi_remember_state without prior SFrame FRE state"));
      return SFRAME_XLATE_ERR_INVAL;
    }

  if (!xlate_ctx->remember_fre)
    xlate_ctx->remember_fre = sframe_row_entry_new ();
  sframe_row_entry_initialize (xlate_ctx->remember_fre, last_fre);

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_restore_state into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_restore_state (struct sframe_xlate_ctx *xlate_ctx)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (xlate_ctx->remember_fre);
  gas_assert (cur_fre && cur_fre->merge_candidate);

  /* Get the CFA state from the DW_CFA_remember_state insn.  */
  sframe_row_entry_initialize (cur_fre, xlate_ctx->remember_fre);
  /* The PC boundaries of the current SFrame FRE are updated
     via other machinery.  */
  cur_fre->merge_candidate = false;
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_restore into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_restore (struct sframe_xlate_ctx *xlate_ctx,
			 struct cfi_insn_data *cfi_insn)
{
  struct sframe_row_entry *cie_fre = xlate_ctx->first_fre;
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  /* PR gas/33170.  It is valid to have a:
       .cfi_restore N
    even at the entry of a function; in which case cie_fre is not yet setup.
    Point cie_fre to cur_fre, and let the machinery proceed to update
    merge_candidate as usual.  */
  if (cie_fre == NULL)
    cie_fre = cur_fre;

  /* Change the rule for the indicated register to the rule assigned to
     it by the initial_instructions in the CIE.  SFrame FREs track only CFA
     and FP / RA for backtracing purposes; skip the other .cfi_restore
     directives.  */
  if (cfi_insn->u.r == SFRAME_CFA_FP_REG)
    {
      gas_assert (cur_fre);
      cur_fre->bp_loc = cie_fre->bp_loc;
      cur_fre->bp_offset = cie_fre->bp_offset;
      cur_fre->merge_candidate = false;
    }
  else if (sframe_ra_tracking_p ()
	   && cfi_insn->u.r == SFRAME_CFA_RA_REG)
    {
      gas_assert (cur_fre);
      cur_fre->ra_loc = cie_fre->ra_loc;
      cur_fre->ra_offset = cie_fre->ra_offset;
      cur_fre->merge_candidate = false;
    }
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_AARCH64_negate_ra_state into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_aarch64_negate_ra_state (struct sframe_xlate_ctx *xlate_ctx,
					 struct cfi_insn_data *cfi_insn ATTRIBUTE_UNUSED)
{
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);
  /* Toggle the mangled RA status bit.  */
  cur_fre->mangled_ra_p = !cur_fre->mangled_ra_p;
  cur_fre->merge_candidate = false;

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_AARCH64_negate_ra_state_with_pc into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_aarch64_negate_ra_state_with_pc (struct sframe_xlate_ctx *xlate_ctx ATTRIBUTE_UNUSED,
						 struct cfi_insn_data *cfi_insn ATTRIBUTE_UNUSED)
{
  as_warn (_("no SFrame FDE emitted; .cfi_negate_ra_state_with_pc"));
  /* The used signing method should be encoded inside the FDE in SFrame v3.
     For now, PAuth_LR extension is not supported with SFrame.  */
  return SFRAME_XLATE_ERR_NOTREPRESENTED;  /* Not represented.  */
}

/* Translate DW_CFA_GNU_window_save into SFrame context.
   DW_CFA_GNU_window_save is a DWARF Sparc extension, but is multiplexed with a
   directive of DWARF AArch64 extension: DW_CFA_AARCH64_negate_ra_state.
   The AArch64 backend of GCC 14 and older versions was emitting mistakenly the
   Sparc CFI directive (.cfi_window_save).  From GCC 15, the AArch64 backend
   only emits .cfi_negate_ra_state.  For backward compatibility, the handler for
   .cfi_window_save needs to check whether the directive was used in a AArch64
   ABI context or not.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_gnu_window_save (struct sframe_xlate_ctx *xlate_ctx,
				 struct cfi_insn_data *cfi_insn)
{
  unsigned char abi_arch = sframe_get_abi_arch ();

  /* Translate DW_CFA_AARCH64_negate_ra_state into SFrame context.  */
  if (abi_arch == SFRAME_ABI_AARCH64_ENDIAN_BIG
      || abi_arch == SFRAME_ABI_AARCH64_ENDIAN_LITTLE)
    return sframe_xlate_do_aarch64_negate_ra_state (xlate_ctx, cfi_insn);

  as_warn (_("no SFrame FDE emitted; .cfi_window_save"));
  return SFRAME_XLATE_ERR_NOTREPRESENTED;  /* Not represented.  */
}

/* Handle DW_CFA_expression in .cfi_escape.

   As with sframe_xlate_do_cfi_escape, the intent of this function is to detect
   only the simple-to-process but common cases, where skipping over the escape
   expr data does not affect correctness of the SFrame stack trace data.

   Sets CALLER_WARN_P for skipped cases (and returns SFRAME_XLATE_OK) where the
   caller must warn.  The caller then must also set
   SFRAME_XLATE_ERR_NOTREPRESENTED for their callers.  */

static int
sframe_xlate_do_escape_expr (const struct sframe_xlate_ctx *xlate_ctx,
			     const struct cfi_insn_data *cfi_insn,
			     bool *caller_warn_p)
{
  const struct cfi_escape_data *e = cfi_insn->u.esc;
  int err = SFRAME_XLATE_OK;
  unsigned int reg = 0;
  unsigned int i = 0;

  /* Check roughly for an expression
     DW_CFA_expression: r1 (rdx) (DW_OP_bregN (reg): OFFSET).  */
#define CFI_ESC_NUM_EXP 4
  offsetT items[CFI_ESC_NUM_EXP] = {0};
  while (e->next)
    {
      e = e->next;
      if ((i == 2 && (items[1] != 2)) /* Expected len of 2 in DWARF expr.  */
	  /* We do not care for the exact values of items[2] and items[3],
	     so an explicit check for O_constant isnt necessary either.  */
	  || i >= CFI_ESC_NUM_EXP
	  || (i < 2
	      && (e->exp.X_op != O_constant
	 	  || e->type != CFI_ESC_byte
		  || e->reloc != TC_PARSE_CONS_RETURN_NONE)))
	goto warn_and_exit;
      items[i] = e->exp.X_add_number;
      i++;
    }

  if (i <= CFI_ESC_NUM_EXP - 1)
    goto warn_and_exit;

  /* reg operand to DW_CFA_expression is ULEB128.  For the purpose at hand,
     however, the register value will be less than 128 (CFI_ESC_NUM_EXP set
     to 4).  See an extended comment in sframe_xlate_do_escape_expr for why
     reading ULEB is okay to skip without sacrificing correctness.  */
  reg = items[0];
#undef CFI_ESC_NUM_EXP

  if (reg == SFRAME_CFA_SP_REG || reg == SFRAME_CFA_FP_REG
      || (sframe_ra_tracking_p () && reg == SFRAME_CFA_RA_REG)
      || reg == xlate_ctx->cur_fre->cfa_base_reg)
    {
      as_warn (_("no SFrame FDE emitted; "
		 ".cfi_escape DW_CFA_expression with %s reg %u"),
	       sframe_register_name (reg), reg);
      err = SFRAME_XLATE_ERR_NOTREPRESENTED;
    }
  /* else safe to skip, so continue to return SFRAME_XLATE_OK.  */

  return err;

warn_and_exit:
  *caller_warn_p = true;
  return err;
}

/* Handle DW_CFA_val_offset in .cfi_escape.

   As with sframe_xlate_do_cfi_escape, the intent of this function is to detect
   only the simple-to-process but common cases, where skipping over the escape
   expr data does not affect correctness of the SFrame stack trace data.

   Sets CALLER_WARN_P for skipped cases (and returns SFRAME_XLATE_OK) where the
   caller must warn.  The caller then must also set
   SFRAME_XLATE_ERR_NOTREPRESENTED for their callers.  */

static int
sframe_xlate_do_escape_val_offset (const struct sframe_xlate_ctx *xlate_ctx,
				   const struct cfi_insn_data *cfi_insn,
				   bool *caller_warn_p)
{
  const struct cfi_escape_data *e = cfi_insn->u.esc;
  int err = SFRAME_XLATE_OK;
  unsigned int i = 0;
  unsigned int reg;
  offsetT offset;

  /* Check for (DW_CFA_val_offset reg scaled_offset) sequence.  */
#define CFI_ESC_NUM_EXP 2
  offsetT items[CFI_ESC_NUM_EXP] = {0};
  while (e->next)
    {
      e = e->next;
      if (i >= CFI_ESC_NUM_EXP || e->exp.X_op != O_constant
	  || e->type != CFI_ESC_byte
	  || e->reloc != TC_PARSE_CONS_RETURN_NONE)
	goto warn_and_exit;
      items[i] = e->exp.X_add_number;
      i++;
    }
  if (i <= CFI_ESC_NUM_EXP - 1)
    goto warn_and_exit;

  /* Both arguments to DW_CFA_val_offset are ULEB128.  Especially with APX (on
     x86) we're going to see DWARF register numbers above 127, for the extended
     GPRs.  And large enough stack frames would also require multi-byte offset
     representation.  However, since we limit our focus on cases when
     CFI_ESC_NUM_EXP is 2, reading ULEB can be skipped.  IOW, although not
     ideal, SFrame FDE generation in case of an APX register in
     DW_CFA_val_offset is being skipped (PS: this does _not_ mean incorrect
     SFrame stack trace data).

     Recall that the intent here is to check for simple and prevalent cases,
     when feasible.  */

  reg = items[0];
  offset = items[1];
#undef CFI_ESC_NUM_EXP

  /* Invoke sframe_xlate_do_val_offset itself for checking.  */
  struct cfi_insn_data temp = {
    .insn = DW_CFA_val_offset,
    .u = {
      .ri = {
	.reg = reg,
	.offset = offset * DWARF2_CIE_DATA_ALIGNMENT
      }
    }
  };
  err = sframe_xlate_do_val_offset (xlate_ctx, &temp, true);
  return err;

warn_and_exit:
  *caller_warn_p = true;
  return err;
}

/* Handle CFI_escape in SFrame context.

   .cfi_escape CFI directive allows the user to add arbitrary data to the
   unwind info.  DWARF expressions commonly follow after CFI_escape (fake CFI)
   DWARF opcode.  One might also use CFI_escape to add OS-specific CFI opcodes
   even.

   Complex unwind info added using .cfi_escape directive _may_ be of no
   consequence for SFrame when the affected registers are not SP, FP, RA or
   CFA.  The challenge in confirming the afore-mentioned is that it needs full
   parsing (and validation) of the data presented after .cfi_escape.  Here we
   take a case-by-case approach towards skipping _some_ instances of
   .cfi_escape: skip those that can be *easily* determined to be harmless in
   the context of SFrame stack trace information.

   This function partially processes data following .cfi_escape and returns
   SFRAME_XLATE_OK if OK to skip.  */

static int
sframe_xlate_do_cfi_escape (const struct sframe_xlate_ctx *xlate_ctx,
			    const struct cfi_insn_data *cfi_insn)
{
  const struct cfi_escape_data *e;
  bool warn_p = false;
  int err = SFRAME_XLATE_OK;
  offsetT firstop;

  e = cfi_insn->u.esc;

  if (!e)
    return SFRAME_XLATE_ERR_INVAL;

  if (e->exp.X_op != O_constant
      || e->type != CFI_ESC_byte
      || e->reloc != TC_PARSE_CONS_RETURN_NONE)
    return SFRAME_XLATE_ERR_NOTREPRESENTED;

  firstop = e->exp.X_add_number;
  switch (firstop)
    {
    case DW_CFA_nop:
      /* One or more nops together are harmless for SFrame.  */
      while (e->next)
	{
	  e = e->next;
	  if (e->exp.X_op != O_constant || e->exp.X_add_number != DW_CFA_nop
	      || e->type != CFI_ESC_byte
	      || e->reloc != TC_PARSE_CONS_RETURN_NONE)
	    {
	      warn_p = true;
	      break;
	    }
	}
      break;

    case DW_CFA_expression:
      err = sframe_xlate_do_escape_expr (xlate_ctx, cfi_insn, &warn_p);
      break;

    case DW_CFA_val_offset:
      err = sframe_xlate_do_escape_val_offset (xlate_ctx, cfi_insn, &warn_p);
      break;

    /* FIXME - Also add processing for DW_CFA_GNU_args_size in future?  */

    default:
      warn_p = true;
      break;
    }

  if (warn_p)
    {
      /* In all other cases (e.g., DW_CFA_def_cfa_expression or other
	 OS-specific CFI opcodes), skip inspecting the DWARF expression.
	 This may impact the asynchronicity due to loss of coverage.
	 Continue to warn the user and bail out.  */
      as_warn (_("no SFrame FDE emitted; .cfi_escape with op (%#lx)"),
	       (unsigned long)firstop);
      err = SFRAME_XLATE_ERR_NOTREPRESENTED;
    }

  return err;
}

/* Translate DW_CFA_undefined into SFrame context.

   DW_CFA_undefined op indicates that from now on, the previous value of
   register can’t be restored anymore.  In SFrame stack trace, we cannot
   represent such a semantic.  So, we skip generating an SFrame FDE for this,
   when a register of interest is used with DW_CFA_undefined.

   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_cfi_undefined (const struct sframe_xlate_ctx *xlate_ctx ATTRIBUTE_UNUSED,
			       const struct cfi_insn_data *cfi_insn)
{
  if (cfi_insn->u.r == SFRAME_CFA_FP_REG
      || cfi_insn->u.r == SFRAME_CFA_RA_REG
      || cfi_insn->u.r == SFRAME_CFA_SP_REG)
    {
      as_warn (_("no SFrame FDE emitted; %s reg %u in .cfi_undefined"),
	       sframe_register_name (cfi_insn->u.r), cfi_insn->u.r);
      return SFRAME_XLATE_ERR_NOTREPRESENTED; /* Not represented.  */
    }

  /* Safe to skip.  */
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_same_value into SFrame context.

   DW_CFA_same_value op indicates that current value of register is the same as
   in the previous frame, i.e. no restoration needed.  In SFrame stack trace
   format, the handling is done similar to DW_CFA_restore.

   For SFRAME_CFA_RA_REG, if RA-tracking is enabled, reset the SFrame FRE state
   for REG_RA to indicate that register does not need restoration.  P.S.: Even
   though resetting just REG_RA may be contradicting the AArch64 ABI (as Frame
   Record contains for FP and LR), sframe_xlate_do_same_value () does not
   detect the case and assumes the users' DW_CFA_same_value SFRAME_CFA_RA_REG
   has a sound reason.  For ABIs, where RA-tracking is disabled, handle it
   similar to DW_CFA_restore: ignore the directive, it is safe to skip.  The
   reasoning is similar to that for DW_CFA_restore: if such a restoration was
   meant to be of any consequence, there must have been the necessary CFI
   directives for updating the CFA rule too such that the recovered RA from
   stack is valid.

   SFrame based stacktracers will implement CFA-based SP recovery for all ABIs:
   SP for previous frame is based on the applicable CFA-rule.  There is no
   representation in SFrame to indicate "no restoration needed" for REG_SP,
   when going to the previous frame.  That said, if DW_CFA_same_value is seen
   for SFRAME_CFA_SP_REG, handle it similar to DW_CFA_restore: ignore the
   directive, it is safe to skip.  The reasoning is similar to that for
   DW_CFA_restore: if such a restoration was meant to be of any consequence,
   there must have been the necessary CFI directives for updating the CFA rule
   too.  The latter will be duly processed by the SFrame generation code, as
   expected.

   For SFRAME_CFA_FP_REG, reset the state of the current FRE to indicate that
   the value is the same as previous frame.

   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_same_value (const struct sframe_xlate_ctx *xlate_ctx,
			    const struct cfi_insn_data *cfi_insn)
{
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  if (sframe_ra_tracking_p () && cfi_insn->u.r == SFRAME_CFA_RA_REG)
    {
      cur_fre->ra_loc = SFRAME_FRE_ELEM_LOC_REG;
      cur_fre->ra_offset = 0;
      cur_fre->merge_candidate = false;
    }
  else if (cfi_insn->u.r == SFRAME_CFA_FP_REG)
    {
      cur_fre->bp_loc = SFRAME_FRE_ELEM_LOC_REG;
      cur_fre->bp_offset = 0;
      cur_fre->merge_candidate = false;
    }

  /* Safe to skip.  */
  return SFRAME_XLATE_OK;
}

/* Returns the DWARF call frame instruction name or fake CFI name for the
   specified CFI opcode, or NULL if the value is not recognized.  */

static const char *
sframe_get_cfi_name (int cfi_opc)
{
  const char *cfi_name;

  switch (cfi_opc)
    {
      /* Fake CFI type; outside the byte range of any real CFI insn.  */
      /* See gas/dw2gencfi.h.  */
      case CFI_adjust_cfa_offset:
	cfi_name = "CFI_adjust_cfa_offset";
	break;
      case CFI_return_column:
	cfi_name = "CFI_return_column";
	break;
      case CFI_rel_offset:
	cfi_name = "CFI_rel_offset";
	break;
      case CFI_escape:
	cfi_name = "CFI_escape";
	break;
      case CFI_signal_frame:
	cfi_name = "CFI_signal_frame";
	break;
      case CFI_val_encoded_addr:
	cfi_name = "CFI_val_encoded_addr";
	break;
      case CFI_label:
	cfi_name = "CFI_label";
	break;
      default:
	cfi_name = get_DW_CFA_name (cfi_opc);
    }

  return cfi_name;
}

/* Process CFI_INSN and update the translation context with the FRE
   information.

   Returns an error code (sframe_xlate_err) if CFI_INSN is not successfully
   processed.  */

static int
sframe_do_cfi_insn (struct sframe_xlate_ctx *xlate_ctx,
		    struct cfi_insn_data *cfi_insn)
{
  int err = 0;

  /* Atleast one cfi_insn per FDE is expected.  */
  gas_assert (cfi_insn);
  int op = cfi_insn->insn;

  switch (op)
    {
    case DW_CFA_advance_loc:
      err = sframe_xlate_do_advance_loc (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_def_cfa:
      err = sframe_xlate_do_def_cfa (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_def_cfa_register:
      err = sframe_xlate_do_def_cfa_register (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_def_cfa_offset:
      err = sframe_xlate_do_def_cfa_offset (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_offset:
      err = sframe_xlate_do_offset (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_val_offset:
      err = sframe_xlate_do_val_offset (xlate_ctx, cfi_insn, false);
      break;
    case DW_CFA_remember_state:
      err = sframe_xlate_do_remember_state (xlate_ctx);
      break;
    case DW_CFA_restore_state:
      err = sframe_xlate_do_restore_state (xlate_ctx);
      break;
    case DW_CFA_restore:
      err = sframe_xlate_do_restore (xlate_ctx, cfi_insn);
      break;
    /* DW_CFA_AARCH64_negate_ra_state is multiplexed with
       DW_CFA_GNU_window_save.  */
    case DW_CFA_GNU_window_save:
      err = sframe_xlate_do_gnu_window_save (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_AARCH64_negate_ra_state_with_pc:
      err = sframe_xlate_do_aarch64_negate_ra_state_with_pc (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_register:
      err = sframe_xlate_do_register (xlate_ctx, cfi_insn);
      break;
    case CFI_escape:
      err = sframe_xlate_do_cfi_escape (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_undefined:
      err = sframe_xlate_do_cfi_undefined (xlate_ctx, cfi_insn);
      break;
    case DW_CFA_same_value:
      err = sframe_xlate_do_same_value (xlate_ctx, cfi_insn);
      break;
    default:
      /* Other skipped operations may, however, impact the asynchronicity.  */
      {
	const char *cfi_name = sframe_get_cfi_name (op);

	if (!cfi_name)
	  cfi_name = _("(unknown)");
	as_warn (_("no SFrame FDE emitted; CFI insn %s (%#x)"),
		 cfi_name, op);
	err = SFRAME_XLATE_ERR_NOTREPRESENTED;
      }
    }

  /* Any error will cause no SFrame FDE later.  The user has already been
     warned.  */
  return err;
}


static int
sframe_do_fde (struct sframe_xlate_ctx *xlate_ctx,
	       const struct fde_entry *dw_fde)
{
  struct cfi_insn_data *cfi_insn;
  int err = SFRAME_XLATE_OK;

  xlate_ctx->dw_fde = dw_fde;

  /* SFrame format cannot represent a non-default DWARF return column reg.  */
  if (xlate_ctx->dw_fde->return_column != DWARF2_DEFAULT_RETURN_COLUMN)
    {
      as_warn (_("no SFrame FDE emitted; non-default RA register %u"),
	       xlate_ctx->dw_fde->return_column);
      return SFRAME_XLATE_ERR_NOTREPRESENTED;
    }

  /* Iterate over the CFIs and create SFrame FREs.  */
  for (cfi_insn = dw_fde->data; cfi_insn; cfi_insn = cfi_insn->next)
    {
      /* Translate each CFI, and buffer the state in translation context.  */
      err = sframe_do_cfi_insn (xlate_ctx, cfi_insn);
      if (err != SFRAME_XLATE_OK)
	{
	  /* Skip generating SFrame stack trace info for the function if any
	     offending CFI is encountered by sframe_do_cfi_insn ().  Warning
	     message already printed by sframe_do_cfi_insn ().  */
	  return err; /* Return the error code.  */
	}
    }

  /* Link in the scratchpad FRE that the last few CFI insns helped create.  */
  if (xlate_ctx->cur_fre)
    {
      sframe_xlate_ctx_add_fre (xlate_ctx, xlate_ctx->cur_fre);
      xlate_ctx->cur_fre = NULL;
    }
  /* Designate the end of the last SFrame FRE.  */
  if (xlate_ctx->last_fre)
    {
      xlate_ctx->last_fre->pc_end
	= get_dw_fde_end_addrS (xlate_ctx->dw_fde);
    }

  /* ABI/arch except s390x cannot represent FP without RA saved.  */
  if (sframe_ra_tracking_p ()
      && sframe_get_abi_arch () != SFRAME_ABI_S390X_ENDIAN_BIG)
    {
      struct sframe_row_entry *fre;

      /* Iterate over the scratchpad FREs and validate them.  */
      for (fre = xlate_ctx->first_fre; fre; fre = fre->next)
	{
	  /* SFrame format cannot represent FP on stack without RA on stack.  */
	  if (fre->ra_loc != SFRAME_FRE_ELEM_LOC_STACK
	      && fre->bp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	    {
	      as_warn (_("no SFrame FDE emitted; FP without RA on stack"));
	      return SFRAME_XLATE_ERR_NOTREPRESENTED;
	    }
	}
    }

  return SFRAME_XLATE_OK;
}

/* Create SFrame stack trace info for all functions.

   This function consumes the already generated DWARF FDEs (by dw2gencfi) and
   generates data which is later emitted as stack trace information encoded in
   the SFrame format.  */

static void
create_sframe_all (void)
{
  struct fde_entry *dw_fde = NULL;
  struct sframe_func_entry *sframe_fde = NULL;

  struct sframe_xlate_ctx *xlate_ctx = sframe_xlate_ctx_alloc ();

  for (dw_fde = all_fde_data; dw_fde ; dw_fde = dw_fde->next)
    {
      sframe_fde = sframe_fde_alloc ();
      /* Initialize the translation context with information anew.  */
      sframe_xlate_ctx_init (xlate_ctx);

      /* Process and link SFrame FDEs if no error.  Also skip adding an SFrame
	 FDE if it does not contain any SFrame FREs.  There is little use of an
	 SFrame FDE if there is no stack tracing information for the
	 function.  */
      int err = sframe_do_fde (xlate_ctx, dw_fde);
      if (err || xlate_ctx->num_xlate_fres == 0)
	{
	  sframe_xlate_ctx_cleanup (xlate_ctx);
	  sframe_fde_free (sframe_fde);
	}
      else
	{
	  /* All done.  Transfer the state from the SFrame translation
	     context to the SFrame FDE.  */
	  sframe_xlate_ctx_finalize (xlate_ctx, sframe_fde);
	  *last_sframe_fde = sframe_fde;
	  last_sframe_fde = &sframe_fde->next;
	}
    }

  XDELETE (xlate_ctx);
}

void
output_sframe (segT sframe_seg)
{
  (void) sframe_seg;

  /* Setup the version specific access functions.  */
  sframe_set_version (SFRAME_VERSION_2);

  /* Process all fdes and create SFrame stack trace information.  */
  create_sframe_all ();

  output_sframe_internal ();
}

#else  /*  support_sframe_p  */

void
output_sframe (segT sframe_seg ATTRIBUTE_UNUSED)
{
}

#endif /*  support_sframe_p  */
