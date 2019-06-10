/* *INDENT-OFF* */ /* THIS FILE IS GENERATED -*- buffer-read-only: t -*- */
/* vi:set ro: */

/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2017 Free Software Foundation, Inc.

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

/* This file was created with the aid of ``gdbarch.sh''.

   The Bourne shell script ``gdbarch.sh'' creates the files
   ``new-gdbarch.c'' and ``new-gdbarch.h and then compares them
   against the existing ``gdbarch.[hc]''.  Any differences found
   being reported.

   If editing this file, please also run gdbarch.sh and merge any
   changes into that script. Conversely, when making sweeping changes
   to this file, modifying gdbarch.sh and using its output may prove
   easier.  */


#include "defs.h"
#include "arch-utils.h"

#include "gdbcmd.h"
#include "inferior.h" 
#include "symcat.h"

#include "floatformat.h"
#include "reggroups.h"
#include "osabi.h"
#include "gdb_obstack.h"
#include "observer.h"
#include "regcache.h"
#include "objfiles.h"
#include "auxv.h"

/* Static function declarations */

static void alloc_gdbarch_data (struct gdbarch *);

/* Non-zero if we want to trace architecture code.  */

#ifndef GDBARCH_DEBUG
#define GDBARCH_DEBUG 0
#endif
unsigned int gdbarch_debug = GDBARCH_DEBUG;
static void
show_gdbarch_debug (struct ui_file *file, int from_tty,
                    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Architecture debugging is %s.\n"), value);
}

static const char *
pformat (const struct floatformat **format)
{
  if (format == NULL)
    return "(null)";
  else
    /* Just print out one of them - this is only for diagnostics.  */
    return format[0]->name;
}

static const char *
pstring (const char *string)
{
  if (string == NULL)
    return "(null)";
  return string;
}

static const char *
pstring_ptr (char **string)
{
  if (string == NULL || *string == NULL)
    return "(null)";
  return *string;
}

/* Helper function to print a list of strings, represented as "const
   char *const *".  The list is printed comma-separated.  */

static const char *
pstring_list (const char *const *list)
{
  static char ret[100];
  const char *const *p;
  size_t offset = 0;

  if (list == NULL)
    return "(null)";

  ret[0] = '\0';
  for (p = list; *p != NULL && offset < sizeof (ret); ++p)
    {
      size_t s = xsnprintf (ret + offset, sizeof (ret) - offset, "%s, ", *p);
      offset += 2 + s;
    }

  if (offset > 0)
    {
      gdb_assert (offset - 2 < sizeof (ret));
      ret[offset - 2] = '\0';
    }

  return ret;
}


/* Maintain the struct gdbarch object.  */

struct gdbarch
{
  /* Has this architecture been fully initialized?  */
  int initialized_p;

  /* An obstack bound to the lifetime of the architecture.  */
  struct obstack *obstack;

  /* basic architectural information.  */
  const struct bfd_arch_info * bfd_arch_info;
  enum bfd_endian byte_order;
  enum bfd_endian byte_order_for_code;
  enum gdb_osabi osabi;
  const struct target_desc * target_desc;

  /* target specific vector.  */
  struct gdbarch_tdep *tdep;
  gdbarch_dump_tdep_ftype *dump_tdep;

  /* per-architecture data-pointers.  */
  unsigned nr_data;
  void **data;

  /* Multi-arch values.

     When extending this structure you must:

     Add the field below.

     Declare set/get functions and define the corresponding
     macro in gdbarch.h.

     gdbarch_alloc(): If zero/NULL is not a suitable default,
     initialize the new field.

     verify_gdbarch(): Confirm that the target updated the field
     correctly.

     gdbarch_dump(): Add a fprintf_unfiltered call so that the new
     field is dumped out

     get_gdbarch(): Implement the set/get functions (probably using
     the macro's as shortcuts).

     */

  int bits_big_endian;
  int short_bit;
  int int_bit;
  int long_bit;
  int long_long_bit;
  int long_long_align_bit;
  int half_bit;
  const struct floatformat ** half_format;
  int float_bit;
  const struct floatformat ** float_format;
  int double_bit;
  const struct floatformat ** double_format;
  int long_double_bit;
  const struct floatformat ** long_double_format;
  int wchar_bit;
  int wchar_signed;
  gdbarch_floatformat_for_type_ftype *floatformat_for_type;
  int ptr_bit;
  int addr_bit;
  int dwarf2_addr_size;
  int char_signed;
  gdbarch_read_pc_ftype *read_pc;
  gdbarch_write_pc_ftype *write_pc;
  gdbarch_virtual_frame_pointer_ftype *virtual_frame_pointer;
  gdbarch_pseudo_register_read_ftype *pseudo_register_read;
  gdbarch_pseudo_register_read_value_ftype *pseudo_register_read_value;
  gdbarch_pseudo_register_write_ftype *pseudo_register_write;
  int num_regs;
  int num_pseudo_regs;
  gdbarch_ax_pseudo_register_collect_ftype *ax_pseudo_register_collect;
  gdbarch_ax_pseudo_register_push_stack_ftype *ax_pseudo_register_push_stack;
  gdbarch_handle_segmentation_fault_ftype *handle_segmentation_fault;
  int sp_regnum;
  int pc_regnum;
  int ps_regnum;
  int fp0_regnum;
  gdbarch_stab_reg_to_regnum_ftype *stab_reg_to_regnum;
  gdbarch_ecoff_reg_to_regnum_ftype *ecoff_reg_to_regnum;
  gdbarch_sdb_reg_to_regnum_ftype *sdb_reg_to_regnum;
  gdbarch_dwarf2_reg_to_regnum_ftype *dwarf2_reg_to_regnum;
  gdbarch_register_name_ftype *register_name;
  gdbarch_register_type_ftype *register_type;
  gdbarch_dummy_id_ftype *dummy_id;
  int deprecated_fp_regnum;
  gdbarch_push_dummy_call_ftype *push_dummy_call;
  int call_dummy_location;
  gdbarch_push_dummy_code_ftype *push_dummy_code;
  gdbarch_code_of_frame_writable_ftype *code_of_frame_writable;
  gdbarch_print_registers_info_ftype *print_registers_info;
  gdbarch_print_float_info_ftype *print_float_info;
  gdbarch_print_vector_info_ftype *print_vector_info;
  gdbarch_register_sim_regno_ftype *register_sim_regno;
  gdbarch_cannot_fetch_register_ftype *cannot_fetch_register;
  gdbarch_cannot_store_register_ftype *cannot_store_register;
  gdbarch_get_longjmp_target_ftype *get_longjmp_target;
  int believe_pcc_promotion;
  gdbarch_convert_register_p_ftype *convert_register_p;
  gdbarch_register_to_value_ftype *register_to_value;
  gdbarch_value_to_register_ftype *value_to_register;
  gdbarch_value_from_register_ftype *value_from_register;
  gdbarch_pointer_to_address_ftype *pointer_to_address;
  gdbarch_address_to_pointer_ftype *address_to_pointer;
  gdbarch_integer_to_address_ftype *integer_to_address;
  gdbarch_return_value_ftype *return_value;
  gdbarch_return_in_first_hidden_param_p_ftype *return_in_first_hidden_param_p;
  gdbarch_skip_prologue_ftype *skip_prologue;
  gdbarch_skip_main_prologue_ftype *skip_main_prologue;
  gdbarch_skip_entrypoint_ftype *skip_entrypoint;
  gdbarch_inner_than_ftype *inner_than;
  gdbarch_breakpoint_from_pc_ftype *breakpoint_from_pc;
  gdbarch_breakpoint_kind_from_pc_ftype *breakpoint_kind_from_pc;
  gdbarch_sw_breakpoint_from_kind_ftype *sw_breakpoint_from_kind;
  gdbarch_breakpoint_kind_from_current_state_ftype *breakpoint_kind_from_current_state;
  gdbarch_adjust_breakpoint_address_ftype *adjust_breakpoint_address;
  gdbarch_memory_insert_breakpoint_ftype *memory_insert_breakpoint;
  gdbarch_memory_remove_breakpoint_ftype *memory_remove_breakpoint;
  CORE_ADDR decr_pc_after_break;
  CORE_ADDR deprecated_function_start_offset;
  gdbarch_remote_register_number_ftype *remote_register_number;
  gdbarch_fetch_tls_load_module_address_ftype *fetch_tls_load_module_address;
  CORE_ADDR frame_args_skip;
  gdbarch_unwind_pc_ftype *unwind_pc;
  gdbarch_unwind_sp_ftype *unwind_sp;
  gdbarch_frame_num_args_ftype *frame_num_args;
  gdbarch_frame_align_ftype *frame_align;
  gdbarch_stabs_argument_has_addr_ftype *stabs_argument_has_addr;
  int frame_red_zone_size;
  gdbarch_convert_from_func_ptr_addr_ftype *convert_from_func_ptr_addr;
  gdbarch_addr_bits_remove_ftype *addr_bits_remove;
  gdbarch_software_single_step_ftype *software_single_step;
  gdbarch_single_step_through_delay_ftype *single_step_through_delay;
  gdbarch_print_insn_ftype *print_insn;
  gdbarch_skip_trampoline_code_ftype *skip_trampoline_code;
  gdbarch_skip_solib_resolver_ftype *skip_solib_resolver;
  gdbarch_in_solib_return_trampoline_ftype *in_solib_return_trampoline;
  gdbarch_stack_frame_destroyed_p_ftype *stack_frame_destroyed_p;
  gdbarch_elf_make_msymbol_special_ftype *elf_make_msymbol_special;
  gdbarch_coff_make_msymbol_special_ftype *coff_make_msymbol_special;
  gdbarch_make_symbol_special_ftype *make_symbol_special;
  gdbarch_adjust_dwarf2_addr_ftype *adjust_dwarf2_addr;
  gdbarch_adjust_dwarf2_line_ftype *adjust_dwarf2_line;
  int cannot_step_breakpoint;
  int have_nonsteppable_watchpoint;
  gdbarch_address_class_type_flags_ftype *address_class_type_flags;
  gdbarch_address_class_type_flags_to_name_ftype *address_class_type_flags_to_name;
  gdbarch_address_class_name_to_type_flags_ftype *address_class_name_to_type_flags;
  gdbarch_register_reggroup_p_ftype *register_reggroup_p;
  gdbarch_fetch_pointer_argument_ftype *fetch_pointer_argument;
  gdbarch_iterate_over_regset_sections_ftype *iterate_over_regset_sections;
  gdbarch_make_corefile_notes_ftype *make_corefile_notes;
  gdbarch_elfcore_write_linux_prpsinfo_ftype *elfcore_write_linux_prpsinfo;
  gdbarch_find_memory_regions_ftype *find_memory_regions;
  gdbarch_core_xfer_shared_libraries_ftype *core_xfer_shared_libraries;
  gdbarch_core_xfer_shared_libraries_aix_ftype *core_xfer_shared_libraries_aix;
  gdbarch_core_pid_to_str_ftype *core_pid_to_str;
  gdbarch_core_thread_name_ftype *core_thread_name;
  const char * gcore_bfd_target;
  int vtable_function_descriptors;
  int vbit_in_delta;
  gdbarch_skip_permanent_breakpoint_ftype *skip_permanent_breakpoint;
  ULONGEST max_insn_length;
  gdbarch_displaced_step_copy_insn_ftype *displaced_step_copy_insn;
  gdbarch_displaced_step_hw_singlestep_ftype *displaced_step_hw_singlestep;
  gdbarch_displaced_step_fixup_ftype *displaced_step_fixup;
  gdbarch_displaced_step_free_closure_ftype *displaced_step_free_closure;
  gdbarch_displaced_step_location_ftype *displaced_step_location;
  gdbarch_relocate_instruction_ftype *relocate_instruction;
  gdbarch_overlay_update_ftype *overlay_update;
  gdbarch_core_read_description_ftype *core_read_description;
  gdbarch_static_transform_name_ftype *static_transform_name;
  int sofun_address_maybe_missing;
  gdbarch_process_record_ftype *process_record;
  gdbarch_process_record_signal_ftype *process_record_signal;
  gdbarch_gdb_signal_from_target_ftype *gdb_signal_from_target;
  gdbarch_gdb_signal_to_target_ftype *gdb_signal_to_target;
  gdbarch_get_siginfo_type_ftype *get_siginfo_type;
  gdbarch_record_special_symbol_ftype *record_special_symbol;
  gdbarch_get_syscall_number_ftype *get_syscall_number;
  const char * xml_syscall_file;
  struct syscalls_info * syscalls_info;
  const char *const * stap_integer_prefixes;
  const char *const * stap_integer_suffixes;
  const char *const * stap_register_prefixes;
  const char *const * stap_register_suffixes;
  const char *const * stap_register_indirection_prefixes;
  const char *const * stap_register_indirection_suffixes;
  const char * stap_gdb_register_prefix;
  const char * stap_gdb_register_suffix;
  gdbarch_stap_is_single_operand_ftype *stap_is_single_operand;
  gdbarch_stap_parse_special_token_ftype *stap_parse_special_token;
  gdbarch_dtrace_parse_probe_argument_ftype *dtrace_parse_probe_argument;
  gdbarch_dtrace_probe_is_enabled_ftype *dtrace_probe_is_enabled;
  gdbarch_dtrace_enable_probe_ftype *dtrace_enable_probe;
  gdbarch_dtrace_disable_probe_ftype *dtrace_disable_probe;
  int has_global_solist;
  int has_global_breakpoints;
  gdbarch_has_shared_address_space_ftype *has_shared_address_space;
  gdbarch_fast_tracepoint_valid_at_ftype *fast_tracepoint_valid_at;
  gdbarch_guess_tracepoint_registers_ftype *guess_tracepoint_registers;
  gdbarch_auto_charset_ftype *auto_charset;
  gdbarch_auto_wide_charset_ftype *auto_wide_charset;
  const char * solib_symbols_extension;
  int has_dos_based_file_system;
  gdbarch_gen_return_address_ftype *gen_return_address;
  gdbarch_info_proc_ftype *info_proc;
  gdbarch_core_info_proc_ftype *core_info_proc;
  gdbarch_iterate_over_objfiles_in_search_order_ftype *iterate_over_objfiles_in_search_order;
  struct ravenscar_arch_ops * ravenscar_ops;
  gdbarch_insn_is_call_ftype *insn_is_call;
  gdbarch_insn_is_ret_ftype *insn_is_ret;
  gdbarch_insn_is_jump_ftype *insn_is_jump;
  gdbarch_auxv_parse_ftype *auxv_parse;
  gdbarch_print_auxv_entry_ftype *print_auxv_entry;
  gdbarch_vsyscall_range_ftype *vsyscall_range;
  gdbarch_infcall_mmap_ftype *infcall_mmap;
  gdbarch_infcall_munmap_ftype *infcall_munmap;
  gdbarch_gcc_target_options_ftype *gcc_target_options;
  gdbarch_gnu_triplet_regexp_ftype *gnu_triplet_regexp;
  gdbarch_addressable_memory_unit_size_ftype *addressable_memory_unit_size;
  char ** disassembler_options;
  const disasm_options_t * valid_disassembler_options;
};

/* Create a new ``struct gdbarch'' based on information provided by
   ``struct gdbarch_info''.  */

struct gdbarch *
gdbarch_alloc (const struct gdbarch_info *info,
               struct gdbarch_tdep *tdep)
{
  struct gdbarch *gdbarch;

  /* Create an obstack for allocating all the per-architecture memory,
     then use that to allocate the architecture vector.  */
  struct obstack *obstack = XNEW (struct obstack);
  obstack_init (obstack);
  gdbarch = XOBNEW (obstack, struct gdbarch);
  memset (gdbarch, 0, sizeof (*gdbarch));
  gdbarch->obstack = obstack;

  alloc_gdbarch_data (gdbarch);

  gdbarch->tdep = tdep;

  gdbarch->bfd_arch_info = info->bfd_arch_info;
  gdbarch->byte_order = info->byte_order;
  gdbarch->byte_order_for_code = info->byte_order_for_code;
  gdbarch->osabi = info->osabi;
  gdbarch->target_desc = info->target_desc;

  /* Force the explicit initialization of these.  */
  gdbarch->bits_big_endian = (gdbarch->byte_order == BFD_ENDIAN_BIG);
  gdbarch->short_bit = 2*TARGET_CHAR_BIT;
  gdbarch->int_bit = 4*TARGET_CHAR_BIT;
  gdbarch->long_bit = 4*TARGET_CHAR_BIT;
  gdbarch->long_long_bit = 2*gdbarch->long_bit;
  gdbarch->long_long_align_bit = 2*gdbarch->long_bit;
  gdbarch->half_bit = 2*TARGET_CHAR_BIT;
  gdbarch->float_bit = 4*TARGET_CHAR_BIT;
  gdbarch->double_bit = 8*TARGET_CHAR_BIT;
  gdbarch->long_double_bit = 8*TARGET_CHAR_BIT;
  gdbarch->wchar_bit = 4*TARGET_CHAR_BIT;
  gdbarch->wchar_signed = -1;
  gdbarch->floatformat_for_type = default_floatformat_for_type;
  gdbarch->ptr_bit = gdbarch->int_bit;
  gdbarch->char_signed = -1;
  gdbarch->virtual_frame_pointer = legacy_virtual_frame_pointer;
  gdbarch->num_regs = -1;
  gdbarch->sp_regnum = -1;
  gdbarch->pc_regnum = -1;
  gdbarch->ps_regnum = -1;
  gdbarch->fp0_regnum = -1;
  gdbarch->stab_reg_to_regnum = no_op_reg_to_regnum;
  gdbarch->ecoff_reg_to_regnum = no_op_reg_to_regnum;
  gdbarch->sdb_reg_to_regnum = no_op_reg_to_regnum;
  gdbarch->dwarf2_reg_to_regnum = no_op_reg_to_regnum;
  gdbarch->deprecated_fp_regnum = -1;
  gdbarch->call_dummy_location = AT_ENTRY_POINT;
  gdbarch->code_of_frame_writable = default_code_of_frame_writable;
  gdbarch->print_registers_info = default_print_registers_info;
  gdbarch->print_float_info = default_print_float_info;
  gdbarch->register_sim_regno = legacy_register_sim_regno;
  gdbarch->cannot_fetch_register = cannot_register_not;
  gdbarch->cannot_store_register = cannot_register_not;
  gdbarch->convert_register_p = generic_convert_register_p;
  gdbarch->value_from_register = default_value_from_register;
  gdbarch->pointer_to_address = unsigned_pointer_to_address;
  gdbarch->address_to_pointer = unsigned_address_to_pointer;
  gdbarch->return_in_first_hidden_param_p = default_return_in_first_hidden_param_p;
  gdbarch->breakpoint_from_pc = default_breakpoint_from_pc;
  gdbarch->sw_breakpoint_from_kind = NULL;
  gdbarch->breakpoint_kind_from_current_state = default_breakpoint_kind_from_current_state;
  gdbarch->memory_insert_breakpoint = default_memory_insert_breakpoint;
  gdbarch->memory_remove_breakpoint = default_memory_remove_breakpoint;
  gdbarch->remote_register_number = default_remote_register_number;
  gdbarch->stabs_argument_has_addr = default_stabs_argument_has_addr;
  gdbarch->convert_from_func_ptr_addr = convert_from_func_ptr_addr_identity;
  gdbarch->addr_bits_remove = core_addr_identity;
  gdbarch->skip_trampoline_code = generic_skip_trampoline_code;
  gdbarch->skip_solib_resolver = generic_skip_solib_resolver;
  gdbarch->in_solib_return_trampoline = generic_in_solib_return_trampoline;
  gdbarch->stack_frame_destroyed_p = generic_stack_frame_destroyed_p;
  gdbarch->coff_make_msymbol_special = default_coff_make_msymbol_special;
  gdbarch->make_symbol_special = default_make_symbol_special;
  gdbarch->adjust_dwarf2_addr = default_adjust_dwarf2_addr;
  gdbarch->adjust_dwarf2_line = default_adjust_dwarf2_line;
  gdbarch->register_reggroup_p = default_register_reggroup_p;
  gdbarch->skip_permanent_breakpoint = default_skip_permanent_breakpoint;
  gdbarch->displaced_step_hw_singlestep = default_displaced_step_hw_singlestep;
  gdbarch->displaced_step_fixup = NULL;
  gdbarch->displaced_step_free_closure = NULL;
  gdbarch->displaced_step_location = NULL;
  gdbarch->relocate_instruction = NULL;
  gdbarch->has_shared_address_space = default_has_shared_address_space;
  gdbarch->fast_tracepoint_valid_at = default_fast_tracepoint_valid_at;
  gdbarch->guess_tracepoint_registers = default_guess_tracepoint_registers;
  gdbarch->auto_charset = default_auto_charset;
  gdbarch->auto_wide_charset = default_auto_wide_charset;
  gdbarch->gen_return_address = default_gen_return_address;
  gdbarch->iterate_over_objfiles_in_search_order = default_iterate_over_objfiles_in_search_order;
  gdbarch->ravenscar_ops = NULL;
  gdbarch->insn_is_call = default_insn_is_call;
  gdbarch->insn_is_ret = default_insn_is_ret;
  gdbarch->insn_is_jump = default_insn_is_jump;
  gdbarch->print_auxv_entry = default_print_auxv_entry;
  gdbarch->vsyscall_range = default_vsyscall_range;
  gdbarch->infcall_mmap = default_infcall_mmap;
  gdbarch->infcall_munmap = default_infcall_munmap;
  gdbarch->gcc_target_options = default_gcc_target_options;
  gdbarch->gnu_triplet_regexp = default_gnu_triplet_regexp;
  gdbarch->addressable_memory_unit_size = default_addressable_memory_unit_size;
  /* gdbarch_alloc() */

  return gdbarch;
}


/* Allocate extra space using the per-architecture obstack.  */

void *
gdbarch_obstack_zalloc (struct gdbarch *arch, long size)
{
  void *data = obstack_alloc (arch->obstack, size);

  memset (data, 0, size);
  return data;
}

/* See gdbarch.h.  */

char *
gdbarch_obstack_strdup (struct gdbarch *arch, const char *string)
{
  return obstack_strdup (arch->obstack, string);
}


/* Free a gdbarch struct.  This should never happen in normal
   operation --- once you've created a gdbarch, you keep it around.
   However, if an architecture's init function encounters an error
   building the structure, it may need to clean up a partially
   constructed gdbarch.  */

void
gdbarch_free (struct gdbarch *arch)
{
  struct obstack *obstack;

  gdb_assert (arch != NULL);
  gdb_assert (!arch->initialized_p);
  obstack = arch->obstack;
  obstack_free (obstack, 0); /* Includes the ARCH.  */
  xfree (obstack);
}


/* Ensure that all values in a GDBARCH are reasonable.  */

static void
verify_gdbarch (struct gdbarch *gdbarch)
{
  string_file log;

  /* fundamental */
  if (gdbarch->byte_order == BFD_ENDIAN_UNKNOWN)
    log.puts ("\n\tbyte-order");
  if (gdbarch->bfd_arch_info == NULL)
    log.puts ("\n\tbfd_arch_info");
  /* Check those that need to be defined for the given multi-arch level.  */
  /* Skip verify of bits_big_endian, invalid_p == 0 */
  /* Skip verify of short_bit, invalid_p == 0 */
  /* Skip verify of int_bit, invalid_p == 0 */
  /* Skip verify of long_bit, invalid_p == 0 */
  /* Skip verify of long_long_bit, invalid_p == 0 */
  /* Skip verify of long_long_align_bit, invalid_p == 0 */
  /* Skip verify of half_bit, invalid_p == 0 */
  if (gdbarch->half_format == 0)
    gdbarch->half_format = floatformats_ieee_half;
  /* Skip verify of float_bit, invalid_p == 0 */
  if (gdbarch->float_format == 0)
    gdbarch->float_format = floatformats_ieee_single;
  /* Skip verify of double_bit, invalid_p == 0 */
  if (gdbarch->double_format == 0)
    gdbarch->double_format = floatformats_ieee_double;
  /* Skip verify of long_double_bit, invalid_p == 0 */
  if (gdbarch->long_double_format == 0)
    gdbarch->long_double_format = floatformats_ieee_double;
  /* Skip verify of wchar_bit, invalid_p == 0 */
  if (gdbarch->wchar_signed == -1)
    gdbarch->wchar_signed = 1;
  /* Skip verify of floatformat_for_type, invalid_p == 0 */
  /* Skip verify of ptr_bit, invalid_p == 0 */
  if (gdbarch->addr_bit == 0)
    gdbarch->addr_bit = gdbarch_ptr_bit (gdbarch);
  if (gdbarch->dwarf2_addr_size == 0)
    gdbarch->dwarf2_addr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  if (gdbarch->char_signed == -1)
    gdbarch->char_signed = 1;
  /* Skip verify of read_pc, has predicate.  */
  /* Skip verify of write_pc, has predicate.  */
  /* Skip verify of virtual_frame_pointer, invalid_p == 0 */
  /* Skip verify of pseudo_register_read, has predicate.  */
  /* Skip verify of pseudo_register_read_value, has predicate.  */
  /* Skip verify of pseudo_register_write, has predicate.  */
  if (gdbarch->num_regs == -1)
    log.puts ("\n\tnum_regs");
  /* Skip verify of num_pseudo_regs, invalid_p == 0 */
  /* Skip verify of ax_pseudo_register_collect, has predicate.  */
  /* Skip verify of ax_pseudo_register_push_stack, has predicate.  */
  /* Skip verify of handle_segmentation_fault, has predicate.  */
  /* Skip verify of sp_regnum, invalid_p == 0 */
  /* Skip verify of pc_regnum, invalid_p == 0 */
  /* Skip verify of ps_regnum, invalid_p == 0 */
  /* Skip verify of fp0_regnum, invalid_p == 0 */
  /* Skip verify of stab_reg_to_regnum, invalid_p == 0 */
  /* Skip verify of ecoff_reg_to_regnum, invalid_p == 0 */
  /* Skip verify of sdb_reg_to_regnum, invalid_p == 0 */
  /* Skip verify of dwarf2_reg_to_regnum, invalid_p == 0 */
  if (gdbarch->register_name == 0)
    log.puts ("\n\tregister_name");
  /* Skip verify of register_type, has predicate.  */
  /* Skip verify of dummy_id, has predicate.  */
  /* Skip verify of deprecated_fp_regnum, invalid_p == 0 */
  /* Skip verify of push_dummy_call, has predicate.  */
  /* Skip verify of call_dummy_location, invalid_p == 0 */
  /* Skip verify of push_dummy_code, has predicate.  */
  /* Skip verify of code_of_frame_writable, invalid_p == 0 */
  /* Skip verify of print_registers_info, invalid_p == 0 */
  /* Skip verify of print_float_info, invalid_p == 0 */
  /* Skip verify of print_vector_info, has predicate.  */
  /* Skip verify of register_sim_regno, invalid_p == 0 */
  /* Skip verify of cannot_fetch_register, invalid_p == 0 */
  /* Skip verify of cannot_store_register, invalid_p == 0 */
  /* Skip verify of get_longjmp_target, has predicate.  */
  /* Skip verify of convert_register_p, invalid_p == 0 */
  /* Skip verify of value_from_register, invalid_p == 0 */
  /* Skip verify of pointer_to_address, invalid_p == 0 */
  /* Skip verify of address_to_pointer, invalid_p == 0 */
  /* Skip verify of integer_to_address, has predicate.  */
  /* Skip verify of return_value, has predicate.  */
  /* Skip verify of return_in_first_hidden_param_p, invalid_p == 0 */
  if (gdbarch->skip_prologue == 0)
    log.puts ("\n\tskip_prologue");
  /* Skip verify of skip_main_prologue, has predicate.  */
  /* Skip verify of skip_entrypoint, has predicate.  */
  if (gdbarch->inner_than == 0)
    log.puts ("\n\tinner_than");
  /* Skip verify of breakpoint_from_pc, invalid_p == 0 */
  if (gdbarch->breakpoint_kind_from_pc == 0)
    log.puts ("\n\tbreakpoint_kind_from_pc");
  /* Skip verify of sw_breakpoint_from_kind, invalid_p == 0 */
  /* Skip verify of breakpoint_kind_from_current_state, invalid_p == 0 */
  /* Skip verify of adjust_breakpoint_address, has predicate.  */
  /* Skip verify of memory_insert_breakpoint, invalid_p == 0 */
  /* Skip verify of memory_remove_breakpoint, invalid_p == 0 */
  /* Skip verify of decr_pc_after_break, invalid_p == 0 */
  /* Skip verify of deprecated_function_start_offset, invalid_p == 0 */
  /* Skip verify of remote_register_number, invalid_p == 0 */
  /* Skip verify of fetch_tls_load_module_address, has predicate.  */
  /* Skip verify of frame_args_skip, invalid_p == 0 */
  /* Skip verify of unwind_pc, has predicate.  */
  /* Skip verify of unwind_sp, has predicate.  */
  /* Skip verify of frame_num_args, has predicate.  */
  /* Skip verify of frame_align, has predicate.  */
  /* Skip verify of stabs_argument_has_addr, invalid_p == 0 */
  /* Skip verify of convert_from_func_ptr_addr, invalid_p == 0 */
  /* Skip verify of addr_bits_remove, invalid_p == 0 */
  /* Skip verify of software_single_step, has predicate.  */
  /* Skip verify of single_step_through_delay, has predicate.  */
  if (gdbarch->print_insn == 0)
    log.puts ("\n\tprint_insn");
  /* Skip verify of skip_trampoline_code, invalid_p == 0 */
  /* Skip verify of skip_solib_resolver, invalid_p == 0 */
  /* Skip verify of in_solib_return_trampoline, invalid_p == 0 */
  /* Skip verify of stack_frame_destroyed_p, invalid_p == 0 */
  /* Skip verify of elf_make_msymbol_special, has predicate.  */
  /* Skip verify of coff_make_msymbol_special, invalid_p == 0 */
  /* Skip verify of make_symbol_special, invalid_p == 0 */
  /* Skip verify of adjust_dwarf2_addr, invalid_p == 0 */
  /* Skip verify of adjust_dwarf2_line, invalid_p == 0 */
  /* Skip verify of cannot_step_breakpoint, invalid_p == 0 */
  /* Skip verify of have_nonsteppable_watchpoint, invalid_p == 0 */
  /* Skip verify of address_class_type_flags, has predicate.  */
  /* Skip verify of address_class_type_flags_to_name, has predicate.  */
  /* Skip verify of address_class_name_to_type_flags, has predicate.  */
  /* Skip verify of register_reggroup_p, invalid_p == 0 */
  /* Skip verify of fetch_pointer_argument, has predicate.  */
  /* Skip verify of iterate_over_regset_sections, has predicate.  */
  /* Skip verify of make_corefile_notes, has predicate.  */
  /* Skip verify of elfcore_write_linux_prpsinfo, has predicate.  */
  /* Skip verify of find_memory_regions, has predicate.  */
  /* Skip verify of core_xfer_shared_libraries, has predicate.  */
  /* Skip verify of core_xfer_shared_libraries_aix, has predicate.  */
  /* Skip verify of core_pid_to_str, has predicate.  */
  /* Skip verify of core_thread_name, has predicate.  */
  /* Skip verify of gcore_bfd_target, has predicate.  */
  /* Skip verify of vtable_function_descriptors, invalid_p == 0 */
  /* Skip verify of vbit_in_delta, invalid_p == 0 */
  /* Skip verify of skip_permanent_breakpoint, invalid_p == 0 */
  /* Skip verify of max_insn_length, has predicate.  */
  /* Skip verify of displaced_step_copy_insn, has predicate.  */
  /* Skip verify of displaced_step_hw_singlestep, invalid_p == 0 */
  /* Skip verify of displaced_step_fixup, has predicate.  */
  if ((! gdbarch->displaced_step_free_closure) != (! gdbarch->displaced_step_copy_insn))
    log.puts ("\n\tdisplaced_step_free_closure");
  if ((! gdbarch->displaced_step_location) != (! gdbarch->displaced_step_copy_insn))
    log.puts ("\n\tdisplaced_step_location");
  /* Skip verify of relocate_instruction, has predicate.  */
  /* Skip verify of overlay_update, has predicate.  */
  /* Skip verify of core_read_description, has predicate.  */
  /* Skip verify of static_transform_name, has predicate.  */
  /* Skip verify of sofun_address_maybe_missing, invalid_p == 0 */
  /* Skip verify of process_record, has predicate.  */
  /* Skip verify of process_record_signal, has predicate.  */
  /* Skip verify of gdb_signal_from_target, has predicate.  */
  /* Skip verify of gdb_signal_to_target, has predicate.  */
  /* Skip verify of get_siginfo_type, has predicate.  */
  /* Skip verify of record_special_symbol, has predicate.  */
  /* Skip verify of get_syscall_number, has predicate.  */
  /* Skip verify of xml_syscall_file, invalid_p == 0 */
  /* Skip verify of syscalls_info, invalid_p == 0 */
  /* Skip verify of stap_integer_prefixes, invalid_p == 0 */
  /* Skip verify of stap_integer_suffixes, invalid_p == 0 */
  /* Skip verify of stap_register_prefixes, invalid_p == 0 */
  /* Skip verify of stap_register_suffixes, invalid_p == 0 */
  /* Skip verify of stap_register_indirection_prefixes, invalid_p == 0 */
  /* Skip verify of stap_register_indirection_suffixes, invalid_p == 0 */
  /* Skip verify of stap_gdb_register_prefix, invalid_p == 0 */
  /* Skip verify of stap_gdb_register_suffix, invalid_p == 0 */
  /* Skip verify of stap_is_single_operand, has predicate.  */
  /* Skip verify of stap_parse_special_token, has predicate.  */
  /* Skip verify of dtrace_parse_probe_argument, has predicate.  */
  /* Skip verify of dtrace_probe_is_enabled, has predicate.  */
  /* Skip verify of dtrace_enable_probe, has predicate.  */
  /* Skip verify of dtrace_disable_probe, has predicate.  */
  /* Skip verify of has_global_solist, invalid_p == 0 */
  /* Skip verify of has_global_breakpoints, invalid_p == 0 */
  /* Skip verify of has_shared_address_space, invalid_p == 0 */
  /* Skip verify of fast_tracepoint_valid_at, invalid_p == 0 */
  /* Skip verify of guess_tracepoint_registers, invalid_p == 0 */
  /* Skip verify of auto_charset, invalid_p == 0 */
  /* Skip verify of auto_wide_charset, invalid_p == 0 */
  /* Skip verify of has_dos_based_file_system, invalid_p == 0 */
  /* Skip verify of gen_return_address, invalid_p == 0 */
  /* Skip verify of info_proc, has predicate.  */
  /* Skip verify of core_info_proc, has predicate.  */
  /* Skip verify of iterate_over_objfiles_in_search_order, invalid_p == 0 */
  /* Skip verify of ravenscar_ops, invalid_p == 0 */
  /* Skip verify of insn_is_call, invalid_p == 0 */
  /* Skip verify of insn_is_ret, invalid_p == 0 */
  /* Skip verify of insn_is_jump, invalid_p == 0 */
  /* Skip verify of auxv_parse, has predicate.  */
  /* Skip verify of print_auxv_entry, invalid_p == 0 */
  /* Skip verify of vsyscall_range, invalid_p == 0 */
  /* Skip verify of infcall_mmap, invalid_p == 0 */
  /* Skip verify of infcall_munmap, invalid_p == 0 */
  /* Skip verify of gcc_target_options, invalid_p == 0 */
  /* Skip verify of gnu_triplet_regexp, invalid_p == 0 */
  /* Skip verify of addressable_memory_unit_size, invalid_p == 0 */
  /* Skip verify of disassembler_options, invalid_p == 0 */
  /* Skip verify of valid_disassembler_options, invalid_p == 0 */
  if (!log.empty ())
    internal_error (__FILE__, __LINE__,
                    _("verify_gdbarch: the following are invalid ...%s"),
                    log.c_str ());
}


/* Print out the details of the current architecture.  */

void
gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file)
{
  const char *gdb_nm_file = "<not-defined>";

#if defined (GDB_NM_FILE)
  gdb_nm_file = GDB_NM_FILE;
#endif
  fprintf_unfiltered (file,
                      "gdbarch_dump: GDB_NM_FILE = %s\n",
                      gdb_nm_file);
  fprintf_unfiltered (file,
                      "gdbarch_dump: addr_bit = %s\n",
                      plongest (gdbarch->addr_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: addr_bits_remove = <%s>\n",
                      host_address_to_string (gdbarch->addr_bits_remove));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_address_class_name_to_type_flags_p() = %d\n",
                      gdbarch_address_class_name_to_type_flags_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: address_class_name_to_type_flags = <%s>\n",
                      host_address_to_string (gdbarch->address_class_name_to_type_flags));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_address_class_type_flags_p() = %d\n",
                      gdbarch_address_class_type_flags_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: address_class_type_flags = <%s>\n",
                      host_address_to_string (gdbarch->address_class_type_flags));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_address_class_type_flags_to_name_p() = %d\n",
                      gdbarch_address_class_type_flags_to_name_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: address_class_type_flags_to_name = <%s>\n",
                      host_address_to_string (gdbarch->address_class_type_flags_to_name));
  fprintf_unfiltered (file,
                      "gdbarch_dump: address_to_pointer = <%s>\n",
                      host_address_to_string (gdbarch->address_to_pointer));
  fprintf_unfiltered (file,
                      "gdbarch_dump: addressable_memory_unit_size = <%s>\n",
                      host_address_to_string (gdbarch->addressable_memory_unit_size));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_adjust_breakpoint_address_p() = %d\n",
                      gdbarch_adjust_breakpoint_address_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: adjust_breakpoint_address = <%s>\n",
                      host_address_to_string (gdbarch->adjust_breakpoint_address));
  fprintf_unfiltered (file,
                      "gdbarch_dump: adjust_dwarf2_addr = <%s>\n",
                      host_address_to_string (gdbarch->adjust_dwarf2_addr));
  fprintf_unfiltered (file,
                      "gdbarch_dump: adjust_dwarf2_line = <%s>\n",
                      host_address_to_string (gdbarch->adjust_dwarf2_line));
  fprintf_unfiltered (file,
                      "gdbarch_dump: auto_charset = <%s>\n",
                      host_address_to_string (gdbarch->auto_charset));
  fprintf_unfiltered (file,
                      "gdbarch_dump: auto_wide_charset = <%s>\n",
                      host_address_to_string (gdbarch->auto_wide_charset));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_auxv_parse_p() = %d\n",
                      gdbarch_auxv_parse_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: auxv_parse = <%s>\n",
                      host_address_to_string (gdbarch->auxv_parse));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_ax_pseudo_register_collect_p() = %d\n",
                      gdbarch_ax_pseudo_register_collect_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ax_pseudo_register_collect = <%s>\n",
                      host_address_to_string (gdbarch->ax_pseudo_register_collect));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_ax_pseudo_register_push_stack_p() = %d\n",
                      gdbarch_ax_pseudo_register_push_stack_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ax_pseudo_register_push_stack = <%s>\n",
                      host_address_to_string (gdbarch->ax_pseudo_register_push_stack));
  fprintf_unfiltered (file,
                      "gdbarch_dump: believe_pcc_promotion = %s\n",
                      plongest (gdbarch->believe_pcc_promotion));
  fprintf_unfiltered (file,
                      "gdbarch_dump: bfd_arch_info = %s\n",
                      gdbarch_bfd_arch_info (gdbarch)->printable_name);
  fprintf_unfiltered (file,
                      "gdbarch_dump: bits_big_endian = %s\n",
                      plongest (gdbarch->bits_big_endian));
  fprintf_unfiltered (file,
                      "gdbarch_dump: breakpoint_from_pc = <%s>\n",
                      host_address_to_string (gdbarch->breakpoint_from_pc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: breakpoint_kind_from_current_state = <%s>\n",
                      host_address_to_string (gdbarch->breakpoint_kind_from_current_state));
  fprintf_unfiltered (file,
                      "gdbarch_dump: breakpoint_kind_from_pc = <%s>\n",
                      host_address_to_string (gdbarch->breakpoint_kind_from_pc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: byte_order = %s\n",
                      plongest (gdbarch->byte_order));
  fprintf_unfiltered (file,
                      "gdbarch_dump: byte_order_for_code = %s\n",
                      plongest (gdbarch->byte_order_for_code));
  fprintf_unfiltered (file,
                      "gdbarch_dump: call_dummy_location = %s\n",
                      plongest (gdbarch->call_dummy_location));
  fprintf_unfiltered (file,
                      "gdbarch_dump: cannot_fetch_register = <%s>\n",
                      host_address_to_string (gdbarch->cannot_fetch_register));
  fprintf_unfiltered (file,
                      "gdbarch_dump: cannot_step_breakpoint = %s\n",
                      plongest (gdbarch->cannot_step_breakpoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: cannot_store_register = <%s>\n",
                      host_address_to_string (gdbarch->cannot_store_register));
  fprintf_unfiltered (file,
                      "gdbarch_dump: char_signed = %s\n",
                      plongest (gdbarch->char_signed));
  fprintf_unfiltered (file,
                      "gdbarch_dump: code_of_frame_writable = <%s>\n",
                      host_address_to_string (gdbarch->code_of_frame_writable));
  fprintf_unfiltered (file,
                      "gdbarch_dump: coff_make_msymbol_special = <%s>\n",
                      host_address_to_string (gdbarch->coff_make_msymbol_special));
  fprintf_unfiltered (file,
                      "gdbarch_dump: convert_from_func_ptr_addr = <%s>\n",
                      host_address_to_string (gdbarch->convert_from_func_ptr_addr));
  fprintf_unfiltered (file,
                      "gdbarch_dump: convert_register_p = <%s>\n",
                      host_address_to_string (gdbarch->convert_register_p));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_info_proc_p() = %d\n",
                      gdbarch_core_info_proc_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_info_proc = <%s>\n",
                      host_address_to_string (gdbarch->core_info_proc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_pid_to_str_p() = %d\n",
                      gdbarch_core_pid_to_str_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_pid_to_str = <%s>\n",
                      host_address_to_string (gdbarch->core_pid_to_str));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_read_description_p() = %d\n",
                      gdbarch_core_read_description_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_read_description = <%s>\n",
                      host_address_to_string (gdbarch->core_read_description));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_thread_name_p() = %d\n",
                      gdbarch_core_thread_name_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_thread_name = <%s>\n",
                      host_address_to_string (gdbarch->core_thread_name));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_xfer_shared_libraries_p() = %d\n",
                      gdbarch_core_xfer_shared_libraries_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_xfer_shared_libraries = <%s>\n",
                      host_address_to_string (gdbarch->core_xfer_shared_libraries));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_core_xfer_shared_libraries_aix_p() = %d\n",
                      gdbarch_core_xfer_shared_libraries_aix_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: core_xfer_shared_libraries_aix = <%s>\n",
                      host_address_to_string (gdbarch->core_xfer_shared_libraries_aix));
  fprintf_unfiltered (file,
                      "gdbarch_dump: decr_pc_after_break = %s\n",
                      core_addr_to_string_nz (gdbarch->decr_pc_after_break));
  fprintf_unfiltered (file,
                      "gdbarch_dump: deprecated_fp_regnum = %s\n",
                      plongest (gdbarch->deprecated_fp_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: deprecated_function_start_offset = %s\n",
                      core_addr_to_string_nz (gdbarch->deprecated_function_start_offset));
  fprintf_unfiltered (file,
                      "gdbarch_dump: disassembler_options = %s\n",
                      pstring_ptr (gdbarch->disassembler_options));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_displaced_step_copy_insn_p() = %d\n",
                      gdbarch_displaced_step_copy_insn_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: displaced_step_copy_insn = <%s>\n",
                      host_address_to_string (gdbarch->displaced_step_copy_insn));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_displaced_step_fixup_p() = %d\n",
                      gdbarch_displaced_step_fixup_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: displaced_step_fixup = <%s>\n",
                      host_address_to_string (gdbarch->displaced_step_fixup));
  fprintf_unfiltered (file,
                      "gdbarch_dump: displaced_step_free_closure = <%s>\n",
                      host_address_to_string (gdbarch->displaced_step_free_closure));
  fprintf_unfiltered (file,
                      "gdbarch_dump: displaced_step_hw_singlestep = <%s>\n",
                      host_address_to_string (gdbarch->displaced_step_hw_singlestep));
  fprintf_unfiltered (file,
                      "gdbarch_dump: displaced_step_location = <%s>\n",
                      host_address_to_string (gdbarch->displaced_step_location));
  fprintf_unfiltered (file,
                      "gdbarch_dump: double_bit = %s\n",
                      plongest (gdbarch->double_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: double_format = %s\n",
                      pformat (gdbarch->double_format));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_dtrace_disable_probe_p() = %d\n",
                      gdbarch_dtrace_disable_probe_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dtrace_disable_probe = <%s>\n",
                      host_address_to_string (gdbarch->dtrace_disable_probe));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_dtrace_enable_probe_p() = %d\n",
                      gdbarch_dtrace_enable_probe_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dtrace_enable_probe = <%s>\n",
                      host_address_to_string (gdbarch->dtrace_enable_probe));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_dtrace_parse_probe_argument_p() = %d\n",
                      gdbarch_dtrace_parse_probe_argument_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dtrace_parse_probe_argument = <%s>\n",
                      host_address_to_string (gdbarch->dtrace_parse_probe_argument));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_dtrace_probe_is_enabled_p() = %d\n",
                      gdbarch_dtrace_probe_is_enabled_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dtrace_probe_is_enabled = <%s>\n",
                      host_address_to_string (gdbarch->dtrace_probe_is_enabled));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_dummy_id_p() = %d\n",
                      gdbarch_dummy_id_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dummy_id = <%s>\n",
                      host_address_to_string (gdbarch->dummy_id));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dwarf2_addr_size = %s\n",
                      plongest (gdbarch->dwarf2_addr_size));
  fprintf_unfiltered (file,
                      "gdbarch_dump: dwarf2_reg_to_regnum = <%s>\n",
                      host_address_to_string (gdbarch->dwarf2_reg_to_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ecoff_reg_to_regnum = <%s>\n",
                      host_address_to_string (gdbarch->ecoff_reg_to_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_elf_make_msymbol_special_p() = %d\n",
                      gdbarch_elf_make_msymbol_special_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: elf_make_msymbol_special = <%s>\n",
                      host_address_to_string (gdbarch->elf_make_msymbol_special));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_elfcore_write_linux_prpsinfo_p() = %d\n",
                      gdbarch_elfcore_write_linux_prpsinfo_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: elfcore_write_linux_prpsinfo = <%s>\n",
                      host_address_to_string (gdbarch->elfcore_write_linux_prpsinfo));
  fprintf_unfiltered (file,
                      "gdbarch_dump: fast_tracepoint_valid_at = <%s>\n",
                      host_address_to_string (gdbarch->fast_tracepoint_valid_at));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_fetch_pointer_argument_p() = %d\n",
                      gdbarch_fetch_pointer_argument_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: fetch_pointer_argument = <%s>\n",
                      host_address_to_string (gdbarch->fetch_pointer_argument));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_fetch_tls_load_module_address_p() = %d\n",
                      gdbarch_fetch_tls_load_module_address_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: fetch_tls_load_module_address = <%s>\n",
                      host_address_to_string (gdbarch->fetch_tls_load_module_address));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_find_memory_regions_p() = %d\n",
                      gdbarch_find_memory_regions_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: find_memory_regions = <%s>\n",
                      host_address_to_string (gdbarch->find_memory_regions));
  fprintf_unfiltered (file,
                      "gdbarch_dump: float_bit = %s\n",
                      plongest (gdbarch->float_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: float_format = %s\n",
                      pformat (gdbarch->float_format));
  fprintf_unfiltered (file,
                      "gdbarch_dump: floatformat_for_type = <%s>\n",
                      host_address_to_string (gdbarch->floatformat_for_type));
  fprintf_unfiltered (file,
                      "gdbarch_dump: fp0_regnum = %s\n",
                      plongest (gdbarch->fp0_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_frame_align_p() = %d\n",
                      gdbarch_frame_align_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: frame_align = <%s>\n",
                      host_address_to_string (gdbarch->frame_align));
  fprintf_unfiltered (file,
                      "gdbarch_dump: frame_args_skip = %s\n",
                      core_addr_to_string_nz (gdbarch->frame_args_skip));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_frame_num_args_p() = %d\n",
                      gdbarch_frame_num_args_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: frame_num_args = <%s>\n",
                      host_address_to_string (gdbarch->frame_num_args));
  fprintf_unfiltered (file,
                      "gdbarch_dump: frame_red_zone_size = %s\n",
                      plongest (gdbarch->frame_red_zone_size));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gcc_target_options = <%s>\n",
                      host_address_to_string (gdbarch->gcc_target_options));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_gcore_bfd_target_p() = %d\n",
                      gdbarch_gcore_bfd_target_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gcore_bfd_target = %s\n",
                      pstring (gdbarch->gcore_bfd_target));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_gdb_signal_from_target_p() = %d\n",
                      gdbarch_gdb_signal_from_target_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdb_signal_from_target = <%s>\n",
                      host_address_to_string (gdbarch->gdb_signal_from_target));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_gdb_signal_to_target_p() = %d\n",
                      gdbarch_gdb_signal_to_target_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdb_signal_to_target = <%s>\n",
                      host_address_to_string (gdbarch->gdb_signal_to_target));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gen_return_address = <%s>\n",
                      host_address_to_string (gdbarch->gen_return_address));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_get_longjmp_target_p() = %d\n",
                      gdbarch_get_longjmp_target_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: get_longjmp_target = <%s>\n",
                      host_address_to_string (gdbarch->get_longjmp_target));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_get_siginfo_type_p() = %d\n",
                      gdbarch_get_siginfo_type_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: get_siginfo_type = <%s>\n",
                      host_address_to_string (gdbarch->get_siginfo_type));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_get_syscall_number_p() = %d\n",
                      gdbarch_get_syscall_number_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: get_syscall_number = <%s>\n",
                      host_address_to_string (gdbarch->get_syscall_number));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gnu_triplet_regexp = <%s>\n",
                      host_address_to_string (gdbarch->gnu_triplet_regexp));
  fprintf_unfiltered (file,
                      "gdbarch_dump: guess_tracepoint_registers = <%s>\n",
                      host_address_to_string (gdbarch->guess_tracepoint_registers));
  fprintf_unfiltered (file,
                      "gdbarch_dump: half_bit = %s\n",
                      plongest (gdbarch->half_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: half_format = %s\n",
                      pformat (gdbarch->half_format));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_handle_segmentation_fault_p() = %d\n",
                      gdbarch_handle_segmentation_fault_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: handle_segmentation_fault = <%s>\n",
                      host_address_to_string (gdbarch->handle_segmentation_fault));
  fprintf_unfiltered (file,
                      "gdbarch_dump: has_dos_based_file_system = %s\n",
                      plongest (gdbarch->has_dos_based_file_system));
  fprintf_unfiltered (file,
                      "gdbarch_dump: has_global_breakpoints = %s\n",
                      plongest (gdbarch->has_global_breakpoints));
  fprintf_unfiltered (file,
                      "gdbarch_dump: has_global_solist = %s\n",
                      plongest (gdbarch->has_global_solist));
  fprintf_unfiltered (file,
                      "gdbarch_dump: has_shared_address_space = <%s>\n",
                      host_address_to_string (gdbarch->has_shared_address_space));
  fprintf_unfiltered (file,
                      "gdbarch_dump: have_nonsteppable_watchpoint = %s\n",
                      plongest (gdbarch->have_nonsteppable_watchpoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: in_solib_return_trampoline = <%s>\n",
                      host_address_to_string (gdbarch->in_solib_return_trampoline));
  fprintf_unfiltered (file,
                      "gdbarch_dump: infcall_mmap = <%s>\n",
                      host_address_to_string (gdbarch->infcall_mmap));
  fprintf_unfiltered (file,
                      "gdbarch_dump: infcall_munmap = <%s>\n",
                      host_address_to_string (gdbarch->infcall_munmap));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_info_proc_p() = %d\n",
                      gdbarch_info_proc_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: info_proc = <%s>\n",
                      host_address_to_string (gdbarch->info_proc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: inner_than = <%s>\n",
                      host_address_to_string (gdbarch->inner_than));
  fprintf_unfiltered (file,
                      "gdbarch_dump: insn_is_call = <%s>\n",
                      host_address_to_string (gdbarch->insn_is_call));
  fprintf_unfiltered (file,
                      "gdbarch_dump: insn_is_jump = <%s>\n",
                      host_address_to_string (gdbarch->insn_is_jump));
  fprintf_unfiltered (file,
                      "gdbarch_dump: insn_is_ret = <%s>\n",
                      host_address_to_string (gdbarch->insn_is_ret));
  fprintf_unfiltered (file,
                      "gdbarch_dump: int_bit = %s\n",
                      plongest (gdbarch->int_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_integer_to_address_p() = %d\n",
                      gdbarch_integer_to_address_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: integer_to_address = <%s>\n",
                      host_address_to_string (gdbarch->integer_to_address));
  fprintf_unfiltered (file,
                      "gdbarch_dump: iterate_over_objfiles_in_search_order = <%s>\n",
                      host_address_to_string (gdbarch->iterate_over_objfiles_in_search_order));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_iterate_over_regset_sections_p() = %d\n",
                      gdbarch_iterate_over_regset_sections_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: iterate_over_regset_sections = <%s>\n",
                      host_address_to_string (gdbarch->iterate_over_regset_sections));
  fprintf_unfiltered (file,
                      "gdbarch_dump: long_bit = %s\n",
                      plongest (gdbarch->long_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: long_double_bit = %s\n",
                      plongest (gdbarch->long_double_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: long_double_format = %s\n",
                      pformat (gdbarch->long_double_format));
  fprintf_unfiltered (file,
                      "gdbarch_dump: long_long_align_bit = %s\n",
                      plongest (gdbarch->long_long_align_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: long_long_bit = %s\n",
                      plongest (gdbarch->long_long_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_make_corefile_notes_p() = %d\n",
                      gdbarch_make_corefile_notes_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: make_corefile_notes = <%s>\n",
                      host_address_to_string (gdbarch->make_corefile_notes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: make_symbol_special = <%s>\n",
                      host_address_to_string (gdbarch->make_symbol_special));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_max_insn_length_p() = %d\n",
                      gdbarch_max_insn_length_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: max_insn_length = %s\n",
                      plongest (gdbarch->max_insn_length));
  fprintf_unfiltered (file,
                      "gdbarch_dump: memory_insert_breakpoint = <%s>\n",
                      host_address_to_string (gdbarch->memory_insert_breakpoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: memory_remove_breakpoint = <%s>\n",
                      host_address_to_string (gdbarch->memory_remove_breakpoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: num_pseudo_regs = %s\n",
                      plongest (gdbarch->num_pseudo_regs));
  fprintf_unfiltered (file,
                      "gdbarch_dump: num_regs = %s\n",
                      plongest (gdbarch->num_regs));
  fprintf_unfiltered (file,
                      "gdbarch_dump: osabi = %s\n",
                      plongest (gdbarch->osabi));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_overlay_update_p() = %d\n",
                      gdbarch_overlay_update_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: overlay_update = <%s>\n",
                      host_address_to_string (gdbarch->overlay_update));
  fprintf_unfiltered (file,
                      "gdbarch_dump: pc_regnum = %s\n",
                      plongest (gdbarch->pc_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: pointer_to_address = <%s>\n",
                      host_address_to_string (gdbarch->pointer_to_address));
  fprintf_unfiltered (file,
                      "gdbarch_dump: print_auxv_entry = <%s>\n",
                      host_address_to_string (gdbarch->print_auxv_entry));
  fprintf_unfiltered (file,
                      "gdbarch_dump: print_float_info = <%s>\n",
                      host_address_to_string (gdbarch->print_float_info));
  fprintf_unfiltered (file,
                      "gdbarch_dump: print_insn = <%s>\n",
                      host_address_to_string (gdbarch->print_insn));
  fprintf_unfiltered (file,
                      "gdbarch_dump: print_registers_info = <%s>\n",
                      host_address_to_string (gdbarch->print_registers_info));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_print_vector_info_p() = %d\n",
                      gdbarch_print_vector_info_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: print_vector_info = <%s>\n",
                      host_address_to_string (gdbarch->print_vector_info));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_process_record_p() = %d\n",
                      gdbarch_process_record_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: process_record = <%s>\n",
                      host_address_to_string (gdbarch->process_record));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_process_record_signal_p() = %d\n",
                      gdbarch_process_record_signal_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: process_record_signal = <%s>\n",
                      host_address_to_string (gdbarch->process_record_signal));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ps_regnum = %s\n",
                      plongest (gdbarch->ps_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_pseudo_register_read_p() = %d\n",
                      gdbarch_pseudo_register_read_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: pseudo_register_read = <%s>\n",
                      host_address_to_string (gdbarch->pseudo_register_read));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_pseudo_register_read_value_p() = %d\n",
                      gdbarch_pseudo_register_read_value_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: pseudo_register_read_value = <%s>\n",
                      host_address_to_string (gdbarch->pseudo_register_read_value));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_pseudo_register_write_p() = %d\n",
                      gdbarch_pseudo_register_write_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: pseudo_register_write = <%s>\n",
                      host_address_to_string (gdbarch->pseudo_register_write));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ptr_bit = %s\n",
                      plongest (gdbarch->ptr_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_push_dummy_call_p() = %d\n",
                      gdbarch_push_dummy_call_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: push_dummy_call = <%s>\n",
                      host_address_to_string (gdbarch->push_dummy_call));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_push_dummy_code_p() = %d\n",
                      gdbarch_push_dummy_code_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: push_dummy_code = <%s>\n",
                      host_address_to_string (gdbarch->push_dummy_code));
  fprintf_unfiltered (file,
                      "gdbarch_dump: ravenscar_ops = %s\n",
                      host_address_to_string (gdbarch->ravenscar_ops));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_read_pc_p() = %d\n",
                      gdbarch_read_pc_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: read_pc = <%s>\n",
                      host_address_to_string (gdbarch->read_pc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_record_special_symbol_p() = %d\n",
                      gdbarch_record_special_symbol_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: record_special_symbol = <%s>\n",
                      host_address_to_string (gdbarch->record_special_symbol));
  fprintf_unfiltered (file,
                      "gdbarch_dump: register_name = <%s>\n",
                      host_address_to_string (gdbarch->register_name));
  fprintf_unfiltered (file,
                      "gdbarch_dump: register_reggroup_p = <%s>\n",
                      host_address_to_string (gdbarch->register_reggroup_p));
  fprintf_unfiltered (file,
                      "gdbarch_dump: register_sim_regno = <%s>\n",
                      host_address_to_string (gdbarch->register_sim_regno));
  fprintf_unfiltered (file,
                      "gdbarch_dump: register_to_value = <%s>\n",
                      host_address_to_string (gdbarch->register_to_value));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_register_type_p() = %d\n",
                      gdbarch_register_type_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: register_type = <%s>\n",
                      host_address_to_string (gdbarch->register_type));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_relocate_instruction_p() = %d\n",
                      gdbarch_relocate_instruction_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: relocate_instruction = <%s>\n",
                      host_address_to_string (gdbarch->relocate_instruction));
  fprintf_unfiltered (file,
                      "gdbarch_dump: remote_register_number = <%s>\n",
                      host_address_to_string (gdbarch->remote_register_number));
  fprintf_unfiltered (file,
                      "gdbarch_dump: return_in_first_hidden_param_p = <%s>\n",
                      host_address_to_string (gdbarch->return_in_first_hidden_param_p));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_return_value_p() = %d\n",
                      gdbarch_return_value_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: return_value = <%s>\n",
                      host_address_to_string (gdbarch->return_value));
  fprintf_unfiltered (file,
                      "gdbarch_dump: sdb_reg_to_regnum = <%s>\n",
                      host_address_to_string (gdbarch->sdb_reg_to_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: short_bit = %s\n",
                      plongest (gdbarch->short_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_single_step_through_delay_p() = %d\n",
                      gdbarch_single_step_through_delay_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: single_step_through_delay = <%s>\n",
                      host_address_to_string (gdbarch->single_step_through_delay));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_skip_entrypoint_p() = %d\n",
                      gdbarch_skip_entrypoint_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_entrypoint = <%s>\n",
                      host_address_to_string (gdbarch->skip_entrypoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_skip_main_prologue_p() = %d\n",
                      gdbarch_skip_main_prologue_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_main_prologue = <%s>\n",
                      host_address_to_string (gdbarch->skip_main_prologue));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_permanent_breakpoint = <%s>\n",
                      host_address_to_string (gdbarch->skip_permanent_breakpoint));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_prologue = <%s>\n",
                      host_address_to_string (gdbarch->skip_prologue));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_solib_resolver = <%s>\n",
                      host_address_to_string (gdbarch->skip_solib_resolver));
  fprintf_unfiltered (file,
                      "gdbarch_dump: skip_trampoline_code = <%s>\n",
                      host_address_to_string (gdbarch->skip_trampoline_code));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_software_single_step_p() = %d\n",
                      gdbarch_software_single_step_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: software_single_step = <%s>\n",
                      host_address_to_string (gdbarch->software_single_step));
  fprintf_unfiltered (file,
                      "gdbarch_dump: sofun_address_maybe_missing = %s\n",
                      plongest (gdbarch->sofun_address_maybe_missing));
  fprintf_unfiltered (file,
                      "gdbarch_dump: solib_symbols_extension = %s\n",
                      pstring (gdbarch->solib_symbols_extension));
  fprintf_unfiltered (file,
                      "gdbarch_dump: sp_regnum = %s\n",
                      plongest (gdbarch->sp_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stab_reg_to_regnum = <%s>\n",
                      host_address_to_string (gdbarch->stab_reg_to_regnum));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stabs_argument_has_addr = <%s>\n",
                      host_address_to_string (gdbarch->stabs_argument_has_addr));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stack_frame_destroyed_p = <%s>\n",
                      host_address_to_string (gdbarch->stack_frame_destroyed_p));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_gdb_register_prefix = %s\n",
                      pstring (gdbarch->stap_gdb_register_prefix));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_gdb_register_suffix = %s\n",
                      pstring (gdbarch->stap_gdb_register_suffix));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_integer_prefixes = %s\n",
                      pstring_list (gdbarch->stap_integer_prefixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_integer_suffixes = %s\n",
                      pstring_list (gdbarch->stap_integer_suffixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_stap_is_single_operand_p() = %d\n",
                      gdbarch_stap_is_single_operand_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_is_single_operand = <%s>\n",
                      host_address_to_string (gdbarch->stap_is_single_operand));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_stap_parse_special_token_p() = %d\n",
                      gdbarch_stap_parse_special_token_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_parse_special_token = <%s>\n",
                      host_address_to_string (gdbarch->stap_parse_special_token));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_register_indirection_prefixes = %s\n",
                      pstring_list (gdbarch->stap_register_indirection_prefixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_register_indirection_suffixes = %s\n",
                      pstring_list (gdbarch->stap_register_indirection_suffixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_register_prefixes = %s\n",
                      pstring_list (gdbarch->stap_register_prefixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: stap_register_suffixes = %s\n",
                      pstring_list (gdbarch->stap_register_suffixes));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_static_transform_name_p() = %d\n",
                      gdbarch_static_transform_name_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: static_transform_name = <%s>\n",
                      host_address_to_string (gdbarch->static_transform_name));
  fprintf_unfiltered (file,
                      "gdbarch_dump: sw_breakpoint_from_kind = <%s>\n",
                      host_address_to_string (gdbarch->sw_breakpoint_from_kind));
  fprintf_unfiltered (file,
                      "gdbarch_dump: syscalls_info = %s\n",
                      host_address_to_string (gdbarch->syscalls_info));
  fprintf_unfiltered (file,
                      "gdbarch_dump: target_desc = %s\n",
                      host_address_to_string (gdbarch->target_desc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_unwind_pc_p() = %d\n",
                      gdbarch_unwind_pc_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: unwind_pc = <%s>\n",
                      host_address_to_string (gdbarch->unwind_pc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_unwind_sp_p() = %d\n",
                      gdbarch_unwind_sp_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: unwind_sp = <%s>\n",
                      host_address_to_string (gdbarch->unwind_sp));
  fprintf_unfiltered (file,
                      "gdbarch_dump: valid_disassembler_options = %s\n",
                      host_address_to_string (gdbarch->valid_disassembler_options));
  fprintf_unfiltered (file,
                      "gdbarch_dump: value_from_register = <%s>\n",
                      host_address_to_string (gdbarch->value_from_register));
  fprintf_unfiltered (file,
                      "gdbarch_dump: value_to_register = <%s>\n",
                      host_address_to_string (gdbarch->value_to_register));
  fprintf_unfiltered (file,
                      "gdbarch_dump: vbit_in_delta = %s\n",
                      plongest (gdbarch->vbit_in_delta));
  fprintf_unfiltered (file,
                      "gdbarch_dump: virtual_frame_pointer = <%s>\n",
                      host_address_to_string (gdbarch->virtual_frame_pointer));
  fprintf_unfiltered (file,
                      "gdbarch_dump: vsyscall_range = <%s>\n",
                      host_address_to_string (gdbarch->vsyscall_range));
  fprintf_unfiltered (file,
                      "gdbarch_dump: vtable_function_descriptors = %s\n",
                      plongest (gdbarch->vtable_function_descriptors));
  fprintf_unfiltered (file,
                      "gdbarch_dump: wchar_bit = %s\n",
                      plongest (gdbarch->wchar_bit));
  fprintf_unfiltered (file,
                      "gdbarch_dump: wchar_signed = %s\n",
                      plongest (gdbarch->wchar_signed));
  fprintf_unfiltered (file,
                      "gdbarch_dump: gdbarch_write_pc_p() = %d\n",
                      gdbarch_write_pc_p (gdbarch));
  fprintf_unfiltered (file,
                      "gdbarch_dump: write_pc = <%s>\n",
                      host_address_to_string (gdbarch->write_pc));
  fprintf_unfiltered (file,
                      "gdbarch_dump: xml_syscall_file = %s\n",
                      pstring (gdbarch->xml_syscall_file));
  if (gdbarch->dump_tdep != NULL)
    gdbarch->dump_tdep (gdbarch, file);
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
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_bfd_arch_info called\n");
  return gdbarch->bfd_arch_info;
}

enum bfd_endian
gdbarch_byte_order (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_byte_order called\n");
  return gdbarch->byte_order;
}

enum bfd_endian
gdbarch_byte_order_for_code (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_byte_order_for_code called\n");
  return gdbarch->byte_order_for_code;
}

enum gdb_osabi
gdbarch_osabi (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_osabi called\n");
  return gdbarch->osabi;
}

const struct target_desc *
gdbarch_target_desc (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_target_desc called\n");
  return gdbarch->target_desc;
}

int
gdbarch_bits_big_endian (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of bits_big_endian, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_bits_big_endian called\n");
  return gdbarch->bits_big_endian;
}

void
set_gdbarch_bits_big_endian (struct gdbarch *gdbarch,
                             int bits_big_endian)
{
  gdbarch->bits_big_endian = bits_big_endian;
}

int
gdbarch_short_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of short_bit, invalid_p == 0 */
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
  gdb_assert (gdbarch != NULL);
  /* Skip verify of int_bit, invalid_p == 0 */
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
  gdb_assert (gdbarch != NULL);
  /* Skip verify of long_bit, invalid_p == 0 */
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
  gdb_assert (gdbarch != NULL);
  /* Skip verify of long_long_bit, invalid_p == 0 */
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
gdbarch_long_long_align_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of long_long_align_bit, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_long_long_align_bit called\n");
  return gdbarch->long_long_align_bit;
}

void
set_gdbarch_long_long_align_bit (struct gdbarch *gdbarch,
                                 int long_long_align_bit)
{
  gdbarch->long_long_align_bit = long_long_align_bit;
}

int
gdbarch_half_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of half_bit, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_half_bit called\n");
  return gdbarch->half_bit;
}

void
set_gdbarch_half_bit (struct gdbarch *gdbarch,
                      int half_bit)
{
  gdbarch->half_bit = half_bit;
}

const struct floatformat **
gdbarch_half_format (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_half_format called\n");
  return gdbarch->half_format;
}

void
set_gdbarch_half_format (struct gdbarch *gdbarch,
                         const struct floatformat ** half_format)
{
  gdbarch->half_format = half_format;
}

int
gdbarch_float_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of float_bit, invalid_p == 0 */
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

const struct floatformat **
gdbarch_float_format (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_float_format called\n");
  return gdbarch->float_format;
}

void
set_gdbarch_float_format (struct gdbarch *gdbarch,
                          const struct floatformat ** float_format)
{
  gdbarch->float_format = float_format;
}

int
gdbarch_double_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of double_bit, invalid_p == 0 */
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

const struct floatformat **
gdbarch_double_format (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_double_format called\n");
  return gdbarch->double_format;
}

void
set_gdbarch_double_format (struct gdbarch *gdbarch,
                           const struct floatformat ** double_format)
{
  gdbarch->double_format = double_format;
}

int
gdbarch_long_double_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of long_double_bit, invalid_p == 0 */
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

const struct floatformat **
gdbarch_long_double_format (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_long_double_format called\n");
  return gdbarch->long_double_format;
}

void
set_gdbarch_long_double_format (struct gdbarch *gdbarch,
                                const struct floatformat ** long_double_format)
{
  gdbarch->long_double_format = long_double_format;
}

int
gdbarch_wchar_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of wchar_bit, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_wchar_bit called\n");
  return gdbarch->wchar_bit;
}

void
set_gdbarch_wchar_bit (struct gdbarch *gdbarch,
                       int wchar_bit)
{
  gdbarch->wchar_bit = wchar_bit;
}

int
gdbarch_wchar_signed (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->wchar_signed != -1);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_wchar_signed called\n");
  return gdbarch->wchar_signed;
}

void
set_gdbarch_wchar_signed (struct gdbarch *gdbarch,
                          int wchar_signed)
{
  gdbarch->wchar_signed = wchar_signed;
}

const struct floatformat **
gdbarch_floatformat_for_type (struct gdbarch *gdbarch, const char *name, int length)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->floatformat_for_type != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_floatformat_for_type called\n");
  return gdbarch->floatformat_for_type (gdbarch, name, length);
}

void
set_gdbarch_floatformat_for_type (struct gdbarch *gdbarch,
                                  gdbarch_floatformat_for_type_ftype floatformat_for_type)
{
  gdbarch->floatformat_for_type = floatformat_for_type;
}

int
gdbarch_ptr_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of ptr_bit, invalid_p == 0 */
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
gdbarch_addr_bit (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->addr_bit != 0);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_addr_bit called\n");
  return gdbarch->addr_bit;
}

void
set_gdbarch_addr_bit (struct gdbarch *gdbarch,
                      int addr_bit)
{
  gdbarch->addr_bit = addr_bit;
}

int
gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->dwarf2_addr_size != 0);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dwarf2_addr_size called\n");
  return gdbarch->dwarf2_addr_size;
}

void
set_gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch,
                              int dwarf2_addr_size)
{
  gdbarch->dwarf2_addr_size = dwarf2_addr_size;
}

int
gdbarch_char_signed (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->char_signed != -1);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_char_signed called\n");
  return gdbarch->char_signed;
}

void
set_gdbarch_char_signed (struct gdbarch *gdbarch,
                         int char_signed)
{
  gdbarch->char_signed = char_signed;
}

int
gdbarch_read_pc_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->read_pc != NULL;
}

CORE_ADDR
gdbarch_read_pc (struct gdbarch *gdbarch, struct regcache *regcache)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->read_pc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_read_pc called\n");
  return gdbarch->read_pc (regcache);
}

void
set_gdbarch_read_pc (struct gdbarch *gdbarch,
                     gdbarch_read_pc_ftype read_pc)
{
  gdbarch->read_pc = read_pc;
}

int
gdbarch_write_pc_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->write_pc != NULL;
}

void
gdbarch_write_pc (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR val)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->write_pc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_write_pc called\n");
  gdbarch->write_pc (regcache, val);
}

void
set_gdbarch_write_pc (struct gdbarch *gdbarch,
                      gdbarch_write_pc_ftype write_pc)
{
  gdbarch->write_pc = write_pc;
}

void
gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch, CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->virtual_frame_pointer != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_virtual_frame_pointer called\n");
  gdbarch->virtual_frame_pointer (gdbarch, pc, frame_regnum, frame_offset);
}

void
set_gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch,
                                   gdbarch_virtual_frame_pointer_ftype virtual_frame_pointer)
{
  gdbarch->virtual_frame_pointer = virtual_frame_pointer;
}

int
gdbarch_pseudo_register_read_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->pseudo_register_read != NULL;
}

enum register_status
gdbarch_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, gdb_byte *buf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->pseudo_register_read != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pseudo_register_read called\n");
  return gdbarch->pseudo_register_read (gdbarch, regcache, cookednum, buf);
}

void
set_gdbarch_pseudo_register_read (struct gdbarch *gdbarch,
                                  gdbarch_pseudo_register_read_ftype pseudo_register_read)
{
  gdbarch->pseudo_register_read = pseudo_register_read;
}

int
gdbarch_pseudo_register_read_value_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->pseudo_register_read_value != NULL;
}

struct value *
gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->pseudo_register_read_value != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pseudo_register_read_value called\n");
  return gdbarch->pseudo_register_read_value (gdbarch, regcache, cookednum);
}

void
set_gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch,
                                        gdbarch_pseudo_register_read_value_ftype pseudo_register_read_value)
{
  gdbarch->pseudo_register_read_value = pseudo_register_read_value;
}

int
gdbarch_pseudo_register_write_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->pseudo_register_write != NULL;
}

void
gdbarch_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, const gdb_byte *buf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->pseudo_register_write != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pseudo_register_write called\n");
  gdbarch->pseudo_register_write (gdbarch, regcache, cookednum, buf);
}

void
set_gdbarch_pseudo_register_write (struct gdbarch *gdbarch,
                                   gdbarch_pseudo_register_write_ftype pseudo_register_write)
{
  gdbarch->pseudo_register_write = pseudo_register_write;
}

int
gdbarch_num_regs (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->num_regs != -1);
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
gdbarch_num_pseudo_regs (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of num_pseudo_regs, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_num_pseudo_regs called\n");
  return gdbarch->num_pseudo_regs;
}

void
set_gdbarch_num_pseudo_regs (struct gdbarch *gdbarch,
                             int num_pseudo_regs)
{
  gdbarch->num_pseudo_regs = num_pseudo_regs;
}

int
gdbarch_ax_pseudo_register_collect_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->ax_pseudo_register_collect != NULL;
}

int
gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch, struct agent_expr *ax, int reg)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->ax_pseudo_register_collect != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ax_pseudo_register_collect called\n");
  return gdbarch->ax_pseudo_register_collect (gdbarch, ax, reg);
}

void
set_gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch,
                                        gdbarch_ax_pseudo_register_collect_ftype ax_pseudo_register_collect)
{
  gdbarch->ax_pseudo_register_collect = ax_pseudo_register_collect;
}

int
gdbarch_ax_pseudo_register_push_stack_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->ax_pseudo_register_push_stack != NULL;
}

int
gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch, struct agent_expr *ax, int reg)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->ax_pseudo_register_push_stack != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ax_pseudo_register_push_stack called\n");
  return gdbarch->ax_pseudo_register_push_stack (gdbarch, ax, reg);
}

void
set_gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch,
                                           gdbarch_ax_pseudo_register_push_stack_ftype ax_pseudo_register_push_stack)
{
  gdbarch->ax_pseudo_register_push_stack = ax_pseudo_register_push_stack;
}

int
gdbarch_handle_segmentation_fault_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->handle_segmentation_fault != NULL;
}

void
gdbarch_handle_segmentation_fault (struct gdbarch *gdbarch, struct ui_out *uiout)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->handle_segmentation_fault != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_handle_segmentation_fault called\n");
  gdbarch->handle_segmentation_fault (gdbarch, uiout);
}

void
set_gdbarch_handle_segmentation_fault (struct gdbarch *gdbarch,
                                       gdbarch_handle_segmentation_fault_ftype handle_segmentation_fault)
{
  gdbarch->handle_segmentation_fault = handle_segmentation_fault;
}

int
gdbarch_sp_regnum (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of sp_regnum, invalid_p == 0 */
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
gdbarch_pc_regnum (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of pc_regnum, invalid_p == 0 */
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

int
gdbarch_ps_regnum (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of ps_regnum, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ps_regnum called\n");
  return gdbarch->ps_regnum;
}

void
set_gdbarch_ps_regnum (struct gdbarch *gdbarch,
                       int ps_regnum)
{
  gdbarch->ps_regnum = ps_regnum;
}

int
gdbarch_fp0_regnum (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of fp0_regnum, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fp0_regnum called\n");
  return gdbarch->fp0_regnum;
}

void
set_gdbarch_fp0_regnum (struct gdbarch *gdbarch,
                        int fp0_regnum)
{
  gdbarch->fp0_regnum = fp0_regnum;
}

int
gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch, int stab_regnr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->stab_reg_to_regnum != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stab_reg_to_regnum called\n");
  return gdbarch->stab_reg_to_regnum (gdbarch, stab_regnr);
}

void
set_gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch,
                                gdbarch_stab_reg_to_regnum_ftype stab_reg_to_regnum)
{
  gdbarch->stab_reg_to_regnum = stab_reg_to_regnum;
}

int
gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch, int ecoff_regnr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->ecoff_reg_to_regnum != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ecoff_reg_to_regnum called\n");
  return gdbarch->ecoff_reg_to_regnum (gdbarch, ecoff_regnr);
}

void
set_gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch,
                                 gdbarch_ecoff_reg_to_regnum_ftype ecoff_reg_to_regnum)
{
  gdbarch->ecoff_reg_to_regnum = ecoff_reg_to_regnum;
}

int
gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch, int sdb_regnr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->sdb_reg_to_regnum != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_sdb_reg_to_regnum called\n");
  return gdbarch->sdb_reg_to_regnum (gdbarch, sdb_regnr);
}

void
set_gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch,
                               gdbarch_sdb_reg_to_regnum_ftype sdb_reg_to_regnum)
{
  gdbarch->sdb_reg_to_regnum = sdb_reg_to_regnum;
}

int
gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int dwarf2_regnr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dwarf2_reg_to_regnum != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dwarf2_reg_to_regnum called\n");
  return gdbarch->dwarf2_reg_to_regnum (gdbarch, dwarf2_regnr);
}

void
set_gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch,
                                  gdbarch_dwarf2_reg_to_regnum_ftype dwarf2_reg_to_regnum)
{
  gdbarch->dwarf2_reg_to_regnum = dwarf2_reg_to_regnum;
}

const char *
gdbarch_register_name (struct gdbarch *gdbarch, int regnr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->register_name != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_name called\n");
  return gdbarch->register_name (gdbarch, regnr);
}

void
set_gdbarch_register_name (struct gdbarch *gdbarch,
                           gdbarch_register_name_ftype register_name)
{
  gdbarch->register_name = register_name;
}

int
gdbarch_register_type_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->register_type != NULL;
}

struct type *
gdbarch_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->register_type != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_type called\n");
  return gdbarch->register_type (gdbarch, reg_nr);
}

void
set_gdbarch_register_type (struct gdbarch *gdbarch,
                           gdbarch_register_type_ftype register_type)
{
  gdbarch->register_type = register_type;
}

int
gdbarch_dummy_id_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->dummy_id != NULL;
}

struct frame_id
gdbarch_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dummy_id != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dummy_id called\n");
  return gdbarch->dummy_id (gdbarch, this_frame);
}

void
set_gdbarch_dummy_id (struct gdbarch *gdbarch,
                      gdbarch_dummy_id_ftype dummy_id)
{
  gdbarch->dummy_id = dummy_id;
}

int
gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of deprecated_fp_regnum, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_deprecated_fp_regnum called\n");
  return gdbarch->deprecated_fp_regnum;
}

void
set_gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch,
                                  int deprecated_fp_regnum)
{
  gdbarch->deprecated_fp_regnum = deprecated_fp_regnum;
}

int
gdbarch_push_dummy_call_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->push_dummy_call != NULL;
}

CORE_ADDR
gdbarch_push_dummy_call (struct gdbarch *gdbarch, struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->push_dummy_call != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_push_dummy_call called\n");
  return gdbarch->push_dummy_call (gdbarch, function, regcache, bp_addr, nargs, args, sp, struct_return, struct_addr);
}

void
set_gdbarch_push_dummy_call (struct gdbarch *gdbarch,
                             gdbarch_push_dummy_call_ftype push_dummy_call)
{
  gdbarch->push_dummy_call = push_dummy_call;
}

int
gdbarch_call_dummy_location (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of call_dummy_location, invalid_p == 0 */
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

int
gdbarch_push_dummy_code_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->push_dummy_code != NULL;
}

CORE_ADDR
gdbarch_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->push_dummy_code != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_push_dummy_code called\n");
  return gdbarch->push_dummy_code (gdbarch, sp, funaddr, args, nargs, value_type, real_pc, bp_addr, regcache);
}

void
set_gdbarch_push_dummy_code (struct gdbarch *gdbarch,
                             gdbarch_push_dummy_code_ftype push_dummy_code)
{
  gdbarch->push_dummy_code = push_dummy_code;
}

int
gdbarch_code_of_frame_writable (struct gdbarch *gdbarch, struct frame_info *frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->code_of_frame_writable != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_code_of_frame_writable called\n");
  return gdbarch->code_of_frame_writable (gdbarch, frame);
}

void
set_gdbarch_code_of_frame_writable (struct gdbarch *gdbarch,
                                    gdbarch_code_of_frame_writable_ftype code_of_frame_writable)
{
  gdbarch->code_of_frame_writable = code_of_frame_writable;
}

void
gdbarch_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, int regnum, int all)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->print_registers_info != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_print_registers_info called\n");
  gdbarch->print_registers_info (gdbarch, file, frame, regnum, all);
}

void
set_gdbarch_print_registers_info (struct gdbarch *gdbarch,
                                  gdbarch_print_registers_info_ftype print_registers_info)
{
  gdbarch->print_registers_info = print_registers_info;
}

void
gdbarch_print_float_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->print_float_info != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_print_float_info called\n");
  gdbarch->print_float_info (gdbarch, file, frame, args);
}

void
set_gdbarch_print_float_info (struct gdbarch *gdbarch,
                              gdbarch_print_float_info_ftype print_float_info)
{
  gdbarch->print_float_info = print_float_info;
}

int
gdbarch_print_vector_info_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->print_vector_info != NULL;
}

void
gdbarch_print_vector_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->print_vector_info != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_print_vector_info called\n");
  gdbarch->print_vector_info (gdbarch, file, frame, args);
}

void
set_gdbarch_print_vector_info (struct gdbarch *gdbarch,
                               gdbarch_print_vector_info_ftype print_vector_info)
{
  gdbarch->print_vector_info = print_vector_info;
}

int
gdbarch_register_sim_regno (struct gdbarch *gdbarch, int reg_nr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->register_sim_regno != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_sim_regno called\n");
  return gdbarch->register_sim_regno (gdbarch, reg_nr);
}

void
set_gdbarch_register_sim_regno (struct gdbarch *gdbarch,
                                gdbarch_register_sim_regno_ftype register_sim_regno)
{
  gdbarch->register_sim_regno = register_sim_regno;
}

int
gdbarch_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->cannot_fetch_register != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_cannot_fetch_register called\n");
  return gdbarch->cannot_fetch_register (gdbarch, regnum);
}

void
set_gdbarch_cannot_fetch_register (struct gdbarch *gdbarch,
                                   gdbarch_cannot_fetch_register_ftype cannot_fetch_register)
{
  gdbarch->cannot_fetch_register = cannot_fetch_register;
}

int
gdbarch_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->cannot_store_register != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_cannot_store_register called\n");
  return gdbarch->cannot_store_register (gdbarch, regnum);
}

void
set_gdbarch_cannot_store_register (struct gdbarch *gdbarch,
                                   gdbarch_cannot_store_register_ftype cannot_store_register)
{
  gdbarch->cannot_store_register = cannot_store_register;
}

int
gdbarch_get_longjmp_target_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->get_longjmp_target != NULL;
}

int
gdbarch_get_longjmp_target (struct gdbarch *gdbarch, struct frame_info *frame, CORE_ADDR *pc)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->get_longjmp_target != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_get_longjmp_target called\n");
  return gdbarch->get_longjmp_target (frame, pc);
}

void
set_gdbarch_get_longjmp_target (struct gdbarch *gdbarch,
                                gdbarch_get_longjmp_target_ftype get_longjmp_target)
{
  gdbarch->get_longjmp_target = get_longjmp_target;
}

int
gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
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
gdbarch_convert_register_p (struct gdbarch *gdbarch, int regnum, struct type *type)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->convert_register_p != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_convert_register_p called\n");
  return gdbarch->convert_register_p (gdbarch, regnum, type);
}

void
set_gdbarch_convert_register_p (struct gdbarch *gdbarch,
                                gdbarch_convert_register_p_ftype convert_register_p)
{
  gdbarch->convert_register_p = convert_register_p;
}

int
gdbarch_register_to_value (struct gdbarch *gdbarch, struct frame_info *frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->register_to_value != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_to_value called\n");
  return gdbarch->register_to_value (frame, regnum, type, buf, optimizedp, unavailablep);
}

void
set_gdbarch_register_to_value (struct gdbarch *gdbarch,
                               gdbarch_register_to_value_ftype register_to_value)
{
  gdbarch->register_to_value = register_to_value;
}

void
gdbarch_value_to_register (struct gdbarch *gdbarch, struct frame_info *frame, int regnum, struct type *type, const gdb_byte *buf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->value_to_register != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_value_to_register called\n");
  gdbarch->value_to_register (frame, regnum, type, buf);
}

void
set_gdbarch_value_to_register (struct gdbarch *gdbarch,
                               gdbarch_value_to_register_ftype value_to_register)
{
  gdbarch->value_to_register = value_to_register;
}

struct value *
gdbarch_value_from_register (struct gdbarch *gdbarch, struct type *type, int regnum, struct frame_id frame_id)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->value_from_register != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_value_from_register called\n");
  return gdbarch->value_from_register (gdbarch, type, regnum, frame_id);
}

void
set_gdbarch_value_from_register (struct gdbarch *gdbarch,
                                 gdbarch_value_from_register_ftype value_from_register)
{
  gdbarch->value_from_register = value_from_register;
}

CORE_ADDR
gdbarch_pointer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->pointer_to_address != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_pointer_to_address called\n");
  return gdbarch->pointer_to_address (gdbarch, type, buf);
}

void
set_gdbarch_pointer_to_address (struct gdbarch *gdbarch,
                                gdbarch_pointer_to_address_ftype pointer_to_address)
{
  gdbarch->pointer_to_address = pointer_to_address;
}

void
gdbarch_address_to_pointer (struct gdbarch *gdbarch, struct type *type, gdb_byte *buf, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->address_to_pointer != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_address_to_pointer called\n");
  gdbarch->address_to_pointer (gdbarch, type, buf, addr);
}

void
set_gdbarch_address_to_pointer (struct gdbarch *gdbarch,
                                gdbarch_address_to_pointer_ftype address_to_pointer)
{
  gdbarch->address_to_pointer = address_to_pointer;
}

int
gdbarch_integer_to_address_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->integer_to_address != NULL;
}

CORE_ADDR
gdbarch_integer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->integer_to_address != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_integer_to_address called\n");
  return gdbarch->integer_to_address (gdbarch, type, buf);
}

void
set_gdbarch_integer_to_address (struct gdbarch *gdbarch,
                                gdbarch_integer_to_address_ftype integer_to_address)
{
  gdbarch->integer_to_address = integer_to_address;
}

int
gdbarch_return_value_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->return_value != NULL;
}

enum return_value_convention
gdbarch_return_value (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, gdb_byte *readbuf, const gdb_byte *writebuf)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->return_value != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_return_value called\n");
  return gdbarch->return_value (gdbarch, function, valtype, regcache, readbuf, writebuf);
}

void
set_gdbarch_return_value (struct gdbarch *gdbarch,
                          gdbarch_return_value_ftype return_value)
{
  gdbarch->return_value = return_value;
}

int
gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch, struct type *type)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->return_in_first_hidden_param_p != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_return_in_first_hidden_param_p called\n");
  return gdbarch->return_in_first_hidden_param_p (gdbarch, type);
}

void
set_gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch,
                                            gdbarch_return_in_first_hidden_param_p_ftype return_in_first_hidden_param_p)
{
  gdbarch->return_in_first_hidden_param_p = return_in_first_hidden_param_p;
}

CORE_ADDR
gdbarch_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR ip)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_prologue != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_prologue called\n");
  return gdbarch->skip_prologue (gdbarch, ip);
}

void
set_gdbarch_skip_prologue (struct gdbarch *gdbarch,
                           gdbarch_skip_prologue_ftype skip_prologue)
{
  gdbarch->skip_prologue = skip_prologue;
}

int
gdbarch_skip_main_prologue_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->skip_main_prologue != NULL;
}

CORE_ADDR
gdbarch_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR ip)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_main_prologue != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_main_prologue called\n");
  return gdbarch->skip_main_prologue (gdbarch, ip);
}

void
set_gdbarch_skip_main_prologue (struct gdbarch *gdbarch,
                                gdbarch_skip_main_prologue_ftype skip_main_prologue)
{
  gdbarch->skip_main_prologue = skip_main_prologue;
}

int
gdbarch_skip_entrypoint_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->skip_entrypoint != NULL;
}

CORE_ADDR
gdbarch_skip_entrypoint (struct gdbarch *gdbarch, CORE_ADDR ip)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_entrypoint != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_entrypoint called\n");
  return gdbarch->skip_entrypoint (gdbarch, ip);
}

void
set_gdbarch_skip_entrypoint (struct gdbarch *gdbarch,
                             gdbarch_skip_entrypoint_ftype skip_entrypoint)
{
  gdbarch->skip_entrypoint = skip_entrypoint;
}

int
gdbarch_inner_than (struct gdbarch *gdbarch, CORE_ADDR lhs, CORE_ADDR rhs)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->inner_than != NULL);
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

const gdb_byte *
gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->breakpoint_from_pc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_breakpoint_from_pc called\n");
  return gdbarch->breakpoint_from_pc (gdbarch, pcptr, lenptr);
}

void
set_gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch,
                                gdbarch_breakpoint_from_pc_ftype breakpoint_from_pc)
{
  gdbarch->breakpoint_from_pc = breakpoint_from_pc;
}

int
gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->breakpoint_kind_from_pc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_breakpoint_kind_from_pc called\n");
  return gdbarch->breakpoint_kind_from_pc (gdbarch, pcptr);
}

void
set_gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch,
                                     gdbarch_breakpoint_kind_from_pc_ftype breakpoint_kind_from_pc)
{
  gdbarch->breakpoint_kind_from_pc = breakpoint_kind_from_pc;
}

const gdb_byte *
gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->sw_breakpoint_from_kind != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_sw_breakpoint_from_kind called\n");
  return gdbarch->sw_breakpoint_from_kind (gdbarch, kind, size);
}

void
set_gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch,
                                     gdbarch_sw_breakpoint_from_kind_ftype sw_breakpoint_from_kind)
{
  gdbarch->sw_breakpoint_from_kind = sw_breakpoint_from_kind;
}

int
gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR *pcptr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->breakpoint_kind_from_current_state != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_breakpoint_kind_from_current_state called\n");
  return gdbarch->breakpoint_kind_from_current_state (gdbarch, regcache, pcptr);
}

void
set_gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch,
                                                gdbarch_breakpoint_kind_from_current_state_ftype breakpoint_kind_from_current_state)
{
  gdbarch->breakpoint_kind_from_current_state = breakpoint_kind_from_current_state;
}

int
gdbarch_adjust_breakpoint_address_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->adjust_breakpoint_address != NULL;
}

CORE_ADDR
gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->adjust_breakpoint_address != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_adjust_breakpoint_address called\n");
  return gdbarch->adjust_breakpoint_address (gdbarch, bpaddr);
}

void
set_gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch,
                                       gdbarch_adjust_breakpoint_address_ftype adjust_breakpoint_address)
{
  gdbarch->adjust_breakpoint_address = adjust_breakpoint_address;
}

int
gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->memory_insert_breakpoint != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_memory_insert_breakpoint called\n");
  return gdbarch->memory_insert_breakpoint (gdbarch, bp_tgt);
}

void
set_gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch,
                                      gdbarch_memory_insert_breakpoint_ftype memory_insert_breakpoint)
{
  gdbarch->memory_insert_breakpoint = memory_insert_breakpoint;
}

int
gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->memory_remove_breakpoint != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_memory_remove_breakpoint called\n");
  return gdbarch->memory_remove_breakpoint (gdbarch, bp_tgt);
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
  gdb_assert (gdbarch != NULL);
  /* Skip verify of decr_pc_after_break, invalid_p == 0 */
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
gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of deprecated_function_start_offset, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_deprecated_function_start_offset called\n");
  return gdbarch->deprecated_function_start_offset;
}

void
set_gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch,
                                              CORE_ADDR deprecated_function_start_offset)
{
  gdbarch->deprecated_function_start_offset = deprecated_function_start_offset;
}

int
gdbarch_remote_register_number (struct gdbarch *gdbarch, int regno)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->remote_register_number != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_remote_register_number called\n");
  return gdbarch->remote_register_number (gdbarch, regno);
}

void
set_gdbarch_remote_register_number (struct gdbarch *gdbarch,
                                    gdbarch_remote_register_number_ftype remote_register_number)
{
  gdbarch->remote_register_number = remote_register_number;
}

int
gdbarch_fetch_tls_load_module_address_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->fetch_tls_load_module_address != NULL;
}

CORE_ADDR
gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch, struct objfile *objfile)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->fetch_tls_load_module_address != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fetch_tls_load_module_address called\n");
  return gdbarch->fetch_tls_load_module_address (objfile);
}

void
set_gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch,
                                           gdbarch_fetch_tls_load_module_address_ftype fetch_tls_load_module_address)
{
  gdbarch->fetch_tls_load_module_address = fetch_tls_load_module_address;
}

CORE_ADDR
gdbarch_frame_args_skip (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of frame_args_skip, invalid_p == 0 */
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
gdbarch_unwind_pc_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->unwind_pc != NULL;
}

CORE_ADDR
gdbarch_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->unwind_pc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_unwind_pc called\n");
  return gdbarch->unwind_pc (gdbarch, next_frame);
}

void
set_gdbarch_unwind_pc (struct gdbarch *gdbarch,
                       gdbarch_unwind_pc_ftype unwind_pc)
{
  gdbarch->unwind_pc = unwind_pc;
}

int
gdbarch_unwind_sp_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->unwind_sp != NULL;
}

CORE_ADDR
gdbarch_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->unwind_sp != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_unwind_sp called\n");
  return gdbarch->unwind_sp (gdbarch, next_frame);
}

void
set_gdbarch_unwind_sp (struct gdbarch *gdbarch,
                       gdbarch_unwind_sp_ftype unwind_sp)
{
  gdbarch->unwind_sp = unwind_sp;
}

int
gdbarch_frame_num_args_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->frame_num_args != NULL;
}

int
gdbarch_frame_num_args (struct gdbarch *gdbarch, struct frame_info *frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->frame_num_args != NULL);
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

int
gdbarch_frame_align_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->frame_align != NULL;
}

CORE_ADDR
gdbarch_frame_align (struct gdbarch *gdbarch, CORE_ADDR address)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->frame_align != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_align called\n");
  return gdbarch->frame_align (gdbarch, address);
}

void
set_gdbarch_frame_align (struct gdbarch *gdbarch,
                         gdbarch_frame_align_ftype frame_align)
{
  gdbarch->frame_align = frame_align;
}

int
gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch, struct type *type)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->stabs_argument_has_addr != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stabs_argument_has_addr called\n");
  return gdbarch->stabs_argument_has_addr (gdbarch, type);
}

void
set_gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch,
                                     gdbarch_stabs_argument_has_addr_ftype stabs_argument_has_addr)
{
  gdbarch->stabs_argument_has_addr = stabs_argument_has_addr;
}

int
gdbarch_frame_red_zone_size (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_frame_red_zone_size called\n");
  return gdbarch->frame_red_zone_size;
}

void
set_gdbarch_frame_red_zone_size (struct gdbarch *gdbarch,
                                 int frame_red_zone_size)
{
  gdbarch->frame_red_zone_size = frame_red_zone_size;
}

CORE_ADDR
gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr, struct target_ops *targ)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->convert_from_func_ptr_addr != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_convert_from_func_ptr_addr called\n");
  return gdbarch->convert_from_func_ptr_addr (gdbarch, addr, targ);
}

void
set_gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch,
                                        gdbarch_convert_from_func_ptr_addr_ftype convert_from_func_ptr_addr)
{
  gdbarch->convert_from_func_ptr_addr = convert_from_func_ptr_addr;
}

CORE_ADDR
gdbarch_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->addr_bits_remove != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_addr_bits_remove called\n");
  return gdbarch->addr_bits_remove (gdbarch, addr);
}

void
set_gdbarch_addr_bits_remove (struct gdbarch *gdbarch,
                              gdbarch_addr_bits_remove_ftype addr_bits_remove)
{
  gdbarch->addr_bits_remove = addr_bits_remove;
}

int
gdbarch_software_single_step_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->software_single_step != NULL;
}

VEC (CORE_ADDR) *
gdbarch_software_single_step (struct gdbarch *gdbarch, struct regcache *regcache)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->software_single_step != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_software_single_step called\n");
  return gdbarch->software_single_step (regcache);
}

void
set_gdbarch_software_single_step (struct gdbarch *gdbarch,
                                  gdbarch_software_single_step_ftype software_single_step)
{
  gdbarch->software_single_step = software_single_step;
}

int
gdbarch_single_step_through_delay_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->single_step_through_delay != NULL;
}

int
gdbarch_single_step_through_delay (struct gdbarch *gdbarch, struct frame_info *frame)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->single_step_through_delay != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_single_step_through_delay called\n");
  return gdbarch->single_step_through_delay (gdbarch, frame);
}

void
set_gdbarch_single_step_through_delay (struct gdbarch *gdbarch,
                                       gdbarch_single_step_through_delay_ftype single_step_through_delay)
{
  gdbarch->single_step_through_delay = single_step_through_delay;
}

int
gdbarch_print_insn (struct gdbarch *gdbarch, bfd_vma vma, struct disassemble_info *info)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->print_insn != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_print_insn called\n");
  return gdbarch->print_insn (vma, info);
}

void
set_gdbarch_print_insn (struct gdbarch *gdbarch,
                        gdbarch_print_insn_ftype print_insn)
{
  gdbarch->print_insn = print_insn;
}

CORE_ADDR
gdbarch_skip_trampoline_code (struct gdbarch *gdbarch, struct frame_info *frame, CORE_ADDR pc)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_trampoline_code != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_trampoline_code called\n");
  return gdbarch->skip_trampoline_code (frame, pc);
}

void
set_gdbarch_skip_trampoline_code (struct gdbarch *gdbarch,
                                  gdbarch_skip_trampoline_code_ftype skip_trampoline_code)
{
  gdbarch->skip_trampoline_code = skip_trampoline_code;
}

CORE_ADDR
gdbarch_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_solib_resolver != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_solib_resolver called\n");
  return gdbarch->skip_solib_resolver (gdbarch, pc);
}

void
set_gdbarch_skip_solib_resolver (struct gdbarch *gdbarch,
                                 gdbarch_skip_solib_resolver_ftype skip_solib_resolver)
{
  gdbarch->skip_solib_resolver = skip_solib_resolver;
}

int
gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->in_solib_return_trampoline != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_in_solib_return_trampoline called\n");
  return gdbarch->in_solib_return_trampoline (gdbarch, pc, name);
}

void
set_gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch,
                                        gdbarch_in_solib_return_trampoline_ftype in_solib_return_trampoline)
{
  gdbarch->in_solib_return_trampoline = in_solib_return_trampoline;
}

int
gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->stack_frame_destroyed_p != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stack_frame_destroyed_p called\n");
  return gdbarch->stack_frame_destroyed_p (gdbarch, addr);
}

void
set_gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch,
                                     gdbarch_stack_frame_destroyed_p_ftype stack_frame_destroyed_p)
{
  gdbarch->stack_frame_destroyed_p = stack_frame_destroyed_p;
}

int
gdbarch_elf_make_msymbol_special_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->elf_make_msymbol_special != NULL;
}

void
gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch, asymbol *sym, struct minimal_symbol *msym)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->elf_make_msymbol_special != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_elf_make_msymbol_special called\n");
  gdbarch->elf_make_msymbol_special (sym, msym);
}

void
set_gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch,
                                      gdbarch_elf_make_msymbol_special_ftype elf_make_msymbol_special)
{
  gdbarch->elf_make_msymbol_special = elf_make_msymbol_special;
}

void
gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch, int val, struct minimal_symbol *msym)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->coff_make_msymbol_special != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_coff_make_msymbol_special called\n");
  gdbarch->coff_make_msymbol_special (val, msym);
}

void
set_gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch,
                                       gdbarch_coff_make_msymbol_special_ftype coff_make_msymbol_special)
{
  gdbarch->coff_make_msymbol_special = coff_make_msymbol_special;
}

void
gdbarch_make_symbol_special (struct gdbarch *gdbarch, struct symbol *sym, struct objfile *objfile)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->make_symbol_special != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_make_symbol_special called\n");
  gdbarch->make_symbol_special (sym, objfile);
}

void
set_gdbarch_make_symbol_special (struct gdbarch *gdbarch,
                                 gdbarch_make_symbol_special_ftype make_symbol_special)
{
  gdbarch->make_symbol_special = make_symbol_special;
}

CORE_ADDR
gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->adjust_dwarf2_addr != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_adjust_dwarf2_addr called\n");
  return gdbarch->adjust_dwarf2_addr (pc);
}

void
set_gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch,
                                gdbarch_adjust_dwarf2_addr_ftype adjust_dwarf2_addr)
{
  gdbarch->adjust_dwarf2_addr = adjust_dwarf2_addr;
}

CORE_ADDR
gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch, CORE_ADDR addr, int rel)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->adjust_dwarf2_line != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_adjust_dwarf2_line called\n");
  return gdbarch->adjust_dwarf2_line (addr, rel);
}

void
set_gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch,
                                gdbarch_adjust_dwarf2_line_ftype adjust_dwarf2_line)
{
  gdbarch->adjust_dwarf2_line = adjust_dwarf2_line;
}

int
gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of cannot_step_breakpoint, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_cannot_step_breakpoint called\n");
  return gdbarch->cannot_step_breakpoint;
}

void
set_gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch,
                                    int cannot_step_breakpoint)
{
  gdbarch->cannot_step_breakpoint = cannot_step_breakpoint;
}

int
gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of have_nonsteppable_watchpoint, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_have_nonsteppable_watchpoint called\n");
  return gdbarch->have_nonsteppable_watchpoint;
}

void
set_gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch,
                                          int have_nonsteppable_watchpoint)
{
  gdbarch->have_nonsteppable_watchpoint = have_nonsteppable_watchpoint;
}

int
gdbarch_address_class_type_flags_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->address_class_type_flags != NULL;
}

int
gdbarch_address_class_type_flags (struct gdbarch *gdbarch, int byte_size, int dwarf2_addr_class)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->address_class_type_flags != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_address_class_type_flags called\n");
  return gdbarch->address_class_type_flags (byte_size, dwarf2_addr_class);
}

void
set_gdbarch_address_class_type_flags (struct gdbarch *gdbarch,
                                      gdbarch_address_class_type_flags_ftype address_class_type_flags)
{
  gdbarch->address_class_type_flags = address_class_type_flags;
}

int
gdbarch_address_class_type_flags_to_name_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->address_class_type_flags_to_name != NULL;
}

const char *
gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch, int type_flags)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->address_class_type_flags_to_name != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_address_class_type_flags_to_name called\n");
  return gdbarch->address_class_type_flags_to_name (gdbarch, type_flags);
}

void
set_gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch,
                                              gdbarch_address_class_type_flags_to_name_ftype address_class_type_flags_to_name)
{
  gdbarch->address_class_type_flags_to_name = address_class_type_flags_to_name;
}

int
gdbarch_address_class_name_to_type_flags_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->address_class_name_to_type_flags != NULL;
}

int
gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch, const char *name, int *type_flags_ptr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->address_class_name_to_type_flags != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_address_class_name_to_type_flags called\n");
  return gdbarch->address_class_name_to_type_flags (gdbarch, name, type_flags_ptr);
}

void
set_gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch,
                                              gdbarch_address_class_name_to_type_flags_ftype address_class_name_to_type_flags)
{
  gdbarch->address_class_name_to_type_flags = address_class_name_to_type_flags;
}

int
gdbarch_register_reggroup_p (struct gdbarch *gdbarch, int regnum, struct reggroup *reggroup)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->register_reggroup_p != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_register_reggroup_p called\n");
  return gdbarch->register_reggroup_p (gdbarch, regnum, reggroup);
}

void
set_gdbarch_register_reggroup_p (struct gdbarch *gdbarch,
                                 gdbarch_register_reggroup_p_ftype register_reggroup_p)
{
  gdbarch->register_reggroup_p = register_reggroup_p;
}

int
gdbarch_fetch_pointer_argument_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->fetch_pointer_argument != NULL;
}

CORE_ADDR
gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch, struct frame_info *frame, int argi, struct type *type)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->fetch_pointer_argument != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fetch_pointer_argument called\n");
  return gdbarch->fetch_pointer_argument (frame, argi, type);
}

void
set_gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch,
                                    gdbarch_fetch_pointer_argument_ftype fetch_pointer_argument)
{
  gdbarch->fetch_pointer_argument = fetch_pointer_argument;
}

int
gdbarch_iterate_over_regset_sections_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->iterate_over_regset_sections != NULL;
}

void
gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch, iterate_over_regset_sections_cb *cb, void *cb_data, const struct regcache *regcache)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->iterate_over_regset_sections != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_iterate_over_regset_sections called\n");
  gdbarch->iterate_over_regset_sections (gdbarch, cb, cb_data, regcache);
}

void
set_gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch,
                                          gdbarch_iterate_over_regset_sections_ftype iterate_over_regset_sections)
{
  gdbarch->iterate_over_regset_sections = iterate_over_regset_sections;
}

int
gdbarch_make_corefile_notes_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->make_corefile_notes != NULL;
}

char *
gdbarch_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd, int *note_size)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->make_corefile_notes != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_make_corefile_notes called\n");
  return gdbarch->make_corefile_notes (gdbarch, obfd, note_size);
}

void
set_gdbarch_make_corefile_notes (struct gdbarch *gdbarch,
                                 gdbarch_make_corefile_notes_ftype make_corefile_notes)
{
  gdbarch->make_corefile_notes = make_corefile_notes;
}

int
gdbarch_elfcore_write_linux_prpsinfo_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->elfcore_write_linux_prpsinfo != NULL;
}

char *
gdbarch_elfcore_write_linux_prpsinfo (struct gdbarch *gdbarch, bfd *obfd, char *note_data, int *note_size, const struct elf_internal_linux_prpsinfo *info)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->elfcore_write_linux_prpsinfo != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_elfcore_write_linux_prpsinfo called\n");
  return gdbarch->elfcore_write_linux_prpsinfo (obfd, note_data, note_size, info);
}

void
set_gdbarch_elfcore_write_linux_prpsinfo (struct gdbarch *gdbarch,
                                          gdbarch_elfcore_write_linux_prpsinfo_ftype elfcore_write_linux_prpsinfo)
{
  gdbarch->elfcore_write_linux_prpsinfo = elfcore_write_linux_prpsinfo;
}

int
gdbarch_find_memory_regions_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->find_memory_regions != NULL;
}

int
gdbarch_find_memory_regions (struct gdbarch *gdbarch, find_memory_region_ftype func, void *data)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->find_memory_regions != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_find_memory_regions called\n");
  return gdbarch->find_memory_regions (gdbarch, func, data);
}

void
set_gdbarch_find_memory_regions (struct gdbarch *gdbarch,
                                 gdbarch_find_memory_regions_ftype find_memory_regions)
{
  gdbarch->find_memory_regions = find_memory_regions;
}

int
gdbarch_core_xfer_shared_libraries_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_xfer_shared_libraries != NULL;
}

ULONGEST
gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_xfer_shared_libraries != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_xfer_shared_libraries called\n");
  return gdbarch->core_xfer_shared_libraries (gdbarch, readbuf, offset, len);
}

void
set_gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch,
                                        gdbarch_core_xfer_shared_libraries_ftype core_xfer_shared_libraries)
{
  gdbarch->core_xfer_shared_libraries = core_xfer_shared_libraries;
}

int
gdbarch_core_xfer_shared_libraries_aix_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_xfer_shared_libraries_aix != NULL;
}

ULONGEST
gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_xfer_shared_libraries_aix != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_xfer_shared_libraries_aix called\n");
  return gdbarch->core_xfer_shared_libraries_aix (gdbarch, readbuf, offset, len);
}

void
set_gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch,
                                            gdbarch_core_xfer_shared_libraries_aix_ftype core_xfer_shared_libraries_aix)
{
  gdbarch->core_xfer_shared_libraries_aix = core_xfer_shared_libraries_aix;
}

int
gdbarch_core_pid_to_str_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_pid_to_str != NULL;
}

const char *
gdbarch_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_pid_to_str != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_pid_to_str called\n");
  return gdbarch->core_pid_to_str (gdbarch, ptid);
}

void
set_gdbarch_core_pid_to_str (struct gdbarch *gdbarch,
                             gdbarch_core_pid_to_str_ftype core_pid_to_str)
{
  gdbarch->core_pid_to_str = core_pid_to_str;
}

int
gdbarch_core_thread_name_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_thread_name != NULL;
}

const char *
gdbarch_core_thread_name (struct gdbarch *gdbarch, struct thread_info *thr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_thread_name != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_thread_name called\n");
  return gdbarch->core_thread_name (gdbarch, thr);
}

void
set_gdbarch_core_thread_name (struct gdbarch *gdbarch,
                              gdbarch_core_thread_name_ftype core_thread_name)
{
  gdbarch->core_thread_name = core_thread_name;
}

int
gdbarch_gcore_bfd_target_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->gcore_bfd_target != 0;
}

const char *
gdbarch_gcore_bfd_target (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->gcore_bfd_target != 0);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gcore_bfd_target called\n");
  return gdbarch->gcore_bfd_target;
}

void
set_gdbarch_gcore_bfd_target (struct gdbarch *gdbarch,
                              const char * gcore_bfd_target)
{
  gdbarch->gcore_bfd_target = gcore_bfd_target;
}

int
gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of vtable_function_descriptors, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_vtable_function_descriptors called\n");
  return gdbarch->vtable_function_descriptors;
}

void
set_gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch,
                                         int vtable_function_descriptors)
{
  gdbarch->vtable_function_descriptors = vtable_function_descriptors;
}

int
gdbarch_vbit_in_delta (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of vbit_in_delta, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_vbit_in_delta called\n");
  return gdbarch->vbit_in_delta;
}

void
set_gdbarch_vbit_in_delta (struct gdbarch *gdbarch,
                           int vbit_in_delta)
{
  gdbarch->vbit_in_delta = vbit_in_delta;
}

void
gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch, struct regcache *regcache)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->skip_permanent_breakpoint != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_skip_permanent_breakpoint called\n");
  gdbarch->skip_permanent_breakpoint (regcache);
}

void
set_gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch,
                                       gdbarch_skip_permanent_breakpoint_ftype skip_permanent_breakpoint)
{
  gdbarch->skip_permanent_breakpoint = skip_permanent_breakpoint;
}

int
gdbarch_max_insn_length_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->max_insn_length != 0;
}

ULONGEST
gdbarch_max_insn_length (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Check variable changed from pre-default.  */
  gdb_assert (gdbarch->max_insn_length != 0);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_max_insn_length called\n");
  return gdbarch->max_insn_length;
}

void
set_gdbarch_max_insn_length (struct gdbarch *gdbarch,
                             ULONGEST max_insn_length)
{
  gdbarch->max_insn_length = max_insn_length;
}

int
gdbarch_displaced_step_copy_insn_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->displaced_step_copy_insn != NULL;
}

struct displaced_step_closure *
gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to, struct regcache *regs)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->displaced_step_copy_insn != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_displaced_step_copy_insn called\n");
  return gdbarch->displaced_step_copy_insn (gdbarch, from, to, regs);
}

void
set_gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch,
                                      gdbarch_displaced_step_copy_insn_ftype displaced_step_copy_insn)
{
  gdbarch->displaced_step_copy_insn = displaced_step_copy_insn;
}

int
gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch, struct displaced_step_closure *closure)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->displaced_step_hw_singlestep != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_displaced_step_hw_singlestep called\n");
  return gdbarch->displaced_step_hw_singlestep (gdbarch, closure);
}

void
set_gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch,
                                          gdbarch_displaced_step_hw_singlestep_ftype displaced_step_hw_singlestep)
{
  gdbarch->displaced_step_hw_singlestep = displaced_step_hw_singlestep;
}

int
gdbarch_displaced_step_fixup_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->displaced_step_fixup != NULL;
}

void
gdbarch_displaced_step_fixup (struct gdbarch *gdbarch, struct displaced_step_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->displaced_step_fixup != NULL);
  /* Do not check predicate: gdbarch->displaced_step_fixup != NULL, allow call.  */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_displaced_step_fixup called\n");
  gdbarch->displaced_step_fixup (gdbarch, closure, from, to, regs);
}

void
set_gdbarch_displaced_step_fixup (struct gdbarch *gdbarch,
                                  gdbarch_displaced_step_fixup_ftype displaced_step_fixup)
{
  gdbarch->displaced_step_fixup = displaced_step_fixup;
}

void
gdbarch_displaced_step_free_closure (struct gdbarch *gdbarch, struct displaced_step_closure *closure)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->displaced_step_free_closure != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_displaced_step_free_closure called\n");
  gdbarch->displaced_step_free_closure (gdbarch, closure);
}

void
set_gdbarch_displaced_step_free_closure (struct gdbarch *gdbarch,
                                         gdbarch_displaced_step_free_closure_ftype displaced_step_free_closure)
{
  gdbarch->displaced_step_free_closure = displaced_step_free_closure;
}

CORE_ADDR
gdbarch_displaced_step_location (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->displaced_step_location != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_displaced_step_location called\n");
  return gdbarch->displaced_step_location (gdbarch);
}

void
set_gdbarch_displaced_step_location (struct gdbarch *gdbarch,
                                     gdbarch_displaced_step_location_ftype displaced_step_location)
{
  gdbarch->displaced_step_location = displaced_step_location;
}

int
gdbarch_relocate_instruction_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->relocate_instruction != NULL;
}

void
gdbarch_relocate_instruction (struct gdbarch *gdbarch, CORE_ADDR *to, CORE_ADDR from)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->relocate_instruction != NULL);
  /* Do not check predicate: gdbarch->relocate_instruction != NULL, allow call.  */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_relocate_instruction called\n");
  gdbarch->relocate_instruction (gdbarch, to, from);
}

void
set_gdbarch_relocate_instruction (struct gdbarch *gdbarch,
                                  gdbarch_relocate_instruction_ftype relocate_instruction)
{
  gdbarch->relocate_instruction = relocate_instruction;
}

int
gdbarch_overlay_update_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->overlay_update != NULL;
}

void
gdbarch_overlay_update (struct gdbarch *gdbarch, struct obj_section *osect)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->overlay_update != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_overlay_update called\n");
  gdbarch->overlay_update (osect);
}

void
set_gdbarch_overlay_update (struct gdbarch *gdbarch,
                            gdbarch_overlay_update_ftype overlay_update)
{
  gdbarch->overlay_update = overlay_update;
}

int
gdbarch_core_read_description_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_read_description != NULL;
}

const struct target_desc *
gdbarch_core_read_description (struct gdbarch *gdbarch, struct target_ops *target, bfd *abfd)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_read_description != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_read_description called\n");
  return gdbarch->core_read_description (gdbarch, target, abfd);
}

void
set_gdbarch_core_read_description (struct gdbarch *gdbarch,
                                   gdbarch_core_read_description_ftype core_read_description)
{
  gdbarch->core_read_description = core_read_description;
}

int
gdbarch_static_transform_name_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->static_transform_name != NULL;
}

const char *
gdbarch_static_transform_name (struct gdbarch *gdbarch, const char *name)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->static_transform_name != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_static_transform_name called\n");
  return gdbarch->static_transform_name (name);
}

void
set_gdbarch_static_transform_name (struct gdbarch *gdbarch,
                                   gdbarch_static_transform_name_ftype static_transform_name)
{
  gdbarch->static_transform_name = static_transform_name;
}

int
gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of sofun_address_maybe_missing, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_sofun_address_maybe_missing called\n");
  return gdbarch->sofun_address_maybe_missing;
}

void
set_gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch,
                                         int sofun_address_maybe_missing)
{
  gdbarch->sofun_address_maybe_missing = sofun_address_maybe_missing;
}

int
gdbarch_process_record_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->process_record != NULL;
}

int
gdbarch_process_record (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->process_record != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_process_record called\n");
  return gdbarch->process_record (gdbarch, regcache, addr);
}

void
set_gdbarch_process_record (struct gdbarch *gdbarch,
                            gdbarch_process_record_ftype process_record)
{
  gdbarch->process_record = process_record;
}

int
gdbarch_process_record_signal_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->process_record_signal != NULL;
}

int
gdbarch_process_record_signal (struct gdbarch *gdbarch, struct regcache *regcache, enum gdb_signal signal)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->process_record_signal != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_process_record_signal called\n");
  return gdbarch->process_record_signal (gdbarch, regcache, signal);
}

void
set_gdbarch_process_record_signal (struct gdbarch *gdbarch,
                                   gdbarch_process_record_signal_ftype process_record_signal)
{
  gdbarch->process_record_signal = process_record_signal;
}

int
gdbarch_gdb_signal_from_target_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->gdb_signal_from_target != NULL;
}

enum gdb_signal
gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch, int signo)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->gdb_signal_from_target != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gdb_signal_from_target called\n");
  return gdbarch->gdb_signal_from_target (gdbarch, signo);
}

void
set_gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch,
                                    gdbarch_gdb_signal_from_target_ftype gdb_signal_from_target)
{
  gdbarch->gdb_signal_from_target = gdb_signal_from_target;
}

int
gdbarch_gdb_signal_to_target_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->gdb_signal_to_target != NULL;
}

int
gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch, enum gdb_signal signal)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->gdb_signal_to_target != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gdb_signal_to_target called\n");
  return gdbarch->gdb_signal_to_target (gdbarch, signal);
}

void
set_gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch,
                                  gdbarch_gdb_signal_to_target_ftype gdb_signal_to_target)
{
  gdbarch->gdb_signal_to_target = gdb_signal_to_target;
}

int
gdbarch_get_siginfo_type_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->get_siginfo_type != NULL;
}

struct type *
gdbarch_get_siginfo_type (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->get_siginfo_type != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_get_siginfo_type called\n");
  return gdbarch->get_siginfo_type (gdbarch);
}

void
set_gdbarch_get_siginfo_type (struct gdbarch *gdbarch,
                              gdbarch_get_siginfo_type_ftype get_siginfo_type)
{
  gdbarch->get_siginfo_type = get_siginfo_type;
}

int
gdbarch_record_special_symbol_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->record_special_symbol != NULL;
}

void
gdbarch_record_special_symbol (struct gdbarch *gdbarch, struct objfile *objfile, asymbol *sym)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->record_special_symbol != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_record_special_symbol called\n");
  gdbarch->record_special_symbol (gdbarch, objfile, sym);
}

void
set_gdbarch_record_special_symbol (struct gdbarch *gdbarch,
                                   gdbarch_record_special_symbol_ftype record_special_symbol)
{
  gdbarch->record_special_symbol = record_special_symbol;
}

int
gdbarch_get_syscall_number_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->get_syscall_number != NULL;
}

LONGEST
gdbarch_get_syscall_number (struct gdbarch *gdbarch, ptid_t ptid)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->get_syscall_number != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_get_syscall_number called\n");
  return gdbarch->get_syscall_number (gdbarch, ptid);
}

void
set_gdbarch_get_syscall_number (struct gdbarch *gdbarch,
                                gdbarch_get_syscall_number_ftype get_syscall_number)
{
  gdbarch->get_syscall_number = get_syscall_number;
}

const char *
gdbarch_xml_syscall_file (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of xml_syscall_file, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_xml_syscall_file called\n");
  return gdbarch->xml_syscall_file;
}

void
set_gdbarch_xml_syscall_file (struct gdbarch *gdbarch,
                              const char * xml_syscall_file)
{
  gdbarch->xml_syscall_file = xml_syscall_file;
}

struct syscalls_info *
gdbarch_syscalls_info (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of syscalls_info, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_syscalls_info called\n");
  return gdbarch->syscalls_info;
}

void
set_gdbarch_syscalls_info (struct gdbarch *gdbarch,
                           struct syscalls_info * syscalls_info)
{
  gdbarch->syscalls_info = syscalls_info;
}

const char *const *
gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_integer_prefixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_integer_prefixes called\n");
  return gdbarch->stap_integer_prefixes;
}

void
set_gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch,
                                   const char *const * stap_integer_prefixes)
{
  gdbarch->stap_integer_prefixes = stap_integer_prefixes;
}

const char *const *
gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_integer_suffixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_integer_suffixes called\n");
  return gdbarch->stap_integer_suffixes;
}

void
set_gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch,
                                   const char *const * stap_integer_suffixes)
{
  gdbarch->stap_integer_suffixes = stap_integer_suffixes;
}

const char *const *
gdbarch_stap_register_prefixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_register_prefixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_register_prefixes called\n");
  return gdbarch->stap_register_prefixes;
}

void
set_gdbarch_stap_register_prefixes (struct gdbarch *gdbarch,
                                    const char *const * stap_register_prefixes)
{
  gdbarch->stap_register_prefixes = stap_register_prefixes;
}

const char *const *
gdbarch_stap_register_suffixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_register_suffixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_register_suffixes called\n");
  return gdbarch->stap_register_suffixes;
}

void
set_gdbarch_stap_register_suffixes (struct gdbarch *gdbarch,
                                    const char *const * stap_register_suffixes)
{
  gdbarch->stap_register_suffixes = stap_register_suffixes;
}

const char *const *
gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_register_indirection_prefixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_register_indirection_prefixes called\n");
  return gdbarch->stap_register_indirection_prefixes;
}

void
set_gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch,
                                                const char *const * stap_register_indirection_prefixes)
{
  gdbarch->stap_register_indirection_prefixes = stap_register_indirection_prefixes;
}

const char *const *
gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_register_indirection_suffixes, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_register_indirection_suffixes called\n");
  return gdbarch->stap_register_indirection_suffixes;
}

void
set_gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch,
                                                const char *const * stap_register_indirection_suffixes)
{
  gdbarch->stap_register_indirection_suffixes = stap_register_indirection_suffixes;
}

const char *
gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_gdb_register_prefix, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_gdb_register_prefix called\n");
  return gdbarch->stap_gdb_register_prefix;
}

void
set_gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch,
                                      const char * stap_gdb_register_prefix)
{
  gdbarch->stap_gdb_register_prefix = stap_gdb_register_prefix;
}

const char *
gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of stap_gdb_register_suffix, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_gdb_register_suffix called\n");
  return gdbarch->stap_gdb_register_suffix;
}

void
set_gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch,
                                      const char * stap_gdb_register_suffix)
{
  gdbarch->stap_gdb_register_suffix = stap_gdb_register_suffix;
}

int
gdbarch_stap_is_single_operand_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->stap_is_single_operand != NULL;
}

int
gdbarch_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->stap_is_single_operand != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_is_single_operand called\n");
  return gdbarch->stap_is_single_operand (gdbarch, s);
}

void
set_gdbarch_stap_is_single_operand (struct gdbarch *gdbarch,
                                    gdbarch_stap_is_single_operand_ftype stap_is_single_operand)
{
  gdbarch->stap_is_single_operand = stap_is_single_operand;
}

int
gdbarch_stap_parse_special_token_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->stap_parse_special_token != NULL;
}

int
gdbarch_stap_parse_special_token (struct gdbarch *gdbarch, struct stap_parse_info *p)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->stap_parse_special_token != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_stap_parse_special_token called\n");
  return gdbarch->stap_parse_special_token (gdbarch, p);
}

void
set_gdbarch_stap_parse_special_token (struct gdbarch *gdbarch,
                                      gdbarch_stap_parse_special_token_ftype stap_parse_special_token)
{
  gdbarch->stap_parse_special_token = stap_parse_special_token;
}

int
gdbarch_dtrace_parse_probe_argument_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->dtrace_parse_probe_argument != NULL;
}

void
gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch, struct parser_state *pstate, int narg)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dtrace_parse_probe_argument != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dtrace_parse_probe_argument called\n");
  gdbarch->dtrace_parse_probe_argument (gdbarch, pstate, narg);
}

void
set_gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch,
                                         gdbarch_dtrace_parse_probe_argument_ftype dtrace_parse_probe_argument)
{
  gdbarch->dtrace_parse_probe_argument = dtrace_parse_probe_argument;
}

int
gdbarch_dtrace_probe_is_enabled_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->dtrace_probe_is_enabled != NULL;
}

int
gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dtrace_probe_is_enabled != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dtrace_probe_is_enabled called\n");
  return gdbarch->dtrace_probe_is_enabled (gdbarch, addr);
}

void
set_gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch,
                                     gdbarch_dtrace_probe_is_enabled_ftype dtrace_probe_is_enabled)
{
  gdbarch->dtrace_probe_is_enabled = dtrace_probe_is_enabled;
}

int
gdbarch_dtrace_enable_probe_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->dtrace_enable_probe != NULL;
}

void
gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dtrace_enable_probe != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dtrace_enable_probe called\n");
  gdbarch->dtrace_enable_probe (gdbarch, addr);
}

void
set_gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch,
                                 gdbarch_dtrace_enable_probe_ftype dtrace_enable_probe)
{
  gdbarch->dtrace_enable_probe = dtrace_enable_probe;
}

int
gdbarch_dtrace_disable_probe_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->dtrace_disable_probe != NULL;
}

void
gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->dtrace_disable_probe != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_dtrace_disable_probe called\n");
  gdbarch->dtrace_disable_probe (gdbarch, addr);
}

void
set_gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch,
                                  gdbarch_dtrace_disable_probe_ftype dtrace_disable_probe)
{
  gdbarch->dtrace_disable_probe = dtrace_disable_probe;
}

int
gdbarch_has_global_solist (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of has_global_solist, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_has_global_solist called\n");
  return gdbarch->has_global_solist;
}

void
set_gdbarch_has_global_solist (struct gdbarch *gdbarch,
                               int has_global_solist)
{
  gdbarch->has_global_solist = has_global_solist;
}

int
gdbarch_has_global_breakpoints (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of has_global_breakpoints, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_has_global_breakpoints called\n");
  return gdbarch->has_global_breakpoints;
}

void
set_gdbarch_has_global_breakpoints (struct gdbarch *gdbarch,
                                    int has_global_breakpoints)
{
  gdbarch->has_global_breakpoints = has_global_breakpoints;
}

int
gdbarch_has_shared_address_space (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->has_shared_address_space != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_has_shared_address_space called\n");
  return gdbarch->has_shared_address_space (gdbarch);
}

void
set_gdbarch_has_shared_address_space (struct gdbarch *gdbarch,
                                      gdbarch_has_shared_address_space_ftype has_shared_address_space)
{
  gdbarch->has_shared_address_space = has_shared_address_space;
}

int
gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch, CORE_ADDR addr, char **msg)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->fast_tracepoint_valid_at != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_fast_tracepoint_valid_at called\n");
  return gdbarch->fast_tracepoint_valid_at (gdbarch, addr, msg);
}

void
set_gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch,
                                      gdbarch_fast_tracepoint_valid_at_ftype fast_tracepoint_valid_at)
{
  gdbarch->fast_tracepoint_valid_at = fast_tracepoint_valid_at;
}

void
gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->guess_tracepoint_registers != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_guess_tracepoint_registers called\n");
  gdbarch->guess_tracepoint_registers (gdbarch, regcache, addr);
}

void
set_gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch,
                                        gdbarch_guess_tracepoint_registers_ftype guess_tracepoint_registers)
{
  gdbarch->guess_tracepoint_registers = guess_tracepoint_registers;
}

const char *
gdbarch_auto_charset (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->auto_charset != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_auto_charset called\n");
  return gdbarch->auto_charset ();
}

void
set_gdbarch_auto_charset (struct gdbarch *gdbarch,
                          gdbarch_auto_charset_ftype auto_charset)
{
  gdbarch->auto_charset = auto_charset;
}

const char *
gdbarch_auto_wide_charset (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->auto_wide_charset != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_auto_wide_charset called\n");
  return gdbarch->auto_wide_charset ();
}

void
set_gdbarch_auto_wide_charset (struct gdbarch *gdbarch,
                               gdbarch_auto_wide_charset_ftype auto_wide_charset)
{
  gdbarch->auto_wide_charset = auto_wide_charset;
}

const char *
gdbarch_solib_symbols_extension (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_solib_symbols_extension called\n");
  return gdbarch->solib_symbols_extension;
}

void
set_gdbarch_solib_symbols_extension (struct gdbarch *gdbarch,
                                     const char * solib_symbols_extension)
{
  gdbarch->solib_symbols_extension = solib_symbols_extension;
}

int
gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of has_dos_based_file_system, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_has_dos_based_file_system called\n");
  return gdbarch->has_dos_based_file_system;
}

void
set_gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch,
                                       int has_dos_based_file_system)
{
  gdbarch->has_dos_based_file_system = has_dos_based_file_system;
}

void
gdbarch_gen_return_address (struct gdbarch *gdbarch, struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->gen_return_address != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gen_return_address called\n");
  gdbarch->gen_return_address (gdbarch, ax, value, scope);
}

void
set_gdbarch_gen_return_address (struct gdbarch *gdbarch,
                                gdbarch_gen_return_address_ftype gen_return_address)
{
  gdbarch->gen_return_address = gen_return_address;
}

int
gdbarch_info_proc_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->info_proc != NULL;
}

void
gdbarch_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->info_proc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_info_proc called\n");
  gdbarch->info_proc (gdbarch, args, what);
}

void
set_gdbarch_info_proc (struct gdbarch *gdbarch,
                       gdbarch_info_proc_ftype info_proc)
{
  gdbarch->info_proc = info_proc;
}

int
gdbarch_core_info_proc_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->core_info_proc != NULL;
}

void
gdbarch_core_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->core_info_proc != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_core_info_proc called\n");
  gdbarch->core_info_proc (gdbarch, args, what);
}

void
set_gdbarch_core_info_proc (struct gdbarch *gdbarch,
                            gdbarch_core_info_proc_ftype core_info_proc)
{
  gdbarch->core_info_proc = core_info_proc;
}

void
gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype *cb, void *cb_data, struct objfile *current_objfile)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->iterate_over_objfiles_in_search_order != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_iterate_over_objfiles_in_search_order called\n");
  gdbarch->iterate_over_objfiles_in_search_order (gdbarch, cb, cb_data, current_objfile);
}

void
set_gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch,
                                                   gdbarch_iterate_over_objfiles_in_search_order_ftype iterate_over_objfiles_in_search_order)
{
  gdbarch->iterate_over_objfiles_in_search_order = iterate_over_objfiles_in_search_order;
}

struct ravenscar_arch_ops *
gdbarch_ravenscar_ops (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of ravenscar_ops, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_ravenscar_ops called\n");
  return gdbarch->ravenscar_ops;
}

void
set_gdbarch_ravenscar_ops (struct gdbarch *gdbarch,
                           struct ravenscar_arch_ops * ravenscar_ops)
{
  gdbarch->ravenscar_ops = ravenscar_ops;
}

int
gdbarch_insn_is_call (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->insn_is_call != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_insn_is_call called\n");
  return gdbarch->insn_is_call (gdbarch, addr);
}

void
set_gdbarch_insn_is_call (struct gdbarch *gdbarch,
                          gdbarch_insn_is_call_ftype insn_is_call)
{
  gdbarch->insn_is_call = insn_is_call;
}

int
gdbarch_insn_is_ret (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->insn_is_ret != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_insn_is_ret called\n");
  return gdbarch->insn_is_ret (gdbarch, addr);
}

void
set_gdbarch_insn_is_ret (struct gdbarch *gdbarch,
                         gdbarch_insn_is_ret_ftype insn_is_ret)
{
  gdbarch->insn_is_ret = insn_is_ret;
}

int
gdbarch_insn_is_jump (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->insn_is_jump != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_insn_is_jump called\n");
  return gdbarch->insn_is_jump (gdbarch, addr);
}

void
set_gdbarch_insn_is_jump (struct gdbarch *gdbarch,
                          gdbarch_insn_is_jump_ftype insn_is_jump)
{
  gdbarch->insn_is_jump = insn_is_jump;
}

int
gdbarch_auxv_parse_p (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  return gdbarch->auxv_parse != NULL;
}

int
gdbarch_auxv_parse (struct gdbarch *gdbarch, gdb_byte **readptr, gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->auxv_parse != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_auxv_parse called\n");
  return gdbarch->auxv_parse (gdbarch, readptr, endptr, typep, valp);
}

void
set_gdbarch_auxv_parse (struct gdbarch *gdbarch,
                        gdbarch_auxv_parse_ftype auxv_parse)
{
  gdbarch->auxv_parse = auxv_parse;
}

void
gdbarch_print_auxv_entry (struct gdbarch *gdbarch, struct ui_file *file, CORE_ADDR type, CORE_ADDR val)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->print_auxv_entry != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_print_auxv_entry called\n");
  gdbarch->print_auxv_entry (gdbarch, file, type, val);
}

void
set_gdbarch_print_auxv_entry (struct gdbarch *gdbarch,
                              gdbarch_print_auxv_entry_ftype print_auxv_entry)
{
  gdbarch->print_auxv_entry = print_auxv_entry;
}

int
gdbarch_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->vsyscall_range != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_vsyscall_range called\n");
  return gdbarch->vsyscall_range (gdbarch, range);
}

void
set_gdbarch_vsyscall_range (struct gdbarch *gdbarch,
                            gdbarch_vsyscall_range_ftype vsyscall_range)
{
  gdbarch->vsyscall_range = vsyscall_range;
}

CORE_ADDR
gdbarch_infcall_mmap (struct gdbarch *gdbarch, CORE_ADDR size, unsigned prot)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->infcall_mmap != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_infcall_mmap called\n");
  return gdbarch->infcall_mmap (size, prot);
}

void
set_gdbarch_infcall_mmap (struct gdbarch *gdbarch,
                          gdbarch_infcall_mmap_ftype infcall_mmap)
{
  gdbarch->infcall_mmap = infcall_mmap;
}

void
gdbarch_infcall_munmap (struct gdbarch *gdbarch, CORE_ADDR addr, CORE_ADDR size)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->infcall_munmap != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_infcall_munmap called\n");
  gdbarch->infcall_munmap (addr, size);
}

void
set_gdbarch_infcall_munmap (struct gdbarch *gdbarch,
                            gdbarch_infcall_munmap_ftype infcall_munmap)
{
  gdbarch->infcall_munmap = infcall_munmap;
}

char *
gdbarch_gcc_target_options (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->gcc_target_options != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gcc_target_options called\n");
  return gdbarch->gcc_target_options (gdbarch);
}

void
set_gdbarch_gcc_target_options (struct gdbarch *gdbarch,
                                gdbarch_gcc_target_options_ftype gcc_target_options)
{
  gdbarch->gcc_target_options = gcc_target_options;
}

const char *
gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->gnu_triplet_regexp != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_gnu_triplet_regexp called\n");
  return gdbarch->gnu_triplet_regexp (gdbarch);
}

void
set_gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch,
                                gdbarch_gnu_triplet_regexp_ftype gnu_triplet_regexp)
{
  gdbarch->gnu_triplet_regexp = gnu_triplet_regexp;
}

int
gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  gdb_assert (gdbarch->addressable_memory_unit_size != NULL);
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_addressable_memory_unit_size called\n");
  return gdbarch->addressable_memory_unit_size (gdbarch);
}

void
set_gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch,
                                          gdbarch_addressable_memory_unit_size_ftype addressable_memory_unit_size)
{
  gdbarch->addressable_memory_unit_size = addressable_memory_unit_size;
}

char **
gdbarch_disassembler_options (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of disassembler_options, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_disassembler_options called\n");
  return gdbarch->disassembler_options;
}

void
set_gdbarch_disassembler_options (struct gdbarch *gdbarch,
                                  char ** disassembler_options)
{
  gdbarch->disassembler_options = disassembler_options;
}

const disasm_options_t *
gdbarch_valid_disassembler_options (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != NULL);
  /* Skip verify of valid_disassembler_options, invalid_p == 0 */
  if (gdbarch_debug >= 2)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_valid_disassembler_options called\n");
  return gdbarch->valid_disassembler_options;
}

void
set_gdbarch_valid_disassembler_options (struct gdbarch *gdbarch,
                                        const disasm_options_t * valid_disassembler_options)
{
  gdbarch->valid_disassembler_options = valid_disassembler_options;
}


/* Keep a registry of per-architecture data-pointers required by GDB
   modules.  */

struct gdbarch_data
{
  unsigned index;
  int init_p;
  gdbarch_data_pre_init_ftype *pre_init;
  gdbarch_data_post_init_ftype *post_init;
};

struct gdbarch_data_registration
{
  struct gdbarch_data *data;
  struct gdbarch_data_registration *next;
};

struct gdbarch_data_registry
{
  unsigned nr;
  struct gdbarch_data_registration *registrations;
};

struct gdbarch_data_registry gdbarch_data_registry =
{
  0, NULL,
};

static struct gdbarch_data *
gdbarch_data_register (gdbarch_data_pre_init_ftype *pre_init,
		       gdbarch_data_post_init_ftype *post_init)
{
  struct gdbarch_data_registration **curr;

  /* Append the new registration.  */
  for (curr = &gdbarch_data_registry.registrations;
       (*curr) != NULL;
       curr = &(*curr)->next);
  (*curr) = XNEW (struct gdbarch_data_registration);
  (*curr)->next = NULL;
  (*curr)->data = XNEW (struct gdbarch_data);
  (*curr)->data->index = gdbarch_data_registry.nr++;
  (*curr)->data->pre_init = pre_init;
  (*curr)->data->post_init = post_init;
  (*curr)->data->init_p = 1;
  return (*curr)->data;
}

struct gdbarch_data *
gdbarch_data_register_pre_init (gdbarch_data_pre_init_ftype *pre_init)
{
  return gdbarch_data_register (pre_init, NULL);
}

struct gdbarch_data *
gdbarch_data_register_post_init (gdbarch_data_post_init_ftype *post_init)
{
  return gdbarch_data_register (NULL, post_init);
}

/* Create/delete the gdbarch data vector.  */

static void
alloc_gdbarch_data (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch->data == NULL);
  gdbarch->nr_data = gdbarch_data_registry.nr;
  gdbarch->data = GDBARCH_OBSTACK_CALLOC (gdbarch, gdbarch->nr_data, void *);
}

/* Initialize the current value of the specified per-architecture
   data-pointer.  */

void
deprecated_set_gdbarch_data (struct gdbarch *gdbarch,
			     struct gdbarch_data *data,
			     void *pointer)
{
  gdb_assert (data->index < gdbarch->nr_data);
  gdb_assert (gdbarch->data[data->index] == NULL);
  gdb_assert (data->pre_init == NULL);
  gdbarch->data[data->index] = pointer;
}

/* Return the current value of the specified per-architecture
   data-pointer.  */

void *
gdbarch_data (struct gdbarch *gdbarch, struct gdbarch_data *data)
{
  gdb_assert (data->index < gdbarch->nr_data);
  if (gdbarch->data[data->index] == NULL)
    {
      /* The data-pointer isn't initialized, call init() to get a
	 value.  */
      if (data->pre_init != NULL)
	/* Mid architecture creation: pass just the obstack, and not
	   the entire architecture, as that way it isn't possible for
	   pre-init code to refer to undefined architecture
	   fields.  */
	gdbarch->data[data->index] = data->pre_init (gdbarch->obstack);
      else if (gdbarch->initialized_p
	       && data->post_init != NULL)
	/* Post architecture creation: pass the entire architecture
	   (as all fields are valid), but be careful to also detect
	   recursive references.  */
	{
	  gdb_assert (data->init_p);
	  data->init_p = 0;
	  gdbarch->data[data->index] = data->post_init (gdbarch);
	  data->init_p = 1;
	}
      else
	/* The architecture initialization hasn't completed - punt -
	 hope that the caller knows what they are doing.  Once
	 deprecated_set_gdbarch_data has been initialized, this can be
	 changed to an internal error.  */
	return NULL;
      gdb_assert (gdbarch->data[data->index] != NULL);
    }
  return gdbarch->data[data->index];
}


/* Keep a registry of the architectures known by GDB.  */

struct gdbarch_registration
{
  enum bfd_architecture bfd_architecture;
  gdbarch_init_ftype *init;
  gdbarch_dump_tdep_ftype *dump_tdep;
  struct gdbarch_list *arches;
  struct gdbarch_registration *next;
};

static struct gdbarch_registration *gdbarch_registry = NULL;

static void
append_name (const char ***buf, int *nr, const char *name)
{
  *buf = XRESIZEVEC (const char *, *buf, *nr + 1);
  (*buf)[*nr] = name;
  *nr += 1;
}

const char **
gdbarch_printable_names (void)
{
  /* Accumulate a list of names based on the registed list of
     architectures.  */
  int nr_arches = 0;
  const char **arches = NULL;
  struct gdbarch_registration *rego;

  for (rego = gdbarch_registry;
       rego != NULL;
       rego = rego->next)
    {
      const struct bfd_arch_info *ap;
      ap = bfd_lookup_arch (rego->bfd_architecture, 0);
      if (ap == NULL)
        internal_error (__FILE__, __LINE__,
                        _("gdbarch_architecture_names: multi-arch unknown"));
      do
        {
          append_name (&arches, &nr_arches, ap->printable_name);
          ap = ap->next;
        }
      while (ap != NULL);
    }
  append_name (&arches, &nr_arches, NULL);
  return arches;
}


void
gdbarch_register (enum bfd_architecture bfd_architecture,
                  gdbarch_init_ftype *init,
		  gdbarch_dump_tdep_ftype *dump_tdep)
{
  struct gdbarch_registration **curr;
  const struct bfd_arch_info *bfd_arch_info;

  /* Check that BFD recognizes this architecture */
  bfd_arch_info = bfd_lookup_arch (bfd_architecture, 0);
  if (bfd_arch_info == NULL)
    {
      internal_error (__FILE__, __LINE__,
                      _("gdbarch: Attempt to register "
			"unknown architecture (%d)"),
                      bfd_architecture);
    }
  /* Check that we haven't seen this architecture before.  */
  for (curr = &gdbarch_registry;
       (*curr) != NULL;
       curr = &(*curr)->next)
    {
      if (bfd_architecture == (*curr)->bfd_architecture)
	internal_error (__FILE__, __LINE__,
                        _("gdbarch: Duplicate registration "
			  "of architecture (%s)"),
	                bfd_arch_info->printable_name);
    }
  /* log it */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "register_gdbarch_init (%s, %s)\n",
			bfd_arch_info->printable_name,
			host_address_to_string (init));
  /* Append it */
  (*curr) = XNEW (struct gdbarch_registration);
  (*curr)->bfd_architecture = bfd_architecture;
  (*curr)->init = init;
  (*curr)->dump_tdep = dump_tdep;
  (*curr)->arches = NULL;
  (*curr)->next = NULL;
}

void
register_gdbarch_init (enum bfd_architecture bfd_architecture,
		       gdbarch_init_ftype *init)
{
  gdbarch_register (bfd_architecture, init, NULL);
}


/* Look for an architecture using gdbarch_info.  */

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
      if (info->osabi != arches->gdbarch->osabi)
	continue;
      if (info->target_desc != arches->gdbarch->target_desc)
	continue;
      return arches;
    }
  return NULL;
}


/* Find an architecture that matches the specified INFO.  Create a new
   architecture if needed.  Return that new architecture.  */

struct gdbarch *
gdbarch_find_by_info (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;
  struct gdbarch_registration *rego;

  /* Fill in missing parts of the INFO struct using a number of
     sources: "set ..."; INFOabfd supplied; and the global
     defaults.  */
  gdbarch_info_fill (&info);

  /* Must have found some sort of architecture.  */
  gdb_assert (info.bfd_arch_info != NULL);

  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.bfd_arch_info %s\n",
			  (info.bfd_arch_info != NULL
			   ? info.bfd_arch_info->printable_name
			   : "(null)"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.byte_order %d (%s)\n",
			  info.byte_order,
			  (info.byte_order == BFD_ENDIAN_BIG ? "big"
			   : info.byte_order == BFD_ENDIAN_LITTLE ? "little"
			   : "default"));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.osabi %d (%s)\n",
			  info.osabi, gdbarch_osabi_name (info.osabi));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.abfd %s\n",
			  host_address_to_string (info.abfd));
      fprintf_unfiltered (gdb_stdlog,
			  "gdbarch_find_by_info: info.tdep_info %s\n",
			  host_address_to_string (info.tdep_info));
    }

  /* Find the tdep code that knows about this architecture.  */
  for (rego = gdbarch_registry;
       rego != NULL;
       rego = rego->next)
    if (rego->bfd_architecture == info.bfd_arch_info->arch)
      break;
  if (rego == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "No matching architecture\n");
      return 0;
    }

  /* Ask the tdep code for an architecture that matches "info".  */
  new_gdbarch = rego->init (info, rego->arches);

  /* Did the tdep code like it?  No.  Reject the change and revert to
     the old architecture.  */
  if (new_gdbarch == NULL)
    {
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "Target rejected architecture\n");
      return NULL;
    }

  /* Is this a pre-existing architecture (as determined by already
     being initialized)?  Move it to the front of the architecture
     list (keeping the list sorted Most Recently Used).  */
  if (new_gdbarch->initialized_p)
    {
      struct gdbarch_list **list;
      struct gdbarch_list *self;
      if (gdbarch_debug)
	fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			    "Previous architecture %s (%s) selected\n",
			    host_address_to_string (new_gdbarch),
			    new_gdbarch->bfd_arch_info->printable_name);
      /* Find the existing arch in the list.  */
      for (list = &rego->arches;
	   (*list) != NULL && (*list)->gdbarch != new_gdbarch;
	   list = &(*list)->next);
      /* It had better be in the list of architectures.  */
      gdb_assert ((*list) != NULL && (*list)->gdbarch == new_gdbarch);
      /* Unlink SELF.  */
      self = (*list);
      (*list) = self->next;
      /* Insert SELF at the front.  */
      self->next = rego->arches;
      rego->arches = self;
      /* Return it.  */
      return new_gdbarch;
    }

  /* It's a new architecture.  */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "gdbarch_find_by_info: "
			"New architecture %s (%s) selected\n",
			host_address_to_string (new_gdbarch),
			new_gdbarch->bfd_arch_info->printable_name);
  
  /* Insert the new architecture into the front of the architecture
     list (keep the list sorted Most Recently Used).  */
  {
    struct gdbarch_list *self = XNEW (struct gdbarch_list);
    self->next = rego->arches;
    self->gdbarch = new_gdbarch;
    rego->arches = self;
  }    

  /* Check that the newly installed architecture is valid.  Plug in
     any post init values.  */
  new_gdbarch->dump_tdep = rego->dump_tdep;
  verify_gdbarch (new_gdbarch);
  new_gdbarch->initialized_p = 1;

  if (gdbarch_debug)
    gdbarch_dump (new_gdbarch, gdb_stdlog);

  return new_gdbarch;
}

/* Make the specified architecture current.  */

void
set_target_gdbarch (struct gdbarch *new_gdbarch)
{
  gdb_assert (new_gdbarch != NULL);
  gdb_assert (new_gdbarch->initialized_p);
  current_inferior ()->gdbarch = new_gdbarch;
  observer_notify_architecture_changed (new_gdbarch);
  registers_changed ();
}

/* Return the current inferior's arch.  */

struct gdbarch *
target_gdbarch (void)
{
  return current_inferior ()->gdbarch;
}

extern void _initialize_gdbarch (void);

void
_initialize_gdbarch (void)
{
  add_setshow_zuinteger_cmd ("arch", class_maintenance, &gdbarch_debug, _("\
Set architecture debugging."), _("\
Show architecture debugging."), _("\
When non-zero, architecture debugging is enabled."),
                            NULL,
                            show_gdbarch_debug,
                            &setdebuglist, &showdebuglist);
}
