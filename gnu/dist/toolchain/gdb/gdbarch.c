/* *INDENT-OFF* */ /* THIS FILE IS GENERATED */

/* Dynamic architecture support for GDB, the GNU debugger.
   Copyright 1998-1999, Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* This file was created with the aid of ``gdbarch.sh''.

   The bourn shell script ``gdbarch.sh'' creates the files
   ``new-gdbarch.c'' and ``new-gdbarch.h and then compares them
   against the existing ``gdbarch.[hc]''.  Any differences found
   being reported.

   If editing this file, please also run gdbarch.sh and merge any
   changes into that script. Conversely, when makeing sweeping changes
   to this file, modifying gdbarch.sh and using its output may prove
   easier. */


#include "defs.h"
#include "arch-utils.h"

#if GDB_MULTI_ARCH
#include "gdbcmd.h"
#include "inferior.h" /* enum CALL_DUMMY_LOCATION et.al. */
#else
/* Just include everything in sight so that the every old definition
   of macro is visible. */
#include "gdb_string.h"
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "breakpoint.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "gdbthread.h"
#include "annotate.h"
#include "symfile.h"		/* for overlay functions */
#endif
#include "symcat.h"


/* Static function declarations */

static void verify_gdbarch (struct gdbarch *gdbarch);
static void init_gdbarch_data (struct gdbarch *);
static void init_gdbarch_swap (struct gdbarch *);
static void swapout_gdbarch_swap (struct gdbarch *);
static void swapin_gdbarch_swap (struct gdbarch *);

/* Convenience macro for allocting typesafe memory. */

#ifndef XMALLOC
#define XMALLOC(TYPE) (TYPE*) xmalloc (sizeof (TYPE))
#endif


/* Non-zero if we want to trace architecture code.  */

#ifndef GDBARCH_DEBUG
#define GDBARCH_DEBUG 0
#endif
int gdbarch_debug = GDBARCH_DEBUG;


/* Maintain the struct gdbarch object */

struct gdbarch
{
  /* basic architectural information */
  const struct bfd_arch_info * bfd_arch_info;
  int byte_order;

  /* target specific vector. */
  struct gdbarch_tdep *tdep;

  /* per-architecture data-pointers */
  int nr_data;
  void **data;

  /* per-architecture swap-regions */
  struct gdbarch_swap *swap;

  /* Multi-arch values.

     When extending this structure you must:

     Add the field below.

     Declare set/get functions and define the corresponding
     macro in gdbarch.h.

     gdbarch_alloc(): If zero/NULL is not a suitable default,
     initialize the new field.

     verify_gdbarch(): Confirm that the target updated the field
     correctly.

     gdbarch_dump(): Add a fprintf_unfiltered call to so that the new
     field is dumped out

     ``startup_gdbarch()'': Append an initial value to the static
     variable (base values on the host's c-type system).

     get_gdbarch(): Implement the set/get functions (probably using
     the macro's as shortcuts).

     */

  int bfd_vma_bit;
  int ptr_bit;
  int short_bit;
  int int_bit;
  int long_bit;
  int long_long_bit;
  int float_bit;
  int double_bit;
  int long_double_bit;
  gdbarch_read_pc_ftype *read_pc;
  gdbarch_write_pc_ftype *write_pc;
  gdbarch_read_fp_ftype *read_fp;
  gdbarch_write_fp_ftype *write_fp;
  gdbarch_read_sp_ftype *read_sp;
  gdbarch_write_sp_ftype *write_sp;
  int num_regs;
  int sp_regnum;
  int fp_regnum;
  int pc_regnum;
  gdbarch_register_name_ftype *register_name;
  int register_size;
  int register_bytes;
  gdbarch_register_byte_ftype *register_byte;
  gdbarch_register_raw_size_ftype *register_raw_size;
  int max_register_raw_size;
  gdbarch_register_virtual_size_ftype *register_virtual_size;
  int max_register_virtual_size;
  gdbarch_register_virtual_type_ftype *register_virtual_type;
  int use_generic_dummy_frames;
  int call_dummy_location;
  gdbarch_call_dummy_address_ftype *call_dummy_address;
  CORE_ADDR call_dummy_start_offset;
  CORE_ADDR call_dummy_breakpoint_offset;
  int call_dummy_breakpoint_offset_p;
  int call_dummy_length;
  gdbarch_pc_in_call_dummy_ftype *pc_in_call_dummy;
  int call_dummy_p;
  LONGEST * call_dummy_words;
  int sizeof_call_dummy_words;
  int call_dummy_stack_adjust_p;
  int call_dummy_stack_adjust;
  gdbarch_fix_call_dummy_ftype *fix_call_dummy;
  int believe_pcc_promotion;
  int believe_pcc_promotion_type;
  gdbarch_coerce_float_to_double_ftype *coerce_float_to_double;
  gdbarch_get_saved_register_ftype *get_saved_register;
  gdbarch_register_convertible_ftype *register_convertible;
  gdbarch_register_convert_to_virtual_ftype *register_convert_to_virtual;
  gdbarch_register_convert_to_raw_ftype *register_convert_to_raw;
  gdbarch_extract_return_value_ftype *extract_return_value;
  gdbarch_push_arguments_ftype *push_arguments;
  gdbarch_push_dummy_frame_ftype *push_dummy_frame;
  gdbarch_push_return_address_ftype *push_return_address;
  gdbarch_pop_frame_ftype *pop_frame;
  gdbarch_d10v_make_daddr_ftype *d10v_make_daddr;
  gdbarch_d10v_make_iaddr_ftype *d10v_make_iaddr;
  gdbarch_d10v_daddr_p_ftype *d10v_daddr_p;
  gdbarch_d10v_iaddr_p_ftype *d10v_iaddr_p;
  gdbarch_d10v_convert_daddr_to_raw_ftype *d10v_convert_daddr_to_raw;
  gdbarch_d10v_convert_iaddr_to_raw_ftype *d10v_convert_iaddr_to_raw;
  gdbarch_store_struct_return_ftype *store_struct_return;
  gdbarch_store_return_value_ftype *store_return_value;
  gdbarch_extract_struct_value_address_ftype *extract_struct_value_address;
  gdbarch_use_struct_convention_ftype *use_struct_convention;
  gdbarch_frame_init_saved_regs_ftype *frame_init_saved_regs;
  gdbarch_init_extra_frame_info_ftype *init_extra_frame_info;
  gdbarch_skip_prologue_ftype *skip_prologue;
  gdbarch_inner_than_ftype *inner_than;
  gdbarch_breakpoint_from_pc_ftype *breakpoint_from_pc;
  gdbarch_memory_insert_breakpoint_ftype *memory_insert_breakpoint;
  gdbarch_memory_remove_breakpoint_ftype *memory_remove_breakpoint;
  CORE_ADDR decr_pc_after_break;
  CORE_ADDR function_start_offset;
  gdbarch_remote_translate_xfer_address_ftype *remote_translate_xfer_address;
  CORE_ADDR frame_args_skip;
  gdbarch_frameless_function_invocation_ftype *frameless_function_invocation;
  gdbarch_frame_chain_ftype *frame_chain;
  gdbarch_frame_chain_valid_ftype *frame_chain_valid;
  gdbarch_frame_saved_pc_ftype *frame_saved_pc;
  gdbarch_frame_args_address_ftype *frame_args_address;
  gdbarch_frame_locals_address_ftype *frame_locals_address;
  gdbarch_saved_pc_after_call_ftype *saved_pc_after_call;
  gdbarch_frame_num_args_ftype *frame_num_args;
};


/* The default architecture uses host values (for want of a better
   choice). */

extern const struct bfd_arch_info bfd_default_arch_struct;

struct gdbarch startup_gdbarch = {
  /* basic architecture information */
  &bfd_default_arch_struct,
  BIG_ENDIAN,
  /* target specific vector */
  NULL,
  /*per-architecture data-pointers and swap regions */
  0, NULL, NULL,
  /* Multi-arch values */
  8 * sizeof (void*),
  8 * sizeof (void*),
  8 * sizeof (short),
  8 * sizeof (int),
  8 * sizeof (long),
  8 * sizeof (LONGEST),
  8 * sizeof (float),
  8 * sizeof (double),
  8 * sizeof (long double),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  generic_get_saved_register,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  /* startup_gdbarch() */
};
struct gdbarch *current_gdbarch = &startup_gdbarch;


/* Create a new ``struct gdbarch'' based in information provided by
   ``struct gdbarch_info''. */

struct gdbarch *
gdbarch_alloc (const struct gdbarch_info *info,
               struct gdbarch_tdep *tdep)
{
  struct gdbarch *gdbarch = XMALLOC (struct gdbarch);
  memset (gdbarch, 0, sizeof (*gdbarch));

  gdbarch->tdep = tdep;

  gdbarch->bfd_arch_info = info->bfd_arch_info;
  gdbarch->byte_order = info->byte_order;

  /* Force the explicit initialization of these. */
  gdbarch->bfd_vma_bit = TARGET_ARCHITECTURE->bits_per_address;
  gdbarch->num_regs = -1;
  gdbarch->sp_regnum = -1;
  gdbarch->fp_regnum = -1;
  gdbarch->pc_regnum = -1;
  gdbarch->register_name = legacy_register_name;
  gdbarch->register_size = -1;
  gdbarch->register_bytes = -1;
  gdbarch->max_register_raw_size = -1;
  gdbarch->max_register_virtual_size = -1;
  gdbarch->use_generic_dummy_frames = -1;
  gdbarch->call_dummy_start_offset = -1;
  gdbarch->call_dummy_breakpoint_offset = -1;
  gdbarch->call_dummy_breakpoint_offset_p = -1;
  gdbarch->call_dummy_length = -1;
  gdbarch->call_dummy_p = -1;
  gdbarch->call_dummy_words = legacy_call_dummy_words;
  gdbarch->sizeof_call_dummy_words = legacy_sizeof_call_dummy_words;
  gdbarch->call_dummy_stack_adjust_p = -1;
  gdbarch->coerce_float_to_double = default_coerce_float_to_double;
  gdbarch->register_convertible = generic_register_convertible_not;
  gdbarch->breakpoint_from_pc = legacy_breakpoint_from_pc;
  gdbarch->memory_insert_breakpoint = default_memory_insert_breakpoint;
  gdbarch->memory_remove_breakpoint = default_memory_remove_breakpoint;
  gdbarch->decr_pc_after_break = -1;
  gdbarch->function_start_offset = -1;
  gdbarch->remote_translate_xfer_address = generic_remote_translate_xfer_address;
  gdbarch->frame_args_skip = -1;
  gdbarch->frameless_function_invocation = generic_frameless_function_invocation_not;
  /* gdbarch_alloc() */

  return gdbarch;
}


/* Free a gdbarch struct.  This should never happen in normal
   operation --- once you've created a gdbarch, you keep it around.
   However, if an architecture's init function encounters an error
   building the structure, it may need to clean up a partially
   constructed gdbarch.  */
void
gdbarch_free (struct gdbarch *arch)
{
  /* At the moment, this is trivial.  */
  free (arch);
}


/* Ensure that all values in a GDBARCH are reasonable. */

static void
verify_gdbarch (struct gdbarch *gdbarch)
{
  /* Only perform sanity checks on a multi-arch target. */
  if (GDB_MULTI_ARCH <= 0)
    return;
  /* fundamental */
  if (gdbarch->byte_order == 0)
    internal_error ("verify_gdbarch: byte-order unset");
  if (gdbarch->bfd_arch_info == NULL)
    internal_error ("verify_gdbarch: bfd_arch_info unset");
  /* Check those that need to be defined for the given multi-arch level. */
  /* Skip verify of bfd_vma_bit, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->ptr_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: ptr_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->short_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: short_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->int_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: int_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->long_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: long_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->long_long_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: long_long_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->float_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: float_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->double_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: double_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->long_double_bit == 0))
    internal_error ("gdbarch: verify_gdbarch: long_double_bit invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->read_pc == 0))
    internal_error ("gdbarch: verify_gdbarch: read_pc invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->write_pc == 0))
    internal_error ("gdbarch: verify_gdbarch: write_pc invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->read_fp == 0))
    internal_error ("gdbarch: verify_gdbarch: read_fp invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->write_fp == 0))
    internal_error ("gdbarch: verify_gdbarch: write_fp invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->read_sp == 0))
    internal_error ("gdbarch: verify_gdbarch: read_sp invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->write_sp == 0))
    internal_error ("gdbarch: verify_gdbarch: write_sp invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->num_regs == -1))
    internal_error ("gdbarch: verify_gdbarch: num_regs invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->sp_regnum == -1))
    internal_error ("gdbarch: verify_gdbarch: sp_regnum invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->fp_regnum == -1))
    internal_error ("gdbarch: verify_gdbarch: fp_regnum invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->pc_regnum == -1))
    internal_error ("gdbarch: verify_gdbarch: pc_regnum invalid");
  /* Skip verify of register_name, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_size == -1))
    internal_error ("gdbarch: verify_gdbarch: register_size invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_bytes == -1))
    internal_error ("gdbarch: verify_gdbarch: register_bytes invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_byte == 0))
    internal_error ("gdbarch: verify_gdbarch: register_byte invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_raw_size == 0))
    internal_error ("gdbarch: verify_gdbarch: register_raw_size invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->max_register_raw_size == -1))
    internal_error ("gdbarch: verify_gdbarch: max_register_raw_size invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_virtual_size == 0))
    internal_error ("gdbarch: verify_gdbarch: register_virtual_size invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->max_register_virtual_size == -1))
    internal_error ("gdbarch: verify_gdbarch: max_register_virtual_size invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->register_virtual_type == 0))
    internal_error ("gdbarch: verify_gdbarch: register_virtual_type invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->use_generic_dummy_frames == -1))
    internal_error ("gdbarch: verify_gdbarch: use_generic_dummy_frames invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_location == 0))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_location invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_location == AT_ENTRY_POINT && gdbarch->call_dummy_address == 0))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_address invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_start_offset == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_start_offset invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_breakpoint_offset == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_breakpoint_offset invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->call_dummy_breakpoint_offset_p == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_breakpoint_offset_p invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_length == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_length invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->pc_in_call_dummy == 0))
    internal_error ("gdbarch: verify_gdbarch: pc_in_call_dummy invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->call_dummy_p == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_p invalid");
  /* Skip verify of call_dummy_words, invalid_p == 0 */
  /* Skip verify of sizeof_call_dummy_words, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->call_dummy_stack_adjust_p == -1))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_stack_adjust_p invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->call_dummy_stack_adjust_p && gdbarch->call_dummy_stack_adjust == 0))
    internal_error ("gdbarch: verify_gdbarch: call_dummy_stack_adjust invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->fix_call_dummy == 0))
    internal_error ("gdbarch: verify_gdbarch: fix_call_dummy invalid");
  /* Skip verify of coerce_float_to_double, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->get_saved_register == 0))
    internal_error ("gdbarch: verify_gdbarch: get_saved_register invalid");
  /* Skip verify of register_convertible, invalid_p == 0 */
  /* Skip verify of register_convert_to_virtual, invalid_p == 0 */
  /* Skip verify of register_convert_to_raw, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->extract_return_value == 0))
    internal_error ("gdbarch: verify_gdbarch: extract_return_value invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->push_arguments == 0))
    internal_error ("gdbarch: verify_gdbarch: push_arguments invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->push_dummy_frame == 0))
    internal_error ("gdbarch: verify_gdbarch: push_dummy_frame invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->push_return_address == 0))
    internal_error ("gdbarch: verify_gdbarch: push_return_address invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->pop_frame == 0))
    internal_error ("gdbarch: verify_gdbarch: pop_frame invalid");
  /* Skip verify of d10v_make_daddr, invalid_p == 0 */
  /* Skip verify of d10v_make_iaddr, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->d10v_daddr_p == 0))
    internal_error ("gdbarch: verify_gdbarch: d10v_daddr_p invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->d10v_iaddr_p == 0))
    internal_error ("gdbarch: verify_gdbarch: d10v_iaddr_p invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->d10v_convert_daddr_to_raw == 0))
    internal_error ("gdbarch: verify_gdbarch: d10v_convert_daddr_to_raw invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->d10v_convert_iaddr_to_raw == 0))
    internal_error ("gdbarch: verify_gdbarch: d10v_convert_iaddr_to_raw invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->store_struct_return == 0))
    internal_error ("gdbarch: verify_gdbarch: store_struct_return invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->store_return_value == 0))
    internal_error ("gdbarch: verify_gdbarch: store_return_value invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->extract_struct_value_address == 0))
    internal_error ("gdbarch: verify_gdbarch: extract_struct_value_address invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->use_struct_convention == 0))
    internal_error ("gdbarch: verify_gdbarch: use_struct_convention invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_init_saved_regs == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_init_saved_regs invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->init_extra_frame_info == 0))
    internal_error ("gdbarch: verify_gdbarch: init_extra_frame_info invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->skip_prologue == 0))
    internal_error ("gdbarch: verify_gdbarch: skip_prologue invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->inner_than == 0))
    internal_error ("gdbarch: verify_gdbarch: inner_than invalid");
  /* Skip verify of breakpoint_from_pc, invalid_p == 0 */
  /* Skip verify of memory_insert_breakpoint, invalid_p == 0 */
  /* Skip verify of memory_remove_breakpoint, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->decr_pc_after_break == -1))
    internal_error ("gdbarch: verify_gdbarch: decr_pc_after_break invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->function_start_offset == -1))
    internal_error ("gdbarch: verify_gdbarch: function_start_offset invalid");
  /* Skip verify of remote_translate_xfer_address, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_args_skip == -1))
    internal_error ("gdbarch: verify_gdbarch: frame_args_skip invalid");
  /* Skip verify of frameless_function_invocation, invalid_p == 0 */
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_chain == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_chain invalid");
  if ((GDB_MULTI_ARCH >= 1)
      && (gdbarch->frame_chain_valid == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_chain_valid invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_saved_pc == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_saved_pc invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_args_address == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_args_address invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_locals_address == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_locals_address invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->saved_pc_after_call == 0))
    internal_error ("gdbarch: verify_gdbarch: saved_pc_after_call invalid");
  if ((GDB_MULTI_ARCH >= 2)
      && (gdbarch->frame_num_args == 0))
    internal_error ("gdbarch: verify_gdbarch: frame_num_args invalid");
}


/* Print out the details of the current architecture. */

void
gdbarch_dump (void)
{
  if (TARGET_ARCHITECTURE != NULL)
    fprintf_unfiltered (gdb_stdlog,
                        "gdbarch_update: TARGET_ARCHITECTURE = %s\n",
                        TARGET_ARCHITECTURE->printable_name);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_BYTE_ORDER = %ld\n",
                      (long) TARGET_BYTE_ORDER);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_BFD_VMA_BIT = %ld\n",
                      (long) TARGET_BFD_VMA_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_PTR_BIT = %ld\n",
                      (long) TARGET_PTR_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_SHORT_BIT = %ld\n",
                      (long) TARGET_SHORT_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_INT_BIT = %ld\n",
                      (long) TARGET_INT_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_LONG_BIT = %ld\n",
                      (long) TARGET_LONG_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_LONG_LONG_BIT = %ld\n",
                      (long) TARGET_LONG_LONG_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_FLOAT_BIT = %ld\n",
                      (long) TARGET_FLOAT_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_DOUBLE_BIT = %ld\n",
                      (long) TARGET_DOUBLE_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_LONG_DOUBLE_BIT = %ld\n",
                      (long) TARGET_LONG_DOUBLE_BIT);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_READ_PC = 0x%08lx\n",
                      (long) current_gdbarch->read_pc
                      /*TARGET_READ_PC ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_WRITE_PC = 0x%08lx\n",
                      (long) current_gdbarch->write_pc
                      /*TARGET_WRITE_PC ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_READ_FP = 0x%08lx\n",
                      (long) current_gdbarch->read_fp
                      /*TARGET_READ_FP ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_WRITE_FP = 0x%08lx\n",
                      (long) current_gdbarch->write_fp
                      /*TARGET_WRITE_FP ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_READ_SP = 0x%08lx\n",
                      (long) current_gdbarch->read_sp
                      /*TARGET_READ_SP ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: TARGET_WRITE_SP = 0x%08lx\n",
                      (long) current_gdbarch->write_sp
                      /*TARGET_WRITE_SP ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: NUM_REGS = %ld\n",
                      (long) NUM_REGS);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: SP_REGNUM = %ld\n",
                      (long) SP_REGNUM);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FP_REGNUM = %ld\n",
                      (long) FP_REGNUM);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: PC_REGNUM = %ld\n",
                      (long) PC_REGNUM);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_NAME = 0x%08lx\n",
                      (long) current_gdbarch->register_name
                      /*REGISTER_NAME ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_SIZE = %ld\n",
                      (long) REGISTER_SIZE);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_BYTES = %ld\n",
                      (long) REGISTER_BYTES);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_BYTE = 0x%08lx\n",
                      (long) current_gdbarch->register_byte
                      /*REGISTER_BYTE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_RAW_SIZE = 0x%08lx\n",
                      (long) current_gdbarch->register_raw_size
                      /*REGISTER_RAW_SIZE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: MAX_REGISTER_RAW_SIZE = %ld\n",
                      (long) MAX_REGISTER_RAW_SIZE);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_VIRTUAL_SIZE = 0x%08lx\n",
                      (long) current_gdbarch->register_virtual_size
                      /*REGISTER_VIRTUAL_SIZE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: MAX_REGISTER_VIRTUAL_SIZE = %ld\n",
                      (long) MAX_REGISTER_VIRTUAL_SIZE);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_VIRTUAL_TYPE = 0x%08lx\n",
                      (long) current_gdbarch->register_virtual_type
                      /*REGISTER_VIRTUAL_TYPE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: USE_GENERIC_DUMMY_FRAMES = %ld\n",
                      (long) USE_GENERIC_DUMMY_FRAMES);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_LOCATION = %ld\n",
                      (long) CALL_DUMMY_LOCATION);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->call_dummy_address
                      /*CALL_DUMMY_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_START_OFFSET = 0x%08lx\n",
                      (long) CALL_DUMMY_START_OFFSET);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_BREAKPOINT_OFFSET = 0x%08lx\n",
                      (long) CALL_DUMMY_BREAKPOINT_OFFSET);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_BREAKPOINT_OFFSET_P = %ld\n",
                      (long) CALL_DUMMY_BREAKPOINT_OFFSET_P);
  if (CALL_DUMMY_LOCATION == BEFORE_TEXT_END || CALL_DUMMY_LOCATION == AFTER_TEXT_END)
    fprintf_unfiltered (gdb_stdlog,
                        "gdbarch_update: CALL_DUMMY_LENGTH = %ld\n",
                        (long) CALL_DUMMY_LENGTH);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: PC_IN_CALL_DUMMY = 0x%08lx\n",
                      (long) current_gdbarch->pc_in_call_dummy
                      /*PC_IN_CALL_DUMMY ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_P = %ld\n",
                      (long) CALL_DUMMY_P);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_WORDS = 0x%08lx\n",
                      (long) CALL_DUMMY_WORDS);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: SIZEOF_CALL_DUMMY_WORDS = 0x%08lx\n",
                      (long) SIZEOF_CALL_DUMMY_WORDS);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: CALL_DUMMY_STACK_ADJUST_P = 0x%08lx\n",
                      (long) CALL_DUMMY_STACK_ADJUST_P);
  if (CALL_DUMMY_STACK_ADJUST_P)
    fprintf_unfiltered (gdb_stdlog,
                        "gdbarch_update: CALL_DUMMY_STACK_ADJUST = 0x%08lx\n",
                        (long) CALL_DUMMY_STACK_ADJUST);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FIX_CALL_DUMMY = 0x%08lx\n",
                      (long) current_gdbarch->fix_call_dummy
                      /*FIX_CALL_DUMMY ()*/);
#ifdef BELIEVE_PCC_PROMOTION
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: BELIEVE_PCC_PROMOTION = %ld\n",
                      (long) BELIEVE_PCC_PROMOTION);
#endif
#ifdef BELIEVE_PCC_PROMOTION_TYPE
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: BELIEVE_PCC_PROMOTION_TYPE = %ld\n",
                      (long) BELIEVE_PCC_PROMOTION_TYPE);
#endif
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: COERCE_FLOAT_TO_DOUBLE = 0x%08lx\n",
                      (long) current_gdbarch->coerce_float_to_double
                      /*COERCE_FLOAT_TO_DOUBLE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: GET_SAVED_REGISTER = 0x%08lx\n",
                      (long) current_gdbarch->get_saved_register
                      /*GET_SAVED_REGISTER ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_CONVERTIBLE = 0x%08lx\n",
                      (long) current_gdbarch->register_convertible
                      /*REGISTER_CONVERTIBLE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_CONVERT_TO_VIRTUAL = 0x%08lx\n",
                      (long) current_gdbarch->register_convert_to_virtual
                      /*REGISTER_CONVERT_TO_VIRTUAL ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REGISTER_CONVERT_TO_RAW = 0x%08lx\n",
                      (long) current_gdbarch->register_convert_to_raw
                      /*REGISTER_CONVERT_TO_RAW ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: EXTRACT_RETURN_VALUE = 0x%08lx\n",
                      (long) current_gdbarch->extract_return_value
                      /*EXTRACT_RETURN_VALUE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: PUSH_ARGUMENTS = 0x%08lx\n",
                      (long) current_gdbarch->push_arguments
                      /*PUSH_ARGUMENTS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: PUSH_DUMMY_FRAME = 0x%08lx\n",
                      (long) current_gdbarch->push_dummy_frame
                      /*PUSH_DUMMY_FRAME ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: PUSH_RETURN_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->push_return_address
                      /*PUSH_RETURN_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: POP_FRAME = 0x%08lx\n",
                      (long) current_gdbarch->pop_frame
                      /*POP_FRAME ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_MAKE_DADDR = 0x%08lx\n",
                      (long) current_gdbarch->d10v_make_daddr
                      /*D10V_MAKE_DADDR ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_MAKE_IADDR = 0x%08lx\n",
                      (long) current_gdbarch->d10v_make_iaddr
                      /*D10V_MAKE_IADDR ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_DADDR_P = 0x%08lx\n",
                      (long) current_gdbarch->d10v_daddr_p
                      /*D10V_DADDR_P ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_IADDR_P = 0x%08lx\n",
                      (long) current_gdbarch->d10v_iaddr_p
                      /*D10V_IADDR_P ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_CONVERT_DADDR_TO_RAW = 0x%08lx\n",
                      (long) current_gdbarch->d10v_convert_daddr_to_raw
                      /*D10V_CONVERT_DADDR_TO_RAW ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: D10V_CONVERT_IADDR_TO_RAW = 0x%08lx\n",
                      (long) current_gdbarch->d10v_convert_iaddr_to_raw
                      /*D10V_CONVERT_IADDR_TO_RAW ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: STORE_STRUCT_RETURN = 0x%08lx\n",
                      (long) current_gdbarch->store_struct_return
                      /*STORE_STRUCT_RETURN ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: STORE_RETURN_VALUE = 0x%08lx\n",
                      (long) current_gdbarch->store_return_value
                      /*STORE_RETURN_VALUE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: EXTRACT_STRUCT_VALUE_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->extract_struct_value_address
                      /*EXTRACT_STRUCT_VALUE_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: USE_STRUCT_CONVENTION = 0x%08lx\n",
                      (long) current_gdbarch->use_struct_convention
                      /*USE_STRUCT_CONVENTION ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_INIT_SAVED_REGS = 0x%08lx\n",
                      (long) current_gdbarch->frame_init_saved_regs
                      /*FRAME_INIT_SAVED_REGS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: INIT_EXTRA_FRAME_INFO = 0x%08lx\n",
                      (long) current_gdbarch->init_extra_frame_info
                      /*INIT_EXTRA_FRAME_INFO ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: SKIP_PROLOGUE = 0x%08lx\n",
                      (long) current_gdbarch->skip_prologue
                      /*SKIP_PROLOGUE ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: INNER_THAN = 0x%08lx\n",
                      (long) current_gdbarch->inner_than
                      /*INNER_THAN ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: BREAKPOINT_FROM_PC = 0x%08lx\n",
                      (long) current_gdbarch->breakpoint_from_pc
                      /*BREAKPOINT_FROM_PC ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: MEMORY_INSERT_BREAKPOINT = 0x%08lx\n",
                      (long) current_gdbarch->memory_insert_breakpoint
                      /*MEMORY_INSERT_BREAKPOINT ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: MEMORY_REMOVE_BREAKPOINT = 0x%08lx\n",
                      (long) current_gdbarch->memory_remove_breakpoint
                      /*MEMORY_REMOVE_BREAKPOINT ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: DECR_PC_AFTER_BREAK = %ld\n",
                      (long) DECR_PC_AFTER_BREAK);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FUNCTION_START_OFFSET = %ld\n",
                      (long) FUNCTION_START_OFFSET);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: REMOTE_TRANSLATE_XFER_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->remote_translate_xfer_address
                      /*REMOTE_TRANSLATE_XFER_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_ARGS_SKIP = %ld\n",
                      (long) FRAME_ARGS_SKIP);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAMELESS_FUNCTION_INVOCATION = 0x%08lx\n",
                      (long) current_gdbarch->frameless_function_invocation
                      /*FRAMELESS_FUNCTION_INVOCATION ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_CHAIN = 0x%08lx\n",
                      (long) current_gdbarch->frame_chain
                      /*FRAME_CHAIN ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_CHAIN_VALID = 0x%08lx\n",
                      (long) current_gdbarch->frame_chain_valid
                      /*FRAME_CHAIN_VALID ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_SAVED_PC = 0x%08lx\n",
                      (long) current_gdbarch->frame_saved_pc
                      /*FRAME_SAVED_PC ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_ARGS_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->frame_args_address
                      /*FRAME_ARGS_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_LOCALS_ADDRESS = 0x%08lx\n",
                      (long) current_gdbarch->frame_locals_address
                      /*FRAME_LOCALS_ADDRESS ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: SAVED_PC_AFTER_CALL = 0x%08lx\n",
                      (long) current_gdbarch->saved_pc_after_call
                      /*SAVED_PC_AFTER_CALL ()*/);
  fprintf_unfiltered (gdb_stdlog,
                      "gdbarch_update: FRAME_NUM_ARGS = 0x%08lx\n",
                      (long) current_gdbarch->frame_num_args
                      /*FRAME_NUM_ARGS ()*/);
}

struct gdbarch_tdep *
gdbarch_tdep (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_tdep called\n");
  return gdbarch->tdep;
}


const struct bfd_arch_info *
gdbarch_bfd_arch_info (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_bfd_arch_info called\n");
  return gdbarch->bfd_arch_info;
}

int
gdbarch_byte_order (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_byte_order called\n");
  return gdbarch->byte_order;
}

int
gdbarch_bfd_vma_bit (struct gdbarch *gdbarch)
{
  /* Skip verify of bfd_vma_bit, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_bfd_vma_bit called\n");
  return gdbarch->bfd_vma_bit;
}

void
set_gdbarch_bfd_vma_bit (struct gdbarch *gdbarch,
                         int bfd_vma_bit)
{
  gdbarch->bfd_vma_bit = bfd_vma_bit;
}

int
gdbarch_ptr_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->ptr_bit == 0)
    internal_error ("gdbarch: gdbarch_ptr_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ptr_bit called\n");
  return gdbarch->ptr_bit;
}

void
set_gdbarch_ptr_bit (struct gdbarch *gdbarch,
                     int ptr_bit)
{
  gdbarch->ptr_bit = ptr_bit;
}

int
gdbarch_short_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->short_bit == 0)
    internal_error ("gdbarch: gdbarch_short_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_short_bit called\n");
  return gdbarch->short_bit;
}

void
set_gdbarch_short_bit (struct gdbarch *gdbarch,
                       int short_bit)
{
  gdbarch->short_bit = short_bit;
}

int
gdbarch_int_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->int_bit == 0)
    internal_error ("gdbarch: gdbarch_int_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_int_bit called\n");
  return gdbarch->int_bit;
}

void
set_gdbarch_int_bit (struct gdbarch *gdbarch,
                     int int_bit)
{
  gdbarch->int_bit = int_bit;
}

int
gdbarch_long_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->long_bit == 0)
    internal_error ("gdbarch: gdbarch_long_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_long_bit called\n");
  return gdbarch->long_bit;
}

void
set_gdbarch_long_bit (struct gdbarch *gdbarch,
                      int long_bit)
{
  gdbarch->long_bit = long_bit;
}

int
gdbarch_long_long_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->long_long_bit == 0)
    internal_error ("gdbarch: gdbarch_long_long_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_long_long_bit called\n");
  return gdbarch->long_long_bit;
}

void
set_gdbarch_long_long_bit (struct gdbarch *gdbarch,
                           int long_long_bit)
{
  gdbarch->long_long_bit = long_long_bit;
}

int
gdbarch_float_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->float_bit == 0)
    internal_error ("gdbarch: gdbarch_float_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_float_bit called\n");
  return gdbarch->float_bit;
}

void
set_gdbarch_float_bit (struct gdbarch *gdbarch,
                       int float_bit)
{
  gdbarch->float_bit = float_bit;
}

int
gdbarch_double_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->double_bit == 0)
    internal_error ("gdbarch: gdbarch_double_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_double_bit called\n");
  return gdbarch->double_bit;
}

void
set_gdbarch_double_bit (struct gdbarch *gdbarch,
                        int double_bit)
{
  gdbarch->double_bit = double_bit;
}

int
gdbarch_long_double_bit (struct gdbarch *gdbarch)
{
  if (gdbarch->long_double_bit == 0)
    internal_error ("gdbarch: gdbarch_long_double_bit invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_long_double_bit called\n");
  return gdbarch->long_double_bit;
}

void
set_gdbarch_long_double_bit (struct gdbarch *gdbarch,
                             int long_double_bit)
{
  gdbarch->long_double_bit = long_double_bit;
}

CORE_ADDR
gdbarch_read_pc (struct gdbarch *gdbarch, int pid)
{
  if (gdbarch->read_pc == 0)
    internal_error ("gdbarch: gdbarch_read_pc invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_read_pc called\n");
  return gdbarch->read_pc (pid);
}

void
set_gdbarch_read_pc (struct gdbarch *gdbarch,
                     gdbarch_read_pc_ftype read_pc)
{
  gdbarch->read_pc = read_pc;
}

void
gdbarch_write_pc (struct gdbarch *gdbarch, CORE_ADDR val, int pid)
{
  if (gdbarch->write_pc == 0)
    internal_error ("gdbarch: gdbarch_write_pc invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_write_pc called\n");
  gdbarch->write_pc (val, pid);
}

void
set_gdbarch_write_pc (struct gdbarch *gdbarch,
                      gdbarch_write_pc_ftype write_pc)
{
  gdbarch->write_pc = write_pc;
}

CORE_ADDR
gdbarch_read_fp (struct gdbarch *gdbarch)
{
  if (gdbarch->read_fp == 0)
    internal_error ("gdbarch: gdbarch_read_fp invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_read_fp called\n");
  return gdbarch->read_fp ();
}

void
set_gdbarch_read_fp (struct gdbarch *gdbarch,
                     gdbarch_read_fp_ftype read_fp)
{
  gdbarch->read_fp = read_fp;
}

void
gdbarch_write_fp (struct gdbarch *gdbarch, CORE_ADDR val)
{
  if (gdbarch->write_fp == 0)
    internal_error ("gdbarch: gdbarch_write_fp invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_write_fp called\n");
  gdbarch->write_fp (val);
}

void
set_gdbarch_write_fp (struct gdbarch *gdbarch,
                      gdbarch_write_fp_ftype write_fp)
{
  gdbarch->write_fp = write_fp;
}

CORE_ADDR
gdbarch_read_sp (struct gdbarch *gdbarch)
{
  if (gdbarch->read_sp == 0)
    internal_error ("gdbarch: gdbarch_read_sp invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_read_sp called\n");
  return gdbarch->read_sp ();
}

void
set_gdbarch_read_sp (struct gdbarch *gdbarch,
                     gdbarch_read_sp_ftype read_sp)
{
  gdbarch->read_sp = read_sp;
}

void
gdbarch_write_sp (struct gdbarch *gdbarch, CORE_ADDR val)
{
  if (gdbarch->write_sp == 0)
    internal_error ("gdbarch: gdbarch_write_sp invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_write_sp called\n");
  gdbarch->write_sp (val);
}

void
set_gdbarch_write_sp (struct gdbarch *gdbarch,
                      gdbarch_write_sp_ftype write_sp)
{
  gdbarch->write_sp = write_sp;
}

int
gdbarch_num_regs (struct gdbarch *gdbarch)
{
  if (gdbarch->num_regs == -1)
    internal_error ("gdbarch: gdbarch_num_regs invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_num_regs called\n");
  return gdbarch->num_regs;
}

void
set_gdbarch_num_regs (struct gdbarch *gdbarch,
                      int num_regs)
{
  gdbarch->num_regs = num_regs;
}

int
gdbarch_sp_regnum (struct gdbarch *gdbarch)
{
  if (gdbarch->sp_regnum == -1)
    internal_error ("gdbarch: gdbarch_sp_regnum invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_sp_regnum called\n");
  return gdbarch->sp_regnum;
}

void
set_gdbarch_sp_regnum (struct gdbarch *gdbarch,
                       int sp_regnum)
{
  gdbarch->sp_regnum = sp_regnum;
}

int
gdbarch_fp_regnum (struct gdbarch *gdbarch)
{
  if (gdbarch->fp_regnum == -1)
    internal_error ("gdbarch: gdbarch_fp_regnum invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fp_regnum called\n");
  return gdbarch->fp_regnum;
}

void
set_gdbarch_fp_regnum (struct gdbarch *gdbarch,
                       int fp_regnum)
{
  gdbarch->fp_regnum = fp_regnum;
}

int
gdbarch_pc_regnum (struct gdbarch *gdbarch)
{
  if (gdbarch->pc_regnum == -1)
    internal_error ("gdbarch: gdbarch_pc_regnum invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pc_regnum called\n");
  return gdbarch->pc_regnum;
}

void
set_gdbarch_pc_regnum (struct gdbarch *gdbarch,
                       int pc_regnum)
{
  gdbarch->pc_regnum = pc_regnum;
}

char *
gdbarch_register_name (struct gdbarch *gdbarch, int regnr)
{
  if (gdbarch->register_name == 0)
    internal_error ("gdbarch: gdbarch_register_name invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_name called\n");
  return gdbarch->register_name (regnr);
}

void
set_gdbarch_register_name (struct gdbarch *gdbarch,
                           gdbarch_register_name_ftype register_name)
{
  gdbarch->register_name = register_name;
}

int
gdbarch_register_size (struct gdbarch *gdbarch)
{
  if (gdbarch->register_size == -1)
    internal_error ("gdbarch: gdbarch_register_size invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_size called\n");
  return gdbarch->register_size;
}

void
set_gdbarch_register_size (struct gdbarch *gdbarch,
                           int register_size)
{
  gdbarch->register_size = register_size;
}

int
gdbarch_register_bytes (struct gdbarch *gdbarch)
{
  if (gdbarch->register_bytes == -1)
    internal_error ("gdbarch: gdbarch_register_bytes invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_bytes called\n");
  return gdbarch->register_bytes;
}

void
set_gdbarch_register_bytes (struct gdbarch *gdbarch,
                            int register_bytes)
{
  gdbarch->register_bytes = register_bytes;
}

int
gdbarch_register_byte (struct gdbarch *gdbarch, int reg_nr)
{
  if (gdbarch->register_byte == 0)
    internal_error ("gdbarch: gdbarch_register_byte invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_byte called\n");
  return gdbarch->register_byte (reg_nr);
}

void
set_gdbarch_register_byte (struct gdbarch *gdbarch,
                           gdbarch_register_byte_ftype register_byte)
{
  gdbarch->register_byte = register_byte;
}

int
gdbarch_register_raw_size (struct gdbarch *gdbarch, int reg_nr)
{
  if (gdbarch->register_raw_size == 0)
    internal_error ("gdbarch: gdbarch_register_raw_size invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_raw_size called\n");
  return gdbarch->register_raw_size (reg_nr);
}

void
set_gdbarch_register_raw_size (struct gdbarch *gdbarch,
                               gdbarch_register_raw_size_ftype register_raw_size)
{
  gdbarch->register_raw_size = register_raw_size;
}

int
gdbarch_max_register_raw_size (struct gdbarch *gdbarch)
{
  if (gdbarch->max_register_raw_size == -1)
    internal_error ("gdbarch: gdbarch_max_register_raw_size invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_max_register_raw_size called\n");
  return gdbarch->max_register_raw_size;
}

void
set_gdbarch_max_register_raw_size (struct gdbarch *gdbarch,
                                   int max_register_raw_size)
{
  gdbarch->max_register_raw_size = max_register_raw_size;
}

int
gdbarch_register_virtual_size (struct gdbarch *gdbarch, int reg_nr)
{
  if (gdbarch->register_virtual_size == 0)
    internal_error ("gdbarch: gdbarch_register_virtual_size invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_virtual_size called\n");
  return gdbarch->register_virtual_size (reg_nr);
}

void
set_gdbarch_register_virtual_size (struct gdbarch *gdbarch,
                                   gdbarch_register_virtual_size_ftype register_virtual_size)
{
  gdbarch->register_virtual_size = register_virtual_size;
}

int
gdbarch_max_register_virtual_size (struct gdbarch *gdbarch)
{
  if (gdbarch->max_register_virtual_size == -1)
    internal_error ("gdbarch: gdbarch_max_register_virtual_size invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_max_register_virtual_size called\n");
  return gdbarch->max_register_virtual_size;
}

void
set_gdbarch_max_register_virtual_size (struct gdbarch *gdbarch,
                                       int max_register_virtual_size)
{
  gdbarch->max_register_virtual_size = max_register_virtual_size;
}

struct type *
gdbarch_register_virtual_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (gdbarch->register_virtual_type == 0)
    internal_error ("gdbarch: gdbarch_register_virtual_type invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_virtual_type called\n");
  return gdbarch->register_virtual_type (reg_nr);
}

void
set_gdbarch_register_virtual_type (struct gdbarch *gdbarch,
                                   gdbarch_register_virtual_type_ftype register_virtual_type)
{
  gdbarch->register_virtual_type = register_virtual_type;
}

int
gdbarch_use_generic_dummy_frames (struct gdbarch *gdbarch)
{
  if (gdbarch->use_generic_dummy_frames == -1)
    internal_error ("gdbarch: gdbarch_use_generic_dummy_frames invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_use_generic_dummy_frames called\n");
  return gdbarch->use_generic_dummy_frames;
}

void
set_gdbarch_use_generic_dummy_frames (struct gdbarch *gdbarch,
                                      int use_generic_dummy_frames)
{
  gdbarch->use_generic_dummy_frames = use_generic_dummy_frames;
}

int
gdbarch_call_dummy_location (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_location == 0)
    internal_error ("gdbarch: gdbarch_call_dummy_location invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_location called\n");
  return gdbarch->call_dummy_location;
}

void
set_gdbarch_call_dummy_location (struct gdbarch *gdbarch,
                                 int call_dummy_location)
{
  gdbarch->call_dummy_location = call_dummy_location;
}

CORE_ADDR
gdbarch_call_dummy_address (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_address == 0)
    internal_error ("gdbarch: gdbarch_call_dummy_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_address called\n");
  return gdbarch->call_dummy_address ();
}

void
set_gdbarch_call_dummy_address (struct gdbarch *gdbarch,
                                gdbarch_call_dummy_address_ftype call_dummy_address)
{
  gdbarch->call_dummy_address = call_dummy_address;
}

CORE_ADDR
gdbarch_call_dummy_start_offset (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_start_offset == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_start_offset invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_start_offset called\n");
  return gdbarch->call_dummy_start_offset;
}

void
set_gdbarch_call_dummy_start_offset (struct gdbarch *gdbarch,
                                     CORE_ADDR call_dummy_start_offset)
{
  gdbarch->call_dummy_start_offset = call_dummy_start_offset;
}

CORE_ADDR
gdbarch_call_dummy_breakpoint_offset (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_breakpoint_offset == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_breakpoint_offset invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_breakpoint_offset called\n");
  return gdbarch->call_dummy_breakpoint_offset;
}

void
set_gdbarch_call_dummy_breakpoint_offset (struct gdbarch *gdbarch,
                                          CORE_ADDR call_dummy_breakpoint_offset)
{
  gdbarch->call_dummy_breakpoint_offset = call_dummy_breakpoint_offset;
}

int
gdbarch_call_dummy_breakpoint_offset_p (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_breakpoint_offset_p == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_breakpoint_offset_p invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_breakpoint_offset_p called\n");
  return gdbarch->call_dummy_breakpoint_offset_p;
}

void
set_gdbarch_call_dummy_breakpoint_offset_p (struct gdbarch *gdbarch,
                                            int call_dummy_breakpoint_offset_p)
{
  gdbarch->call_dummy_breakpoint_offset_p = call_dummy_breakpoint_offset_p;
}

int
gdbarch_call_dummy_length (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_length == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_length invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_length called\n");
  return gdbarch->call_dummy_length;
}

void
set_gdbarch_call_dummy_length (struct gdbarch *gdbarch,
                               int call_dummy_length)
{
  gdbarch->call_dummy_length = call_dummy_length;
}

int
gdbarch_pc_in_call_dummy (struct gdbarch *gdbarch, CORE_ADDR pc, CORE_ADDR sp, CORE_ADDR frame_address)
{
  if (gdbarch->pc_in_call_dummy == 0)
    internal_error ("gdbarch: gdbarch_pc_in_call_dummy invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pc_in_call_dummy called\n");
  return gdbarch->pc_in_call_dummy (pc, sp, frame_address);
}

void
set_gdbarch_pc_in_call_dummy (struct gdbarch *gdbarch,
                              gdbarch_pc_in_call_dummy_ftype pc_in_call_dummy)
{
  gdbarch->pc_in_call_dummy = pc_in_call_dummy;
}

int
gdbarch_call_dummy_p (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_p == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_p invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_p called\n");
  return gdbarch->call_dummy_p;
}

void
set_gdbarch_call_dummy_p (struct gdbarch *gdbarch,
                          int call_dummy_p)
{
  gdbarch->call_dummy_p = call_dummy_p;
}

LONGEST *
gdbarch_call_dummy_words (struct gdbarch *gdbarch)
{
  /* Skip verify of call_dummy_words, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_words called\n");
  return gdbarch->call_dummy_words;
}

void
set_gdbarch_call_dummy_words (struct gdbarch *gdbarch,
                              LONGEST * call_dummy_words)
{
  gdbarch->call_dummy_words = call_dummy_words;
}

int
gdbarch_sizeof_call_dummy_words (struct gdbarch *gdbarch)
{
  /* Skip verify of sizeof_call_dummy_words, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_sizeof_call_dummy_words called\n");
  return gdbarch->sizeof_call_dummy_words;
}

void
set_gdbarch_sizeof_call_dummy_words (struct gdbarch *gdbarch,
                                     int sizeof_call_dummy_words)
{
  gdbarch->sizeof_call_dummy_words = sizeof_call_dummy_words;
}

int
gdbarch_call_dummy_stack_adjust_p (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_stack_adjust_p == -1)
    internal_error ("gdbarch: gdbarch_call_dummy_stack_adjust_p invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_stack_adjust_p called\n");
  return gdbarch->call_dummy_stack_adjust_p;
}

void
set_gdbarch_call_dummy_stack_adjust_p (struct gdbarch *gdbarch,
                                       int call_dummy_stack_adjust_p)
{
  gdbarch->call_dummy_stack_adjust_p = call_dummy_stack_adjust_p;
}

int
gdbarch_call_dummy_stack_adjust (struct gdbarch *gdbarch)
{
  if (gdbarch->call_dummy_stack_adjust_p && gdbarch->call_dummy_stack_adjust == 0)
    internal_error ("gdbarch: gdbarch_call_dummy_stack_adjust invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_call_dummy_stack_adjust called\n");
  return gdbarch->call_dummy_stack_adjust;
}

void
set_gdbarch_call_dummy_stack_adjust (struct gdbarch *gdbarch,
                                     int call_dummy_stack_adjust)
{
  gdbarch->call_dummy_stack_adjust = call_dummy_stack_adjust;
}

void
gdbarch_fix_call_dummy (struct gdbarch *gdbarch, char *dummy, CORE_ADDR pc, CORE_ADDR fun, int nargs, struct value **args, struct type *type, int gcc_p)
{
  if (gdbarch->fix_call_dummy == 0)
    internal_error ("gdbarch: gdbarch_fix_call_dummy invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fix_call_dummy called\n");
  gdbarch->fix_call_dummy (dummy, pc, fun, nargs, args, type, gcc_p);
}

void
set_gdbarch_fix_call_dummy (struct gdbarch *gdbarch,
                            gdbarch_fix_call_dummy_ftype fix_call_dummy)
{
  gdbarch->fix_call_dummy = fix_call_dummy;
}

int
gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_believe_pcc_promotion called\n");
  return gdbarch->believe_pcc_promotion;
}

void
set_gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch,
                                   int believe_pcc_promotion)
{
  gdbarch->believe_pcc_promotion = believe_pcc_promotion;
}

int
gdbarch_believe_pcc_promotion_type (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_believe_pcc_promotion_type called\n");
  return gdbarch->believe_pcc_promotion_type;
}

void
set_gdbarch_believe_pcc_promotion_type (struct gdbarch *gdbarch,
                                        int believe_pcc_promotion_type)
{
  gdbarch->believe_pcc_promotion_type = believe_pcc_promotion_type;
}

int
gdbarch_coerce_float_to_double (struct gdbarch *gdbarch, struct type *formal, struct type *actual)
{
  if (gdbarch->coerce_float_to_double == 0)
    internal_error ("gdbarch: gdbarch_coerce_float_to_double invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_coerce_float_to_double called\n");
  return gdbarch->coerce_float_to_double (formal, actual);
}

void
set_gdbarch_coerce_float_to_double (struct gdbarch *gdbarch,
                                    gdbarch_coerce_float_to_double_ftype coerce_float_to_double)
{
  gdbarch->coerce_float_to_double = coerce_float_to_double;
}

void
gdbarch_get_saved_register (struct gdbarch *gdbarch, char *raw_buffer, int *optimized, CORE_ADDR *addrp, struct frame_info *frame, int regnum, enum lval_type *lval)
{
  if (gdbarch->get_saved_register == 0)
    internal_error ("gdbarch: gdbarch_get_saved_register invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_get_saved_register called\n");
  gdbarch->get_saved_register (raw_buffer, optimized, addrp, frame, regnum, lval);
}

void
set_gdbarch_get_saved_register (struct gdbarch *gdbarch,
                                gdbarch_get_saved_register_ftype get_saved_register)
{
  gdbarch->get_saved_register = get_saved_register;
}

int
gdbarch_register_convertible (struct gdbarch *gdbarch, int nr)
{
  if (gdbarch->register_convertible == 0)
    internal_error ("gdbarch: gdbarch_register_convertible invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_convertible called\n");
  return gdbarch->register_convertible (nr);
}

void
set_gdbarch_register_convertible (struct gdbarch *gdbarch,
                                  gdbarch_register_convertible_ftype register_convertible)
{
  gdbarch->register_convertible = register_convertible;
}

void
gdbarch_register_convert_to_virtual (struct gdbarch *gdbarch, int regnum, struct type *type, char *from, char *to)
{
  if (gdbarch->register_convert_to_virtual == 0)
    internal_error ("gdbarch: gdbarch_register_convert_to_virtual invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_convert_to_virtual called\n");
  gdbarch->register_convert_to_virtual (regnum, type, from, to);
}

void
set_gdbarch_register_convert_to_virtual (struct gdbarch *gdbarch,
                                         gdbarch_register_convert_to_virtual_ftype register_convert_to_virtual)
{
  gdbarch->register_convert_to_virtual = register_convert_to_virtual;
}

void
gdbarch_register_convert_to_raw (struct gdbarch *gdbarch, struct type *type, int regnum, char *from, char *to)
{
  if (gdbarch->register_convert_to_raw == 0)
    internal_error ("gdbarch: gdbarch_register_convert_to_raw invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_convert_to_raw called\n");
  gdbarch->register_convert_to_raw (type, regnum, from, to);
}

void
set_gdbarch_register_convert_to_raw (struct gdbarch *gdbarch,
                                     gdbarch_register_convert_to_raw_ftype register_convert_to_raw)
{
  gdbarch->register_convert_to_raw = register_convert_to_raw;
}

void
gdbarch_extract_return_value (struct gdbarch *gdbarch, struct type *type, char *regbuf, char *valbuf)
{
  if (gdbarch->extract_return_value == 0)
    internal_error ("gdbarch: gdbarch_extract_return_value invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_extract_return_value called\n");
  gdbarch->extract_return_value (type, regbuf, valbuf);
}

void
set_gdbarch_extract_return_value (struct gdbarch *gdbarch,
                                  gdbarch_extract_return_value_ftype extract_return_value)
{
  gdbarch->extract_return_value = extract_return_value;
}

CORE_ADDR
gdbarch_push_arguments (struct gdbarch *gdbarch, int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr)
{
  if (gdbarch->push_arguments == 0)
    internal_error ("gdbarch: gdbarch_push_arguments invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_push_arguments called\n");
  return gdbarch->push_arguments (nargs, args, sp, struct_return, struct_addr);
}

void
set_gdbarch_push_arguments (struct gdbarch *gdbarch,
                            gdbarch_push_arguments_ftype push_arguments)
{
  gdbarch->push_arguments = push_arguments;
}

void
gdbarch_push_dummy_frame (struct gdbarch *gdbarch)
{
  if (gdbarch->push_dummy_frame == 0)
    internal_error ("gdbarch: gdbarch_push_dummy_frame invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_push_dummy_frame called\n");
  gdbarch->push_dummy_frame ();
}

void
set_gdbarch_push_dummy_frame (struct gdbarch *gdbarch,
                              gdbarch_push_dummy_frame_ftype push_dummy_frame)
{
  gdbarch->push_dummy_frame = push_dummy_frame;
}

CORE_ADDR
gdbarch_push_return_address (struct gdbarch *gdbarch, CORE_ADDR pc, CORE_ADDR sp)
{
  if (gdbarch->push_return_address == 0)
    internal_error ("gdbarch: gdbarch_push_return_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_push_return_address called\n");
  return gdbarch->push_return_address (pc, sp);
}

void
set_gdbarch_push_return_address (struct gdbarch *gdbarch,
                                 gdbarch_push_return_address_ftype push_return_address)
{
  gdbarch->push_return_address = push_return_address;
}

void
gdbarch_pop_frame (struct gdbarch *gdbarch)
{
  if (gdbarch->pop_frame == 0)
    internal_error ("gdbarch: gdbarch_pop_frame invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pop_frame called\n");
  gdbarch->pop_frame ();
}

void
set_gdbarch_pop_frame (struct gdbarch *gdbarch,
                       gdbarch_pop_frame_ftype pop_frame)
{
  gdbarch->pop_frame = pop_frame;
}

CORE_ADDR
gdbarch_d10v_make_daddr (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_make_daddr == 0)
    internal_error ("gdbarch: gdbarch_d10v_make_daddr invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_make_daddr called\n");
  return gdbarch->d10v_make_daddr (x);
}

void
set_gdbarch_d10v_make_daddr (struct gdbarch *gdbarch,
                             gdbarch_d10v_make_daddr_ftype d10v_make_daddr)
{
  gdbarch->d10v_make_daddr = d10v_make_daddr;
}

CORE_ADDR
gdbarch_d10v_make_iaddr (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_make_iaddr == 0)
    internal_error ("gdbarch: gdbarch_d10v_make_iaddr invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_make_iaddr called\n");
  return gdbarch->d10v_make_iaddr (x);
}

void
set_gdbarch_d10v_make_iaddr (struct gdbarch *gdbarch,
                             gdbarch_d10v_make_iaddr_ftype d10v_make_iaddr)
{
  gdbarch->d10v_make_iaddr = d10v_make_iaddr;
}

int
gdbarch_d10v_daddr_p (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_daddr_p == 0)
    internal_error ("gdbarch: gdbarch_d10v_daddr_p invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_daddr_p called\n");
  return gdbarch->d10v_daddr_p (x);
}

void
set_gdbarch_d10v_daddr_p (struct gdbarch *gdbarch,
                          gdbarch_d10v_daddr_p_ftype d10v_daddr_p)
{
  gdbarch->d10v_daddr_p = d10v_daddr_p;
}

int
gdbarch_d10v_iaddr_p (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_iaddr_p == 0)
    internal_error ("gdbarch: gdbarch_d10v_iaddr_p invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_iaddr_p called\n");
  return gdbarch->d10v_iaddr_p (x);
}

void
set_gdbarch_d10v_iaddr_p (struct gdbarch *gdbarch,
                          gdbarch_d10v_iaddr_p_ftype d10v_iaddr_p)
{
  gdbarch->d10v_iaddr_p = d10v_iaddr_p;
}

CORE_ADDR
gdbarch_d10v_convert_daddr_to_raw (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_convert_daddr_to_raw == 0)
    internal_error ("gdbarch: gdbarch_d10v_convert_daddr_to_raw invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_convert_daddr_to_raw called\n");
  return gdbarch->d10v_convert_daddr_to_raw (x);
}

void
set_gdbarch_d10v_convert_daddr_to_raw (struct gdbarch *gdbarch,
                                       gdbarch_d10v_convert_daddr_to_raw_ftype d10v_convert_daddr_to_raw)
{
  gdbarch->d10v_convert_daddr_to_raw = d10v_convert_daddr_to_raw;
}

CORE_ADDR
gdbarch_d10v_convert_iaddr_to_raw (struct gdbarch *gdbarch, CORE_ADDR x)
{
  if (gdbarch->d10v_convert_iaddr_to_raw == 0)
    internal_error ("gdbarch: gdbarch_d10v_convert_iaddr_to_raw invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_d10v_convert_iaddr_to_raw called\n");
  return gdbarch->d10v_convert_iaddr_to_raw (x);
}

void
set_gdbarch_d10v_convert_iaddr_to_raw (struct gdbarch *gdbarch,
                                       gdbarch_d10v_convert_iaddr_to_raw_ftype d10v_convert_iaddr_to_raw)
{
  gdbarch->d10v_convert_iaddr_to_raw = d10v_convert_iaddr_to_raw;
}

void
gdbarch_store_struct_return (struct gdbarch *gdbarch, CORE_ADDR addr, CORE_ADDR sp)
{
  if (gdbarch->store_struct_return == 0)
    internal_error ("gdbarch: gdbarch_store_struct_return invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_store_struct_return called\n");
  gdbarch->store_struct_return (addr, sp);
}

void
set_gdbarch_store_struct_return (struct gdbarch *gdbarch,
                                 gdbarch_store_struct_return_ftype store_struct_return)
{
  gdbarch->store_struct_return = store_struct_return;
}

void
gdbarch_store_return_value (struct gdbarch *gdbarch, struct type *type, char *valbuf)
{
  if (gdbarch->store_return_value == 0)
    internal_error ("gdbarch: gdbarch_store_return_value invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_store_return_value called\n");
  gdbarch->store_return_value (type, valbuf);
}

void
set_gdbarch_store_return_value (struct gdbarch *gdbarch,
                                gdbarch_store_return_value_ftype store_return_value)
{
  gdbarch->store_return_value = store_return_value;
}

CORE_ADDR
gdbarch_extract_struct_value_address (struct gdbarch *gdbarch, char *regbuf)
{
  if (gdbarch->extract_struct_value_address == 0)
    internal_error ("gdbarch: gdbarch_extract_struct_value_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_extract_struct_value_address called\n");
  return gdbarch->extract_struct_value_address (regbuf);
}

void
set_gdbarch_extract_struct_value_address (struct gdbarch *gdbarch,
                                          gdbarch_extract_struct_value_address_ftype extract_struct_value_address)
{
  gdbarch->extract_struct_value_address = extract_struct_value_address;
}

int
gdbarch_use_struct_convention (struct gdbarch *gdbarch, int gcc_p, struct type *value_type)
{
  if (gdbarch->use_struct_convention == 0)
    internal_error ("gdbarch: gdbarch_use_struct_convention invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_use_struct_convention called\n");
  return gdbarch->use_struct_convention (gcc_p, value_type);
}

void
set_gdbarch_use_struct_convention (struct gdbarch *gdbarch,
                                   gdbarch_use_struct_convention_ftype use_struct_convention)
{
  gdbarch->use_struct_convention = use_struct_convention;
}

void
gdbarch_frame_init_saved_regs (struct gdbarch *gdbarch, struct frame_info *frame)
{
  if (gdbarch->frame_init_saved_regs == 0)
    internal_error ("gdbarch: gdbarch_frame_init_saved_regs invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_init_saved_regs called\n");
  gdbarch->frame_init_saved_regs (frame);
}

void
set_gdbarch_frame_init_saved_regs (struct gdbarch *gdbarch,
                                   gdbarch_frame_init_saved_regs_ftype frame_init_saved_regs)
{
  gdbarch->frame_init_saved_regs = frame_init_saved_regs;
}

void
gdbarch_init_extra_frame_info (struct gdbarch *gdbarch, int fromleaf, struct frame_info *frame)
{
  if (gdbarch->init_extra_frame_info == 0)
    internal_error ("gdbarch: gdbarch_init_extra_frame_info invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_init_extra_frame_info called\n");
  gdbarch->init_extra_frame_info (fromleaf, frame);
}

void
set_gdbarch_init_extra_frame_info (struct gdbarch *gdbarch,
                                   gdbarch_init_extra_frame_info_ftype init_extra_frame_info)
{
  gdbarch->init_extra_frame_info = init_extra_frame_info;
}

CORE_ADDR
gdbarch_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR ip)
{
  if (gdbarch->skip_prologue == 0)
    internal_error ("gdbarch: gdbarch_skip_prologue invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_prologue called\n");
  return gdbarch->skip_prologue (ip);
}

void
set_gdbarch_skip_prologue (struct gdbarch *gdbarch,
                           gdbarch_skip_prologue_ftype skip_prologue)
{
  gdbarch->skip_prologue = skip_prologue;
}

int
gdbarch_inner_than (struct gdbarch *gdbarch, CORE_ADDR lhs, CORE_ADDR rhs)
{
  if (gdbarch->inner_than == 0)
    internal_error ("gdbarch: gdbarch_inner_than invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_inner_than called\n");
  return gdbarch->inner_than (lhs, rhs);
}

void
set_gdbarch_inner_than (struct gdbarch *gdbarch,
                        gdbarch_inner_than_ftype inner_than)
{
  gdbarch->inner_than = inner_than;
}

unsigned char *
gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr)
{
  if (gdbarch->breakpoint_from_pc == 0)
    internal_error ("gdbarch: gdbarch_breakpoint_from_pc invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_breakpoint_from_pc called\n");
  return gdbarch->breakpoint_from_pc (pcptr, lenptr);
}

void
set_gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch,
                                gdbarch_breakpoint_from_pc_ftype breakpoint_from_pc)
{
  gdbarch->breakpoint_from_pc = breakpoint_from_pc;
}

int
gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, CORE_ADDR addr, char *contents_cache)
{
  if (gdbarch->memory_insert_breakpoint == 0)
    internal_error ("gdbarch: gdbarch_memory_insert_breakpoint invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_memory_insert_breakpoint called\n");
  return gdbarch->memory_insert_breakpoint (addr, contents_cache);
}

void
set_gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch,
                                      gdbarch_memory_insert_breakpoint_ftype memory_insert_breakpoint)
{
  gdbarch->memory_insert_breakpoint = memory_insert_breakpoint;
}

int
gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, CORE_ADDR addr, char *contents_cache)
{
  if (gdbarch->memory_remove_breakpoint == 0)
    internal_error ("gdbarch: gdbarch_memory_remove_breakpoint invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_memory_remove_breakpoint called\n");
  return gdbarch->memory_remove_breakpoint (addr, contents_cache);
}

void
set_gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch,
                                      gdbarch_memory_remove_breakpoint_ftype memory_remove_breakpoint)
{
  gdbarch->memory_remove_breakpoint = memory_remove_breakpoint;
}

CORE_ADDR
gdbarch_decr_pc_after_break (struct gdbarch *gdbarch)
{
  if (gdbarch->decr_pc_after_break == -1)
    internal_error ("gdbarch: gdbarch_decr_pc_after_break invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_decr_pc_after_break called\n");
  return gdbarch->decr_pc_after_break;
}

void
set_gdbarch_decr_pc_after_break (struct gdbarch *gdbarch,
                                 CORE_ADDR decr_pc_after_break)
{
  gdbarch->decr_pc_after_break = decr_pc_after_break;
}

CORE_ADDR
gdbarch_function_start_offset (struct gdbarch *gdbarch)
{
  if (gdbarch->function_start_offset == -1)
    internal_error ("gdbarch: gdbarch_function_start_offset invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_function_start_offset called\n");
  return gdbarch->function_start_offset;
}

void
set_gdbarch_function_start_offset (struct gdbarch *gdbarch,
                                   CORE_ADDR function_start_offset)
{
  gdbarch->function_start_offset = function_start_offset;
}

void
gdbarch_remote_translate_xfer_address (struct gdbarch *gdbarch, CORE_ADDR gdb_addr, int gdb_len, CORE_ADDR *rem_addr, int *rem_len)
{
  if (gdbarch->remote_translate_xfer_address == 0)
    internal_error ("gdbarch: gdbarch_remote_translate_xfer_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_remote_translate_xfer_address called\n");
  gdbarch->remote_translate_xfer_address (gdb_addr, gdb_len, rem_addr, rem_len);
}

void
set_gdbarch_remote_translate_xfer_address (struct gdbarch *gdbarch,
                                           gdbarch_remote_translate_xfer_address_ftype remote_translate_xfer_address)
{
  gdbarch->remote_translate_xfer_address = remote_translate_xfer_address;
}

CORE_ADDR
gdbarch_frame_args_skip (struct gdbarch *gdbarch)
{
  if (gdbarch->frame_args_skip == -1)
    internal_error ("gdbarch: gdbarch_frame_args_skip invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_args_skip called\n");
  return gdbarch->frame_args_skip;
}

void
set_gdbarch_frame_args_skip (struct gdbarch *gdbarch,
                             CORE_ADDR frame_args_skip)
{
  gdbarch->frame_args_skip = frame_args_skip;
}

int
gdbarch_frameless_function_invocation (struct gdbarch *gdbarch, struct frame_info *fi)
{
  if (gdbarch->frameless_function_invocation == 0)
    internal_error ("gdbarch: gdbarch_frameless_function_invocation invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frameless_function_invocation called\n");
  return gdbarch->frameless_function_invocation (fi);
}

void
set_gdbarch_frameless_function_invocation (struct gdbarch *gdbarch,
                                           gdbarch_frameless_function_invocation_ftype frameless_function_invocation)
{
  gdbarch->frameless_function_invocation = frameless_function_invocation;
}

CORE_ADDR
gdbarch_frame_chain (struct gdbarch *gdbarch, struct frame_info *frame)
{
  if (gdbarch->frame_chain == 0)
    internal_error ("gdbarch: gdbarch_frame_chain invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_chain called\n");
  return gdbarch->frame_chain (frame);
}

void
set_gdbarch_frame_chain (struct gdbarch *gdbarch,
                         gdbarch_frame_chain_ftype frame_chain)
{
  gdbarch->frame_chain = frame_chain;
}

int
gdbarch_frame_chain_valid (struct gdbarch *gdbarch, CORE_ADDR chain, struct frame_info *thisframe)
{
  if (gdbarch->frame_chain_valid == 0)
    internal_error ("gdbarch: gdbarch_frame_chain_valid invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_chain_valid called\n");
  return gdbarch->frame_chain_valid (chain, thisframe);
}

void
set_gdbarch_frame_chain_valid (struct gdbarch *gdbarch,
                               gdbarch_frame_chain_valid_ftype frame_chain_valid)
{
  gdbarch->frame_chain_valid = frame_chain_valid;
}

CORE_ADDR
gdbarch_frame_saved_pc (struct gdbarch *gdbarch, struct frame_info *fi)
{
  if (gdbarch->frame_saved_pc == 0)
    internal_error ("gdbarch: gdbarch_frame_saved_pc invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_saved_pc called\n");
  return gdbarch->frame_saved_pc (fi);
}

void
set_gdbarch_frame_saved_pc (struct gdbarch *gdbarch,
                            gdbarch_frame_saved_pc_ftype frame_saved_pc)
{
  gdbarch->frame_saved_pc = frame_saved_pc;
}

CORE_ADDR
gdbarch_frame_args_address (struct gdbarch *gdbarch, struct frame_info *fi)
{
  if (gdbarch->frame_args_address == 0)
    internal_error ("gdbarch: gdbarch_frame_args_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_args_address called\n");
  return gdbarch->frame_args_address (fi);
}

void
set_gdbarch_frame_args_address (struct gdbarch *gdbarch,
                                gdbarch_frame_args_address_ftype frame_args_address)
{
  gdbarch->frame_args_address = frame_args_address;
}

CORE_ADDR
gdbarch_frame_locals_address (struct gdbarch *gdbarch, struct frame_info *fi)
{
  if (gdbarch->frame_locals_address == 0)
    internal_error ("gdbarch: gdbarch_frame_locals_address invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_locals_address called\n");
  return gdbarch->frame_locals_address (fi);
}

void
set_gdbarch_frame_locals_address (struct gdbarch *gdbarch,
                                  gdbarch_frame_locals_address_ftype frame_locals_address)
{
  gdbarch->frame_locals_address = frame_locals_address;
}

CORE_ADDR
gdbarch_saved_pc_after_call (struct gdbarch *gdbarch, struct frame_info *frame)
{
  if (gdbarch->saved_pc_after_call == 0)
    internal_error ("gdbarch: gdbarch_saved_pc_after_call invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_saved_pc_after_call called\n");
  return gdbarch->saved_pc_after_call (frame);
}

void
set_gdbarch_saved_pc_after_call (struct gdbarch *gdbarch,
                                 gdbarch_saved_pc_after_call_ftype saved_pc_after_call)
{
  gdbarch->saved_pc_after_call = saved_pc_after_call;
}

int
gdbarch_frame_num_args (struct gdbarch *gdbarch, struct frame_info *frame)
{
  if (gdbarch->frame_num_args == 0)
    internal_error ("gdbarch: gdbarch_frame_num_args invalid");
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_num_args called\n");
  return gdbarch->frame_num_args (frame);
}

void
set_gdbarch_frame_num_args (struct gdbarch *gdbarch,
                            gdbarch_frame_num_args_ftype frame_num_args)
{
  gdbarch->frame_num_args = frame_num_args;
}


/* Keep a registrary of per-architecture data-pointers required by GDB
   modules. */

struct gdbarch_data
{
  int index;
};

struct gdbarch_data_registration
{
  gdbarch_data_ftype *init;
  struct gdbarch_data *data;
  struct gdbarch_data_registration *next;
};

struct gdbarch_data_registrary
{
  int nr;
  struct gdbarch_data_registration *registrations;
};

struct gdbarch_data_registrary gdbarch_data_registrary =
{
  0, NULL,
};

struct gdbarch_data *
register_gdbarch_data (gdbarch_data_ftype *init)
{
  struct gdbarch_data_registration **curr;
  for (curr = &gdbarch_data_registrary.registrations;
       (*curr) != NULL;
       curr = &(*curr)->next);
  (*curr) = XMALLOC (struct gdbarch_data_registration);
  (*curr)->next = NULL;
  (*curr)->init = init;
  (*curr)->data = XMALLOC (struct gdbarch_data);
  (*curr)->data->index = gdbarch_data_registrary.nr++;
  return (*curr)->data;
}


/* Walk through all the registered users initializing each in turn. */

static void
init_gdbarch_data (struct gdbarch *gdbarch)
{
  struct gdbarch_data_registration *rego;
  gdbarch->nr_data = gdbarch_data_registrary.nr + 1;
  gdbarch->data = xmalloc (sizeof (void*) * gdbarch->nr_data);
  for (rego = gdbarch_data_registrary.registrations;
       rego != NULL;
       rego = rego->next)
    {
      if (rego->data->index < gdbarch->nr_data)
	gdbarch->data[rego->data->index] = rego->init ();
    }
}


/* Return the current value of the specified per-architecture
   data-pointer. */

void *
gdbarch_data (data)
     struct gdbarch_data *data;
{
  if (data->index >= current_gdbarch->nr_data)
    internal_error ("gdbarch_data: request for non-existant data.");
  return current_gdbarch->data[data->index];
}



/* Keep a registrary of swaped data required by GDB modules. */

struct gdbarch_swap
{
  void *swap;
  struct gdbarch_swap_registration *source;
  struct gdbarch_swap *next;
};

struct gdbarch_swap_registration
{
  void *data;
  unsigned long sizeof_data;
  gdbarch_swap_ftype *init;
  struct gdbarch_swap_registration *next;
};

struct gdbarch_swap_registrary
{
  int nr;
  struct gdbarch_swap_registration *registrations;
};

struct gdbarch_swap_registrary gdbarch_swap_registrary = 
{
  0, NULL,
};

void
register_gdbarch_swap (void *data,
		       unsigned long sizeof_data,
		       gdbarch_swap_ftype *init)
{
  struct gdbarch_swap_registration **rego;
  for (rego = &gdbarch_swap_registrary.registrations;
       (*rego) != NULL;
       rego = &(*rego)->next);
  (*rego) = XMALLOC (struct gdbarch_swap_registration);
  (*rego)->next = NULL;
  (*rego)->init = init;
  (*rego)->data = data;
  (*rego)->sizeof_data = sizeof_data;
}


static void
init_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap_registration *rego;
  struct gdbarch_swap **curr = &gdbarch->swap;
  for (rego = gdbarch_swap_registrary.registrations;
       rego != NULL;
       rego = rego->next)
    {
      if (rego->data != NULL)
	{
	  (*curr) = XMALLOC (struct gdbarch_swap);
	  (*curr)->source = rego;
	  (*curr)->swap = xmalloc (rego->sizeof_data);
	  (*curr)->next = NULL;
	  memset (rego->data, 0, rego->sizeof_data);
	  curr = &(*curr)->next;
	}
      if (rego->init != NULL)
	rego->init ();
    }
}

static void
swapout_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap *curr;
  for (curr = gdbarch->swap;
       curr != NULL;
       curr = curr->next)
    memcpy (curr->swap, curr->source->data, curr->source->sizeof_data);
}

static void
swapin_gdbarch_swap (struct gdbarch *gdbarch)
{
  struct gdbarch_swap *curr;
  for (curr = gdbarch->swap;
       curr != NULL;
       curr = curr->next)
    memcpy (curr->source->data, curr->swap, curr->source->sizeof_data);
}


/* Keep a registrary of the architectures known by GDB. */

struct gdbarch_init_registration
{
  enum bfd_architecture bfd_architecture;
  gdbarch_init_ftype *init;
  struct gdbarch_list *arches;
  struct gdbarch_init_registration *next;
};

static struct gdbarch_init_registration *gdbarch_init_registrary = NULL;

void
register_gdbarch_init (enum bfd_architecture bfd_architecture,
                       gdbarch_init_ftype *init)
{
  struct gdbarch_init_registration **curr;
  const struct bfd_arch_info *bfd_arch_info;
  /* Check that BFD reconizes this architecture */
  bfd_arch_info = bfd_lookup_arch (bfd_architecture, 0);
  if (bfd_arch_info == NULL)
    {
      internal_error ("gdbarch: Attempt to register unknown architecture (%d)", bfd_architecture);
    }
  /* Check that we haven't seen this architecture before */
  for (curr = &gdbarch_init_registrary;
       (*curr) != NULL;
       curr = &(*curr)->next)
    {
      if (bfd_architecture == (*curr)->bfd_architecture)
	internal_error ("gdbarch: Duplicate registraration of architecture (%s)",
	       bfd_arch_info->printable_name);
    }
  /* log it */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "register_gdbarch_init (%s, 0x%08lx)\n",
			bfd_arch_info->printable_name,
			(long) init);
  /* Append it */
  (*curr) = XMALLOC (struct gdbarch_init_registration);
  (*curr)->bfd_architecture = bfd_architecture;
  (*curr)->init = init;
  (*curr)->arches = NULL;
  (*curr)->next = NULL;
}
  


/* Look for an architecture using gdbarch_info.  Base search on only
   BFD_ARCH_INFO and BYTE_ORDER. */

struct gdbarch_list *
gdbarch_list_lookup_by_info (struct gdbarch_list *arches,
                             const struct gdbarch_info *info)
{
  for (; arches != NULL; arches = arches->next)
    {
      if (info->bfd_arch_info != arches->gdbarch->bfd_arch_info)
	continue;
      if (info->byte_order != arches->gdbarch->byte_order)
	continue;
      return arches;
    }
  return NULL;
}


/* Update the current architecture. Return ZERO if the update request
   failed. */

int
gdbarch_update (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;
  struct gdbarch_list **list;
  struct gdbarch_init_registration *rego;

  /* Fill in any missing bits. Most important is the bfd_architecture
     which is used to select the target architecture. */
  if (info.bfd_architecture == bfd_arch_unknown)
    {
      if (info.bfd_arch_info != NULL)
	info.bfd_architecture = info.bfd_arch_info->arch;
      else if (info.abfd != NULL)
	info.bfd_architecture = bfd_get_arch (info.abfd);
      /* FIXME - should query BFD for its default architecture. */
      else
	info.bfd_architecture = current_gdbarch->bfd_arch_info->arch;
    }
  if (info.bfd_arch_info == NULL)
    {
      if (target_architecture_auto && info.abfd != NULL)
	info.bfd_arch_info = bfd_get_arch_info (info.abfd);
      else
	info.bfd_arch_info = current_gdbarch->bfd_arch_info;
    }
  if (info.byte_order == 0)
    {
      if (target_byte_order_auto && info.abfd != NULL)
	info.byte_order = (bfd_big_endian (info.abfd) ? BIG_ENDIAN
			   : bfd_little_endian (info.abfd) ? LITTLE_ENDIAN
			   : 0);
      else
	info.byte_order = current_gdbarch->byte_order;
      /* FIXME - should query BFD for its default byte-order. */
    }
  /* A default for abfd? */

  /* Find the target that knows about this architecture. */
  for (rego = gdbarch_init_registrary;
       rego != NULL && rego->bfd_architecture != info.bfd_architecture;
       rego = rego->next);
  if (rego == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: No matching architecture\n");
      return 0;
    }

  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.bfd_architecture %d (%s)\n",
			  info.bfd_architecture,
			  bfd_lookup_arch (info.bfd_architecture, 0)->printable_name);
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.bfd_arch_info %s\n",
			  (info.bfd_arch_info != NULL
			   ? info.bfd_arch_info->printable_name
			   : "(null)"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.byte_order %d (%s)\n",
			  info.byte_order,
			  (info.byte_order == BIG_ENDIAN ? "big"
			   : info.byte_order == LITTLE_ENDIAN ? "little"
			   : "default"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.abfd 0x%lx\n",
			  (long) info.abfd);
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: info.tdep_info 0x%lx\n",
			  (long) info.tdep_info);
    }

  /* Ask the target for a replacement architecture. */
  new_gdbarch = rego->init (info, rego->arches);

  /* Did the target like it?  No. Reject the change. */
  if (new_gdbarch == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Target rejected architecture\n");
      return 0;
    }

  /* Did the architecture change?  No. Do nothing. */
  if (current_gdbarch == new_gdbarch)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Architecture 0x%08lx (%s) unchanged\n",
			    (long) new_gdbarch,
			    new_gdbarch->bfd_arch_info->printable_name);
      return 1;
    }

  /* Swap all data belonging to the old target out */
  swapout_gdbarch_swap (current_gdbarch);

  /* Is this a pre-existing architecture?  Yes. Swap it in.  */
  for (list = &rego->arches;
       (*list) != NULL;
       list = &(*list)->next)
    {
      if ((*list)->gdbarch == new_gdbarch)
	{
	  if (gdbarch_debug)
	    fprintf_unfiltered (gdb_stdlog, "gdbarch_update: Previous architecture 0x%08lx (%s) selected\n",
				(long) new_gdbarch,
				new_gdbarch->bfd_arch_info->printable_name);
	  current_gdbarch = new_gdbarch;
	  swapin_gdbarch_swap (new_gdbarch);
	  return 1;
	}
    }
    
  /* Append this new architecture to this targets list. */
  (*list) = XMALLOC (struct gdbarch_list);
  (*list)->next = NULL;
  (*list)->gdbarch = new_gdbarch;

  /* Switch to this new architecture.  Dump it out. */
  current_gdbarch = new_gdbarch;
  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_update: New architecture 0x%08lx (%s) selected\n",
			  (long) new_gdbarch,
			  new_gdbarch->bfd_arch_info->printable_name);
      gdbarch_dump ();
    }
  
  /* Check that the newly installed architecture is valid.  */
  verify_gdbarch (new_gdbarch);

  /* Initialize the per-architecture memory (swap) areas.
     CURRENT_GDBARCH must be update before these modules are
     called. */
  init_gdbarch_swap (new_gdbarch);
  
  /* Initialize the per-architecture data-pointer of all parties that
     registered an interest in this architecture.  CURRENT_GDBARCH
     must be updated before these modules are called. */
  init_gdbarch_data (new_gdbarch);
  
  return 1;
}



/* Functions to manipulate the endianness of the target.  */

#ifdef TARGET_BYTE_ORDER_SELECTABLE
/* compat - Catch old targets that expect a selectable byte-order to
   default to BIG_ENDIAN */
#ifndef TARGET_BYTE_ORDER_DEFAULT
#define TARGET_BYTE_ORDER_DEFAULT BIG_ENDIAN
#endif
#endif
#if !TARGET_BYTE_ORDER_SELECTABLE_P
#ifndef TARGET_BYTE_ORDER_DEFAULT
/* compat - Catch old non byte-order selectable targets that do not
   define TARGET_BYTE_ORDER_DEFAULT and instead expect
   TARGET_BYTE_ORDER to be used as the default.  For targets that
   defined neither TARGET_BYTE_ORDER nor TARGET_BYTE_ORDER_DEFAULT the
   below will get a strange compiler warning. */
#define TARGET_BYTE_ORDER_DEFAULT TARGET_BYTE_ORDER
#endif
#endif
#ifndef TARGET_BYTE_ORDER_DEFAULT
#define TARGET_BYTE_ORDER_DEFAULT BIG_ENDIAN /* arbitrary */
#endif
int target_byte_order = TARGET_BYTE_ORDER_DEFAULT;
int target_byte_order_auto = 1;

/* Chain containing the \"set endian\" commands.  */
static struct cmd_list_element *endianlist = NULL;

/* Called by ``show endian''.  */
static void
show_endian (char *args, int from_tty)
{
  char *msg =
    (TARGET_BYTE_ORDER_AUTO
     ? "The target endianness is set automatically (currently %s endian)\n"
     : "The target is assumed to be %s endian\n");
  printf_unfiltered (msg, (TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little"));
}

/* Called if the user enters ``set endian'' without an argument.  */
static void
set_endian (char *args, int from_tty)
{
  printf_unfiltered ("\"set endian\" must be followed by \"auto\", \"big\" or \"little\".\n");
  show_endian (args, from_tty);
}

/* Called by ``set endian big''.  */
static void
set_endian_big (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order = BIG_ENDIAN;
      target_byte_order_auto = 0;
      if (GDB_MULTI_ARCH)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.byte_order = BIG_ENDIAN;
	  gdbarch_update (info);
	}
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Called by ``set endian little''.  */
static void
set_endian_little (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order = LITTLE_ENDIAN;
      target_byte_order_auto = 0;
      if (GDB_MULTI_ARCH)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.byte_order = LITTLE_ENDIAN;
	  gdbarch_update (info);
	}
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Called by ``set endian auto''.  */
static void
set_endian_auto (char *args, int from_tty)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      target_byte_order_auto = 1;
    }
  else
    {
      printf_unfiltered ("Byte order is not selectable.");
      show_endian (args, from_tty);
    }
}

/* Set the endianness from a BFD.  */
static void
set_endian_from_file (bfd *abfd)
{
  if (TARGET_BYTE_ORDER_SELECTABLE_P)
    {
      int want;
      
      if (bfd_big_endian (abfd))
	want = BIG_ENDIAN;
      else
	want = LITTLE_ENDIAN;
      if (TARGET_BYTE_ORDER_AUTO)
	target_byte_order = want;
      else if (TARGET_BYTE_ORDER != want)
	warning ("%s endian file does not match %s endian target.",
		 want == BIG_ENDIAN ? "big" : "little",
		 TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little");
    }
  else
    {
      if (bfd_big_endian (abfd)
	  ? TARGET_BYTE_ORDER != BIG_ENDIAN
	  : TARGET_BYTE_ORDER == BIG_ENDIAN)
	warning ("%s endian file does not match %s endian target.",
		 bfd_big_endian (abfd) ? "big" : "little",
		 TARGET_BYTE_ORDER == BIG_ENDIAN ? "big" : "little");
    }
}



/* Functions to manipulate the architecture of the target */

enum set_arch { set_arch_auto, set_arch_manual };

int target_architecture_auto = 1;
extern const struct bfd_arch_info bfd_default_arch_struct;
const struct bfd_arch_info *target_architecture = &bfd_default_arch_struct;
int (*target_architecture_hook) (const struct bfd_arch_info *ap);

static void show_endian (char *, int);
static void set_endian (char *, int);
static void set_endian_big (char *, int);
static void set_endian_little (char *, int);
static void set_endian_auto (char *, int);
static void set_endian_from_file (bfd *);
static int arch_ok (const struct bfd_arch_info *arch);
static void set_arch (const struct bfd_arch_info *arch, enum set_arch type);
static void show_architecture (char *, int);
static void set_architecture (char *, int);
static void info_architecture (char *, int);
static void set_architecture_from_file (bfd *);

/* Do the real work of changing the current architecture */

static int
arch_ok (const struct bfd_arch_info *arch)
{
  /* Should be performing the more basic check that the binary is
     compatible with GDB. */
  /* Check with the target that the architecture is valid. */
  return (target_architecture_hook == NULL
	  || target_architecture_hook (arch));
}

static void
set_arch (const struct bfd_arch_info *arch,
          enum set_arch type)
{
  switch (type)
    {
    case set_arch_auto:
      if (!arch_ok (arch))
	warning ("Target may not support %s architecture",
		 arch->printable_name);
      target_architecture = arch;
      break;
    case set_arch_manual:
      if (!arch_ok (arch))
	{
	  printf_unfiltered ("Target does not support `%s' architecture.\n",
			     arch->printable_name);
	}
      else
	{
	  target_architecture_auto = 0;
	  target_architecture = arch;
	}
      break;
    }
  if (gdbarch_debug)
    gdbarch_dump ();
}

/* Called if the user enters ``show architecture'' without an argument. */
static void
show_architecture (char *args, int from_tty)
{
  const char *arch;
  arch = TARGET_ARCHITECTURE->printable_name;
  if (target_architecture_auto)
    printf_filtered ("The target architecture is set automatically (currently %s)\n", arch);
  else
    printf_filtered ("The target architecture is assumed to be %s\n", arch);
}

/* Called if the user enters ``set architecture'' with or without an
   argument. */
static void
set_architecture (char *args, int from_tty)
{
  if (args == NULL)
    {
      printf_unfiltered ("\"set architecture\" must be followed by \"auto\" or an architecture name.\n");
    }
  else if (strcmp (args, "auto") == 0)
    {
      target_architecture_auto = 1;
    }
  else if (GDB_MULTI_ARCH)
    {
      const struct bfd_arch_info *arch = bfd_scan_arch (args);
      if (arch == NULL)
	printf_unfiltered ("Architecture `%s' not reconized.\n", args);
      else
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.bfd_arch_info = arch;
	  if (gdbarch_update (info))
	    target_architecture_auto = 0;
	  else
	    printf_unfiltered ("Architecture `%s' not reconized.\n", args);
	}
    }
  else
    {
      const struct bfd_arch_info *arch = bfd_scan_arch (args);
      if (arch != NULL)
	set_arch (arch, set_arch_manual);
      else
	printf_unfiltered ("Architecture `%s' not reconized.\n", args);
    }
}

/* Called if the user enters ``info architecture'' without an argument. */
static void
info_architecture (char *args, int from_tty)
{
  enum bfd_architecture a;
  if (GDB_MULTI_ARCH)
    {
      if (gdbarch_init_registrary != NULL)
	{
	  struct gdbarch_init_registration *rego;
	  printf_filtered ("Available architectures are:\n");
	  for (rego = gdbarch_init_registrary;
	       rego != NULL;
	       rego = rego->next)
	    {
	      const struct bfd_arch_info *ap;
	      ap = bfd_lookup_arch (rego->bfd_architecture, 0);
	      if (ap != NULL)
		{
		  do
		    {
		      printf_filtered (" %s", ap->printable_name);
		      ap = ap->next;
		    }
		  while (ap != NULL);
		  printf_filtered ("\n");
		}
	    }
	}
      else
	{
	  printf_filtered ("There are no available architectures.\n");
	}
      return;
    }
  printf_filtered ("Available architectures are:\n");
  for (a = bfd_arch_obscure + 1; a < bfd_arch_last; a++)
    {
      const struct bfd_arch_info *ap = bfd_lookup_arch (a, 0);
      if (ap != NULL)
	{
	  do
	    {
	      printf_filtered (" %s", ap->printable_name);
	      ap = ap->next;
	    }
	  while (ap != NULL);
	  printf_filtered ("\n");
	}
    }
}

/* Set the architecture from arch/machine */
void
set_architecture_from_arch_mach (arch, mach)
     enum bfd_architecture arch;
     unsigned long mach;
{
  const struct bfd_arch_info *wanted = bfd_lookup_arch (arch, mach);
  if (wanted != NULL)
    set_arch (wanted, set_arch_manual);
  else
    internal_error ("gdbarch: hardwired architecture/machine not reconized");
}

/* Set the architecture from a BFD */
static void
set_architecture_from_file (bfd *abfd)
{
  const struct bfd_arch_info *wanted = bfd_get_arch_info (abfd);
  if (target_architecture_auto)
    {
      set_arch (wanted, set_arch_auto);
    }
  else if (wanted != target_architecture)
    {
      warning ("%s architecture file may be incompatible with %s target.",
	       wanted->printable_name,
	       target_architecture->printable_name);
    }
}


/* Misc helper functions for targets. */

int
frame_num_args_unknown (fi)
     struct frame_info *fi;
{
  return -1;
}


int
generic_register_convertible_not (num)
     int num;
{
  return 0;
}
  
/* Disassembler */

/* Pointer to the target-dependent disassembly function.  */
int (*tm_print_insn) (bfd_vma, disassemble_info *);
disassemble_info tm_print_insn_info;



/* Set the dynamic target-system-dependant parameters (architecture,
   byte-order) using information found in the BFD */

void
set_gdbarch_from_file (abfd)
     bfd *abfd;
{
  if (GDB_MULTI_ARCH)
    {
      struct gdbarch_info info;
      memset (&info, 0, sizeof info);
      info.abfd = abfd;
      gdbarch_update (info);
      return;
    }
  set_architecture_from_file (abfd);
  set_endian_from_file (abfd);
}


#if defined (CALL_DUMMY)
/* FIXME - this should go away */
LONGEST call_dummy_words[] = CALL_DUMMY;
int sizeof_call_dummy_words = sizeof (call_dummy_words);
#endif


/* Initialize the current architecture.  */
void
initialize_current_architecture ()
{
  if (GDB_MULTI_ARCH)
    {
      struct gdbarch_init_registration *rego;
      const struct bfd_arch_info *chosen = NULL;
      for (rego = gdbarch_init_registrary; rego != NULL; rego = rego->next)
	{
	  const struct bfd_arch_info *ap
	    = bfd_lookup_arch (rego->bfd_architecture, 0);

	  /* Choose the first architecture alphabetically.  */
	  if (chosen == NULL
	      || strcmp (ap->printable_name, chosen->printable_name) < 0)
	    chosen = ap;
	}

      if (chosen != NULL)
	{
	  struct gdbarch_info info;
	  memset (&info, 0, sizeof info);
	  info.bfd_arch_info = chosen;
	  gdbarch_update (info);
	}
    }
}

extern void _initialize_gdbarch (void);
void
_initialize_gdbarch ()
{
  struct cmd_list_element *c;

  add_prefix_cmd ("endian", class_support, set_endian,
		  "Set endianness of target.",
		  &endianlist, "set endian ", 0, &setlist);
  add_cmd ("big", class_support, set_endian_big,
	   "Set target as being big endian.", &endianlist);
  add_cmd ("little", class_support, set_endian_little,
	   "Set target as being little endian.", &endianlist);
  add_cmd ("auto", class_support, set_endian_auto,
	   "Select target endianness automatically.", &endianlist);
  add_cmd ("endian", class_support, show_endian,
	   "Show endianness of target.", &showlist);

  add_cmd ("architecture", class_support, set_architecture,
	   "Set architecture of target.", &setlist);
  add_alias_cmd ("processor", "architecture", class_support, 1, &setlist);
  add_cmd ("architecture", class_support, show_architecture,
	   "Show architecture of target.", &showlist);
  add_cmd ("architecture", class_support, info_architecture,
	   "List supported target architectures", &infolist);

  INIT_DISASSEMBLE_INFO_NO_ARCH (tm_print_insn_info, gdb_stdout, (fprintf_ftype)fprintf_filtered);
  tm_print_insn_info.flavour = bfd_target_unknown_flavour;
  tm_print_insn_info.read_memory_func = dis_asm_read_memory;
  tm_print_insn_info.memory_error_func = dis_asm_memory_error;
  tm_print_insn_info.print_address_func = dis_asm_print_address;

  add_show_from_set (add_set_cmd ("arch",
				  class_maintenance,
				  var_zinteger,
				  (char *)&gdbarch_debug,
				  "Set architecture debugging.\n\
When non-zero, architecture debugging is enabled.", &setdebuglist),
		     &showdebuglist);
  c = add_set_cmd ("archdebug",
		   class_maintenance,
		   var_zinteger,
		   (char *)&gdbarch_debug,
		   "Set architecture debugging.\n\
When non-zero, architecture debugging is enabled.", &setlist);

  deprecate_cmd (c, "set debug arch");
  deprecate_cmd (add_show_from_set (c, &showlist), "show debug arch");
}
