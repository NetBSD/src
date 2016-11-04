/* Frame unwinder for frames with DWARF Call Frame Information.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

   Contributed by Mark Kettenis.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "dwarf2expr.h"
#include "dwarf2.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"
#include "record.h"

#include "complaints.h"
#include "dwarf2-frame.h"
#include "ax.h"
#include "dwarf2loc.h"
#include "dwarf2-frame-tailcall.h"

struct comp_unit;

/* Call Frame Information (CFI).  */

/* Common Information Entry (CIE).  */

struct dwarf2_cie
{
  /* Computation Unit for this CIE.  */
  struct comp_unit *unit;

  /* Offset into the .debug_frame section where this CIE was found.
     Used to identify this CIE.  */
  ULONGEST cie_pointer;

  /* Constant that is factored out of all advance location
     instructions.  */
  ULONGEST code_alignment_factor;

  /* Constants that is factored out of all offset instructions.  */
  LONGEST data_alignment_factor;

  /* Return address column.  */
  ULONGEST return_address_register;

  /* Instruction sequence to initialize a register set.  */
  const gdb_byte *initial_instructions;
  const gdb_byte *end;

  /* Saved augmentation, in case it's needed later.  */
  char *augmentation;

  /* Encoding of addresses.  */
  gdb_byte encoding;

  /* Target address size in bytes.  */
  int addr_size;

  /* Target pointer size in bytes.  */
  int ptr_size;

  /* True if a 'z' augmentation existed.  */
  unsigned char saw_z_augmentation;

  /* True if an 'S' augmentation existed.  */
  unsigned char signal_frame;

  /* The version recorded in the CIE.  */
  unsigned char version;

  /* The segment size.  */
  unsigned char segment_size;
};

struct dwarf2_cie_table
{
  int num_entries;
  struct dwarf2_cie **entries;
};

/* Frame Description Entry (FDE).  */

struct dwarf2_fde
{
  /* CIE for this FDE.  */
  struct dwarf2_cie *cie;

  /* First location associated with this FDE.  */
  CORE_ADDR initial_location;

  /* Number of bytes of program instructions described by this FDE.  */
  CORE_ADDR address_range;

  /* Instruction sequence.  */
  const gdb_byte *instructions;
  const gdb_byte *end;

  /* True if this FDE is read from a .eh_frame instead of a .debug_frame
     section.  */
  unsigned char eh_frame_p;
};

struct dwarf2_fde_table
{
  int num_entries;
  struct dwarf2_fde **entries;
};

/* A minimal decoding of DWARF2 compilation units.  We only decode
   what's needed to get to the call frame information.  */

struct comp_unit
{
  /* Keep the bfd convenient.  */
  bfd *abfd;

  struct objfile *objfile;

  /* Pointer to the .debug_frame section loaded into memory.  */
  const gdb_byte *dwarf_frame_buffer;

  /* Length of the loaded .debug_frame section.  */
  bfd_size_type dwarf_frame_size;

  /* Pointer to the .debug_frame section.  */
  asection *dwarf_frame_section;

  /* Base for DW_EH_PE_datarel encodings.  */
  bfd_vma dbase;

  /* Base for DW_EH_PE_textrel encodings.  */
  bfd_vma tbase;
};

static struct dwarf2_fde *dwarf2_frame_find_fde (CORE_ADDR *pc,
						 CORE_ADDR *out_offset);

static int dwarf2_frame_adjust_regnum (struct gdbarch *gdbarch, int regnum,
				       int eh_frame_p);

static CORE_ADDR read_encoded_value (struct comp_unit *unit, gdb_byte encoding,
				     int ptr_len, const gdb_byte *buf,
				     unsigned int *bytes_read_ptr,
				     CORE_ADDR func_base);


enum cfa_how_kind
{
  CFA_UNSET,
  CFA_REG_OFFSET,
  CFA_EXP
};

struct dwarf2_frame_state_reg_info
{
  struct dwarf2_frame_state_reg *reg;
  int num_regs;

  LONGEST cfa_offset;
  ULONGEST cfa_reg;
  enum cfa_how_kind cfa_how;
  const gdb_byte *cfa_exp;

  /* Used to implement DW_CFA_remember_state.  */
  struct dwarf2_frame_state_reg_info *prev;
};

/* Structure describing a frame state.  */

struct dwarf2_frame_state
{
  /* Each register save state can be described in terms of a CFA slot,
     another register, or a location expression.  */
  struct dwarf2_frame_state_reg_info regs;

  /* The PC described by the current frame state.  */
  CORE_ADDR pc;

  /* Initial register set from the CIE.
     Used to implement DW_CFA_restore.  */
  struct dwarf2_frame_state_reg_info initial;

  /* The information we care about from the CIE.  */
  LONGEST data_align;
  ULONGEST code_align;
  ULONGEST retaddr_column;

  /* Flags for known producer quirks.  */

  /* The ARM compilers, in DWARF2 mode, assume that DW_CFA_def_cfa
     and DW_CFA_def_cfa_offset takes a factored offset.  */
  int armcc_cfa_offsets_sf;

  /* The ARM compilers, in DWARF2 or DWARF3 mode, may assume that
     the CFA is defined as REG - OFFSET rather than REG + OFFSET.  */
  int armcc_cfa_offsets_reversed;
};

/* Store the length the expression for the CFA in the `cfa_reg' field,
   which is unused in that case.  */
#define cfa_exp_len cfa_reg

/* Assert that the register set RS is large enough to store gdbarch_num_regs
   columns.  If necessary, enlarge the register set.  */

static void
dwarf2_frame_state_alloc_regs (struct dwarf2_frame_state_reg_info *rs,
			       int num_regs)
{
  size_t size = sizeof (struct dwarf2_frame_state_reg);

  if (num_regs <= rs->num_regs)
    return;

  rs->reg = (struct dwarf2_frame_state_reg *)
    xrealloc (rs->reg, num_regs * size);

  /* Initialize newly allocated registers.  */
  memset (rs->reg + rs->num_regs, 0, (num_regs - rs->num_regs) * size);
  rs->num_regs = num_regs;
}

/* Copy the register columns in register set RS into newly allocated
   memory and return a pointer to this newly created copy.  */

static struct dwarf2_frame_state_reg *
dwarf2_frame_state_copy_regs (struct dwarf2_frame_state_reg_info *rs)
{
  size_t size = rs->num_regs * sizeof (struct dwarf2_frame_state_reg);
  struct dwarf2_frame_state_reg *reg;

  reg = (struct dwarf2_frame_state_reg *) xmalloc (size);
  memcpy (reg, rs->reg, size);

  return reg;
}

/* Release the memory allocated to register set RS.  */

static void
dwarf2_frame_state_free_regs (struct dwarf2_frame_state_reg_info *rs)
{
  if (rs)
    {
      dwarf2_frame_state_free_regs (rs->prev);

      xfree (rs->reg);
      xfree (rs);
    }
}

/* Release the memory allocated to the frame state FS.  */

static void
dwarf2_frame_state_free (void *p)
{
  struct dwarf2_frame_state *fs = (struct dwarf2_frame_state *) p;

  dwarf2_frame_state_free_regs (fs->initial.prev);
  dwarf2_frame_state_free_regs (fs->regs.prev);
  xfree (fs->initial.reg);
  xfree (fs->regs.reg);
  xfree (fs);
}


/* Helper functions for execute_stack_op.  */

static CORE_ADDR
read_addr_from_reg (void *baton, int reg)
{
  struct frame_info *this_frame = (struct frame_info *) baton;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int regnum = dwarf_reg_to_regnum_or_error (gdbarch, reg);

  return address_from_register (regnum, this_frame);
}

/* Implement struct dwarf_expr_context_funcs' "get_reg_value" callback.  */

static struct value *
get_reg_value (void *baton, struct type *type, int reg)
{
  struct frame_info *this_frame = (struct frame_info *) baton;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int regnum = dwarf_reg_to_regnum_or_error (gdbarch, reg);

  return value_from_register (type, regnum, this_frame);
}

static void
read_mem (void *baton, gdb_byte *buf, CORE_ADDR addr, size_t len)
{
  read_memory (addr, buf, len);
}

/* Execute the required actions for both the DW_CFA_restore and
DW_CFA_restore_extended instructions.  */
static void
dwarf2_restore_rule (struct gdbarch *gdbarch, ULONGEST reg_num,
		     struct dwarf2_frame_state *fs, int eh_frame_p)
{
  ULONGEST reg;

  gdb_assert (fs->initial.reg);
  reg = dwarf2_frame_adjust_regnum (gdbarch, reg_num, eh_frame_p);
  dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);

  /* Check if this register was explicitly initialized in the
  CIE initial instructions.  If not, default the rule to
  UNSPECIFIED.  */
  if (reg < fs->initial.num_regs)
    fs->regs.reg[reg] = fs->initial.reg[reg];
  else
    fs->regs.reg[reg].how = DWARF2_FRAME_REG_UNSPECIFIED;

  if (fs->regs.reg[reg].how == DWARF2_FRAME_REG_UNSPECIFIED)
    {
      int regnum = dwarf_reg_to_regnum (gdbarch, reg);

      complaint (&symfile_complaints, _("\
incomplete CFI data; DW_CFA_restore unspecified\n\
register %s (#%d) at %s"),
		 gdbarch_register_name (gdbarch, regnum), regnum,
		 paddress (gdbarch, fs->pc));
    }
}

/* Virtual method table for execute_stack_op below.  */

static const struct dwarf_expr_context_funcs dwarf2_frame_ctx_funcs =
{
  read_addr_from_reg,
  get_reg_value,
  read_mem,
  ctx_no_get_frame_base,
  ctx_no_get_frame_cfa,
  ctx_no_get_frame_pc,
  ctx_no_get_tls_address,
  ctx_no_dwarf_call,
  ctx_no_get_base_type,
  ctx_no_push_dwarf_reg_entry_value,
  ctx_no_get_addr_index
};

static CORE_ADDR
execute_stack_op (const gdb_byte *exp, ULONGEST len, int addr_size,
		  CORE_ADDR offset, struct frame_info *this_frame,
		  CORE_ADDR initial, int initial_in_stack_memory)
{
  struct dwarf_expr_context *ctx;
  CORE_ADDR result;
  struct cleanup *old_chain;

  ctx = new_dwarf_expr_context ();
  old_chain = make_cleanup_free_dwarf_expr_context (ctx);
  make_cleanup_value_free_to_mark (value_mark ());

  ctx->gdbarch = get_frame_arch (this_frame);
  ctx->addr_size = addr_size;
  ctx->ref_addr_size = -1;
  ctx->offset = offset;
  ctx->baton = this_frame;
  ctx->funcs = &dwarf2_frame_ctx_funcs;

  dwarf_expr_push_address (ctx, initial, initial_in_stack_memory);
  dwarf_expr_eval (ctx, exp, len);

  if (ctx->location == DWARF_VALUE_MEMORY)
    result = dwarf_expr_fetch_address (ctx, 0);
  else if (ctx->location == DWARF_VALUE_REGISTER)
    result = read_addr_from_reg (this_frame,
				 value_as_long (dwarf_expr_fetch (ctx, 0)));
  else
    {
      /* This is actually invalid DWARF, but if we ever do run across
	 it somehow, we might as well support it.  So, instead, report
	 it as unimplemented.  */
      error (_("\
Not implemented: computing unwound register using explicit value operator"));
    }

  do_cleanups (old_chain);

  return result;
}


/* Execute FDE program from INSN_PTR possibly up to INSN_END or up to inferior
   PC.  Modify FS state accordingly.  Return current INSN_PTR where the
   execution has stopped, one can resume it on the next call.  */

static const gdb_byte *
execute_cfa_program (struct dwarf2_fde *fde, const gdb_byte *insn_ptr,
		     const gdb_byte *insn_end, struct gdbarch *gdbarch,
		     CORE_ADDR pc, struct dwarf2_frame_state *fs)
{
  int eh_frame_p = fde->eh_frame_p;
  unsigned int bytes_read;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  while (insn_ptr < insn_end && fs->pc <= pc)
    {
      gdb_byte insn = *insn_ptr++;
      uint64_t utmp, reg;
      int64_t offset;

      if ((insn & 0xc0) == DW_CFA_advance_loc)
	fs->pc += (insn & 0x3f) * fs->code_align;
      else if ((insn & 0xc0) == DW_CFA_offset)
	{
	  reg = insn & 0x3f;
	  reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	  insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	  offset = utmp * fs->data_align;
	  dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	  fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	  fs->regs.reg[reg].loc.offset = offset;
	}
      else if ((insn & 0xc0) == DW_CFA_restore)
	{
	  reg = insn & 0x3f;
	  dwarf2_restore_rule (gdbarch, reg, fs, eh_frame_p);
	}
      else
	{
	  switch (insn)
	    {
	    case DW_CFA_set_loc:
	      fs->pc = read_encoded_value (fde->cie->unit, fde->cie->encoding,
					   fde->cie->ptr_size, insn_ptr,
					   &bytes_read, fde->initial_location);
	      /* Apply the objfile offset for relocatable objects.  */
	      fs->pc += ANOFFSET (fde->cie->unit->objfile->section_offsets,
				  SECT_OFF_TEXT (fde->cie->unit->objfile));
	      insn_ptr += bytes_read;
	      break;

	    case DW_CFA_advance_loc1:
	      utmp = extract_unsigned_integer (insn_ptr, 1, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr++;
	      break;
	    case DW_CFA_advance_loc2:
	      utmp = extract_unsigned_integer (insn_ptr, 2, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr += 2;
	      break;
	    case DW_CFA_advance_loc4:
	      utmp = extract_unsigned_integer (insn_ptr, 4, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr += 4;
	      break;

	    case DW_CFA_offset_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_restore_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      dwarf2_restore_rule (gdbarch, reg, fs, eh_frame_p);
	      break;

	    case DW_CFA_undefined:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_UNDEFINED;
	      break;

	    case DW_CFA_same_value:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAME_VALUE;
	      break;

	    case DW_CFA_register:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      utmp = dwarf2_frame_adjust_regnum (gdbarch, utmp, eh_frame_p);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_REG;
	      fs->regs.reg[reg].loc.reg = utmp;
	      break;

	    case DW_CFA_remember_state:
	      {
		struct dwarf2_frame_state_reg_info *new_rs;

		new_rs = XNEW (struct dwarf2_frame_state_reg_info);
		*new_rs = fs->regs;
		fs->regs.reg = dwarf2_frame_state_copy_regs (&fs->regs);
		fs->regs.prev = new_rs;
	      }
	      break;

	    case DW_CFA_restore_state:
	      {
		struct dwarf2_frame_state_reg_info *old_rs = fs->regs.prev;

		if (old_rs == NULL)
		  {
		    complaint (&symfile_complaints, _("\
bad CFI data; mismatched DW_CFA_restore_state at %s"),
			       paddress (gdbarch, fs->pc));
		  }
		else
		  {
		    xfree (fs->regs.reg);
		    fs->regs = *old_rs;
		    xfree (old_rs);
		  }
	      }
	      break;

	    case DW_CFA_def_cfa:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = reg;
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);

	      if (fs->armcc_cfa_offsets_sf)
		utmp *= fs->data_align;

	      fs->regs.cfa_offset = utmp;
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_register:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = dwarf2_frame_adjust_regnum (gdbarch, reg,
                                                             eh_frame_p);
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_offset:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);

	      if (fs->armcc_cfa_offsets_sf)
		utmp *= fs->data_align;

	      fs->regs.cfa_offset = utmp;
	      /* cfa_how deliberately not set.  */
	      break;

	    case DW_CFA_nop:
	      break;

	    case DW_CFA_def_cfa_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.cfa_exp_len = utmp;
	      fs->regs.cfa_exp = insn_ptr;
	      fs->regs.cfa_how = CFA_EXP;
	      insn_ptr += fs->regs.cfa_exp_len;
	      break;

	    case DW_CFA_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.reg[reg].loc.exp = insn_ptr;
	      fs->regs.reg[reg].exp_len = utmp;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_EXP;
	      insn_ptr += utmp;
	      break;

	    case DW_CFA_offset_extended_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      offset *= fs->data_align;
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_offset:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_offset_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      offset *= fs->data_align;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.reg[reg].loc.exp = insn_ptr;
	      fs->regs.reg[reg].exp_len = utmp;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_EXP;
	      insn_ptr += utmp;
	      break;

	    case DW_CFA_def_cfa_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = dwarf2_frame_adjust_regnum (gdbarch, reg,
                                                             eh_frame_p);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      fs->regs.cfa_offset = offset * fs->data_align;
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_offset_sf:
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      fs->regs.cfa_offset = offset * fs->data_align;
	      /* cfa_how deliberately not set.  */
	      break;

	    case DW_CFA_GNU_window_save:
	      /* This is SPARC-specific code, and contains hard-coded
		 constants for the register numbering scheme used by
		 GCC.  Rather than having a architecture-specific
		 operation that's only ever used by a single
		 architecture, we provide the implementation here.
		 Incidentally that's what GCC does too in its
		 unwinder.  */
	      {
		int size = register_size (gdbarch, 0);

		dwarf2_frame_state_alloc_regs (&fs->regs, 32);
		for (reg = 8; reg < 16; reg++)
		  {
		    fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_REG;
		    fs->regs.reg[reg].loc.reg = reg + 16;
		  }
		for (reg = 16; reg < 32; reg++)
		  {
		    fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
		    fs->regs.reg[reg].loc.offset = (reg - 16) * size;
		  }
	      }
	      break;

	    case DW_CFA_GNU_args_size:
	      /* Ignored.  */
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      break;

	    case DW_CFA_GNU_negative_offset_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      dwarf2_frame_state_alloc_regs (&fs->regs, reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = -offset;
	      break;

	    default:
	      internal_error (__FILE__, __LINE__,
			      _("Unknown CFI encountered."));
	    }
	}
    }

  if (fs->initial.reg == NULL)
    {
      /* Don't allow remember/restore between CIE and FDE programs.  */
      dwarf2_frame_state_free_regs (fs->regs.prev);
      fs->regs.prev = NULL;
    }

  return insn_ptr;
}


/* Architecture-specific operations.  */

/* Per-architecture data key.  */
static struct gdbarch_data *dwarf2_frame_data;

struct dwarf2_frame_ops
{
  /* Pre-initialize the register state REG for register REGNUM.  */
  void (*init_reg) (struct gdbarch *, int, struct dwarf2_frame_state_reg *,
		    struct frame_info *);

  /* Check whether the THIS_FRAME is a signal trampoline.  */
  int (*signal_frame_p) (struct gdbarch *, struct frame_info *);

  /* Convert .eh_frame register number to DWARF register number, or
     adjust .debug_frame register number.  */
  int (*adjust_regnum) (struct gdbarch *, int, int);
};

/* Default architecture-specific register state initialization
   function.  */

static void
dwarf2_frame_default_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       struct frame_info *this_frame)
{
  /* If we have a register that acts as a program counter, mark it as
     a destination for the return address.  If we have a register that
     serves as the stack pointer, arrange for it to be filled with the
     call frame address (CFA).  The other registers are marked as
     unspecified.

     We copy the return address to the program counter, since many
     parts in GDB assume that it is possible to get the return address
     by unwinding the program counter register.  However, on ISA's
     with a dedicated return address register, the CFI usually only
     contains information to unwind that return address register.

     The reason we're treating the stack pointer special here is
     because in many cases GCC doesn't emit CFI for the stack pointer
     and implicitly assumes that it is equal to the CFA.  This makes
     some sense since the DWARF specification (version 3, draft 8,
     p. 102) says that:

     "Typically, the CFA is defined to be the value of the stack
     pointer at the call site in the previous frame (which may be
     different from its value on entry to the current frame)."

     However, this isn't true for all platforms supported by GCC
     (e.g. IBM S/390 and zSeries).  Those architectures should provide
     their own architecture-specific initialization function.  */

  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;
}

/* Return a default for the architecture-specific operations.  */

static void *
dwarf2_frame_init (struct obstack *obstack)
{
  struct dwarf2_frame_ops *ops;
  
  ops = OBSTACK_ZALLOC (obstack, struct dwarf2_frame_ops);
  ops->init_reg = dwarf2_frame_default_init_reg;
  return ops;
}

/* Set the architecture-specific register state initialization
   function for GDBARCH to INIT_REG.  */

void
dwarf2_frame_set_init_reg (struct gdbarch *gdbarch,
			   void (*init_reg) (struct gdbarch *, int,
					     struct dwarf2_frame_state_reg *,
					     struct frame_info *))
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  ops->init_reg = init_reg;
}

/* Pre-initialize the register state REG for register REGNUM.  */

static void
dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
		       struct dwarf2_frame_state_reg *reg,
		       struct frame_info *this_frame)
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  ops->init_reg (gdbarch, regnum, reg, this_frame);
}

/* Set the architecture-specific signal trampoline recognition
   function for GDBARCH to SIGNAL_FRAME_P.  */

void
dwarf2_frame_set_signal_frame_p (struct gdbarch *gdbarch,
				 int (*signal_frame_p) (struct gdbarch *,
							struct frame_info *))
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  ops->signal_frame_p = signal_frame_p;
}

/* Query the architecture-specific signal frame recognizer for
   THIS_FRAME.  */

static int
dwarf2_frame_signal_frame_p (struct gdbarch *gdbarch,
			     struct frame_info *this_frame)
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  if (ops->signal_frame_p == NULL)
    return 0;
  return ops->signal_frame_p (gdbarch, this_frame);
}

/* Set the architecture-specific adjustment of .eh_frame and .debug_frame
   register numbers.  */

void
dwarf2_frame_set_adjust_regnum (struct gdbarch *gdbarch,
				int (*adjust_regnum) (struct gdbarch *,
						      int, int))
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  ops->adjust_regnum = adjust_regnum;
}

/* Translate a .eh_frame register to DWARF register, or adjust a .debug_frame
   register.  */

static int
dwarf2_frame_adjust_regnum (struct gdbarch *gdbarch,
			    int regnum, int eh_frame_p)
{
  struct dwarf2_frame_ops *ops
    = (struct dwarf2_frame_ops *) gdbarch_data (gdbarch, dwarf2_frame_data);

  if (ops->adjust_regnum == NULL)
    return regnum;
  return ops->adjust_regnum (gdbarch, regnum, eh_frame_p);
}

static void
dwarf2_frame_find_quirks (struct dwarf2_frame_state *fs,
			  struct dwarf2_fde *fde)
{
  struct compunit_symtab *cust;

  cust = find_pc_compunit_symtab (fs->pc);
  if (cust == NULL)
    return;

  if (producer_is_realview (COMPUNIT_PRODUCER (cust)))
    {
      if (fde->cie->version == 1)
	fs->armcc_cfa_offsets_sf = 1;

      if (fde->cie->version == 1)
	fs->armcc_cfa_offsets_reversed = 1;

      /* The reversed offset problem is present in some compilers
	 using DWARF3, but it was eventually fixed.  Check the ARM
	 defined augmentations, which are in the format "armcc" followed
	 by a list of one-character options.  The "+" option means
	 this problem is fixed (no quirk needed).  If the armcc
	 augmentation is missing, the quirk is needed.  */
      if (fde->cie->version == 3
	  && (!startswith (fde->cie->augmentation, "armcc")
	      || strchr (fde->cie->augmentation + 5, '+') == NULL))
	fs->armcc_cfa_offsets_reversed = 1;

      return;
    }
}


/* See dwarf2-frame.h.  */

int
dwarf2_fetch_cfa_info (struct gdbarch *gdbarch, CORE_ADDR pc,
		       struct dwarf2_per_cu_data *data,
		       int *regnum_out, LONGEST *offset_out,
		       CORE_ADDR *text_offset_out,
		       const gdb_byte **cfa_start_out,
		       const gdb_byte **cfa_end_out)
{
  struct dwarf2_fde *fde;
  CORE_ADDR text_offset;
  struct dwarf2_frame_state fs;

  memset (&fs, 0, sizeof (struct dwarf2_frame_state));

  fs.pc = pc;

  /* Find the correct FDE.  */
  fde = dwarf2_frame_find_fde (&fs.pc, &text_offset);
  if (fde == NULL)
    error (_("Could not compute CFA; needed to translate this expression"));

  /* Extract any interesting information from the CIE.  */
  fs.data_align = fde->cie->data_alignment_factor;
  fs.code_align = fde->cie->code_alignment_factor;
  fs.retaddr_column = fde->cie->return_address_register;

  /* Check for "quirks" - known bugs in producers.  */
  dwarf2_frame_find_quirks (&fs, fde);

  /* First decode all the insns in the CIE.  */
  execute_cfa_program (fde, fde->cie->initial_instructions,
		       fde->cie->end, gdbarch, pc, &fs);

  /* Save the initialized register set.  */
  fs.initial = fs.regs;
  fs.initial.reg = dwarf2_frame_state_copy_regs (&fs.regs);

  /* Then decode the insns in the FDE up to our target PC.  */
  execute_cfa_program (fde, fde->instructions, fde->end, gdbarch, pc, &fs);

  /* Calculate the CFA.  */
  switch (fs.regs.cfa_how)
    {
    case CFA_REG_OFFSET:
      {
	int regnum = dwarf_reg_to_regnum_or_error (gdbarch, fs.regs.cfa_reg);

	*regnum_out = regnum;
	if (fs.armcc_cfa_offsets_reversed)
	  *offset_out = -fs.regs.cfa_offset;
	else
	  *offset_out = fs.regs.cfa_offset;
	return 1;
      }

    case CFA_EXP:
      *text_offset_out = text_offset;
      *cfa_start_out = fs.regs.cfa_exp;
      *cfa_end_out = fs.regs.cfa_exp + fs.regs.cfa_exp_len;
      return 0;

    default:
      internal_error (__FILE__, __LINE__, _("Unknown CFA rule."));
    }
}


struct dwarf2_frame_cache
{
  /* DWARF Call Frame Address.  */
  CORE_ADDR cfa;

  /* Set if the return address column was marked as unavailable
     (required non-collected memory or registers to compute).  */
  int unavailable_retaddr;

  /* Set if the return address column was marked as undefined.  */
  int undefined_retaddr;

  /* Saved registers, indexed by GDB register number, not by DWARF
     register number.  */
  struct dwarf2_frame_state_reg *reg;

  /* Return address register.  */
  struct dwarf2_frame_state_reg retaddr_reg;

  /* Target address size in bytes.  */
  int addr_size;

  /* The .text offset.  */
  CORE_ADDR text_offset;

  /* True if we already checked whether this frame is the bottom frame
     of a virtual tail call frame chain.  */
  int checked_tailcall_bottom;

  /* If not NULL then this frame is the bottom frame of a TAILCALL_FRAME
     sequence.  If NULL then it is a normal case with no TAILCALL_FRAME
     involved.  Non-bottom frames of a virtual tail call frames chain use
     dwarf2_tailcall_frame_unwind unwinder so this field does not apply for
     them.  */
  void *tailcall_cache;

  /* The number of bytes to subtract from TAILCALL_FRAME frames frame
     base to get the SP, to simulate the return address pushed on the
     stack.  */
  LONGEST entry_cfa_sp_offset;
  int entry_cfa_sp_offset_p;
};

/* A cleanup that sets a pointer to NULL.  */

static void
clear_pointer_cleanup (void *arg)
{
  void **ptr = (void **) arg;

  *ptr = NULL;
}

static struct dwarf2_frame_cache *
dwarf2_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct cleanup *reset_cache_cleanup, *old_chain;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  const int num_regs = gdbarch_num_regs (gdbarch)
		       + gdbarch_num_pseudo_regs (gdbarch);
  struct dwarf2_frame_cache *cache;
  struct dwarf2_frame_state *fs;
  struct dwarf2_fde *fde;
  CORE_ADDR entry_pc;
  const gdb_byte *instr;

  if (*this_cache)
    return (struct dwarf2_frame_cache *) *this_cache;

  /* Allocate a new cache.  */
  cache = FRAME_OBSTACK_ZALLOC (struct dwarf2_frame_cache);
  cache->reg = FRAME_OBSTACK_CALLOC (num_regs, struct dwarf2_frame_state_reg);
  *this_cache = cache;
  reset_cache_cleanup = make_cleanup (clear_pointer_cleanup, this_cache);

  /* Allocate and initialize the frame state.  */
  fs = XCNEW (struct dwarf2_frame_state);
  old_chain = make_cleanup (dwarf2_frame_state_free, fs);

  /* Unwind the PC.

     Note that if the next frame is never supposed to return (i.e. a call
     to abort), the compiler might optimize away the instruction at
     its return address.  As a result the return address will
     point at some random instruction, and the CFI for that
     instruction is probably worthless to us.  GCC's unwinder solves
     this problem by substracting 1 from the return address to get an
     address in the middle of a presumed call instruction (or the
     instruction in the associated delay slot).  This should only be
     done for "normal" frames and not for resume-type frames (signal
     handlers, sentinel frames, dummy frames).  The function
     get_frame_address_in_block does just this.  It's not clear how
     reliable the method is though; there is the potential for the
     register state pre-call being different to that on return.  */
  fs->pc = get_frame_address_in_block (this_frame);

  /* Find the correct FDE.  */
  fde = dwarf2_frame_find_fde (&fs->pc, &cache->text_offset);
  gdb_assert (fde != NULL);

  /* Extract any interesting information from the CIE.  */
  fs->data_align = fde->cie->data_alignment_factor;
  fs->code_align = fde->cie->code_alignment_factor;
  fs->retaddr_column = fde->cie->return_address_register;
  cache->addr_size = fde->cie->addr_size;

  /* Check for "quirks" - known bugs in producers.  */
  dwarf2_frame_find_quirks (fs, fde);

  /* First decode all the insns in the CIE.  */
  execute_cfa_program (fde, fde->cie->initial_instructions,
		       fde->cie->end, gdbarch,
		       get_frame_address_in_block (this_frame), fs);

  /* Save the initialized register set.  */
  fs->initial = fs->regs;
  fs->initial.reg = dwarf2_frame_state_copy_regs (&fs->regs);

  if (get_frame_func_if_available (this_frame, &entry_pc))
    {
      /* Decode the insns in the FDE up to the entry PC.  */
      instr = execute_cfa_program (fde, fde->instructions, fde->end, gdbarch,
				   entry_pc, fs);

      if (fs->regs.cfa_how == CFA_REG_OFFSET
	  && (dwarf_reg_to_regnum (gdbarch, fs->regs.cfa_reg)
	      == gdbarch_sp_regnum (gdbarch)))
	{
	  cache->entry_cfa_sp_offset = fs->regs.cfa_offset;
	  cache->entry_cfa_sp_offset_p = 1;
	}
    }
  else
    instr = fde->instructions;

  /* Then decode the insns in the FDE up to our target PC.  */
  execute_cfa_program (fde, instr, fde->end, gdbarch,
		       get_frame_address_in_block (this_frame), fs);

  TRY
    {
      /* Calculate the CFA.  */
      switch (fs->regs.cfa_how)
	{
	case CFA_REG_OFFSET:
	  cache->cfa = read_addr_from_reg (this_frame, fs->regs.cfa_reg);
	  if (fs->armcc_cfa_offsets_reversed)
	    cache->cfa -= fs->regs.cfa_offset;
	  else
	    cache->cfa += fs->regs.cfa_offset;
	  break;

	case CFA_EXP:
	  cache->cfa =
	    execute_stack_op (fs->regs.cfa_exp, fs->regs.cfa_exp_len,
			      cache->addr_size, cache->text_offset,
			      this_frame, 0, 0);
	  break;

	default:
	  internal_error (__FILE__, __LINE__, _("Unknown CFA rule."));
	}
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  cache->unavailable_retaddr = 1;
	  do_cleanups (old_chain);
	  discard_cleanups (reset_cache_cleanup);
	  return cache;
	}

      throw_exception (ex);
    }
  END_CATCH

  /* Initialize the register state.  */
  {
    int regnum;

    for (regnum = 0; regnum < num_regs; regnum++)
      dwarf2_frame_init_reg (gdbarch, regnum, &cache->reg[regnum], this_frame);
  }

  /* Go through the DWARF2 CFI generated table and save its register
     location information in the cache.  Note that we don't skip the
     return address column; it's perfectly all right for it to
     correspond to a real register.  */
  {
    int column;		/* CFI speak for "register number".  */

    for (column = 0; column < fs->regs.num_regs; column++)
      {
	/* Use the GDB register number as the destination index.  */
	int regnum = dwarf_reg_to_regnum (gdbarch, column);

	/* Protect against a target returning a bad register.  */
	if (regnum < 0 || regnum >= num_regs)
	  continue;

	/* NOTE: cagney/2003-09-05: CFI should specify the disposition
	   of all debug info registers.  If it doesn't, complain (but
	   not too loudly).  It turns out that GCC assumes that an
	   unspecified register implies "same value" when CFI (draft
	   7) specifies nothing at all.  Such a register could equally
	   be interpreted as "undefined".  Also note that this check
	   isn't sufficient; it only checks that all registers in the
	   range [0 .. max column] are specified, and won't detect
	   problems when a debug info register falls outside of the
	   table.  We need a way of iterating through all the valid
	   DWARF2 register numbers.  */
	if (fs->regs.reg[column].how == DWARF2_FRAME_REG_UNSPECIFIED)
	  {
	    if (cache->reg[regnum].how == DWARF2_FRAME_REG_UNSPECIFIED)
	      complaint (&symfile_complaints, _("\
incomplete CFI data; unspecified registers (e.g., %s) at %s"),
			 gdbarch_register_name (gdbarch, regnum),
			 paddress (gdbarch, fs->pc));
	  }
	else
	  cache->reg[regnum] = fs->regs.reg[column];
      }
  }

  /* Eliminate any DWARF2_FRAME_REG_RA rules, and save the information
     we need for evaluating DWARF2_FRAME_REG_RA_OFFSET rules.  */
  {
    int regnum;

    for (regnum = 0; regnum < num_regs; regnum++)
      {
	if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA
	    || cache->reg[regnum].how == DWARF2_FRAME_REG_RA_OFFSET)
	  {
	    struct dwarf2_frame_state_reg *retaddr_reg =
	      &fs->regs.reg[fs->retaddr_column];

	    /* It seems rather bizarre to specify an "empty" column as
               the return adress column.  However, this is exactly
               what GCC does on some targets.  It turns out that GCC
               assumes that the return address can be found in the
               register corresponding to the return address column.
               Incidentally, that's how we should treat a return
               address column specifying "same value" too.  */
	    if (fs->retaddr_column < fs->regs.num_regs
		&& retaddr_reg->how != DWARF2_FRAME_REG_UNSPECIFIED
		&& retaddr_reg->how != DWARF2_FRAME_REG_SAME_VALUE)
	      {
		if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA)
		  cache->reg[regnum] = *retaddr_reg;
		else
		  cache->retaddr_reg = *retaddr_reg;
	      }
	    else
	      {
		if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA)
		  {
		    cache->reg[regnum].loc.reg = fs->retaddr_column;
		    cache->reg[regnum].how = DWARF2_FRAME_REG_SAVED_REG;
		  }
		else
		  {
		    cache->retaddr_reg.loc.reg = fs->retaddr_column;
		    cache->retaddr_reg.how = DWARF2_FRAME_REG_SAVED_REG;
		  }
	      }
	  }
      }
  }

  if (fs->retaddr_column < fs->regs.num_regs
      && fs->regs.reg[fs->retaddr_column].how == DWARF2_FRAME_REG_UNDEFINED)
    cache->undefined_retaddr = 1;

  do_cleanups (old_chain);
  discard_cleanups (reset_cache_cleanup);
  return cache;
}

static enum unwind_stop_reason
dwarf2_frame_unwind_stop_reason (struct frame_info *this_frame,
				 void **this_cache)
{
  struct dwarf2_frame_cache *cache
    = dwarf2_frame_cache (this_frame, this_cache);

  if (cache->unavailable_retaddr)
    return UNWIND_UNAVAILABLE;

  if (cache->undefined_retaddr)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

static void
dwarf2_frame_this_id (struct frame_info *this_frame, void **this_cache,
		      struct frame_id *this_id)
{
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);

  if (cache->unavailable_retaddr)
    (*this_id) = frame_id_build_unavailable_stack (get_frame_func (this_frame));
  else if (cache->undefined_retaddr)
    return;
  else
    (*this_id) = frame_id_build (cache->cfa, get_frame_func (this_frame));
}

static struct value *
dwarf2_frame_prev_register (struct frame_info *this_frame, void **this_cache,
			    int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);
  CORE_ADDR addr;
  int realnum;

  /* Check whether THIS_FRAME is the bottom frame of a virtual tail
     call frame chain.  */
  if (!cache->checked_tailcall_bottom)
    {
      cache->checked_tailcall_bottom = 1;
      dwarf2_tailcall_sniffer_first (this_frame, &cache->tailcall_cache,
				     (cache->entry_cfa_sp_offset_p
				      ? &cache->entry_cfa_sp_offset : NULL));
    }

  /* Non-bottom frames of a virtual tail call frames chain use
     dwarf2_tailcall_frame_unwind unwinder so this code does not apply for
     them.  If dwarf2_tailcall_prev_register_first does not have specific value
     unwind the register, tail call frames are assumed to have the register set
     of the top caller.  */
  if (cache->tailcall_cache)
    {
      struct value *val;
      
      val = dwarf2_tailcall_prev_register_first (this_frame,
						 &cache->tailcall_cache,
						 regnum);
      if (val)
	return val;
    }

  switch (cache->reg[regnum].how)
    {
    case DWARF2_FRAME_REG_UNDEFINED:
      /* If CFI explicitly specified that the value isn't defined,
	 mark it as optimized away; the value isn't available.  */
      return frame_unwind_got_optimized (this_frame, regnum);

    case DWARF2_FRAME_REG_SAVED_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_memory (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_REG:
      realnum = dwarf_reg_to_regnum_or_error
	(gdbarch, cache->reg[regnum].loc.reg);
      return frame_unwind_got_register (this_frame, regnum, realnum);

    case DWARF2_FRAME_REG_SAVED_EXP:
      addr = execute_stack_op (cache->reg[regnum].loc.exp,
			       cache->reg[regnum].exp_len,
			       cache->addr_size, cache->text_offset,
			       this_frame, cache->cfa, 1);
      return frame_unwind_got_memory (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_VAL_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_constant (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_VAL_EXP:
      addr = execute_stack_op (cache->reg[regnum].loc.exp,
			       cache->reg[regnum].exp_len,
			       cache->addr_size, cache->text_offset,
			       this_frame, cache->cfa, 1);
      return frame_unwind_got_constant (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_UNSPECIFIED:
      /* GCC, in its infinite wisdom decided to not provide unwind
	 information for registers that are "same value".  Since
	 DWARF2 (3 draft 7) doesn't define such behavior, said
	 registers are actually undefined (which is different to CFI
	 "undefined").  Code above issues a complaint about this.
	 Here just fudge the books, assume GCC, and that the value is
	 more inner on the stack.  */
      return frame_unwind_got_register (this_frame, regnum, regnum);

    case DWARF2_FRAME_REG_SAME_VALUE:
      return frame_unwind_got_register (this_frame, regnum, regnum);

    case DWARF2_FRAME_REG_CFA:
      return frame_unwind_got_address (this_frame, regnum, cache->cfa);

    case DWARF2_FRAME_REG_CFA_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_address (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_RA_OFFSET:
      addr = cache->reg[regnum].loc.offset;
      regnum = dwarf_reg_to_regnum_or_error
	(gdbarch, cache->retaddr_reg.loc.reg);
      addr += get_frame_register_unsigned (this_frame, regnum);
      return frame_unwind_got_address (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_FN:
      return cache->reg[regnum].loc.fn (this_frame, this_cache, regnum);

    default:
      internal_error (__FILE__, __LINE__, _("Unknown register rule."));
    }
}

/* Proxy for tailcall_frame_dealloc_cache for bottom frame of a virtual tail
   call frames chain.  */

static void
dwarf2_frame_dealloc_cache (struct frame_info *self, void *this_cache)
{
  struct dwarf2_frame_cache *cache = dwarf2_frame_cache (self, &this_cache);

  if (cache->tailcall_cache)
    dwarf2_tailcall_frame_unwind.dealloc_cache (self, cache->tailcall_cache);
}

static int
dwarf2_frame_sniffer (const struct frame_unwind *self,
		      struct frame_info *this_frame, void **this_cache)
{
  /* Grab an address that is guarenteed to reside somewhere within the
     function.  get_frame_pc(), with a no-return next function, can
     end up returning something past the end of this function's body.
     If the frame we're sniffing for is a signal frame whose start
     address is placed on the stack by the OS, its FDE must
     extend one byte before its start address or we could potentially
     select the FDE of the previous function.  */
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  struct dwarf2_fde *fde = dwarf2_frame_find_fde (&block_addr, NULL);

  if (!fde)
    return 0;

  /* On some targets, signal trampolines may have unwind information.
     We need to recognize them so that we set the frame type
     correctly.  */

  if (fde->cie->signal_frame
      || dwarf2_frame_signal_frame_p (get_frame_arch (this_frame),
				      this_frame))
    return self->type == SIGTRAMP_FRAME;

  if (self->type != NORMAL_FRAME)
    return 0;

  return 1;
}

static const struct frame_unwind dwarf2_frame_unwind =
{
  NORMAL_FRAME,
  dwarf2_frame_unwind_stop_reason,
  dwarf2_frame_this_id,
  dwarf2_frame_prev_register,
  NULL,
  dwarf2_frame_sniffer,
  dwarf2_frame_dealloc_cache
};

static const struct frame_unwind dwarf2_signal_frame_unwind =
{
  SIGTRAMP_FRAME,
  dwarf2_frame_unwind_stop_reason,
  dwarf2_frame_this_id,
  dwarf2_frame_prev_register,
  NULL,
  dwarf2_frame_sniffer,

  /* TAILCALL_CACHE can never be in such frame to need dealloc_cache.  */
  NULL
};

/* Append the DWARF-2 frame unwinders to GDBARCH's list.  */

void
dwarf2_append_unwinders (struct gdbarch *gdbarch)
{
  /* TAILCALL_FRAME must be first to find the record by
     dwarf2_tailcall_sniffer_first.  */
  frame_unwind_append_unwinder (gdbarch, &dwarf2_tailcall_frame_unwind);

  frame_unwind_append_unwinder (gdbarch, &dwarf2_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &dwarf2_signal_frame_unwind);
}


/* There is no explicitly defined relationship between the CFA and the
   location of frame's local variables and arguments/parameters.
   Therefore, frame base methods on this page should probably only be
   used as a last resort, just to avoid printing total garbage as a
   response to the "info frame" command.  */

static CORE_ADDR
dwarf2_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);

  return cache->cfa;
}

static const struct frame_base dwarf2_frame_base =
{
  &dwarf2_frame_unwind,
  dwarf2_frame_base_address,
  dwarf2_frame_base_address,
  dwarf2_frame_base_address
};

const struct frame_base *
dwarf2_frame_base_sniffer (struct frame_info *this_frame)
{
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);

  if (dwarf2_frame_find_fde (&block_addr, NULL))
    return &dwarf2_frame_base;

  return NULL;
}

/* Compute the CFA for THIS_FRAME, but only if THIS_FRAME came from
   the DWARF unwinder.  This is used to implement
   DW_OP_call_frame_cfa.  */

CORE_ADDR
dwarf2_frame_cfa (struct frame_info *this_frame)
{
  if (frame_unwinder_is (this_frame, &record_btrace_tailcall_frame_unwind)
      || frame_unwinder_is (this_frame, &record_btrace_frame_unwind))
    throw_error (NOT_AVAILABLE_ERROR,
		 _("cfa not available for record btrace target"));

  while (get_frame_type (this_frame) == INLINE_FRAME)
    this_frame = get_prev_frame (this_frame);
  if (get_frame_unwind_stop_reason (this_frame) == UNWIND_UNAVAILABLE)
    throw_error (NOT_AVAILABLE_ERROR,
                _("can't compute CFA for this frame: "
                  "required registers or memory are unavailable"));

  if (get_frame_id (this_frame).stack_status != FID_STACK_VALID)
    throw_error (NOT_AVAILABLE_ERROR,
                _("can't compute CFA for this frame: "
                  "frame base not available"));

  return get_frame_base (this_frame);
}

const struct objfile_data *dwarf2_frame_objfile_data;

static unsigned int
read_1_byte (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_8 (abfd, buf);
}

static unsigned int
read_4_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_32 (abfd, buf);
}

static ULONGEST
read_8_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_64 (abfd, buf);
}

static ULONGEST
read_initial_length (bfd *abfd, const gdb_byte *buf,
		     unsigned int *bytes_read_ptr)
{
  LONGEST result;

  result = bfd_get_32 (abfd, buf);
  if (result == 0xffffffff)
    {
      result = bfd_get_64 (abfd, buf + 4);
      *bytes_read_ptr = 12;
    }
  else
    *bytes_read_ptr = 4;

  return result;
}


/* Pointer encoding helper functions.  */

/* GCC supports exception handling based on DWARF2 CFI.  However, for
   technical reasons, it encodes addresses in its FDE's in a different
   way.  Several "pointer encodings" are supported.  The encoding
   that's used for a particular FDE is determined by the 'R'
   augmentation in the associated CIE.  The argument of this
   augmentation is a single byte.  

   The address can be encoded as 2 bytes, 4 bytes, 8 bytes, or as a
   LEB128.  This is encoded in bits 0, 1 and 2.  Bit 3 encodes whether
   the address is signed or unsigned.  Bits 4, 5 and 6 encode how the
   address should be interpreted (absolute, relative to the current
   position in the FDE, ...).  Bit 7, indicates that the address
   should be dereferenced.  */

static gdb_byte
encoding_for_size (unsigned int size)
{
  switch (size)
    {
    case 2:
      return DW_EH_PE_udata2;
    case 4:
      return DW_EH_PE_udata4;
    case 8:
      return DW_EH_PE_udata8;
    default:
      internal_error (__FILE__, __LINE__, _("Unsupported address size"));
    }
}

static CORE_ADDR
read_encoded_value (struct comp_unit *unit, gdb_byte encoding,
		    int ptr_len, const gdb_byte *buf,
		    unsigned int *bytes_read_ptr,
		    CORE_ADDR func_base)
{
  ptrdiff_t offset;
  CORE_ADDR base;

  /* GCC currently doesn't generate DW_EH_PE_indirect encodings for
     FDE's.  */
  if (encoding & DW_EH_PE_indirect)
    internal_error (__FILE__, __LINE__, 
		    _("Unsupported encoding: DW_EH_PE_indirect"));

  *bytes_read_ptr = 0;

  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
      base = 0;
      break;
    case DW_EH_PE_pcrel:
      base = bfd_get_section_vma (unit->abfd, unit->dwarf_frame_section);
      base += (buf - unit->dwarf_frame_buffer);
      break;
    case DW_EH_PE_datarel:
      base = unit->dbase;
      break;
    case DW_EH_PE_textrel:
      base = unit->tbase;
      break;
    case DW_EH_PE_funcrel:
      base = func_base;
      break;
    case DW_EH_PE_aligned:
      base = 0;
      offset = buf - unit->dwarf_frame_buffer;
      if ((offset % ptr_len) != 0)
	{
	  *bytes_read_ptr = ptr_len - (offset % ptr_len);
	  buf += *bytes_read_ptr;
	}
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid or unsupported encoding"));
    }

  if ((encoding & 0x07) == 0x00)
    {
      encoding |= encoding_for_size (ptr_len);
      if (bfd_get_sign_extend_vma (unit->abfd))
	encoding |= DW_EH_PE_signed;
    }

  switch (encoding & 0x0f)
    {
    case DW_EH_PE_uleb128:
      {
	uint64_t value;
	const gdb_byte *end_buf = buf + (sizeof (value) + 1) * 8 / 7;

	*bytes_read_ptr += safe_read_uleb128 (buf, end_buf, &value) - buf;
	return base + value;
      }
    case DW_EH_PE_udata2:
      *bytes_read_ptr += 2;
      return (base + bfd_get_16 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_udata4:
      *bytes_read_ptr += 4;
      return (base + bfd_get_32 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_udata8:
      *bytes_read_ptr += 8;
      return (base + bfd_get_64 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sleb128:
      {
	int64_t value;
	const gdb_byte *end_buf = buf + (sizeof (value) + 1) * 8 / 7;

	*bytes_read_ptr += safe_read_sleb128 (buf, end_buf, &value) - buf;
	return base + value;
      }
    case DW_EH_PE_sdata2:
      *bytes_read_ptr += 2;
      return (base + bfd_get_signed_16 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sdata4:
      *bytes_read_ptr += 4;
      return (base + bfd_get_signed_32 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sdata8:
      *bytes_read_ptr += 8;
      return (base + bfd_get_signed_64 (unit->abfd, (bfd_byte *) buf));
    default:
      internal_error (__FILE__, __LINE__,
		      _("Invalid or unsupported encoding"));
    }
}


static int
bsearch_cie_cmp (const void *key, const void *element)
{
  ULONGEST cie_pointer = *(ULONGEST *) key;
  struct dwarf2_cie *cie = *(struct dwarf2_cie **) element;

  if (cie_pointer == cie->cie_pointer)
    return 0;

  return (cie_pointer < cie->cie_pointer) ? -1 : 1;
}

/* Find CIE with the given CIE_POINTER in CIE_TABLE.  */
static struct dwarf2_cie *
find_cie (struct dwarf2_cie_table *cie_table, ULONGEST cie_pointer)
{
  struct dwarf2_cie **p_cie;

  /* The C standard (ISO/IEC 9899:TC2) requires the BASE argument to
     bsearch be non-NULL.  */
  if (cie_table->entries == NULL)
    {
      gdb_assert (cie_table->num_entries == 0);
      return NULL;
    }

  p_cie = ((struct dwarf2_cie **)
	   bsearch (&cie_pointer, cie_table->entries, cie_table->num_entries,
		    sizeof (cie_table->entries[0]), bsearch_cie_cmp));
  if (p_cie != NULL)
    return *p_cie;
  return NULL;
}

/* Add a pointer to new CIE to the CIE_TABLE, allocating space for it.  */
static void
add_cie (struct dwarf2_cie_table *cie_table, struct dwarf2_cie *cie)
{
  const int n = cie_table->num_entries;

  gdb_assert (n < 1
              || cie_table->entries[n - 1]->cie_pointer < cie->cie_pointer);

  cie_table->entries
    = XRESIZEVEC (struct dwarf2_cie *, cie_table->entries, n + 1);
  cie_table->entries[n] = cie;
  cie_table->num_entries = n + 1;
}

static int
bsearch_fde_cmp (const void *key, const void *element)
{
  CORE_ADDR seek_pc = *(CORE_ADDR *) key;
  struct dwarf2_fde *fde = *(struct dwarf2_fde **) element;

  if (seek_pc < fde->initial_location)
    return -1;
  if (seek_pc < fde->initial_location + fde->address_range)
    return 0;
  return 1;
}

/* Find the FDE for *PC.  Return a pointer to the FDE, and store the
   inital location associated with it into *PC.  */

static struct dwarf2_fde *
dwarf2_frame_find_fde (CORE_ADDR *pc, CORE_ADDR *out_offset)
{
  struct objfile *objfile;

  ALL_OBJFILES (objfile)
    {
      struct dwarf2_fde_table *fde_table;
      struct dwarf2_fde **p_fde;
      CORE_ADDR offset;
      CORE_ADDR seek_pc;

      fde_table = ((struct dwarf2_fde_table *)
		   objfile_data (objfile, dwarf2_frame_objfile_data));
      if (fde_table == NULL)
	{
	  dwarf2_build_frame_info (objfile);
	  fde_table = ((struct dwarf2_fde_table *)
		       objfile_data (objfile, dwarf2_frame_objfile_data));
	}
      gdb_assert (fde_table != NULL);

      if (fde_table->num_entries == 0)
	continue;

      gdb_assert (objfile->section_offsets);
      offset = ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));

      gdb_assert (fde_table->num_entries > 0);
      if (*pc < offset + fde_table->entries[0]->initial_location)
        continue;

      seek_pc = *pc - offset;
      p_fde = ((struct dwarf2_fde **)
	       bsearch (&seek_pc, fde_table->entries, fde_table->num_entries,
                        sizeof (fde_table->entries[0]), bsearch_fde_cmp));
      if (p_fde != NULL)
        {
          *pc = (*p_fde)->initial_location + offset;
	  if (out_offset)
	    *out_offset = offset;
          return *p_fde;
        }
    }
  return NULL;
}

/* Add a pointer to new FDE to the FDE_TABLE, allocating space for it.  */
static void
add_fde (struct dwarf2_fde_table *fde_table, struct dwarf2_fde *fde)
{
  if (fde->address_range == 0)
    /* Discard useless FDEs.  */
    return;

  fde_table->num_entries += 1;
  fde_table->entries = XRESIZEVEC (struct dwarf2_fde *, fde_table->entries,
				   fde_table->num_entries);
  fde_table->entries[fde_table->num_entries - 1] = fde;
}

#define DW64_CIE_ID 0xffffffffffffffffULL

/* Defines the type of eh_frames that are expected to be decoded: CIE, FDE
   or any of them.  */

enum eh_frame_type
{
  EH_CIE_TYPE_ID = 1 << 0,
  EH_FDE_TYPE_ID = 1 << 1,
  EH_CIE_OR_FDE_TYPE_ID = EH_CIE_TYPE_ID | EH_FDE_TYPE_ID
};

static const gdb_byte *decode_frame_entry (struct comp_unit *unit,
					   const gdb_byte *start,
					   int eh_frame_p,
					   struct dwarf2_cie_table *cie_table,
					   struct dwarf2_fde_table *fde_table,
					   enum eh_frame_type entry_type);

/* Decode the next CIE or FDE, entry_type specifies the expected type.
   Return NULL if invalid input, otherwise the next byte to be processed.  */

static const gdb_byte *
decode_frame_entry_1 (struct comp_unit *unit, const gdb_byte *start,
		      int eh_frame_p,
                      struct dwarf2_cie_table *cie_table,
                      struct dwarf2_fde_table *fde_table,
                      enum eh_frame_type entry_type)
{
  struct gdbarch *gdbarch = get_objfile_arch (unit->objfile);
  const gdb_byte *buf, *end;
  LONGEST length;
  unsigned int bytes_read;
  int dwarf64_p;
  ULONGEST cie_id;
  ULONGEST cie_pointer;
  int64_t sleb128;
  uint64_t uleb128;

  buf = start;
  length = read_initial_length (unit->abfd, buf, &bytes_read);
  buf += bytes_read;
  end = buf + length;

  /* Are we still within the section?  */
  if (end > unit->dwarf_frame_buffer + unit->dwarf_frame_size)
    return NULL;

  if (length == 0)
    return end;

  /* Distinguish between 32 and 64-bit encoded frame info.  */
  dwarf64_p = (bytes_read == 12);

  /* In a .eh_frame section, zero is used to distinguish CIEs from FDEs.  */
  if (eh_frame_p)
    cie_id = 0;
  else if (dwarf64_p)
    cie_id = DW64_CIE_ID;
  else
    cie_id = DW_CIE_ID;

  if (dwarf64_p)
    {
      cie_pointer = read_8_bytes (unit->abfd, buf);
      buf += 8;
    }
  else
    {
      cie_pointer = read_4_bytes (unit->abfd, buf);
      buf += 4;
    }

  if (cie_pointer == cie_id)
    {
      /* This is a CIE.  */
      struct dwarf2_cie *cie;
      char *augmentation;
      unsigned int cie_version;

      /* Check that a CIE was expected.  */
      if ((entry_type & EH_CIE_TYPE_ID) == 0)
	error (_("Found a CIE when not expecting it."));

      /* Record the offset into the .debug_frame section of this CIE.  */
      cie_pointer = start - unit->dwarf_frame_buffer;

      /* Check whether we've already read it.  */
      if (find_cie (cie_table, cie_pointer))
	return end;

      cie = XOBNEW (&unit->objfile->objfile_obstack, struct dwarf2_cie);
      cie->initial_instructions = NULL;
      cie->cie_pointer = cie_pointer;

      /* The encoding for FDE's in a normal .debug_frame section
         depends on the target address size.  */
      cie->encoding = DW_EH_PE_absptr;

      /* We'll determine the final value later, but we need to
	 initialize it conservatively.  */
      cie->signal_frame = 0;

      /* Check version number.  */
      cie_version = read_1_byte (unit->abfd, buf);
      if (cie_version != 1 && cie_version != 3 && cie_version != 4)
	return NULL;
      cie->version = cie_version;
      buf += 1;

      /* Interpret the interesting bits of the augmentation.  */
      cie->augmentation = augmentation = (char *) buf;
      buf += (strlen (augmentation) + 1);

      /* Ignore armcc augmentations.  We only use them for quirks,
	 and that doesn't happen until later.  */
      if (startswith (augmentation, "armcc"))
	augmentation += strlen (augmentation);

      /* The GCC 2.x "eh" augmentation has a pointer immediately
         following the augmentation string, so it must be handled
         first.  */
      if (augmentation[0] == 'e' && augmentation[1] == 'h')
	{
	  /* Skip.  */
	  buf += gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
	  augmentation += 2;
	}

      if (cie->version >= 4)
	{
	  /* FIXME: check that this is the same as from the CU header.  */
	  cie->addr_size = read_1_byte (unit->abfd, buf);
	  ++buf;
	  cie->segment_size = read_1_byte (unit->abfd, buf);
	  ++buf;
	}
      else
	{
	  cie->addr_size = gdbarch_dwarf2_addr_size (gdbarch);
	  cie->segment_size = 0;
	}
      /* Address values in .eh_frame sections are defined to have the
	 target's pointer size.  Watchout: This breaks frame info for
	 targets with pointer size < address size, unless a .debug_frame
	 section exists as well.  */
      if (eh_frame_p)
	cie->ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      else
	cie->ptr_size = cie->addr_size;

      buf = gdb_read_uleb128 (buf, end, &uleb128);
      if (buf == NULL)
	return NULL;
      cie->code_alignment_factor = uleb128;

      buf = gdb_read_sleb128 (buf, end, &sleb128);
      if (buf == NULL)
	return NULL;
      cie->data_alignment_factor = sleb128;

      if (cie_version == 1)
	{
	  cie->return_address_register = read_1_byte (unit->abfd, buf);
	  ++buf;
	}
      else
	{
	  buf = gdb_read_uleb128 (buf, end, &uleb128);
	  if (buf == NULL)
	    return NULL;
	  cie->return_address_register = uleb128;
	}

      cie->return_address_register
	= dwarf2_frame_adjust_regnum (gdbarch,
				      cie->return_address_register,
				      eh_frame_p);

      cie->saw_z_augmentation = (*augmentation == 'z');
      if (cie->saw_z_augmentation)
	{
	  uint64_t length;

	  buf = gdb_read_uleb128 (buf, end, &length);
	  if (buf == NULL)
	    return NULL;
	  cie->initial_instructions = buf + length;
	  augmentation++;
	}

      while (*augmentation)
	{
	  /* "L" indicates a byte showing how the LSDA pointer is encoded.  */
	  if (*augmentation == 'L')
	    {
	      /* Skip.  */
	      buf++;
	      augmentation++;
	    }

	  /* "R" indicates a byte indicating how FDE addresses are encoded.  */
	  else if (*augmentation == 'R')
	    {
	      cie->encoding = *buf++;
	      augmentation++;
	    }

	  /* "P" indicates a personality routine in the CIE augmentation.  */
	  else if (*augmentation == 'P')
	    {
	      /* Skip.  Avoid indirection since we throw away the result.  */
	      gdb_byte encoding = (*buf++) & ~DW_EH_PE_indirect;
	      read_encoded_value (unit, encoding, cie->ptr_size,
				  buf, &bytes_read, 0);
	      buf += bytes_read;
	      augmentation++;
	    }

	  /* "S" indicates a signal frame, such that the return
	     address must not be decremented to locate the call frame
	     info for the previous frame; it might even be the first
	     instruction of a function, so decrementing it would take
	     us to a different function.  */
	  else if (*augmentation == 'S')
	    {
	      cie->signal_frame = 1;
	      augmentation++;
	    }

	  /* Otherwise we have an unknown augmentation.  Assume that either
	     there is no augmentation data, or we saw a 'z' prefix.  */
	  else
	    {
	      if (cie->initial_instructions)
		buf = cie->initial_instructions;
	      break;
	    }
	}

      cie->initial_instructions = buf;
      cie->end = end;
      cie->unit = unit;

      add_cie (cie_table, cie);
    }
  else
    {
      /* This is a FDE.  */
      struct dwarf2_fde *fde;
      CORE_ADDR addr;

      /* Check that an FDE was expected.  */
      if ((entry_type & EH_FDE_TYPE_ID) == 0)
	error (_("Found an FDE when not expecting it."));

      /* In an .eh_frame section, the CIE pointer is the delta between the
	 address within the FDE where the CIE pointer is stored and the
	 address of the CIE.  Convert it to an offset into the .eh_frame
	 section.  */
      if (eh_frame_p)
	{
	  cie_pointer = buf - unit->dwarf_frame_buffer - cie_pointer;
	  cie_pointer -= (dwarf64_p ? 8 : 4);
	}

      /* In either case, validate the result is still within the section.  */
      if (cie_pointer >= unit->dwarf_frame_size)
	return NULL;

      fde = XOBNEW (&unit->objfile->objfile_obstack, struct dwarf2_fde);
      fde->cie = find_cie (cie_table, cie_pointer);
      if (fde->cie == NULL)
	{
	  decode_frame_entry (unit, unit->dwarf_frame_buffer + cie_pointer,
			      eh_frame_p, cie_table, fde_table,
			      EH_CIE_TYPE_ID);
	  fde->cie = find_cie (cie_table, cie_pointer);
	}

      gdb_assert (fde->cie != NULL);

      addr = read_encoded_value (unit, fde->cie->encoding, fde->cie->ptr_size,
				 buf, &bytes_read, 0);
      fde->initial_location = gdbarch_adjust_dwarf2_addr (gdbarch, addr);
      buf += bytes_read;

      fde->address_range =
	read_encoded_value (unit, fde->cie->encoding & 0x0f,
			    fde->cie->ptr_size, buf, &bytes_read, 0);
      addr = gdbarch_adjust_dwarf2_addr (gdbarch, addr + fde->address_range);
      fde->address_range = addr - fde->initial_location;
      buf += bytes_read;

      /* A 'z' augmentation in the CIE implies the presence of an
	 augmentation field in the FDE as well.  The only thing known
	 to be in here at present is the LSDA entry for EH.  So we
	 can skip the whole thing.  */
      if (fde->cie->saw_z_augmentation)
	{
	  uint64_t length;

	  buf = gdb_read_uleb128 (buf, end, &length);
	  if (buf == NULL)
	    return NULL;
	  buf += length;
	  if (buf > end)
	    return NULL;
	}

      fde->instructions = buf;
      fde->end = end;

      fde->eh_frame_p = eh_frame_p;

      add_fde (fde_table, fde);
    }

  return end;
}

/* Read a CIE or FDE in BUF and decode it. Entry_type specifies whether we
   expect an FDE or a CIE.  */

static const gdb_byte *
decode_frame_entry (struct comp_unit *unit, const gdb_byte *start,
		    int eh_frame_p,
                    struct dwarf2_cie_table *cie_table,
                    struct dwarf2_fde_table *fde_table,
                    enum eh_frame_type entry_type)
{
  enum { NONE, ALIGN4, ALIGN8, FAIL } workaround = NONE;
  const gdb_byte *ret;
  ptrdiff_t start_offset;

  while (1)
    {
      ret = decode_frame_entry_1 (unit, start, eh_frame_p,
				  cie_table, fde_table, entry_type);
      if (ret != NULL)
	break;

      /* We have corrupt input data of some form.  */

      /* ??? Try, weakly, to work around compiler/assembler/linker bugs
	 and mismatches wrt padding and alignment of debug sections.  */
      /* Note that there is no requirement in the standard for any
	 alignment at all in the frame unwind sections.  Testing for
	 alignment before trying to interpret data would be incorrect.

	 However, GCC traditionally arranged for frame sections to be
	 sized such that the FDE length and CIE fields happen to be
	 aligned (in theory, for performance).  This, unfortunately,
	 was done with .align directives, which had the side effect of
	 forcing the section to be aligned by the linker.

	 This becomes a problem when you have some other producer that
	 creates frame sections that are not as strictly aligned.  That
	 produces a hole in the frame info that gets filled by the 
	 linker with zeros.

	 The GCC behaviour is arguably a bug, but it's effectively now
	 part of the ABI, so we're now stuck with it, at least at the
	 object file level.  A smart linker may decide, in the process
	 of compressing duplicate CIE information, that it can rewrite
	 the entire output section without this extra padding.  */

      start_offset = start - unit->dwarf_frame_buffer;
      if (workaround < ALIGN4 && (start_offset & 3) != 0)
	{
	  start += 4 - (start_offset & 3);
	  workaround = ALIGN4;
	  continue;
	}
      if (workaround < ALIGN8 && (start_offset & 7) != 0)
	{
	  start += 8 - (start_offset & 7);
	  workaround = ALIGN8;
	  continue;
	}

      /* Nothing left to try.  Arrange to return as if we've consumed
	 the entire input section.  Hopefully we'll get valid info from
	 the other of .debug_frame/.eh_frame.  */
      workaround = FAIL;
      ret = unit->dwarf_frame_buffer + unit->dwarf_frame_size;
      break;
    }

  switch (workaround)
    {
    case NONE:
      break;

    case ALIGN4:
      complaint (&symfile_complaints, _("\
Corrupt data in %s:%s; align 4 workaround apparently succeeded"),
		 unit->dwarf_frame_section->owner->filename,
		 unit->dwarf_frame_section->name);
      break;

    case ALIGN8:
      complaint (&symfile_complaints, _("\
Corrupt data in %s:%s; align 8 workaround apparently succeeded"),
		 unit->dwarf_frame_section->owner->filename,
		 unit->dwarf_frame_section->name);
      break;

    default:
      complaint (&symfile_complaints,
		 _("Corrupt data in %s:%s"),
		 unit->dwarf_frame_section->owner->filename,
		 unit->dwarf_frame_section->name);
      break;
    }

  return ret;
}

static int
qsort_fde_cmp (const void *a, const void *b)
{
  struct dwarf2_fde *aa = *(struct dwarf2_fde **)a;
  struct dwarf2_fde *bb = *(struct dwarf2_fde **)b;

  if (aa->initial_location == bb->initial_location)
    {
      if (aa->address_range != bb->address_range
          && aa->eh_frame_p == 0 && bb->eh_frame_p == 0)
        /* Linker bug, e.g. gold/10400.
           Work around it by keeping stable sort order.  */
        return (a < b) ? -1 : 1;
      else
        /* Put eh_frame entries after debug_frame ones.  */
        return aa->eh_frame_p - bb->eh_frame_p;
    }

  return (aa->initial_location < bb->initial_location) ? -1 : 1;
}

void
dwarf2_build_frame_info (struct objfile *objfile)
{
  struct comp_unit *unit;
  const gdb_byte *frame_ptr;
  struct dwarf2_cie_table cie_table;
  struct dwarf2_fde_table fde_table;
  struct dwarf2_fde_table *fde_table2;

  cie_table.num_entries = 0;
  cie_table.entries = NULL;

  fde_table.num_entries = 0;
  fde_table.entries = NULL;

  /* Build a minimal decoding of the DWARF2 compilation unit.  */
  unit = (struct comp_unit *) obstack_alloc (&objfile->objfile_obstack,
					     sizeof (struct comp_unit));
  unit->abfd = objfile->obfd;
  unit->objfile = objfile;
  unit->dbase = 0;
  unit->tbase = 0;

  if (objfile->separate_debug_objfile_backlink == NULL)
    {
      /* Do not read .eh_frame from separate file as they must be also
         present in the main file.  */
      dwarf2_get_section_info (objfile, DWARF2_EH_FRAME,
                               &unit->dwarf_frame_section,
                               &unit->dwarf_frame_buffer,
                               &unit->dwarf_frame_size);
      if (unit->dwarf_frame_size)
        {
          asection *got, *txt;

          /* FIXME: kettenis/20030602: This is the DW_EH_PE_datarel base
             that is used for the i386/amd64 target, which currently is
             the only target in GCC that supports/uses the
             DW_EH_PE_datarel encoding.  */
          got = bfd_get_section_by_name (unit->abfd, ".got");
          if (got)
            unit->dbase = got->vma;

          /* GCC emits the DW_EH_PE_textrel encoding type on sh and ia64
             so far.  */
          txt = bfd_get_section_by_name (unit->abfd, ".text");
          if (txt)
            unit->tbase = txt->vma;

	  TRY
	    {
	      frame_ptr = unit->dwarf_frame_buffer;
	      while (frame_ptr < unit->dwarf_frame_buffer + unit->dwarf_frame_size)
		frame_ptr = decode_frame_entry (unit, frame_ptr, 1,
						&cie_table, &fde_table,
						EH_CIE_OR_FDE_TYPE_ID);
	    }

	  CATCH (e, RETURN_MASK_ERROR)
	    {
	      warning (_("skipping .eh_frame info of %s: %s"),
		       objfile_name (objfile), e.message);

	      if (fde_table.num_entries != 0)
		{
                  xfree (fde_table.entries);
		  fde_table.entries = NULL;
		  fde_table.num_entries = 0;
		}
	      /* The cie_table is discarded by the next if.  */
	    }
	  END_CATCH

          if (cie_table.num_entries != 0)
            {
              /* Reinit cie_table: debug_frame has different CIEs.  */
              xfree (cie_table.entries);
              cie_table.num_entries = 0;
              cie_table.entries = NULL;
            }
        }
    }

  dwarf2_get_section_info (objfile, DWARF2_DEBUG_FRAME,
                           &unit->dwarf_frame_section,
                           &unit->dwarf_frame_buffer,
                           &unit->dwarf_frame_size);
  if (unit->dwarf_frame_size)
    {
      int num_old_fde_entries = fde_table.num_entries;

      TRY
	{
	  frame_ptr = unit->dwarf_frame_buffer;
	  while (frame_ptr < unit->dwarf_frame_buffer + unit->dwarf_frame_size)
	    frame_ptr = decode_frame_entry (unit, frame_ptr, 0,
					    &cie_table, &fde_table,
					    EH_CIE_OR_FDE_TYPE_ID);
	}
      CATCH (e, RETURN_MASK_ERROR)
	{
	  warning (_("skipping .debug_frame info of %s: %s"),
		   objfile_name (objfile), e.message);

	  if (fde_table.num_entries != 0)
	    {
	      fde_table.num_entries = num_old_fde_entries;
	      if (num_old_fde_entries == 0)
		{
		  xfree (fde_table.entries);
		  fde_table.entries = NULL;
		}
	      else
		{
		  fde_table.entries
		    = XRESIZEVEC (struct dwarf2_fde *, fde_table.entries,
				  fde_table.num_entries);
		}
	    }
	  fde_table.num_entries = num_old_fde_entries;
	  /* The cie_table is discarded by the next if.  */
	}
      END_CATCH
    }

  /* Discard the cie_table, it is no longer needed.  */
  if (cie_table.num_entries != 0)
    {
      xfree (cie_table.entries);
      cie_table.entries = NULL;   /* Paranoia.  */
      cie_table.num_entries = 0;  /* Paranoia.  */
    }

  /* Copy fde_table to obstack: it is needed at runtime.  */
  fde_table2 = XOBNEW (&objfile->objfile_obstack, struct dwarf2_fde_table);

  if (fde_table.num_entries == 0)
    {
      fde_table2->entries = NULL;
      fde_table2->num_entries = 0;
    }
  else
    {
      struct dwarf2_fde *fde_prev = NULL;
      struct dwarf2_fde *first_non_zero_fde = NULL;
      int i;

      /* Prepare FDE table for lookups.  */
      qsort (fde_table.entries, fde_table.num_entries,
             sizeof (fde_table.entries[0]), qsort_fde_cmp);

      /* Check for leftovers from --gc-sections.  The GNU linker sets
	 the relevant symbols to zero, but doesn't zero the FDE *end*
	 ranges because there's no relocation there.  It's (offset,
	 length), not (start, end).  On targets where address zero is
	 just another valid address this can be a problem, since the
	 FDEs appear to be non-empty in the output --- we could pick
	 out the wrong FDE.  To work around this, when overlaps are
	 detected, we prefer FDEs that do not start at zero.

	 Start by finding the first FDE with non-zero start.  Below
	 we'll discard all FDEs that start at zero and overlap this
	 one.  */
      for (i = 0; i < fde_table.num_entries; i++)
	{
	  struct dwarf2_fde *fde = fde_table.entries[i];

	  if (fde->initial_location != 0)
	    {
	      first_non_zero_fde = fde;
	      break;
	    }
	}

      /* Since we'll be doing bsearch, squeeze out identical (except
	 for eh_frame_p) fde entries so bsearch result is predictable.
	 Also discard leftovers from --gc-sections.  */
      fde_table2->num_entries = 0;
      for (i = 0; i < fde_table.num_entries; i++)
	{
	  struct dwarf2_fde *fde = fde_table.entries[i];

	  if (fde->initial_location == 0
	      && first_non_zero_fde != NULL
	      && (first_non_zero_fde->initial_location
		  < fde->initial_location + fde->address_range))
	    continue;

	  if (fde_prev != NULL
	      && fde_prev->initial_location == fde->initial_location)
	    continue;

	  obstack_grow (&objfile->objfile_obstack, &fde_table.entries[i],
			sizeof (fde_table.entries[0]));
	  ++fde_table2->num_entries;
	  fde_prev = fde;
	}
      fde_table2->entries
	= (struct dwarf2_fde **) obstack_finish (&objfile->objfile_obstack);

      /* Discard the original fde_table.  */
      xfree (fde_table.entries);
    }

  set_objfile_data (objfile, dwarf2_frame_objfile_data, fde_table2);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_dwarf2_frame (void);

void
_initialize_dwarf2_frame (void)
{
  dwarf2_frame_data = gdbarch_data_register_pre_init (dwarf2_frame_init);
  dwarf2_frame_objfile_data = register_objfile_data ();
}
