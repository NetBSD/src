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

#ifndef GDBARCH_H
#define GDBARCH_H

#include "frame.h"
#include "dis-asm.h"

struct floatformat;
struct ui_file;
struct value;
struct objfile;
struct obj_section;
struct minimal_symbol;
struct regcache;
struct reggroup;
struct regset;
struct disassemble_info;
struct target_ops;
struct obstack;
struct bp_target_info;
struct target_desc;
struct objfile;
struct symbol;
struct displaced_step_closure;
struct syscall;
struct agent_expr;
struct axs_value;
struct stap_parse_info;
struct parser_state;
struct ravenscar_arch_ops;
struct elf_internal_linux_prpsinfo;
struct mem_range;
struct syscalls_info;
struct thread_info;
struct ui_out;

#include "regcache.h"

/* The architecture associated with the inferior through the
   connection to the target.

   The architecture vector provides some information that is really a
   property of the inferior, accessed through a particular target:
   ptrace operations; the layout of certain RSP packets; the solib_ops
   vector; etc.  To differentiate architecture accesses to
   per-inferior/target properties from
   per-thread/per-frame/per-objfile properties, accesses to
   per-inferior/target properties should be made through this
   gdbarch.  */

/* This is a convenience wrapper for 'current_inferior ()->gdbarch'.  */
extern struct gdbarch *target_gdbarch (void);

/* Callback type for the 'iterate_over_objfiles_in_search_order'
   gdbarch  method.  */

typedef int (iterate_over_objfiles_in_search_order_cb_ftype)
  (struct objfile *objfile, void *cb_data);

/* Callback type for regset section iterators.  The callback usually
   invokes the REGSET's supply or collect method, to which it must
   pass a buffer with at least the given SIZE.  SECT_NAME is a BFD
   section name, and HUMAN_NAME is used for diagnostic messages.
   CB_DATA should have been passed unchanged through the iterator.  */

typedef void (iterate_over_regset_sections_cb)
  (const char *sect_name, int size, const struct regset *regset,
   const char *human_name, void *cb_data);


/* The following are pre-initialized by GDBARCH.  */

extern const struct bfd_arch_info * gdbarch_bfd_arch_info (struct gdbarch *gdbarch);
/* set_gdbarch_bfd_arch_info() - not applicable - pre-initialized.  */

extern enum bfd_endian gdbarch_byte_order (struct gdbarch *gdbarch);
/* set_gdbarch_byte_order() - not applicable - pre-initialized.  */

extern enum bfd_endian gdbarch_byte_order_for_code (struct gdbarch *gdbarch);
/* set_gdbarch_byte_order_for_code() - not applicable - pre-initialized.  */

extern enum gdb_osabi gdbarch_osabi (struct gdbarch *gdbarch);
/* set_gdbarch_osabi() - not applicable - pre-initialized.  */

extern const struct target_desc * gdbarch_target_desc (struct gdbarch *gdbarch);
/* set_gdbarch_target_desc() - not applicable - pre-initialized.  */


/* The following are initialized by the target dependent code.  */

/* The bit byte-order has to do just with numbering of bits in debugging symbols
   and such.  Conceptually, it's quite separate from byte/word byte order. */

extern int gdbarch_bits_big_endian (struct gdbarch *gdbarch);
extern void set_gdbarch_bits_big_endian (struct gdbarch *gdbarch, int bits_big_endian);

/* Number of bits in a char or unsigned char for the target machine.
   Just like CHAR_BIT in <limits.h> but describes the target machine.
   v:TARGET_CHAR_BIT:int:char_bit::::8 * sizeof (char):8::0:
  
   Number of bits in a short or unsigned short for the target machine. */

extern int gdbarch_short_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_short_bit (struct gdbarch *gdbarch, int short_bit);

/* Number of bits in an int or unsigned int for the target machine. */

extern int gdbarch_int_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_int_bit (struct gdbarch *gdbarch, int int_bit);

/* Number of bits in a long or unsigned long for the target machine. */

extern int gdbarch_long_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_bit (struct gdbarch *gdbarch, int long_bit);

/* Number of bits in a long long or unsigned long long for the target
   machine. */

extern int gdbarch_long_long_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_long_bit (struct gdbarch *gdbarch, int long_long_bit);

/* Alignment of a long long or unsigned long long for the target
   machine. */

extern int gdbarch_long_long_align_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_long_align_bit (struct gdbarch *gdbarch, int long_long_align_bit);

/* The ABI default bit-size and format for "half", "float", "double", and
   "long double".  These bit/format pairs should eventually be combined
   into a single object.  For the moment, just initialize them as a pair.
   Each format describes both the big and little endian layouts (if
   useful). */

extern int gdbarch_half_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_half_bit (struct gdbarch *gdbarch, int half_bit);

extern const struct floatformat ** gdbarch_half_format (struct gdbarch *gdbarch);
extern void set_gdbarch_half_format (struct gdbarch *gdbarch, const struct floatformat ** half_format);

extern int gdbarch_float_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_float_bit (struct gdbarch *gdbarch, int float_bit);

extern const struct floatformat ** gdbarch_float_format (struct gdbarch *gdbarch);
extern void set_gdbarch_float_format (struct gdbarch *gdbarch, const struct floatformat ** float_format);

extern int gdbarch_double_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_double_bit (struct gdbarch *gdbarch, int double_bit);

extern const struct floatformat ** gdbarch_double_format (struct gdbarch *gdbarch);
extern void set_gdbarch_double_format (struct gdbarch *gdbarch, const struct floatformat ** double_format);

extern int gdbarch_long_double_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_double_bit (struct gdbarch *gdbarch, int long_double_bit);

extern const struct floatformat ** gdbarch_long_double_format (struct gdbarch *gdbarch);
extern void set_gdbarch_long_double_format (struct gdbarch *gdbarch, const struct floatformat ** long_double_format);

/* The ABI default bit-size for "wchar_t".  wchar_t is a built-in type
   starting with C++11. */

extern int gdbarch_wchar_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_wchar_bit (struct gdbarch *gdbarch, int wchar_bit);

/* One if `wchar_t' is signed, zero if unsigned. */

extern int gdbarch_wchar_signed (struct gdbarch *gdbarch);
extern void set_gdbarch_wchar_signed (struct gdbarch *gdbarch, int wchar_signed);

/* Returns the floating-point format to be used for values of length LENGTH.
   NAME, if non-NULL, is the type name, which may be used to distinguish
   different target formats of the same length. */

typedef const struct floatformat ** (gdbarch_floatformat_for_type_ftype) (struct gdbarch *gdbarch, const char *name, int length);
extern const struct floatformat ** gdbarch_floatformat_for_type (struct gdbarch *gdbarch, const char *name, int length);
extern void set_gdbarch_floatformat_for_type (struct gdbarch *gdbarch, gdbarch_floatformat_for_type_ftype *floatformat_for_type);

/* For most targets, a pointer on the target and its representation as an
   address in GDB have the same size and "look the same".  For such a
   target, you need only set gdbarch_ptr_bit and gdbarch_addr_bit
   / addr_bit will be set from it.
  
   If gdbarch_ptr_bit and gdbarch_addr_bit are different, you'll probably
   also need to set gdbarch_dwarf2_addr_size, gdbarch_pointer_to_address and
   gdbarch_address_to_pointer as well.
  
   ptr_bit is the size of a pointer on the target */

extern int gdbarch_ptr_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_ptr_bit (struct gdbarch *gdbarch, int ptr_bit);

/* addr_bit is the size of a target address as represented in gdb */

extern int gdbarch_addr_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_addr_bit (struct gdbarch *gdbarch, int addr_bit);

/* dwarf2_addr_size is the target address size as used in the Dwarf debug
   info.  For .debug_frame FDEs, this is supposed to be the target address
   size from the associated CU header, and which is equivalent to the
   DWARF2_ADDR_SIZE as defined by the target specific GCC back-end.
   Unfortunately there is no good way to determine this value.  Therefore
   dwarf2_addr_size simply defaults to the target pointer size.
  
   dwarf2_addr_size is not used for .eh_frame FDEs, which are generally
   defined using the target's pointer size so far.
  
   Note that dwarf2_addr_size only needs to be redefined by a target if the
   GCC back-end defines a DWARF2_ADDR_SIZE other than the target pointer size,
   and if Dwarf versions < 4 need to be supported. */

extern int gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch);
extern void set_gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch, int dwarf2_addr_size);

/* One if `char' acts like `signed char', zero if `unsigned char'. */

extern int gdbarch_char_signed (struct gdbarch *gdbarch);
extern void set_gdbarch_char_signed (struct gdbarch *gdbarch, int char_signed);

extern int gdbarch_read_pc_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_read_pc_ftype) (struct regcache *regcache);
extern CORE_ADDR gdbarch_read_pc (struct gdbarch *gdbarch, struct regcache *regcache);
extern void set_gdbarch_read_pc (struct gdbarch *gdbarch, gdbarch_read_pc_ftype *read_pc);

extern int gdbarch_write_pc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_write_pc_ftype) (struct regcache *regcache, CORE_ADDR val);
extern void gdbarch_write_pc (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR val);
extern void set_gdbarch_write_pc (struct gdbarch *gdbarch, gdbarch_write_pc_ftype *write_pc);

/* Function for getting target's idea of a frame pointer.  FIXME: GDB's
   whole scheme for dealing with "frames" and "frame pointers" needs a
   serious shakedown. */

typedef void (gdbarch_virtual_frame_pointer_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset);
extern void gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch, CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset);
extern void set_gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch, gdbarch_virtual_frame_pointer_ftype *virtual_frame_pointer);

extern int gdbarch_pseudo_register_read_p (struct gdbarch *gdbarch);

typedef enum register_status (gdbarch_pseudo_register_read_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, gdb_byte *buf);
extern enum register_status gdbarch_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, gdb_byte *buf);
extern void set_gdbarch_pseudo_register_read (struct gdbarch *gdbarch, gdbarch_pseudo_register_read_ftype *pseudo_register_read);

/* Read a register into a new struct value.  If the register is wholly
   or partly unavailable, this should call mark_value_bytes_unavailable
   as appropriate.  If this is defined, then pseudo_register_read will
   never be called. */

extern int gdbarch_pseudo_register_read_value_p (struct gdbarch *gdbarch);

typedef struct value * (gdbarch_pseudo_register_read_value_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum);
extern struct value * gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum);
extern void set_gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch, gdbarch_pseudo_register_read_value_ftype *pseudo_register_read_value);

extern int gdbarch_pseudo_register_write_p (struct gdbarch *gdbarch);

typedef void (gdbarch_pseudo_register_write_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, const gdb_byte *buf);
extern void gdbarch_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, const gdb_byte *buf);
extern void set_gdbarch_pseudo_register_write (struct gdbarch *gdbarch, gdbarch_pseudo_register_write_ftype *pseudo_register_write);

extern int gdbarch_num_regs (struct gdbarch *gdbarch);
extern void set_gdbarch_num_regs (struct gdbarch *gdbarch, int num_regs);

/* This macro gives the number of pseudo-registers that live in the
   register namespace but do not get fetched or stored on the target.
   These pseudo-registers may be aliases for other registers,
   combinations of other registers, or they may be computed by GDB. */

extern int gdbarch_num_pseudo_regs (struct gdbarch *gdbarch);
extern void set_gdbarch_num_pseudo_regs (struct gdbarch *gdbarch, int num_pseudo_regs);

/* Assemble agent expression bytecode to collect pseudo-register REG.
   Return -1 if something goes wrong, 0 otherwise. */

extern int gdbarch_ax_pseudo_register_collect_p (struct gdbarch *gdbarch);

typedef int (gdbarch_ax_pseudo_register_collect_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern int gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern void set_gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch, gdbarch_ax_pseudo_register_collect_ftype *ax_pseudo_register_collect);

/* Assemble agent expression bytecode to push the value of pseudo-register
   REG on the interpreter stack.
   Return -1 if something goes wrong, 0 otherwise. */

extern int gdbarch_ax_pseudo_register_push_stack_p (struct gdbarch *gdbarch);

typedef int (gdbarch_ax_pseudo_register_push_stack_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern int gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern void set_gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch, gdbarch_ax_pseudo_register_push_stack_ftype *ax_pseudo_register_push_stack);

/* Some targets/architectures can do extra processing/display of
   segmentation faults.  E.g., Intel MPX boundary faults.
   Call the architecture dependent function to handle the fault.
   UIOUT is the output stream where the handler will place information. */

extern int gdbarch_handle_segmentation_fault_p (struct gdbarch *gdbarch);

typedef void (gdbarch_handle_segmentation_fault_ftype) (struct gdbarch *gdbarch, struct ui_out *uiout);
extern void gdbarch_handle_segmentation_fault (struct gdbarch *gdbarch, struct ui_out *uiout);
extern void set_gdbarch_handle_segmentation_fault (struct gdbarch *gdbarch, gdbarch_handle_segmentation_fault_ftype *handle_segmentation_fault);

/* GDB's standard (or well known) register numbers.  These can map onto
   a real register or a pseudo (computed) register or not be defined at
   all (-1).
   gdbarch_sp_regnum will hopefully be replaced by UNWIND_SP. */

extern int gdbarch_sp_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_sp_regnum (struct gdbarch *gdbarch, int sp_regnum);

extern int gdbarch_pc_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_pc_regnum (struct gdbarch *gdbarch, int pc_regnum);

extern int gdbarch_ps_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_ps_regnum (struct gdbarch *gdbarch, int ps_regnum);

extern int gdbarch_fp0_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_fp0_regnum (struct gdbarch *gdbarch, int fp0_regnum);

/* Convert stab register number (from `r' declaration) to a gdb REGNUM. */

typedef int (gdbarch_stab_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int stab_regnr);
extern int gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch, int stab_regnr);
extern void set_gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_stab_reg_to_regnum_ftype *stab_reg_to_regnum);

/* Provide a default mapping from a ecoff register number to a gdb REGNUM. */

typedef int (gdbarch_ecoff_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int ecoff_regnr);
extern int gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch, int ecoff_regnr);
extern void set_gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_ecoff_reg_to_regnum_ftype *ecoff_reg_to_regnum);

/* Convert from an sdb register number to an internal gdb register number. */

typedef int (gdbarch_sdb_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int sdb_regnr);
extern int gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch, int sdb_regnr);
extern void set_gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_sdb_reg_to_regnum_ftype *sdb_reg_to_regnum);

/* Provide a default mapping from a DWARF2 register number to a gdb REGNUM.
   Return -1 for bad REGNUM.  Note: Several targets get this wrong. */

typedef int (gdbarch_dwarf2_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int dwarf2_regnr);
extern int gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int dwarf2_regnr);
extern void set_gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_dwarf2_reg_to_regnum_ftype *dwarf2_reg_to_regnum);

typedef const char * (gdbarch_register_name_ftype) (struct gdbarch *gdbarch, int regnr);
extern const char * gdbarch_register_name (struct gdbarch *gdbarch, int regnr);
extern void set_gdbarch_register_name (struct gdbarch *gdbarch, gdbarch_register_name_ftype *register_name);

/* Return the type of a register specified by the architecture.  Only
   the register cache should call this function directly; others should
   use "register_type". */

extern int gdbarch_register_type_p (struct gdbarch *gdbarch);

typedef struct type * (gdbarch_register_type_ftype) (struct gdbarch *gdbarch, int reg_nr);
extern struct type * gdbarch_register_type (struct gdbarch *gdbarch, int reg_nr);
extern void set_gdbarch_register_type (struct gdbarch *gdbarch, gdbarch_register_type_ftype *register_type);

extern int gdbarch_dummy_id_p (struct gdbarch *gdbarch);

typedef struct frame_id (gdbarch_dummy_id_ftype) (struct gdbarch *gdbarch, struct frame_info *this_frame);
extern struct frame_id gdbarch_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame);
extern void set_gdbarch_dummy_id (struct gdbarch *gdbarch, gdbarch_dummy_id_ftype *dummy_id);

/* Implement DUMMY_ID and PUSH_DUMMY_CALL, then delete
   deprecated_fp_regnum. */

extern int gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch, int deprecated_fp_regnum);

extern int gdbarch_push_dummy_call_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_push_dummy_call_ftype) (struct gdbarch *gdbarch, struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr);
extern CORE_ADDR gdbarch_push_dummy_call (struct gdbarch *gdbarch, struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, int struct_return, CORE_ADDR struct_addr);
extern void set_gdbarch_push_dummy_call (struct gdbarch *gdbarch, gdbarch_push_dummy_call_ftype *push_dummy_call);

extern int gdbarch_call_dummy_location (struct gdbarch *gdbarch);
extern void set_gdbarch_call_dummy_location (struct gdbarch *gdbarch, int call_dummy_location);

extern int gdbarch_push_dummy_code_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_push_dummy_code_ftype) (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache);
extern CORE_ADDR gdbarch_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache);
extern void set_gdbarch_push_dummy_code (struct gdbarch *gdbarch, gdbarch_push_dummy_code_ftype *push_dummy_code);

/* Return true if the code of FRAME is writable. */

typedef int (gdbarch_code_of_frame_writable_ftype) (struct gdbarch *gdbarch, struct frame_info *frame);
extern int gdbarch_code_of_frame_writable (struct gdbarch *gdbarch, struct frame_info *frame);
extern void set_gdbarch_code_of_frame_writable (struct gdbarch *gdbarch, gdbarch_code_of_frame_writable_ftype *code_of_frame_writable);

typedef void (gdbarch_print_registers_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, int regnum, int all);
extern void gdbarch_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, int regnum, int all);
extern void set_gdbarch_print_registers_info (struct gdbarch *gdbarch, gdbarch_print_registers_info_ftype *print_registers_info);

typedef void (gdbarch_print_float_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args);
extern void gdbarch_print_float_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args);
extern void set_gdbarch_print_float_info (struct gdbarch *gdbarch, gdbarch_print_float_info_ftype *print_float_info);

extern int gdbarch_print_vector_info_p (struct gdbarch *gdbarch);

typedef void (gdbarch_print_vector_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args);
extern void gdbarch_print_vector_info (struct gdbarch *gdbarch, struct ui_file *file, struct frame_info *frame, const char *args);
extern void set_gdbarch_print_vector_info (struct gdbarch *gdbarch, gdbarch_print_vector_info_ftype *print_vector_info);

/* MAP a GDB RAW register number onto a simulator register number.  See
   also include/...-sim.h. */

typedef int (gdbarch_register_sim_regno_ftype) (struct gdbarch *gdbarch, int reg_nr);
extern int gdbarch_register_sim_regno (struct gdbarch *gdbarch, int reg_nr);
extern void set_gdbarch_register_sim_regno (struct gdbarch *gdbarch, gdbarch_register_sim_regno_ftype *register_sim_regno);

typedef int (gdbarch_cannot_fetch_register_ftype) (struct gdbarch *gdbarch, int regnum);
extern int gdbarch_cannot_fetch_register (struct gdbarch *gdbarch, int regnum);
extern void set_gdbarch_cannot_fetch_register (struct gdbarch *gdbarch, gdbarch_cannot_fetch_register_ftype *cannot_fetch_register);

typedef int (gdbarch_cannot_store_register_ftype) (struct gdbarch *gdbarch, int regnum);
extern int gdbarch_cannot_store_register (struct gdbarch *gdbarch, int regnum);
extern void set_gdbarch_cannot_store_register (struct gdbarch *gdbarch, gdbarch_cannot_store_register_ftype *cannot_store_register);

/* Determine the address where a longjmp will land and save this address
   in PC.  Return nonzero on success.
  
   FRAME corresponds to the longjmp frame. */

extern int gdbarch_get_longjmp_target_p (struct gdbarch *gdbarch);

typedef int (gdbarch_get_longjmp_target_ftype) (struct frame_info *frame, CORE_ADDR *pc);
extern int gdbarch_get_longjmp_target (struct gdbarch *gdbarch, struct frame_info *frame, CORE_ADDR *pc);
extern void set_gdbarch_get_longjmp_target (struct gdbarch *gdbarch, gdbarch_get_longjmp_target_ftype *get_longjmp_target);

extern int gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch);
extern void set_gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch, int believe_pcc_promotion);

typedef int (gdbarch_convert_register_p_ftype) (struct gdbarch *gdbarch, int regnum, struct type *type);
extern int gdbarch_convert_register_p (struct gdbarch *gdbarch, int regnum, struct type *type);
extern void set_gdbarch_convert_register_p (struct gdbarch *gdbarch, gdbarch_convert_register_p_ftype *convert_register_p);

typedef int (gdbarch_register_to_value_ftype) (struct frame_info *frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep);
extern int gdbarch_register_to_value (struct gdbarch *gdbarch, struct frame_info *frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep);
extern void set_gdbarch_register_to_value (struct gdbarch *gdbarch, gdbarch_register_to_value_ftype *register_to_value);

typedef void (gdbarch_value_to_register_ftype) (struct frame_info *frame, int regnum, struct type *type, const gdb_byte *buf);
extern void gdbarch_value_to_register (struct gdbarch *gdbarch, struct frame_info *frame, int regnum, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_value_to_register (struct gdbarch *gdbarch, gdbarch_value_to_register_ftype *value_to_register);

/* Construct a value representing the contents of register REGNUM in
   frame FRAME_ID, interpreted as type TYPE.  The routine needs to
   allocate and return a struct value with all value attributes
   (but not the value contents) filled in. */

typedef struct value * (gdbarch_value_from_register_ftype) (struct gdbarch *gdbarch, struct type *type, int regnum, struct frame_id frame_id);
extern struct value * gdbarch_value_from_register (struct gdbarch *gdbarch, struct type *type, int regnum, struct frame_id frame_id);
extern void set_gdbarch_value_from_register (struct gdbarch *gdbarch, gdbarch_value_from_register_ftype *value_from_register);

typedef CORE_ADDR (gdbarch_pointer_to_address_ftype) (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern CORE_ADDR gdbarch_pointer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_pointer_to_address (struct gdbarch *gdbarch, gdbarch_pointer_to_address_ftype *pointer_to_address);

typedef void (gdbarch_address_to_pointer_ftype) (struct gdbarch *gdbarch, struct type *type, gdb_byte *buf, CORE_ADDR addr);
extern void gdbarch_address_to_pointer (struct gdbarch *gdbarch, struct type *type, gdb_byte *buf, CORE_ADDR addr);
extern void set_gdbarch_address_to_pointer (struct gdbarch *gdbarch, gdbarch_address_to_pointer_ftype *address_to_pointer);

extern int gdbarch_integer_to_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_integer_to_address_ftype) (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern CORE_ADDR gdbarch_integer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_integer_to_address (struct gdbarch *gdbarch, gdbarch_integer_to_address_ftype *integer_to_address);

/* Return the return-value convention that will be used by FUNCTION
   to return a value of type VALTYPE.  FUNCTION may be NULL in which
   case the return convention is computed based only on VALTYPE.
  
   If READBUF is not NULL, extract the return value and save it in this buffer.
  
   If WRITEBUF is not NULL, it contains a return value which will be
   stored into the appropriate register.  This can be used when we want
   to force the value returned by a function (see the "return" command
   for instance). */

extern int gdbarch_return_value_p (struct gdbarch *gdbarch);

typedef enum return_value_convention (gdbarch_return_value_ftype) (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, gdb_byte *readbuf, const gdb_byte *writebuf);
extern enum return_value_convention gdbarch_return_value (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, gdb_byte *readbuf, const gdb_byte *writebuf);
extern void set_gdbarch_return_value (struct gdbarch *gdbarch, gdbarch_return_value_ftype *return_value);

/* Return true if the return value of function is stored in the first hidden
   parameter.  In theory, this feature should be language-dependent, specified
   by language and its ABI, such as C++.  Unfortunately, compiler may
   implement it to a target-dependent feature.  So that we need such hook here
   to be aware of this in GDB. */

typedef int (gdbarch_return_in_first_hidden_param_p_ftype) (struct gdbarch *gdbarch, struct type *type);
extern int gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch, struct type *type);
extern void set_gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch, gdbarch_return_in_first_hidden_param_p_ftype *return_in_first_hidden_param_p);

typedef CORE_ADDR (gdbarch_skip_prologue_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_prologue (struct gdbarch *gdbarch, gdbarch_skip_prologue_ftype *skip_prologue);

extern int gdbarch_skip_main_prologue_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_skip_main_prologue_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_main_prologue (struct gdbarch *gdbarch, gdbarch_skip_main_prologue_ftype *skip_main_prologue);

/* On some platforms, a single function may provide multiple entry points,
   e.g. one that is used for function-pointer calls and a different one
   that is used for direct function calls.
   In order to ensure that breakpoints set on the function will trigger
   no matter via which entry point the function is entered, a platform
   may provide the skip_entrypoint callback.  It is called with IP set
   to the main entry point of a function (as determined by the symbol table),
   and should return the address of the innermost entry point, where the
   actual breakpoint needs to be set.  Note that skip_entrypoint is used
   by GDB common code even when debugging optimized code, where skip_prologue
   is not used. */

extern int gdbarch_skip_entrypoint_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_skip_entrypoint_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_entrypoint (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_entrypoint (struct gdbarch *gdbarch, gdbarch_skip_entrypoint_ftype *skip_entrypoint);

typedef int (gdbarch_inner_than_ftype) (CORE_ADDR lhs, CORE_ADDR rhs);
extern int gdbarch_inner_than (struct gdbarch *gdbarch, CORE_ADDR lhs, CORE_ADDR rhs);
extern void set_gdbarch_inner_than (struct gdbarch *gdbarch, gdbarch_inner_than_ftype *inner_than);

typedef const gdb_byte * (gdbarch_breakpoint_from_pc_ftype) (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr);
extern const gdb_byte * gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr);
extern void set_gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, gdbarch_breakpoint_from_pc_ftype *breakpoint_from_pc);

/* Return the breakpoint kind for this target based on *PCPTR. */

typedef int (gdbarch_breakpoint_kind_from_pc_ftype) (struct gdbarch *gdbarch, CORE_ADDR *pcptr);
extern int gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr);
extern void set_gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch, gdbarch_breakpoint_kind_from_pc_ftype *breakpoint_kind_from_pc);

/* Return the software breakpoint from KIND.  KIND can have target
   specific meaning like the Z0 kind parameter.
   SIZE is set to the software breakpoint's length in memory. */

typedef const gdb_byte * (gdbarch_sw_breakpoint_from_kind_ftype) (struct gdbarch *gdbarch, int kind, int *size);
extern const gdb_byte * gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size);
extern void set_gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch, gdbarch_sw_breakpoint_from_kind_ftype *sw_breakpoint_from_kind);

/* Return the breakpoint kind for this target based on the current
   processor state (e.g. the current instruction mode on ARM) and the
   *PCPTR.  In default, it is gdbarch->breakpoint_kind_from_pc. */

typedef int (gdbarch_breakpoint_kind_from_current_state_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR *pcptr);
extern int gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR *pcptr);
extern void set_gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch, gdbarch_breakpoint_kind_from_current_state_ftype *breakpoint_kind_from_current_state);

extern int gdbarch_adjust_breakpoint_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_adjust_breakpoint_address_ftype) (struct gdbarch *gdbarch, CORE_ADDR bpaddr);
extern CORE_ADDR gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr);
extern void set_gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch, gdbarch_adjust_breakpoint_address_ftype *adjust_breakpoint_address);

typedef int (gdbarch_memory_insert_breakpoint_ftype) (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern int gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern void set_gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, gdbarch_memory_insert_breakpoint_ftype *memory_insert_breakpoint);

typedef int (gdbarch_memory_remove_breakpoint_ftype) (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern int gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern void set_gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, gdbarch_memory_remove_breakpoint_ftype *memory_remove_breakpoint);

extern CORE_ADDR gdbarch_decr_pc_after_break (struct gdbarch *gdbarch);
extern void set_gdbarch_decr_pc_after_break (struct gdbarch *gdbarch, CORE_ADDR decr_pc_after_break);

/* A function can be addressed by either it's "pointer" (possibly a
   descriptor address) or "entry point" (first executable instruction).
   The method "convert_from_func_ptr_addr" converting the former to the
   latter.  gdbarch_deprecated_function_start_offset is being used to implement
   a simplified subset of that functionality - the function's address
   corresponds to the "function pointer" and the function's start
   corresponds to the "function entry point" - and hence is redundant. */

extern CORE_ADDR gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch);
extern void set_gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch, CORE_ADDR deprecated_function_start_offset);

/* Return the remote protocol register number associated with this
   register.  Normally the identity mapping. */

typedef int (gdbarch_remote_register_number_ftype) (struct gdbarch *gdbarch, int regno);
extern int gdbarch_remote_register_number (struct gdbarch *gdbarch, int regno);
extern void set_gdbarch_remote_register_number (struct gdbarch *gdbarch, gdbarch_remote_register_number_ftype *remote_register_number);

/* Fetch the target specific address used to represent a load module. */

extern int gdbarch_fetch_tls_load_module_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_fetch_tls_load_module_address_ftype) (struct objfile *objfile);
extern CORE_ADDR gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch, struct objfile *objfile);
extern void set_gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch, gdbarch_fetch_tls_load_module_address_ftype *fetch_tls_load_module_address);

extern CORE_ADDR gdbarch_frame_args_skip (struct gdbarch *gdbarch);
extern void set_gdbarch_frame_args_skip (struct gdbarch *gdbarch, CORE_ADDR frame_args_skip);

extern int gdbarch_unwind_pc_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_unwind_pc_ftype) (struct gdbarch *gdbarch, struct frame_info *next_frame);
extern CORE_ADDR gdbarch_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame);
extern void set_gdbarch_unwind_pc (struct gdbarch *gdbarch, gdbarch_unwind_pc_ftype *unwind_pc);

extern int gdbarch_unwind_sp_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_unwind_sp_ftype) (struct gdbarch *gdbarch, struct frame_info *next_frame);
extern CORE_ADDR gdbarch_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame);
extern void set_gdbarch_unwind_sp (struct gdbarch *gdbarch, gdbarch_unwind_sp_ftype *unwind_sp);

/* DEPRECATED_FRAME_LOCALS_ADDRESS as been replaced by the per-frame
   frame-base.  Enable frame-base before frame-unwind. */

extern int gdbarch_frame_num_args_p (struct gdbarch *gdbarch);

typedef int (gdbarch_frame_num_args_ftype) (struct frame_info *frame);
extern int gdbarch_frame_num_args (struct gdbarch *gdbarch, struct frame_info *frame);
extern void set_gdbarch_frame_num_args (struct gdbarch *gdbarch, gdbarch_frame_num_args_ftype *frame_num_args);

extern int gdbarch_frame_align_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_frame_align_ftype) (struct gdbarch *gdbarch, CORE_ADDR address);
extern CORE_ADDR gdbarch_frame_align (struct gdbarch *gdbarch, CORE_ADDR address);
extern void set_gdbarch_frame_align (struct gdbarch *gdbarch, gdbarch_frame_align_ftype *frame_align);

typedef int (gdbarch_stabs_argument_has_addr_ftype) (struct gdbarch *gdbarch, struct type *type);
extern int gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch, struct type *type);
extern void set_gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch, gdbarch_stabs_argument_has_addr_ftype *stabs_argument_has_addr);

extern int gdbarch_frame_red_zone_size (struct gdbarch *gdbarch);
extern void set_gdbarch_frame_red_zone_size (struct gdbarch *gdbarch, int frame_red_zone_size);

typedef CORE_ADDR (gdbarch_convert_from_func_ptr_addr_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr, struct target_ops *targ);
extern CORE_ADDR gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr, struct target_ops *targ);
extern void set_gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch, gdbarch_convert_from_func_ptr_addr_ftype *convert_from_func_ptr_addr);

/* On some machines there are bits in addresses which are not really
   part of the address, but are used by the kernel, the hardware, etc.
   for special purposes.  gdbarch_addr_bits_remove takes out any such bits so
   we get a "real" address such as one would find in a symbol table.
   This is used only for addresses of instructions, and even then I'm
   not sure it's used in all contexts.  It exists to deal with there
   being a few stray bits in the PC which would mislead us, not as some
   sort of generic thing to handle alignment or segmentation (it's
   possible it should be in TARGET_READ_PC instead). */

typedef CORE_ADDR (gdbarch_addr_bits_remove_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern CORE_ADDR gdbarch_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_addr_bits_remove (struct gdbarch *gdbarch, gdbarch_addr_bits_remove_ftype *addr_bits_remove);

/* FIXME/cagney/2001-01-18: This should be split in two.  A target method that
   indicates if the target needs software single step.  An ISA method to
   implement it.
  
   FIXME/cagney/2001-01-18: The logic is backwards.  It should be asking if the
   target can single step.  If not, then implement single step using breakpoints.
  
   Return a vector of addresses on which the software single step
   breakpoints should be inserted.  NULL means software single step is
   not used.
   Multiple breakpoints may be inserted for some instructions such as
   conditional branch.  However, each implementation must always evaluate
   the condition and only put the breakpoint at the branch destination if
   the condition is true, so that we ensure forward progress when stepping
   past a conditional branch to self. */

extern int gdbarch_software_single_step_p (struct gdbarch *gdbarch);

typedef VEC (CORE_ADDR) * (gdbarch_software_single_step_ftype) (struct regcache *regcache);
extern VEC (CORE_ADDR) * gdbarch_software_single_step (struct gdbarch *gdbarch, struct regcache *regcache);
extern void set_gdbarch_software_single_step (struct gdbarch *gdbarch, gdbarch_software_single_step_ftype *software_single_step);

/* Return non-zero if the processor is executing a delay slot and a
   further single-step is needed before the instruction finishes. */

extern int gdbarch_single_step_through_delay_p (struct gdbarch *gdbarch);

typedef int (gdbarch_single_step_through_delay_ftype) (struct gdbarch *gdbarch, struct frame_info *frame);
extern int gdbarch_single_step_through_delay (struct gdbarch *gdbarch, struct frame_info *frame);
extern void set_gdbarch_single_step_through_delay (struct gdbarch *gdbarch, gdbarch_single_step_through_delay_ftype *single_step_through_delay);

/* FIXME: cagney/2003-08-28: Need to find a better way of selecting the
   disassembler.  Perhaps objdump can handle it? */

typedef int (gdbarch_print_insn_ftype) (bfd_vma vma, struct disassemble_info *info);
extern int gdbarch_print_insn (struct gdbarch *gdbarch, bfd_vma vma, struct disassemble_info *info);
extern void set_gdbarch_print_insn (struct gdbarch *gdbarch, gdbarch_print_insn_ftype *print_insn);

typedef CORE_ADDR (gdbarch_skip_trampoline_code_ftype) (struct frame_info *frame, CORE_ADDR pc);
extern CORE_ADDR gdbarch_skip_trampoline_code (struct gdbarch *gdbarch, struct frame_info *frame, CORE_ADDR pc);
extern void set_gdbarch_skip_trampoline_code (struct gdbarch *gdbarch, gdbarch_skip_trampoline_code_ftype *skip_trampoline_code);

/* If in_solib_dynsym_resolve_code() returns true, and SKIP_SOLIB_RESOLVER
   evaluates non-zero, this is the address where the debugger will place
   a step-resume breakpoint to get us past the dynamic linker. */

typedef CORE_ADDR (gdbarch_skip_solib_resolver_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc);
extern CORE_ADDR gdbarch_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_skip_solib_resolver (struct gdbarch *gdbarch, gdbarch_skip_solib_resolver_ftype *skip_solib_resolver);

/* Some systems also have trampoline code for returning from shared libs. */

typedef int (gdbarch_in_solib_return_trampoline_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name);
extern int gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name);
extern void set_gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch, gdbarch_in_solib_return_trampoline_ftype *in_solib_return_trampoline);

/* A target might have problems with watchpoints as soon as the stack
   frame of the current function has been destroyed.  This mostly happens
   as the first action in a function's epilogue.  stack_frame_destroyed_p()
   is defined to return a non-zero value if either the given addr is one
   instruction after the stack destroying instruction up to the trailing
   return instruction or if we can figure out that the stack frame has
   already been invalidated regardless of the value of addr.  Targets
   which don't suffer from that problem could just let this functionality
   untouched. */

typedef int (gdbarch_stack_frame_destroyed_p_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch, gdbarch_stack_frame_destroyed_p_ftype *stack_frame_destroyed_p);

/* Process an ELF symbol in the minimal symbol table in a backend-specific
   way.  Normally this hook is supposed to do nothing, however if required,
   then this hook can be used to apply tranformations to symbols that are
   considered special in some way.  For example the MIPS backend uses it
   to interpret `st_other' information to mark compressed code symbols so
   that they can be treated in the appropriate manner in the processing of
   the main symbol table and DWARF-2 records. */

extern int gdbarch_elf_make_msymbol_special_p (struct gdbarch *gdbarch);

typedef void (gdbarch_elf_make_msymbol_special_ftype) (asymbol *sym, struct minimal_symbol *msym);
extern void gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch, asymbol *sym, struct minimal_symbol *msym);
extern void set_gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch, gdbarch_elf_make_msymbol_special_ftype *elf_make_msymbol_special);

typedef void (gdbarch_coff_make_msymbol_special_ftype) (int val, struct minimal_symbol *msym);
extern void gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch, int val, struct minimal_symbol *msym);
extern void set_gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch, gdbarch_coff_make_msymbol_special_ftype *coff_make_msymbol_special);

/* Process a symbol in the main symbol table in a backend-specific way.
   Normally this hook is supposed to do nothing, however if required,
   then this hook can be used to apply tranformations to symbols that
   are considered special in some way.  This is currently used by the
   MIPS backend to make sure compressed code symbols have the ISA bit
   set.  This in turn is needed for symbol values seen in GDB to match
   the values used at the runtime by the program itself, for function
   and label references. */

typedef void (gdbarch_make_symbol_special_ftype) (struct symbol *sym, struct objfile *objfile);
extern void gdbarch_make_symbol_special (struct gdbarch *gdbarch, struct symbol *sym, struct objfile *objfile);
extern void set_gdbarch_make_symbol_special (struct gdbarch *gdbarch, gdbarch_make_symbol_special_ftype *make_symbol_special);

/* Adjust the address retrieved from a DWARF-2 record other than a line
   entry in a backend-specific way.  Normally this hook is supposed to
   return the address passed unchanged, however if that is incorrect for
   any reason, then this hook can be used to fix the address up in the
   required manner.  This is currently used by the MIPS backend to make
   sure addresses in FDE, range records, etc. referring to compressed
   code have the ISA bit set, matching line information and the symbol
   table. */

typedef CORE_ADDR (gdbarch_adjust_dwarf2_addr_ftype) (CORE_ADDR pc);
extern CORE_ADDR gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch, gdbarch_adjust_dwarf2_addr_ftype *adjust_dwarf2_addr);

/* Adjust the address updated by a line entry in a backend-specific way.
   Normally this hook is supposed to return the address passed unchanged,
   however in the case of inconsistencies in these records, this hook can
   be used to fix them up in the required manner.  This is currently used
   by the MIPS backend to make sure all line addresses in compressed code
   are presented with the ISA bit set, which is not always the case.  This
   in turn ensures breakpoint addresses are correctly matched against the
   stop PC. */

typedef CORE_ADDR (gdbarch_adjust_dwarf2_line_ftype) (CORE_ADDR addr, int rel);
extern CORE_ADDR gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch, CORE_ADDR addr, int rel);
extern void set_gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch, gdbarch_adjust_dwarf2_line_ftype *adjust_dwarf2_line);

extern int gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch);
extern void set_gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch, int cannot_step_breakpoint);

extern int gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch);
extern void set_gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch, int have_nonsteppable_watchpoint);

extern int gdbarch_address_class_type_flags_p (struct gdbarch *gdbarch);

typedef int (gdbarch_address_class_type_flags_ftype) (int byte_size, int dwarf2_addr_class);
extern int gdbarch_address_class_type_flags (struct gdbarch *gdbarch, int byte_size, int dwarf2_addr_class);
extern void set_gdbarch_address_class_type_flags (struct gdbarch *gdbarch, gdbarch_address_class_type_flags_ftype *address_class_type_flags);

extern int gdbarch_address_class_type_flags_to_name_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_address_class_type_flags_to_name_ftype) (struct gdbarch *gdbarch, int type_flags);
extern const char * gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch, int type_flags);
extern void set_gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch, gdbarch_address_class_type_flags_to_name_ftype *address_class_type_flags_to_name);

/* Return the appropriate type_flags for the supplied address class.
   This function should return 1 if the address class was recognized and
   type_flags was set, zero otherwise. */

extern int gdbarch_address_class_name_to_type_flags_p (struct gdbarch *gdbarch);

typedef int (gdbarch_address_class_name_to_type_flags_ftype) (struct gdbarch *gdbarch, const char *name, int *type_flags_ptr);
extern int gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch, const char *name, int *type_flags_ptr);
extern void set_gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch, gdbarch_address_class_name_to_type_flags_ftype *address_class_name_to_type_flags);

/* Is a register in a group */

typedef int (gdbarch_register_reggroup_p_ftype) (struct gdbarch *gdbarch, int regnum, struct reggroup *reggroup);
extern int gdbarch_register_reggroup_p (struct gdbarch *gdbarch, int regnum, struct reggroup *reggroup);
extern void set_gdbarch_register_reggroup_p (struct gdbarch *gdbarch, gdbarch_register_reggroup_p_ftype *register_reggroup_p);

/* Fetch the pointer to the ith function argument. */

extern int gdbarch_fetch_pointer_argument_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_fetch_pointer_argument_ftype) (struct frame_info *frame, int argi, struct type *type);
extern CORE_ADDR gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch, struct frame_info *frame, int argi, struct type *type);
extern void set_gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch, gdbarch_fetch_pointer_argument_ftype *fetch_pointer_argument);

/* Iterate over all supported register notes in a core file.  For each
   supported register note section, the iterator must call CB and pass
   CB_DATA unchanged.  If REGCACHE is not NULL, the iterator can limit
   the supported register note sections based on the current register
   values.  Otherwise it should enumerate all supported register note
   sections. */

extern int gdbarch_iterate_over_regset_sections_p (struct gdbarch *gdbarch);

typedef void (gdbarch_iterate_over_regset_sections_ftype) (struct gdbarch *gdbarch, iterate_over_regset_sections_cb *cb, void *cb_data, const struct regcache *regcache);
extern void gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch, iterate_over_regset_sections_cb *cb, void *cb_data, const struct regcache *regcache);
extern void set_gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch, gdbarch_iterate_over_regset_sections_ftype *iterate_over_regset_sections);

/* Create core file notes */

extern int gdbarch_make_corefile_notes_p (struct gdbarch *gdbarch);

typedef char * (gdbarch_make_corefile_notes_ftype) (struct gdbarch *gdbarch, bfd *obfd, int *note_size);
extern char * gdbarch_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd, int *note_size);
extern void set_gdbarch_make_corefile_notes (struct gdbarch *gdbarch, gdbarch_make_corefile_notes_ftype *make_corefile_notes);

/* The elfcore writer hook to use to write Linux prpsinfo notes to core
   files.  Most Linux architectures use the same prpsinfo32 or
   prpsinfo64 layouts, and so won't need to provide this hook, as we
   call the Linux generic routines in bfd to write prpsinfo notes by
   default. */

extern int gdbarch_elfcore_write_linux_prpsinfo_p (struct gdbarch *gdbarch);

typedef char * (gdbarch_elfcore_write_linux_prpsinfo_ftype) (bfd *obfd, char *note_data, int *note_size, const struct elf_internal_linux_prpsinfo *info);
extern char * gdbarch_elfcore_write_linux_prpsinfo (struct gdbarch *gdbarch, bfd *obfd, char *note_data, int *note_size, const struct elf_internal_linux_prpsinfo *info);
extern void set_gdbarch_elfcore_write_linux_prpsinfo (struct gdbarch *gdbarch, gdbarch_elfcore_write_linux_prpsinfo_ftype *elfcore_write_linux_prpsinfo);

/* Find core file memory regions */

extern int gdbarch_find_memory_regions_p (struct gdbarch *gdbarch);

typedef int (gdbarch_find_memory_regions_ftype) (struct gdbarch *gdbarch, find_memory_region_ftype func, void *data);
extern int gdbarch_find_memory_regions (struct gdbarch *gdbarch, find_memory_region_ftype func, void *data);
extern void set_gdbarch_find_memory_regions (struct gdbarch *gdbarch, gdbarch_find_memory_regions_ftype *find_memory_regions);

/* Read offset OFFSET of TARGET_OBJECT_LIBRARIES formatted shared libraries list from
   core file into buffer READBUF with length LEN.  Return the number of bytes read
   (zero indicates failure).
   failed, otherwise, return the red length of READBUF. */

extern int gdbarch_core_xfer_shared_libraries_p (struct gdbarch *gdbarch);

typedef ULONGEST (gdbarch_core_xfer_shared_libraries_ftype) (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern ULONGEST gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern void set_gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch, gdbarch_core_xfer_shared_libraries_ftype *core_xfer_shared_libraries);

/* Read offset OFFSET of TARGET_OBJECT_LIBRARIES_AIX formatted shared
   libraries list from core file into buffer READBUF with length LEN.
   Return the number of bytes read (zero indicates failure). */

extern int gdbarch_core_xfer_shared_libraries_aix_p (struct gdbarch *gdbarch);

typedef ULONGEST (gdbarch_core_xfer_shared_libraries_aix_ftype) (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern ULONGEST gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern void set_gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch, gdbarch_core_xfer_shared_libraries_aix_ftype *core_xfer_shared_libraries_aix);

/* How the core target converts a PTID from a core file to a string. */

extern int gdbarch_core_pid_to_str_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_core_pid_to_str_ftype) (struct gdbarch *gdbarch, ptid_t ptid);
extern const char * gdbarch_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid);
extern void set_gdbarch_core_pid_to_str (struct gdbarch *gdbarch, gdbarch_core_pid_to_str_ftype *core_pid_to_str);

/* How the core target extracts the name of a thread from a core file. */

extern int gdbarch_core_thread_name_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_core_thread_name_ftype) (struct gdbarch *gdbarch, struct thread_info *thr);
extern const char * gdbarch_core_thread_name (struct gdbarch *gdbarch, struct thread_info *thr);
extern void set_gdbarch_core_thread_name (struct gdbarch *gdbarch, gdbarch_core_thread_name_ftype *core_thread_name);

/* BFD target to use when generating a core file. */

extern int gdbarch_gcore_bfd_target_p (struct gdbarch *gdbarch);

extern const char * gdbarch_gcore_bfd_target (struct gdbarch *gdbarch);
extern void set_gdbarch_gcore_bfd_target (struct gdbarch *gdbarch, const char * gcore_bfd_target);

/* If the elements of C++ vtables are in-place function descriptors rather
   than normal function pointers (which may point to code or a descriptor),
   set this to one. */

extern int gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch);
extern void set_gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch, int vtable_function_descriptors);

/* Set if the least significant bit of the delta is used instead of the least
   significant bit of the pfn for pointers to virtual member functions. */

extern int gdbarch_vbit_in_delta (struct gdbarch *gdbarch);
extern void set_gdbarch_vbit_in_delta (struct gdbarch *gdbarch, int vbit_in_delta);

/* Advance PC to next instruction in order to skip a permanent breakpoint. */

typedef void (gdbarch_skip_permanent_breakpoint_ftype) (struct regcache *regcache);
extern void gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch, struct regcache *regcache);
extern void set_gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch, gdbarch_skip_permanent_breakpoint_ftype *skip_permanent_breakpoint);

/* The maximum length of an instruction on this architecture in bytes. */

extern int gdbarch_max_insn_length_p (struct gdbarch *gdbarch);

extern ULONGEST gdbarch_max_insn_length (struct gdbarch *gdbarch);
extern void set_gdbarch_max_insn_length (struct gdbarch *gdbarch, ULONGEST max_insn_length);

/* Copy the instruction at FROM to TO, and make any adjustments
   necessary to single-step it at that address.
  
   REGS holds the state the thread's registers will have before
   executing the copied instruction; the PC in REGS will refer to FROM,
   not the copy at TO.  The caller should update it to point at TO later.
  
   Return a pointer to data of the architecture's choice to be passed
   to gdbarch_displaced_step_fixup.  Or, return NULL to indicate that
   the instruction's effects have been completely simulated, with the
   resulting state written back to REGS.
  
   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c.
  
   The TO area is only guaranteed to have space for
   gdbarch_max_insn_length (arch) bytes, so this function must not
   write more bytes than that to that area.
  
   If you do not provide this function, GDB assumes that the
   architecture does not support displaced stepping.
  
   If your architecture doesn't need to adjust instructions before
   single-stepping them, consider using simple_displaced_step_copy_insn
   here.
  
   If the instruction cannot execute out of line, return NULL.  The
   core falls back to stepping past the instruction in-line instead in
   that case. */

extern int gdbarch_displaced_step_copy_insn_p (struct gdbarch *gdbarch);

typedef struct displaced_step_closure * (gdbarch_displaced_step_copy_insn_ftype) (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern struct displaced_step_closure * gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern void set_gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch, gdbarch_displaced_step_copy_insn_ftype *displaced_step_copy_insn);

/* Return true if GDB should use hardware single-stepping to execute
   the displaced instruction identified by CLOSURE.  If false,
   GDB will simply restart execution at the displaced instruction
   location, and it is up to the target to ensure GDB will receive
   control again (e.g. by placing a software breakpoint instruction
   into the displaced instruction buffer).
  
   The default implementation returns false on all targets that
   provide a gdbarch_software_single_step routine, and true otherwise. */

typedef int (gdbarch_displaced_step_hw_singlestep_ftype) (struct gdbarch *gdbarch, struct displaced_step_closure *closure);
extern int gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch, struct displaced_step_closure *closure);
extern void set_gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch, gdbarch_displaced_step_hw_singlestep_ftype *displaced_step_hw_singlestep);

/* Fix up the state resulting from successfully single-stepping a
   displaced instruction, to give the result we would have gotten from
   stepping the instruction in its original location.
  
   REGS is the register state resulting from single-stepping the
   displaced instruction.
  
   CLOSURE is the result from the matching call to
   gdbarch_displaced_step_copy_insn.
  
   If you provide gdbarch_displaced_step_copy_insn.but not this
   function, then GDB assumes that no fixup is needed after
   single-stepping the instruction.
  
   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c. */

extern int gdbarch_displaced_step_fixup_p (struct gdbarch *gdbarch);

typedef void (gdbarch_displaced_step_fixup_ftype) (struct gdbarch *gdbarch, struct displaced_step_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern void gdbarch_displaced_step_fixup (struct gdbarch *gdbarch, struct displaced_step_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern void set_gdbarch_displaced_step_fixup (struct gdbarch *gdbarch, gdbarch_displaced_step_fixup_ftype *displaced_step_fixup);

/* Free a closure returned by gdbarch_displaced_step_copy_insn.
  
   If you provide gdbarch_displaced_step_copy_insn, you must provide
   this function as well.
  
   If your architecture uses closures that don't need to be freed, then
   you can use simple_displaced_step_free_closure here.
  
   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c. */

typedef void (gdbarch_displaced_step_free_closure_ftype) (struct gdbarch *gdbarch, struct displaced_step_closure *closure);
extern void gdbarch_displaced_step_free_closure (struct gdbarch *gdbarch, struct displaced_step_closure *closure);
extern void set_gdbarch_displaced_step_free_closure (struct gdbarch *gdbarch, gdbarch_displaced_step_free_closure_ftype *displaced_step_free_closure);

/* Return the address of an appropriate place to put displaced
   instructions while we step over them.  There need only be one such
   place, since we're only stepping one thread over a breakpoint at a
   time.
  
   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c. */

typedef CORE_ADDR (gdbarch_displaced_step_location_ftype) (struct gdbarch *gdbarch);
extern CORE_ADDR gdbarch_displaced_step_location (struct gdbarch *gdbarch);
extern void set_gdbarch_displaced_step_location (struct gdbarch *gdbarch, gdbarch_displaced_step_location_ftype *displaced_step_location);

/* Relocate an instruction to execute at a different address.  OLDLOC
   is the address in the inferior memory where the instruction to
   relocate is currently at.  On input, TO points to the destination
   where we want the instruction to be copied (and possibly adjusted)
   to.  On output, it points to one past the end of the resulting
   instruction(s).  The effect of executing the instruction at TO shall
   be the same as if executing it at FROM.  For example, call
   instructions that implicitly push the return address on the stack
   should be adjusted to return to the instruction after OLDLOC;
   relative branches, and other PC-relative instructions need the
   offset adjusted; etc. */

extern int gdbarch_relocate_instruction_p (struct gdbarch *gdbarch);

typedef void (gdbarch_relocate_instruction_ftype) (struct gdbarch *gdbarch, CORE_ADDR *to, CORE_ADDR from);
extern void gdbarch_relocate_instruction (struct gdbarch *gdbarch, CORE_ADDR *to, CORE_ADDR from);
extern void set_gdbarch_relocate_instruction (struct gdbarch *gdbarch, gdbarch_relocate_instruction_ftype *relocate_instruction);

/* Refresh overlay mapped state for section OSECT. */

extern int gdbarch_overlay_update_p (struct gdbarch *gdbarch);

typedef void (gdbarch_overlay_update_ftype) (struct obj_section *osect);
extern void gdbarch_overlay_update (struct gdbarch *gdbarch, struct obj_section *osect);
extern void set_gdbarch_overlay_update (struct gdbarch *gdbarch, gdbarch_overlay_update_ftype *overlay_update);

extern int gdbarch_core_read_description_p (struct gdbarch *gdbarch);

typedef const struct target_desc * (gdbarch_core_read_description_ftype) (struct gdbarch *gdbarch, struct target_ops *target, bfd *abfd);
extern const struct target_desc * gdbarch_core_read_description (struct gdbarch *gdbarch, struct target_ops *target, bfd *abfd);
extern void set_gdbarch_core_read_description (struct gdbarch *gdbarch, gdbarch_core_read_description_ftype *core_read_description);

/* Handle special encoding of static variables in stabs debug info. */

extern int gdbarch_static_transform_name_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_static_transform_name_ftype) (const char *name);
extern const char * gdbarch_static_transform_name (struct gdbarch *gdbarch, const char *name);
extern void set_gdbarch_static_transform_name (struct gdbarch *gdbarch, gdbarch_static_transform_name_ftype *static_transform_name);

/* Set if the address in N_SO or N_FUN stabs may be zero. */

extern int gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch);
extern void set_gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch, int sofun_address_maybe_missing);

/* Parse the instruction at ADDR storing in the record execution log
   the registers REGCACHE and memory ranges that will be affected when
   the instruction executes, along with their current values.
   Return -1 if something goes wrong, 0 otherwise. */

extern int gdbarch_process_record_p (struct gdbarch *gdbarch);

typedef int (gdbarch_process_record_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern int gdbarch_process_record (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void set_gdbarch_process_record (struct gdbarch *gdbarch, gdbarch_process_record_ftype *process_record);

/* Save process state after a signal.
   Return -1 if something goes wrong, 0 otherwise. */

extern int gdbarch_process_record_signal_p (struct gdbarch *gdbarch);

typedef int (gdbarch_process_record_signal_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, enum gdb_signal signal);
extern int gdbarch_process_record_signal (struct gdbarch *gdbarch, struct regcache *regcache, enum gdb_signal signal);
extern void set_gdbarch_process_record_signal (struct gdbarch *gdbarch, gdbarch_process_record_signal_ftype *process_record_signal);

/* Signal translation: translate inferior's signal (target's) number
   into GDB's representation.  The implementation of this method must
   be host independent.  IOW, don't rely on symbols of the NAT_FILE
   header (the nm-*.h files), the host <signal.h> header, or similar
   headers.  This is mainly used when cross-debugging core files ---
   "Live" targets hide the translation behind the target interface
   (target_wait, target_resume, etc.). */

extern int gdbarch_gdb_signal_from_target_p (struct gdbarch *gdbarch);

typedef enum gdb_signal (gdbarch_gdb_signal_from_target_ftype) (struct gdbarch *gdbarch, int signo);
extern enum gdb_signal gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch, int signo);
extern void set_gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch, gdbarch_gdb_signal_from_target_ftype *gdb_signal_from_target);

/* Signal translation: translate the GDB's internal signal number into
   the inferior's signal (target's) representation.  The implementation
   of this method must be host independent.  IOW, don't rely on symbols
   of the NAT_FILE header (the nm-*.h files), the host <signal.h>
   header, or similar headers.
   Return the target signal number if found, or -1 if the GDB internal
   signal number is invalid. */

extern int gdbarch_gdb_signal_to_target_p (struct gdbarch *gdbarch);

typedef int (gdbarch_gdb_signal_to_target_ftype) (struct gdbarch *gdbarch, enum gdb_signal signal);
extern int gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch, enum gdb_signal signal);
extern void set_gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch, gdbarch_gdb_signal_to_target_ftype *gdb_signal_to_target);

/* Extra signal info inspection.
  
   Return a type suitable to inspect extra signal information. */

extern int gdbarch_get_siginfo_type_p (struct gdbarch *gdbarch);

typedef struct type * (gdbarch_get_siginfo_type_ftype) (struct gdbarch *gdbarch);
extern struct type * gdbarch_get_siginfo_type (struct gdbarch *gdbarch);
extern void set_gdbarch_get_siginfo_type (struct gdbarch *gdbarch, gdbarch_get_siginfo_type_ftype *get_siginfo_type);

/* Record architecture-specific information from the symbol table. */

extern int gdbarch_record_special_symbol_p (struct gdbarch *gdbarch);

typedef void (gdbarch_record_special_symbol_ftype) (struct gdbarch *gdbarch, struct objfile *objfile, asymbol *sym);
extern void gdbarch_record_special_symbol (struct gdbarch *gdbarch, struct objfile *objfile, asymbol *sym);
extern void set_gdbarch_record_special_symbol (struct gdbarch *gdbarch, gdbarch_record_special_symbol_ftype *record_special_symbol);

/* Function for the 'catch syscall' feature.
   Get architecture-specific system calls information from registers. */

extern int gdbarch_get_syscall_number_p (struct gdbarch *gdbarch);

typedef LONGEST (gdbarch_get_syscall_number_ftype) (struct gdbarch *gdbarch, ptid_t ptid);
extern LONGEST gdbarch_get_syscall_number (struct gdbarch *gdbarch, ptid_t ptid);
extern void set_gdbarch_get_syscall_number (struct gdbarch *gdbarch, gdbarch_get_syscall_number_ftype *get_syscall_number);

/* The filename of the XML syscall for this architecture. */

extern const char * gdbarch_xml_syscall_file (struct gdbarch *gdbarch);
extern void set_gdbarch_xml_syscall_file (struct gdbarch *gdbarch, const char * xml_syscall_file);

/* Information about system calls from this architecture */

extern struct syscalls_info * gdbarch_syscalls_info (struct gdbarch *gdbarch);
extern void set_gdbarch_syscalls_info (struct gdbarch *gdbarch, struct syscalls_info * syscalls_info);

/* SystemTap related fields and functions.
   A NULL-terminated array of prefixes used to mark an integer constant
   on the architecture's assembly.
   For example, on x86 integer constants are written as:
  
    $10 ;; integer constant 10
  
   in this case, this prefix would be the character `$'. */

extern const char *const * gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch, const char *const * stap_integer_prefixes);

/* A NULL-terminated array of suffixes used to mark an integer constant
   on the architecture's assembly. */

extern const char *const * gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch, const char *const * stap_integer_suffixes);

/* A NULL-terminated array of prefixes used to mark a register name on
   the architecture's assembly.
   For example, on x86 the register name is written as:
  
    %eax ;; register eax
  
   in this case, this prefix would be the character `%'. */

extern const char *const * gdbarch_stap_register_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_prefixes (struct gdbarch *gdbarch, const char *const * stap_register_prefixes);

/* A NULL-terminated array of suffixes used to mark a register name on
   the architecture's assembly. */

extern const char *const * gdbarch_stap_register_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_suffixes (struct gdbarch *gdbarch, const char *const * stap_register_suffixes);

/* A NULL-terminated array of prefixes used to mark a register
   indirection on the architecture's assembly.
   For example, on x86 the register indirection is written as:
  
    (%eax) ;; indirecting eax
  
   in this case, this prefix would be the charater `('.
  
   Please note that we use the indirection prefix also for register
   displacement, e.g., `4(%eax)' on x86. */

extern const char *const * gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch, const char *const * stap_register_indirection_prefixes);

/* A NULL-terminated array of suffixes used to mark a register
   indirection on the architecture's assembly.
   For example, on x86 the register indirection is written as:
  
    (%eax) ;; indirecting eax
  
   in this case, this prefix would be the charater `)'.
  
   Please note that we use the indirection suffix also for register
   displacement, e.g., `4(%eax)' on x86. */

extern const char *const * gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch, const char *const * stap_register_indirection_suffixes);

/* Prefix(es) used to name a register using GDB's nomenclature.
  
   For example, on PPC a register is represented by a number in the assembly
   language (e.g., `10' is the 10th general-purpose register).  However,
   inside GDB this same register has an `r' appended to its name, so the 10th
   register would be represented as `r10' internally. */

extern const char * gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch, const char * stap_gdb_register_prefix);

/* Suffix used to name a register using GDB's nomenclature. */

extern const char * gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch, const char * stap_gdb_register_suffix);

/* Check if S is a single operand.
  
   Single operands can be:
    - Literal integers, e.g. `$10' on x86
    - Register access, e.g. `%eax' on x86
    - Register indirection, e.g. `(%eax)' on x86
    - Register displacement, e.g. `4(%eax)' on x86
  
   This function should check for these patterns on the string
   and return 1 if some were found, or zero otherwise.  Please try to match
   as much info as you can from the string, i.e., if you have to match
   something like `(%', do not match just the `('. */

extern int gdbarch_stap_is_single_operand_p (struct gdbarch *gdbarch);

typedef int (gdbarch_stap_is_single_operand_ftype) (struct gdbarch *gdbarch, const char *s);
extern int gdbarch_stap_is_single_operand (struct gdbarch *gdbarch, const char *s);
extern void set_gdbarch_stap_is_single_operand (struct gdbarch *gdbarch, gdbarch_stap_is_single_operand_ftype *stap_is_single_operand);

/* Function used to handle a "special case" in the parser.
  
   A "special case" is considered to be an unknown token, i.e., a token
   that the parser does not know how to parse.  A good example of special
   case would be ARM's register displacement syntax:
  
    [R0, #4]  ;; displacing R0 by 4
  
   Since the parser assumes that a register displacement is of the form:
  
    <number> <indirection_prefix> <register_name> <indirection_suffix>
  
   it means that it will not be able to recognize and parse this odd syntax.
   Therefore, we should add a special case function that will handle this token.
  
   This function should generate the proper expression form of the expression
   using GDB's internal expression mechanism (e.g., `write_exp_elt_opcode'
   and so on).  It should also return 1 if the parsing was successful, or zero
   if the token was not recognized as a special token (in this case, returning
   zero means that the special parser is deferring the parsing to the generic
   parser), and should advance the buffer pointer (p->arg). */

extern int gdbarch_stap_parse_special_token_p (struct gdbarch *gdbarch);

typedef int (gdbarch_stap_parse_special_token_ftype) (struct gdbarch *gdbarch, struct stap_parse_info *p);
extern int gdbarch_stap_parse_special_token (struct gdbarch *gdbarch, struct stap_parse_info *p);
extern void set_gdbarch_stap_parse_special_token (struct gdbarch *gdbarch, gdbarch_stap_parse_special_token_ftype *stap_parse_special_token);

/* DTrace related functions.
   The expression to compute the NARTGth+1 argument to a DTrace USDT probe.
   NARG must be >= 0. */

extern int gdbarch_dtrace_parse_probe_argument_p (struct gdbarch *gdbarch);

typedef void (gdbarch_dtrace_parse_probe_argument_ftype) (struct gdbarch *gdbarch, struct parser_state *pstate, int narg);
extern void gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch, struct parser_state *pstate, int narg);
extern void set_gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch, gdbarch_dtrace_parse_probe_argument_ftype *dtrace_parse_probe_argument);

/* True if the given ADDR does not contain the instruction sequence
   corresponding to a disabled DTrace is-enabled probe. */

extern int gdbarch_dtrace_probe_is_enabled_p (struct gdbarch *gdbarch);

typedef int (gdbarch_dtrace_probe_is_enabled_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch, gdbarch_dtrace_probe_is_enabled_ftype *dtrace_probe_is_enabled);

/* Enable a DTrace is-enabled probe at ADDR. */

extern int gdbarch_dtrace_enable_probe_p (struct gdbarch *gdbarch);

typedef void (gdbarch_dtrace_enable_probe_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch, gdbarch_dtrace_enable_probe_ftype *dtrace_enable_probe);

/* Disable a DTrace is-enabled probe at ADDR. */

extern int gdbarch_dtrace_disable_probe_p (struct gdbarch *gdbarch);

typedef void (gdbarch_dtrace_disable_probe_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch, gdbarch_dtrace_disable_probe_ftype *dtrace_disable_probe);

/* True if the list of shared libraries is one and only for all
   processes, as opposed to a list of shared libraries per inferior.
   This usually means that all processes, although may or may not share
   an address space, will see the same set of symbols at the same
   addresses. */

extern int gdbarch_has_global_solist (struct gdbarch *gdbarch);
extern void set_gdbarch_has_global_solist (struct gdbarch *gdbarch, int has_global_solist);

/* On some targets, even though each inferior has its own private
   address space, the debug interface takes care of making breakpoints
   visible to all address spaces automatically.  For such cases,
   this property should be set to true. */

extern int gdbarch_has_global_breakpoints (struct gdbarch *gdbarch);
extern void set_gdbarch_has_global_breakpoints (struct gdbarch *gdbarch, int has_global_breakpoints);

/* True if inferiors share an address space (e.g., uClinux). */

typedef int (gdbarch_has_shared_address_space_ftype) (struct gdbarch *gdbarch);
extern int gdbarch_has_shared_address_space (struct gdbarch *gdbarch);
extern void set_gdbarch_has_shared_address_space (struct gdbarch *gdbarch, gdbarch_has_shared_address_space_ftype *has_shared_address_space);

/* True if a fast tracepoint can be set at an address. */

typedef int (gdbarch_fast_tracepoint_valid_at_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr, char **msg);
extern int gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch, CORE_ADDR addr, char **msg);
extern void set_gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch, gdbarch_fast_tracepoint_valid_at_ftype *fast_tracepoint_valid_at);

/* Guess register state based on tracepoint location.  Used for tracepoints
   where no registers have been collected, but there's only one location,
   allowing us to guess the PC value, and perhaps some other registers.
   On entry, regcache has all registers marked as unavailable. */

typedef void (gdbarch_guess_tracepoint_registers_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void set_gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch, gdbarch_guess_tracepoint_registers_ftype *guess_tracepoint_registers);

/* Return the "auto" target charset. */

typedef const char * (gdbarch_auto_charset_ftype) (void);
extern const char * gdbarch_auto_charset (struct gdbarch *gdbarch);
extern void set_gdbarch_auto_charset (struct gdbarch *gdbarch, gdbarch_auto_charset_ftype *auto_charset);

/* Return the "auto" target wide charset. */

typedef const char * (gdbarch_auto_wide_charset_ftype) (void);
extern const char * gdbarch_auto_wide_charset (struct gdbarch *gdbarch);
extern void set_gdbarch_auto_wide_charset (struct gdbarch *gdbarch, gdbarch_auto_wide_charset_ftype *auto_wide_charset);

/* If non-empty, this is a file extension that will be opened in place
   of the file extension reported by the shared library list.
  
   This is most useful for toolchains that use a post-linker tool,
   where the names of the files run on the target differ in extension
   compared to the names of the files GDB should load for debug info. */

extern const char * gdbarch_solib_symbols_extension (struct gdbarch *gdbarch);
extern void set_gdbarch_solib_symbols_extension (struct gdbarch *gdbarch, const char * solib_symbols_extension);

/* If true, the target OS has DOS-based file system semantics.  That
   is, absolute paths include a drive name, and the backslash is
   considered a directory separator. */

extern int gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch);
extern void set_gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch, int has_dos_based_file_system);

/* Generate bytecodes to collect the return address in a frame.
   Since the bytecodes run on the target, possibly with GDB not even
   connected, the full unwinding machinery is not available, and
   typically this function will issue bytecodes for one or more likely
   places that the return address may be found. */

typedef void (gdbarch_gen_return_address_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope);
extern void gdbarch_gen_return_address (struct gdbarch *gdbarch, struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope);
extern void set_gdbarch_gen_return_address (struct gdbarch *gdbarch, gdbarch_gen_return_address_ftype *gen_return_address);

/* Implement the "info proc" command. */

extern int gdbarch_info_proc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_info_proc_ftype) (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void gdbarch_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void set_gdbarch_info_proc (struct gdbarch *gdbarch, gdbarch_info_proc_ftype *info_proc);

/* Implement the "info proc" command for core files.  Noe that there
   are two "info_proc"-like methods on gdbarch -- one for core files,
   one for live targets. */

extern int gdbarch_core_info_proc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_core_info_proc_ftype) (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void gdbarch_core_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void set_gdbarch_core_info_proc (struct gdbarch *gdbarch, gdbarch_core_info_proc_ftype *core_info_proc);

/* Iterate over all objfiles in the order that makes the most sense
   for the architecture to make global symbol searches.
  
   CB is a callback function where OBJFILE is the objfile to be searched,
   and CB_DATA a pointer to user-defined data (the same data that is passed
   when calling this gdbarch method).  The iteration stops if this function
   returns nonzero.
  
   CB_DATA is a pointer to some user-defined data to be passed to
   the callback.
  
   If not NULL, CURRENT_OBJFILE corresponds to the objfile being
   inspected when the symbol search was requested. */

typedef void (gdbarch_iterate_over_objfiles_in_search_order_ftype) (struct gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype *cb, void *cb_data, struct objfile *current_objfile);
extern void gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype *cb, void *cb_data, struct objfile *current_objfile);
extern void set_gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch, gdbarch_iterate_over_objfiles_in_search_order_ftype *iterate_over_objfiles_in_search_order);

/* Ravenscar arch-dependent ops. */

extern struct ravenscar_arch_ops * gdbarch_ravenscar_ops (struct gdbarch *gdbarch);
extern void set_gdbarch_ravenscar_ops (struct gdbarch *gdbarch, struct ravenscar_arch_ops * ravenscar_ops);

/* Return non-zero if the instruction at ADDR is a call; zero otherwise. */

typedef int (gdbarch_insn_is_call_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_call (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_call (struct gdbarch *gdbarch, gdbarch_insn_is_call_ftype *insn_is_call);

/* Return non-zero if the instruction at ADDR is a return; zero otherwise. */

typedef int (gdbarch_insn_is_ret_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_ret (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_ret (struct gdbarch *gdbarch, gdbarch_insn_is_ret_ftype *insn_is_ret);

/* Return non-zero if the instruction at ADDR is a jump; zero otherwise. */

typedef int (gdbarch_insn_is_jump_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_jump (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_jump (struct gdbarch *gdbarch, gdbarch_insn_is_jump_ftype *insn_is_jump);

/* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
   Return 0 if *READPTR is already at the end of the buffer.
   Return -1 if there is insufficient buffer for a whole entry.
   Return 1 if an entry was read into *TYPEP and *VALP. */

extern int gdbarch_auxv_parse_p (struct gdbarch *gdbarch);

typedef int (gdbarch_auxv_parse_ftype) (struct gdbarch *gdbarch, gdb_byte **readptr, gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp);
extern int gdbarch_auxv_parse (struct gdbarch *gdbarch, gdb_byte **readptr, gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp);
extern void set_gdbarch_auxv_parse (struct gdbarch *gdbarch, gdbarch_auxv_parse_ftype *auxv_parse);

/* Print the description of a single auxv entry described by TYPE and VAL
   to FILE. */

typedef void (gdbarch_print_auxv_entry_ftype) (struct gdbarch *gdbarch, struct ui_file *file, CORE_ADDR type, CORE_ADDR val);
extern void gdbarch_print_auxv_entry (struct gdbarch *gdbarch, struct ui_file *file, CORE_ADDR type, CORE_ADDR val);
extern void set_gdbarch_print_auxv_entry (struct gdbarch *gdbarch, gdbarch_print_auxv_entry_ftype *print_auxv_entry);

/* Find the address range of the current inferior's vsyscall/vDSO, and
   write it to *RANGE.  If the vsyscall's length can't be determined, a
   range with zero length is returned.  Returns true if the vsyscall is
   found, false otherwise. */

typedef int (gdbarch_vsyscall_range_ftype) (struct gdbarch *gdbarch, struct mem_range *range);
extern int gdbarch_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range);
extern void set_gdbarch_vsyscall_range (struct gdbarch *gdbarch, gdbarch_vsyscall_range_ftype *vsyscall_range);

/* Allocate SIZE bytes of PROT protected page aligned memory in inferior.
   PROT has GDB_MMAP_PROT_* bitmask format.
   Throw an error if it is not possible.  Returned address is always valid. */

typedef CORE_ADDR (gdbarch_infcall_mmap_ftype) (CORE_ADDR size, unsigned prot);
extern CORE_ADDR gdbarch_infcall_mmap (struct gdbarch *gdbarch, CORE_ADDR size, unsigned prot);
extern void set_gdbarch_infcall_mmap (struct gdbarch *gdbarch, gdbarch_infcall_mmap_ftype *infcall_mmap);

/* Deallocate SIZE bytes of memory at ADDR in inferior from gdbarch_infcall_mmap.
   Print a warning if it is not possible. */

typedef void (gdbarch_infcall_munmap_ftype) (CORE_ADDR addr, CORE_ADDR size);
extern void gdbarch_infcall_munmap (struct gdbarch *gdbarch, CORE_ADDR addr, CORE_ADDR size);
extern void set_gdbarch_infcall_munmap (struct gdbarch *gdbarch, gdbarch_infcall_munmap_ftype *infcall_munmap);

/* Return string (caller has to use xfree for it) with options for GCC
   to produce code for this target, typically "-m64", "-m32" or "-m31".
   These options are put before CU's DW_AT_producer compilation options so that
   they can override it.  Method may also return NULL. */

typedef char * (gdbarch_gcc_target_options_ftype) (struct gdbarch *gdbarch);
extern char * gdbarch_gcc_target_options (struct gdbarch *gdbarch);
extern void set_gdbarch_gcc_target_options (struct gdbarch *gdbarch, gdbarch_gcc_target_options_ftype *gcc_target_options);

/* Return a regular expression that matches names used by this
   architecture in GNU configury triplets.  The result is statically
   allocated and must not be freed.  The default implementation simply
   returns the BFD architecture name, which is correct in nearly every
   case. */

typedef const char * (gdbarch_gnu_triplet_regexp_ftype) (struct gdbarch *gdbarch);
extern const char * gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch);
extern void set_gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch, gdbarch_gnu_triplet_regexp_ftype *gnu_triplet_regexp);

/* Return the size in 8-bit bytes of an addressable memory unit on this
   architecture.  This corresponds to the number of 8-bit bytes associated to
   each address in memory. */

typedef int (gdbarch_addressable_memory_unit_size_ftype) (struct gdbarch *gdbarch);
extern int gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch);
extern void set_gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch, gdbarch_addressable_memory_unit_size_ftype *addressable_memory_unit_size);

/* Functions for allowing a target to modify its disassembler options. */

extern char ** gdbarch_disassembler_options (struct gdbarch *gdbarch);
extern void set_gdbarch_disassembler_options (struct gdbarch *gdbarch, char ** disassembler_options);

extern const disasm_options_t * gdbarch_valid_disassembler_options (struct gdbarch *gdbarch);
extern void set_gdbarch_valid_disassembler_options (struct gdbarch *gdbarch, const disasm_options_t * valid_disassembler_options);

/* Definition for an unknown syscall, used basically in error-cases.  */
#define UNKNOWN_SYSCALL (-1)

extern struct gdbarch_tdep *gdbarch_tdep (struct gdbarch *gdbarch);


/* Mechanism for co-ordinating the selection of a specific
   architecture.

   GDB targets (*-tdep.c) can register an interest in a specific
   architecture.  Other GDB components can register a need to maintain
   per-architecture data.

   The mechanisms below ensures that there is only a loose connection
   between the set-architecture command and the various GDB
   components.  Each component can independently register their need
   to maintain architecture specific data with gdbarch.

   Pragmatics:

   Previously, a single TARGET_ARCHITECTURE_HOOK was provided.  It
   didn't scale.

   The more traditional mega-struct containing architecture specific
   data for all the various GDB components was also considered.  Since
   GDB is built from a variable number of (fairly independent)
   components it was determined that the global aproach was not
   applicable.  */


/* Register a new architectural family with GDB.

   Register support for the specified ARCHITECTURE with GDB.  When
   gdbarch determines that the specified architecture has been
   selected, the corresponding INIT function is called.

   --

   The INIT function takes two parameters: INFO which contains the
   information available to gdbarch about the (possibly new)
   architecture; ARCHES which is a list of the previously created
   ``struct gdbarch'' for this architecture.

   The INFO parameter is, as far as possible, be pre-initialized with
   information obtained from INFO.ABFD or the global defaults.

   The ARCHES parameter is a linked list (sorted most recently used)
   of all the previously created architures for this architecture
   family.  The (possibly NULL) ARCHES->gdbarch can used to access
   values from the previously selected architecture for this
   architecture family.

   The INIT function shall return any of: NULL - indicating that it
   doesn't recognize the selected architecture; an existing ``struct
   gdbarch'' from the ARCHES list - indicating that the new
   architecture is just a synonym for an earlier architecture (see
   gdbarch_list_lookup_by_info()); a newly created ``struct gdbarch''
   - that describes the selected architecture (see gdbarch_alloc()).

   The DUMP_TDEP function shall print out all target specific values.
   Care should be taken to ensure that the function works in both the
   multi-arch and non- multi-arch cases.  */

struct gdbarch_list
{
  struct gdbarch *gdbarch;
  struct gdbarch_list *next;
};

struct gdbarch_info
{
  /* Use default: NULL (ZERO).  */
  const struct bfd_arch_info *bfd_arch_info;

  /* Use default: BFD_ENDIAN_UNKNOWN (NB: is not ZERO).  */
  enum bfd_endian byte_order;

  enum bfd_endian byte_order_for_code;

  /* Use default: NULL (ZERO).  */
  bfd *abfd;

  /* Use default: NULL (ZERO).  */
  void *tdep_info;

  /* Use default: GDB_OSABI_UNINITIALIZED (-1).  */
  enum gdb_osabi osabi;

  /* Use default: NULL (ZERO).  */
  const struct target_desc *target_desc;
};

typedef struct gdbarch *(gdbarch_init_ftype) (struct gdbarch_info info, struct gdbarch_list *arches);
typedef void (gdbarch_dump_tdep_ftype) (struct gdbarch *gdbarch, struct ui_file *file);

/* DEPRECATED - use gdbarch_register() */
extern void register_gdbarch_init (enum bfd_architecture architecture, gdbarch_init_ftype *);

extern void gdbarch_register (enum bfd_architecture architecture,
                              gdbarch_init_ftype *,
                              gdbarch_dump_tdep_ftype *);


/* Return a freshly allocated, NULL terminated, array of the valid
   architecture names.  Since architectures are registered during the
   _initialize phase this function only returns useful information
   once initialization has been completed.  */

extern const char **gdbarch_printable_names (void);


/* Helper function.  Search the list of ARCHES for a GDBARCH that
   matches the information provided by INFO.  */

extern struct gdbarch_list *gdbarch_list_lookup_by_info (struct gdbarch_list *arches, const struct gdbarch_info *info);


/* Helper function.  Create a preliminary ``struct gdbarch''.  Perform
   basic initialization using values obtained from the INFO and TDEP
   parameters.  set_gdbarch_*() functions are called to complete the
   initialization of the object.  */

extern struct gdbarch *gdbarch_alloc (const struct gdbarch_info *info, struct gdbarch_tdep *tdep);


/* Helper function.  Free a partially-constructed ``struct gdbarch''.
   It is assumed that the caller freeds the ``struct
   gdbarch_tdep''.  */

extern void gdbarch_free (struct gdbarch *);


/* Helper function.  Allocate memory from the ``struct gdbarch''
   obstack.  The memory is freed when the corresponding architecture
   is also freed.  */

extern void *gdbarch_obstack_zalloc (struct gdbarch *gdbarch, long size);
#define GDBARCH_OBSTACK_CALLOC(GDBARCH, NR, TYPE) ((TYPE *) gdbarch_obstack_zalloc ((GDBARCH), (NR) * sizeof (TYPE)))
#define GDBARCH_OBSTACK_ZALLOC(GDBARCH, TYPE) ((TYPE *) gdbarch_obstack_zalloc ((GDBARCH), sizeof (TYPE)))

/* Duplicate STRING, returning an equivalent string that's allocated on the
   obstack associated with GDBARCH.  The string is freed when the corresponding
   architecture is also freed.  */

extern char *gdbarch_obstack_strdup (struct gdbarch *arch, const char *string);

/* Helper function.  Force an update of the current architecture.

   The actual architecture selected is determined by INFO, ``(gdb) set
   architecture'' et.al., the existing architecture and BFD's default
   architecture.  INFO should be initialized to zero and then selected
   fields should be updated.

   Returns non-zero if the update succeeds.  */

extern int gdbarch_update_p (struct gdbarch_info info);


/* Helper function.  Find an architecture matching info.

   INFO should be initialized using gdbarch_info_init, relevant fields
   set, and then finished using gdbarch_info_fill.

   Returns the corresponding architecture, or NULL if no matching
   architecture was found.  */

extern struct gdbarch *gdbarch_find_by_info (struct gdbarch_info info);


/* Helper function.  Set the target gdbarch to "gdbarch".  */

extern void set_target_gdbarch (struct gdbarch *gdbarch);


/* Register per-architecture data-pointer.

   Reserve space for a per-architecture data-pointer.  An identifier
   for the reserved data-pointer is returned.  That identifer should
   be saved in a local static variable.

   Memory for the per-architecture data shall be allocated using
   gdbarch_obstack_zalloc.  That memory will be deleted when the
   corresponding architecture object is deleted.

   When a previously created architecture is re-selected, the
   per-architecture data-pointer for that previous architecture is
   restored.  INIT() is not re-called.

   Multiple registrarants for any architecture are allowed (and
   strongly encouraged).  */

struct gdbarch_data;

typedef void *(gdbarch_data_pre_init_ftype) (struct obstack *obstack);
extern struct gdbarch_data *gdbarch_data_register_pre_init (gdbarch_data_pre_init_ftype *init);
typedef void *(gdbarch_data_post_init_ftype) (struct gdbarch *gdbarch);
extern struct gdbarch_data *gdbarch_data_register_post_init (gdbarch_data_post_init_ftype *init);
extern void deprecated_set_gdbarch_data (struct gdbarch *gdbarch,
                                         struct gdbarch_data *data,
			                 void *pointer);

extern void *gdbarch_data (struct gdbarch *gdbarch, struct gdbarch_data *);


/* Set the dynamic target-system-dependent parameters (architecture,
   byte-order, ...) using information found in the BFD.  */

extern void set_gdbarch_from_file (bfd *);


/* Initialize the current architecture to the "first" one we find on
   our list.  */

extern void initialize_current_architecture (void);

/* gdbarch trace variable */
extern unsigned int gdbarch_debug;

extern void gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file);

#endif
