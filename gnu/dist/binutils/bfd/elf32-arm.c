/* 32-bit ELF support for ARM
   Copyright 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "elf/arm.h"
#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "elf-bfd.h"

#ifndef NUM_ELEM
#define NUM_ELEM(a)  (sizeof (a) / (sizeof (a)[0]))
#endif

#define elf_info_to_howto               0
#define elf_info_to_howto_rel           elf32_arm_info_to_howto

#define ARM_ELF_ABI_VERSION		0
#define ARM_ELF_OS_ABI_VERSION		ELFOSABI_ARM

static reloc_howto_type * elf32_arm_reloc_type_lookup
  PARAMS ((bfd * abfd, bfd_reloc_code_real_type code));
static bfd_boolean elf32_arm_nabi_grok_prstatus
  PARAMS ((bfd *abfd, Elf_Internal_Note *note));
static bfd_boolean elf32_arm_nabi_grok_psinfo
  PARAMS ((bfd *abfd, Elf_Internal_Note *note));

/* Note: code such as elf32_arm_reloc_type_lookup expect to use e.g.
   R_ARM_PC24 as an index into this, and find the R_ARM_PC24 HOWTO
   in that slot.  */

static reloc_howto_type elf32_arm_howto_table[] =
{
  /* No relocation */
  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_PC24,		/* type */
	 2,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_PC24",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00ffffff,		/* src_mask */
	 0x00ffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 32 bit absolute */
  HOWTO (R_ARM_ABS32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ABS32",		/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* standard 32bit pc-relative reloc */
  HOWTO (R_ARM_REL32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_REL32",		/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 8 bit absolute */
  HOWTO (R_ARM_PC13,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_PC13",		/* name */
	 FALSE,			/* partial_inplace */
	 0x000000ff,		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

   /* 16 bit absolute */
  HOWTO (R_ARM_ABS16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ABS16",		/* name */
	 FALSE,			/* partial_inplace */
	 0x0000ffff,		/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 12 bit absolute */
  HOWTO (R_ARM_ABS12,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ABS12",		/* name */
	 FALSE,			/* partial_inplace */
	 0x000008ff,		/* src_mask */
	 0x000008ff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_THM_ABS5,	/* type */
	 6,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 5,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_ABS5",	/* name */
	 FALSE,			/* partial_inplace */
	 0x000007e0,		/* src_mask */
	 0x000007e0,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 8 bit absolute */
  HOWTO (R_ARM_ABS8,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ABS8",		/* name */
	 FALSE,			/* partial_inplace */
	 0x000000ff,		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_SBREL32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_SBREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_THM_PC22,	/* type */
	 1,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 23,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_PC22",	/* name */
	 FALSE,			/* partial_inplace */
	 0x07ff07ff,		/* src_mask */
	 0x07ff07ff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_THM_PC8,	        /* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_PC8",	/* name */
	 FALSE,			/* partial_inplace */
	 0x000000ff,		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_AMP_VCALL9,	/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_AMP_VCALL9",	/* name */
	 FALSE,			/* partial_inplace */
	 0x000000ff,		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_SWI24,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_SWI24",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x00000000,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_THM_SWI8,	/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_SWI8",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00000000,		/* src_mask */
	 0x00000000,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* BLX instruction for the ARM.  */
  HOWTO (R_ARM_XPC25,		/* type */
	 2,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 25,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_XPC25",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00ffffff,		/* src_mask */
	 0x00ffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* BLX instruction for the Thumb.  */
  HOWTO (R_ARM_THM_XPC22,	/* type */
	 2,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 22,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_XPC22",	/* name */
	 FALSE,			/* partial_inplace */
	 0x07ff07ff,		/* src_mask */
	 0x07ff07ff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* These next three relocs are not defined, but we need to fill the space.  */

  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_unknown_17",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_unknown_18",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_unknown_19",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Relocs used in ARM Linux */

  HOWTO (R_ARM_COPY,		/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_COPY",		/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),                /* pcrel_offset */

  HOWTO (R_ARM_GLOB_DAT,	/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_GLOB_DAT",	/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),                /* pcrel_offset */

  HOWTO (R_ARM_JUMP_SLOT,	/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_JUMP_SLOT",	/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),                /* pcrel_offset */

  HOWTO (R_ARM_RELATIVE,	/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_RELATIVE",	/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),                /* pcrel_offset */

  HOWTO (R_ARM_GOTOFF,		/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_GOTOFF",	/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),                /* pcrel_offset */

  HOWTO (R_ARM_GOTPC,		/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         TRUE,			/* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_GOTPC",		/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_GOT32,		/* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         32,                    /* bitsize */
         FALSE,			/* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_GOT32",		/* name */
         TRUE,			/* partial_inplace */
         0xffffffff,		/* src_mask */
         0xffffffff,		/* dst_mask */
         FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_PLT32,		/* type */
         2,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         26,                    /* bitsize */
         TRUE,			/* pc_relative */
         0,                     /* bitpos */
         complain_overflow_bitfield,/* complain_on_overflow */
         bfd_elf_generic_reloc, /* special_function */
         "R_ARM_PLT32",		/* name */
         TRUE,			/* partial_inplace */
         0x00ffffff,		/* src_mask */
         0x00ffffff,		/* dst_mask */
         TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_CALL,		/* type */
	 2,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_CALL",		/* name */
	 FALSE,			/* partial_inplace */
	 0x00ffffff,		/* src_mask */
	 0x00ffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_JUMP24,		/* type */
	 2,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 24,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_JUMP24",	/* name */
	 FALSE,			/* partial_inplace */
	 0x00ffffff,		/* src_mask */
	 0x00ffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_unknown_30",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_unknown_31",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_ALU_PCREL7_0,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ALU_PCREL_7_0",	/* name */
	 FALSE,			/* partial_inplace */
	 0x00000fff,		/* src_mask */
	 0x00000fff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_ALU_PCREL15_8,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 TRUE,			/* pc_relative */
	 8,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ALU_PCREL_15_8",/* name */
	 FALSE,			/* partial_inplace */
	 0x00000fff,		/* src_mask */
	 0x00000fff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_ALU_PCREL23_15,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 TRUE,			/* pc_relative */
	 16,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ALU_PCREL_23_15",/* name */
	 FALSE,			/* partial_inplace */
	 0x00000fff,		/* src_mask */
	 0x00000fff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_LDR_SBREL_11_0,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 12,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_LDR_SBREL_11_0",/* name */
	 FALSE,			/* partial_inplace */
	 0x00000fff,		/* src_mask */
	 0x00000fff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_ALU_SBREL_19_12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 12,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ALU_SBREL_19_12",/* name */
	 FALSE,			/* partial_inplace */
	 0x000ff000,		/* src_mask */
	 0x000ff000,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_ALU_SBREL_27_20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 20,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ALU_SBREL_27_20",/* name */
	 FALSE,			/* partial_inplace */
	 0x0ff00000,		/* src_mask */
	 0x0ff00000,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_TARGET1,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_TARGET1",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_ROSEGREL32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_ROSEGREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_V4BX,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_V4BX",		/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_TARGET2,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_TARGET2",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_ARM_PREL31,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 31,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_PREL31",	/* name */
	 FALSE,			/* partial_inplace */
	 0x7fffffff,		/* src_mask */
	 0x7fffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */
};

  /* GNU extension to record C++ vtable hierarchy */
static reloc_howto_type elf32_arm_vtinherit_howto =
  HOWTO (R_ARM_GNU_VTINHERIT, /* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         0,                     /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_dont, /* complain_on_overflow */
         NULL,                  /* special_function */
         "R_ARM_GNU_VTINHERIT", /* name */
         FALSE,                 /* partial_inplace */
         0,                     /* src_mask */
         0,                     /* dst_mask */
         FALSE);                /* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
static reloc_howto_type elf32_arm_vtentry_howto =
  HOWTO (R_ARM_GNU_VTENTRY,     /* type */
         0,                     /* rightshift */
         2,                     /* size (0 = byte, 1 = short, 2 = long) */
         0,                     /* bitsize */
         FALSE,                 /* pc_relative */
         0,                     /* bitpos */
         complain_overflow_dont, /* complain_on_overflow */
         _bfd_elf_rel_vtable_reloc_fn,  /* special_function */
         "R_ARM_GNU_VTENTRY",   /* name */
         FALSE,                 /* partial_inplace */
         0,                     /* src_mask */
         0,                     /* dst_mask */
         FALSE);                /* pcrel_offset */

  /* 12 bit pc relative */
static reloc_howto_type elf32_arm_thm_pc11_howto =
  HOWTO (R_ARM_THM_PC11,	/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 11,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_PC11",	/* name */
	 FALSE,			/* partial_inplace */
	 0x000007ff,		/* src_mask */
	 0x000007ff,		/* dst_mask */
	 TRUE);			/* pcrel_offset */

  /* 12 bit pc relative */
static reloc_howto_type elf32_arm_thm_pc9_howto =
  HOWTO (R_ARM_THM_PC9,		/* type */
	 1,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_THM_PC9",	/* name */
	 FALSE,			/* partial_inplace */
	 0x000000ff,		/* src_mask */
	 0x000000ff,		/* dst_mask */
	 TRUE);			/* pcrel_offset */

/* Place relative GOT-indirect.  */
static reloc_howto_type elf32_arm_got_prel =
  HOWTO (R_ARM_GOT_PREL,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_GOT_PREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE);			/* pcrel_offset */

/* Currently unused relocations.  */
static reloc_howto_type elf32_arm_r_howto[4] =
{
  HOWTO (R_ARM_RREL32,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_RREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_RABS32,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_RABS32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_RPC24,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_RPC24",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_ARM_RBASE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_ARM_RBASE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE)			/* pcrel_offset */
};

static reloc_howto_type *
elf32_arm_howto_from_type (unsigned int r_type)
{
  if (r_type < NUM_ELEM (elf32_arm_howto_table))
    return &elf32_arm_howto_table[r_type];
    
  switch (r_type)
    {
    case R_ARM_GOT_PREL:
      return &elf32_arm_got_prel;

    case R_ARM_GNU_VTINHERIT:
      return &elf32_arm_vtinherit_howto;

    case R_ARM_GNU_VTENTRY:
      return &elf32_arm_vtentry_howto;

    case R_ARM_THM_PC11:
      return &elf32_arm_thm_pc11_howto;

    case R_ARM_THM_PC9:
      return &elf32_arm_thm_pc9_howto;

    case R_ARM_RREL32:
    case R_ARM_RABS32:
    case R_ARM_RPC24:
    case R_ARM_RBASE:
      return &elf32_arm_r_howto[r_type - R_ARM_RREL32];

    default:
      return NULL;
    }
}

static void
elf32_arm_info_to_howto (bfd * abfd ATTRIBUTE_UNUSED, arelent * bfd_reloc,
			 Elf_Internal_Rela * elf_reloc)
{
  unsigned int r_type;

  r_type = ELF32_R_TYPE (elf_reloc->r_info);
  bfd_reloc->howto = elf32_arm_howto_from_type (r_type);
}

struct elf32_arm_reloc_map
  {
    bfd_reloc_code_real_type  bfd_reloc_val;
    unsigned char             elf_reloc_val;
  };

/* All entries in this list must also be present in elf32_arm_howto_table.  */
static const struct elf32_arm_reloc_map elf32_arm_reloc_map[] =
  {
    {BFD_RELOC_NONE,                 R_ARM_NONE},
    {BFD_RELOC_ARM_PCREL_BRANCH,     R_ARM_PC24},
    {BFD_RELOC_ARM_PCREL_BLX,        R_ARM_XPC25},
    {BFD_RELOC_THUMB_PCREL_BLX,      R_ARM_THM_XPC22},
    {BFD_RELOC_32,                   R_ARM_ABS32},
    {BFD_RELOC_32_PCREL,             R_ARM_REL32},
    {BFD_RELOC_8,                    R_ARM_ABS8},
    {BFD_RELOC_16,                   R_ARM_ABS16},
    {BFD_RELOC_ARM_OFFSET_IMM,       R_ARM_ABS12},
    {BFD_RELOC_ARM_THUMB_OFFSET,     R_ARM_THM_ABS5},
    {BFD_RELOC_THUMB_PCREL_BRANCH23, R_ARM_THM_PC22},
    {BFD_RELOC_ARM_COPY,             R_ARM_COPY},
    {BFD_RELOC_ARM_GLOB_DAT,         R_ARM_GLOB_DAT},
    {BFD_RELOC_ARM_JUMP_SLOT,        R_ARM_JUMP_SLOT},
    {BFD_RELOC_ARM_RELATIVE,         R_ARM_RELATIVE},
    {BFD_RELOC_ARM_GOTOFF,           R_ARM_GOTOFF},
    {BFD_RELOC_ARM_GOTPC,            R_ARM_GOTPC},
    {BFD_RELOC_ARM_GOT32,            R_ARM_GOT32},
    {BFD_RELOC_ARM_PLT32,            R_ARM_PLT32},
    {BFD_RELOC_ARM_TARGET1,	     R_ARM_TARGET1},
    {BFD_RELOC_ARM_ROSEGREL32,	     R_ARM_ROSEGREL32},
    {BFD_RELOC_ARM_SBREL32,	     R_ARM_SBREL32},
    {BFD_RELOC_ARM_PREL31,	     R_ARM_PREL31},
    {BFD_RELOC_ARM_TARGET2,	     R_ARM_TARGET2}
  };

static reloc_howto_type *
elf32_arm_reloc_type_lookup (abfd, code)
     bfd *abfd ATTRIBUTE_UNUSED;
     bfd_reloc_code_real_type code;
{
  unsigned int i;

  switch (code)
    {
    case BFD_RELOC_VTABLE_INHERIT:
      return & elf32_arm_vtinherit_howto;

    case BFD_RELOC_VTABLE_ENTRY:
      return & elf32_arm_vtentry_howto;

    case BFD_RELOC_THUMB_PCREL_BRANCH12:
      return & elf32_arm_thm_pc11_howto;

    case BFD_RELOC_THUMB_PCREL_BRANCH9:
      return & elf32_arm_thm_pc9_howto;

    default:
      for (i = 0; i < NUM_ELEM (elf32_arm_reloc_map); i ++)
	if (elf32_arm_reloc_map[i].bfd_reloc_val == code)
	  return & elf32_arm_howto_table[elf32_arm_reloc_map[i].elf_reloc_val];

      return NULL;
   }
}

/* Support for core dump NOTE sections */
static bfd_boolean
elf32_arm_nabi_grok_prstatus (abfd, note)
     bfd *abfd;
     Elf_Internal_Note *note;
{
  int offset;
  size_t size;

  switch (note->descsz)
    {
      default:
	return FALSE;

      case 148:		/* Linux/ARM 32-bit*/
	/* pr_cursig */
	elf_tdata (abfd)->core_signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core_pid = bfd_get_32 (abfd, note->descdata + 24);

	/* pr_reg */
	offset = 72;
	size = 72;

	break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  size, note->descpos + offset);
}

static bfd_boolean
elf32_arm_nabi_grok_psinfo (abfd, note)
     bfd *abfd;
     Elf_Internal_Note *note;
{
  switch (note->descsz)
    {
      default:
	return FALSE;

      case 124:		/* Linux/ARM elf_prpsinfo */
	elf_tdata (abfd)->core_program
	 = _bfd_elfcore_strndup (abfd, note->descdata + 28, 16);
	elf_tdata (abfd)->core_command
	 = _bfd_elfcore_strndup (abfd, note->descdata + 44, 80);
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core_command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return TRUE;
}

#define TARGET_LITTLE_SYM               bfd_elf32_littlearm_vec
#define TARGET_LITTLE_NAME              "elf32-littlearm"
#define TARGET_BIG_SYM                  bfd_elf32_bigarm_vec
#define TARGET_BIG_NAME                 "elf32-bigarm"

#define elf_backend_grok_prstatus	elf32_arm_nabi_grok_prstatus
#define elf_backend_grok_psinfo		elf32_arm_nabi_grok_psinfo

typedef unsigned long int insn32;
typedef unsigned short int insn16;

/* In lieu of proper flags, assume all EABIv4 objects are interworkable.  */
#define INTERWORK_FLAG(abfd)  \
  (EF_ARM_EABI_VERSION (elf_elfheader (abfd)->e_flags) == EF_ARM_EABI_VER4 \
  || (elf_elfheader (abfd)->e_flags & EF_ARM_INTERWORK))

/* The linker script knows the section names for placement.
   The entry_names are used to do simple name mangling on the stubs.
   Given a function name, and its type, the stub can be found. The
   name can be changed. The only requirement is the %s be present.  */
#define THUMB2ARM_GLUE_SECTION_NAME ".glue_7t"
#define THUMB2ARM_GLUE_ENTRY_NAME   "__%s_from_thumb"

#define ARM2THUMB_GLUE_SECTION_NAME ".glue_7"
#define ARM2THUMB_GLUE_ENTRY_NAME   "__%s_from_arm"

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */
#define ELF_DYNAMIC_INTERPRETER     "/usr/lib/ld.so.1"

#ifdef FOUR_WORD_PLT

/* The first entry in a procedure linkage table looks like
   this.  It is set up so that any shared library function that is
   called before the relocation has been set up calls the dynamic
   linker first.  */
static const bfd_vma elf32_arm_plt0_entry [] =
  {
    0xe52de004,		/* str   lr, [sp, #-4]! */
    0xe59fe010,		/* ldr   lr, [pc, #16]  */
    0xe08fe00e,		/* add   lr, pc, lr     */
    0xe5bef008,		/* ldr   pc, [lr, #8]!  */
  };

/* Subsequent entries in a procedure linkage table look like
   this.  */
static const bfd_vma elf32_arm_plt_entry [] =
  {
    0xe28fc600,		/* add   ip, pc, #NN	*/
    0xe28cca00,		/* add	 ip, ip, #NN	*/
    0xe5bcf000,		/* ldr	 pc, [ip, #NN]! */
    0x00000000,		/* unused		*/
  };

#else

/* The first entry in a procedure linkage table looks like
   this.  It is set up so that any shared library function that is
   called before the relocation has been set up calls the dynamic
   linker first.  */
static const bfd_vma elf32_arm_plt0_entry [] =
  {
    0xe52de004,		/* str   lr, [sp, #-4]! */
    0xe59fe004,		/* ldr   lr, [pc, #4]   */
    0xe08fe00e,		/* add   lr, pc, lr     */
    0xe5bef008,		/* ldr   pc, [lr, #8]!  */
    0x00000000,		/* &GOT[0] - .          */
  };

/* Subsequent entries in a procedure linkage table look like
   this.  */
static const bfd_vma elf32_arm_plt_entry [] =
  {
    0xe28fc600,		/* add   ip, pc, #0xNN00000 */
    0xe28cca00,		/* add	 ip, ip, #0xNN000   */
    0xe5bcf000,		/* ldr	 pc, [ip, #0xNNN]!  */
  };

#endif

/* An initial stub used if the PLT entry is referenced from Thumb code.  */
#define PLT_THUMB_STUB_SIZE 4
static const bfd_vma elf32_arm_plt_thumb_stub [] =
  {
    0x4778,		/* bx pc */
    0x46c0		/* nop   */
  };

/* The entries in a PLT when using a DLL-based target with multiple
   address spaces.  */
static const bfd_vma elf32_arm_symbian_plt_entry [] = 
  {
    0xe51ff004,         /* ldr   pc, [pc, #-4] */
    0x00000000,         /* dcd   R_ARM_GLOB_DAT(X) */
  };

/* Used to build a map of a section.  This is required for mixed-endian
   code/data.  */

typedef struct elf32_elf_section_map
{
  bfd_vma vma;
  char type;
}
elf32_arm_section_map;

struct _arm_elf_section_data
{
  struct bfd_elf_section_data elf;
  int mapcount;
  elf32_arm_section_map *map;
};

#define elf32_arm_section_data(sec) \
  ((struct _arm_elf_section_data *) elf_section_data (sec))

/* The ARM linker needs to keep track of the number of relocs that it
   decides to copy in check_relocs for each symbol.  This is so that
   it can discard PC relative relocs if it doesn't need them when
   linking with -Bsymbolic.  We store the information in a field
   extending the regular ELF linker hash table.  */

/* This structure keeps track of the number of PC relative relocs we
   have copied for a given symbol.  */
struct elf32_arm_relocs_copied
  {
    /* Next section.  */
    struct elf32_arm_relocs_copied * next;
    /* A section in dynobj.  */
    asection * section;
    /* Number of relocs copied in this section.  */
    bfd_size_type count;
  };

/* Arm ELF linker hash entry.  */
struct elf32_arm_link_hash_entry
  {
    struct elf_link_hash_entry root;

    /* Number of PC relative relocs copied for this symbol.  */
    struct elf32_arm_relocs_copied * relocs_copied;

    /* We reference count Thumb references to a PLT entry separately,
       so that we can emit the Thumb trampoline only if needed.  */
    bfd_signed_vma plt_thumb_refcount;

    /* Since PLT entries have variable size if the Thumb prologue is
       used, we need to record the index into .got.plt instead of
       recomputing it from the PLT offset.  */
    bfd_signed_vma plt_got_offset;
  };

/* Traverse an arm ELF linker hash table.  */
#define elf32_arm_link_hash_traverse(table, func, info)			\
  (elf_link_hash_traverse						\
   (&(table)->root,							\
    (bfd_boolean (*) (struct elf_link_hash_entry *, void *)) (func),	\
    (info)))

/* Get the ARM elf linker hash table from a link_info structure.  */
#define elf32_arm_hash_table(info) \
  ((struct elf32_arm_link_hash_table *) ((info)->hash))

/* ARM ELF linker hash table.  */
struct elf32_arm_link_hash_table
  {
    /* The main hash table.  */
    struct elf_link_hash_table root;

    /* The size in bytes of the section containing the Thumb-to-ARM glue.  */
    bfd_size_type thumb_glue_size;

    /* The size in bytes of the section containing the ARM-to-Thumb glue.  */
    bfd_size_type arm_glue_size;

    /* An arbitrary input BFD chosen to hold the glue sections.  */
    bfd * bfd_of_glue_owner;

    /* Nonzero to output a BE8 image.  */
    int byteswap_code;

    /* Zero if R_ARM_TARGET1 means R_ARM_ABS32.
       Nonzero if R_ARM_TARGET1 means R_ARM_ABS32.  */
    int target1_is_rel;

    /* The relocation to use for R_ARM_TARGET2 relocations.  */
    int target2_reloc;

    /* Nonzero to fix BX instructions for ARMv4 targets.  */
    int fix_v4bx;

    /* The number of bytes in the initial entry in the PLT.  */
    bfd_size_type plt_header_size;

    /* The number of bytes in the subsequent PLT etries.  */
    bfd_size_type plt_entry_size;

    /* True if the target system is Symbian OS.  */
    int symbian_p;

    /* True if the target uses REL relocations.  */
    int use_rel;

    /* Short-cuts to get to dynamic linker sections.  */
    asection *sgot;
    asection *sgotplt;
    asection *srelgot;
    asection *splt;
    asection *srelplt;
    asection *sdynbss;
    asection *srelbss;

    /* Small local sym to section mapping cache.  */
    struct sym_sec_cache sym_sec;

    /* For convenience in allocate_dynrelocs.  */
    bfd * obfd;
  };

/* Create an entry in an ARM ELF linker hash table.  */

static struct bfd_hash_entry *
elf32_arm_link_hash_newfunc (struct bfd_hash_entry * entry,
                             struct bfd_hash_table * table,
                             const char * string)
{
  struct elf32_arm_link_hash_entry * ret =
    (struct elf32_arm_link_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == (struct elf32_arm_link_hash_entry *) NULL)
    ret = bfd_hash_allocate (table, sizeof (struct elf32_arm_link_hash_entry));
  if (ret == NULL)
    return (struct bfd_hash_entry *) ret;

  /* Call the allocation method of the superclass.  */
  ret = ((struct elf32_arm_link_hash_entry *)
	 _bfd_elf_link_hash_newfunc ((struct bfd_hash_entry *) ret,
				     table, string));
  if (ret != NULL)
    {
      ret->relocs_copied = NULL;
      ret->plt_thumb_refcount = 0;
      ret->plt_got_offset = -1;
    }

  return (struct bfd_hash_entry *) ret;
}

/* Create .got, .gotplt, and .rel.got sections in DYNOBJ, and set up
   shortcuts to them in our hash table.  */

static bfd_boolean
create_got_section (bfd *dynobj, struct bfd_link_info *info)
{
  struct elf32_arm_link_hash_table *htab;

  htab = elf32_arm_hash_table (info);
  /* BPABI objects never have a GOT, or associated sections.  */
  if (htab->symbian_p)
    return TRUE;

  if (! _bfd_elf_create_got_section (dynobj, info))
    return FALSE;

  htab->sgot = bfd_get_section_by_name (dynobj, ".got");
  htab->sgotplt = bfd_get_section_by_name (dynobj, ".got.plt");
  if (!htab->sgot || !htab->sgotplt)
    abort ();

  htab->srelgot = bfd_make_section (dynobj, ".rel.got");
  if (htab->srelgot == NULL
      || ! bfd_set_section_flags (dynobj, htab->srelgot,
				  (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS
				   | SEC_IN_MEMORY | SEC_LINKER_CREATED
				   | SEC_READONLY))
      || ! bfd_set_section_alignment (dynobj, htab->srelgot, 2))
    return FALSE;
  return TRUE;
}

/* Create .plt, .rel.plt, .got, .got.plt, .rel.got, .dynbss, and
   .rel.bss sections in DYNOBJ, and set up shortcuts to them in our
   hash table.  */

static bfd_boolean
elf32_arm_create_dynamic_sections (bfd *dynobj, struct bfd_link_info *info)
{
  struct elf32_arm_link_hash_table *htab;

  htab = elf32_arm_hash_table (info);
  if (!htab->sgot && !create_got_section (dynobj, info))
    return FALSE;

  if (!_bfd_elf_create_dynamic_sections (dynobj, info))
    return FALSE;

  htab->splt = bfd_get_section_by_name (dynobj, ".plt");
  htab->srelplt = bfd_get_section_by_name (dynobj, ".rel.plt");
  htab->sdynbss = bfd_get_section_by_name (dynobj, ".dynbss");
  if (!info->shared)
    htab->srelbss = bfd_get_section_by_name (dynobj, ".rel.bss");

  if (!htab->splt 
      || !htab->srelplt
      || !htab->sdynbss
      || (!info->shared && !htab->srelbss))
    abort ();

  return TRUE;
}

/* Copy the extra info we tack onto an elf_link_hash_entry.  */

static void
elf32_arm_copy_indirect_symbol (const struct elf_backend_data *bed,
				struct elf_link_hash_entry *dir,
				struct elf_link_hash_entry *ind)
{
  struct elf32_arm_link_hash_entry *edir, *eind;

  edir = (struct elf32_arm_link_hash_entry *) dir;
  eind = (struct elf32_arm_link_hash_entry *) ind;

  if (eind->relocs_copied != NULL)
    {
      if (edir->relocs_copied != NULL)
	{
	  struct elf32_arm_relocs_copied **pp;
	  struct elf32_arm_relocs_copied *p;

	  if (ind->root.type == bfd_link_hash_indirect)
	    abort ();

	  /* Add reloc counts against the weak sym to the strong sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->relocs_copied; (p = *pp) != NULL; )
	    {
	      struct elf32_arm_relocs_copied *q;

	      for (q = edir->relocs_copied; q != NULL; q = q->next)
		if (q->section == p->section)
		  {
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->relocs_copied;
	}

      edir->relocs_copied = eind->relocs_copied;
      eind->relocs_copied = NULL;
    }

  /* If the direct symbol already has an associated PLT entry, the
     indirect symbol should not.  If it doesn't, swap refcount information
     from the indirect symbol.  */
  if (edir->plt_thumb_refcount == 0)
    {
      edir->plt_thumb_refcount = eind->plt_thumb_refcount;
      eind->plt_thumb_refcount = 0;
    }
  else
    BFD_ASSERT (eind->plt_thumb_refcount == 0);

  _bfd_elf_link_hash_copy_indirect (bed, dir, ind);
}

/* Create an ARM elf linker hash table.  */

static struct bfd_link_hash_table *
elf32_arm_link_hash_table_create (bfd *abfd)
{
  struct elf32_arm_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct elf32_arm_link_hash_table);

  ret = bfd_malloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (& ret->root, abfd,
				      elf32_arm_link_hash_newfunc))
    {
      free (ret);
      return NULL;
    }

  ret->sgot = NULL;
  ret->sgotplt = NULL;
  ret->srelgot = NULL;
  ret->splt = NULL;
  ret->srelplt = NULL;
  ret->sdynbss = NULL;
  ret->srelbss = NULL;
  ret->thumb_glue_size = 0;
  ret->arm_glue_size = 0;
  ret->bfd_of_glue_owner = NULL;
  ret->byteswap_code = 0;
  ret->target1_is_rel = 0;
  ret->target2_reloc = R_ARM_NONE;
#ifdef FOUR_WORD_PLT
  ret->plt_header_size = 16;
  ret->plt_entry_size = 16;
#else
  ret->plt_header_size = 20;
  ret->plt_entry_size = 12;
#endif
  ret->symbian_p = 0;
  ret->use_rel = 1;
  ret->sym_sec.abfd = NULL;
  ret->obfd = abfd;

  return &ret->root.root;
}

/* Locate the Thumb encoded calling stub for NAME.  */

static struct elf_link_hash_entry *
find_thumb_glue (struct bfd_link_info *link_info,
		 const char *name,
		 bfd *input_bfd)
{
  char *tmp_name;
  struct elf_link_hash_entry *hash;
  struct elf32_arm_link_hash_table *hash_table;

  /* We need a pointer to the armelf specific hash table.  */
  hash_table = elf32_arm_hash_table (link_info);

  tmp_name = bfd_malloc ((bfd_size_type) strlen (name)
			 + strlen (THUMB2ARM_GLUE_ENTRY_NAME) + 1);

  BFD_ASSERT (tmp_name);

  sprintf (tmp_name, THUMB2ARM_GLUE_ENTRY_NAME, name);

  hash = elf_link_hash_lookup
    (&(hash_table)->root, tmp_name, FALSE, FALSE, TRUE);

  if (hash == NULL)
    /* xgettext:c-format */
    (*_bfd_error_handler) (_("%B: unable to find THUMB glue '%s' for `%s'"),
			   input_bfd, tmp_name, name);

  free (tmp_name);

  return hash;
}

/* Locate the ARM encoded calling stub for NAME.  */

static struct elf_link_hash_entry *
find_arm_glue (struct bfd_link_info *link_info,
	       const char *name,
	       bfd *input_bfd)
{
  char *tmp_name;
  struct elf_link_hash_entry *myh;
  struct elf32_arm_link_hash_table *hash_table;

  /* We need a pointer to the elfarm specific hash table.  */
  hash_table = elf32_arm_hash_table (link_info);

  tmp_name = bfd_malloc ((bfd_size_type) strlen (name)
			 + strlen (ARM2THUMB_GLUE_ENTRY_NAME) + 1);

  BFD_ASSERT (tmp_name);

  sprintf (tmp_name, ARM2THUMB_GLUE_ENTRY_NAME, name);

  myh = elf_link_hash_lookup
    (&(hash_table)->root, tmp_name, FALSE, FALSE, TRUE);

  if (myh == NULL)
    /* xgettext:c-format */
    (*_bfd_error_handler) (_("%B: unable to find ARM glue '%s' for `%s'"),
			   input_bfd, tmp_name, name);

  free (tmp_name);

  return myh;
}

/* ARM->Thumb glue:

   .arm
   __func_from_arm:
   ldr r12, __func_addr
   bx  r12
   __func_addr:
   .word func    @ behave as if you saw a ARM_32 reloc.  */

#define ARM2THUMB_GLUE_SIZE 12
static const insn32 a2t1_ldr_insn = 0xe59fc000;
static const insn32 a2t2_bx_r12_insn = 0xe12fff1c;
static const insn32 a2t3_func_addr_insn = 0x00000001;

/* Thumb->ARM:                          Thumb->(non-interworking aware) ARM

   .thumb                               .thumb
   .align 2                             .align 2
   __func_from_thumb:              __func_from_thumb:
   bx pc                                push {r6, lr}
   nop                                  ldr  r6, __func_addr
   .arm                                         mov  lr, pc
   __func_change_to_arm:                        bx   r6
   b func                       .arm
   __func_back_to_thumb:
   ldmia r13! {r6, lr}
   bx    lr
   __func_addr:
   .word        func  */

#define THUMB2ARM_GLUE_SIZE 8
static const insn16 t2a1_bx_pc_insn = 0x4778;
static const insn16 t2a2_noop_insn = 0x46c0;
static const insn32 t2a3_b_insn = 0xea000000;

#ifndef ELFARM_NABI_C_INCLUDED
bfd_boolean
bfd_elf32_arm_allocate_interworking_sections (struct bfd_link_info * info)
{
  asection * s;
  bfd_byte * foo;
  struct elf32_arm_link_hash_table * globals;

  globals = elf32_arm_hash_table (info);

  BFD_ASSERT (globals != NULL);

  if (globals->arm_glue_size != 0)
    {
      BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

      s = bfd_get_section_by_name (globals->bfd_of_glue_owner,
				   ARM2THUMB_GLUE_SECTION_NAME);

      BFD_ASSERT (s != NULL);

      foo = bfd_alloc (globals->bfd_of_glue_owner, globals->arm_glue_size);

      s->size = globals->arm_glue_size;
      s->contents = foo;
    }

  if (globals->thumb_glue_size != 0)
    {
      BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

      s = bfd_get_section_by_name
	(globals->bfd_of_glue_owner, THUMB2ARM_GLUE_SECTION_NAME);

      BFD_ASSERT (s != NULL);

      foo = bfd_alloc (globals->bfd_of_glue_owner, globals->thumb_glue_size);

      s->size = globals->thumb_glue_size;
      s->contents = foo;
    }

  return TRUE;
}

static void
record_arm_to_thumb_glue (struct bfd_link_info * link_info,
			  struct elf_link_hash_entry * h)
{
  const char * name = h->root.root.string;
  asection * s;
  char * tmp_name;
  struct elf_link_hash_entry * myh;
  struct bfd_link_hash_entry * bh;
  struct elf32_arm_link_hash_table * globals;
  bfd_vma val;

  globals = elf32_arm_hash_table (link_info);

  BFD_ASSERT (globals != NULL);
  BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

  s = bfd_get_section_by_name
    (globals->bfd_of_glue_owner, ARM2THUMB_GLUE_SECTION_NAME);

  BFD_ASSERT (s != NULL);

  tmp_name = bfd_malloc ((bfd_size_type) strlen (name) + strlen (ARM2THUMB_GLUE_ENTRY_NAME) + 1);

  BFD_ASSERT (tmp_name);

  sprintf (tmp_name, ARM2THUMB_GLUE_ENTRY_NAME, name);

  myh = elf_link_hash_lookup
    (&(globals)->root, tmp_name, FALSE, FALSE, TRUE);

  if (myh != NULL)
    {
      /* We've already seen this guy.  */
      free (tmp_name);
      return;
    }

  /* The only trick here is using hash_table->arm_glue_size as the value.
     Even though the section isn't allocated yet, this is where we will be
     putting it.  */
  bh = NULL;
  val = globals->arm_glue_size + 1;
  _bfd_generic_link_add_one_symbol (link_info, globals->bfd_of_glue_owner,
				    tmp_name, BSF_GLOBAL, s, val,
				    NULL, TRUE, FALSE, &bh);

  myh = (struct elf_link_hash_entry *) bh;
  myh->type = ELF_ST_INFO (STB_LOCAL, STT_FUNC);
  myh->forced_local = 1;

  free (tmp_name);

  globals->arm_glue_size += ARM2THUMB_GLUE_SIZE;

  return;
}

static void
record_thumb_to_arm_glue (struct bfd_link_info *link_info,
			  struct elf_link_hash_entry *h)
{
  const char *name = h->root.root.string;
  asection *s;
  char *tmp_name;
  struct elf_link_hash_entry *myh;
  struct bfd_link_hash_entry *bh;
  struct elf32_arm_link_hash_table *hash_table;
  bfd_vma val;

  hash_table = elf32_arm_hash_table (link_info);

  BFD_ASSERT (hash_table != NULL);
  BFD_ASSERT (hash_table->bfd_of_glue_owner != NULL);

  s = bfd_get_section_by_name
    (hash_table->bfd_of_glue_owner, THUMB2ARM_GLUE_SECTION_NAME);

  BFD_ASSERT (s != NULL);

  tmp_name = bfd_malloc ((bfd_size_type) strlen (name)
			 + strlen (THUMB2ARM_GLUE_ENTRY_NAME) + 1);

  BFD_ASSERT (tmp_name);

  sprintf (tmp_name, THUMB2ARM_GLUE_ENTRY_NAME, name);

  myh = elf_link_hash_lookup
    (&(hash_table)->root, tmp_name, FALSE, FALSE, TRUE);

  if (myh != NULL)
    {
      /* We've already seen this guy.  */
      free (tmp_name);
      return;
    }

  bh = NULL;
  val = hash_table->thumb_glue_size + 1;
  _bfd_generic_link_add_one_symbol (link_info, hash_table->bfd_of_glue_owner,
				    tmp_name, BSF_GLOBAL, s, val,
				    NULL, TRUE, FALSE, &bh);

  /* If we mark it 'Thumb', the disassembler will do a better job.  */
  myh = (struct elf_link_hash_entry *) bh;
  myh->type = ELF_ST_INFO (STB_LOCAL, STT_ARM_TFUNC);
  myh->forced_local = 1;

  free (tmp_name);

#define CHANGE_TO_ARM "__%s_change_to_arm"
#define BACK_FROM_ARM "__%s_back_from_arm"

  /* Allocate another symbol to mark where we switch to Arm mode.  */
  tmp_name = bfd_malloc ((bfd_size_type) strlen (name)
			 + strlen (CHANGE_TO_ARM) + 1);

  BFD_ASSERT (tmp_name);

  sprintf (tmp_name, CHANGE_TO_ARM, name);

  bh = NULL;
  val = hash_table->thumb_glue_size + 4,
  _bfd_generic_link_add_one_symbol (link_info, hash_table->bfd_of_glue_owner,
				    tmp_name, BSF_LOCAL, s, val,
				    NULL, TRUE, FALSE, &bh);

  free (tmp_name);

  hash_table->thumb_glue_size += THUMB2ARM_GLUE_SIZE;

  return;
}

/* Add the glue sections to ABFD.  This function is called from the
   linker scripts in ld/emultempl/{armelf}.em.  */

bfd_boolean
bfd_elf32_arm_add_glue_sections_to_bfd (bfd *abfd,
					struct bfd_link_info *info)
{
  flagword flags;
  asection *sec;

  /* If we are only performing a partial
     link do not bother adding the glue.  */
  if (info->relocatable)
    return TRUE;

  sec = bfd_get_section_by_name (abfd, ARM2THUMB_GLUE_SECTION_NAME);

  if (sec == NULL)
    {
      /* Note: we do not include the flag SEC_LINKER_CREATED, as this
	 will prevent elf_link_input_bfd() from processing the contents
	 of this section.  */
      flags = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY | SEC_CODE | SEC_READONLY;

      sec = bfd_make_section (abfd, ARM2THUMB_GLUE_SECTION_NAME);

      if (sec == NULL
	  || !bfd_set_section_flags (abfd, sec, flags)
	  || !bfd_set_section_alignment (abfd, sec, 2))
	return FALSE;

      /* Set the gc mark to prevent the section from being removed by garbage
	 collection, despite the fact that no relocs refer to this section.  */
      sec->gc_mark = 1;
    }

  sec = bfd_get_section_by_name (abfd, THUMB2ARM_GLUE_SECTION_NAME);

  if (sec == NULL)
    {
      flags = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY
	| SEC_CODE | SEC_READONLY;

      sec = bfd_make_section (abfd, THUMB2ARM_GLUE_SECTION_NAME);

      if (sec == NULL
	  || !bfd_set_section_flags (abfd, sec, flags)
	  || !bfd_set_section_alignment (abfd, sec, 2))
	return FALSE;

      sec->gc_mark = 1;
    }

  return TRUE;
}

/* Select a BFD to be used to hold the sections used by the glue code.
   This function is called from the linker scripts in ld/emultempl/
   {armelf/pe}.em  */

bfd_boolean
bfd_elf32_arm_get_bfd_for_interworking (bfd *abfd, struct bfd_link_info *info)
{
  struct elf32_arm_link_hash_table *globals;

  /* If we are only performing a partial link
     do not bother getting a bfd to hold the glue.  */
  if (info->relocatable)
    return TRUE;

  /* Make sure we don't attach the glue sections to a dynamic object.  */
  BFD_ASSERT (!(abfd->flags & DYNAMIC));

  globals = elf32_arm_hash_table (info);

  BFD_ASSERT (globals != NULL);

  if (globals->bfd_of_glue_owner != NULL)
    return TRUE;

  /* Save the bfd for later use.  */
  globals->bfd_of_glue_owner = abfd;

  return TRUE;
}

bfd_boolean
bfd_elf32_arm_process_before_allocation (bfd *abfd,
					 struct bfd_link_info *link_info,
					 int byteswap_code)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs = NULL;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents = NULL;

  asection *sec;
  struct elf32_arm_link_hash_table *globals;

  /* If we are only performing a partial link do not bother
     to construct any glue.  */
  if (link_info->relocatable)
    return TRUE;

  /* Here we have a bfd that is to be included on the link.  We have a hook
     to do reloc rummaging, before section sizes are nailed down.  */
  globals = elf32_arm_hash_table (link_info);

  BFD_ASSERT (globals != NULL);
  BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

  if (byteswap_code && !bfd_big_endian (abfd))
    {
      _bfd_error_handler (_("%B: BE8 images only valid in big-endian mode."),
			  abfd);
      return FALSE;
    }
  globals->byteswap_code = byteswap_code;

  /* Rummage around all the relocs and map the glue vectors.  */
  sec = abfd->sections;

  if (sec == NULL)
    return TRUE;

  for (; sec != NULL; sec = sec->next)
    {
      if (sec->reloc_count == 0)
	continue;

      symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

      /* Load the relocs.  */
      internal_relocs
	= _bfd_elf_link_read_relocs (abfd, sec, (void *) NULL,
				     (Elf_Internal_Rela *) NULL, FALSE);

      if (internal_relocs == NULL)
	goto error_return;

      irelend = internal_relocs + sec->reloc_count;
      for (irel = internal_relocs; irel < irelend; irel++)
	{
	  long r_type;
	  unsigned long r_index;

	  struct elf_link_hash_entry *h;

	  r_type = ELF32_R_TYPE (irel->r_info);
	  r_index = ELF32_R_SYM (irel->r_info);

	  /* These are the only relocation types we care about.  */
	  if (   r_type != R_ARM_PC24
	      && r_type != R_ARM_PLT32
#ifndef OLD_ARM_ABI
	      && r_type != R_ARM_CALL
	      && r_type != R_ARM_JUMP24
#endif
	      && r_type != R_ARM_THM_PC22)
	    continue;

	  /* Get the section contents if we haven't done so already.  */
	  if (contents == NULL)
	    {
	      /* Get cached copy if it exists.  */
	      if (elf_section_data (sec)->this_hdr.contents != NULL)
		contents = elf_section_data (sec)->this_hdr.contents;
	      else
		{
		  /* Go get them off disk.  */
		  if (! bfd_malloc_and_get_section (abfd, sec, &contents))
		    goto error_return;
		}
	    }

	  /* If the relocation is not against a symbol it cannot concern us.  */
	  h = NULL;

	  /* We don't care about local symbols.  */
	  if (r_index < symtab_hdr->sh_info)
	    continue;

	  /* This is an external symbol.  */
	  r_index -= symtab_hdr->sh_info;
	  h = (struct elf_link_hash_entry *)
	    elf_sym_hashes (abfd)[r_index];

	  /* If the relocation is against a static symbol it must be within
	     the current section and so cannot be a cross ARM/Thumb relocation.  */
	  if (h == NULL)
	    continue;

	  /* If the call will go through a PLT entry then we do not need
	     glue.  */
	  if (globals->splt != NULL && h->plt.offset != (bfd_vma) -1)
	    continue;

	  switch (r_type)
	    {
	    case R_ARM_PC24:
	    case R_ARM_PLT32:
#ifndef OLD_ARM_ABI
	    case R_ARM_CALL:
	    case R_ARM_JUMP24:
#endif
	      /* This one is a call from arm code.  We need to look up
	         the target of the call.  If it is a thumb target, we
	         insert glue.  */
	      if (ELF_ST_TYPE(h->type) == STT_ARM_TFUNC)
		record_arm_to_thumb_glue (link_info, h);
	      break;

	    case R_ARM_THM_PC22:
	      /* This one is a call from thumb code.  We look
	         up the target of the call.  If it is not a thumb
                 target, we insert glue.  */
	      if (ELF_ST_TYPE (h->type) != STT_ARM_TFUNC)
		record_thumb_to_arm_glue (link_info, h);
	      break;

	    default:
	      abort ();
	    }
	}

      if (contents != NULL
	  && elf_section_data (sec)->this_hdr.contents != contents)
	free (contents);
      contents = NULL;

      if (internal_relocs != NULL
	  && elf_section_data (sec)->relocs != internal_relocs)
	free (internal_relocs);
      internal_relocs = NULL;
    }

  return TRUE;

error_return:
  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    free (contents);
  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return FALSE;
}
#endif


#ifndef OLD_ARM_ABI
/* Set target relocation values needed during linking.  */

void
bfd_elf32_arm_set_target_relocs (struct bfd_link_info *link_info,
				 int target1_is_rel,
				 char * target2_type,
                                 int fix_v4bx)
{
  struct elf32_arm_link_hash_table *globals;

  globals = elf32_arm_hash_table (link_info);

  globals->target1_is_rel = target1_is_rel;
  if (strcmp (target2_type, "rel") == 0)
    globals->target2_reloc = R_ARM_REL32;
  else if (strcmp (target2_type, "abs") == 0)
    globals->target2_reloc = R_ARM_ABS32;
  else if (strcmp (target2_type, "got-rel") == 0)
    globals->target2_reloc = R_ARM_GOT_PREL;
  else
    {
      _bfd_error_handler (_("Invalid TARGET2 relocation type '%s'."),
			  target2_type);
    }
  globals->fix_v4bx = fix_v4bx;
}
#endif

/* The thumb form of a long branch is a bit finicky, because the offset
   encoding is split over two fields, each in it's own instruction. They
   can occur in any order. So given a thumb form of long branch, and an
   offset, insert the offset into the thumb branch and return finished
   instruction.

   It takes two thumb instructions to encode the target address. Each has
   11 bits to invest. The upper 11 bits are stored in one (identified by
   H-0.. see below), the lower 11 bits are stored in the other (identified
   by H-1).

   Combine together and shifted left by 1 (it's a half word address) and
   there you have it.

   Op: 1111 = F,
   H-0, upper address-0 = 000
   Op: 1111 = F,
   H-1, lower address-0 = 800

   They can be ordered either way, but the arm tools I've seen always put
   the lower one first. It probably doesn't matter. krk@cygnus.com

   XXX:  Actually the order does matter.  The second instruction (H-1)
   moves the computed address into the PC, so it must be the second one
   in the sequence.  The problem, however is that whilst little endian code
   stores the instructions in HI then LOW order, big endian code does the
   reverse.  nickc@cygnus.com.  */

#define LOW_HI_ORDER      0xF800F000
#define HI_LOW_ORDER      0xF000F800

static insn32
insert_thumb_branch (insn32 br_insn, int rel_off)
{
  unsigned int low_bits;
  unsigned int high_bits;

  BFD_ASSERT ((rel_off & 1) != 1);

  rel_off >>= 1;				/* Half word aligned address.  */
  low_bits = rel_off & 0x000007FF;		/* The bottom 11 bits.  */
  high_bits = (rel_off >> 11) & 0x000007FF;	/* The top 11 bits.  */

  if ((br_insn & LOW_HI_ORDER) == LOW_HI_ORDER)
    br_insn = LOW_HI_ORDER | (low_bits << 16) | high_bits;
  else if ((br_insn & HI_LOW_ORDER) == HI_LOW_ORDER)
    br_insn = HI_LOW_ORDER | (high_bits << 16) | low_bits;
  else
    /* FIXME: abort is probably not the right call. krk@cygnus.com  */
    abort ();	/* Error - not a valid branch instruction form.  */

  return br_insn;
}

/* Thumb code calling an ARM function.  */

static int
elf32_thumb_to_arm_stub (struct bfd_link_info * info,
			 const char *           name,
			 bfd *                  input_bfd,
			 bfd *                  output_bfd,
			 asection *             input_section,
			 bfd_byte *             hit_data,
			 asection *             sym_sec,
			 bfd_vma                offset,
			 bfd_signed_vma         addend,
			 bfd_vma                val)
{
  asection * s = 0;
  bfd_vma my_offset;
  unsigned long int tmp;
  long int ret_offset;
  struct elf_link_hash_entry * myh;
  struct elf32_arm_link_hash_table * globals;

  myh = find_thumb_glue (info, name, input_bfd);
  if (myh == NULL)
    return FALSE;

  globals = elf32_arm_hash_table (info);

  BFD_ASSERT (globals != NULL);
  BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

  my_offset = myh->root.u.def.value;

  s = bfd_get_section_by_name (globals->bfd_of_glue_owner,
			       THUMB2ARM_GLUE_SECTION_NAME);

  BFD_ASSERT (s != NULL);
  BFD_ASSERT (s->contents != NULL);
  BFD_ASSERT (s->output_section != NULL);

  if ((my_offset & 0x01) == 0x01)
    {
      if (sym_sec != NULL
	  && sym_sec->owner != NULL
	  && !INTERWORK_FLAG (sym_sec->owner))
	{
	  (*_bfd_error_handler)
	    (_("%B(%s): warning: interworking not enabled.\n"
	       "  first occurrence: %B: thumb call to arm"),
	     sym_sec->owner, input_bfd, name);

	  return FALSE;
	}

      --my_offset;
      myh->root.u.def.value = my_offset;

      bfd_put_16 (output_bfd, (bfd_vma) t2a1_bx_pc_insn,
		  s->contents + my_offset);

      bfd_put_16 (output_bfd, (bfd_vma) t2a2_noop_insn,
		  s->contents + my_offset + 2);

      ret_offset =
	/* Address of destination of the stub.  */
	((bfd_signed_vma) val)
	- ((bfd_signed_vma)
	   /* Offset from the start of the current section
	      to the start of the stubs.  */
	   (s->output_offset
	    /* Offset of the start of this stub from the start of the stubs.  */
	    + my_offset
	    /* Address of the start of the current section.  */
	    + s->output_section->vma)
	   /* The branch instruction is 4 bytes into the stub.  */
	   + 4
	   /* ARM branches work from the pc of the instruction + 8.  */
	   + 8);

      bfd_put_32 (output_bfd,
		  (bfd_vma) t2a3_b_insn | ((ret_offset >> 2) & 0x00FFFFFF),
		  s->contents + my_offset + 4);
    }

  BFD_ASSERT (my_offset <= globals->thumb_glue_size);

  /* Now go back and fix up the original BL insn to point to here.  */
  ret_offset =
    /* Address of where the stub is located.  */
    (s->output_section->vma + s->output_offset + my_offset)
     /* Address of where the BL is located.  */
    - (input_section->output_section->vma + input_section->output_offset
       + offset)
    /* Addend in the relocation.  */
    - addend
    /* Biassing for PC-relative addressing.  */
    - 8;

  tmp = bfd_get_32 (input_bfd, hit_data
		    - input_section->vma);

  bfd_put_32 (output_bfd,
	      (bfd_vma) insert_thumb_branch (tmp, ret_offset),
	      hit_data - input_section->vma);

  return TRUE;
}

/* Arm code calling a Thumb function.  */

static int
elf32_arm_to_thumb_stub (struct bfd_link_info * info,
			 const char *           name,
			 bfd *                  input_bfd,
			 bfd *                  output_bfd,
			 asection *             input_section,
			 bfd_byte *             hit_data,
			 asection *             sym_sec,
			 bfd_vma                offset,
			 bfd_signed_vma         addend,
			 bfd_vma                val)
{
  unsigned long int tmp;
  bfd_vma my_offset;
  asection * s;
  long int ret_offset;
  struct elf_link_hash_entry * myh;
  struct elf32_arm_link_hash_table * globals;

  myh = find_arm_glue (info, name, input_bfd);
  if (myh == NULL)
    return FALSE;

  globals = elf32_arm_hash_table (info);

  BFD_ASSERT (globals != NULL);
  BFD_ASSERT (globals->bfd_of_glue_owner != NULL);

  my_offset = myh->root.u.def.value;
  s = bfd_get_section_by_name (globals->bfd_of_glue_owner,
			       ARM2THUMB_GLUE_SECTION_NAME);
  BFD_ASSERT (s != NULL);
  BFD_ASSERT (s->contents != NULL);
  BFD_ASSERT (s->output_section != NULL);

  if ((my_offset & 0x01) == 0x01)
    {
      if (sym_sec != NULL
	  && sym_sec->owner != NULL
	  && !INTERWORK_FLAG (sym_sec->owner))
	{
	  (*_bfd_error_handler)
	    (_("%B(%s): warning: interworking not enabled.\n"
	       "  first occurrence: %B: arm call to thumb"),
	     sym_sec->owner, input_bfd, name);
	}

      --my_offset;
      myh->root.u.def.value = my_offset;

      bfd_put_32 (output_bfd, (bfd_vma) a2t1_ldr_insn,
		  s->contents + my_offset);

      bfd_put_32 (output_bfd, (bfd_vma) a2t2_bx_r12_insn,
		  s->contents + my_offset + 4);

      /* It's a thumb address.  Add the low order bit.  */
      bfd_put_32 (output_bfd, val | a2t3_func_addr_insn,
		  s->contents + my_offset + 8);
    }

  BFD_ASSERT (my_offset <= globals->arm_glue_size);

  tmp = bfd_get_32 (input_bfd, hit_data);
  tmp = tmp & 0xFF000000;

  /* Somehow these are both 4 too far, so subtract 8.  */
  ret_offset = (s->output_offset
		+ my_offset
		+ s->output_section->vma
		- (input_section->output_offset
		   + input_section->output_section->vma
		   + offset + addend)
		- 8);

  tmp = tmp | ((ret_offset >> 2) & 0x00FFFFFF);

  bfd_put_32 (output_bfd, (bfd_vma) tmp, hit_data - input_section->vma);

  return TRUE;
}


#ifndef OLD_ARM_ABI
/* Some relocations map to different relocations depending on the
   target.  Return the real relocation.  */
static int
arm_real_reloc_type (struct elf32_arm_link_hash_table * globals,
		     int r_type)
{
  switch (r_type)
    {
    case R_ARM_TARGET1:
      if (globals->target1_is_rel)
	return R_ARM_REL32;
      else
	return R_ARM_ABS32;

    case R_ARM_TARGET2:
      return globals->target2_reloc;

    default:
      return r_type;
    }
}
#endif /* OLD_ARM_ABI */


/* Perform a relocation as part of a final link.  */

static bfd_reloc_status_type
elf32_arm_final_link_relocate (reloc_howto_type *           howto,
			       bfd *                        input_bfd,
			       bfd *                        output_bfd,
			       asection *                   input_section,
			       bfd_byte *                   contents,
			       Elf_Internal_Rela *          rel,
			       bfd_vma                      value,
			       struct bfd_link_info *       info,
			       asection *                   sym_sec,
			       const char *                 sym_name,
			       int		            sym_flags,
			       struct elf_link_hash_entry * h,
			       bfd_boolean *                unresolved_reloc_p)
{
  unsigned long                 r_type = howto->type;
  unsigned long                 r_symndx;
  bfd_byte *                    hit_data = contents + rel->r_offset;
  bfd *                         dynobj = NULL;
  Elf_Internal_Shdr *           symtab_hdr;
  struct elf_link_hash_entry ** sym_hashes;
  bfd_vma *                     local_got_offsets;
  asection *                    sgot = NULL;
  asection *                    splt = NULL;
  asection *                    sreloc = NULL;
  bfd_vma                       addend;
  bfd_signed_vma                signed_addend;
  struct elf32_arm_link_hash_table * globals;

  globals = elf32_arm_hash_table (info);

#ifndef OLD_ARM_ABI
  /* Some relocation type map to different relocations depending on the
     target.  We pick the right one here.  */
  r_type = arm_real_reloc_type (globals, r_type);
  if (r_type != howto->type)
    howto = elf32_arm_howto_from_type (r_type);
#endif /* OLD_ARM_ABI */

  /* If the start address has been set, then set the EF_ARM_HASENTRY
     flag.  Setting this more than once is redundant, but the cost is
     not too high, and it keeps the code simple.

     The test is done  here, rather than somewhere else, because the
     start address is only set just before the final link commences.

     Note - if the user deliberately sets a start address of 0, the
     flag will not be set.  */
  if (bfd_get_start_address (output_bfd) != 0)
    elf_elfheader (output_bfd)->e_flags |= EF_ARM_HASENTRY;

  dynobj = elf_hash_table (info)->dynobj;
  if (dynobj)
    {
      sgot = bfd_get_section_by_name (dynobj, ".got");
      splt = bfd_get_section_by_name (dynobj, ".plt");
    }
  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  local_got_offsets = elf_local_got_offsets (input_bfd);
  r_symndx = ELF32_R_SYM (rel->r_info);

  if (globals->use_rel)
    {
      addend = bfd_get_32 (input_bfd, hit_data) & howto->src_mask;

      if (addend & ((howto->src_mask + 1) >> 1))
	{
	  signed_addend = -1;
	  signed_addend &= ~ howto->src_mask;
	  signed_addend |= addend;
	}
      else
	signed_addend = addend;
    }
  else
    addend = signed_addend = rel->r_addend;

  switch (r_type)
    {
    case R_ARM_NONE:
      /* We don't need to find a value for this symbol.  It's just a
	 marker.  */
      *unresolved_reloc_p = FALSE;
      return bfd_reloc_ok;

    case R_ARM_PC24:
    case R_ARM_ABS32:
    case R_ARM_REL32:
#ifndef OLD_ARM_ABI
    case R_ARM_CALL:
    case R_ARM_JUMP24:
    case R_ARM_XPC25:
    case R_ARM_PREL31:
#endif
    case R_ARM_PLT32:
      /* r_symndx will be zero only for relocs against symbols
	 from removed linkonce sections, or sections discarded by
	 a linker script.  */
      if (r_symndx == 0)
	return bfd_reloc_ok;

      /* Handle relocations which should use the PLT entry.  ABS32/REL32
	 will use the symbol's value, which may point to a PLT entry, but we
	 don't need to handle that here.  If we created a PLT entry, all
	 branches in this object should go to it.  */
      if ((r_type != R_ARM_ABS32 && r_type != R_ARM_REL32)
	  && h != NULL
	  && splt != NULL
	  && h->plt.offset != (bfd_vma) -1)
	{
	  /* If we've created a .plt section, and assigned a PLT entry to
	     this function, it should not be known to bind locally.  If
	     it were, we would have cleared the PLT entry.  */
	  BFD_ASSERT (!SYMBOL_CALLS_LOCAL (info, h));

	  value = (splt->output_section->vma
		   + splt->output_offset
		   + h->plt.offset);
	  *unresolved_reloc_p = FALSE;
	  return _bfd_final_link_relocate (howto, input_bfd, input_section,
					   contents, rel->r_offset, value,
					   (bfd_vma) 0);
	}

      /* When generating a shared object or relocatable executable, these
	 relocations are copied into the output file to be resolved at
	 run time.  */
      if ((info->shared || globals->root.is_relocatable_executable)
	  && (input_section->flags & SEC_ALLOC)
	  && (r_type != R_ARM_REL32
	      || !SYMBOL_CALLS_LOCAL (info, h))
	  && (h == NULL
	      || ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	      || h->root.type != bfd_link_hash_undefweak)
	  && r_type != R_ARM_PC24
#ifndef OLD_ARM_ABI
	  && r_type != R_ARM_CALL
	  && r_type != R_ARM_JUMP24
	  && r_type != R_ARM_PREL31
#endif
	  && r_type != R_ARM_PLT32)
	{
	  Elf_Internal_Rela outrel;
	  bfd_byte *loc;
	  bfd_boolean skip, relocate;

	  *unresolved_reloc_p = FALSE;

	  if (sreloc == NULL)
	    {
	      const char * name;

	      name = (bfd_elf_string_from_elf_section
		      (input_bfd,
		       elf_elfheader (input_bfd)->e_shstrndx,
		       elf_section_data (input_section)->rel_hdr.sh_name));
	      if (name == NULL)
		return bfd_reloc_notsupported;

	      BFD_ASSERT (strncmp (name, ".rel", 4) == 0
			  && strcmp (bfd_get_section_name (input_bfd,
							   input_section),
				     name + 4) == 0);

	      sreloc = bfd_get_section_by_name (dynobj, name);
	      BFD_ASSERT (sreloc != NULL);
	    }

	  skip = FALSE;
	  relocate = FALSE;

	  outrel.r_offset =
	    _bfd_elf_section_offset (output_bfd, info, input_section,
				     rel->r_offset);
	  if (outrel.r_offset == (bfd_vma) -1)
	    skip = TRUE;
	  else if (outrel.r_offset == (bfd_vma) -2)
	    skip = TRUE, relocate = TRUE;
	  outrel.r_offset += (input_section->output_section->vma
			      + input_section->output_offset);

	  if (skip)
	    memset (&outrel, 0, sizeof outrel);
	  else if (h != NULL
		   && h->dynindx != -1
		   && (!info->shared
		       || !info->symbolic
		       || !h->def_regular))
	    outrel.r_info = ELF32_R_INFO (h->dynindx, r_type);
	  else
	    {
	      int symbol;

	      /* This symbol is local, or marked to become local.  */
	      relocate = TRUE;
	      if (sym_flags == STT_ARM_TFUNC)
		value |= 1;
	      if (globals->symbian_p)
		{
		  /* On Symbian OS, the data segment and text segement
		     can be relocated independently.  Therefore, we
		     must indicate the segment to which this
		     relocation is relative.  The BPABI allows us to
		     use any symbol in the right segment; we just use
		     the section symbol as it is convenient.  (We
		     cannot use the symbol given by "h" directly as it
		     will not appear in the dynamic symbol table.)  */
		  symbol = elf_section_data (sym_sec->output_section)->dynindx;
		  BFD_ASSERT (symbol != 0);
		}
	      else
		/* On SVR4-ish systems, the dynamic loader cannot
		   relocate the text and data segments independently,
		   so the symbol does not matter.  */
		symbol = 0;
	      outrel.r_info = ELF32_R_INFO (symbol, R_ARM_RELATIVE);
	    }

	  loc = sreloc->contents;
	  loc += sreloc->reloc_count++ * sizeof (Elf32_External_Rel);
	  bfd_elf32_swap_reloc_out (output_bfd, &outrel, loc);

	  /* If this reloc is against an external symbol, we do not want to
	     fiddle with the addend.  Otherwise, we need to include the symbol
	     value so that it becomes an addend for the dynamic reloc.  */
	  if (! relocate)
	    return bfd_reloc_ok;

	  return _bfd_final_link_relocate (howto, input_bfd, input_section,
					   contents, rel->r_offset, value,
					   (bfd_vma) 0);
	}
      else switch (r_type)
	{
#ifndef OLD_ARM_ABI
	case R_ARM_XPC25:	  /* Arm BLX instruction.  */
	case R_ARM_CALL:
	case R_ARM_JUMP24:
#endif
	case R_ARM_PC24:	  /* Arm B/BL instruction */
	case R_ARM_PLT32:
#ifndef OLD_ARM_ABI
	  if (r_type == R_ARM_XPC25)
	    {
	      /* Check for Arm calling Arm function.  */
	      /* FIXME: Should we translate the instruction into a BL
		 instruction instead ?  */
	      if (sym_flags != STT_ARM_TFUNC)
		(*_bfd_error_handler)
		  (_("\%B: Warning: Arm BLX instruction targets Arm function '%s'."),
		   input_bfd,
		   h ? h->root.root.string : "(local)");
	    }
	  else
#endif
	    {
	      /* Check for Arm calling Thumb function.  */
	      if (sym_flags == STT_ARM_TFUNC)
		{
		  elf32_arm_to_thumb_stub (info, sym_name, input_bfd,
					   output_bfd, input_section,
					   hit_data, sym_sec, rel->r_offset,
					   signed_addend, value);
		  return bfd_reloc_ok;
		}
	    }

	  /* The ARM ELF ABI says that this reloc is computed as: S - P + A
	     where:
	      S is the address of the symbol in the relocation.
	      P is address of the instruction being relocated.
	      A is the addend (extracted from the instruction) in bytes.

	     S is held in 'value'.
	     P is the base address of the section containing the
	       instruction plus the offset of the reloc into that
	       section, ie:
		 (input_section->output_section->vma +
		  input_section->output_offset +
		  rel->r_offset).
	     A is the addend, converted into bytes, ie:
		 (signed_addend * 4)

	     Note: None of these operations have knowledge of the pipeline
	     size of the processor, thus it is up to the assembler to
	     encode this information into the addend.  */
	  value -= (input_section->output_section->vma
		    + input_section->output_offset);
	  value -= rel->r_offset;
	  if (globals->use_rel)
	    value += (signed_addend << howto->size);
	  else
	    /* RELA addends do not have to be adjusted by howto->size.  */
	    value += signed_addend;

	  signed_addend = value;
	  signed_addend >>= howto->rightshift;

	  /* It is not an error for an undefined weak reference to be
	     out of range.  Any program that branches to such a symbol
	     is going to crash anyway, so there is no point worrying
	     about getting the destination exactly right.  */
	  if (! h || h->root.type != bfd_link_hash_undefweak)
	    {
	      /* Perform a signed range check.  */
	      if (   signed_addend >   ((bfd_signed_vma)  (howto->dst_mask >> 1))
		  || signed_addend < - ((bfd_signed_vma) ((howto->dst_mask + 1) >> 1)))
		return bfd_reloc_overflow;
	    }

#ifndef OLD_ARM_ABI
	  /* If necessary set the H bit in the BLX instruction.  */
	  if (r_type == R_ARM_XPC25 && ((value & 2) == 2))
	    value = (signed_addend & howto->dst_mask)
	      | (bfd_get_32 (input_bfd, hit_data) & (~ howto->dst_mask))
	      | (1 << 24);
	  else
#endif
	    value = (signed_addend & howto->dst_mask)
	      | (bfd_get_32 (input_bfd, hit_data) & (~ howto->dst_mask));
	  break;

	case R_ARM_ABS32:
	  value += addend;
	  if (sym_flags == STT_ARM_TFUNC)
	    value |= 1;
	  break;

	case R_ARM_REL32:
	  value -= (input_section->output_section->vma
		    + input_section->output_offset + rel->r_offset);
	  value += addend;
	  break;

#ifndef OLD_ARM_ABI
	case R_ARM_PREL31:
	  value -= (input_section->output_section->vma
		    + input_section->output_offset + rel->r_offset);
	  value += signed_addend;
	  if (! h || h->root.type != bfd_link_hash_undefweak)
	    {
	      /* Check for overflow */
	      if ((value ^ (value >> 1)) & (1 << 30))
		return bfd_reloc_overflow;
	    }
	  value &= 0x7fffffff;
	  value |= (bfd_get_32 (input_bfd, hit_data) & 0x80000000);
	  if (sym_flags == STT_ARM_TFUNC)
	    value |= 1;
	  break;
#endif
	}

      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_ARM_ABS8:
      value += addend;
      if ((long) value > 0x7f || (long) value < -0x80)
	return bfd_reloc_overflow;

      bfd_put_8 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_ARM_ABS16:
      value += addend;

      if ((long) value > 0x7fff || (long) value < -0x8000)
	return bfd_reloc_overflow;

      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_ARM_ABS12:
      /* Support ldr and str instruction for the arm */
      /* Also thumb b (unconditional branch).  ??? Really?  */
      value += addend;

      if ((long) value > 0x7ff || (long) value < -0x800)
	return bfd_reloc_overflow;

      value |= (bfd_get_32 (input_bfd, hit_data) & 0xfffff000);
      bfd_put_32 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

    case R_ARM_THM_ABS5:
      /* Support ldr and str instructions for the thumb.  */
      if (globals->use_rel)
	{
	  /* Need to refetch addend.  */
	  addend = bfd_get_16 (input_bfd, hit_data) & howto->src_mask;
	  /* ??? Need to determine shift amount from operand size.  */
	  addend >>= howto->rightshift;
	}
      value += addend;

      /* ??? Isn't value unsigned?  */
      if ((long) value > 0x1f || (long) value < -0x10)
	return bfd_reloc_overflow;

      /* ??? Value needs to be properly shifted into place first.  */
      value |= bfd_get_16 (input_bfd, hit_data) & 0xf83f;
      bfd_put_16 (input_bfd, value, hit_data);
      return bfd_reloc_ok;

#ifndef OLD_ARM_ABI
    case R_ARM_THM_XPC22:
#endif
    case R_ARM_THM_PC22:
      /* Thumb BL (branch long instruction).  */
      {
	bfd_vma relocation;
	bfd_boolean overflow = FALSE;
	bfd_vma upper_insn = bfd_get_16 (input_bfd, hit_data);
	bfd_vma lower_insn = bfd_get_16 (input_bfd, hit_data + 2);
	bfd_signed_vma reloc_signed_max = ((1 << (howto->bitsize - 1)) - 1) >> howto->rightshift;
	bfd_signed_vma reloc_signed_min = ~ reloc_signed_max;
	bfd_vma check;
	bfd_signed_vma signed_check;

	/* Need to refetch the addend and squish the two 11 bit pieces
	   together.  */
	if (globals->use_rel)
	  {
	    bfd_vma upper = upper_insn & 0x7ff;
	    bfd_vma lower = lower_insn & 0x7ff;
	    upper = (upper ^ 0x400) - 0x400; /* Sign extend.  */
	    addend = (upper << 12) | (lower << 1);
	    signed_addend = addend;
	  }
#ifndef OLD_ARM_ABI
	if (r_type == R_ARM_THM_XPC22)
	  {
	    /* Check for Thumb to Thumb call.  */
	    /* FIXME: Should we translate the instruction into a BL
	       instruction instead ?  */
	    if (sym_flags == STT_ARM_TFUNC)
	      (*_bfd_error_handler)
		(_("%B: Warning: Thumb BLX instruction targets thumb function '%s'."),
		 input_bfd,
		 h ? h->root.root.string : "(local)");
	  }
	else
#endif
	  {
	    /* If it is not a call to Thumb, assume call to Arm.
	       If it is a call relative to a section name, then it is not a
	       function call at all, but rather a long jump.  Calls through
	       the PLT do not require stubs.  */
	    if (sym_flags != STT_ARM_TFUNC && sym_flags != STT_SECTION
		&& (h == NULL || splt == NULL
		    || h->plt.offset == (bfd_vma) -1))
	      {
		if (elf32_thumb_to_arm_stub
		    (info, sym_name, input_bfd, output_bfd, input_section,
		     hit_data, sym_sec, rel->r_offset, signed_addend, value))
		  return bfd_reloc_ok;
		else
		  return bfd_reloc_dangerous;
	      }
	  }

	/* Handle calls via the PLT.  */
	if (h != NULL && splt != NULL && h->plt.offset != (bfd_vma) -1)
	  {
	    value = (splt->output_section->vma
		     + splt->output_offset
		     + h->plt.offset);
	    /* Target the Thumb stub before the ARM PLT entry.  */
	    value -= 4;
	    *unresolved_reloc_p = FALSE;
	  }

	relocation = value + signed_addend;

	relocation -= (input_section->output_section->vma
		       + input_section->output_offset
		       + rel->r_offset);

	check = relocation >> howto->rightshift;

	/* If this is a signed value, the rightshift just dropped
	   leading 1 bits (assuming twos complement).  */
	if ((bfd_signed_vma) relocation >= 0)
	  signed_check = check;
	else
	  signed_check = check | ~((bfd_vma) -1 >> howto->rightshift);

	/* Assumes two's complement.  */
	if (signed_check > reloc_signed_max || signed_check < reloc_signed_min)
	  overflow = TRUE;

#ifndef OLD_ARM_ABI
	if (r_type == R_ARM_THM_XPC22
	    && ((lower_insn & 0x1800) == 0x0800))
	  /* For a BLX instruction, make sure that the relocation is rounded up
	     to a word boundary.  This follows the semantics of the instruction
	     which specifies that bit 1 of the target address will come from bit
	     1 of the base address.  */
	  relocation = (relocation + 2) & ~ 3;
#endif
	/* Put RELOCATION back into the insn.  */
	upper_insn = (upper_insn & ~(bfd_vma) 0x7ff) | ((relocation >> 12) & 0x7ff);
	lower_insn = (lower_insn & ~(bfd_vma) 0x7ff) | ((relocation >> 1) & 0x7ff);

	/* Put the relocated value back in the object file:  */
	bfd_put_16 (input_bfd, upper_insn, hit_data);
	bfd_put_16 (input_bfd, lower_insn, hit_data + 2);

	return (overflow ? bfd_reloc_overflow : bfd_reloc_ok);
      }
      break;

    case R_ARM_THM_PC11:
    case R_ARM_THM_PC9:
      /* Thumb B (branch) instruction).  */
      {
	bfd_signed_vma relocation;
	bfd_signed_vma reloc_signed_max = (1 << (howto->bitsize - 1)) - 1;
	bfd_signed_vma reloc_signed_min = ~ reloc_signed_max;
	bfd_signed_vma signed_check;

	if (globals->use_rel)
	  {
	    /* Need to refetch addend.  */
	    addend = bfd_get_16 (input_bfd, hit_data) & howto->src_mask;
	    if (addend & ((howto->src_mask + 1) >> 1))
	      {
		signed_addend = -1;
		signed_addend &= ~ howto->src_mask;
		signed_addend |= addend;
	      }
	    else
	      signed_addend = addend;
	    /* The value in the insn has been right shifted.  We need to
	       undo this, so that we can perform the address calculation
	       in terms of bytes.  */
	    signed_addend <<= howto->rightshift;
	  }
	relocation = value + signed_addend;

	relocation -= (input_section->output_section->vma
		       + input_section->output_offset
		       + rel->r_offset);

	relocation >>= howto->rightshift;
	signed_check = relocation;
	relocation &= howto->dst_mask;
	relocation |= (bfd_get_16 (input_bfd, hit_data) & (~ howto->dst_mask));

	bfd_put_16 (input_bfd, relocation, hit_data);

	/* Assumes two's complement.  */
	if (signed_check > reloc_signed_max || signed_check < reloc_signed_min)
	  return bfd_reloc_overflow;

	return bfd_reloc_ok;
      }

#ifndef OLD_ARM_ABI
    case R_ARM_ALU_PCREL7_0:
    case R_ARM_ALU_PCREL15_8:
    case R_ARM_ALU_PCREL23_15:
      {
	bfd_vma insn;
	bfd_vma relocation;

	insn = bfd_get_32 (input_bfd, hit_data);
	if (globals->use_rel)
	  {
	    /* Extract the addend.  */
	    addend = (insn & 0xff) << ((insn & 0xf00) >> 7);
	    signed_addend = addend;
	  }
	relocation = value + signed_addend;

	relocation -= (input_section->output_section->vma
		       + input_section->output_offset
		       + rel->r_offset);
	insn = (insn & ~0xfff)
	       | ((howto->bitpos << 7) & 0xf00)
	       | ((relocation >> howto->bitpos) & 0xff);
	bfd_put_32 (input_bfd, value, hit_data);
      }
      return bfd_reloc_ok;
#endif

    case R_ARM_GNU_VTINHERIT:
    case R_ARM_GNU_VTENTRY:
      return bfd_reloc_ok;

    case R_ARM_COPY:
      return bfd_reloc_notsupported;

    case R_ARM_GLOB_DAT:
      return bfd_reloc_notsupported;

    case R_ARM_JUMP_SLOT:
      return bfd_reloc_notsupported;

    case R_ARM_RELATIVE:
      return bfd_reloc_notsupported;

    case R_ARM_GOTOFF:
      /* Relocation is relative to the start of the
         global offset table.  */

      BFD_ASSERT (sgot != NULL);
      if (sgot == NULL)
        return bfd_reloc_notsupported;

      /* If we are addressing a Thumb function, we need to adjust the
	 address by one, so that attempts to call the function pointer will
	 correctly interpret it as Thumb code.  */
      if (sym_flags == STT_ARM_TFUNC)
	value += 1;

      /* Note that sgot->output_offset is not involved in this
         calculation.  We always want the start of .got.  If we
         define _GLOBAL_OFFSET_TABLE in a different way, as is
         permitted by the ABI, we might have to change this
         calculation.  */
      value -= sgot->output_section->vma;
      return _bfd_final_link_relocate (howto, input_bfd, input_section,
				       contents, rel->r_offset, value,
				       (bfd_vma) 0);

    case R_ARM_GOTPC:
      /* Use global offset table as symbol value.  */
      BFD_ASSERT (sgot != NULL);

      if (sgot == NULL)
        return bfd_reloc_notsupported;

      *unresolved_reloc_p = FALSE;
      value = sgot->output_section->vma;
      return _bfd_final_link_relocate (howto, input_bfd, input_section,
				       contents, rel->r_offset, value,
				       (bfd_vma) 0);

    case R_ARM_GOT32:
#ifndef OLD_ARM_ABI
    case R_ARM_GOT_PREL:
#endif
      /* Relocation is to the entry for this symbol in the
         global offset table.  */
      if (sgot == NULL)
	return bfd_reloc_notsupported;

      if (h != NULL)
	{
	  bfd_vma off;
	  bfd_boolean dyn;

	  off = h->got.offset;
	  BFD_ASSERT (off != (bfd_vma) -1);
	  dyn = globals->root.dynamic_sections_created;

	  if (! WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h)
	      || (info->shared
		  && SYMBOL_REFERENCES_LOCAL (info, h))
	      || (ELF_ST_VISIBILITY (h->other)
		  && h->root.type == bfd_link_hash_undefweak))
	    {
	      /* This is actually a static link, or it is a -Bsymbolic link
		 and the symbol is defined locally.  We must initialize this
		 entry in the global offset table.  Since the offset must
		 always be a multiple of 4, we use the least significant bit
		 to record whether we have initialized it already.

		 When doing a dynamic link, we create a .rel.got relocation
		 entry to initialize the value.  This is done in the
		 finish_dynamic_symbol routine.  */
	      if ((off & 1) != 0)
		off &= ~1;
	      else
		{
		  /* If we are addressing a Thumb function, we need to
		     adjust the address by one, so that attempts to
		     call the function pointer will correctly
		     interpret it as Thumb code.  */
		  if (sym_flags == STT_ARM_TFUNC)
		    value |= 1;

		  bfd_put_32 (output_bfd, value, sgot->contents + off);
		  h->got.offset |= 1;
		}
	    }
	  else
	    *unresolved_reloc_p = FALSE;

	  value = sgot->output_offset + off;
	}
      else
	{
	  bfd_vma off;

	  BFD_ASSERT (local_got_offsets != NULL &&
		      local_got_offsets[r_symndx] != (bfd_vma) -1);

	  off = local_got_offsets[r_symndx];

	  /* The offset must always be a multiple of 4.  We use the
	     least significant bit to record whether we have already
	     generated the necessary reloc.  */
	  if ((off & 1) != 0)
	    off &= ~1;
	  else
	    {
	      /* If we are addressing a Thumb function, we need to
		 adjust the address by one, so that attempts to
		 call the function pointer will correctly
		 interpret it as Thumb code.  */
	      if (sym_flags == STT_ARM_TFUNC)
		value |= 1;

	      bfd_put_32 (output_bfd, value, sgot->contents + off);

	      if (info->shared)
		{
		  asection * srelgot;
		  Elf_Internal_Rela outrel;
		  bfd_byte *loc;

		  srelgot = bfd_get_section_by_name (dynobj, ".rel.got");
		  BFD_ASSERT (srelgot != NULL);

		  outrel.r_offset = (sgot->output_section->vma
				     + sgot->output_offset
				     + off);
		  outrel.r_info = ELF32_R_INFO (0, R_ARM_RELATIVE);
		  loc = srelgot->contents;
		  loc += srelgot->reloc_count++ * sizeof (Elf32_External_Rel);
		  bfd_elf32_swap_reloc_out (output_bfd, &outrel, loc);
		}

	      local_got_offsets[r_symndx] |= 1;
	    }

	  value = sgot->output_offset + off;
	}
      if (r_type != R_ARM_GOT32)
	value += sgot->output_section->vma;

      return _bfd_final_link_relocate (howto, input_bfd, input_section,
				       contents, rel->r_offset, value,
				       (bfd_vma) 0);

    case R_ARM_SBREL32:
      return bfd_reloc_notsupported;

    case R_ARM_AMP_VCALL9:
      return bfd_reloc_notsupported;

    case R_ARM_RSBREL32:
      return bfd_reloc_notsupported;

    case R_ARM_THM_RPC22:
      return bfd_reloc_notsupported;

    case R_ARM_RREL32:
      return bfd_reloc_notsupported;

    case R_ARM_RABS32:
      return bfd_reloc_notsupported;

    case R_ARM_RPC24:
      return bfd_reloc_notsupported;

    case R_ARM_RBASE:
      return bfd_reloc_notsupported;

    case R_ARM_V4BX:
      if (globals->fix_v4bx)
        {
          bfd_vma insn = bfd_get_32 (input_bfd, hit_data);

          /* Ensure that we have a BX instruction.  */
          BFD_ASSERT ((insn & 0x0ffffff0) == 0x012fff10);

          /* Preserve Rm (lowest four bits) and the condition code
             (highest four bits). Other bits encode MOV PC,Rm.  */
          insn = (insn & 0xf000000f) | 0x01a0f000;

          bfd_put_32 (input_bfd, insn, hit_data);
        }
      return bfd_reloc_ok;

    default:
      return bfd_reloc_notsupported;
    }
}

/* Add INCREMENT to the reloc (of type HOWTO) at ADDRESS.  */
static void
arm_add_to_rel (bfd *              abfd,
		bfd_byte *         address,
		reloc_howto_type * howto,
		bfd_signed_vma     increment)
{
  bfd_signed_vma addend;

  if (howto->type == R_ARM_THM_PC22)
    {
      int upper_insn, lower_insn;
      int upper, lower;

      upper_insn = bfd_get_16 (abfd, address);
      lower_insn = bfd_get_16 (abfd, address + 2);
      upper = upper_insn & 0x7ff;
      lower = lower_insn & 0x7ff;

      addend = (upper << 12) | (lower << 1);
      addend += increment;
      addend >>= 1;

      upper_insn = (upper_insn & 0xf800) | ((addend >> 11) & 0x7ff);
      lower_insn = (lower_insn & 0xf800) | (addend & 0x7ff);

      bfd_put_16 (abfd, (bfd_vma) upper_insn, address);
      bfd_put_16 (abfd, (bfd_vma) lower_insn, address + 2);
    }
  else
    {
      bfd_vma        contents;

      contents = bfd_get_32 (abfd, address);

      /* Get the (signed) value from the instruction.  */
      addend = contents & howto->src_mask;
      if (addend & ((howto->src_mask + 1) >> 1))
	{
	  bfd_signed_vma mask;

	  mask = -1;
	  mask &= ~ howto->src_mask;
	  addend |= mask;
	}

      /* Add in the increment, (which is a byte value).  */
      switch (howto->type)
	{
	default:
	  addend += increment;
	  break;

	case R_ARM_PC24:
	case R_ARM_PLT32:
#ifndef OLD_ARM_ABI
	case R_ARM_CALL:
	case R_ARM_JUMP24:
#endif
	  addend <<= howto->size;
	  addend += increment;

	  /* Should we check for overflow here ?  */

	  /* Drop any undesired bits.  */
	  addend >>= howto->rightshift;
	  break;
	}

      contents = (contents & ~ howto->dst_mask) | (addend & howto->dst_mask);

      bfd_put_32 (abfd, contents, address);
    }
}

/* Relocate an ARM ELF section.  */
static bfd_boolean
elf32_arm_relocate_section (bfd *                  output_bfd,
			    struct bfd_link_info * info,
			    bfd *                  input_bfd,
			    asection *             input_section,
			    bfd_byte *             contents,
			    Elf_Internal_Rela *    relocs,
			    Elf_Internal_Sym *     local_syms,
			    asection **            local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;
  const char *name;
  struct elf32_arm_link_hash_table * globals;

  globals = elf32_arm_hash_table (info);
  if (info->relocatable && !globals->use_rel)
    return TRUE;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);

  rel = relocs;
  relend = relocs + input_section->reloc_count;
  for (; rel < relend; rel++)
    {
      int                          r_type;
      reloc_howto_type *           howto;
      unsigned long                r_symndx;
      Elf_Internal_Sym *           sym;
      asection *                   sec;
      struct elf_link_hash_entry * h;
      bfd_vma                      relocation;
      bfd_reloc_status_type        r;
      arelent                      bfd_reloc;
      bfd_boolean                  unresolved_reloc = FALSE;

      r_symndx = ELF32_R_SYM (rel->r_info);
      r_type   = ELF32_R_TYPE (rel->r_info);
      r_type   = arm_real_reloc_type (globals, r_type);

      if (   r_type == R_ARM_GNU_VTENTRY
          || r_type == R_ARM_GNU_VTINHERIT)
        continue;

      bfd_reloc.howto = elf32_arm_howto_from_type (r_type);
      howto = bfd_reloc.howto;

      if (info->relocatable && globals->use_rel)
	{
	  /* This is a relocatable link.  We don't have to change
	     anything, unless the reloc is against a section symbol,
	     in which case we have to adjust according to where the
	     section symbol winds up in the output section.  */
	  if (r_symndx < symtab_hdr->sh_info)
	    {
	      sym = local_syms + r_symndx;
	      if (ELF_ST_TYPE (sym->st_info) == STT_SECTION)
		{
		  sec = local_sections[r_symndx];
		  arm_add_to_rel (input_bfd, contents + rel->r_offset,
				  howto,
				  (bfd_signed_vma) (sec->output_offset
						    + sym->st_value));
		}
	    }

	  continue;
	}

      /* This is a final link.  */
      h = NULL;
      sym = NULL;
      sec = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  if (globals->use_rel)
	    {
	      relocation = (sec->output_section->vma
			    + sec->output_offset
			    + sym->st_value);
	      if ((sec->flags & SEC_MERGE)
		       && ELF_ST_TYPE (sym->st_info) == STT_SECTION)
		{
		  asection *msec;
		  bfd_vma addend, value;

		  if (howto->rightshift)
		    {
		      (*_bfd_error_handler)
			(_("%B(%A+0x%lx): %s relocation against SEC_MERGE section"),
			 input_bfd, input_section,
			 (long) rel->r_offset, howto->name);
		      return FALSE;
		    }

		  value = bfd_get_32 (input_bfd, contents + rel->r_offset);

		  /* Get the (signed) value from the instruction.  */
		  addend = value & howto->src_mask;
		  if (addend & ((howto->src_mask + 1) >> 1))
		    {
		      bfd_signed_vma mask;

		      mask = -1;
		      mask &= ~ howto->src_mask;
		      addend |= mask;
		    }
		  msec = sec;
		  addend =
		    _bfd_elf_rel_local_sym (output_bfd, sym, &msec, addend)
		    - relocation;
		  addend += msec->output_section->vma + msec->output_offset;
		  value = (value & ~ howto->dst_mask) | (addend & howto->dst_mask);
		  bfd_put_32 (input_bfd, value, contents + rel->r_offset);
		}
	    }
	  else
	    relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	}
      else
	{
	  bfd_boolean warned;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned);
	}

      if (h != NULL)
	name = h->root.root.string;
      else
	{
	  name = (bfd_elf_string_from_elf_section
		  (input_bfd, symtab_hdr->sh_link, sym->st_name));
	  if (name == NULL || *name == '\0')
	    name = bfd_section_name (input_bfd, sec);
	}

      r = elf32_arm_final_link_relocate (howto, input_bfd, output_bfd,
					 input_section, contents, rel,
					 relocation, info, sec, name,
					 (h ? ELF_ST_TYPE (h->type) :
					  ELF_ST_TYPE (sym->st_info)), h,
					 &unresolved_reloc);

      /* Dynamic relocs are not propagated for SEC_DEBUGGING sections
	 because such sections are not SEC_ALLOC and thus ld.so will
	 not process them.  */
      if (unresolved_reloc
          && !((input_section->flags & SEC_DEBUGGING) != 0
               && h->def_dynamic))
	{
	  (*_bfd_error_handler)
	    (_("%B(%A+0x%lx): warning: unresolvable relocation %d against symbol `%s'"),
	     input_bfd, input_section, (long) rel->r_offset,
	     r_type, h->root.root.string);
	  return FALSE;
	}

      if (r != bfd_reloc_ok)
	{
	  const char * msg = (const char *) 0;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      /* If the overflowing reloc was to an undefined symbol,
		 we have already printed one error message and there
		 is no point complaining again.  */
	      if ((! h ||
		   h->root.type != bfd_link_hash_undefined)
		  && (!((*info->callbacks->reloc_overflow)
			(info, (h ? &h->root : NULL), name, howto->name,
			 (bfd_vma) 0, input_bfd, input_section,
			 rel->r_offset))))
		  return FALSE;
	      break;

	    case bfd_reloc_undefined:
	      if (!((*info->callbacks->undefined_symbol)
		    (info, name, input_bfd, input_section,
		     rel->r_offset, TRUE)))
		return FALSE;
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      goto common_error;

	    case bfd_reloc_notsupported:
	      msg = _("internal error: unsupported relocation error");
	      goto common_error;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous error");
	      goto common_error;

	    default:
	      msg = _("internal error: unknown error");
	      /* fall through */

	    common_error:
	      if (!((*info->callbacks->warning)
		    (info, msg, name, input_bfd, input_section,
		     rel->r_offset)))
		return FALSE;
	      break;
	    }
	}
    }

  return TRUE;
}

/* Set the right machine number.  */

static bfd_boolean
elf32_arm_object_p (bfd *abfd)
{
  unsigned int mach;

  mach = bfd_arm_get_mach_from_notes (abfd, ARM_NOTE_SECTION);

  if (mach != bfd_mach_arm_unknown)
    bfd_default_set_arch_mach (abfd, bfd_arch_arm, mach);

  else if (elf_elfheader (abfd)->e_flags & EF_ARM_MAVERICK_FLOAT)
    bfd_default_set_arch_mach (abfd, bfd_arch_arm, bfd_mach_arm_ep9312);

  else
    bfd_default_set_arch_mach (abfd, bfd_arch_arm, mach);

  return TRUE;
}

/* Function to keep ARM specific flags in the ELF header.  */

static bfd_boolean
elf32_arm_set_private_flags (bfd *abfd, flagword flags)
{
  if (elf_flags_init (abfd)
      && elf_elfheader (abfd)->e_flags != flags)
    {
      if (EF_ARM_EABI_VERSION (flags) == EF_ARM_EABI_UNKNOWN)
	{
	  if (flags & EF_ARM_INTERWORK)
	    (*_bfd_error_handler)
	      (_("Warning: Not setting interworking flag of %B since it has already been specified as non-interworking"),
	       abfd);
	  else
	    _bfd_error_handler
	      (_("Warning: Clearing the interworking flag of %B due to outside request"),
	       abfd);
	}
    }
  else
    {
      elf_elfheader (abfd)->e_flags = flags;
      elf_flags_init (abfd) = TRUE;
    }

  return TRUE;
}

/* Copy backend specific data from one object module to another.  */

static bfd_boolean
elf32_arm_copy_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  flagword in_flags;
  flagword out_flags;

  if (   bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  in_flags  = elf_elfheader (ibfd)->e_flags;
  out_flags = elf_elfheader (obfd)->e_flags;

  if (elf_flags_init (obfd)
      && EF_ARM_EABI_VERSION (out_flags) == EF_ARM_EABI_UNKNOWN
      && in_flags != out_flags)
    {
      /* Cannot mix APCS26 and APCS32 code.  */
      if ((in_flags & EF_ARM_APCS_26) != (out_flags & EF_ARM_APCS_26))
	return FALSE;

      /* Cannot mix float APCS and non-float APCS code.  */
      if ((in_flags & EF_ARM_APCS_FLOAT) != (out_flags & EF_ARM_APCS_FLOAT))
	return FALSE;

      /* If the src and dest have different interworking flags
         then turn off the interworking bit.  */
      if ((in_flags & EF_ARM_INTERWORK) != (out_flags & EF_ARM_INTERWORK))
	{
	  if (out_flags & EF_ARM_INTERWORK)
	    _bfd_error_handler
	      (_("Warning: Clearing the interworking flag of %B because non-interworking code in %B has been linked with it"),
	       obfd, ibfd);

	  in_flags &= ~EF_ARM_INTERWORK;
	}

      /* Likewise for PIC, though don't warn for this case.  */
      if ((in_flags & EF_ARM_PIC) != (out_flags & EF_ARM_PIC))
	in_flags &= ~EF_ARM_PIC;
    }

  elf_elfheader (obfd)->e_flags = in_flags;
  elf_flags_init (obfd) = TRUE;

  /* Also copy the EI_OSABI field.  */
  elf_elfheader (obfd)->e_ident[EI_OSABI] =
    elf_elfheader (ibfd)->e_ident[EI_OSABI];

  return TRUE;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bfd_boolean
elf32_arm_merge_private_bfd_data (bfd * ibfd, bfd * obfd)
{
  flagword out_flags;
  flagword in_flags;
  bfd_boolean flags_compatible = TRUE;
  asection *sec;

  /* Check if we have the same endianess.  */
  if (! _bfd_generic_verify_endian_match (ibfd, obfd))
    return FALSE;

  if (   bfd_get_flavour (ibfd) != bfd_target_elf_flavour
      || bfd_get_flavour (obfd) != bfd_target_elf_flavour)
    return TRUE;

  /* The input BFD must have had its flags initialised.  */
  /* The following seems bogus to me -- The flags are initialized in
     the assembler but I don't think an elf_flags_init field is
     written into the object.  */
  /* BFD_ASSERT (elf_flags_init (ibfd)); */

  in_flags  = elf_elfheader (ibfd)->e_flags;
  out_flags = elf_elfheader (obfd)->e_flags;

  if (!elf_flags_init (obfd))
    {
      /* If the input is the default architecture and had the default
	 flags then do not bother setting the flags for the output
	 architecture, instead allow future merges to do this.  If no
	 future merges ever set these flags then they will retain their
         uninitialised values, which surprise surprise, correspond
         to the default values.  */
      if (bfd_get_arch_info (ibfd)->the_default
	  && elf_elfheader (ibfd)->e_flags == 0)
	return TRUE;

      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = in_flags;

      if (bfd_get_arch (obfd) == bfd_get_arch (ibfd)
	  && bfd_get_arch_info (obfd)->the_default)
	return bfd_set_arch_mach (obfd, bfd_get_arch (ibfd), bfd_get_mach (ibfd));

      return TRUE;
    }

  /* Determine what should happen if the input ARM architecture
     does not match the output ARM architecture.  */
  if (! bfd_arm_merge_machines (ibfd, obfd))
    return FALSE;

  /* Identical flags must be compatible.  */
  if (in_flags == out_flags)
    return TRUE;

  /* Check to see if the input BFD actually contains any sections.  If
     not, its flags may not have been initialised either, but it
     cannot actually cause any incompatibility.  Do not short-circuit
     dynamic objects; their section list may be emptied by
    elf_link_add_object_symbols.

    Also check to see if there are no code sections in the input.
    In this case there is no need to check for code specific flags.
    XXX - do we need to worry about floating-point format compatability
    in data sections ?  */
  if (!(ibfd->flags & DYNAMIC))
    {
      bfd_boolean null_input_bfd = TRUE;
      bfd_boolean only_data_sections = TRUE;

      for (sec = ibfd->sections; sec != NULL; sec = sec->next)
	{
	  /* Ignore synthetic glue sections.  */
	  if (strcmp (sec->name, ".glue_7")
	      && strcmp (sec->name, ".glue_7t"))
	    {
	      if ((bfd_get_section_flags (ibfd, sec)
		   & (SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS))
		  == (SEC_LOAD | SEC_CODE | SEC_HAS_CONTENTS))
	    	only_data_sections = FALSE;

	      null_input_bfd = FALSE;
	      break;
	    }
	}

      if (null_input_bfd || only_data_sections)
	return TRUE;
    }

  /* Complain about various flag mismatches.  */
  if (EF_ARM_EABI_VERSION (in_flags) != EF_ARM_EABI_VERSION (out_flags))
    {
      _bfd_error_handler
	(_("ERROR: Source object %B has EABI version %d, but target %B has EABI version %d"),
	 ibfd, obfd,
	 (in_flags & EF_ARM_EABIMASK) >> 24,
	 (out_flags & EF_ARM_EABIMASK) >> 24);
      return FALSE;
    }

  /* Not sure what needs to be checked for EABI versions >= 1.  */
  if (EF_ARM_EABI_VERSION (in_flags) == EF_ARM_EABI_UNKNOWN)
    {
      if ((in_flags & EF_ARM_APCS_26) != (out_flags & EF_ARM_APCS_26))
	{
	  _bfd_error_handler
	    (_("ERROR: %B is compiled for APCS-%d, whereas target %B uses APCS-%d"),
	     ibfd, obfd,
	     in_flags & EF_ARM_APCS_26 ? 26 : 32,
	     out_flags & EF_ARM_APCS_26 ? 26 : 32);
	  flags_compatible = FALSE;
	}

      if ((in_flags & EF_ARM_APCS_FLOAT) != (out_flags & EF_ARM_APCS_FLOAT))
	{
	  if (in_flags & EF_ARM_APCS_FLOAT)
	    _bfd_error_handler
	      (_("ERROR: %B passes floats in float registers, whereas %B passes them in integer registers"),
	       ibfd, obfd);
	  else
	    _bfd_error_handler
	      (_("ERROR: %B passes floats in integer registers, whereas %B passes them in float registers"),
	       ibfd, obfd);

	  flags_compatible = FALSE;
	}

      if ((in_flags & EF_ARM_VFP_FLOAT) != (out_flags & EF_ARM_VFP_FLOAT))
	{
	  if (in_flags & EF_ARM_VFP_FLOAT)
	    _bfd_error_handler
	      (_("ERROR: %B uses VFP instructions, whereas %B does not"),
	       ibfd, obfd);
	  else
	    _bfd_error_handler
	      (_("ERROR: %B uses FPA instructions, whereas %B does not"),
	       ibfd, obfd);

	  flags_compatible = FALSE;
	}

      if ((in_flags & EF_ARM_MAVERICK_FLOAT) != (out_flags & EF_ARM_MAVERICK_FLOAT))
	{
	  if (in_flags & EF_ARM_MAVERICK_FLOAT)
	    _bfd_error_handler
	      (_("ERROR: %B uses Maverick instructions, whereas %B does not"),
	       ibfd, obfd);
	  else
	    _bfd_error_handler
	      (_("ERROR: %B does not use Maverick instructions, whereas %B does"),
	       ibfd, obfd);

	  flags_compatible = FALSE;
	}

#ifdef EF_ARM_SOFT_FLOAT
      if ((in_flags & EF_ARM_SOFT_FLOAT) != (out_flags & EF_ARM_SOFT_FLOAT))
	{
	  /* We can allow interworking between code that is VFP format
	     layout, and uses either soft float or integer regs for
	     passing floating point arguments and results.  We already
	     know that the APCS_FLOAT flags match; similarly for VFP
	     flags.  */
	  if ((in_flags & EF_ARM_APCS_FLOAT) != 0
	      || (in_flags & EF_ARM_VFP_FLOAT) == 0)
	    {
	      if (in_flags & EF_ARM_SOFT_FLOAT)
		_bfd_error_handler
		  (_("ERROR: %B uses software FP, whereas %B uses hardware FP"),
		   ibfd, obfd);
	      else
		_bfd_error_handler
		  (_("ERROR: %B uses hardware FP, whereas %B uses software FP"),
		   ibfd, obfd);

	      flags_compatible = FALSE;
	    }
	}
#endif

      /* Interworking mismatch is only a warning.  */
      if ((in_flags & EF_ARM_INTERWORK) != (out_flags & EF_ARM_INTERWORK))
	{
	  if (in_flags & EF_ARM_INTERWORK)
	    {
	      _bfd_error_handler
		(_("Warning: %B supports interworking, whereas %B does not"),
		 ibfd, obfd);
	    }
	  else
	    {
	      _bfd_error_handler
		(_("Warning: %B does not support interworking, whereas %B does"),
		 ibfd, obfd);
	    }
	}
    }

  return flags_compatible;
}

/* Display the flags field.  */

static bfd_boolean
elf32_arm_print_private_bfd_data (bfd *abfd, void * ptr)
{
  FILE * file = (FILE *) ptr;
  unsigned long flags;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  /* Print normal ELF private data.  */
  _bfd_elf_print_private_bfd_data (abfd, ptr);

  flags = elf_elfheader (abfd)->e_flags;
  /* Ignore init flag - it may not be set, despite the flags field
     containing valid data.  */

  /* xgettext:c-format */
  fprintf (file, _("private flags = %lx:"), elf_elfheader (abfd)->e_flags);

  switch (EF_ARM_EABI_VERSION (flags))
    {
    case EF_ARM_EABI_UNKNOWN:
      /* The following flag bits are GNU extensions and not part of the
	 official ARM ELF extended ABI.  Hence they are only decoded if
	 the EABI version is not set.  */
      if (flags & EF_ARM_INTERWORK)
	fprintf (file, _(" [interworking enabled]"));

      if (flags & EF_ARM_APCS_26)
	fprintf (file, " [APCS-26]");
      else
	fprintf (file, " [APCS-32]");

      if (flags & EF_ARM_VFP_FLOAT)
	fprintf (file, _(" [VFP float format]"));
      else if (flags & EF_ARM_MAVERICK_FLOAT)
	fprintf (file, _(" [Maverick float format]"));
      else
	fprintf (file, _(" [FPA float format]"));

      if (flags & EF_ARM_APCS_FLOAT)
	fprintf (file, _(" [floats passed in float registers]"));

      if (flags & EF_ARM_PIC)
	fprintf (file, _(" [position independent]"));

      if (flags & EF_ARM_NEW_ABI)
	fprintf (file, _(" [new ABI]"));

      if (flags & EF_ARM_OLD_ABI)
	fprintf (file, _(" [old ABI]"));

      if (flags & EF_ARM_SOFT_FLOAT)
	fprintf (file, _(" [software FP]"));

      flags &= ~(EF_ARM_INTERWORK | EF_ARM_APCS_26 | EF_ARM_APCS_FLOAT
		 | EF_ARM_PIC | EF_ARM_NEW_ABI | EF_ARM_OLD_ABI
		 | EF_ARM_SOFT_FLOAT | EF_ARM_VFP_FLOAT
		 | EF_ARM_MAVERICK_FLOAT);
      break;

    case EF_ARM_EABI_VER1:
      fprintf (file, _(" [Version1 EABI]"));

      if (flags & EF_ARM_SYMSARESORTED)
	fprintf (file, _(" [sorted symbol table]"));
      else
	fprintf (file, _(" [unsorted symbol table]"));

      flags &= ~ EF_ARM_SYMSARESORTED;
      break;

    case EF_ARM_EABI_VER2:
      fprintf (file, _(" [Version2 EABI]"));

      if (flags & EF_ARM_SYMSARESORTED)
	fprintf (file, _(" [sorted symbol table]"));
      else
	fprintf (file, _(" [unsorted symbol table]"));

      if (flags & EF_ARM_DYNSYMSUSESEGIDX)
	fprintf (file, _(" [dynamic symbols use segment index]"));

      if (flags & EF_ARM_MAPSYMSFIRST)
	fprintf (file, _(" [mapping symbols precede others]"));

      flags &= ~(EF_ARM_SYMSARESORTED | EF_ARM_DYNSYMSUSESEGIDX
		 | EF_ARM_MAPSYMSFIRST);
      break;

    case EF_ARM_EABI_VER3:
      fprintf (file, _(" [Version3 EABI]"));
      break;

    case EF_ARM_EABI_VER4:
      fprintf (file, _(" [Version4 EABI]"));

      if (flags & EF_ARM_BE8)
	fprintf (file, _(" [BE8]"));

      if (flags & EF_ARM_LE8)
	fprintf (file, _(" [LE8]"));

      flags &= ~(EF_ARM_LE8 | EF_ARM_BE8);
      break;

    default:
      fprintf (file, _(" <EABI version unrecognised>"));
      break;
    }

  flags &= ~ EF_ARM_EABIMASK;

  if (flags & EF_ARM_RELEXEC)
    fprintf (file, _(" [relocatable executable]"));

  if (flags & EF_ARM_HASENTRY)
    fprintf (file, _(" [has entry point]"));

  flags &= ~ (EF_ARM_RELEXEC | EF_ARM_HASENTRY);

  if (flags)
    fprintf (file, _("<Unrecognised flag bits set>"));

  fputc ('\n', file);

  return TRUE;
}

static int
elf32_arm_get_symbol_type (Elf_Internal_Sym * elf_sym, int type)
{
  switch (ELF_ST_TYPE (elf_sym->st_info))
    {
    case STT_ARM_TFUNC:
      return ELF_ST_TYPE (elf_sym->st_info);

    case STT_ARM_16BIT:
      /* If the symbol is not an object, return the STT_ARM_16BIT flag.
	 This allows us to distinguish between data used by Thumb instructions
	 and non-data (which is probably code) inside Thumb regions of an
	 executable.  */
      if (type != STT_OBJECT)
	return ELF_ST_TYPE (elf_sym->st_info);
      break;

    default:
      break;
    }

  return type;
}

static asection *
elf32_arm_gc_mark_hook (asection *                   sec,
			struct bfd_link_info *       info ATTRIBUTE_UNUSED,
			Elf_Internal_Rela *          rel,
			struct elf_link_hash_entry * h,
			Elf_Internal_Sym *           sym)
{
  if (h != NULL)
    {
      switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_ARM_GNU_VTINHERIT:
      case R_ARM_GNU_VTENTRY:
        break;

      default:
        switch (h->root.type)
          {
          case bfd_link_hash_defined:
          case bfd_link_hash_defweak:
            return h->root.u.def.section;

          case bfd_link_hash_common:
            return h->root.u.c.p->section;

	  default:
	    break;
          }
       }
     }
   else
     return bfd_section_from_elf_index (sec->owner, sym->st_shndx);

  return NULL;
}

/* Update the got entry reference counts for the section being removed.  */

static bfd_boolean
elf32_arm_gc_sweep_hook (bfd *                     abfd ATTRIBUTE_UNUSED,
			 struct bfd_link_info *    info ATTRIBUTE_UNUSED,
			 asection *                sec ATTRIBUTE_UNUSED,
			 const Elf_Internal_Rela * relocs ATTRIBUTE_UNUSED)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_signed_vma *local_got_refcounts;
  const Elf_Internal_Rela *rel, *relend;
  struct elf32_arm_link_hash_table * globals;

  globals = elf32_arm_hash_table (info);

  elf_section_data (sec)->local_dynrel = NULL;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  local_got_refcounts = elf_local_got_refcounts (abfd);

  relend = relocs + sec->reloc_count;
  for (rel = relocs; rel < relend; rel++)
    {
      unsigned long r_symndx;
      struct elf_link_hash_entry *h = NULL;
      int r_type;

      r_symndx = ELF32_R_SYM (rel->r_info);
      if (r_symndx >= symtab_hdr->sh_info)
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      r_type = ELF32_R_TYPE (rel->r_info);
#ifndef OLD_ARM_ABI
      r_type = arm_real_reloc_type (globals, r_type);
#endif
      switch (r_type)
	{
	case R_ARM_GOT32:
#ifndef OLD_ARM_ABI
	case R_ARM_GOT_PREL:
#endif
	  if (h != NULL)
	    {
	      if (h->got.refcount > 0)
		h->got.refcount -= 1;
	    }
	  else if (local_got_refcounts != NULL)
	    {
	      if (local_got_refcounts[r_symndx] > 0)
		local_got_refcounts[r_symndx] -= 1;
	    }
	  break;

	case R_ARM_ABS32:
	case R_ARM_REL32:
	case R_ARM_PC24:
	case R_ARM_PLT32:
#ifndef OLD_ARM_ABI
	case R_ARM_CALL:
	case R_ARM_JUMP24:
	case R_ARM_PREL31:
#endif
	case R_ARM_THM_PC22:
	  /* Should the interworking branches be here also?  */

	  if (h != NULL)
	    {
	      struct elf32_arm_link_hash_entry *eh;
	      struct elf32_arm_relocs_copied **pp;
	      struct elf32_arm_relocs_copied *p;

	      eh = (struct elf32_arm_link_hash_entry *) h;

	      if (h->plt.refcount > 0)
		{
		  h->plt.refcount -= 1;
		  if (ELF32_R_TYPE (rel->r_info) == R_ARM_THM_PC22)
		    eh->plt_thumb_refcount--;
		}

	      if (r_type == R_ARM_ABS32
		  || r_type == R_ARM_REL32)
		{
		  for (pp = &eh->relocs_copied; (p = *pp) != NULL;
		       pp = &p->next)
		  if (p->section == sec)
		    {
		      p->count -= 1;
		      if (p->count == 0)
			*pp = p->next;
		      break;
		    }
		}
	    }
	  break;

	default:
	  break;
	}
    }

  return TRUE;
}

/* Look through the relocs for a section during the first phase.  */

static bfd_boolean
elf32_arm_check_relocs (bfd *abfd, struct bfd_link_info *info,
			asection *sec, const Elf_Internal_Rela *relocs)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  struct elf_link_hash_entry **sym_hashes_end;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  bfd *dynobj;
  asection *sreloc;
  bfd_vma *local_got_offsets;
  struct elf32_arm_link_hash_table *htab;

  if (info->relocatable)
    return TRUE;

  htab = elf32_arm_hash_table (info);
  sreloc = NULL;

  /* Create dynamic sections for relocatable executables so that we can
     copy relocations.  */
  if (htab->root.is_relocatable_executable
      && ! htab->root.dynamic_sections_created)
    {
      if (! _bfd_elf_link_create_dynamic_sections (abfd, info))
	return FALSE;
    }

  dynobj = elf_hash_table (info)->dynobj;
  local_got_offsets = elf_local_got_offsets (abfd);

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  sym_hashes_end = sym_hashes
    + symtab_hdr->sh_size / sizeof (Elf32_External_Sym);

  if (!elf_bad_symtab (abfd))
    sym_hashes_end -= symtab_hdr->sh_info;

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      struct elf_link_hash_entry *h;
      struct elf32_arm_link_hash_entry *eh;
      unsigned long r_symndx;
      int r_type;

      r_symndx = ELF32_R_SYM (rel->r_info);
      r_type = ELF32_R_TYPE (rel->r_info);
#ifndef OLD_ARM_ABI
      r_type = arm_real_reloc_type (htab, r_type);
#endif
      if (r_symndx < symtab_hdr->sh_info)
        h = NULL;
      else
        h = sym_hashes[r_symndx - symtab_hdr->sh_info];

      eh = (struct elf32_arm_link_hash_entry *) h;

      switch (r_type)
        {
	  case R_ARM_GOT32:
#ifndef OLD_ARM_ABI
	  case R_ARM_GOT_PREL:
#endif
	    /* This symbol requires a global offset table entry.  */
	    if (h != NULL)
	      {
		h->got.refcount++;
	      }
	    else
	      {
		bfd_signed_vma *local_got_refcounts;

		/* This is a global offset table entry for a local symbol.  */
		local_got_refcounts = elf_local_got_refcounts (abfd);
		if (local_got_refcounts == NULL)
		  {
		    bfd_size_type size;

		    size = symtab_hdr->sh_info;
		    size *= (sizeof (bfd_signed_vma) + sizeof (char));
		    local_got_refcounts = bfd_zalloc (abfd, size);
		    if (local_got_refcounts == NULL)
		      return FALSE;
		    elf_local_got_refcounts (abfd) = local_got_refcounts;
		  }
		local_got_refcounts[r_symndx] += 1;
	      }
	    if (r_type == R_ARM_GOT32)
	      break;
	    /* Fall through.  */

	  case R_ARM_GOTOFF:
	  case R_ARM_GOTPC:
	    if (htab->sgot == NULL)
	      {
		if (htab->root.dynobj == NULL)
		  htab->root.dynobj = abfd;
		if (!create_got_section (htab->root.dynobj, info))
		  return FALSE;
	      }
	    break;

	  case R_ARM_ABS32:
	  case R_ARM_REL32:
	  case R_ARM_PC24:
	  case R_ARM_PLT32:
#ifndef OLD_ARM_ABI
	  case R_ARM_CALL:
	  case R_ARM_JUMP24:
	  case R_ARM_PREL31:
#endif
	  case R_ARM_THM_PC22:
	    /* Should the interworking branches be listed here?  */
	    if (h != NULL)
	      {
		/* If this reloc is in a read-only section, we might
		   need a copy reloc.  We can't check reliably at this
		   stage whether the section is read-only, as input
		   sections have not yet been mapped to output sections.
		   Tentatively set the flag for now, and correct in
		   adjust_dynamic_symbol.  */
		if (!info->shared)
		  h->non_got_ref = 1;

		/* We may need a .plt entry if the function this reloc
		   refers to is in a different object.  We can't tell for
		   sure yet, because something later might force the
		   symbol local.  */
		if (r_type == R_ARM_PC24
#ifndef OLD_ARM_ABI
		    || r_type == R_ARM_CALL
		    || r_type == R_ARM_JUMP24
		    || r_type == R_ARM_PREL31
#endif
		    || r_type == R_ARM_PLT32
		    || r_type == R_ARM_THM_PC22)
		  h->needs_plt = 1;

		/* If we create a PLT entry, this relocation will reference
		   it, even if it's an ABS32 relocation.  */
		h->plt.refcount += 1;

		if (r_type == R_ARM_THM_PC22)
		  eh->plt_thumb_refcount += 1;
	      }

	    /* If we are creating a shared library or relocatable executable,
	       and this is a reloc against a global symbol, or a non PC
	       relative reloc against a local symbol, then we need to copy
	       the reloc into the shared library.  However, if we are linking
	       with -Bsymbolic, we do not need to copy a reloc against a
               global symbol which is defined in an object we are
               including in the link (i.e., DEF_REGULAR is set).  At
               this point we have not seen all the input files, so it is
               possible that DEF_REGULAR is not set now but will be set
               later (it is never cleared).  We account for that
               possibility below by storing information in the
               relocs_copied field of the hash table entry.  */
	    if ((info->shared || htab->root.is_relocatable_executable)
		&& (sec->flags & SEC_ALLOC) != 0
		&& (r_type == R_ARM_ABS32
		    || (h != NULL && ! h->needs_plt
			&& (! info->symbolic || ! h->def_regular))))
	      {
		struct elf32_arm_relocs_copied *p, **head;

	        /* When creating a shared object, we must copy these
                   reloc types into the output file.  We create a reloc
                   section in dynobj and make room for this reloc.  */
	        if (sreloc == NULL)
		  {
		    const char * name;

		    name = (bfd_elf_string_from_elf_section
			    (abfd,
			     elf_elfheader (abfd)->e_shstrndx,
			     elf_section_data (sec)->rel_hdr.sh_name));
		    if (name == NULL)
		      return FALSE;

		    BFD_ASSERT (strncmp (name, ".rel", 4) == 0
			        && strcmp (bfd_get_section_name (abfd, sec),
					   name + 4) == 0);

		    sreloc = bfd_get_section_by_name (dynobj, name);
		    if (sreloc == NULL)
		      {
		        flagword flags;

		        sreloc = bfd_make_section (dynobj, name);
		        flags = (SEC_HAS_CONTENTS | SEC_READONLY
			         | SEC_IN_MEMORY | SEC_LINKER_CREATED);
		        if ((sec->flags & SEC_ALLOC) != 0
			    /* BPABI objects never have dynamic
			       relocations mapped.  */
			    && !htab->symbian_p)
			  flags |= SEC_ALLOC | SEC_LOAD;
		        if (sreloc == NULL
			    || ! bfd_set_section_flags (dynobj, sreloc, flags)
			    || ! bfd_set_section_alignment (dynobj, sreloc, 2))
			  return FALSE;
		      }

		    elf_section_data (sec)->sreloc = sreloc;
		  }

		/* If this is a global symbol, we count the number of
		   relocations we need for this symbol.  */
		if (h != NULL)
		  {
		    head = &((struct elf32_arm_link_hash_entry *) h)->relocs_copied;
		  }
		else
		  {
		    /* Track dynamic relocs needed for local syms too.
		       We really need local syms available to do this
		       easily.  Oh well.  */

		    asection *s;
		    s = bfd_section_from_r_symndx (abfd, &htab->sym_sec,
						   sec, r_symndx);
		    if (s == NULL)
		      return FALSE;

		    head = ((struct elf32_arm_relocs_copied **)
			    &elf_section_data (s)->local_dynrel);
		  }

		p = *head;
		if (p == NULL || p->section != sec)
		  {
		    bfd_size_type amt = sizeof *p;

		    p = bfd_alloc (htab->root.dynobj, amt);
		    if (p == NULL)
		      return FALSE;
		    p->next = *head;
		    *head = p;
		    p->section = sec;
		    p->count = 0;
		  }

		p->count += 1;
	      }
	    break;

        /* This relocation describes the C++ object vtable hierarchy.
           Reconstruct it for later use during GC.  */
        case R_ARM_GNU_VTINHERIT:
          if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
            return FALSE;
          break;

        /* This relocation describes which C++ vtable entries are actually
           used.  Record for later use during GC.  */
        case R_ARM_GNU_VTENTRY:
          if (!bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_offset))
            return FALSE;
          break;
        }
    }

  return TRUE;
}

static bfd_boolean
is_arm_mapping_symbol_name (const char * name)
{
  return (name != NULL)
    && (name[0] == '$')
    && ((name[1] == 'a') || (name[1] == 't') || (name[1] == 'd'))
    && (name[2] == 0);
}

/* Treat mapping symbols as special target symbols.  */

static bfd_boolean
elf32_arm_is_target_special_symbol (bfd * abfd ATTRIBUTE_UNUSED, asymbol * sym)
{
  return is_arm_mapping_symbol_name (sym->name);
}

/* This is a copy of elf_find_function() from elf.c except that
   ARM mapping symbols are ignored when looking for function names
   and STT_ARM_TFUNC is considered to a function type.  */

static bfd_boolean
arm_elf_find_function (bfd *         abfd ATTRIBUTE_UNUSED,
		       asection *    section,
		       asymbol **    symbols,
		       bfd_vma       offset,
		       const char ** filename_ptr,
		       const char ** functionname_ptr)
{
  const char * filename = NULL;
  asymbol * func = NULL;
  bfd_vma low_func = 0;
  asymbol ** p;

  for (p = symbols; *p != NULL; p++)
    {
      elf_symbol_type *q;

      q = (elf_symbol_type *) *p;

      switch (ELF_ST_TYPE (q->internal_elf_sym.st_info))
	{
	default:
	  break;
	case STT_FILE:
	  filename = bfd_asymbol_name (&q->symbol);
	  break;
	case STT_FUNC:
	case STT_ARM_TFUNC:
	  /* Skip $a and $t symbols.  */
	  if ((q->symbol.flags & BSF_LOCAL)
	      && is_arm_mapping_symbol_name (q->symbol.name))
	    continue;
	  /* Fall through.  */
	case STT_NOTYPE:
	  if (bfd_get_section (&q->symbol) == section
	      && q->symbol.value >= low_func
	      && q->symbol.value <= offset)
	    {
	      func = (asymbol *) q;
	      low_func = q->symbol.value;
	    }
	  break;
	}
    }

  if (func == NULL)
    return FALSE;

  if (filename_ptr)
    *filename_ptr = filename;
  if (functionname_ptr)
    *functionname_ptr = bfd_asymbol_name (func);

  return TRUE;
}  


/* Find the nearest line to a particular section and offset, for error
   reporting.   This code is a duplicate of the code in elf.c, except
   that it uses arm_elf_find_function.  */

static bfd_boolean
elf32_arm_find_nearest_line (bfd *          abfd,
			     asection *     section,
			     asymbol **     symbols,
			     bfd_vma        offset,
			     const char **  filename_ptr,
			     const char **  functionname_ptr,
			     unsigned int * line_ptr)
{
  bfd_boolean found = FALSE;

  /* We skip _bfd_dwarf1_find_nearest_line since no known ARM toolchain uses it.  */

  if (_bfd_dwarf2_find_nearest_line (abfd, section, symbols, offset,
				     filename_ptr, functionname_ptr,
				     line_ptr, 0,
				     & elf_tdata (abfd)->dwarf2_find_line_info))
    {
      if (!*functionname_ptr)
	arm_elf_find_function (abfd, section, symbols, offset,
			       *filename_ptr ? NULL : filename_ptr,
			       functionname_ptr);

      return TRUE;
    }

  if (! _bfd_stab_section_find_nearest_line (abfd, symbols, section, offset,
					     & found, filename_ptr,
					     functionname_ptr, line_ptr,
					     & elf_tdata (abfd)->line_info))
    return FALSE;

  if (found && (*functionname_ptr || *line_ptr))
    return TRUE;

  if (symbols == NULL)
    return FALSE;

  if (! arm_elf_find_function (abfd, section, symbols, offset,
			       filename_ptr, functionname_ptr))
    return FALSE;

  *line_ptr = 0;
  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

static bfd_boolean
elf32_arm_adjust_dynamic_symbol (struct bfd_link_info * info,
				 struct elf_link_hash_entry * h)
{
  bfd * dynobj;
  asection * s;
  unsigned int power_of_two;
  struct elf32_arm_link_hash_entry * eh;
  struct elf32_arm_link_hash_table *globals;

  globals = elf32_arm_hash_table (info);
  dynobj = elf_hash_table (info)->dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->u.weakdef != NULL
		  || (h->def_dynamic
		      && h->ref_regular
		      && !h->def_regular)));

  eh = (struct elf32_arm_link_hash_entry *) h;

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC || h->type == STT_ARM_TFUNC
      || h->needs_plt)
    {
      if (h->plt.refcount <= 0
	  || SYMBOL_CALLS_LOCAL (info, h)
	  || (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      && h->root.type == bfd_link_hash_undefweak))
	{
	  /* This case can occur if we saw a PLT32 reloc in an input
	     file, but the symbol was never referred to by a dynamic
	     object, or if all references were garbage collected.  In
	     such a case, we don't actually need to build a procedure
	     linkage table, and we can just do a PC24 reloc instead.  */
	  h->plt.offset = (bfd_vma) -1;
	  eh->plt_thumb_refcount = 0;
	  h->needs_plt = 0;
	}

      return TRUE;
    }
  else
    {
      /* It's possible that we incorrectly decided a .plt reloc was
	 needed for an R_ARM_PC24 or similar reloc to a non-function sym
	 in check_relocs.  We can't decide accurately between function
	 and non-function syms in check-relocs; Objects loaded later in
	 the link may change h->type.  So fix it now.  */
      h->plt.offset = (bfd_vma) -1;
      eh->plt_thumb_refcount = 0;
    }

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  Relocatable executables
     can reference data in shared objects directly, so we don't need to
     do anything here.  */
  if (info->shared || globals->root.is_relocatable_executable)
    return TRUE;

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */
  s = bfd_get_section_by_name (dynobj, ".dynbss");
  BFD_ASSERT (s != NULL);

  /* We must generate a R_ARM_COPY reloc to tell the dynamic linker to
     copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rel.bss section we are going to use.  */
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0)
    {
      asection *srel;

      srel = bfd_get_section_by_name (dynobj, ".rel.bss");
      BFD_ASSERT (srel != NULL);
      srel->size += sizeof (Elf32_External_Rel);
      h->needs_copy = 1;
    }

  /* We need to figure out the alignment required for this symbol.  I
     have no idea how ELF linkers handle this.  */
  power_of_two = bfd_log2 (h->size);
  if (power_of_two > 3)
    power_of_two = 3;

  /* Apply the required alignment.  */
  s->size = BFD_ALIGN (s->size, (bfd_size_type) (1 << power_of_two));
  if (power_of_two > bfd_get_section_alignment (dynobj, s))
    {
      if (! bfd_set_section_alignment (dynobj, s, power_of_two))
	return FALSE;
    }

  /* Define the symbol as being at this point in the section.  */
  h->root.u.def.section = s;
  h->root.u.def.value = s->size;

  /* Increment the section size to make room for the symbol.  */
  s->size += h->size;

  return TRUE;
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs.  */

static bfd_boolean
allocate_dynrelocs (struct elf_link_hash_entry *h, void * inf)
{
  struct bfd_link_info *info;
  struct elf32_arm_link_hash_table *htab;
  struct elf32_arm_link_hash_entry *eh;
  struct elf32_arm_relocs_copied *p;

  eh = (struct elf32_arm_link_hash_entry *) h;

  if (h->root.type == bfd_link_hash_indirect)
    return TRUE;

  if (h->root.type == bfd_link_hash_warning)
    /* When warning symbols are created, they **replace** the "real"
       entry in the hash table, thus we never get to see the real
       symbol in a hash traversal.  So look at it now.  */
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  info = (struct bfd_link_info *) inf;
  htab = elf32_arm_hash_table (info);

  if (htab->root.dynamic_sections_created
      && h->plt.refcount > 0)
    {
      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      if (info->shared
	  || WILL_CALL_FINISH_DYNAMIC_SYMBOL (1, 0, h))
	{
	  asection *s = htab->splt;

	  /* If this is the first .plt entry, make room for the special
	     first entry.  */
	  if (s->size == 0)
	    s->size += htab->plt_header_size;

	  h->plt.offset = s->size;

	  /* If we will insert a Thumb trampoline before this PLT, leave room
	     for it.  */
	  if (!htab->symbian_p && eh->plt_thumb_refcount > 0)
	    {
	      h->plt.offset += PLT_THUMB_STUB_SIZE;
	      s->size += PLT_THUMB_STUB_SIZE;
	    }

	  /* If this symbol is not defined in a regular file, and we are
	     not generating a shared library, then set the symbol to this
	     location in the .plt.  This is required to make function
	     pointers compare as equal between the normal executable and
	     the shared library.  */
	  if (! info->shared
	      && !h->def_regular)
	    {
	      h->root.u.def.section = s;
	      h->root.u.def.value = h->plt.offset;

	      /* Make sure the function is not marked as Thumb, in case
		 it is the target of an ABS32 relocation, which will
		 point to the PLT entry.  */
	      if (ELF_ST_TYPE (h->type) == STT_ARM_TFUNC)
		h->type = ELF_ST_INFO (ELF_ST_BIND (h->type), STT_FUNC);
	    }

	  /* Make room for this entry.  */
	  s->size += htab->plt_entry_size;

	  if (!htab->symbian_p)
	    {
	      /* We also need to make an entry in the .got.plt section, which
		 will be placed in the .got section by the linker script.  */
	      eh->plt_got_offset = htab->sgotplt->size;
	      htab->sgotplt->size += 4;
	    }

	  /* We also need to make an entry in the .rel.plt section.  */
	  htab->srelplt->size += sizeof (Elf32_External_Rel);
	}
      else
	{
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
    }
  else
    {
      h->plt.offset = (bfd_vma) -1;
      h->needs_plt = 0;
    }

  if (h->got.refcount > 0)
    {
      asection *s;
      bfd_boolean dyn;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      if (!htab->symbian_p)
	{
	  s = htab->sgot;
	  h->got.offset = s->size;
	  s->size += 4;
	  dyn = htab->root.dynamic_sections_created;
	  if ((ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	       || h->root.type != bfd_link_hash_undefweak)
	      && (info->shared
		  || WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, 0, h)))
	    htab->srelgot->size += sizeof (Elf32_External_Rel);
	}
    }
  else
    h->got.offset = (bfd_vma) -1;

  if (eh->relocs_copied == NULL)
    return TRUE;

  /* In the shared -Bsymbolic case, discard space allocated for
     dynamic pc-relative relocs against symbols which turn out to be
     defined in regular objects.  For the normal shared case, discard
     space for pc-relative relocs that have become local due to symbol
     visibility changes.  */

  if (info->shared || htab->root.is_relocatable_executable)
    {
      /* Discard relocs on undefined weak syms with non-default
         visibility.  */
      if (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	  && h->root.type == bfd_link_hash_undefweak)
	eh->relocs_copied = NULL;
      else if (htab->root.is_relocatable_executable && h->dynindx == -1
	       && h->root.type == bfd_link_hash_new)
	{
	  /* Output absolute symbols so that we can create relocations
	     against them.  For normal symbols we output a relocation
	     against the section that contains them.  */
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

    }
  else
    {
      /* For the non-shared case, discard space for relocs against
	 symbols which turn out to need copy relocs or are not
	 dynamic.  */

      if (!h->non_got_ref
	  && ((h->def_dynamic
	       && !h->def_regular)
	      || (htab->root.dynamic_sections_created
		  && (h->root.type == bfd_link_hash_undefweak
		      || h->root.type == bfd_link_hash_undefined))))
	{
	  /* Make sure this symbol is output as a dynamic symbol.
	     Undefined weak syms won't yet be marked as dynamic.  */
	  if (h->dynindx == -1
	      && !h->forced_local)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }

	  /* If that succeeded, we know we'll be keeping all the
	     relocs.  */
	  if (h->dynindx != -1)
	    goto keep;
	}

      eh->relocs_copied = NULL;

    keep: ;
    }

  /* Finally, allocate space.  */
  for (p = eh->relocs_copied; p != NULL; p = p->next)
    {
      asection *sreloc = elf_section_data (p->section)->sreloc;
      sreloc->size += p->count * sizeof (Elf32_External_Rel);
    }

  return TRUE;
}

/* Find any dynamic relocs that apply to read-only sections.  */

static bfd_boolean
elf32_arm_readonly_dynrelocs (struct elf_link_hash_entry *h, PTR inf)
{
  struct elf32_arm_link_hash_entry *eh;
  struct elf32_arm_relocs_copied *p;

  if (h->root.type == bfd_link_hash_warning)
    h = (struct elf_link_hash_entry *) h->root.u.i.link;

  eh = (struct elf32_arm_link_hash_entry *) h;
  for (p = eh->relocs_copied; p != NULL; p = p->next)
    {
      asection *s = p->section;

      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	{
	  struct bfd_link_info *info = (struct bfd_link_info *) inf;

	  info->flags |= DF_TEXTREL;

	  /* Not an error, just cut short the traversal.  */
	  return FALSE;
	}
    }
  return TRUE;
}

/* Set the sizes of the dynamic sections.  */

static bfd_boolean
elf32_arm_size_dynamic_sections (bfd * output_bfd ATTRIBUTE_UNUSED,
				 struct bfd_link_info * info)
{
  bfd * dynobj;
  asection * s;
  bfd_boolean plt;
  bfd_boolean relocs;
  bfd *ibfd;
  struct elf32_arm_link_hash_table *htab;

  htab = elf32_arm_hash_table (info);
  dynobj = elf_hash_table (info)->dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (info->executable)
	{
	  s = bfd_get_section_by_name (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size = sizeof ELF_DYNAMIC_INTERPRETER;
	  s->contents = (unsigned char *) ELF_DYNAMIC_INTERPRETER;
	}
    }

  /* Set up .got offsets for local syms, and space for local dynamic
     relocs.  */
  for (ibfd = info->input_bfds; ibfd != NULL; ibfd = ibfd->link_next)
    {
      bfd_signed_vma *local_got;
      bfd_signed_vma *end_local_got;
      char *local_tls_type;
      bfd_size_type locsymcount;
      Elf_Internal_Shdr *symtab_hdr;
      asection *srel;

      if (bfd_get_flavour (ibfd) != bfd_target_elf_flavour)
	continue;

      for (s = ibfd->sections; s != NULL; s = s->next)
	{
	  struct elf32_arm_relocs_copied *p;

	  for (p = *((struct elf32_arm_relocs_copied **)
		     &elf_section_data (s)->local_dynrel);
	       p != NULL;
	       p = p->next)
	    {
	      if (!bfd_is_abs_section (p->section)
		  && bfd_is_abs_section (p->section->output_section))
		{
		  /* Input section has been discarded, either because
		     it is a copy of a linkonce section or due to
		     linker script /DISCARD/, so we'll be discarding
		     the relocs too.  */
		}
	      else if (p->count != 0)
		{
		  srel = elf_section_data (p->section)->sreloc;
		  srel->size += p->count * sizeof (Elf32_External_Rel);
		  if ((p->section->output_section->flags & SEC_READONLY) != 0)
		    info->flags |= DF_TEXTREL;
		}
	    }
	}

      local_got = elf_local_got_refcounts (ibfd);
      if (!local_got)
	continue;

      symtab_hdr = &elf_tdata (ibfd)->symtab_hdr;
      locsymcount = symtab_hdr->sh_info;
      end_local_got = local_got + locsymcount;
      s = htab->sgot;
      srel = htab->srelgot;
      for (; local_got < end_local_got; ++local_got, ++local_tls_type)
	{
	  if (*local_got > 0)
	    {
	      *local_got = s->size;
	      s->size += 4;
	      if (info->shared)
		srel->size += sizeof (Elf32_External_Rel);
	    }
	  else
	    *local_got = (bfd_vma) -1;
	}
    }

  /* Allocate global sym .plt and .got entries, and space for global
     sym dynamic relocs.  */
  elf_link_hash_traverse (& htab->root, allocate_dynrelocs, info);

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  plt = FALSE;
  relocs = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      const char * name;
      bfd_boolean strip;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      /* It's OK to base decisions on the section name, because none
	 of the dynobj section names depend upon the input files.  */
      name = bfd_get_section_name (dynobj, s);

      strip = FALSE;

      if (strcmp (name, ".plt") == 0)
	{
	  if (s->size == 0)
	    {
	      /* Strip this section if we don't need it; see the
                 comment below.  */
	      strip = TRUE;
	    }
	  else
	    {
	      /* Remember whether there is a PLT.  */
	      plt = TRUE;
	    }
	}
      else if (strncmp (name, ".rel", 4) == 0)
	{
	  if (s->size == 0)
	    {
	      /* If we don't need this section, strip it from the
		 output file.  This is mostly to handle .rel.bss and
		 .rel.plt.  We must create both sections in
		 create_dynamic_sections, because they must be created
		 before the linker maps input sections to output
		 sections.  The linker does that before
		 adjust_dynamic_symbol is called, and it is that
		 function which decides whether anything needs to go
		 into these sections.  */
	      strip = TRUE;
	    }
	  else
	    {
	      /* Remember whether there are any reloc sections other
                 than .rel.plt.  */
	      if (strcmp (name, ".rel.plt") != 0)
		relocs = TRUE;

	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else if (strncmp (name, ".got", 4) != 0)
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

      if (strip)
	{
	  _bfd_strip_section_from_output (info, s);
	  continue;
	}

      /* Allocate memory for the section contents.  */
      s->contents = (bfd_byte *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL && s->size != 0)
	return FALSE;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in elf32_arm_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  The DT_DEBUG entry is filled in by the
	 dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (!info->shared)
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}

      if (plt)
	{
	  if (   !add_dynamic_entry (DT_PLTGOT, 0)
	      || !add_dynamic_entry (DT_PLTRELSZ, 0)
	      || !add_dynamic_entry (DT_PLTREL, DT_REL)
	      || !add_dynamic_entry (DT_JMPREL, 0))
	    return FALSE;
	}

      if (relocs)
	{
	  if (   !add_dynamic_entry (DT_REL, 0)
	      || !add_dynamic_entry (DT_RELSZ, 0)
	      || !add_dynamic_entry (DT_RELENT, sizeof (Elf32_External_Rel)))
	    return FALSE;
	}

      /* If any dynamic relocs apply to a read-only section,
	 then we need a DT_TEXTREL entry.  */
      if ((info->flags & DF_TEXTREL) == 0)
	elf_link_hash_traverse (&htab->root, elf32_arm_readonly_dynrelocs,
				(PTR) info);

      if ((info->flags & DF_TEXTREL) != 0)
	{
	  if (!add_dynamic_entry (DT_TEXTREL, 0))
	    return FALSE;
	  info->flags |= DF_TEXTREL;
	}
    }
#undef add_synamic_entry

  return TRUE;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bfd_boolean
elf32_arm_finish_dynamic_symbol (bfd * output_bfd, struct bfd_link_info * info,
				 struct elf_link_hash_entry * h, Elf_Internal_Sym * sym)
{
  bfd * dynobj;
  struct elf32_arm_link_hash_table *htab;
  struct elf32_arm_link_hash_entry *eh;

  dynobj = elf_hash_table (info)->dynobj;
  htab = elf32_arm_hash_table (info);
  eh = (struct elf32_arm_link_hash_entry *) h;

  if (h->plt.offset != (bfd_vma) -1)
    {
      asection * splt;
      asection * srel;
      bfd_byte *loc;
      bfd_vma plt_index;
      Elf_Internal_Rela rel;

      /* This symbol has an entry in the procedure linkage table.  Set
	 it up.  */

      BFD_ASSERT (h->dynindx != -1);

      splt = bfd_get_section_by_name (dynobj, ".plt");
      srel = bfd_get_section_by_name (dynobj, ".rel.plt");
      BFD_ASSERT (splt != NULL && srel != NULL);

      /* Fill in the entry in the procedure linkage table.  */
      if (htab->symbian_p)
	{
	  unsigned i;
	  for (i = 0; i < htab->plt_entry_size / 4; ++i)
	    bfd_put_32 (output_bfd, 
			elf32_arm_symbian_plt_entry[i],
			splt->contents + h->plt.offset + 4 * i);
	  
	  /* Fill in the entry in the .rel.plt section.  */
	  rel.r_offset = (splt->output_section->vma
			  + splt->output_offset
			  + h->plt.offset + 4 * (i - 1));
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_ARM_GLOB_DAT);

	  /* Get the index in the procedure linkage table which
	     corresponds to this symbol.  This is the index of this symbol
	     in all the symbols for which we are making plt entries.  The
	     first entry in the procedure linkage table is reserved.  */
	  plt_index = ((h->plt.offset - htab->plt_header_size) 
		       / htab->plt_entry_size);
	}
      else
	{
	  bfd_vma got_offset;
	  bfd_vma got_displacement;
	  asection * sgot;
	  
	  sgot = bfd_get_section_by_name (dynobj, ".got.plt");
	  BFD_ASSERT (sgot != NULL);

	  /* Get the offset into the .got.plt table of the entry that
	     corresponds to this function.  */
	  got_offset = eh->plt_got_offset;

	  /* Get the index in the procedure linkage table which
	     corresponds to this symbol.  This is the index of this symbol
	     in all the symbols for which we are making plt entries.  The
	     first three entries in .got.plt are reserved; after that
	     symbols appear in the same order as in .plt.  */
	  plt_index = (got_offset - 12) / 4;

	  /* Calculate the displacement between the PLT slot and the
	     entry in the GOT.  The eight-byte offset accounts for the
	     value produced by adding to pc in the first instruction
	     of the PLT stub.  */
	  got_displacement = (sgot->output_section->vma
			      + sgot->output_offset
			      + got_offset
			      - splt->output_section->vma
			      - splt->output_offset
			      - h->plt.offset
			      - 8);

	  BFD_ASSERT ((got_displacement & 0xf0000000) == 0);

	  if (eh->plt_thumb_refcount > 0)
	    {
	      bfd_put_16 (output_bfd, elf32_arm_plt_thumb_stub[0],
			  splt->contents + h->plt.offset - 4);
	      bfd_put_16 (output_bfd, elf32_arm_plt_thumb_stub[1],
			  splt->contents + h->plt.offset - 2);
	    }

	  bfd_put_32 (output_bfd, elf32_arm_plt_entry[0] | ((got_displacement & 0x0ff00000) >> 20),
		      splt->contents + h->plt.offset + 0);
	  bfd_put_32 (output_bfd, elf32_arm_plt_entry[1] | ((got_displacement & 0x000ff000) >> 12),
		      splt->contents + h->plt.offset + 4);
	  bfd_put_32 (output_bfd, elf32_arm_plt_entry[2] | (got_displacement & 0x00000fff),
		      splt->contents + h->plt.offset + 8);
#ifdef FOUR_WORD_PLT
	  bfd_put_32 (output_bfd, elf32_arm_plt_entry[3],
		      splt->contents + h->plt.offset + 12);
#endif

	  /* Fill in the entry in the global offset table.  */
	  bfd_put_32 (output_bfd,
		      (splt->output_section->vma
		       + splt->output_offset),
		      sgot->contents + got_offset);
	  
	  /* Fill in the entry in the .rel.plt section.  */
	  rel.r_offset = (sgot->output_section->vma
			  + sgot->output_offset
			  + got_offset);
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_ARM_JUMP_SLOT);
	}

      loc = srel->contents + plt_index * sizeof (Elf32_External_Rel);
      bfd_elf32_swap_reloc_out (output_bfd, &rel, loc);

      if (!h->def_regular)
	{
	  /* Mark the symbol as undefined, rather than as defined in
	     the .plt section.  Leave the value alone.  */
	  sym->st_shndx = SHN_UNDEF;
	  /* If the symbol is weak, we do need to clear the value.
	     Otherwise, the PLT entry would provide a definition for
	     the symbol even if the symbol wasn't defined anywhere,
	     and so the symbol would never be NULL.  */
	  if (!h->ref_regular_nonweak)
	    sym->st_value = 0;
	}
    }

  if (h->got.offset != (bfd_vma) -1)
    {
      asection * sgot;
      asection * srel;
      Elf_Internal_Rela rel;
      bfd_byte *loc;

      /* This symbol has an entry in the global offset table.  Set it
	 up.  */
      sgot = bfd_get_section_by_name (dynobj, ".got");
      srel = bfd_get_section_by_name (dynobj, ".rel.got");
      BFD_ASSERT (sgot != NULL && srel != NULL);

      rel.r_offset = (sgot->output_section->vma
		      + sgot->output_offset
		      + (h->got.offset &~ (bfd_vma) 1));

      /* If this is a static link, or it is a -Bsymbolic link and the
	 symbol is defined locally or was forced to be local because
	 of a version file, we just want to emit a RELATIVE reloc.
	 The entry in the global offset table will already have been
	 initialized in the relocate_section function.  */
      if (info->shared
	  && SYMBOL_REFERENCES_LOCAL (info, h))
	{
	  BFD_ASSERT((h->got.offset & 1) != 0);
	  rel.r_info = ELF32_R_INFO (0, R_ARM_RELATIVE);
	}
      else
	{
	  BFD_ASSERT((h->got.offset & 1) == 0);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + h->got.offset);
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_ARM_GLOB_DAT);
	}

      loc = srel->contents + srel->reloc_count++ * sizeof (Elf32_External_Rel);
      bfd_elf32_swap_reloc_out (output_bfd, &rel, loc);
    }

  if (h->needs_copy)
    {
      asection * s;
      Elf_Internal_Rela rel;
      bfd_byte *loc;

      /* This symbol needs a copy reloc.  Set it up.  */
      BFD_ASSERT (h->dynindx != -1
		  && (h->root.type == bfd_link_hash_defined
		      || h->root.type == bfd_link_hash_defweak));

      s = bfd_get_section_by_name (h->root.u.def.section->owner,
				   ".rel.bss");
      BFD_ASSERT (s != NULL);

      rel.r_offset = (h->root.u.def.value
		      + h->root.u.def.section->output_section->vma
		      + h->root.u.def.section->output_offset);
      rel.r_info = ELF32_R_INFO (h->dynindx, R_ARM_COPY);
      loc = s->contents + s->reloc_count++ * sizeof (Elf32_External_Rel);
      bfd_elf32_swap_reloc_out (output_bfd, &rel, loc);
    }

  /* Mark _DYNAMIC and _GLOBAL_OFFSET_TABLE_ as absolute.  */
  if (strcmp (h->root.root.string, "_DYNAMIC") == 0
      || strcmp (h->root.root.string, "_GLOBAL_OFFSET_TABLE_") == 0)
    sym->st_shndx = SHN_ABS;

  return TRUE;
}

/* Finish up the dynamic sections.  */

static bfd_boolean
elf32_arm_finish_dynamic_sections (bfd * output_bfd, struct bfd_link_info * info)
{
  bfd * dynobj;
  asection * sgot;
  asection * sdyn;

  dynobj = elf_hash_table (info)->dynobj;

  sgot = bfd_get_section_by_name (dynobj, ".got.plt");
  BFD_ASSERT (elf32_arm_hash_table (info)->symbian_p || sgot != NULL);
  sdyn = bfd_get_section_by_name (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      asection *splt;
      Elf32_External_Dyn *dyncon, *dynconend;
      struct elf32_arm_link_hash_table *htab;

      htab = elf32_arm_hash_table (info);
      splt = bfd_get_section_by_name (dynobj, ".plt");
      BFD_ASSERT (splt != NULL && sdyn != NULL);

      dyncon = (Elf32_External_Dyn *) sdyn->contents;
      dynconend = (Elf32_External_Dyn *) (sdyn->contents + sdyn->size);

      for (; dyncon < dynconend; dyncon++)
	{
	  Elf_Internal_Dyn dyn;
	  const char * name;
	  asection * s;

	  bfd_elf32_swap_dyn_in (dynobj, dyncon, &dyn);

	  switch (dyn.d_tag)
	    {
	      unsigned int type;

	    default:
	      break;

	    case DT_HASH:
	      name = ".hash";
	      goto get_vma_if_bpabi;
	    case DT_STRTAB:
	      name = ".dynstr";
	      goto get_vma_if_bpabi;
	    case DT_SYMTAB:
	      name = ".dynsym";
	      goto get_vma_if_bpabi;
	    case DT_VERSYM:
	      name = ".gnu.version";
	      goto get_vma_if_bpabi;
	    case DT_VERDEF:
	      name = ".gnu.version_d";
	      goto get_vma_if_bpabi;
	    case DT_VERNEED:
	      name = ".gnu.version_r";
	      goto get_vma_if_bpabi;

	    case DT_PLTGOT:
	      name = ".got";
	      goto get_vma;
	    case DT_JMPREL:
	      name = ".rel.plt";
	    get_vma:
	      s = bfd_get_section_by_name (output_bfd, name);
	      BFD_ASSERT (s != NULL);
	      if (!htab->symbian_p)
		dyn.d_un.d_ptr = s->vma;
	      else
		/* In the BPABI, tags in the PT_DYNAMIC section point
		   at the file offset, not the memory address, for the
		   convenience of the post linker.  */
		dyn.d_un.d_ptr = s->filepos;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;

	    get_vma_if_bpabi:
	      if (htab->symbian_p)
		goto get_vma;
	      break;

	    case DT_PLTRELSZ:
	      s = bfd_get_section_by_name (output_bfd, ".rel.plt");
	      BFD_ASSERT (s != NULL);
	      dyn.d_un.d_val = s->size;
	      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	      break;
	      
	    case DT_RELSZ:
	      if (!htab->symbian_p)
		{
		  /* My reading of the SVR4 ABI indicates that the
		     procedure linkage table relocs (DT_JMPREL) should be
		     included in the overall relocs (DT_REL).  This is
		     what Solaris does.  However, UnixWare can not handle
		     that case.  Therefore, we override the DT_RELSZ entry
		     here to make it not include the JMPREL relocs.  Since
		     the linker script arranges for .rel.plt to follow all
		     other relocation sections, we don't have to worry
		     about changing the DT_REL entry.  */
		  s = bfd_get_section_by_name (output_bfd, ".rel.plt");
		  if (s != NULL)
		    dyn.d_un.d_val -= s->size;
		  bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
		  break;
		}
	      /* Fall through */

	    case DT_REL:
	    case DT_RELA:
	    case DT_RELASZ:
	      /* In the BPABI, the DT_REL tag must point at the file
		 offset, not the VMA, of the first relocation
		 section.  So, we use code similar to that in
		 elflink.c, but do not check for SHF_ALLOC on the
		 relcoation section, since relocations sections are
		 never allocated under the BPABI.  The comments above
		 about Unixware notwithstanding, we include all of the
		 relocations here.  */
	      if (htab->symbian_p)
		{
		  unsigned int i;
		  type = ((dyn.d_tag == DT_REL || dyn.d_tag == DT_RELSZ)
			  ? SHT_REL : SHT_RELA);
		  dyn.d_un.d_val = 0;
		  for (i = 1; i < elf_numsections (output_bfd); i++)
		    {
		      Elf_Internal_Shdr *hdr 
			= elf_elfsections (output_bfd)[i];
		      if (hdr->sh_type == type)
			{
			  if (dyn.d_tag == DT_RELSZ 
			      || dyn.d_tag == DT_RELASZ)
			    dyn.d_un.d_val += hdr->sh_size;
			  else if ((ufile_ptr) hdr->sh_offset
				   <= dyn.d_un.d_val - 1)
			    dyn.d_un.d_val = hdr->sh_offset;
			}
		    }
		  bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
		}
	      break;

	      /* Set the bottom bit of DT_INIT/FINI if the
		 corresponding function is Thumb.  */
	    case DT_INIT:
	      name = info->init_function;
	      goto get_sym;
	    case DT_FINI:
	      name = info->fini_function;
	    get_sym:
	      /* If it wasn't set by elf_bfd_final_link
		 then there is nothing to adjust.  */
	      if (dyn.d_un.d_val != 0)
		{
		  struct elf_link_hash_entry * eh;

		  eh = elf_link_hash_lookup (elf_hash_table (info), name,
					     FALSE, FALSE, TRUE);
		  if (eh != (struct elf_link_hash_entry *) NULL
		      && ELF_ST_TYPE (eh->type) == STT_ARM_TFUNC)
		    {
		      dyn.d_un.d_val |= 1;
		      bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
		    }
		}
	      break;
	    }
	}

      /* Fill in the first entry in the procedure linkage table.  */
      if (splt->size > 0 && elf32_arm_hash_table (info)->plt_header_size)
	{
	  bfd_vma got_displacement;

	  /* Calculate the displacement between the PLT slot and &GOT[0].  */
	  got_displacement = (sgot->output_section->vma
			      + sgot->output_offset
			      - splt->output_section->vma
			      - splt->output_offset
			      - 16);

	  bfd_put_32 (output_bfd, elf32_arm_plt0_entry[0], splt->contents +  0);
	  bfd_put_32 (output_bfd, elf32_arm_plt0_entry[1], splt->contents +  4);
	  bfd_put_32 (output_bfd, elf32_arm_plt0_entry[2], splt->contents +  8);
	  bfd_put_32 (output_bfd, elf32_arm_plt0_entry[3], splt->contents + 12);
#ifdef FOUR_WORD_PLT
	  /* The displacement value goes in the otherwise-unused last word of
	     the second entry.  */
	  bfd_put_32 (output_bfd, got_displacement,        splt->contents + 28);
#else
	  bfd_put_32 (output_bfd, got_displacement,        splt->contents + 16);
#endif
	}

      /* UnixWare sets the entsize of .plt to 4, although that doesn't
	 really seem like the right value.  */
      elf_section_data (splt->output_section)->this_hdr.sh_entsize = 4;
    }

  /* Fill in the first three entries in the global offset table.  */
  if (sgot)
    {
      if (sgot->size > 0)
	{
	  if (sdyn == NULL)
	    bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents);
	  else
	    bfd_put_32 (output_bfd,
			sdyn->output_section->vma + sdyn->output_offset,
			sgot->contents);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + 4);
	  bfd_put_32 (output_bfd, (bfd_vma) 0, sgot->contents + 8);
	}

      elf_section_data (sgot->output_section)->this_hdr.sh_entsize = 4;
    }

  return TRUE;
}

static void
elf32_arm_post_process_headers (bfd * abfd, struct bfd_link_info * link_info ATTRIBUTE_UNUSED)
{
  Elf_Internal_Ehdr * i_ehdrp;	/* ELF file header, internal form.  */
  struct elf32_arm_link_hash_table *globals;

  i_ehdrp = elf_elfheader (abfd);

  if (EF_ARM_EABI_VERSION (i_ehdrp->e_flags) == EF_ARM_EABI_UNKNOWN)
    i_ehdrp->e_ident[EI_OSABI] = ELFOSABI_ARM;
  else
    i_ehdrp->e_ident[EI_OSABI] = 0;
  i_ehdrp->e_ident[EI_ABIVERSION] = ARM_ELF_ABI_VERSION;

  if (link_info)
    {
      globals = elf32_arm_hash_table (link_info);
      if (globals->byteswap_code)
	i_ehdrp->e_flags |= EF_ARM_BE8;
    }
}

static enum elf_reloc_type_class
elf32_arm_reloc_type_class (const Elf_Internal_Rela *rela)
{
  switch ((int) ELF32_R_TYPE (rela->r_info))
    {
    case R_ARM_RELATIVE:
      return reloc_class_relative;
    case R_ARM_JUMP_SLOT:
      return reloc_class_plt;
    case R_ARM_COPY:
      return reloc_class_copy;
    default:
      return reloc_class_normal;
    }
}

/* Set the right machine number for an Arm ELF file.  */

static bfd_boolean
elf32_arm_section_flags (flagword *flags, const Elf_Internal_Shdr *hdr)
{
  if (hdr->sh_type == SHT_NOTE)
    *flags |= SEC_LINK_ONCE | SEC_LINK_DUPLICATES_SAME_CONTENTS;

  return TRUE;
}

static void
elf32_arm_final_write_processing (bfd *abfd, bfd_boolean linker ATTRIBUTE_UNUSED)
{
  bfd_arm_update_notes (abfd, ARM_NOTE_SECTION);
}

/* Return TRUE if this is an unwinding table entry.  */

static bfd_boolean
is_arm_elf_unwind_section_name (bfd * abfd ATTRIBUTE_UNUSED, const char * name)
{
  size_t len1, len2;

  len1 = sizeof (ELF_STRING_ARM_unwind) - 1;
  len2 = sizeof (ELF_STRING_ARM_unwind_once) - 1;
  return (strncmp (name, ELF_STRING_ARM_unwind, len1) == 0
	  || strncmp (name, ELF_STRING_ARM_unwind_once, len2) == 0);
}


/* Set the type and flags for an ARM section.  We do this by
   the section name, which is a hack, but ought to work.  */

static bfd_boolean
elf32_arm_fake_sections (bfd * abfd, Elf_Internal_Shdr * hdr, asection * sec)
{
  const char * name;

  name = bfd_get_section_name (abfd, sec);

  if (is_arm_elf_unwind_section_name (abfd, name))
    {
      hdr->sh_type = SHT_ARM_EXIDX;
      hdr->sh_flags |= SHF_LINK_ORDER;
    }
  return TRUE;
}

/* Handle an ARM specific section when reading an object file.
   This is called when elf.c finds a section with an unknown type.  */

static bfd_boolean
elf32_arm_section_from_shdr (bfd *abfd,
			     Elf_Internal_Shdr * hdr,
			     const char *name)
{
  /* There ought to be a place to keep ELF backend specific flags, but
     at the moment there isn't one.  We just keep track of the
     sections by their name, instead.  Fortunately, the ABI gives
     names for all the ARM specific sections, so we will probably get
     away with this.  */
  switch (hdr->sh_type)
    {
    case SHT_ARM_EXIDX:
      break;

    default:
      return FALSE;
    }

  if (! _bfd_elf_make_section_from_shdr (abfd, hdr, name))
    return FALSE;

  return TRUE;
}

/* Called for each symbol.  Builds a section map based on mapping symbols.
   Does not alter any of the symbols.  */

static bfd_boolean
elf32_arm_output_symbol_hook (struct bfd_link_info *info,
			      const char *name,
			      Elf_Internal_Sym *elfsym,
			      asection *input_sec,
			      struct elf_link_hash_entry *h ATTRIBUTE_UNUSED)
{
  int mapcount;
  elf32_arm_section_map *map;
  struct elf32_arm_link_hash_table *globals;

  /* Only do this on final link.  */
  if (info->relocatable)
    return TRUE;

  /* Only build a map if we need to byteswap code.  */
  globals = elf32_arm_hash_table (info);
  if (!globals->byteswap_code)
    return TRUE;

  /* We only want mapping symbols.  */
  if (! is_arm_mapping_symbol_name (name))
    return TRUE;

  mapcount = ++(elf32_arm_section_data (input_sec)->mapcount);
  map = elf32_arm_section_data (input_sec)->map;
  /* TODO: This may be inefficient, but we probably don't usually have many
     mapping symbols per section.  */
  map = bfd_realloc (map, mapcount * sizeof (elf32_arm_section_map));
  elf32_arm_section_data (input_sec)->map = map;

  map[mapcount - 1].vma = elfsym->st_value;
  map[mapcount - 1].type = name[1];
  return TRUE;
}


/* Allocate target specific section data.  */

static bfd_boolean
elf32_arm_new_section_hook (bfd *abfd, asection *sec)
{
  struct _arm_elf_section_data *sdata;
  bfd_size_type amt = sizeof (*sdata);

  sdata = bfd_zalloc (abfd, amt);
  if (sdata == NULL)
    return FALSE;
  sec->used_by_bfd = sdata;

  return _bfd_elf_new_section_hook (abfd, sec);
}


/* Used to order a list of mapping symbols by address.  */

static int
elf32_arm_compare_mapping (const void * a, const void * b)
{
  return ((const elf32_arm_section_map *) a)->vma
	 > ((const elf32_arm_section_map *) b)->vma;
}


/* Do code byteswapping.  Return FALSE afterwards so that the section is
   written out as normal.  */

static bfd_boolean
elf32_arm_write_section (bfd *output_bfd ATTRIBUTE_UNUSED, asection *sec,
			 bfd_byte *contents)
{
  int mapcount;
  elf32_arm_section_map *map;
  bfd_vma ptr;
  bfd_vma end;
  bfd_vma offset;
  bfd_byte tmp;
  int i;

  mapcount = elf32_arm_section_data (sec)->mapcount;
  map = elf32_arm_section_data (sec)->map;

  if (mapcount == 0)
    return FALSE;

  qsort (map, mapcount, sizeof (elf32_arm_section_map),
	 elf32_arm_compare_mapping);

  offset = sec->output_section->vma + sec->output_offset;
  ptr = map[0].vma - offset;
  for (i = 0; i < mapcount; i++)
    {
      if (i == mapcount - 1)
	end = sec->size;
      else
	end = map[i + 1].vma - offset;

      switch (map[i].type)
	{
	case 'a':
	  /* Byte swap code words.  */
	  while (ptr + 3 < end)
	    {
	      tmp = contents[ptr];
	      contents[ptr] = contents[ptr + 3];
	      contents[ptr + 3] = tmp;
	      tmp = contents[ptr + 1];
	      contents[ptr + 1] = contents[ptr + 2];
	      contents[ptr + 2] = tmp;
	      ptr += 4;
	    }
	  break;

	case 't':
	  /* Byte swap code halfwords.  */
	  while (ptr + 1 < end)
	    {
	      tmp = contents[ptr];
	      contents[ptr] = contents[ptr + 1];
	      contents[ptr + 1] = tmp;
	      ptr += 2;
	    }
	  break;

	case 'd':
	  /* Leave data alone.  */
	  break;
	}
      ptr = end;
    }
  free (map);
  return FALSE;
}

/* Display STT_ARM_TFUNC symbols as functions.  */

static void
elf32_arm_symbol_processing (bfd *abfd ATTRIBUTE_UNUSED,
			     asymbol *asym)
{
  elf_symbol_type *elfsym = (elf_symbol_type *) asym;

  if (ELF_ST_TYPE (elfsym->internal_elf_sym.st_info) == STT_ARM_TFUNC)
    elfsym->symbol.flags |= BSF_FUNCTION;
}


/* Mangle thumb function symbols as we read them in.  */

static void
elf32_arm_swap_symbol_in (bfd * abfd,
			  const void *psrc,
			  const void *pshn,
			  Elf_Internal_Sym *dst)
{
  bfd_elf32_swap_symbol_in (abfd, psrc, pshn, dst);

  /* New EABI objects mark thumb function symbols by setting the low bit of
     the address.  Turn these into STT_ARM_TFUNC.  */
  if (ELF_ST_TYPE (dst->st_info) == STT_FUNC
      && (dst->st_value & 1))
    {
      dst->st_info = ELF_ST_INFO (ELF_ST_BIND (dst->st_info), STT_ARM_TFUNC);
      dst->st_value &= ~(bfd_vma) 1;
    }
}


/* Mangle thumb function symbols as we write them out.  */

static void
elf32_arm_swap_symbol_out (bfd *abfd,
			   const Elf_Internal_Sym *src,
			   void *cdst,
			   void *shndx)
{
  Elf_Internal_Sym newsym;

  /* We convert STT_ARM_TFUNC symbols into STT_FUNC with the low bit
     of the address set, as per the new EABI.  We do this unconditionally
     because objcopy does not set the elf header flags until after
     it writes out the symbol table.  */
  if (ELF_ST_TYPE (src->st_info) == STT_ARM_TFUNC)
    {
      newsym = *src;
      newsym.st_info = ELF_ST_INFO (ELF_ST_BIND (src->st_info), STT_FUNC);
      newsym.st_value |= 1;
      
      src = &newsym;
    }
  bfd_elf32_swap_symbol_out (abfd, src, cdst, shndx);
}

/* Add the PT_ARM_EXIDX program header.  */

static bfd_boolean
elf32_arm_modify_segment_map (bfd *abfd, 
			      struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  struct elf_segment_map *m;
  asection *sec;

  sec = bfd_get_section_by_name (abfd, ".ARM.exidx");
  if (sec != NULL && (sec->flags & SEC_LOAD) != 0)
    {
      /* If there is already a PT_ARM_EXIDX header, then we do not
	 want to add another one.  This situation arises when running
	 "strip"; the input binary already has the header.  */
      m = elf_tdata (abfd)->segment_map;
      while (m && m->p_type != PT_ARM_EXIDX)
	m = m->next;
      if (!m)
	{
	  m = bfd_zalloc (abfd, sizeof (struct elf_segment_map));
	  if (m == NULL)
	    return FALSE;
	  m->p_type = PT_ARM_EXIDX;
	  m->count = 1;
	  m->sections[0] = sec;

	  m->next = elf_tdata (abfd)->segment_map;
	  elf_tdata (abfd)->segment_map = m;
	}
    }

  return TRUE;
}

/* We may add a PT_ARM_EXIDX program header.  */

static int
elf32_arm_additional_program_headers (bfd *abfd)
{
  asection *sec;

  sec = bfd_get_section_by_name (abfd, ".ARM.exidx");
  if (sec != NULL && (sec->flags & SEC_LOAD) != 0)
    return 1;
  else
    return 0;
}

/* We use this to override swap_symbol_in and swap_symbol_out.  */
const struct elf_size_info elf32_arm_size_info = {
  sizeof (Elf32_External_Ehdr),
  sizeof (Elf32_External_Phdr),
  sizeof (Elf32_External_Shdr),
  sizeof (Elf32_External_Rel),
  sizeof (Elf32_External_Rela),
  sizeof (Elf32_External_Sym),
  sizeof (Elf32_External_Dyn),
  sizeof (Elf_External_Note),
  4,
  1,
  32, 2,
  ELFCLASS32, EV_CURRENT,
  bfd_elf32_write_out_phdrs,
  bfd_elf32_write_shdrs_and_ehdr,
  bfd_elf32_write_relocs,
  elf32_arm_swap_symbol_in,
  elf32_arm_swap_symbol_out,
  bfd_elf32_slurp_reloc_table,
  bfd_elf32_slurp_symbol_table,
  bfd_elf32_swap_dyn_in,
  bfd_elf32_swap_dyn_out,
  bfd_elf32_swap_reloc_in,
  bfd_elf32_swap_reloc_out,
  bfd_elf32_swap_reloca_in,
  bfd_elf32_swap_reloca_out
};

#define ELF_ARCH			bfd_arch_arm
#define ELF_MACHINE_CODE		EM_ARM
#ifdef __QNXTARGET__
#define ELF_MAXPAGESIZE			0x1000
#else
#define ELF_MAXPAGESIZE			0x8000
#endif
#define ELF_MINPAGESIZE			0x1000

#define bfd_elf32_bfd_copy_private_bfd_data	elf32_arm_copy_private_bfd_data
#define bfd_elf32_bfd_merge_private_bfd_data	elf32_arm_merge_private_bfd_data
#define bfd_elf32_bfd_set_private_flags		elf32_arm_set_private_flags
#define bfd_elf32_bfd_print_private_bfd_data	elf32_arm_print_private_bfd_data
#define bfd_elf32_bfd_link_hash_table_create    elf32_arm_link_hash_table_create
#define bfd_elf32_bfd_reloc_type_lookup		elf32_arm_reloc_type_lookup
#define bfd_elf32_find_nearest_line	        elf32_arm_find_nearest_line
#define bfd_elf32_new_section_hook		elf32_arm_new_section_hook
#define bfd_elf32_bfd_is_target_special_symbol	elf32_arm_is_target_special_symbol

#define elf_backend_get_symbol_type             elf32_arm_get_symbol_type
#define elf_backend_gc_mark_hook                elf32_arm_gc_mark_hook
#define elf_backend_gc_sweep_hook               elf32_arm_gc_sweep_hook
#define elf_backend_check_relocs                elf32_arm_check_relocs
#define elf_backend_relocate_section		elf32_arm_relocate_section
#define elf_backend_write_section		elf32_arm_write_section
#define elf_backend_adjust_dynamic_symbol	elf32_arm_adjust_dynamic_symbol
#define elf_backend_create_dynamic_sections     elf32_arm_create_dynamic_sections
#define elf_backend_finish_dynamic_symbol	elf32_arm_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections	elf32_arm_finish_dynamic_sections
#define elf_backend_link_output_symbol_hook	elf32_arm_output_symbol_hook
#define elf_backend_size_dynamic_sections	elf32_arm_size_dynamic_sections
#define elf_backend_post_process_headers	elf32_arm_post_process_headers
#define elf_backend_reloc_type_class		elf32_arm_reloc_type_class
#define elf_backend_object_p			elf32_arm_object_p
#define elf_backend_section_flags		elf32_arm_section_flags
#define elf_backend_fake_sections  		elf32_arm_fake_sections
#define elf_backend_section_from_shdr  		elf32_arm_section_from_shdr
#define elf_backend_final_write_processing      elf32_arm_final_write_processing
#define elf_backend_copy_indirect_symbol        elf32_arm_copy_indirect_symbol
#define elf_backend_symbol_processing		elf32_arm_symbol_processing
#define elf_backend_size_info			elf32_arm_size_info
#define elf_backend_modify_segment_map		elf32_arm_modify_segment_map
#define elf_backend_additional_program_headers \
  elf32_arm_additional_program_headers

#define elf_backend_can_refcount    1
#define elf_backend_can_gc_sections 1
#define elf_backend_plt_readonly    1
#define elf_backend_want_got_plt    1
#define elf_backend_want_plt_sym    0
#define elf_backend_may_use_rel_p   1
#define elf_backend_may_use_rela_p  0
#define elf_backend_default_use_rela_p 0
#define elf_backend_rela_normal     0

#define elf_backend_got_header_size	12

#include "elf32-target.h"

/* VxWorks Targets */

#undef TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM               bfd_elf32_littlearm_vxworks_vec
#undef TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME              "elf32-littlearm-vxworks"
#undef TARGET_BIG_SYM
#define TARGET_BIG_SYM                  bfd_elf32_bigarm_vxworks_vec
#undef TARGET_BIG_NAME
#define TARGET_BIG_NAME                 "elf32-bigarm-vxworks"

/* Like elf32_arm_link_hash_table_create -- but overrides
   appropriately for VxWorks.  */
static struct bfd_link_hash_table *
elf32_arm_vxworks_link_hash_table_create (bfd *abfd)
{
  struct bfd_link_hash_table *ret;

  ret = elf32_arm_link_hash_table_create (abfd);
  if (ret)
    {
      struct elf32_arm_link_hash_table *htab
	= (struct elf32_arm_link_hash_table *)ret;
      htab->use_rel = 0;
    }
  return ret;
}     

#undef elf32_bed
#define elf32_bed elf32_arm_vxworks_bed

#undef bfd_elf32_bfd_link_hash_table_create
#define bfd_elf32_bfd_link_hash_table_create \
  elf32_arm_vxworks_link_hash_table_create

#undef elf_backend_may_use_rel_p
#define elf_backend_may_use_rel_p   0
#undef elf_backend_may_use_rela_p
#define elf_backend_may_use_rela_p  1
#undef elf_backend_default_use_rela_p
#define elf_backend_default_use_rela_p 1
#undef elf_backend_rela_normal
#define elf_backend_rela_normal     1

#include "elf32-target.h"


/* Symbian OS Targets */

#undef TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM               bfd_elf32_littlearm_symbian_vec
#undef TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME              "elf32-littlearm-symbian"
#undef TARGET_BIG_SYM
#define TARGET_BIG_SYM                  bfd_elf32_bigarm_symbian_vec
#undef TARGET_BIG_NAME
#define TARGET_BIG_NAME                 "elf32-bigarm-symbian"

/* Like elf32_arm_link_hash_table_create -- but overrides
   appropriately for Symbian OS.  */
static struct bfd_link_hash_table *
elf32_arm_symbian_link_hash_table_create (bfd *abfd)
{
  struct bfd_link_hash_table *ret;

  ret = elf32_arm_link_hash_table_create (abfd);
  if (ret)
    {
      struct elf32_arm_link_hash_table *htab
	= (struct elf32_arm_link_hash_table *)ret;
      /* There is no PLT header for Symbian OS.  */
      htab->plt_header_size = 0;
      /* The PLT entries are each three instructions.  */
      htab->plt_entry_size = 4 * NUM_ELEM (elf32_arm_symbian_plt_entry);
      htab->symbian_p = 1;
      htab->root.is_relocatable_executable = 1;
    }
  return ret;
}     

static struct bfd_elf_special_section const 
  elf32_arm_symbian_special_sections[]=
{
  /* In a BPABI executable, the dynamic linking sections do not go in
     the loadable read-only segment.  The post-linker may wish to
     refer to these sections, but they are not part of the final
     program image.  */
  { ".dynamic",        8,  0, SHT_DYNAMIC,  0 },
  { ".dynstr",         7,  0, SHT_STRTAB,   0 },
  { ".dynsym",         7,  0, SHT_DYNSYM,   0 },
  { ".got",            4,  0, SHT_PROGBITS, 0 },
  { ".hash",           5,  0, SHT_HASH,     0 },
  /* These sections do not need to be writable as the SymbianOS
     postlinker will arrange things so that no dynamic relocation is
     required.  */
  { ".init_array",    11,  0, SHT_INIT_ARRAY, SHF_ALLOC },
  { ".fini_array",    11,  0, SHT_FINI_ARRAY, SHF_ALLOC },
  { ".preinit_array", 14,  0, SHT_PREINIT_ARRAY, SHF_ALLOC },
  { NULL,              0,  0, 0,            0 }
};

static void
elf32_arm_symbian_begin_write_processing (bfd *abfd, 
					  struct bfd_link_info *link_info
					    ATTRIBUTE_UNUSED)
{
  /* BPABI objects are never loaded directly by an OS kernel; they are
     processed by a postlinker first, into an OS-specific format.  If
     the D_PAGED bit is set on the file, BFD will align segments on
     page boundaries, so that an OS can directly map the file.  With
     BPABI objects, that just results in wasted space.  In addition,
     because we clear the D_PAGED bit, map_sections_to_segments will
     recognize that the program headers should not be mapped into any
     loadable segment.  */
  abfd->flags &= ~D_PAGED;
}

static bfd_boolean
elf32_arm_symbian_modify_segment_map (bfd *abfd, 
				      struct bfd_link_info *info)
{
  struct elf_segment_map *m;
  asection *dynsec;

  /* BPABI shared libraries and executables should have a PT_DYNAMIC
     segment.  However, because the .dynamic section is not marked
     with SEC_LOAD, the generic ELF code will not create such a
     segment.  */
  dynsec = bfd_get_section_by_name (abfd, ".dynamic");
  if (dynsec)
    {
      m = _bfd_elf_make_dynamic_segment (abfd, dynsec);
      m->next = elf_tdata (abfd)->segment_map;
      elf_tdata (abfd)->segment_map = m;
    }

  /* Also call the generic arm routine.  */
  return elf32_arm_modify_segment_map (abfd, info);
}

#undef elf32_bed
#define elf32_bed elf32_arm_symbian_bed

/* The dynamic sections are not allocated on SymbianOS; the postlinker
   will process them and then discard them.  */
#undef ELF_DYNAMIC_SEC_FLAGS
#define ELF_DYNAMIC_SEC_FLAGS \
  (SEC_HAS_CONTENTS | SEC_IN_MEMORY | SEC_LINKER_CREATED)

#undef bfd_elf32_bfd_link_hash_table_create
#define bfd_elf32_bfd_link_hash_table_create \
  elf32_arm_symbian_link_hash_table_create

#undef elf_backend_special_sections
#define elf_backend_special_sections elf32_arm_symbian_special_sections

#undef elf_backend_begin_write_processing
#define elf_backend_begin_write_processing \
    elf32_arm_symbian_begin_write_processing

#undef elf_backend_modify_segment_map
#define elf_backend_modify_segment_map elf32_arm_symbian_modify_segment_map

/* There is no .got section for BPABI objects, and hence no header.  */
#undef elf_backend_got_header_size
#define elf_backend_got_header_size 0

/* Similarly, there is no .got.plt section.  */
#undef elf_backend_want_got_plt
#define elf_backend_want_got_plt 0

#undef elf_backend_may_use_rel_p
#define elf_backend_may_use_rel_p   1
#undef elf_backend_may_use_rela_p
#define elf_backend_may_use_rela_p  0
#undef elf_backend_default_use_rela_p
#define elf_backend_default_use_rela_p 0
#undef elf_backend_rela_normal
#define elf_backend_rela_normal     0

#include "elf32-target.h"
