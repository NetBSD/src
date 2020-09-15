/* RISC-V ELF specific backend routines.
   Copyright (C) 2011-2020 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (andrew@sifive.com).
   Based on MIPS target.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "elf/common.h"
#include "elf/internal.h"
#include "opcode/riscv.h"

extern reloc_howto_type *
riscv_reloc_name_lookup (bfd *, const char *);

extern reloc_howto_type *
riscv_reloc_type_lookup (bfd *, bfd_reloc_code_real_type);

extern reloc_howto_type *
riscv_elf_rtype_to_howto (bfd *, unsigned int r_type);

#define RISCV_DONT_CARE_VERSION -1

/* The information of architecture attribute.  */
struct riscv_subset_t
{
  const char *name;
  int major_version;
  int minor_version;
  struct riscv_subset_t *next;
};

typedef struct riscv_subset_t riscv_subset_t;

typedef struct {
  riscv_subset_t *head;
  riscv_subset_t *tail;
} riscv_subset_list_t;

extern void
riscv_release_subset_list (riscv_subset_list_t *);

extern void
riscv_add_subset (riscv_subset_list_t *,
		  const char *,
		  int, int);

extern riscv_subset_t *
riscv_lookup_subset (const riscv_subset_list_t *,
		     const char *);

extern riscv_subset_t *
riscv_lookup_subset_version (const riscv_subset_list_t *,
			     const char *,
			     int, int);

typedef struct {
  riscv_subset_list_t *subset_list;
  void (*error_handler) (const char *,
			 ...) ATTRIBUTE_PRINTF_1;
  unsigned *xlen;
  void (*get_default_version) (const char *,
                              unsigned int *,
                              unsigned int *);
} riscv_parse_subset_t;

extern bfd_boolean
riscv_parse_subset (riscv_parse_subset_t *,
		    const char *);

extern const char *
riscv_supported_std_ext (void);

extern void
riscv_release_subset_list (riscv_subset_list_t *);

extern char *
riscv_arch_str (unsigned, const riscv_subset_list_t *);

extern size_t
riscv_estimate_digit (unsigned);

/* ISA extension name class. E.g. "zbb" corresponds to RV_ISA_CLASS_Z,
   "xargs" corresponds to RV_ISA_CLASS_X, etc.  Order is important
   here.  */

typedef enum riscv_isa_ext_class
  {
   RV_ISA_CLASS_S,
   RV_ISA_CLASS_Z,
   RV_ISA_CLASS_X,
   RV_ISA_CLASS_UNKNOWN
  } riscv_isa_ext_class_t;

/* Classify the argument 'ext' into one of riscv_isa_ext_class_t.  */

riscv_isa_ext_class_t
riscv_get_prefix_class (const char *);

extern int
riscv_get_priv_spec_class (const char *, enum riscv_priv_spec_class *);

extern int
riscv_get_priv_spec_class_from_numbers (unsigned int,
					unsigned int,
					unsigned int,
					enum riscv_priv_spec_class *);

extern const char *
riscv_get_priv_spec_name (enum riscv_priv_spec_class);
