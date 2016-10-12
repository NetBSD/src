/* Target-dependent code for GNU/Linux, architecture independent.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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

#ifndef LINUX_TDEP_H
#define LINUX_TDEP_H

#include "bfd.h"

struct regcache;

/* Enum used to define the extra fields of the siginfo type used by an
   architecture.  */
enum linux_siginfo_extra_field_values
{
  /* Add bound fields into the segmentation fault field.  */
  LINUX_SIGINFO_FIELD_ADDR_BND = 1
};

/* Defines a type for the values defined in linux_siginfo_extra_field_values.  */
DEF_ENUM_FLAGS_TYPE (enum linux_siginfo_extra_field_values,
		     linux_siginfo_extra_fields);

/* This function is suitable for architectures that
   extend/override the standard siginfo in a specific way.  */
struct type *linux_get_siginfo_type_with_fields (struct gdbarch *gdbarch,
						 linux_siginfo_extra_fields);

typedef char *(*linux_collect_thread_registers_ftype) (const struct regcache *,
						       ptid_t,
						       bfd *, char *, int *,
						       enum gdb_signal);

extern enum gdb_signal linux_gdb_signal_from_target (struct gdbarch *gdbarch,
						     int signal);

extern int linux_gdb_signal_to_target (struct gdbarch *gdbarch,
				       enum gdb_signal signal);

/* Default GNU/Linux implementation of `displaced_step_location', as
   defined in gdbarch.h.  Determines the entry point from AT_ENTRY in
   the target auxiliary vector.  */
extern CORE_ADDR linux_displaced_step_location (struct gdbarch *gdbarch);

extern void linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch);

extern int linux_is_uclinux (void);

#endif /* linux-tdep.h */
