/* gen-sframe.c - Support for generating SFrame section.
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

#include "as.h"
#include "subsegs.h"
#include "sframe.h"
#include "sframe-internal.h"
#include "gen-sframe.h"
#include "dw2gencfi.h"
#include "leb128.h"

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

/* gas emits SFrame Version 3 only at this time.  */
typedef sframe_func_desc_idx_v3 sframe_func_desc_idx;

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

/* Get whether signal frame.  */
static bool
get_dw_fde_signal_p (const struct fde_entry *dw_fde)
{
  return (dw_fde->signal_frame == 1);
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

/* All stack offsets in SFrame stack trace format must be representable as a
   1-byte (SFRAME_FRE_DATAWORD_1B), 2-byte (SFRAME_FRE_DATAWORD_2B) or 4-byte
   (SFRAME_FRE_DATAWORD_4B) value.

   At the moment, sanity check on CFA offset (only) is performed to address PR
   gas/33277.  Arguably, such updates to ra_offset or fp_offset will only
   follow after updates to cfa_offset in a real-world, useful program.  */

static bool
sframe_fre_stack_offset_bound_p (offsetT offset, bool cfa_reg_p)
{
  /* For s390x, CFA offset is adjusted to enable 8-bit offsets.  */
  if (cfa_reg_p && sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG)
    offset = SFRAME_V2_S390X_CFA_OFFSET_ENCODE (offset);

  return (offset >= INT32_MIN && offset <= INT32_MAX);
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
  fre->ra_undefined_p = false;
  fre->merge_candidate = false;
}

static void
sframe_fre_set_fp_track (struct sframe_row_entry *fre, offsetT fp_offset)
{
  fre->fp_loc = SFRAME_FRE_ELEM_LOC_STACK;
  fre->fp_offset = fp_offset;
  fre->merge_candidate = false;
}

/* Given a signed offset, return the size in bytes needed to represent it.  */

static unsigned int
get_offset_size_in_bytes (offsetT value)
{
  unsigned int size = 0;

  if (value == (int8_t)value)
    size = 1;
  else if (value == (int16_t)value)
    size = 2;
  else if (value == (int32_t)value)
    size = 4;
  else
    return 8;

  return size;
}

/* Given an unsigned item, return the size in bytes needed to represent it.  */

static unsigned int
get_udata_size_in_bytes (unsigned int value)
{
  unsigned int size = 0;

  if (value <= UINT8_MAX)
    size = 1;
  else if (value <= UINT16_MAX)
    size = 2;
  else
    size = 4;

  return size;
}
#define SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_1B  0 /* SFRAME_FRE_DATAWORD_1B.  */
#define SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_2B  1 /* SFRAME_FRE_DATAWORD_2B.  */
#define SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_4B  2 /* SFRAME_FRE_DATAWORD_4B.  */
#define SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_8B  3 /* Not supported in SFrame.  */
#define SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_MAX \
  SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_8B

/* Helper struct for mapping FRE data word size to output functions.  */

struct sframe_fre_dataword_func_map
{
  unsigned int dataword_size;
  void (*out_func)(int);
};

/* Given an DATAWORD_SIZE, return the size in bytes needed to represent it.  */

static unsigned int
sframe_fre_dataword_func_map_index (unsigned int dataword_size)
{
  unsigned int idx = SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_MAX;

  switch (dataword_size)
    {
      case SFRAME_FRE_DATAWORD_1B:
	idx = SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_1B;
	break;
      case SFRAME_FRE_DATAWORD_2B:
	idx = SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_2B;
	break;
      case SFRAME_FRE_DATAWORD_4B:
	idx = SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_4B;
	break;
      default:
	/* Not supported in SFrame.  */
	break;
    }

  return idx;
}

/* Mapping from data word size to the output function to emit the value.  */

static const
struct sframe_fre_dataword_func_map
dataword_func_map[SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_MAX+1] =
{
  { SFRAME_FRE_DATAWORD_1B, out_one },
  { SFRAME_FRE_DATAWORD_2B, out_two },
  { SFRAME_FRE_DATAWORD_4B, out_four },
  { -1, NULL } /* Not Supported in SFrame.  */
};

/* SFrame version specific operations access.  */

static struct sframe_version_ops sframe_ver_ops;

/* SFrame (SFRAME_VERSION_1) set FRE info.  */

static unsigned char
sframe_v1_set_fre_info (unsigned int cfa_base_reg, unsigned int dataword_count,
			unsigned int dataword_size, bool mangled_ra_p)
{
  unsigned char fre_info;
  fre_info = SFRAME_V1_FRE_INFO (cfa_base_reg, dataword_count, dataword_size);
  fre_info = SFRAME_V1_FRE_INFO_UPDATE_MANGLED_RA_P (mangled_ra_p, fre_info);
  return fre_info;
}

/* SFrame (SFRAME_VERSION_3) set function info.  */

static unsigned char
sframe_v3_set_func_info (unsigned int fde_pc_type, unsigned int fre_type,
			 unsigned int pauth_key, bool signal_p)
{
  unsigned char func_info;
  func_info = SFRAME_V3_FDE_FUNC_INFO (fde_pc_type, fre_type);
  func_info = SFRAME_V3_FDE_UPDATE_PAUTH_KEY (pauth_key, func_info);
  func_info = SFRAME_V3_FDE_UPDATE_SIGNAL_P (signal_p, func_info);
  return func_info;
}

/* SFrame version specific operations setup.  */

static void
sframe_set_version (enum gen_sframe_version flag_ver)
{
  if (flag_ver == GEN_SFRAME_VERSION_3)
    {
      sframe_ver_ops.format_version = SFRAME_VERSION_3;
      /* These operations remain the same for SFRAME_VERSION_3 as fre_info and
	 func_info layout has not changed from SFRAME_VERSION_2 and
	 SFRAME_VERSION_1.  */
      sframe_ver_ops.set_fre_info = sframe_v1_set_fre_info;
      sframe_ver_ops.set_func_info = sframe_v3_set_func_info;
    }
}

/* SFrame set FRE info.  */

static unsigned char
sframe_set_fre_info (unsigned int cfa_base_reg, unsigned int dataword_count,
		     unsigned int dataword_size, bool mangled_ra_p)
{
  return sframe_ver_ops.set_fre_info (cfa_base_reg, dataword_count,
				      dataword_size, mangled_ra_p);
}

/* SFrame set func info. */

static unsigned char
sframe_set_func_info (unsigned int fde_type, unsigned int fre_type,
		      unsigned int pauth_key, bool signal_p)
{
  return sframe_ver_ops.set_func_info (fde_type, fre_type, pauth_key,
				       signal_p);
}

/* Get the number of SFrame FDEs for the current file.  */

static unsigned int
get_num_sframe_fdes (void);

/* Get the number of SFrame frame row entries for the current file.  */

static unsigned int
get_num_sframe_fres (void);

/* Get CFA base register ID as represented in SFrame Frame Row Entry.  */

static unsigned int
get_fre_base_reg_id (const struct sframe_row_entry *sframe_fre)
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

/* Get number of data words necessary for the SFrame Frame Row Entry.  */

static unsigned int
get_fre_dataword_count (const struct sframe_row_entry *sframe_fre, bool flex_p)
{
  /* For SFRAME_FDE_TYPE_FLEX FDE type, each entity (CFA, FP, RA) may carry up
     to two data words.  */
  unsigned int count = flex_p ? 2 : 1;

  /* CFA data word (or data words when flex_p) must always be present.  */
  unsigned int fre_dataword_count = count;

  /* For flexible FDE type, there will be two data words for RA (if RA
     has a recovery rule applicable).  1 padding data word otherwise.  */
  if (flex_p)
    {
     if (sframe_fre->ra_loc != SFRAME_FRE_ELEM_LOC_NONE)
       fre_dataword_count += count;
     else if (sframe_fre->fp_loc != SFRAME_FRE_ELEM_LOC_NONE)
       fre_dataword_count += 1;
    }
  else if (sframe_ra_tracking_p ()
	   && (sframe_fre->ra_loc != SFRAME_FRE_ELEM_LOC_NONE
	       /* For s390x account padding RA data word, if FP without RA
		  saved.  */
	       || (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
		   && sframe_fre->fp_loc != SFRAME_FRE_ELEM_LOC_NONE)))
    fre_dataword_count++;

  if (sframe_fre->fp_loc != SFRAME_FRE_ELEM_LOC_NONE)
    fre_dataword_count += count;

  return fre_dataword_count;
}

/* Get the minimum necessary data word size (in bytes) for this
   SFrame frame row entry.  */

static unsigned int
sframe_get_fre_dataword_size (const struct sframe_row_entry *sframe_fre,
			    bool flex_p)
{
  unsigned int max_dataword_size = 0;
  unsigned int cfa_offset_size = 0;
  unsigned int fp_offset_size = 0;
  unsigned int ra_offset_size = 0;

  unsigned int fre_dataword_size = 0;

  /* What size of data words appear in this frame row entry.  */
  cfa_offset_size = get_offset_size_in_bytes (sframe_fre->cfa_offset);
  if (sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_STACK)
    fp_offset_size = get_offset_size_in_bytes (sframe_fre->fp_offset);
  if (sframe_ra_tracking_p ())
    {
      if (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_STACK)
	ra_offset_size = get_offset_size_in_bytes (sframe_fre->ra_offset);
      /* For s390x account padding RA offset, if FP without RA saved.  */
      else if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
	       && sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	ra_offset_size = get_offset_size_in_bytes (SFRAME_FRE_RA_OFFSET_INVALID);
    }

  /* Get the maximum size needed to represent the offsets.  */
  max_dataword_size = cfa_offset_size;
  if (fp_offset_size > max_dataword_size)
    max_dataword_size = fp_offset_size;
  if (ra_offset_size > max_dataword_size)
    max_dataword_size = ra_offset_size;

  /* If flex FDE, account for reg data too.  */
  if (flex_p)
    {
      bool reg_p = (sframe_fre->cfa_base_reg != SFRAME_FRE_REG_INVALID);
      unsigned int data
	= SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (sframe_fre->cfa_base_reg,
					      sframe_fre->cfa_deref_p, reg_p);
      unsigned int cfa_control_word_size = get_udata_size_in_bytes (data);
      if (cfa_control_word_size > max_dataword_size)
	max_dataword_size = cfa_control_word_size;

      if (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_REG)
	{
	  data = SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (sframe_fre->ra_reg,
						     sframe_fre->ra_deref_p,
						     1 /* reg_p.  */);
	  unsigned ra_control_word_size = get_udata_size_in_bytes (data);
	  if (ra_control_word_size > max_dataword_size)
	    max_dataword_size = ra_control_word_size;
	}

      if (sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_REG)
	{
	  data = SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (sframe_fre->fp_reg,
						     sframe_fre->fp_deref_p,
						     1 /* reg_p.  */);
	  unsigned fp_control_word_size = get_udata_size_in_bytes (data);
	  if (fp_control_word_size > max_dataword_size)
	    max_dataword_size = fp_control_word_size;
	}
    }

  gas_assert (max_dataword_size);

  switch (max_dataword_size)
    {
    case 1:
      fre_dataword_size = SFRAME_FRE_DATAWORD_1B;
      break;
    case 2:
      fre_dataword_size = SFRAME_FRE_DATAWORD_2B;
      break;
    case 4:
      fre_dataword_size = SFRAME_FRE_DATAWORD_4B;
      break;
    default:
      /* FRE data words of size 8 bytes is not supported in SFrame.  */
      as_fatal (_("SFrame unsupported FRE data word size\n"));
      break;
    }

  return fre_dataword_size;
}

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
  /* val expression stores the FDE start address offset from the start PC
     of function.  */
  expressionS val = {
    .X_op = O_subtract,
    .X_add_symbol = fre_pc_begin,
    .X_op_symbol = fde_start_address,
  };

  /* width expressions stores the size of the function.  This is used later
     to determine the number of bytes to be used to encode the FRE start
     address of each FRE of the function.  */
  expressionS width = {
    .X_op = O_subtract,
    .X_add_symbol = fde_end_address,
    .X_op_symbol = fde_start_address,
  };

  *cexp = (expressionS) {
    .X_op = O_absent,
    .X_add_symbol = make_expr_symbol (&val),
    .X_op_symbol = make_expr_symbol (&width)
  };
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
  expressionS width = {
    .X_op = O_subtract,
    .X_add_symbol = dw_fde_end_addrS,
    .X_op_symbol = dw_fde_start_addrS
  };

  expressionS rest_of_func_info = {
    .X_op = O_constant,
    .X_add_number = func_info
  };

  *cexp = (expressionS) {
    .X_op = O_modulus,
    .X_add_symbol = make_expr_symbol (&rest_of_func_info),
    .X_op_symbol = make_expr_symbol (&width)
  };
}

static struct sframe_row_entry*
sframe_row_entry_new (void)
{
  struct sframe_row_entry *fre = XCNEW (struct sframe_row_entry);
  /* Reset all regs to SFRAME_FRE_REG_INVALID.  A value of 0 may imply a
     valid register for a supported arch.  */
  fre->cfa_base_reg = SFRAME_FRE_REG_INVALID;
  fre->fp_reg = SFRAME_FRE_REG_INVALID;
  fre->ra_reg = SFRAME_FRE_REG_INVALID;
  fre->merge_candidate = true;
  /* Reset the mangled RA status bit to zero by default.  We will
     initialize it in sframe_row_entry_initialize () with the sticky
     bit if set.  */
  fre->mangled_ra_p = false;
  /* Reset the RA undefined status by to zero by default.  */
  fre->ra_undefined_p = false;

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
  if (sframe_fde == NULL)
    return;

  if (sframe_fde->sframe_fres)
    sframe_row_entry_free (sframe_fde->sframe_fres);

  XDELETE (sframe_fde);
}

/* Output the varlen data (SFrame FRE data words) for SFrame FRE object
   SFRAME_FRE of the SFrame FDE object SFRAME_FDE.  Each emitted entry is of
   size FRE_DATAWORD_SIZE.  Write out the data words in order - CFA, RA, FP.  */

static unsigned int
output_sframe_row_entry_datawords (const struct sframe_func_entry *sframe_fde,
				   const struct sframe_row_entry *sframe_fre,
				   unsigned int fre_dataword_size)
{
  unsigned int fre_write_datawords = 0;

  unsigned int idx = sframe_fre_dataword_func_map_index (fre_dataword_size);
  gas_assert (idx < SFRAME_FRE_DATAWORD_FUNC_MAP_INDEX_MAX);

  if (sframe_fde->fde_flex_p)
    {
      /* SFrame FDE of type SFRAME_FDE_TYPE_FLEX.  */
      /* Output CFA related FRE data words.  */
      uint32_t reg = sframe_fre->cfa_base_reg;
      bool deref_p = sframe_fre->cfa_deref_p;
      uint32_t reg_data
	= SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (reg, deref_p, 1 /* reg_p.  */);
      offsetT offset_data = sframe_fre->cfa_offset;
      dataword_func_map[idx].out_func (reg_data);
      dataword_func_map[idx].out_func (offset_data);
      fre_write_datawords += 2;

      bool reg_p = false;
      if (sframe_fre->ra_loc != SFRAME_FRE_ELEM_LOC_NONE)
	{
	  /* Output RA related FRE data words.  */
	  reg_p = sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_REG;
	  reg = reg_p ? sframe_fre->ra_reg : 0;
	  deref_p = sframe_fre->ra_deref_p;
	  reg_data = SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (reg, deref_p, reg_p);

	  offset_data = sframe_fre->ra_offset;
	  dataword_func_map[idx].out_func (reg_data);
	  dataword_func_map[idx].out_func (offset_data);
	  fre_write_datawords += 2;
	}
      else if (sframe_fre->fp_loc != SFRAME_FRE_ELEM_LOC_NONE)
	{
	  /* If RA is not in REG/STACK, emit RA padding if there are more
	     data words to follow.  Note that, emitting
	     SFRAME_FRE_RA_OFFSET_INVALID is equivalent to emitting
	     SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (0, 0, 0).  */
	  dataword_func_map[idx].out_func (SFRAME_FRE_RA_OFFSET_INVALID);
	  fre_write_datawords += 1;
	}

      if (sframe_fre->fp_loc != SFRAME_FRE_ELEM_LOC_NONE)
	{
	  /* Output FP related FRE data words.  */
	  reg_p = sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_REG;
	  reg = reg_p ? sframe_fre->fp_reg : 0;
	  deref_p = sframe_fre->fp_deref_p;
	  reg_data = SFRAME_V3_FLEX_FDE_CTRLWORD_ENCODE (reg, deref_p, reg_p);

	  offset_data = sframe_fre->fp_offset;
	  dataword_func_map[idx].out_func (reg_data);
	  dataword_func_map[idx].out_func (offset_data);
	  fre_write_datawords += 2;
	}
    }
  else
    {
      /* SFrame FDE of type SFRAME_FDE_TYPE_DEFAULT.  */
      /* Output CFA related FRE data words.  */
      dataword_func_map[idx].out_func (sframe_fre->cfa_offset);
      fre_write_datawords++;

      if (sframe_ra_tracking_p ())
	{
	  if (sframe_fre->ra_loc == SFRAME_FRE_ELEM_LOC_STACK)
	    {
	      dataword_func_map[idx].out_func (sframe_fre->ra_offset);
	      fre_write_datawords++;
	    }
	  /* For s390x write padding RA offset, if FP without RA saved.  */
	  else if (sframe_get_abi_arch () == SFRAME_ABI_S390X_ENDIAN_BIG
		   && sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	    {
	      dataword_func_map[idx].out_func (SFRAME_FRE_RA_OFFSET_INVALID);
	      fre_write_datawords++;
	    }
	}
      if (sframe_fre->fp_loc == SFRAME_FRE_ELEM_LOC_STACK)
	{
	  dataword_func_map[idx].out_func (sframe_fre->fp_offset);
	  fre_write_datawords++;
	}
    }

  return fre_write_datawords;
}

static void
output_sframe_row_entry (const struct sframe_func_entry *sframe_fde,
			 const struct sframe_row_entry *sframe_fre)
{
  unsigned char fre_info;
  unsigned int fre_dataword_count;
  unsigned int fre_dataword_size;
  unsigned int fre_base_reg;
  bool fre_mangled_ra_p;
  expressionS exp;
  unsigned int fre_addr_size;

  unsigned int fre_write_datawords = 0;
  symbolS *fde_start_addr = get_dw_fde_start_addrS (sframe_fde->dw_fde);
  symbolS *fde_end_addr = get_dw_fde_end_addrS (sframe_fde->dw_fde);
  bool flex_p = sframe_fde->fde_flex_p;

  fre_addr_size = 4; /* 4 bytes by default.   FIXME tie it to fre_type? */

  /* SFrame FRE Start Address.  */
  if (SFRAME_FRE_TYPE_SELECTION_OPT)
    {
      create_fre_start_addr_exp (&exp, sframe_fre->pc_begin, fde_start_addr,
				 fde_end_addr);
      frag_grow (fre_addr_size);
      frag_var (rs_sframe, fre_addr_size, 0, 0,
		make_expr_symbol (&exp), 0, (char *) frag_now);
    }
  else
    {
      gas_assert (fde_end_addr);
      exp = (expressionS) {
	.X_op = O_subtract,
	.X_add_symbol = sframe_fre->pc_begin, /* to.  */
	.X_op_symbol = fde_start_addr /* from.  */
      };
      emit_expr (&exp, fre_addr_size);
    }

  /* Create the fre_info using the CFA base register, number of data words and
     max size of a data word in this FRE.  Represent RA undefined as FRE
     without any data words and all FRE info word fields zeroed.  */
  if (sframe_fre->ra_undefined_p)
    {
      fre_base_reg = 0;
      fre_dataword_count = 0;
      fre_dataword_size = 0;
      fre_mangled_ra_p = 0;
    }
  else
    {
      fre_base_reg = get_fre_base_reg_id (sframe_fre);
      fre_dataword_count = get_fre_dataword_count (sframe_fre, flex_p);
      fre_dataword_size = sframe_get_fre_dataword_size (sframe_fre, flex_p);
      fre_mangled_ra_p = sframe_fre->mangled_ra_p;
    }

  /* Unused for flex FDE.  Set to zero.  */
  if (flex_p)
    fre_base_reg = 0;

  fre_info = sframe_set_fre_info (fre_base_reg, fre_dataword_count,
				  fre_dataword_size, fre_mangled_ra_p);
  out_one (fre_info);

  /* Represent RA undefined as FRE without any data words.  */
  if (sframe_fre->ra_undefined_p)
    return;

  fre_write_datawords = output_sframe_row_entry_datawords (sframe_fde,
							   sframe_fre,
							   fre_dataword_size);

  /* Check if the expected number data words have been written out
     in this FRE.  */
  gas_assert (fre_write_datawords == fre_dataword_count);
}

static void
output_sframe_funcdesc_idx (symbolS *start_of_fre_section,
			    symbolS *fre_symbol,
			    const struct sframe_func_entry *sframe_fde)
{
  expressionS exp;
  symbolS *dw_fde_start_addrS, *dw_fde_end_addrS;

  dw_fde_start_addrS = get_dw_fde_start_addrS (sframe_fde->dw_fde);
  dw_fde_end_addrS = get_dw_fde_end_addrS (sframe_fde->dw_fde);

  /* Start address of the function.  gas always emits this value with encoding
     SFRAME_F_FDE_FUNC_START_PCREL.  See PR ld/32666.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = dw_fde_start_addrS; /* to location.  */
  exp.X_op_symbol = symbol_temp_new_now (); /* from location.  */
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_idx,
				  sfdi_func_start_offset));

  /* Size of the function in bytes.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = dw_fde_end_addrS;
  exp.X_op_symbol = dw_fde_start_addrS;
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_idx,
				  sfdi_func_size));

  /* Offset to the function data (attribtues, FREs) in the FRE subsection.  */
  exp.X_op = O_subtract;
  exp.X_add_symbol = fre_symbol; /* Minuend.  */
  exp.X_op_symbol = start_of_fre_section; /* Subtrahend.  */
  exp.X_add_number = 0;
  emit_expr (&exp, sizeof_member (sframe_func_desc_idx,
				  sfdi_func_start_fre_off));
}

static void
output_sframe_func_desc_attr (const struct sframe_func_entry *sframe_fde)
{
  symbolS *dw_fde_start_addrS, *dw_fde_end_addrS;
  unsigned int pauth_key;
  bool signal_p;

  dw_fde_start_addrS = get_dw_fde_start_addrS (sframe_fde->dw_fde);
  dw_fde_end_addrS = get_dw_fde_end_addrS (sframe_fde->dw_fde);

  /* Number of FREs must fit uint16_t.  */
  gas_assert (sframe_fde->num_fres <= UINT16_MAX);
  out_two (sframe_fde->num_fres);

  /* SFrame FDE function info.  */
  unsigned char func_info;
  pauth_key = (get_dw_fde_pauth_b_key_p (sframe_fde->dw_fde)
	       ? SFRAME_AARCH64_PAUTH_KEY_B : SFRAME_AARCH64_PAUTH_KEY_A);
  signal_p = get_dw_fde_signal_p (sframe_fde->dw_fde);
  func_info = sframe_set_func_info (SFRAME_V3_FDE_PCTYPE_INC,
				    SFRAME_FRE_TYPE_ADDR4,
				    pauth_key, signal_p);
  if (SFRAME_FRE_TYPE_SELECTION_OPT)
    {
      expressionS cexp;
      create_func_info_exp (&cexp, dw_fde_end_addrS, dw_fde_start_addrS,
			    func_info);
      frag_grow (1); /* Size of func info is unsigned char.  */
      frag_var (rs_sframe, 1, 0, 0, make_expr_symbol (&cexp), 0,
		(char *) frag_now);
    }
  else
    out_one (func_info);

  uint8_t finfo2 = 0;
  if (sframe_fde->fde_flex_p)
    finfo2 = SFRAME_V3_SET_FDE_TYPE (finfo2, SFRAME_FDE_TYPE_FLEX);
  out_one (finfo2);

  /* Currently, GAS only emits SFrame FDE with PC Type
     SFRAME_V3_FDE_PCTYPE_INC.  Emit repetitive block size of 0.  */
  out_one (0);
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
      output_sframe_funcdesc_idx (start_of_fre_section, fde_fre_symbols[i],
				  sframe_fde);
      i++;
    }

  symbol_set_value_now (start_of_fre_section);

  /* Output the SFrame FREs.  */
  i = 0;
  sframe_fde = all_sframe_fdes;

  for (sframe_fde = all_sframe_fdes; sframe_fde; sframe_fde = sframe_fde_next)
    {
      symbol_set_value_now (fde_fre_symbols[i]);

      output_sframe_func_desc_attr (sframe_fde);

      for (sframe_fre = sframe_fde->sframe_fres;
	   sframe_fre;
	   sframe_fre = sframe_fre->next)
	{
	  output_sframe_row_entry (sframe_fde, sframe_fre);
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
  xlate_ctx->flex_p = false;
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
  xlate_ctx->remember_fre = NULL;
  XDELETE (xlate_ctx->cur_fre);
  xlate_ctx->cur_fre = NULL;
}

/* Transfer the state from the SFrame translation context to the SFrame FDE.  */

static void
sframe_xlate_ctx_finalize (struct sframe_xlate_ctx *xlate_ctx,
			   struct sframe_func_entry *sframe_fde)
{
  sframe_fde->dw_fde = xlate_ctx->dw_fde;
  sframe_fde->fde_flex_p = xlate_ctx->flex_p;
  sframe_fde->sframe_fres = xlate_ctx->first_fre;
  sframe_fde->num_fres = xlate_ctx->num_xlate_fres;
  /* remember_fre is cloned copy of the applicable fre (where necessary).
     Since this is not included in the list of sframe_fres, free it.  */
  XDELETE (xlate_ctx->remember_fre);
  xlate_ctx->remember_fre = NULL;
}

/* Get the current CFA base register from the scratchpad FRE (cur_fre).
   NB: this may return a value of SFRAME_FRE_REG_INVALID.  */

static unsigned int
sframe_xlate_ctx_get_cur_cfa_reg (const struct sframe_xlate_ctx *xlate_ctx)
{
  gas_assert (xlate_ctx && xlate_ctx->cur_fre);

  return xlate_ctx->cur_fre->cfa_base_reg;
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
			     const struct sframe_row_entry *prev_fre)
{
  gas_assert (prev_fre);
  cur_fre->cfa_base_reg = prev_fre->cfa_base_reg;
  cur_fre->cfa_offset = prev_fre->cfa_offset;
  cur_fre->cfa_deref_p = prev_fre->cfa_deref_p;
  cur_fre->fp_loc = prev_fre->fp_loc;
  cur_fre->fp_reg = prev_fre->fp_reg;
  cur_fre->fp_offset = prev_fre->fp_offset;
  cur_fre->fp_deref_p = prev_fre->fp_deref_p;
  cur_fre->ra_loc = prev_fre->ra_loc;
  cur_fre->ra_reg = prev_fre->ra_reg;
  cur_fre->ra_offset = prev_fre->ra_offset;
  cur_fre->ra_deref_p = prev_fre->ra_deref_p;
  /* Treat RA mangling as a sticky bit.  It retains its value until another
     .cfi_negate_ra_state is seen.  */
  cur_fre->mangled_ra_p = prev_fre->mangled_ra_p;
  /* Treat RA undefined as a sticky bit.  It retains its value until a
     .cfi_offset RA, .cfi_register RA, .cfi_restore RA, or .cfi_same_value RA
     is seen.  */
  cur_fre->ra_undefined_p = prev_fre->ra_undefined_p;
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
			     const struct cfi_insn_data *cfi_insn)
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
			 const struct cfi_insn_data *cfi_insn)

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

  offsetT offset = cfi_insn->u.ri.offset;
  bool bound_p = sframe_fre_stack_offset_bound_p (offset, true);
  if (!bound_p)
    {
      as_warn (_("no SFrame FDE emitted; "
		 ".cfi_def_cfa with unsupported offset value"));
      return SFRAME_XLATE_ERR_NOTREPRESENTED;
    }

  /* Define the current CFA rule to use the provided register and
     offset.  Typically, the CFA rule uses SP/FP based CFA.  However, with
     SFrame V3 specification, if the CFA register is not FP/SP, SFrame FDE type
     SFRAME_FDE_TYPE_FLEX type may be used.

     GAS uses the hook sframe_support_flex_fde_p () to determine if SFrame FDE
     of type SFRAME_FDE_TYPE_FLEX can be emitted for the specific target.
     Non-SP/FP based CFA may be seen for:
       - AMD64 (e.g., DRAP, stack alignment), or
       - s390x, where this may be seen for (GCC) generated code for static stack
         clash protection.  */
  if (cfi_insn->u.ri.reg != SFRAME_CFA_SP_REG
      && cfi_insn->u.ri.reg != SFRAME_CFA_FP_REG)
    {
      if (!sframe_support_flex_fde_p ())
	{
	  as_warn (_("no SFrame FDE emitted; "
		     "non-SP/FP register %u in .cfi_def_cfa"),
		   cfi_insn->u.ri.reg);
	  return SFRAME_XLATE_ERR_NOTREPRESENTED;
	}
      else
	xlate_ctx->flex_p = true;
    }

  sframe_fre_set_cfa_base_reg (cur_fre, cfi_insn->u.ri.reg);
  sframe_fre_set_cfa_offset (cur_fre, cfi_insn->u.ri.offset);
  cur_fre->merge_candidate = false;
  cur_fre->cfa_deref_p = false;

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_def_cfa_register into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_def_cfa_register (struct sframe_xlate_ctx *xlate_ctx,
				  const struct cfi_insn_data *cfi_insn)
{
  const struct sframe_row_entry *last_fre = xlate_ctx->last_fre;
  /* Get the scratchpad FRE.  This FRE will eventually get linked in.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  gas_assert (cur_fre);
  /* Define the current CFA rule to use the provided register (but to
     keep the old offset).  However, if the register is not FP/SP,
     skip creating SFrame stack trace info for the function.  */
  if (cfi_insn->u.r != SFRAME_CFA_SP_REG
      && cfi_insn->u.r != SFRAME_CFA_FP_REG)
    {
      if (!sframe_support_flex_fde_p ())
	{
	  as_warn (_("no SFrame FDE emitted; "
		     "non-SP/FP register %u in .cfi_def_cfa_register"),
		   cfi_insn->u.ri.reg);
	  return SFRAME_XLATE_ERR_NOTREPRESENTED;
	}
      else
	/* Currently, SFRAME_FDE_TYPE_FLEX is generated for AMD64 only.  */
	xlate_ctx->flex_p = true;
    }

  sframe_fre_set_cfa_base_reg (cur_fre, cfi_insn->u.r);
  if (last_fre)
    sframe_fre_set_cfa_offset (cur_fre, sframe_fre_get_cfa_offset (last_fre));
  cur_fre->cfa_deref_p = false;

  cur_fre->merge_candidate = false;

  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_def_cfa_offset into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_def_cfa_offset (struct sframe_xlate_ctx *xlate_ctx,
				const struct cfi_insn_data *cfi_insn)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  unsigned int cur_cfa_reg = sframe_xlate_ctx_get_cur_cfa_reg (xlate_ctx);

  gas_assert (cur_fre);
  /*  Define the current CFA rule to use the provided offset (but to keep
      the old register).  However, if the old register is not FP/SP,
      skip creating SFrame stack trace info for the function.  */
  if (cur_cfa_reg == SFRAME_CFA_FP_REG || cur_cfa_reg == SFRAME_CFA_SP_REG)
    {
      if (sframe_fre_stack_offset_bound_p (cfi_insn->u.i, true))
	{
	  sframe_fre_set_cfa_offset (cur_fre, cfi_insn->u.i);
	  cur_fre->merge_candidate = false;
	}
      else
	{
	  as_warn (_("no SFrame FDE emitted; "
		     ".cfi_def_cfa_offset with unsupported offset value"));
	  return SFRAME_XLATE_ERR_NOTREPRESENTED;
	}
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
			const struct cfi_insn_data *cfi_insn)
{
  /* The scratchpad FRE currently being updated with each cfi_insn
     being interpreted.  This FRE eventually gets linked in into the
     list of FREs for the specific function.  */
  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  gas_assert (cur_fre);

  /* For ABIs not tracking RA, the return address is expected to be in a
     specific location.  Explicit manourvering to a different offset (than the
     default offset) is non-representable in SFrame, unless flex FDE generation
     is supported for the ABI.  */
  if (!sframe_support_flex_fde_p () && !sframe_ra_tracking_p ()
      && cfi_insn->u.ri.reg == SFRAME_CFA_RA_REG
      && cfi_insn->u.ri.offset != sframe_cfa_ra_offset ())
    {
      as_warn (_("no SFrame FDE emitted; %s register %u in .cfi_offset"),
	       sframe_register_name (cfi_insn->u.ri.reg), cfi_insn->u.ri.reg);
      return SFRAME_XLATE_ERR_NOTREPRESENTED;  /* Not represented.  */
    }

  /* Change the rule for the register indicated by the register number to
     be the specified offset.  */
  /* Ignore SP reg, as it can be recovered from the CFA tracking info.  */
  if (cfi_insn->u.ri.reg == SFRAME_CFA_FP_REG)
    {
      sframe_fre_set_fp_track (cur_fre, cfi_insn->u.ri.offset);
      cur_fre->fp_reg = SFRAME_FRE_REG_INVALID;
      cur_fre->fp_deref_p = true;
      cur_fre->merge_candidate = false;
    }
  /* Either the ABI has enabled RA tracking, in which case we must process the
     DW_CFA_offset opcode for REG_RA like usual.  Or if the ABI has not enabled
     RA tracking, but flex FDE generation is supported, distinguish between
     whether its time to reset the RA tracking state or not.  */
  else if (cfi_insn->u.ri.reg == SFRAME_CFA_RA_REG)
    {
      if (!sframe_ra_tracking_p ()
	  && cfi_insn->u.ri.offset == sframe_cfa_ra_offset ())
	{
	  /* Reset RA tracking info, if fixed offset.  */
	  cur_fre->ra_reg = SFRAME_FRE_REG_INVALID;
	  cur_fre->ra_loc = SFRAME_FRE_ELEM_LOC_NONE;
	  cur_fre->ra_deref_p = false;
	  cur_fre->merge_candidate = false;
	}
      else
	{
	  sframe_fre_set_ra_track (cur_fre, cfi_insn->u.ri.offset);
	  cur_fre->ra_reg = SFRAME_FRE_REG_INVALID;
	  cur_fre->ra_loc = SFRAME_FRE_ELEM_LOC_STACK;
	  cur_fre->ra_deref_p = true;
	  cur_fre->merge_candidate = false;

	  if (!sframe_ra_tracking_p () && sframe_support_flex_fde_p ())
	    xlate_ctx->flex_p = true;
	}
    }

  /* Skip all other registers.  */
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

/* Translate DW_CFA_register into SFrame context.

   This opcode indicates: Previous value of register1 is register2.  This is
   not representable using FDE type SFRAME_FDE_TYPE_DEFAULT.  Hence, if
   flexible FDE is not enabled for the ABI/arch, detect the use of registers
   interesting to SFrame (FP, RA for this opcode), and skip FDE generation
   while warning the user.  Same applies for SP, except that it needs special
   handling for s390.

   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_register (struct sframe_xlate_ctx *xlate_ctx,
			  const struct cfi_insn_data *cfi_insn)
{
  if (sframe_support_flex_fde_p ())
    {
      struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

      if (cfi_insn->u.rr.reg1 == SFRAME_CFA_FP_REG)
	{
	  sframe_fre_set_fp_track (cur_fre, 0);
	  cur_fre->fp_loc = SFRAME_FRE_ELEM_LOC_REG;
	  cur_fre->fp_reg = cfi_insn->u.rr.reg2;
	  cur_fre->fp_deref_p = false;
	  cur_fre->merge_candidate = false;
	  xlate_ctx->flex_p = true;
	  return SFRAME_XLATE_OK;
	}
      else if (cfi_insn->u.rr.reg1 == SFRAME_CFA_RA_REG)
	{
	  sframe_fre_set_ra_track (cur_fre, 0);
	  cur_fre->ra_loc = SFRAME_FRE_ELEM_LOC_REG;
	  cur_fre->ra_reg = cfi_insn->u.rr.reg2;
	  cur_fre->ra_deref_p = false;
	  cur_fre->merge_candidate = false;
	  xlate_ctx->flex_p = true;
	  return SFRAME_XLATE_OK;
	}
      /* Recovering REG_SP from an alternate register is not represented in
	 SFrame.  Fallthrough if SFRAME_CFA_SP_REG and error out.  */
    }

  if (cfi_insn->u.rr.reg1 == SFRAME_CFA_RA_REG
      /* SFrame does not track SP explicitly.  */
      || (cfi_insn->u.rr.reg1 == SFRAME_CFA_SP_REG
	  && sframe_get_abi_arch () != SFRAME_ABI_S390X_ENDIAN_BIG)
      || cfi_insn->u.rr.reg1 == SFRAME_CFA_FP_REG)
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
  const struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

  /* If there is no FRE state to remember, nothing to do here.  Return
     early with non-zero error code, this will cause no SFrame stack trace
     info for the function involved.  */
  if (!cur_fre)
    {
      as_warn (_("no SFrame FDE emitted; "
		 ".cfi_remember_state without prior SFrame FRE state"));
      return SFRAME_XLATE_ERR_INVAL;
    }

  if (!xlate_ctx->remember_fre)
    xlate_ctx->remember_fre = sframe_row_entry_new ();
  sframe_row_entry_initialize (xlate_ctx->remember_fre, cur_fre);

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
			 const struct cfi_insn_data *cfi_insn)
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
      cur_fre->fp_loc = cie_fre->fp_loc;
      cur_fre->fp_offset = cie_fre->fp_offset;
      cur_fre->merge_candidate = false;
    }
  else if (sframe_ra_tracking_p ()
	   && cfi_insn->u.r == SFRAME_CFA_RA_REG)
    {
      gas_assert (cur_fre);
      cur_fre->ra_loc = cie_fre->ra_loc;
      cur_fre->ra_offset = cie_fre->ra_offset;
      cur_fre->ra_undefined_p = cie_fre->ra_undefined_p;
      cur_fre->merge_candidate = false;
    }
  return SFRAME_XLATE_OK;
}

/* Translate DW_CFA_AARCH64_negate_ra_state into SFrame context.
   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_aarch64_negate_ra_state (struct sframe_xlate_ctx *xlate_ctx,
					 const struct cfi_insn_data *cfi_insn ATTRIBUTE_UNUSED)
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
						 const struct cfi_insn_data *cfi_insn ATTRIBUTE_UNUSED)
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
				 const struct cfi_insn_data *cfi_insn)
{
  unsigned char abi_arch = sframe_get_abi_arch ();

  /* Translate DW_CFA_AARCH64_negate_ra_state into SFrame context.  */
  if (abi_arch == SFRAME_ABI_AARCH64_ENDIAN_BIG
      || abi_arch == SFRAME_ABI_AARCH64_ENDIAN_LITTLE)
    return sframe_xlate_do_aarch64_negate_ra_state (xlate_ctx, cfi_insn);

  as_warn (_("no SFrame FDE emitted; .cfi_window_save"));
  return SFRAME_XLATE_ERR_NOTREPRESENTED;  /* Not represented.  */
}

/* Translate a DWARF sleb128 offset in the CFI escape data E to an offsetT.
   Update the value in OFFSET if success (and return SFRAME_XLATE_OK).
   Return SFRAME_XLATE_ERR_INVAL if error.  */

static int
sframe_xlate_escape_sleb128_to_offsetT (const struct cfi_escape_data *e,
					offsetT *offset)
{
  gas_assert (e->type == CFI_ESC_byte || e->type == CFI_ESC_sleb128);
  /* Read the offset.  */
  if (e->type == CFI_ESC_byte)
    {
      /* The user/compiler may provide an sleb128 encoded data of a single byte
	 length (DWARF offset of DW_OP_bregN is sleb128).  On a big-endian
	 host, the endianness of data itself needs to be accommodated then.  To
	 keep it simple, gather the LSB, and translate it to int64.  */
      unsigned char sleb_data = e->exp.X_add_number & 0xff;
      const unsigned char *buf_start = (const unsigned char *)&sleb_data;
      const unsigned char *buf_end = buf_start + 1;
      int64_t value = 0;
      size_t read = read_sleb128_to_int64 (buf_start, buf_end, &value);
      /* In case of bogus input (highest bit erroneously set, e.g., 0x80),
	 gracefully exit.  */
      if (!read)
	return SFRAME_XLATE_ERR_INVAL;
      *offset = (offsetT) value;
    }
  else
    /* offset must be CFI_ESC_sleb128.  */
    *offset = e->exp.X_add_number;

  return SFRAME_XLATE_OK;
}

/* Handle DW_CFA_def_cfa_expression in .cfi_escape.

   As with sframe_xlate_do_cfi_escape, the intent of this function is to detect
   only the simple-to-process but common cases.  All other CFA escape
   expressions continue to be inadmissible (no SFrame FDE emitted).

   Sets CALLER_WARN_P for skipped cases (and returns SFRAME_XLATE_OK) where the
   caller must warn.  The caller then must also set
   SFRAME_XLATE_ERR_NOTREPRESENTED for their callers.  */

static int
sframe_xlate_do_escape_cfa_expr (struct sframe_xlate_ctx *xlate_ctx,
				 const struct cfi_insn_data *cfi_insn,
				 bool *caller_warn_p)
{
  const struct cfi_escape_data *e = cfi_insn->u.esc;
  const struct cfi_escape_data *e_offset = NULL;
  int err = SFRAME_XLATE_OK;
  unsigned int opcode1, opcode2;
  offsetT offset = 0;
  unsigned int reg = SFRAME_FRE_REG_INVALID;
  unsigned int i = 0;
  bool x86_cfa_deref_p = false;

  /* Check roughly for an expression like so:
     DW_CFA_def_cfa_expression (DW_OP_breg6 (rbp): -8; DW_OP_deref).  */
#define CFI_ESC_NUM_EXP 4
  offsetT items[CFI_ESC_NUM_EXP] = {0};
  while (e->next)
    {
      e = e->next;
      /* Bounds check, must be constant, no relocs.  */
      if (i >= CFI_ESC_NUM_EXP
	  || e->exp.X_op != O_constant
	  || e->reloc != TC_PARSE_CONS_RETURN_NONE)
	goto warn_and_exit;
      /* Other checks based on index i.
	   - For item[2], allow byte OR sleb128.
	   - items at index 0, 1, and 3: Must be byte.  */
      if (i == 2 && (e->type != CFI_ESC_byte && e->type != CFI_ESC_sleb128))
	goto warn_and_exit;
      else if (i != 2 && e->type != CFI_ESC_byte)
	goto warn_and_exit;
      /* Block length (items[0]) of 3 in DWARF expr.  */
      if (i == 1 && items[0] != 3)
	goto warn_and_exit;

      if (i == 2)
	e_offset = e;

      items[i] = e->exp.X_add_number;
      i++;
    }

  if (i != CFI_ESC_NUM_EXP)
    goto warn_and_exit;
#undef CFI_ESC_NUM_EXP

  err = sframe_xlate_escape_sleb128_to_offsetT (e_offset, &offset);
  if (err == SFRAME_XLATE_ERR_INVAL)
    goto warn_and_exit;

  opcode1 = items[1];
  opcode2 = items[3];
  /* DW_OP_breg6 is rbp.  FIXME - this stub can be enhanced to handle more
     regs.  */
  if (sframe_get_abi_arch () == SFRAME_ABI_AMD64_ENDIAN_LITTLE
      && sframe_support_flex_fde_p ()
      && opcode1 == DW_OP_breg6 && opcode2 == DW_OP_deref)
    {
      x86_cfa_deref_p = true;
      reg = SFRAME_CFA_FP_REG;
    }

  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  gas_assert (cur_fre);

  /* Handle the specific CFA expression mentioned above.  */
  if (x86_cfa_deref_p
      && sframe_fre_stack_offset_bound_p (offset, false)
      && reg != SFRAME_FRE_REG_INVALID)
    {
      xlate_ctx->flex_p = true;
      sframe_fre_set_cfa_base_reg (cur_fre, reg);
      sframe_fre_set_cfa_offset (cur_fre, offset);
      cur_fre->cfa_deref_p = true;
      cur_fre->merge_candidate = false;
      /* Done handling here.  */
      caller_warn_p = false;

      return err;
    }
  /* Any other CFA expression may not be safe to skip.  Fall through to
     warn_and_exit.  */

warn_and_exit:
  *caller_warn_p = true;
  return err;
}

/* Handle DW_CFA_expression in .cfi_escape.

   As with sframe_xlate_do_cfi_escape, the intent of this function is to detect
   only the simple-to-process but common cases, where skipping over the escape
   expr data does not affect correctness of the SFrame stack trace data.

   Sets CALLER_WARN_P for skipped cases (and returns SFRAME_XLATE_OK) where the
   caller must warn.  The caller then must also set
   SFRAME_XLATE_ERR_NOTREPRESENTED for their callers.  */

static int
sframe_xlate_do_escape_expr (struct sframe_xlate_ctx *xlate_ctx,
			     const struct cfi_insn_data *cfi_insn,
			     bool *caller_warn_p)
{
  const struct cfi_escape_data *e = cfi_insn->u.esc;
  const struct cfi_escape_data *e_offset = NULL;
  int err = SFRAME_XLATE_OK;
  offsetT offset = 0;
  unsigned int i = 0;

  /* Check roughly for an expression
     DW_CFA_expression: r1 (rdx) (DW_OP_bregN (reg): OFFSET).  */
#define CFI_ESC_NUM_EXP 4
  offsetT items[CFI_ESC_NUM_EXP] = {0};
  while (e->next)
    {
      e = e->next;
      /* Bounds check, must be constant, no relocs.  */
      if (i >= CFI_ESC_NUM_EXP
	  || e->exp.X_op != O_constant
	  || e->reloc != TC_PARSE_CONS_RETURN_NONE)
	goto warn_and_exit;
      /* Other checks based on index i.
	   - For item[3], allow byte OR sleb128.
	   - items at index 0, 1, and 2: Must be byte.  */
      if (i == 3 && (e->type != CFI_ESC_byte && e->type != CFI_ESC_sleb128))
	goto warn_and_exit;
      else if (i != 3 && e->type != CFI_ESC_byte)
	goto warn_and_exit;
      /* Block length (items[1]) of 2 in DWARF expr.  */
      if (i == 2 && items[1] != 2)
	goto warn_and_exit;

      if (i == 3)
	e_offset = e;

      items[i] = e->exp.X_add_number;
      i++;
    }

  if (i <= CFI_ESC_NUM_EXP - 1)
    goto warn_and_exit;
#undef CFI_ESC_NUM_EXP

  err = sframe_xlate_escape_sleb128_to_offsetT (e_offset, &offset);
  if (err == SFRAME_XLATE_ERR_INVAL)
    goto warn_and_exit;

  /* reg operand to DW_CFA_expression is ULEB128.  For the purpose at hand,
     however, the register value will be less than 128 (CFI_ESC_NUM_EXP set
     to 4).  See an extended comment in sframe_xlate_do_escape_expr for why
     reading ULEB is okay to skip without sacrificing correctness.  */
  unsigned int reg = items[0];

  unsigned opcode = items[2];
  unsigned int fp_base_reg = SFRAME_FRE_REG_INVALID;
  bool x86_fp_deref_p = true;

  if (sframe_get_abi_arch () == SFRAME_ABI_AMD64_ENDIAN_LITTLE
      && sframe_support_flex_fde_p ()
      && opcode == DW_OP_breg6)
    {
      x86_fp_deref_p = true;
      fp_base_reg = SFRAME_CFA_FP_REG;
    }

  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  gas_assert (cur_fre);

  if (x86_fp_deref_p
      && reg == SFRAME_CFA_FP_REG
      && sframe_fre_stack_offset_bound_p (offset, false))
    {
      xlate_ctx->flex_p = true;
      sframe_fre_set_fp_track (cur_fre, offset);
      cur_fre->fp_loc = SFRAME_FRE_ELEM_LOC_REG;
      cur_fre->fp_reg = fp_base_reg;
      cur_fre->fp_deref_p = true;
      cur_fre->merge_candidate = false;
    }
  else if (reg == SFRAME_CFA_SP_REG || reg == SFRAME_CFA_FP_REG
	   || (sframe_ra_tracking_p () && reg == SFRAME_CFA_RA_REG)
	   || reg == sframe_xlate_ctx_get_cur_cfa_reg (xlate_ctx))
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

/* Handle DW_CFA_GNU_args_size in .cfi_escape.

   The purpose of DW_CFA_GNU_args_size is to adjust SP when performing stack
   unwinding for exception handling.  For stack tracing needs,
   DW_CFA_GNU_args_size can be ignored, when CFA is FP-based.  This is because
   if the topmost frame is that of the catch block, the SP has been restored to
   correct value by exception handling logic.  From this point of interest in
   the catch block now, stack tracing intends to go backwards to the caller
   frame.  If CFA restoration does not need SP, DW_CFA_GNU_args_size can be
   ignored for stack tracing.

   Continue to warn and not emit SFrame FDE if CFA is SP based.  The pattern
   where the CFA is SP based and there is a DW_CFA_GNU_args_size for
   SP-adjustment is not entirely clear.

   Sets CALLER_WARN_P for skipped cases (and returns SFRAME_XLATE_OK) where the
   caller must warn.  The caller then must also set
   SFRAME_XLATE_ERR_NOTREPRESENTED for their callers.  */

static int
sframe_xlate_do_escape_gnu_args_size (const struct sframe_xlate_ctx *xlate_ctx,
				      const struct cfi_insn_data *cfi_insn,
				      bool *caller_warn_p)
{
  const struct cfi_escape_data *e = cfi_insn->u.esc;
  unsigned int i = 0;

  /* Check for (DW_CFA_GNU_args_size offset) sequence.  */
#define CFI_ESC_NUM_EXP 1
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
  if (i == 0)
    goto warn_and_exit;

#undef CFI_ESC_NUM_EXP

  offsetT offset = items[0];

  struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;
  gas_assert (cur_fre);
 /* If CFA is FP based, safe to skip.  */
  if (offset == 0
      || sframe_xlate_ctx_get_cur_cfa_reg (xlate_ctx) == SFRAME_CFA_FP_REG)
    return SFRAME_XLATE_OK;

warn_and_exit:
  *caller_warn_p = true;
  return SFRAME_XLATE_OK;
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
sframe_xlate_do_cfi_escape (struct sframe_xlate_ctx *xlate_ctx,
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

    case DW_CFA_def_cfa_expression:
      err = sframe_xlate_do_escape_cfa_expr (xlate_ctx, cfi_insn, &warn_p);
      break;

    case DW_CFA_expression:
      err = sframe_xlate_do_escape_expr (xlate_ctx, cfi_insn, &warn_p);
      break;

    case DW_CFA_val_offset:
      err = sframe_xlate_do_escape_val_offset (xlate_ctx, cfi_insn, &warn_p);
      break;

    case DW_CFA_GNU_args_size:
      err = sframe_xlate_do_escape_gnu_args_size (xlate_ctx, cfi_insn, &warn_p);
      break;

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
   register can’t be restored anymore.  In DWARF, for the return address (RA)
   register, this indicates to an unwinder that there is no return address
   and the unwind is complete.

   In SFrame, represent the use of the RA register with DW_CFA_undefined as
   SFrame FRE without any trailing FRE data words.  Stack tracers can use this
   as indication that an outermost frame has been reached and the stack trace
   is complete.  The use of other registers of interest with  DW_CFA_undefined
   cannot be represented in SFrame.  Therefore skip generating an SFrame FDE.

   Return SFRAME_XLATE_OK if success.  */

static int
sframe_xlate_do_cfi_undefined (const struct sframe_xlate_ctx *xlate_ctx ATTRIBUTE_UNUSED,
			       const struct cfi_insn_data *cfi_insn)
{
  if (cfi_insn->u.r == SFRAME_CFA_FP_REG
      || cfi_insn->u.r == SFRAME_CFA_SP_REG)
    {
      as_warn (_("no SFrame FDE emitted; %s reg %u in .cfi_undefined"),
	       sframe_register_name (cfi_insn->u.r), cfi_insn->u.r);
      return SFRAME_XLATE_ERR_NOTREPRESENTED; /* Not represented.  */
    }
  else if (cfi_insn->u.r == SFRAME_CFA_RA_REG)
    {
      /* Represent RA undefined (i.e. outermost frame) as FRE without any
	 data words.  */
      struct sframe_row_entry *cur_fre = xlate_ctx->cur_fre;

      gas_assert (cur_fre);
      /* Set RA undefined status bit.  */
      cur_fre->ra_undefined_p = true;
      cur_fre->merge_candidate = false;
    }

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
      cur_fre->ra_loc = SFRAME_FRE_ELEM_LOC_NONE;
      cur_fre->ra_offset = 0;
      cur_fre->ra_undefined_p = false;
      cur_fre->merge_candidate = false;
    }
  else if (cfi_insn->u.r == SFRAME_CFA_FP_REG)
    {
      cur_fre->fp_loc = SFRAME_FRE_ELEM_LOC_NONE;
      cur_fre->fp_offset = 0;
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
		    const struct cfi_insn_data *cfi_insn)
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
  const struct cfi_insn_data *cfi_insn;
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

  /* Number of FREs must fit uint16_t.  Check now, and do not emit the SFrame
     FDE if it doesnt fit (although, it is not expected to happen for
     real-world, useful programs).  The approach of truncating the FDE and
     emitting multiple SFrame FDEs instead, is not a clearly preferable
     handling either.  Its a divergence from the model where an SFrame FDE
     encodes stack trace data between a .cfi_startproc and .cfi_endproc pair.
     Further, some components (linkers, stack tracers) want to associate the
     Start PC of a function to a known symbol in the file?  */
  if (xlate_ctx->num_xlate_fres > UINT16_MAX)
    {
      as_warn (_("no SFrame FDE emitted; Number of FREs exceeds UINT16_MAX"));
      return SFRAME_XLATE_ERR_NOTREPRESENTED;
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
	      && fre->fp_loc == SFRAME_FRE_ELEM_LOC_STACK)
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

      /* Process and link SFrame FDEs if no error.  */
      int err = sframe_do_fde (xlate_ctx, dw_fde);
      if (err && get_dw_fde_signal_p (dw_fde))
	{
	  sframe_xlate_ctx_cleanup (xlate_ctx);
	  xlate_ctx->flex_p = false;
	  err = SFRAME_XLATE_OK;
	}

      if (err)
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

  /* Currently only SFRAME_VERSION_3 can be emitted.  */
  gas_assert (flag_gen_sframe_version == GEN_SFRAME_VERSION_3);
  /* Setup the version specific access functions.  */
  sframe_set_version (flag_gen_sframe_version);

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
