/* RISC-V-specific support for ELF.
   Copyright (C) 2011-2020 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (andrew@sifive.com).
   Based on TILE-Gx and MIPS targets.

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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/riscv.h"
#include "opcode/riscv.h"
#include "libiberty.h"
#include "elfxx-riscv.h"
#include "safe-ctype.h"

#define MINUS_ONE ((bfd_vma)0 - 1)

/* Special handler for ADD/SUB relocations that allows them to be filled out
   both in the pre-linked and post-linked file.  This is necessary to make
   pre-linked debug info work, as due to linker relaxations we need to emit
   relocations for the debug info.  */
static bfd_reloc_status_type riscv_elf_add_sub_reloc
  (bfd *, arelent *, asymbol *, void *, asection *, bfd *, char **);

/* The relocation table used for SHT_RELA sections.  */

static reloc_howto_type howto_table[] =
{
  /* No relocation.  */
  HOWTO (R_RISCV_NONE,			/* type */
	 0,				/* rightshift */
	 3,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_NONE",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32 bit relocation.  */
  HOWTO (R_RISCV_32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_32",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64 bit relocation.  */
  HOWTO (R_RISCV_64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_64",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Relocation against a local symbol in a shared object.  */
  HOWTO (R_RISCV_RELATIVE,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RELATIVE",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_COPY,			/* type */
	 0,				/* rightshift */
	 0,				/* this one is variable size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_COPY",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_JUMP_SLOT,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_JUMP_SLOT",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Dynamic TLS relocations.  */
  HOWTO (R_RISCV_TLS_DTPMOD32,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_DTPMOD32",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPMOD64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_DTPMOD64",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL32,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_DTPREL32",	/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_DTPREL64",	/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL32,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_TPREL32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_TPREL64",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Reserved for future relocs that the dynamic linker must understand.  */
  EMPTY_HOWTO (12),
  EMPTY_HOWTO (13),
  EMPTY_HOWTO (14),
  EMPTY_HOWTO (15),

  /* 12-bit PC-relative branch offset.  */
  HOWTO (R_RISCV_BRANCH,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_BRANCH",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_SBTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 20-bit PC-relative jump offset.  */
  HOWTO (R_RISCV_JAL,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_JAL",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UJTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 32-bit PC-relative function call (AUIPC/JALR).  */
  HOWTO (R_RISCV_CALL,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 64,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_CALL",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U) | ((bfd_vma) ENCODE_ITYPE_IMM (-1U) << 32),
					/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* Like R_RISCV_CALL, but not locally binding.  */
  HOWTO (R_RISCV_CALL_PLT,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 64,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_CALL_PLT",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U) | ((bfd_vma) ENCODE_ITYPE_IMM (-1U) << 32),
					/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative GOT access.  */
  HOWTO (R_RISCV_GOT_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_GOT_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS IE GOT access.  */
  HOWTO (R_RISCV_TLS_GOT_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_GOT_HI20",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS GD GOT reference.  */
  HOWTO (R_RISCV_TLS_GD_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_GD_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative reference.  */
  HOWTO (R_RISCV_PCREL_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative load or add.  */
  HOWTO (R_RISCV_PCREL_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_LO12_I",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative store.  */
  HOWTO (R_RISCV_PCREL_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_LO12_S",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit absolute address.  */
  HOWTO (R_RISCV_HI20,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 12 bits of 32-bit load or add.  */
  HOWTO (R_RISCV_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_LO12_I",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 12 bits of 32-bit store.  */
  HOWTO (R_RISCV_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_LO12_S",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_HI20",		/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for loads and adds.  */
  HOWTO (R_RISCV_TPREL_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_LO12_I",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for stores.  */
  HOWTO (R_RISCV_TPREL_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_LO12_S",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* TLS LE thread pointer usage.  May be relaxed.  */
  HOWTO (R_RISCV_TPREL_ADD,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_ADD",		/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 8-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD8,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_ADD8",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 16-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD16,			/* type */
	 0,				/* rightshift */
	 1,				/* size */
	 16,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_ADD16",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_ADD32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_ADD64",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 8-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB8,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_SUB8",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 16-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB16,			/* type */
	 0,				/* rightshift */
	 1,				/* size */
	 16,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_SUB16",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_SUB32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_SUB64",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy */
  HOWTO (R_RISCV_GNU_VTINHERIT,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 NULL,				/* special_function */
	 "R_RISCV_GNU_VTINHERIT",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_RISCV_GNU_VTENTRY,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_RISCV_GNU_VTENTRY",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Indicates an alignment statement.  The addend field encodes how many
     bytes of NOPs follow the statement.  The desired alignment is the
     addend rounded up to the next power of two.  */
  HOWTO (R_RISCV_ALIGN,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ALIGN",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 8-bit PC-relative branch offset.  */
  HOWTO (R_RISCV_RVC_BRANCH,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RVC_BRANCH",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_RVC_B_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 11-bit PC-relative jump offset.  */
  HOWTO (R_RISCV_RVC_JUMP,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RVC_JUMP",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_RVC_J_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* High 6 bits of 18-bit absolute address.  */
  HOWTO (R_RISCV_RVC_LUI,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RVC_LUI",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_RVC_IMM (-1U),		/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GP-relative load.  */
  HOWTO (R_RISCV_GPREL_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_GPREL_I",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GP-relative store.  */
  HOWTO (R_RISCV_GPREL_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_GPREL_S",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* TP-relative TLS LE load.  */
  HOWTO (R_RISCV_TPREL_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_I",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* TP-relative TLS LE store.  */
  HOWTO (R_RISCV_TPREL_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_S",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* The paired relocation may be relaxed.  */
  HOWTO (R_RISCV_RELAX,			/* type */
	 0,				/* rightshift */
	 3,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RELAX",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 6-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB6,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 riscv_elf_add_sub_reloc,	/* special_function */
	 "R_RISCV_SUB6",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0x3f,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 6-bit in-place setting, for local label subtraction.  */
  HOWTO (R_RISCV_SET6,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SET6",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0x3f,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 8-bit in-place setting, for local label subtraction.  */
  HOWTO (R_RISCV_SET8,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SET8",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 16-bit in-place setting, for local label subtraction.  */
  HOWTO (R_RISCV_SET16,			/* type */
	 0,				/* rightshift */
	 1,				/* size */
	 16,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SET16",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit in-place setting, for local label subtraction.  */
  HOWTO (R_RISCV_SET32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SET32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit PC relative.  */
  HOWTO (R_RISCV_32_PCREL,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_32_PCREL",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */
};

/* A mapping from BFD reloc types to RISC-V ELF reloc types.  */

struct elf_reloc_map
{
  bfd_reloc_code_real_type bfd_val;
  enum elf_riscv_reloc_type elf_val;
};

static const struct elf_reloc_map riscv_reloc_map[] =
{
  { BFD_RELOC_NONE, R_RISCV_NONE },
  { BFD_RELOC_32, R_RISCV_32 },
  { BFD_RELOC_64, R_RISCV_64 },
  { BFD_RELOC_RISCV_ADD8, R_RISCV_ADD8 },
  { BFD_RELOC_RISCV_ADD16, R_RISCV_ADD16 },
  { BFD_RELOC_RISCV_ADD32, R_RISCV_ADD32 },
  { BFD_RELOC_RISCV_ADD64, R_RISCV_ADD64 },
  { BFD_RELOC_RISCV_SUB8, R_RISCV_SUB8 },
  { BFD_RELOC_RISCV_SUB16, R_RISCV_SUB16 },
  { BFD_RELOC_RISCV_SUB32, R_RISCV_SUB32 },
  { BFD_RELOC_RISCV_SUB64, R_RISCV_SUB64 },
  { BFD_RELOC_CTOR, R_RISCV_64 },
  { BFD_RELOC_12_PCREL, R_RISCV_BRANCH },
  { BFD_RELOC_RISCV_HI20, R_RISCV_HI20 },
  { BFD_RELOC_RISCV_LO12_I, R_RISCV_LO12_I },
  { BFD_RELOC_RISCV_LO12_S, R_RISCV_LO12_S },
  { BFD_RELOC_RISCV_PCREL_LO12_I, R_RISCV_PCREL_LO12_I },
  { BFD_RELOC_RISCV_PCREL_LO12_S, R_RISCV_PCREL_LO12_S },
  { BFD_RELOC_RISCV_CALL, R_RISCV_CALL },
  { BFD_RELOC_RISCV_CALL_PLT, R_RISCV_CALL_PLT },
  { BFD_RELOC_RISCV_PCREL_HI20, R_RISCV_PCREL_HI20 },
  { BFD_RELOC_RISCV_JMP, R_RISCV_JAL },
  { BFD_RELOC_RISCV_GOT_HI20, R_RISCV_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_DTPMOD32, R_RISCV_TLS_DTPMOD32 },
  { BFD_RELOC_RISCV_TLS_DTPREL32, R_RISCV_TLS_DTPREL32 },
  { BFD_RELOC_RISCV_TLS_DTPMOD64, R_RISCV_TLS_DTPMOD64 },
  { BFD_RELOC_RISCV_TLS_DTPREL64, R_RISCV_TLS_DTPREL64 },
  { BFD_RELOC_RISCV_TLS_TPREL32, R_RISCV_TLS_TPREL32 },
  { BFD_RELOC_RISCV_TLS_TPREL64, R_RISCV_TLS_TPREL64 },
  { BFD_RELOC_RISCV_TPREL_HI20, R_RISCV_TPREL_HI20 },
  { BFD_RELOC_RISCV_TPREL_ADD, R_RISCV_TPREL_ADD },
  { BFD_RELOC_RISCV_TPREL_LO12_S, R_RISCV_TPREL_LO12_S },
  { BFD_RELOC_RISCV_TPREL_LO12_I, R_RISCV_TPREL_LO12_I },
  { BFD_RELOC_RISCV_TLS_GOT_HI20, R_RISCV_TLS_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_GD_HI20, R_RISCV_TLS_GD_HI20 },
  { BFD_RELOC_RISCV_ALIGN, R_RISCV_ALIGN },
  { BFD_RELOC_RISCV_RVC_BRANCH, R_RISCV_RVC_BRANCH },
  { BFD_RELOC_RISCV_RVC_JUMP, R_RISCV_RVC_JUMP },
  { BFD_RELOC_RISCV_RVC_LUI, R_RISCV_RVC_LUI },
  { BFD_RELOC_RISCV_GPREL_I, R_RISCV_GPREL_I },
  { BFD_RELOC_RISCV_GPREL_S, R_RISCV_GPREL_S },
  { BFD_RELOC_RISCV_TPREL_I, R_RISCV_TPREL_I },
  { BFD_RELOC_RISCV_TPREL_S, R_RISCV_TPREL_S },
  { BFD_RELOC_RISCV_RELAX, R_RISCV_RELAX },
  { BFD_RELOC_RISCV_SUB6, R_RISCV_SUB6 },
  { BFD_RELOC_RISCV_SET6, R_RISCV_SET6 },
  { BFD_RELOC_RISCV_SET8, R_RISCV_SET8 },
  { BFD_RELOC_RISCV_SET16, R_RISCV_SET16 },
  { BFD_RELOC_RISCV_SET32, R_RISCV_SET32 },
  { BFD_RELOC_RISCV_32_PCREL, R_RISCV_32_PCREL },
};

/* Given a BFD reloc type, return a howto structure.  */

reloc_howto_type *
riscv_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			 bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (riscv_reloc_map); i++)
    if (riscv_reloc_map[i].bfd_val == code)
      return &howto_table[(int) riscv_reloc_map[i].elf_val];

  bfd_set_error (bfd_error_bad_value);
  return NULL;
}

reloc_howto_type *
riscv_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (howto_table); i++)
    if (howto_table[i].name && strcasecmp (howto_table[i].name, r_name) == 0)
      return &howto_table[i];

  return NULL;
}

reloc_howto_type *
riscv_elf_rtype_to_howto (bfd *abfd, unsigned int r_type)
{
  if (r_type >= ARRAY_SIZE (howto_table))
    {
      (*_bfd_error_handler) (_("%pB: unsupported relocation type %#x"),
			     abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  return &howto_table[r_type];
}

/* Special_function of RISCV_ADD and RISCV_SUB relocations.  */

static bfd_reloc_status_type
riscv_elf_add_sub_reloc (bfd *abfd,
			 arelent *reloc_entry,
			 asymbol *symbol,
			 void *data,
			 asection *input_section,
			 bfd *output_bfd,
			 char **error_message ATTRIBUTE_UNUSED)
{
  reloc_howto_type *howto = reloc_entry->howto;
  bfd_vma relocation;

  if (output_bfd != NULL
      && (symbol->flags & BSF_SECTION_SYM) == 0
      && (!reloc_entry->howto->partial_inplace || reloc_entry->addend == 0))
    {
      reloc_entry->address += input_section->output_offset;
      return bfd_reloc_ok;
    }

  if (output_bfd != NULL)
    return bfd_reloc_continue;

  relocation = symbol->value + symbol->section->output_section->vma
    + symbol->section->output_offset + reloc_entry->addend;
  bfd_vma old_value = bfd_get (howto->bitsize, abfd,
			       data + reloc_entry->address);

  switch (howto->type)
    {
    case R_RISCV_ADD8:
    case R_RISCV_ADD16:
    case R_RISCV_ADD32:
    case R_RISCV_ADD64:
      relocation = old_value + relocation;
      break;
    case R_RISCV_SUB6:
    case R_RISCV_SUB8:
    case R_RISCV_SUB16:
    case R_RISCV_SUB32:
    case R_RISCV_SUB64:
      relocation = old_value - relocation;
      break;
    }
  bfd_put (howto->bitsize, abfd, relocation, data + reloc_entry->address);

  return bfd_reloc_ok;
}

/* Parsing subset version.

   Return Value:
     Points to the end of version

   Arguments:
     `rps`: Hooks and status for parsing subset.
     `march`: Full arch string.
     `p`: Curent parsing position.
     `major_version`: Parsing result of major version, using
      default_major_version if version is not present in arch string.
     `minor_version`: Parsing result of minor version, set to 0 if version is
     not present in arch string, but set to `default_minor_version` if
     `major_version` using default_major_version.
     `default_major_version`: Default major version.
     `default_minor_version`: Default minor version.
     `std_ext_p`: True if parsing std extension.  */

static const char *
riscv_parsing_subset_version (riscv_parse_subset_t *rps,
			      const char *march,
			      const char *p,
			      unsigned *major_version,
			      unsigned *minor_version,
			      unsigned default_major_version,
			      unsigned default_minor_version,
			      bfd_boolean std_ext_p)
{
  bfd_boolean major_p = TRUE;
  unsigned version = 0;
  unsigned major = 0;
  unsigned minor = 0;
  char np;

  for (;*p; ++p)
    {
      if (*p == 'p')
	{
	  np = *(p + 1);

	  if (!ISDIGIT (np))
	    {
	      /* Might be beginning of `p` extension.  */
	      if (std_ext_p)
		{
		  *major_version = version;
		  *minor_version = 0;
		  return p;
		}
	      else
		{
		  rps->error_handler ("-march=%s: Expect number after `%dp'.",
				      march, version);
		  return NULL;
		}
	    }

	  major = version;
	  major_p = FALSE;
	  version = 0;
	}
      else if (ISDIGIT (*p))
	version = (version * 10) + (*p - '0');
      else
	break;
    }

  if (major_p)
    major = version;
  else
    minor = version;

  if (major == 0 && minor == 0)
    {
      /* We don't found any version string, use default version.  */
      *major_version = default_major_version;
      *minor_version = default_minor_version;
    }
  else
    {
      *major_version = major;
      *minor_version = minor;
    }
  return p;
}

/* Return string which contain all supported standard extensions in
   canonical order.  */

const char *
riscv_supported_std_ext (void)
{
  return "mafdqlcbjtpvn";
}

/* Parsing function for standard extensions.

   Return Value:
     Points to the end of extensions.

   Arguments:
     `rps`: Hooks and status for parsing subset.
     `march`: Full arch string.
     `p`: Curent parsing position.  */

static const char *
riscv_parse_std_ext (riscv_parse_subset_t *rps,
		     const char *march, const char *p)
{
  const char *all_std_exts = riscv_supported_std_ext ();
  const char *std_exts = all_std_exts;

  unsigned major_version = 0;
  unsigned minor_version = 0;
  char std_ext = '\0';

  /* First letter must start with i, e or g.  */
  switch (*p)
    {
      case 'i':
	p++;
	p = riscv_parsing_subset_version (
	      rps,
	      march,
	      p, &major_version, &minor_version,
	      /* default_major_version= */ 2,
	      /* default_minor_version= */ 0,
	      /* std_ext_p= */TRUE);
	riscv_add_subset (rps->subset_list, "i", major_version, minor_version);
	break;

      case 'e':
	p++;
	p = riscv_parsing_subset_version (
	      rps,
	      march,
	      p, &major_version, &minor_version,
	      /* default_major_version= */ 1,
	      /* default_minor_version= */ 9,
	      /* std_ext_p= */TRUE);

	riscv_add_subset (rps->subset_list, "e", major_version, minor_version);
	riscv_add_subset (rps->subset_list, "i", 2, 0);

	if (*rps->xlen > 32)
	  {
	    rps->error_handler ("-march=%s: rv%de is not a valid base ISA",
				march, *rps->xlen);
	    return NULL;
	  }

	break;

      case 'g':
	p++;
	p = riscv_parsing_subset_version (
	      rps,
	      march,
	      p, &major_version, &minor_version,
	      /* default_major_version= */ 2,
	      /* default_minor_version= */ 0,
	      /* std_ext_p= */TRUE);
	riscv_add_subset (rps->subset_list, "i", major_version, minor_version);

	for ( ; *std_exts != 'q'; std_exts++)
	  {
	    const char subset[] = {*std_exts, '\0'};
	    riscv_add_subset (
	      rps->subset_list, subset, major_version, minor_version);
	  }
	break;

      default:
	rps->error_handler (
	  "-march=%s: first ISA subset must be `e', `i' or `g'", march);
	return NULL;
    }

  while (*p)
    {
      char subset[2] = {0, 0};

      if (*p == 'x' || *p == 's')
	break;

      if (*p == '_')
	{
	  p++;
	  continue;
	}

      std_ext = *p;

      /* Checking canonical order.  */
      while (*std_exts && std_ext != *std_exts) std_exts++;

      if (std_ext != *std_exts)
	{
	  if (strchr (all_std_exts, std_ext) == NULL)
	    rps->error_handler (
	      "-march=%s: unsupported ISA subset `%c'", march, *p);
	  else
	    rps->error_handler (
	      "-march=%s: ISA string is not in canonical order. `%c'",
	      march, *p);
	  return NULL;
	}

      std_exts++;

      p++;
      p = riscv_parsing_subset_version (
	    rps,
	    march,
	    p, &major_version, &minor_version,
	    /* default_major_version= */ 2,
	    /* default_minor_version= */ 0,
	    /* std_ext_p= */TRUE);

      subset[0] = std_ext;

      riscv_add_subset (rps->subset_list, subset, major_version, minor_version);
    }
  return p;
}

/* Parsing function for non-standard and supervisor extensions.

   Return Value:
     Points to the end of extensions.

   Arguments:
     `rps`: Hooks and status for parsing subset.
     `march`: Full arch string.
     `p`: Curent parsing position.
     `ext_type`: What kind of extensions, 'x', 's' or 'sx'.
     `ext_type_str`: Full name for kind of extension.  */

static const char *
riscv_parse_sv_or_non_std_ext (riscv_parse_subset_t *rps,
			       const char *march,
			       const char *p,
			       const char *ext_type,
			       const char *ext_type_str)
{
  unsigned major_version = 0;
  unsigned minor_version = 0;
  size_t ext_type_len = strlen (ext_type);

  while (*p)
    {
      if (*p == '_')
	{
	  p++;
	  continue;
	}

      if (strncmp (p, ext_type, ext_type_len) != 0)
	break;

      /* It's non-standard supervisor extension if it prefix with sx.  */
      if ((ext_type[0] == 's') && (ext_type_len == 1)
	  && (*(p + 1) == 'x'))
	break;

      char *subset = xstrdup (p);
      char *q = subset;
      const char *end_of_version;

      while (*++q != '\0' && *q != '_' && !ISDIGIT (*q))
	;

      end_of_version =
	riscv_parsing_subset_version (
	  rps,
	  march,
	  q, &major_version, &minor_version,
	  /* default_major_version= */ 2,
	  /* default_minor_version= */ 0,
	  /* std_ext_p= */FALSE);

      *q = '\0';

      riscv_add_subset (rps->subset_list, subset, major_version, minor_version);
      free (subset);
      p += end_of_version - subset;

      if (*p != '\0' && *p != '_')
	{
	  rps->error_handler ("-march=%s: %s must separate with _",
			      march, ext_type_str);
	  return NULL;
	}
    }

  return p;
}

/* Function for parsing arch string.

   Return Value:
     Return TRUE on success.

   Arguments:
     `rps`: Hooks and status for parsing subset.
     `arch`: Arch string.  */

bfd_boolean
riscv_parse_subset (riscv_parse_subset_t *rps,
		    const char *arch)
{
  const char *p = arch;

  if (strncmp (p, "rv32", 4) == 0)
    {
      *rps->xlen = 32;
      p += 4;
    }
  else if (strncmp (p, "rv64", 4) == 0)
    {
      *rps->xlen = 64;
      p += 4;
    }
  else
    {
      rps->error_handler ("-march=%s: ISA string must begin with rv32 or rv64",
			  arch);
      return FALSE;
    }

  /* Parsing standard extension.  */
  p = riscv_parse_std_ext (rps, arch, p);

  if (p == NULL)
    return FALSE;

  /* Parsing non-standard extension.  */
  p = riscv_parse_sv_or_non_std_ext (
	rps, arch, p, "x", "non-standard extension");

  if (p == NULL)
    return FALSE;

  /* Parsing supervisor extension.  */
  p = riscv_parse_sv_or_non_std_ext (
	rps, arch, p, "s", "supervisor extension");

  if (p == NULL)
    return FALSE;

  /* Parsing non-standard supervisor extension.  */
  p = riscv_parse_sv_or_non_std_ext (
	rps, arch, p, "sx", "non-standard supervisor extension");

  if (p == NULL)
    return FALSE;

  if (*p != '\0')
    {
      rps->error_handler ("-march=%s: unexpected ISA string at end: %s",
			  arch, p);
      return FALSE;
    }

  if (riscv_lookup_subset (rps->subset_list, "e")
      && riscv_lookup_subset (rps->subset_list, "f"))
    {
      rps->error_handler ("-march=%s: rv32e does not support the `f' extension",
			  arch);
      return FALSE;
    }

  if (riscv_lookup_subset (rps->subset_list, "d")
      && !riscv_lookup_subset (rps->subset_list, "f"))
    {
      rps->error_handler ("-march=%s: `d' extension requires `f' extension",
			  arch);
      return FALSE;
    }

  if (riscv_lookup_subset (rps->subset_list, "q")
      && !riscv_lookup_subset (rps->subset_list, "d"))
    {
      rps->error_handler ("-march=%s: `q' extension requires `d' extension",
			  arch);
      return FALSE;
    }

  if (riscv_lookup_subset (rps->subset_list, "q") && *rps->xlen < 64)
    {
      rps->error_handler ("-march=%s: rv32 does not support the `q' extension",
			  arch);
      return FALSE;
    }
  return TRUE;
}

/* Add new subset to list.  */

void
riscv_add_subset (riscv_subset_list_t *subset_list,
		  const char *subset,
		  int major, int minor)
{
  riscv_subset_t *s = xmalloc (sizeof *s);

  if (subset_list->head == NULL)
    subset_list->head = s;

  s->name = xstrdup (subset);
  s->major_version = major;
  s->minor_version = minor;
  s->next = NULL;

  if (subset_list->tail != NULL)
    subset_list->tail->next = s;

  subset_list->tail = s;
}

/* Find subset in list without version checking, return NULL if not found.  */

riscv_subset_t *
riscv_lookup_subset (const riscv_subset_list_t *subset_list,
		     const char *subset)
{
  return riscv_lookup_subset_version (
	   subset_list, subset,
	   RISCV_DONT_CARE_VERSION,
	   RISCV_DONT_CARE_VERSION);
}

/* Find subset in list with version checking, return NULL if not found.  */

riscv_subset_t *
riscv_lookup_subset_version (const riscv_subset_list_t *subset_list,
			     const char *subset,
			     int major, int minor)
{
  riscv_subset_t *s;

  for (s = subset_list->head; s != NULL; s = s->next)
    if (strcasecmp (s->name, subset) == 0)
      {
	if ((major != RISCV_DONT_CARE_VERSION)
	    && (s->major_version != major))
	  return NULL;

	if ((minor != RISCV_DONT_CARE_VERSION)
	    && (s->minor_version != minor))
	  return NULL;

	return s;
      }

  return NULL;
}

/* Release subset list.  */

void
riscv_release_subset_list (riscv_subset_list_t *subset_list)
{
   while (subset_list->head != NULL)
    {
      riscv_subset_t *next = subset_list->head->next;
      free ((void *)subset_list->head->name);
      free (subset_list->head);
      subset_list->head = next;
    }

  subset_list->tail = NULL;
}

/* Return the number of digits for the input.  */

static size_t
riscv_estimate_digit (unsigned num)
{
  size_t digit = 0;
  if (num == 0)
    return 1;

  for (digit = 0; num ; num /= 10)
    digit++;

  return digit;
}

/* Auxiliary function to estimate string length of subset list.  */

static size_t
riscv_estimate_arch_strlen1 (const riscv_subset_t *subset)
{
  if (subset == NULL)
    return 6; /* For rv32/rv64/rv128 and string terminator.  */

  return riscv_estimate_arch_strlen1 (subset->next)
	 + strlen (subset->name)
	 + riscv_estimate_digit (subset->major_version)
	 + 1 /* For version seperator: 'p'.  */
	 + riscv_estimate_digit (subset->minor_version)
	 + 1 /* For underscore.  */;
}

/* Estimate the string length of this subset list.  */

static size_t
riscv_estimate_arch_strlen (const riscv_subset_list_t *subset_list)
{
  return riscv_estimate_arch_strlen1 (subset_list->head);
}

/* Auxiliary function to convert subset info to string.  */

static void
riscv_arch_str1 (riscv_subset_t *subset,
		 char *attr_str, char *buf, size_t bufsz)
{
  const char *underline = "_";

  if (subset == NULL)
    return;

  /* No underline between rvXX and i/e.   */
  if ((strcasecmp (subset->name, "i") == 0)
      || (strcasecmp (subset->name, "e") == 0))
    underline = "";

  snprintf (buf, bufsz, "%s%s%dp%d",
	    underline,
	    subset->name,
	    subset->major_version,
	    subset->minor_version);

  strncat (attr_str, buf, bufsz);

  /* Skip 'i' extension after 'e'.  */
  if ((strcasecmp (subset->name, "e") == 0)
      && subset->next
      && (strcasecmp (subset->next->name, "i") == 0))
    riscv_arch_str1 (subset->next->next, attr_str, buf, bufsz);
  else
    riscv_arch_str1 (subset->next, attr_str, buf, bufsz);
}

/* Convert subset info to string with explicit version info.  */

char *
riscv_arch_str (unsigned xlen, const riscv_subset_list_t *subset)
{
  size_t arch_str_len = riscv_estimate_arch_strlen (subset);
  char *attr_str = xmalloc (arch_str_len);
  char *buf = xmalloc (arch_str_len);

  snprintf (attr_str, arch_str_len, "rv%u", xlen);

  riscv_arch_str1 (subset->head, attr_str, buf, arch_str_len);
  free (buf);

  return attr_str;
}
