/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2014 Free Software Foundation, Inc.

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

#ifndef GDBARCH_UTILS_H
#define GDBARCH_UTILS_H

struct gdbarch;
struct frame_info;
struct minimal_symbol;
struct type;
struct gdbarch_info;

/* An implementation of gdbarch_displaced_step_copy_insn for
   processors that don't need to modify the instruction before
   single-stepping the displaced copy.

   Simply copy gdbarch_max_insn_length (ARCH) bytes from FROM to TO.
   The closure is an array of that many bytes containing the
   instruction's bytes, allocated with xmalloc.  */
extern struct displaced_step_closure *
  simple_displaced_step_copy_insn (struct gdbarch *gdbarch,
                                   CORE_ADDR from, CORE_ADDR to,
                                   struct regcache *regs);

/* Simple implementation of gdbarch_displaced_step_free_closure: Call
   xfree.
   This is appropriate for use with simple_displaced_step_copy_insn.  */
extern void
  simple_displaced_step_free_closure (struct gdbarch *gdbarch,
                                      struct displaced_step_closure *closure);

/* Default implementation of gdbarch_displaced_hw_singlestep.  */
extern int
  default_displaced_step_hw_singlestep (struct gdbarch *,
					struct displaced_step_closure *);

/* Possible value for gdbarch_displaced_step_location:
   Place displaced instructions at the program's entry point,
   leaving space for inferior function call return breakpoints.  */
extern CORE_ADDR displaced_step_at_entry_point (struct gdbarch *gdbarch);

/* The only possible cases for inner_than.  */
extern int core_addr_lessthan (CORE_ADDR lhs, CORE_ADDR rhs);
extern int core_addr_greaterthan (CORE_ADDR lhs, CORE_ADDR rhs);

/* Identity functions on a CORE_ADDR.  Just return the "addr".  */

extern CORE_ADDR core_addr_identity (struct gdbarch *gdbarch, CORE_ADDR addr);
extern gdbarch_convert_from_func_ptr_addr_ftype convert_from_func_ptr_addr_identity;

/* No-op conversion of reg to regnum.  */

extern int no_op_reg_to_regnum (struct gdbarch *gdbarch, int reg);

/* Do nothing version of elf_make_msymbol_special.  */

void default_elf_make_msymbol_special (asymbol *sym,
				       struct minimal_symbol *msym);

/* Do nothing version of coff_make_msymbol_special.  */

void default_coff_make_msymbol_special (int val, struct minimal_symbol *msym);

/* Version of cannot_fetch_register() / cannot_store_register() that
   always fails.  */

int cannot_register_not (struct gdbarch *gdbarch, int regnum);

/* Legacy version of target_virtual_frame_pointer().  Assumes that
   there is an gdbarch_deprecated_fp_regnum and that it is the same, cooked or
   raw.  */

extern gdbarch_virtual_frame_pointer_ftype legacy_virtual_frame_pointer;

extern CORE_ADDR generic_skip_trampoline_code (struct frame_info *frame,
					       CORE_ADDR pc);

extern CORE_ADDR generic_skip_solib_resolver (struct gdbarch *gdbarch,
					      CORE_ADDR pc);

extern int generic_in_solib_return_trampoline (struct gdbarch *gdbarch,
					       CORE_ADDR pc, const char *name);

extern int generic_in_function_epilogue_p (struct gdbarch *gdbarch,
					   CORE_ADDR pc);

/* By default, registers are not convertible.  */
extern int generic_convert_register_p (struct gdbarch *gdbarch, int regnum,
				       struct type *type);

extern int default_stabs_argument_has_addr (struct gdbarch *gdbarch,
					    struct type *type);

extern int generic_instruction_nullified (struct gdbarch *gdbarch,
					  struct regcache *regcache);

int default_remote_register_number (struct gdbarch *gdbarch,
				    int regno);

/* For compatibility with older architectures, returns
   (LEGACY_SIM_REGNO_IGNORE) when the register doesn't have a valid
   name.  */

extern int legacy_register_sim_regno (struct gdbarch *gdbarch, int regnum);

/* Return the selected byte order, or BFD_ENDIAN_UNKNOWN if no byte
   order was explicitly selected.  */
extern enum bfd_endian selected_byte_order (void);

/* Return the selected architecture's name, or NULL if no architecture
   was explicitly selected.  */
extern const char *selected_architecture_name (void);

/* Initialize a ``struct info''.  Can't use memset(0) since some
   default values are not zero.  "fill" takes all available
   information and fills in any unspecified fields.  */

extern void gdbarch_info_init (struct gdbarch_info *info);

/* Similar to init, but this time fill in the blanks.  Information is
   obtained from the global "set ..." options and explicitly
   initialized INFO fields.  */
extern void gdbarch_info_fill (struct gdbarch_info *info);

/* Return the architecture for ABFD.  If no suitable architecture
   could be find, return NULL.  */

extern struct gdbarch *gdbarch_from_bfd (bfd *abfd);

/* Return "current" architecture.  If the target is running, this is the
   architecture of the selected frame.  Otherwise, the "current" architecture
   defaults to the target architecture.

   This function should normally be called solely by the command interpreter
   routines to determine the architecture to execute a command in.  */
extern struct gdbarch *get_current_arch (void);

extern int default_has_shared_address_space (struct gdbarch *);

extern int default_fast_tracepoint_valid_at (struct gdbarch *gdbarch,
					     CORE_ADDR addr,
					     int *isize, char **msg);

extern void default_remote_breakpoint_from_pc (struct gdbarch *,
					       CORE_ADDR *pcptr, int *kindptr);

extern void default_gen_return_address (struct gdbarch *gdbarch,
					struct agent_expr *ax,
					struct axs_value *value,
					CORE_ADDR scope);

extern const char *default_auto_charset (void);
extern const char *default_auto_wide_charset (void);

extern int default_return_in_first_hidden_param_p (struct gdbarch *,
						   struct type *);
#endif
