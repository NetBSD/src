/* Intel 80386/80486-specific support for 32-bit ELF
   Copyright (C) 1993-2017 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf-nacl.h"
#include "elf-vxworks.h"
#include "bfd_stdint.h"
#include "objalloc.h"
#include "hashtab.h"
#include "dwarf2.h"
#include "opcode/i386.h"

/* 386 uses REL relocations instead of RELA.  */
#define USE_REL	1

#include "elf/i386.h"

static reloc_howto_type elf_howto_table[]=
{
  HOWTO(R_386_NONE, 0, 3, 0, FALSE, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_386_NONE",
	TRUE, 0x00000000, 0x00000000, FALSE),
  HOWTO(R_386_32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_PC32, 0, 2, 32, TRUE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_PC32",
	TRUE, 0xffffffff, 0xffffffff, TRUE),
  HOWTO(R_386_GOT32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_GOT32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_PLT32, 0, 2, 32, TRUE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_PLT32",
	TRUE, 0xffffffff, 0xffffffff, TRUE),
  HOWTO(R_386_COPY, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_COPY",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_GLOB_DAT, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_GLOB_DAT",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_JUMP_SLOT, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_JUMP_SLOT",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_RELATIVE, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_RELATIVE",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_GOTOFF, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_GOTOFF",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_GOTPC, 0, 2, 32, TRUE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_GOTPC",
	TRUE, 0xffffffff, 0xffffffff, TRUE),

  /* We have a gap in the reloc numbers here.
     R_386_standard counts the number up to this point, and
     R_386_ext_offset is the value to subtract from a reloc type of
     R_386_16 thru R_386_PC8 to form an index into this table.  */
#define R_386_standard (R_386_GOTPC + 1)
#define R_386_ext_offset (R_386_TLS_TPOFF - R_386_standard)

  /* These relocs are a GNU extension.  */
  HOWTO(R_386_TLS_TPOFF, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_TPOFF",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_IE, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_IE",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_GOTIE, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_GOTIE",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_LE, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_LE",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_GD, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_GD",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_LDM, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_LDM",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_16, 0, 1, 16, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_16",
	TRUE, 0xffff, 0xffff, FALSE),
  HOWTO(R_386_PC16, 0, 1, 16, TRUE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_PC16",
	TRUE, 0xffff, 0xffff, TRUE),
  HOWTO(R_386_8, 0, 0, 8, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_8",
	TRUE, 0xff, 0xff, FALSE),
  HOWTO(R_386_PC8, 0, 0, 8, TRUE, 0, complain_overflow_signed,
	bfd_elf_generic_reloc, "R_386_PC8",
	TRUE, 0xff, 0xff, TRUE),

#define R_386_ext (R_386_PC8 + 1 - R_386_ext_offset)
#define R_386_tls_offset (R_386_TLS_LDO_32 - R_386_ext)
  /* These are common with Solaris TLS implementation.  */
  HOWTO(R_386_TLS_LDO_32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_LDO_32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_IE_32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_IE_32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_LE_32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_LE_32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_DTPMOD32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_DTPMOD32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_DTPOFF32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_DTPOFF32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_TPOFF32, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_TPOFF32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_SIZE32, 0, 2, 32, FALSE, 0, complain_overflow_unsigned,
	bfd_elf_generic_reloc, "R_386_SIZE32",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_GOTDESC, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_GOTDESC",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_TLS_DESC_CALL, 0, 0, 0, FALSE, 0, complain_overflow_dont,
	bfd_elf_generic_reloc, "R_386_TLS_DESC_CALL",
	FALSE, 0, 0, FALSE),
  HOWTO(R_386_TLS_DESC, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_TLS_DESC",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_IRELATIVE, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_IRELATIVE",
	TRUE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(R_386_GOT32X, 0, 2, 32, FALSE, 0, complain_overflow_bitfield,
	bfd_elf_generic_reloc, "R_386_GOT32X",
	TRUE, 0xffffffff, 0xffffffff, FALSE),

  /* Another gap.  */
#define R_386_ext2 (R_386_GOT32X + 1 - R_386_tls_offset)
#define R_386_vt_offset (R_386_GNU_VTINHERIT - R_386_ext2)

/* GNU extension to record C++ vtable hierarchy.  */
  HOWTO (R_386_GNU_VTINHERIT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_386_GNU_VTINHERIT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

/* GNU extension to record C++ vtable member usage.  */
  HOWTO (R_386_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn, /* special_function */
	 "R_386_GNU_VTENTRY",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE)			/* pcrel_offset */

#define R_386_vt (R_386_GNU_VTENTRY + 1 - R_386_vt_offset)

};

#ifdef DEBUG_GEN_RELOC
#define TRACE(str) \
  fprintf (stderr, "i386 bfd reloc lookup %d (%s)\n", code, str)
#else
#define TRACE(str)
#endif

static reloc_howto_type *
elf_i386_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			    bfd_reloc_code_real_type code)
{
  switch (code)
    {
    case BFD_RELOC_NONE:
      TRACE ("BFD_RELOC_NONE");
      return &elf_howto_table[R_386_NONE];

    case BFD_RELOC_32:
      TRACE ("BFD_RELOC_32");
      return &elf_howto_table[R_386_32];

    case BFD_RELOC_CTOR:
      TRACE ("BFD_RELOC_CTOR");
      return &elf_howto_table[R_386_32];

    case BFD_RELOC_32_PCREL:
      TRACE ("BFD_RELOC_PC32");
      return &elf_howto_table[R_386_PC32];

    case BFD_RELOC_386_GOT32:
      TRACE ("BFD_RELOC_386_GOT32");
      return &elf_howto_table[R_386_GOT32];

    case BFD_RELOC_386_PLT32:
      TRACE ("BFD_RELOC_386_PLT32");
      return &elf_howto_table[R_386_PLT32];

    case BFD_RELOC_386_COPY:
      TRACE ("BFD_RELOC_386_COPY");
      return &elf_howto_table[R_386_COPY];

    case BFD_RELOC_386_GLOB_DAT:
      TRACE ("BFD_RELOC_386_GLOB_DAT");
      return &elf_howto_table[R_386_GLOB_DAT];

    case BFD_RELOC_386_JUMP_SLOT:
      TRACE ("BFD_RELOC_386_JUMP_SLOT");
      return &elf_howto_table[R_386_JUMP_SLOT];

    case BFD_RELOC_386_RELATIVE:
      TRACE ("BFD_RELOC_386_RELATIVE");
      return &elf_howto_table[R_386_RELATIVE];

    case BFD_RELOC_386_GOTOFF:
      TRACE ("BFD_RELOC_386_GOTOFF");
      return &elf_howto_table[R_386_GOTOFF];

    case BFD_RELOC_386_GOTPC:
      TRACE ("BFD_RELOC_386_GOTPC");
      return &elf_howto_table[R_386_GOTPC];

      /* These relocs are a GNU extension.  */
    case BFD_RELOC_386_TLS_TPOFF:
      TRACE ("BFD_RELOC_386_TLS_TPOFF");
      return &elf_howto_table[R_386_TLS_TPOFF - R_386_ext_offset];

    case BFD_RELOC_386_TLS_IE:
      TRACE ("BFD_RELOC_386_TLS_IE");
      return &elf_howto_table[R_386_TLS_IE - R_386_ext_offset];

    case BFD_RELOC_386_TLS_GOTIE:
      TRACE ("BFD_RELOC_386_TLS_GOTIE");
      return &elf_howto_table[R_386_TLS_GOTIE - R_386_ext_offset];

    case BFD_RELOC_386_TLS_LE:
      TRACE ("BFD_RELOC_386_TLS_LE");
      return &elf_howto_table[R_386_TLS_LE - R_386_ext_offset];

    case BFD_RELOC_386_TLS_GD:
      TRACE ("BFD_RELOC_386_TLS_GD");
      return &elf_howto_table[R_386_TLS_GD - R_386_ext_offset];

    case BFD_RELOC_386_TLS_LDM:
      TRACE ("BFD_RELOC_386_TLS_LDM");
      return &elf_howto_table[R_386_TLS_LDM - R_386_ext_offset];

    case BFD_RELOC_16:
      TRACE ("BFD_RELOC_16");
      return &elf_howto_table[R_386_16 - R_386_ext_offset];

    case BFD_RELOC_16_PCREL:
      TRACE ("BFD_RELOC_16_PCREL");
      return &elf_howto_table[R_386_PC16 - R_386_ext_offset];

    case BFD_RELOC_8:
      TRACE ("BFD_RELOC_8");
      return &elf_howto_table[R_386_8 - R_386_ext_offset];

    case BFD_RELOC_8_PCREL:
      TRACE ("BFD_RELOC_8_PCREL");
      return &elf_howto_table[R_386_PC8 - R_386_ext_offset];

    /* Common with Sun TLS implementation.  */
    case BFD_RELOC_386_TLS_LDO_32:
      TRACE ("BFD_RELOC_386_TLS_LDO_32");
      return &elf_howto_table[R_386_TLS_LDO_32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_IE_32:
      TRACE ("BFD_RELOC_386_TLS_IE_32");
      return &elf_howto_table[R_386_TLS_IE_32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_LE_32:
      TRACE ("BFD_RELOC_386_TLS_LE_32");
      return &elf_howto_table[R_386_TLS_LE_32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_DTPMOD32:
      TRACE ("BFD_RELOC_386_TLS_DTPMOD32");
      return &elf_howto_table[R_386_TLS_DTPMOD32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_DTPOFF32:
      TRACE ("BFD_RELOC_386_TLS_DTPOFF32");
      return &elf_howto_table[R_386_TLS_DTPOFF32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_TPOFF32:
      TRACE ("BFD_RELOC_386_TLS_TPOFF32");
      return &elf_howto_table[R_386_TLS_TPOFF32 - R_386_tls_offset];

    case BFD_RELOC_SIZE32:
      TRACE ("BFD_RELOC_SIZE32");
      return &elf_howto_table[R_386_SIZE32 - R_386_tls_offset];

    case BFD_RELOC_386_TLS_GOTDESC:
      TRACE ("BFD_RELOC_386_TLS_GOTDESC");
      return &elf_howto_table[R_386_TLS_GOTDESC - R_386_tls_offset];

    case BFD_RELOC_386_TLS_DESC_CALL:
      TRACE ("BFD_RELOC_386_TLS_DESC_CALL");
      return &elf_howto_table[R_386_TLS_DESC_CALL - R_386_tls_offset];

    case BFD_RELOC_386_TLS_DESC:
      TRACE ("BFD_RELOC_386_TLS_DESC");
      return &elf_howto_table[R_386_TLS_DESC - R_386_tls_offset];

    case BFD_RELOC_386_IRELATIVE:
      TRACE ("BFD_RELOC_386_IRELATIVE");
      return &elf_howto_table[R_386_IRELATIVE - R_386_tls_offset];

    case BFD_RELOC_386_GOT32X:
      TRACE ("BFD_RELOC_386_GOT32X");
      return &elf_howto_table[R_386_GOT32X - R_386_tls_offset];

    case BFD_RELOC_VTABLE_INHERIT:
      TRACE ("BFD_RELOC_VTABLE_INHERIT");
      return &elf_howto_table[R_386_GNU_VTINHERIT - R_386_vt_offset];

    case BFD_RELOC_VTABLE_ENTRY:
      TRACE ("BFD_RELOC_VTABLE_ENTRY");
      return &elf_howto_table[R_386_GNU_VTENTRY - R_386_vt_offset];

    default:
      break;
    }

  TRACE ("Unknown");
  return 0;
}

static reloc_howto_type *
elf_i386_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			    const char *r_name)
{
  unsigned int i;

  for (i = 0; i < sizeof (elf_howto_table) / sizeof (elf_howto_table[0]); i++)
    if (elf_howto_table[i].name != NULL
	&& strcasecmp (elf_howto_table[i].name, r_name) == 0)
      return &elf_howto_table[i];

  return NULL;
}

static reloc_howto_type *
elf_i386_rtype_to_howto (bfd *abfd, unsigned r_type)
{
  unsigned int indx;

  if ((indx = r_type) >= R_386_standard
      && ((indx = r_type - R_386_ext_offset) - R_386_standard
	  >= R_386_ext - R_386_standard)
      && ((indx = r_type - R_386_tls_offset) - R_386_ext
	  >= R_386_ext2 - R_386_ext)
      && ((indx = r_type - R_386_vt_offset) - R_386_ext2
	  >= R_386_vt - R_386_ext2))
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%B: invalid relocation type %d"),
			  abfd, (int) r_type);
      indx = R_386_NONE;
    }
  /* PR 17512: file: 0f67f69d.  */
  if (elf_howto_table [indx].type != r_type)
    return NULL;
  return &elf_howto_table[indx];
}

static void
elf_i386_info_to_howto_rel (bfd *abfd ATTRIBUTE_UNUSED,
			    arelent *cache_ptr,
			    Elf_Internal_Rela *dst)
{
  unsigned int r_type = ELF32_R_TYPE (dst->r_info);
  cache_ptr->howto = elf_i386_rtype_to_howto (abfd, r_type);
}

/* Return whether a symbol name implies a local label.  The UnixWare
   2.1 cc generates temporary symbols that start with .X, so we
   recognize them here.  FIXME: do other SVR4 compilers also use .X?.
   If so, we should move the .X recognition into
   _bfd_elf_is_local_label_name.  */

static bfd_boolean
elf_i386_is_local_label_name (bfd *abfd, const char *name)
{
  if (name[0] == '.' && name[1] == 'X')
    return TRUE;

  return _bfd_elf_is_local_label_name (abfd, name);
}

/* Support for core dump NOTE sections.  */

static bfd_boolean
elf_i386_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  size_t size;

  if (note->namesz == 8 && strcmp (note->namedata, "FreeBSD") == 0)
    {
      int pr_version = bfd_get_32 (abfd, note->descdata);

      if (pr_version != 1)
 	return FALSE;

      /* pr_cursig */
      elf_tdata (abfd)->core->signal = bfd_get_32 (abfd, note->descdata + 20);

      /* pr_pid */
      elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 24);

      /* pr_reg */
      offset = 28;
      size = bfd_get_32 (abfd, note->descdata + 8);
    }
  else
    {
      switch (note->descsz)
	{
	default:
	  return FALSE;

	case 144:		/* Linux/i386 */
	  /* pr_cursig */
	  elf_tdata (abfd)->core->signal = bfd_get_16 (abfd, note->descdata + 12);

	  /* pr_pid */
	  elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 24);

	  /* pr_reg */
	  offset = 72;
	  size = 68;

	  break;
	}
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  size, note->descpos + offset);
}

static bfd_boolean
elf_i386_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  if (note->namesz == 8 && strcmp (note->namedata, "FreeBSD") == 0)
    {
      int pr_version = bfd_get_32 (abfd, note->descdata);

      if (pr_version != 1)
	return FALSE;

      elf_tdata (abfd)->core->program
	= _bfd_elfcore_strndup (abfd, note->descdata + 8, 17);
      elf_tdata (abfd)->core->command
	= _bfd_elfcore_strndup (abfd, note->descdata + 25, 81);
    }
  else
    {
      switch (note->descsz)
	{
	default:
	  return FALSE;

	case 124:		/* Linux/i386 elf_prpsinfo.  */
	  elf_tdata (abfd)->core->pid
	    = bfd_get_32 (abfd, note->descdata + 12);
	  elf_tdata (abfd)->core->program
	    = _bfd_elfcore_strndup (abfd, note->descdata + 28, 16);
	  elf_tdata (abfd)->core->command
	    = _bfd_elfcore_strndup (abfd, note->descdata + 44, 80);
	}
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */
  {
    char *command = elf_tdata (abfd)->core->command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return TRUE;
}

/* Functions for the i386 ELF linker.

   In order to gain some understanding of code in this file without
   knowing all the intricate details of the linker, note the
   following:

   Functions named elf_i386_* are called by external routines, other
   functions are only called locally.  elf_i386_* functions appear
   in this file more or less in the order in which they are called
   from external routines.  eg. elf_i386_check_relocs is called
   early in the link process, elf_i386_finish_dynamic_sections is
   one of the last functions.  */


/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF_DYNAMIC_INTERPRETER "/usr/lib/libc.so.1"

/* If ELIMINATE_COPY_RELOCS is non-zero, the linker will try to avoid
   copying dynamic variables from a shared lib into an app's dynbss
   section, and instead use a dynamic relocation to point into the
   shared lib.  */
#define ELIMINATE_COPY_RELOCS 1

/* The size in bytes of an entry in the procedure linkage table.  */

#define PLT_ENTRY_SIZE 16

/* The first entry in an absolute procedure linkage table looks like
   this.  See the SVR4 ABI i386 supplement to see how this works.
   Will be padded to PLT_ENTRY_SIZE with htab->plt0_pad_byte.  */

static const bfd_byte elf_i386_plt0_entry[12] =
{
  0xff, 0x35,	/* pushl contents of address */
  0, 0, 0, 0,	/* replaced with address of .got + 4.  */
  0xff, 0x25,	/* jmp indirect */
  0, 0, 0, 0	/* replaced with address of .got + 8.  */
};

/* Subsequent entries in an absolute procedure linkage table look like
   this.  */

static const bfd_byte elf_i386_plt_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0x25,	/* jmp indirect */
  0, 0, 0, 0,	/* replaced with address of this symbol in .got.  */
  0x68,		/* pushl immediate */
  0, 0, 0, 0,	/* replaced with offset into relocation table.  */
  0xe9,		/* jmp relative */
  0, 0, 0, 0	/* replaced with offset to start of .plt.  */
};

/* The first entry in a PIC procedure linkage table look like this.
   Will be padded to PLT_ENTRY_SIZE with htab->plt0_pad_byte.  */

static const bfd_byte elf_i386_pic_plt0_entry[12] =
{
  0xff, 0xb3, 4, 0, 0, 0,	/* pushl 4(%ebx) */
  0xff, 0xa3, 8, 0, 0, 0	/* jmp *8(%ebx) */
};

/* Subsequent entries in a PIC procedure linkage table look like this.  */

static const bfd_byte elf_i386_pic_plt_entry[PLT_ENTRY_SIZE] =
{
  0xff, 0xa3,	/* jmp *offset(%ebx) */
  0, 0, 0, 0,	/* replaced with offset of this symbol in .got.  */
  0x68,		/* pushl immediate */
  0, 0, 0, 0,	/* replaced with offset into relocation table.  */
  0xe9,		/* jmp relative */
  0, 0, 0, 0	/* replaced with offset to start of .plt.  */
};

/* Entries in the GOT procedure linkage table look like this.  */

static const bfd_byte elf_i386_got_plt_entry[8] =
{
  0xff, 0x25,	/* jmp indirect */
  0, 0, 0, 0,	/* replaced with offset of this symbol in .got.  */
  0x66, 0x90	/* xchg %ax,%ax  */
};

/* Entries in the PIC GOT procedure linkage table look like this.  */

static const bfd_byte elf_i386_pic_got_plt_entry[8] =
{
  0xff, 0xa3,	/* jmp *offset(%ebx)  */
  0, 0, 0, 0,	/* replaced with offset of this symbol in .got.  */
  0x66, 0x90	/* xchg %ax,%ax  */
};

/* .eh_frame covering the .plt section.  */

static const bfd_byte elf_i386_eh_frame_plt[] =
{
#define PLT_CIE_LENGTH		20
#define PLT_FDE_LENGTH		36
#define PLT_FDE_START_OFFSET	4 + PLT_CIE_LENGTH + 8
#define PLT_FDE_LEN_OFFSET	4 + PLT_CIE_LENGTH + 12
  PLT_CIE_LENGTH, 0, 0, 0,	/* CIE length */
  0, 0, 0, 0,			/* CIE ID */
  1,				/* CIE version */
  'z', 'R', 0,			/* Augmentation string */
  1,				/* Code alignment factor */
  0x7c,				/* Data alignment factor */
  8,				/* Return address column */
  1,				/* Augmentation size */
  DW_EH_PE_pcrel | DW_EH_PE_sdata4, /* FDE encoding */
  DW_CFA_def_cfa, 4, 4,		/* DW_CFA_def_cfa: r4 (esp) ofs 4 */
  DW_CFA_offset + 8, 1,		/* DW_CFA_offset: r8 (eip) at cfa-4 */
  DW_CFA_nop, DW_CFA_nop,

  PLT_FDE_LENGTH, 0, 0, 0,	/* FDE length */
  PLT_CIE_LENGTH + 8, 0, 0, 0,	/* CIE pointer */
  0, 0, 0, 0,			/* R_386_PC32 .plt goes here */
  0, 0, 0, 0,			/* .plt size goes here */
  0,				/* Augmentation size */
  DW_CFA_def_cfa_offset, 8,	/* DW_CFA_def_cfa_offset: 8 */
  DW_CFA_advance_loc + 6,	/* DW_CFA_advance_loc: 6 to __PLT__+6 */
  DW_CFA_def_cfa_offset, 12,	/* DW_CFA_def_cfa_offset: 12 */
  DW_CFA_advance_loc + 10,	/* DW_CFA_advance_loc: 10 to __PLT__+16 */
  DW_CFA_def_cfa_expression,	/* DW_CFA_def_cfa_expression */
  11,				/* Block length */
  DW_OP_breg4, 4,		/* DW_OP_breg4 (esp): 4 */
  DW_OP_breg8, 0,		/* DW_OP_breg8 (eip): 0 */
  DW_OP_lit15, DW_OP_and, DW_OP_lit11, DW_OP_ge,
  DW_OP_lit2, DW_OP_shl, DW_OP_plus,
  DW_CFA_nop, DW_CFA_nop, DW_CFA_nop, DW_CFA_nop
};

/* .eh_frame covering the .plt.got section.  */

static const bfd_byte elf_i386_eh_frame_plt_got[] =
{
#define PLT_GOT_FDE_LENGTH		16
  PLT_CIE_LENGTH, 0, 0, 0,	/* CIE length */
  0, 0, 0, 0,			/* CIE ID */
  1,				/* CIE version */
  'z', 'R', 0,			/* Augmentation string */
  1,				/* Code alignment factor */
  0x7c,				/* Data alignment factor */
  8,				/* Return address column */
  1,				/* Augmentation size */
  DW_EH_PE_pcrel | DW_EH_PE_sdata4, /* FDE encoding */
  DW_CFA_def_cfa, 4, 4,		/* DW_CFA_def_cfa: r4 (esp) ofs 4 */
  DW_CFA_offset + 8, 1,		/* DW_CFA_offset: r8 (eip) at cfa-4 */
  DW_CFA_nop, DW_CFA_nop,

  PLT_GOT_FDE_LENGTH, 0, 0, 0,	/* FDE length */
  PLT_CIE_LENGTH + 8, 0, 0, 0,	/* CIE pointer */
  0, 0, 0, 0,			/* the start of .plt.got goes here */
  0, 0, 0, 0,			/* .plt.got size goes here */
  0,				/* Augmentation size */
  DW_CFA_nop, DW_CFA_nop, DW_CFA_nop
};

struct elf_i386_plt_layout
{
  /* The first entry in an absolute procedure linkage table looks like this.  */
  const bfd_byte *plt0_entry;
  unsigned int plt0_entry_size;

  /* Offsets into plt0_entry that are to be replaced with GOT[1] and GOT[2].  */
  unsigned int plt0_got1_offset;
  unsigned int plt0_got2_offset;

  /* Later entries in an absolute procedure linkage table look like this.  */
  const bfd_byte *plt_entry;
  unsigned int plt_entry_size;

  /* Offsets into plt_entry that are to be replaced with...  */
  unsigned int plt_got_offset;    /* ... address of this symbol in .got. */
  unsigned int plt_reloc_offset;  /* ... offset into relocation table. */
  unsigned int plt_plt_offset;    /* ... offset to start of .plt. */

  /* Offset into plt_entry where the initial value of the GOT entry points.  */
  unsigned int plt_lazy_offset;

  /* The first entry in a PIC procedure linkage table looks like this.  */
  const bfd_byte *pic_plt0_entry;

  /* Subsequent entries in a PIC procedure linkage table look like this.  */
  const bfd_byte *pic_plt_entry;

  /* .eh_frame covering the .plt section.  */
  const bfd_byte *eh_frame_plt;
  unsigned int eh_frame_plt_size;

  /* .eh_frame covering the .plt.got section.  */
  const bfd_byte *eh_frame_plt_got;
  unsigned int eh_frame_plt_got_size;
};

#define GET_PLT_ENTRY_SIZE(abfd) \
  get_elf_i386_backend_data (abfd)->plt->plt_entry_size

/* These are the standard parameters.  */
static const struct elf_i386_plt_layout elf_i386_plt =
  {
    elf_i386_plt0_entry,                /* plt0_entry */
    sizeof (elf_i386_plt0_entry),       /* plt0_entry_size */
    2,                                  /* plt0_got1_offset */
    8,                                  /* plt0_got2_offset */
    elf_i386_plt_entry,                 /* plt_entry */
    PLT_ENTRY_SIZE,                     /* plt_entry_size */
    2,                                  /* plt_got_offset */
    7,                                  /* plt_reloc_offset */
    12,                                 /* plt_plt_offset */
    6,                                  /* plt_lazy_offset */
    elf_i386_pic_plt0_entry,            /* pic_plt0_entry */
    elf_i386_pic_plt_entry,             /* pic_plt_entry */
    elf_i386_eh_frame_plt,              /* eh_frame_plt */
    sizeof (elf_i386_eh_frame_plt),     /* eh_frame_plt_size */
    elf_i386_eh_frame_plt_got,          /* eh_frame_plt_got */
    sizeof (elf_i386_eh_frame_plt_got), /* eh_frame_plt_got_size */
  };


/* On VxWorks, the .rel.plt.unloaded section has absolute relocations
   for the PLTResolve stub and then for each PLT entry.  */
#define PLTRESOLVE_RELOCS_SHLIB 0
#define PLTRESOLVE_RELOCS 2
#define PLT_NON_JUMP_SLOT_RELOCS 2

/* Architecture-specific backend data for i386.  */

struct elf_i386_backend_data
{
  /* Parameters describing PLT generation.  */
  const struct elf_i386_plt_layout *plt;

  /* Value used to fill the unused bytes of the first PLT entry.  */
  bfd_byte plt0_pad_byte;

  /* True if the target system is VxWorks.  */
  int is_vxworks;
};

#define get_elf_i386_backend_data(abfd) \
  ((const struct elf_i386_backend_data *) \
   get_elf_backend_data (abfd)->arch_data)

/* These are the standard parameters.  */
static const struct elf_i386_backend_data elf_i386_arch_bed =
  {
    &elf_i386_plt,                      /* plt */
    0,                                  /* plt0_pad_byte */
    0,                                  /* is_vxworks */
  };

#define	elf_backend_arch_data	&elf_i386_arch_bed

/* Is a undefined weak symbol which is resolved to 0.  Reference to an
   undefined weak symbol is resolved to 0 when building executable if
   it isn't dynamic and
   1. Has non-GOT/non-PLT relocations in text section.  Or
   2. Has no GOT/PLT relocation.
 */
#define UNDEFINED_WEAK_RESOLVED_TO_ZERO(INFO, GOT_RELOC, EH)	\
  ((EH)->elf.root.type == bfd_link_hash_undefweak		\
   && bfd_link_executable (INFO)				\
   && (elf_i386_hash_table (INFO)->interp == NULL	 	\
       || !(GOT_RELOC)						\
       || (EH)->has_non_got_reloc				\
       || !(INFO)->dynamic_undefined_weak))

/* i386 ELF linker hash entry.  */

struct elf_i386_link_hash_entry
{
  struct elf_link_hash_entry elf;

  /* Track dynamic relocs copied for this symbol.  */
  struct elf_dyn_relocs *dyn_relocs;

#define GOT_UNKNOWN	0
#define GOT_NORMAL	1
#define GOT_TLS_GD	2
#define GOT_TLS_IE	4
#define GOT_TLS_IE_POS	5
#define GOT_TLS_IE_NEG	6
#define GOT_TLS_IE_BOTH 7
#define GOT_TLS_GDESC	8
#define GOT_TLS_GD_BOTH_P(type)						\
  ((type) == (GOT_TLS_GD | GOT_TLS_GDESC))
#define GOT_TLS_GD_P(type)						\
  ((type) == GOT_TLS_GD || GOT_TLS_GD_BOTH_P (type))
#define GOT_TLS_GDESC_P(type)						\
  ((type) == GOT_TLS_GDESC || GOT_TLS_GD_BOTH_P (type))
#define GOT_TLS_GD_ANY_P(type)						\
  (GOT_TLS_GD_P (type) || GOT_TLS_GDESC_P (type))
  unsigned char tls_type;

  /* Symbol is referenced by R_386_GOTOFF relocation.  */
  unsigned int gotoff_ref : 1;

  /* Symbol has GOT or PLT relocations.  */
  unsigned int has_got_reloc : 1;

  /* Symbol has non-GOT/non-PLT relocations in text sections.  */
  unsigned int has_non_got_reloc : 1;

  /* 0: symbol isn't ___tls_get_addr.
     1: symbol is ___tls_get_addr.
     2: symbol is unknown.  */
  unsigned int tls_get_addr : 2;

  /* Reference count of C/C++ function pointer relocations in read-write
     section which can be resolved at run-time.  */
  bfd_signed_vma func_pointer_refcount;

  /* Information about the GOT PLT entry. Filled when there are both
     GOT and PLT relocations against the same function.  */
  union gotplt_union plt_got;

  /* Offset of the GOTPLT entry reserved for the TLS descriptor,
     starting at the end of the jump table.  */
  bfd_vma tlsdesc_got;
};

#define elf_i386_hash_entry(ent) ((struct elf_i386_link_hash_entry *)(ent))

struct elf_i386_obj_tdata
{
  struct elf_obj_tdata root;

  /* tls_type for each local got entry.  */
  char *local_got_tls_type;

  /* GOTPLT entries for TLS descriptors.  */
  bfd_vma *local_tlsdesc_gotent;
};

#define elf_i386_tdata(abfd) \
  ((struct elf_i386_obj_tdata *) (abfd)->tdata.any)

#define elf_i386_local_got_tls_type(abfd) \
  (elf_i386_tdata (abfd)->local_got_tls_type)

#define elf_i386_local_tlsdesc_gotent(abfd) \
  (elf_i386_tdata (abfd)->local_tlsdesc_gotent)

#define is_i386_elf(bfd)				\
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour	\
   && elf_tdata (bfd) != NULL				\
   && elf_object_id (bfd) == I386_ELF_DATA)

static bfd_boolean
elf_i386_mkobject (bfd *abfd)
{
  return bfd_elf_allocate_object (abfd, sizeof (struct elf_i386_obj_tdata),
				  I386_ELF_DATA);
}

/* i386 ELF linker hash table.  */

struct elf_i386_link_hash_table
{
  struct elf_link_hash_table elf;

  /* Short-cuts to get to dynamic linker sections.  */
  asection *interp;
  asection *plt_eh_frame;
  asection *plt_got;
  asection *plt_got_eh_frame;

  union
  {
    bfd_signed_vma refcount;
    bfd_vma offset;
  } tls_ldm_got;

  /* The amount of space used by the reserved portion of the sgotplt
     section, plus whatever space is used by the jump slots.  */
  bfd_vma sgotplt_jump_table_size;

  /* Small local sym cache.  */
  struct sym_cache sym_cache;

  /* _TLS_MODULE_BASE_ symbol.  */
  struct bfd_link_hash_entry *tls_module_base;

  /* Used by local STT_GNU_IFUNC symbols.  */
  htab_t loc_hash_table;
  void * loc_hash_memory;

  /* The (unloaded but important) .rel.plt.unloaded section on VxWorks.  */
  asection *srelplt2;

  /* The index of the next unused R_386_TLS_DESC slot in .rel.plt.  */
  bfd_vma next_tls_desc_index;

  /* The index of the next unused R_386_JUMP_SLOT slot in .rel.plt.  */
  bfd_vma next_jump_slot_index;

  /* The index of the next unused R_386_IRELATIVE slot in .rel.plt.  */
  bfd_vma next_irelative_index;

  /* TRUE if there are dynamic relocs against IFUNC symbols that apply
     to read-only sections.  */
  bfd_boolean readonly_dynrelocs_against_ifunc;
};

/* Get the i386 ELF linker hash table from a link_info structure.  */

#define elf_i386_hash_table(p) \
  (elf_hash_table_id  ((struct elf_link_hash_table *) ((p)->hash)) \
  == I386_ELF_DATA ? ((struct elf_i386_link_hash_table *) ((p)->hash)) : NULL)

#define elf_i386_compute_jump_table_size(htab) \
  ((htab)->elf.srelplt->reloc_count * 4)

/* Create an entry in an i386 ELF linker hash table.  */

static struct bfd_hash_entry *
elf_i386_link_hash_newfunc (struct bfd_hash_entry *entry,
			    struct bfd_hash_table *table,
			    const char *string)
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *)
          bfd_hash_allocate (table, sizeof (struct elf_i386_link_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = _bfd_elf_link_hash_newfunc (entry, table, string);
  if (entry != NULL)
    {
      struct elf_i386_link_hash_entry *eh;

      eh = (struct elf_i386_link_hash_entry *) entry;
      eh->dyn_relocs = NULL;
      eh->tls_type = GOT_UNKNOWN;
      eh->gotoff_ref = 0;
      eh->has_got_reloc = 0;
      eh->has_non_got_reloc = 0;
      eh->tls_get_addr = 2;
      eh->func_pointer_refcount = 0;
      eh->plt_got.offset = (bfd_vma) -1;
      eh->tlsdesc_got = (bfd_vma) -1;
    }

  return entry;
}

/* Compute a hash of a local hash entry.  We use elf_link_hash_entry
  for local symbol so that we can handle local STT_GNU_IFUNC symbols
  as global symbol.  We reuse indx and dynstr_index for local symbol
  hash since they aren't used by global symbols in this backend.  */

static hashval_t
elf_i386_local_htab_hash (const void *ptr)
{
  struct elf_link_hash_entry *h
    = (struct elf_link_hash_entry *) ptr;
  return ELF_LOCAL_SYMBOL_HASH (h->indx, h->dynstr_index);
}

/* Compare local hash entries.  */

static int
elf_i386_local_htab_eq (const void *ptr1, const void *ptr2)
{
  struct elf_link_hash_entry *h1
     = (struct elf_link_hash_entry *) ptr1;
  struct elf_link_hash_entry *h2
    = (struct elf_link_hash_entry *) ptr2;

  return h1->indx == h2->indx && h1->dynstr_index == h2->dynstr_index;
}

/* Find and/or create a hash entry for local symbol.  */

static struct elf_link_hash_entry *
elf_i386_get_local_sym_hash (struct elf_i386_link_hash_table *htab,
			     bfd *abfd, const Elf_Internal_Rela *rel,
			     bfd_boolean create)
{
  struct elf_i386_link_hash_entry e, *ret;
  asection *sec = abfd->sections;
  hashval_t h = ELF_LOCAL_SYMBOL_HASH (sec->id,
				       ELF32_R_SYM (rel->r_info));
  void **slot;

  e.elf.indx = sec->id;
  e.elf.dynstr_index = ELF32_R_SYM (rel->r_info);
  slot = htab_find_slot_with_hash (htab->loc_hash_table, &e, h,
				   create ? INSERT : NO_INSERT);

  if (!slot)
    return NULL;

  if (*slot)
    {
      ret = (struct elf_i386_link_hash_entry *) *slot;
      return &ret->elf;
    }

  ret = (struct elf_i386_link_hash_entry *)
	objalloc_alloc ((struct objalloc *) htab->loc_hash_memory,
			sizeof (struct elf_i386_link_hash_entry));
  if (ret)
    {
      memset (ret, 0, sizeof (*ret));
      ret->elf.indx = sec->id;
      ret->elf.dynstr_index = ELF32_R_SYM (rel->r_info);
      ret->elf.dynindx = -1;
      ret->func_pointer_refcount = 0;
      ret->plt_got.offset = (bfd_vma) -1;
      *slot = ret;
    }
  return &ret->elf;
}

/* Destroy an i386 ELF linker hash table.  */

static void
elf_i386_link_hash_table_free (bfd *obfd)
{
  struct elf_i386_link_hash_table *htab
    = (struct elf_i386_link_hash_table *) obfd->link.hash;

  if (htab->loc_hash_table)
    htab_delete (htab->loc_hash_table);
  if (htab->loc_hash_memory)
    objalloc_free ((struct objalloc *) htab->loc_hash_memory);
  _bfd_elf_link_hash_table_free (obfd);
}

/* Create an i386 ELF linker hash table.  */

static struct bfd_link_hash_table *
elf_i386_link_hash_table_create (bfd *abfd)
{
  struct elf_i386_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct elf_i386_link_hash_table);

  ret = (struct elf_i386_link_hash_table *) bfd_zmalloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->elf, abfd,
				      elf_i386_link_hash_newfunc,
				      sizeof (struct elf_i386_link_hash_entry),
				      I386_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  ret->loc_hash_table = htab_try_create (1024,
					 elf_i386_local_htab_hash,
					 elf_i386_local_htab_eq,
					 NULL);
  ret->loc_hash_memory = objalloc_create ();
  if (!ret->loc_hash_table || !ret->loc_hash_memory)
    {
      elf_i386_link_hash_table_free (abfd);
      return NULL;
    }
  ret->elf.root.hash_table_free = elf_i386_link_hash_table_free;

  return &ret->elf.root;
}

/* Create .plt, .rel.plt, .got, .got.plt, .rel.got, .dynbss, and
   .rel.bss sections in DYNOBJ, and set up shortcuts to them in our
   hash table.  */

static bfd_boolean
elf_i386_create_dynamic_sections (bfd *dynobj, struct bfd_link_info *info)
{
  struct elf_i386_link_hash_table *htab;

  if (!_bfd_elf_create_dynamic_sections (dynobj, info))
    return FALSE;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  /* Set the contents of the .interp section to the interpreter.  */
  if (bfd_link_executable (info) && !info->nointerp)
    {
      asection *s = bfd_get_linker_section (dynobj, ".interp");
      if (s == NULL)
	abort ();
      s->size = sizeof ELF_DYNAMIC_INTERPRETER;
      s->contents = (unsigned char *) ELF_DYNAMIC_INTERPRETER;
      htab->interp = s;
    }

  if (get_elf_i386_backend_data (dynobj)->is_vxworks
      && !elf_vxworks_create_dynamic_sections (dynobj, info,
					       &htab->srelplt2))
    return FALSE;

  if (!info->no_ld_generated_unwind_info
      && htab->plt_eh_frame == NULL
      && htab->elf.splt != NULL)
    {
      flagword flags = (SEC_ALLOC | SEC_LOAD | SEC_READONLY
			| SEC_HAS_CONTENTS | SEC_IN_MEMORY
			| SEC_LINKER_CREATED);
      htab->plt_eh_frame
	= bfd_make_section_anyway_with_flags (dynobj, ".eh_frame", flags);
      if (htab->plt_eh_frame == NULL
	  || !bfd_set_section_alignment (dynobj, htab->plt_eh_frame, 2))
	return FALSE;
    }

  return TRUE;
}

/* Copy the extra info we tack onto an elf_link_hash_entry.  */

static void
elf_i386_copy_indirect_symbol (struct bfd_link_info *info,
			       struct elf_link_hash_entry *dir,
			       struct elf_link_hash_entry *ind)
{
  struct elf_i386_link_hash_entry *edir, *eind;

  edir = (struct elf_i386_link_hash_entry *) dir;
  eind = (struct elf_i386_link_hash_entry *) ind;

  if (eind->dyn_relocs != NULL)
    {
      if (edir->dyn_relocs != NULL)
	{
	  struct elf_dyn_relocs **pp;
	  struct elf_dyn_relocs *p;

	  /* Add reloc counts against the indirect sym to the direct sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->dyn_relocs; (p = *pp) != NULL; )
	    {
	      struct elf_dyn_relocs *q;

	      for (q = edir->dyn_relocs; q != NULL; q = q->next)
		if (q->sec == p->sec)
		  {
		    q->pc_count += p->pc_count;
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->dyn_relocs;
	}

      edir->dyn_relocs = eind->dyn_relocs;
      eind->dyn_relocs = NULL;
    }

  if (ind->root.type == bfd_link_hash_indirect
      && dir->got.refcount <= 0)
    {
      edir->tls_type = eind->tls_type;
      eind->tls_type = GOT_UNKNOWN;
    }

  /* Copy gotoff_ref so that elf_i386_adjust_dynamic_symbol will
     generate a R_386_COPY reloc.  */
  edir->gotoff_ref |= eind->gotoff_ref;

  edir->has_got_reloc |= eind->has_got_reloc;
  edir->has_non_got_reloc |= eind->has_non_got_reloc;

  if (ELIMINATE_COPY_RELOCS
      && ind->root.type != bfd_link_hash_indirect
      && dir->dynamic_adjusted)
    {
      /* If called to transfer flags for a weakdef during processing
	 of elf_adjust_dynamic_symbol, don't copy non_got_ref.
	 We clear it ourselves for ELIMINATE_COPY_RELOCS.  */
      if (dir->versioned != versioned_hidden)
	dir->ref_dynamic |= ind->ref_dynamic;
      dir->ref_regular |= ind->ref_regular;
      dir->ref_regular_nonweak |= ind->ref_regular_nonweak;
      dir->needs_plt |= ind->needs_plt;
      dir->pointer_equality_needed |= ind->pointer_equality_needed;
    }
  else
    {
      if (eind->func_pointer_refcount > 0)
	{
	  edir->func_pointer_refcount += eind->func_pointer_refcount;
	  eind->func_pointer_refcount = 0;
	}

      _bfd_elf_link_hash_copy_indirect (info, dir, ind);
    }
}

/* Return TRUE if the TLS access code sequence support transition
   from R_TYPE.  */

static bfd_boolean
elf_i386_check_tls_transition (asection *sec,
			       bfd_byte *contents,
			       Elf_Internal_Shdr *symtab_hdr,
			       struct elf_link_hash_entry **sym_hashes,
			       unsigned int r_type,
			       const Elf_Internal_Rela *rel,
			       const Elf_Internal_Rela *relend)
{
  unsigned int val, type, reg;
  unsigned long r_symndx;
  struct elf_link_hash_entry *h;
  bfd_vma offset;
  bfd_byte *call;
  bfd_boolean indirect_call, tls_get_addr;

  offset = rel->r_offset;
  switch (r_type)
    {
    case R_386_TLS_GD:
    case R_386_TLS_LDM:
      if (offset < 2 || (rel + 1) >= relend)
	return FALSE;

      indirect_call = FALSE;
      call = contents + offset + 4;
      val = *(call - 5);
      type = *(call - 6);
      if (r_type == R_386_TLS_GD)
	{
	  /* Check transition from GD access model.  Only
		leal foo@tlsgd(,%ebx,1), %eax
		call ___tls_get_addr@PLT
	     or
		leal foo@tlsgd(%ebx) %eax
		call ___tls_get_addr@PLT
		nop
	     or
		leal foo@tlsgd(%reg), %eax
		call *___tls_get_addr@GOT(%reg)
		which may be converted to
		addr32 call ___tls_get_addr
	     can transit to different access model.  */
	  if ((offset + 10) > sec->size
	      || (type != 0x8d && type != 0x04))
	    return FALSE;

	  if (type == 0x04)
	    {
	      /* leal foo@tlsgd(,%ebx,1), %eax
		 call ___tls_get_addr@PLT  */
	      if (offset < 3)
		return FALSE;

	      if (*(call - 7) != 0x8d
		  || val != 0x1d
		  || call[0] != 0xe8)
		return FALSE;
	    }
	  else
	    {
	      /* This must be
			leal foo@tlsgd(%ebx), %eax
			call ___tls_get_addr@PLT
			nop
		 or
			leal foo@tlsgd(%reg), %eax
			call *___tls_get_addr@GOT(%reg)
			which may be converted to
			addr32 call ___tls_get_addr

		 %eax can't be used as the GOT base register since it
		 is used to pass parameter to ___tls_get_addr.  */
	      reg = val & 7;
	      if ((val & 0xf8) != 0x80 || reg == 4 || reg == 0)
		return FALSE;

	      indirect_call = call[0] == 0xff;
	      if (!(reg == 3 && call[0] == 0xe8 && call[5] == 0x90)
		  && !(call[0] == 0x67 && call[1] == 0xe8)
		  && !(indirect_call
		       && (call[1] & 0xf8) == 0x90
		       && (call[1] & 0x7) == reg))
		return FALSE;
	    }
	}
      else
	{
	  /* Check transition from LD access model.  Only
		leal foo@tlsldm(%ebx), %eax
		call ___tls_get_addr@PLT
	     or
		leal foo@tlsldm(%reg), %eax
		call *___tls_get_addr@GOT(%reg)
		which may be converted to
		addr32 call ___tls_get_addr
	     can transit to different access model.  */
	  if (type != 0x8d || (offset + 9) > sec->size)
	    return FALSE;

	  /* %eax can't be used as the GOT base register since it is
	     used to pass parameter to ___tls_get_addr.  */
	  reg = val & 7;
	  if ((val & 0xf8) != 0x80 || reg == 4 || reg == 0)
	    return FALSE;

	  indirect_call = call[0] == 0xff;
	  if (!(reg == 3 && call[0] == 0xe8)
	      && !(call[0] == 0x67 && call[1] == 0xe8)
	      && !(indirect_call
		   && (call[1] & 0xf8) == 0x90
		   && (call[1] & 0x7) == reg))
	    return FALSE;
	}

      r_symndx = ELF32_R_SYM (rel[1].r_info);
      if (r_symndx < symtab_hdr->sh_info)
	return FALSE;

      tls_get_addr = FALSE;
      h = sym_hashes[r_symndx - symtab_hdr->sh_info];
      if (h != NULL && h->root.root.string != NULL)
	{
	  struct elf_i386_link_hash_entry *eh
	    = (struct elf_i386_link_hash_entry *) h;
	  tls_get_addr = eh->tls_get_addr == 1;
	  if (eh->tls_get_addr > 1)
	    {
	      /* Use strncmp to check ___tls_get_addr since
		 ___tls_get_addr may be versioned.  */
	      if (strncmp (h->root.root.string, "___tls_get_addr", 15)
		  == 0)
		{
		  eh->tls_get_addr = 1;
		  tls_get_addr = TRUE;
		}
	      else
		eh->tls_get_addr = 0;
	    }
	}

      if (!tls_get_addr)
	return FALSE;
      else if (indirect_call)
	return (ELF32_R_TYPE (rel[1].r_info) == R_386_GOT32X);
      else
	return (ELF32_R_TYPE (rel[1].r_info) == R_386_PC32
		|| ELF32_R_TYPE (rel[1].r_info) == R_386_PLT32);

    case R_386_TLS_IE:
      /* Check transition from IE access model:
		movl foo@indntpoff(%rip), %eax
		movl foo@indntpoff(%rip), %reg
		addl foo@indntpoff(%rip), %reg
       */

      if (offset < 1 || (offset + 4) > sec->size)
	return FALSE;

      /* Check "movl foo@tpoff(%rip), %eax" first.  */
      val = bfd_get_8 (abfd, contents + offset - 1);
      if (val == 0xa1)
	return TRUE;

      if (offset < 2)
	return FALSE;

      /* Check movl|addl foo@tpoff(%rip), %reg.   */
      type = bfd_get_8 (abfd, contents + offset - 2);
      return ((type == 0x8b || type == 0x03)
	      && (val & 0xc7) == 0x05);

    case R_386_TLS_GOTIE:
    case R_386_TLS_IE_32:
      /* Check transition from {IE_32,GOTIE} access model:
		subl foo@{tpoff,gontoff}(%reg1), %reg2
		movl foo@{tpoff,gontoff}(%reg1), %reg2
		addl foo@{tpoff,gontoff}(%reg1), %reg2
       */

      if (offset < 2 || (offset + 4) > sec->size)
	return FALSE;

      val = bfd_get_8 (abfd, contents + offset - 1);
      if ((val & 0xc0) != 0x80 || (val & 7) == 4)
	return FALSE;

      type = bfd_get_8 (abfd, contents + offset - 2);
      return type == 0x8b || type == 0x2b || type == 0x03;

    case R_386_TLS_GOTDESC:
      /* Check transition from GDesc access model:
		leal x@tlsdesc(%ebx), %eax

	 Make sure it's a leal adding ebx to a 32-bit offset
	 into any register, although it's probably almost always
	 going to be eax.  */

      if (offset < 2 || (offset + 4) > sec->size)
	return FALSE;

      if (bfd_get_8 (abfd, contents + offset - 2) != 0x8d)
	return FALSE;

      val = bfd_get_8 (abfd, contents + offset - 1);
      return (val & 0xc7) == 0x83;

    case R_386_TLS_DESC_CALL:
      /* Check transition from GDesc access model:
		call *x@tlsdesc(%eax)
       */
      if (offset + 2 <= sec->size)
	{
	  /* Make sure that it's a call *x@tlsdesc(%eax).  */
	  call = contents + offset;
	  return call[0] == 0xff && call[1] == 0x10;
	}

      return FALSE;

    default:
      abort ();
    }
}

/* Return TRUE if the TLS access transition is OK or no transition
   will be performed.  Update R_TYPE if there is a transition.  */

static bfd_boolean
elf_i386_tls_transition (struct bfd_link_info *info, bfd *abfd,
			 asection *sec, bfd_byte *contents,
			 Elf_Internal_Shdr *symtab_hdr,
			 struct elf_link_hash_entry **sym_hashes,
			 unsigned int *r_type, int tls_type,
			 const Elf_Internal_Rela *rel,
			 const Elf_Internal_Rela *relend,
			 struct elf_link_hash_entry *h,
			 unsigned long r_symndx,
			 bfd_boolean from_relocate_section)
{
  unsigned int from_type = *r_type;
  unsigned int to_type = from_type;
  bfd_boolean check = TRUE;

  /* Skip TLS transition for functions.  */
  if (h != NULL
      && (h->type == STT_FUNC
	  || h->type == STT_GNU_IFUNC))
    return TRUE;

  switch (from_type)
    {
    case R_386_TLS_GD:
    case R_386_TLS_GOTDESC:
    case R_386_TLS_DESC_CALL:
    case R_386_TLS_IE_32:
    case R_386_TLS_IE:
    case R_386_TLS_GOTIE:
      if (bfd_link_executable (info))
	{
	  if (h == NULL)
	    to_type = R_386_TLS_LE_32;
	  else if (from_type != R_386_TLS_IE
		   && from_type != R_386_TLS_GOTIE)
	    to_type = R_386_TLS_IE_32;
	}

      /* When we are called from elf_i386_relocate_section, there may
	 be additional transitions based on TLS_TYPE.  */
      if (from_relocate_section)
	{
	  unsigned int new_to_type = to_type;

	  if (bfd_link_executable (info)
	      && h != NULL
	      && h->dynindx == -1
	      && (tls_type & GOT_TLS_IE))
	    new_to_type = R_386_TLS_LE_32;

	  if (to_type == R_386_TLS_GD
	      || to_type == R_386_TLS_GOTDESC
	      || to_type == R_386_TLS_DESC_CALL)
	    {
	      if (tls_type == GOT_TLS_IE_POS)
		new_to_type = R_386_TLS_GOTIE;
	      else if (tls_type & GOT_TLS_IE)
		new_to_type = R_386_TLS_IE_32;
	    }

	  /* We checked the transition before when we were called from
	     elf_i386_check_relocs.  We only want to check the new
	     transition which hasn't been checked before.  */
	  check = new_to_type != to_type && from_type == to_type;
	  to_type = new_to_type;
	}

      break;

    case R_386_TLS_LDM:
      if (bfd_link_executable (info))
	to_type = R_386_TLS_LE_32;
      break;

    default:
      return TRUE;
    }

  /* Return TRUE if there is no transition.  */
  if (from_type == to_type)
    return TRUE;

  /* Check if the transition can be performed.  */
  if (check
      && ! elf_i386_check_tls_transition (sec, contents,
					  symtab_hdr, sym_hashes,
					  from_type, rel, relend))
    {
      reloc_howto_type *from, *to;
      const char *name;

      from = elf_i386_rtype_to_howto (abfd, from_type);
      to = elf_i386_rtype_to_howto (abfd, to_type);

      if (h)
	name = h->root.root.string;
      else
	{
	  struct elf_i386_link_hash_table *htab;

	  htab = elf_i386_hash_table (info);
	  if (htab == NULL)
	    name = "*unknown*";
	  else
	    {
	      Elf_Internal_Sym *isym;

	      isym = bfd_sym_from_r_symndx (&htab->sym_cache,
					    abfd, r_symndx);
	      name = bfd_elf_sym_name (abfd, symtab_hdr, isym, NULL);
	    }
	}

      _bfd_error_handler
	/* xgettext:c-format */
	(_("%B: TLS transition from %s to %s against `%s' at 0x%lx "
	   "in section `%A' failed"),
	 abfd, from->name, to->name, name,
	 (unsigned long) rel->r_offset, sec);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  *r_type = to_type;
  return TRUE;
}

/* With the local symbol, foo, we convert
   mov foo@GOT[(%reg1)], %reg2
   to
   lea foo[@GOTOFF(%reg1)], %reg2
   and convert
   call/jmp *foo@GOT[(%reg)]
   to
   nop call foo/jmp foo nop
   When PIC is false, convert
   test %reg1, foo@GOT[(%reg2)]
   to
   test $foo, %reg1
   and convert
   binop foo@GOT[(%reg1)], %reg2
   to
   binop $foo, %reg2
   where binop is one of adc, add, and, cmp, or, sbb, sub, xor
   instructions.  */

static
bfd_boolean
elf_i386_convert_load_reloc (bfd *abfd, Elf_Internal_Shdr *symtab_hdr,
			     bfd_byte *contents,
			     Elf_Internal_Rela *irel,
			     struct elf_link_hash_entry *h,
			     bfd_boolean *converted,
			     struct bfd_link_info *link_info)
{
  struct elf_i386_link_hash_table *htab;
  unsigned int opcode;
  unsigned int modrm;
  bfd_boolean baseless;
  Elf_Internal_Sym *isym;
  unsigned int addend;
  unsigned int nop;
  bfd_vma nop_offset;
  bfd_boolean is_pic;
  bfd_boolean to_reloc_32;
  unsigned int r_type;
  unsigned int r_symndx;
  bfd_vma roff = irel->r_offset;

  if (roff < 2)
    return TRUE;

  /* Addend for R_386_GOT32X relocations must be 0.  */
  addend = bfd_get_32 (abfd, contents + roff);
  if (addend != 0)
    return TRUE;

  htab = elf_i386_hash_table (link_info);
  is_pic = bfd_link_pic (link_info);

  r_type = ELF32_R_TYPE (irel->r_info);
  r_symndx = ELF32_R_SYM (irel->r_info);

  modrm = bfd_get_8 (abfd, contents + roff - 1);
  baseless = (modrm & 0xc7) == 0x5;

  if (baseless && is_pic)
    {
      /* For PIC, disallow R_386_GOT32X without a base register
	 since we don't know what the GOT base is.  */
      const char *name;

      if (h == NULL)
	{
	  isym = bfd_sym_from_r_symndx (&htab->sym_cache, abfd,
					r_symndx);
	  name = bfd_elf_sym_name (abfd, symtab_hdr, isym, NULL);
	}
      else
	name = h->root.root.string;

      _bfd_error_handler
	/* xgettext:c-format */
	(_("%B: direct GOT relocation R_386_GOT32X against `%s' without base"
	   " register can not be used when making a shared object"),
	 abfd, name);
      return FALSE;
    }

  opcode = bfd_get_8 (abfd, contents + roff - 2);

  /* Convert to R_386_32 if PIC is false or there is no base
     register.  */
  to_reloc_32 = !is_pic || baseless;

  /* Try to convert R_386_GOT32X.  Get the symbol referred to by the
     reloc.  */
  if (h == NULL)
    {
      if (opcode == 0x0ff)
	/* Convert "call/jmp *foo@GOT[(%reg)]".  */
	goto convert_branch;
      else
	/* Convert "mov foo@GOT[(%reg1)], %reg2",
	   "test %reg1, foo@GOT(%reg2)" and
	   "binop foo@GOT[(%reg1)], %reg2". */
	goto convert_load;
    }

  /* Undefined weak symbol is only bound locally in executable
     and its reference is resolved as 0.  */
  if (UNDEFINED_WEAK_RESOLVED_TO_ZERO (link_info, TRUE,
				       elf_i386_hash_entry (h)))
    {
      if (opcode == 0xff)
	{
	  /* No direct branch to 0 for PIC.  */
	  if (is_pic)
	    return TRUE;
	  else
	    goto convert_branch;
	}
      else
	{
	  /* We can convert load of address 0 to R_386_32.  */
	  to_reloc_32 = TRUE;
	  goto convert_load;
	}
    }

  if (opcode == 0xff)
    {
      /* We have "call/jmp *foo@GOT[(%reg)]".  */
      if ((h->root.type == bfd_link_hash_defined
	   || h->root.type == bfd_link_hash_defweak)
	  && SYMBOL_REFERENCES_LOCAL (link_info, h))
	{
	  /* The function is locally defined.   */
convert_branch:
	  /* Convert R_386_GOT32X to R_386_PC32.  */
	  if (modrm == 0x15 || (modrm & 0xf8) == 0x90)
	    {
	      struct elf_i386_link_hash_entry *eh
		= (struct elf_i386_link_hash_entry *) h;

	      /* Convert to "nop call foo".  ADDR_PREFIX_OPCODE
		 is a nop prefix.  */
	      modrm = 0xe8;
	      /* To support TLS optimization, always use addr32 prefix
		 for "call *___tls_get_addr@GOT(%reg)".  */
	      if (eh && eh->tls_get_addr == 1)
		{
		  nop = 0x67;
		  nop_offset = irel->r_offset - 2;
		}
	      else
		{
		  nop = link_info->call_nop_byte;
		  if (link_info->call_nop_as_suffix)
		    {
		      nop_offset = roff + 3;
		      irel->r_offset -= 1;
		    }
		  else
		    nop_offset = roff - 2;
		}
	    }
	  else
	    {
	      /* Convert to "jmp foo nop".  */
	      modrm = 0xe9;
	      nop = NOP_OPCODE;
	      nop_offset = roff + 3;
	      irel->r_offset -= 1;
	    }

	  bfd_put_8 (abfd, nop, contents + nop_offset);
	  bfd_put_8 (abfd, modrm, contents + irel->r_offset - 1);
	  /* When converting to PC-relative relocation, we
	     need to adjust addend by -4.  */
	  bfd_put_32 (abfd, -4, contents + irel->r_offset);
	  irel->r_info = ELF32_R_INFO (r_symndx, R_386_PC32);

	  *converted = TRUE;
	}
    }
  else
    {
      /* We have "mov foo@GOT[(%re1g)], %reg2",
	 "test %reg1, foo@GOT(%reg2)" and
	 "binop foo@GOT[(%reg1)], %reg2".

	 Avoid optimizing _DYNAMIC since ld.so may use its
	 link-time address.  */
      if (h == htab->elf.hdynamic)
	return TRUE;

      /* def_regular is set by an assignment in a linker script in
	 bfd_elf_record_link_assignment.  */
      if ((h->def_regular
	   || h->root.type == bfd_link_hash_defined
	   || h->root.type == bfd_link_hash_defweak)
	  && SYMBOL_REFERENCES_LOCAL (link_info, h))
	{
convert_load:
	  if (opcode == 0x8b)
	    {
	      if (to_reloc_32)
		{
		  /* Convert "mov foo@GOT[(%reg1)], %reg2" to
		     "mov $foo, %reg2" with R_386_32.  */
		  r_type = R_386_32;
		  modrm = 0xc0 | (modrm & 0x38) >> 3;
		  bfd_put_8 (abfd, modrm, contents + roff - 1);
		  opcode = 0xc7;
		}
	      else
		{
		  /* Convert "mov foo@GOT(%reg1), %reg2" to
		     "lea foo@GOTOFF(%reg1), %reg2".  */
		  r_type = R_386_GOTOFF;
		  opcode = 0x8d;
		}
	    }
	  else
	    {
	      /* Only R_386_32 is supported.  */
	      if (!to_reloc_32)
		return TRUE;

	      if (opcode == 0x85)
		{
		  /* Convert "test %reg1, foo@GOT(%reg2)" to
		     "test $foo, %reg1".  */
		  modrm = 0xc0 | (modrm & 0x38) >> 3;
		  opcode = 0xf7;
		}
	      else
		{
		  /* Convert "binop foo@GOT(%reg1), %reg2" to
		     "binop $foo, %reg2".  */
		  modrm = (0xc0
			   | (modrm & 0x38) >> 3
			   | (opcode & 0x3c));
		  opcode = 0x81;
		}
	      bfd_put_8 (abfd, modrm, contents + roff - 1);
	      r_type = R_386_32;
	    }

	  bfd_put_8 (abfd, opcode, contents + roff - 2);
	  irel->r_info = ELF32_R_INFO (r_symndx, r_type);

	  *converted = TRUE;
	}
    }

  return TRUE;
}

/* Rename some of the generic section flags to better document how they
   are used here.  */
#define need_convert_load	sec_flg0
#define check_relocs_failed	sec_flg1

/* Look through the relocs for a section during the first phase, and
   calculate needed space in the global offset table, procedure linkage
   table, and dynamic reloc sections.  */

static bfd_boolean
elf_i386_check_relocs (bfd *abfd,
		       struct bfd_link_info *info,
		       asection *sec,
		       const Elf_Internal_Rela *relocs)
{
  struct elf_i386_link_hash_table *htab;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  asection *sreloc;
  bfd_byte *contents;
  bfd_boolean use_plt_got;

  if (bfd_link_relocatable (info))
    return TRUE;

  /* Don't do anything special with non-loaded, non-alloced sections.
     In particular, any relocs in such sections should not affect GOT
     and PLT reference counting (ie. we don't allow them to create GOT
     or PLT entries), there's no possibility or desire to optimize TLS
     relocs, and there's not much point in propagating relocs to shared
     libs that the dynamic linker won't relocate.  */
  if ((sec->flags & SEC_ALLOC) == 0)
    return TRUE;

  BFD_ASSERT (is_i386_elf (abfd));

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    {
      sec->check_relocs_failed = 1;
      return FALSE;
    }

  /* Get the section contents.  */
  if (elf_section_data (sec)->this_hdr.contents != NULL)
    contents = elf_section_data (sec)->this_hdr.contents;
  else if (!bfd_malloc_and_get_section (abfd, sec, &contents))
    {
      sec->check_relocs_failed = 1;
      return FALSE;
    }

  use_plt_got = (!get_elf_i386_backend_data (abfd)->is_vxworks
		 && (get_elf_i386_backend_data (abfd)
		     == &elf_i386_arch_bed));

  symtab_hdr = &elf_symtab_hdr (abfd);
  sym_hashes = elf_sym_hashes (abfd);

  sreloc = NULL;

  rel_end = relocs + sec->reloc_count;
  for (rel = relocs; rel < rel_end; rel++)
    {
      unsigned int r_type;
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;
      struct elf_i386_link_hash_entry *eh;
      Elf_Internal_Sym *isym;
      const char *name;
      bfd_boolean size_reloc;

      r_symndx = ELF32_R_SYM (rel->r_info);
      r_type = ELF32_R_TYPE (rel->r_info);

      if (r_symndx >= NUM_SHDR_ENTRIES (symtab_hdr))
	{
	  /* xgettext:c-format */
	  _bfd_error_handler (_("%B: bad symbol index: %d"),
			      abfd, r_symndx);
	  goto error_return;
	}

      if (r_symndx < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  isym = bfd_sym_from_r_symndx (&htab->sym_cache,
					abfd, r_symndx);
	  if (isym == NULL)
	    goto error_return;

	  /* Check relocation against local STT_GNU_IFUNC symbol.  */
	  if (ELF32_ST_TYPE (isym->st_info) == STT_GNU_IFUNC)
	    {
	      h = elf_i386_get_local_sym_hash (htab, abfd, rel, TRUE);
	      if (h == NULL)
		goto error_return;

	      /* Fake a STT_GNU_IFUNC symbol.  */
	      h->type = STT_GNU_IFUNC;
	      h->def_regular = 1;
	      h->ref_regular = 1;
	      h->forced_local = 1;
	      h->root.type = bfd_link_hash_defined;
	    }
	  else
	    h = NULL;
	}
      else
	{
	  isym = NULL;
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      eh = (struct elf_i386_link_hash_entry *) h;
      if (h != NULL)
	{
	  switch (r_type)
	    {
	    default:
	      break;

	    case R_386_GOTOFF:
	      eh->gotoff_ref = 1;
	      /* Fall through.  */
	    case R_386_32:
	    case R_386_PC32:
	    case R_386_PLT32:
	    case R_386_GOT32:
	    case R_386_GOT32X:
	      if (htab->elf.dynobj == NULL)
		htab->elf.dynobj = abfd;
	      /* Create the ifunc sections for static executables.  */
	      if (h->type == STT_GNU_IFUNC
		  && !_bfd_elf_create_ifunc_sections (htab->elf.dynobj,
						      info))
		goto error_return;
	      break;
	    }

	  /* It is referenced by a non-shared object. */
	  h->ref_regular = 1;
	  h->root.non_ir_ref = 1;

	  if (h->type == STT_GNU_IFUNC)
	    elf_tdata (info->output_bfd)->has_gnu_symbols
	      |= elf_gnu_symbol_ifunc;
	}

      if (! elf_i386_tls_transition (info, abfd, sec, contents,
				     symtab_hdr, sym_hashes,
				     &r_type, GOT_UNKNOWN,
				     rel, rel_end, h, r_symndx, FALSE))
	goto error_return;

      switch (r_type)
	{
	case R_386_TLS_LDM:
	  htab->tls_ldm_got.refcount += 1;
	  goto create_got;

	case R_386_PLT32:
	  /* This symbol requires a procedure linkage table entry.  We
	     actually build the entry in adjust_dynamic_symbol,
	     because this might be a case of linking PIC code which is
	     never referenced by a dynamic object, in which case we
	     don't need to generate a procedure linkage table entry
	     after all.  */

	  /* If this is a local symbol, we resolve it directly without
	     creating a procedure linkage table entry.  */
	  if (h == NULL)
	    continue;

	  eh->has_got_reloc = 1;
	  h->needs_plt = 1;
	  h->plt.refcount += 1;
	  break;

	case R_386_SIZE32:
	  size_reloc = TRUE;
	  goto do_size;

	case R_386_TLS_IE_32:
	case R_386_TLS_IE:
	case R_386_TLS_GOTIE:
	  if (!bfd_link_executable (info))
	    info->flags |= DF_STATIC_TLS;
	  /* Fall through */

	case R_386_GOT32:
	case R_386_GOT32X:
	case R_386_TLS_GD:
	case R_386_TLS_GOTDESC:
	case R_386_TLS_DESC_CALL:
	  /* This symbol requires a global offset table entry.  */
	  {
	    int tls_type, old_tls_type;

	    switch (r_type)
	      {
	      default:
	      case R_386_GOT32:
	      case R_386_GOT32X:
		tls_type = GOT_NORMAL;
		break;
	      case R_386_TLS_GD: tls_type = GOT_TLS_GD; break;
	      case R_386_TLS_GOTDESC:
	      case R_386_TLS_DESC_CALL:
		tls_type = GOT_TLS_GDESC; break;
	      case R_386_TLS_IE_32:
		if (ELF32_R_TYPE (rel->r_info) == r_type)
		  tls_type = GOT_TLS_IE_NEG;
		else
		  /* If this is a GD->IE transition, we may use either of
		     R_386_TLS_TPOFF and R_386_TLS_TPOFF32.  */
		  tls_type = GOT_TLS_IE;
		break;
	      case R_386_TLS_IE:
	      case R_386_TLS_GOTIE:
		tls_type = GOT_TLS_IE_POS; break;
	      }

	    if (h != NULL)
	      {
		h->got.refcount += 1;
		old_tls_type = elf_i386_hash_entry(h)->tls_type;
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
		    size *= (sizeof (bfd_signed_vma)
			     + sizeof (bfd_vma) + sizeof(char));
		    local_got_refcounts = (bfd_signed_vma *)
                        bfd_zalloc (abfd, size);
		    if (local_got_refcounts == NULL)
		      goto error_return;
		    elf_local_got_refcounts (abfd) = local_got_refcounts;
		    elf_i386_local_tlsdesc_gotent (abfd)
		      = (bfd_vma *) (local_got_refcounts + symtab_hdr->sh_info);
		    elf_i386_local_got_tls_type (abfd)
		      = (char *) (local_got_refcounts + 2 * symtab_hdr->sh_info);
		  }
		local_got_refcounts[r_symndx] += 1;
		old_tls_type = elf_i386_local_got_tls_type (abfd) [r_symndx];
	      }

	    if ((old_tls_type & GOT_TLS_IE) && (tls_type & GOT_TLS_IE))
	      tls_type |= old_tls_type;
	    /* If a TLS symbol is accessed using IE at least once,
	       there is no point to use dynamic model for it.  */
	    else if (old_tls_type != tls_type && old_tls_type != GOT_UNKNOWN
		     && (! GOT_TLS_GD_ANY_P (old_tls_type)
			 || (tls_type & GOT_TLS_IE) == 0))
	      {
		if ((old_tls_type & GOT_TLS_IE) && GOT_TLS_GD_ANY_P (tls_type))
		  tls_type = old_tls_type;
		else if (GOT_TLS_GD_ANY_P (old_tls_type)
			 && GOT_TLS_GD_ANY_P (tls_type))
		  tls_type |= old_tls_type;
		else
		  {
		    if (h)
		      name = h->root.root.string;
		    else
		      name = bfd_elf_sym_name (abfd, symtab_hdr, isym,
					     NULL);
		    _bfd_error_handler
		      /* xgettext:c-format */
		      (_("%B: `%s' accessed both as normal and "
			 "thread local symbol"),
		       abfd, name);
		    bfd_set_error (bfd_error_bad_value);
		    goto error_return;
		  }
	      }

	    if (old_tls_type != tls_type)
	      {
		if (h != NULL)
		  elf_i386_hash_entry (h)->tls_type = tls_type;
		else
		  elf_i386_local_got_tls_type (abfd) [r_symndx] = tls_type;
	      }
	  }
	  /* Fall through */

	case R_386_GOTOFF:
	case R_386_GOTPC:
	create_got:
	  if (htab->elf.sgot == NULL)
	    {
	      if (htab->elf.dynobj == NULL)
		htab->elf.dynobj = abfd;
	      if (!_bfd_elf_create_got_section (htab->elf.dynobj, info))
		goto error_return;
	    }
	  if (r_type != R_386_TLS_IE)
	    {
	      if (eh != NULL)
		eh->has_got_reloc = 1;
	      break;
	    }
	  /* Fall through */

	case R_386_TLS_LE_32:
	case R_386_TLS_LE:
	  if (eh != NULL)
	    eh->has_got_reloc = 1;
	  if (bfd_link_executable (info))
	    break;
	  info->flags |= DF_STATIC_TLS;
	  goto do_relocation;

	case R_386_32:
	case R_386_PC32:
	  if (eh != NULL && (sec->flags & SEC_CODE) != 0)
	    eh->has_non_got_reloc = 1;
do_relocation:
	  /* We are called after all symbols have been resolved.  Only
	     relocation against STT_GNU_IFUNC symbol must go through
	     PLT.  */
	  if (h != NULL
	      && (bfd_link_executable (info)
		  || h->type == STT_GNU_IFUNC))
	    {
	      /* If this reloc is in a read-only section, we might
		 need a copy reloc.  We can't check reliably at this
		 stage whether the section is read-only, as input
		 sections have not yet been mapped to output sections.
		 Tentatively set the flag for now, and correct in
		 adjust_dynamic_symbol.  */
	      h->non_got_ref = 1;

	      /* We may need a .plt entry if the symbol is a function
		 defined in a shared lib or is a STT_GNU_IFUNC function
		 referenced from the code or read-only section.  */
	      if (!h->def_regular
		  || (sec->flags & (SEC_CODE | SEC_READONLY)) != 0)
		h->plt.refcount += 1;

	      if (r_type == R_386_PC32)
		{
		  /* Since something like ".long foo - ." may be used
		     as pointer, make sure that PLT is used if foo is
		     a function defined in a shared library.  */
		  if ((sec->flags & SEC_CODE) == 0)
		    h->pointer_equality_needed = 1;
		  else if (h->type == STT_GNU_IFUNC
			   && bfd_link_pic (info))
		    {
		    if (isym == NULL)
		      name = h->root.root.string;
		    else
		      name = bfd_elf_sym_name (abfd, symtab_hdr, isym,
					       NULL);
		    _bfd_error_handler
		      /* xgettext:c-format */
		      (_("%B: unsupported non-PIC call to IFUNC `%s'"),
		       abfd, name);
		      bfd_set_error (bfd_error_bad_value);
		      goto error_return;
		    }
		}
	      else
		{
		  h->pointer_equality_needed = 1;
		  /* R_386_32 can be resolved at run-time.  */
		  if (r_type == R_386_32
		      && (sec->flags & SEC_READONLY) == 0)
		    eh->func_pointer_refcount += 1;
		}
	    }

	  size_reloc = FALSE;
do_size:
	  /* If we are creating a shared library, and this is a reloc
	     against a global symbol, or a non PC relative reloc
	     against a local symbol, then we need to copy the reloc
	     into the shared library.  However, if we are linking with
	     -Bsymbolic, we do not need to copy a reloc against a
	     global symbol which is defined in an object we are
	     including in the link (i.e., DEF_REGULAR is set).  At
	     this point we have not seen all the input files, so it is
	     possible that DEF_REGULAR is not set now but will be set
	     later (it is never cleared).  In case of a weak definition,
	     DEF_REGULAR may be cleared later by a strong definition in
	     a shared library.  We account for that possibility below by
	     storing information in the relocs_copied field of the hash
	     table entry.  A similar situation occurs when creating
	     shared libraries and symbol visibility changes render the
	     symbol local.

	     If on the other hand, we are creating an executable, we
	     may need to keep relocations for symbols satisfied by a
	     dynamic library if we manage to avoid copy relocs for the
	     symbol.

	     Generate dynamic pointer relocation against STT_GNU_IFUNC
	     symbol in the non-code section.  */
	  if ((bfd_link_pic (info)
	       && (r_type != R_386_PC32
		   || (h != NULL
		       && (! (bfd_link_pie (info)
			      || SYMBOLIC_BIND (info, h))
			   || h->root.type == bfd_link_hash_defweak
			   || !h->def_regular))))
	      || (h != NULL
		  && h->type == STT_GNU_IFUNC
		  && r_type == R_386_32
		  && (sec->flags & SEC_CODE) == 0)
	      || (ELIMINATE_COPY_RELOCS
		  && !bfd_link_pic (info)
		  && h != NULL
		  && (h->root.type == bfd_link_hash_defweak
		      || !h->def_regular)))
	    {
	      struct elf_dyn_relocs *p;
	      struct elf_dyn_relocs **head;

	      /* We must copy these reloc types into the output file.
		 Create a reloc section in dynobj and make room for
		 this reloc.  */
	      if (sreloc == NULL)
		{
		  if (htab->elf.dynobj == NULL)
		    htab->elf.dynobj = abfd;

		  sreloc = _bfd_elf_make_dynamic_reloc_section
		    (sec, htab->elf.dynobj, 2, abfd, /*rela?*/ FALSE);

		  if (sreloc == NULL)
		    goto error_return;
		}

	      /* If this is a global symbol, we count the number of
		 relocations we need for this symbol.  */
	      if (h != NULL)
		{
		  head = &eh->dyn_relocs;
		}
	      else
		{
		  /* Track dynamic relocs needed for local syms too.
		     We really need local syms available to do this
		     easily.  Oh well.  */
		  void **vpp;
		  asection *s;

		  isym = bfd_sym_from_r_symndx (&htab->sym_cache,
						abfd, r_symndx);
		  if (isym == NULL)
		    goto error_return;

		  s = bfd_section_from_elf_index (abfd, isym->st_shndx);
		  if (s == NULL)
		    s = sec;

		  vpp = &elf_section_data (s)->local_dynrel;
		  head = (struct elf_dyn_relocs **)vpp;
		}

	      p = *head;
	      if (p == NULL || p->sec != sec)
		{
		  bfd_size_type amt = sizeof *p;
		  p = (struct elf_dyn_relocs *) bfd_alloc (htab->elf.dynobj,
                                                           amt);
		  if (p == NULL)
		    goto error_return;
		  p->next = *head;
		  *head = p;
		  p->sec = sec;
		  p->count = 0;
		  p->pc_count = 0;
		}

	      p->count += 1;
	      /* Count size relocation as PC-relative relocation.  */
	      if (r_type == R_386_PC32 || size_reloc)
		p->pc_count += 1;
	    }
	  break;

	  /* This relocation describes the C++ object vtable hierarchy.
	     Reconstruct it for later use during GC.  */
	case R_386_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    goto error_return;
	  break;

	  /* This relocation describes which C++ vtable entries are actually
	     used.  Record for later use during GC.  */
	case R_386_GNU_VTENTRY:
	  BFD_ASSERT (h != NULL);
	  if (h != NULL
	      && !bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_offset))
	    goto error_return;
	  break;

	default:
	  break;
	}

      if (use_plt_got
	  && h != NULL
	  && h->plt.refcount > 0
	  && (((info->flags & DF_BIND_NOW) && !h->pointer_equality_needed)
	      || h->got.refcount > 0)
	  && htab->plt_got == NULL)
	{
	  /* Create the GOT procedure linkage table.  */
	  unsigned int plt_got_align;
	  const struct elf_backend_data *bed;

	  bed = get_elf_backend_data (info->output_bfd);
	  BFD_ASSERT (sizeof (elf_i386_got_plt_entry) == 8
		      && (sizeof (elf_i386_got_plt_entry)
			  == sizeof (elf_i386_pic_got_plt_entry)));
	  plt_got_align = 3;

	  if (htab->elf.dynobj == NULL)
	    htab->elf.dynobj = abfd;
	  htab->plt_got
	    = bfd_make_section_anyway_with_flags (htab->elf.dynobj,
						  ".plt.got",
						  (bed->dynamic_sec_flags
						   | SEC_ALLOC
						   | SEC_CODE
						   | SEC_LOAD
						   | SEC_READONLY));
	  if (htab->plt_got == NULL
	      || !bfd_set_section_alignment (htab->elf.dynobj,
					     htab->plt_got,
					     plt_got_align))
	    goto error_return;

	  if (!info->no_ld_generated_unwind_info
	      && htab->plt_got_eh_frame == NULL
	      && get_elf_i386_backend_data (abfd)->plt->eh_frame_plt_got != NULL)
	    {
	      flagword flags = (SEC_ALLOC | SEC_LOAD | SEC_READONLY
				| SEC_HAS_CONTENTS | SEC_IN_MEMORY
				| SEC_LINKER_CREATED);
	      htab->plt_got_eh_frame
		= bfd_make_section_anyway_with_flags (htab->elf.dynobj,
						      ".eh_frame",
						      flags);
	      if (htab->plt_got_eh_frame == NULL
		  || !bfd_set_section_alignment (htab->elf.dynobj,
						 htab->plt_got_eh_frame,
						 2))
		goto error_return;
	    }
	}

      if (r_type == R_386_GOT32X
	  && (h == NULL || h->type != STT_GNU_IFUNC))
	sec->need_convert_load = 1;
    }

  if (elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (!info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  return TRUE;

error_return:
  if (elf_section_data (sec)->this_hdr.contents != contents)
    free (contents);
  sec->check_relocs_failed = 1;
  return FALSE;
}

/* Return the section that should be marked against GC for a given
   relocation.  */

static asection *
elf_i386_gc_mark_hook (asection *sec,
		       struct bfd_link_info *info,
		       Elf_Internal_Rela *rel,
		       struct elf_link_hash_entry *h,
		       Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELF32_R_TYPE (rel->r_info))
      {
      case R_386_GNU_VTINHERIT:
      case R_386_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Remove undefined weak symbol from the dynamic symbol table if it
   is resolved to 0.   */

static bfd_boolean
elf_i386_fixup_symbol (struct bfd_link_info *info,
		       struct elf_link_hash_entry *h)
{
  if (h->dynindx != -1
      && UNDEFINED_WEAK_RESOLVED_TO_ZERO (info,
					  elf_i386_hash_entry (h)->has_got_reloc,
					  elf_i386_hash_entry (h)))
    {
      h->dynindx = -1;
      _bfd_elf_strtab_delref (elf_hash_table (info)->dynstr,
			      h->dynstr_index);
    }
  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

static bfd_boolean
elf_i386_adjust_dynamic_symbol (struct bfd_link_info *info,
				struct elf_link_hash_entry *h)
{
  struct elf_i386_link_hash_table *htab;
  asection *s, *srel;
  struct elf_i386_link_hash_entry *eh;
  struct elf_dyn_relocs *p;

  /* STT_GNU_IFUNC symbol must go through PLT. */
  if (h->type == STT_GNU_IFUNC)
    {
      /* All local STT_GNU_IFUNC references must be treate as local
	 calls via local PLT.  */
      if (h->ref_regular
	  && SYMBOL_CALLS_LOCAL (info, h))
	{
	  bfd_size_type pc_count = 0, count = 0;
	  struct elf_dyn_relocs **pp;

	  eh = (struct elf_i386_link_hash_entry *) h;
	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      pc_count += p->pc_count;
	      p->count -= p->pc_count;
	      p->pc_count = 0;
	      count += p->count;
	      if (p->count == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }

	  if (pc_count || count)
	    {
	      h->non_got_ref = 1;
	      if (pc_count)
		{
		  /* Increment PLT reference count only for PC-relative
		     references.  */
		  h->needs_plt = 1;
		  if (h->plt.refcount <= 0)
		    h->plt.refcount = 1;
		  else
		    h->plt.refcount += 1;
		}
	    }
	}

      if (h->plt.refcount <= 0)
	{
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
      return TRUE;
    }

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later,
     when we know the address of the .got section.  */
  if (h->type == STT_FUNC
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
	     linkage table, and we can just do a PC32 reloc instead.  */
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}

      return TRUE;
    }
  else
    /* It's possible that we incorrectly decided a .plt reloc was
       needed for an R_386_PC32 reloc to a non-function sym in
       check_relocs.  We can't decide accurately between function and
       non-function syms in check-relocs;  Objects loaded later in
       the link may change h->type.  So fix it now.  */
    h->plt.offset = (bfd_vma) -1;

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      if (ELIMINATE_COPY_RELOCS || info->nocopyreloc)
	h->non_got_ref = h->u.weakdef->non_got_ref;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (!bfd_link_executable (info))
    return TRUE;

  /* If there are no references to this symbol that do not use the
     GOT nor R_386_GOTOFF relocation, we don't need to generate a copy
     reloc.  */
  eh = (struct elf_i386_link_hash_entry *) h;
  if (!h->non_got_ref && !eh->gotoff_ref)
    return TRUE;

  /* If -z nocopyreloc was given, we won't generate them either.  */
  if (info->nocopyreloc)
    {
      h->non_got_ref = 0;
      return TRUE;
    }

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  /* If there aren't any dynamic relocs in read-only sections nor
     R_386_GOTOFF relocation, then we can keep the dynamic relocs and
     avoid the copy reloc.  This doesn't work on VxWorks, where we can
     not have dynamic relocations (other than copy and jump slot
     relocations) in an executable.  */
  if (ELIMINATE_COPY_RELOCS
      && !eh->gotoff_ref
      && !get_elf_i386_backend_data (info->output_bfd)->is_vxworks)
    {
      for (p = eh->dyn_relocs; p != NULL; p = p->next)
	{
	  s = p->sec->output_section;
	  if (s != NULL && (s->flags & SEC_READONLY) != 0)
	    break;
	}

      if (p == NULL)
	{
	  h->non_got_ref = 0;
	  return TRUE;
	}
    }

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  /* We must generate a R_386_COPY reloc to tell the dynamic linker to
     copy the initial value out of the dynamic object and into the
     runtime process image.  */
  if ((h->root.u.def.section->flags & SEC_READONLY) != 0)
    {
      s = htab->elf.sdynrelro;
      srel = htab->elf.sreldynrelro;
    }
  else
    {
      s = htab->elf.sdynbss;
      srel = htab->elf.srelbss;
    }
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      srel->size += sizeof (Elf32_External_Rel);
      h->needs_copy = 1;
    }

  return _bfd_elf_adjust_dynamic_copy (info, h, s);
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs.  */

static bfd_boolean
elf_i386_allocate_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct bfd_link_info *info;
  struct elf_i386_link_hash_table *htab;
  struct elf_i386_link_hash_entry *eh;
  struct elf_dyn_relocs *p;
  unsigned plt_entry_size;
  bfd_boolean resolved_to_zero;

  if (h->root.type == bfd_link_hash_indirect)
    return TRUE;

  eh = (struct elf_i386_link_hash_entry *) h;

  info = (struct bfd_link_info *) inf;
  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  plt_entry_size = GET_PLT_ENTRY_SIZE (info->output_bfd);

  resolved_to_zero = UNDEFINED_WEAK_RESOLVED_TO_ZERO (info,
						      eh->has_got_reloc,
						      eh);

  /* Clear the reference count of function pointer relocations if
     symbol isn't a normal function.  */
  if (h->type != STT_FUNC)
    eh->func_pointer_refcount = 0;

  /* We can't use the GOT PLT if pointer equality is needed since
     finish_dynamic_symbol won't clear symbol value and the dynamic
     linker won't update the GOT slot.  We will get into an infinite
     loop at run-time.  */
  if (htab->plt_got != NULL
      && h->type != STT_GNU_IFUNC
      && !h->pointer_equality_needed
      && h->plt.refcount > 0
      && h->got.refcount > 0)
    {
      /* Don't use the regular PLT if there are both GOT and GOTPLT
         reloctions.  */
      h->plt.offset = (bfd_vma) -1;

      /* Use the GOT PLT.  */
      eh->plt_got.refcount = 1;
    }

  /* Since STT_GNU_IFUNC symbol must go through PLT, we handle it
     here if it is defined and referenced in a non-shared object.  */
  if (h->type == STT_GNU_IFUNC
      && h->def_regular)
    return _bfd_elf_allocate_ifunc_dyn_relocs (info, h, &eh->dyn_relocs,
					       &htab->readonly_dynrelocs_against_ifunc,
					       plt_entry_size,
					       plt_entry_size, 4, TRUE);
  /* Don't create the PLT entry if there are only function pointer
     relocations which can be resolved at run-time.  */
  else if (htab->elf.dynamic_sections_created
	   && (h->plt.refcount > eh->func_pointer_refcount
	       || eh->plt_got.refcount > 0))
    {
      bfd_boolean use_plt_got;

      /* Clear the reference count of function pointer relocations
	 if PLT is used.  */
      eh->func_pointer_refcount = 0;

      if ((info->flags & DF_BIND_NOW) && !h->pointer_equality_needed)
	{
	  /* Don't use the regular PLT for DF_BIND_NOW. */
	  h->plt.offset = (bfd_vma) -1;

	  /* Use the GOT PLT.  */
	  h->got.refcount = 1;
	  eh->plt_got.refcount = 1;
	}

      use_plt_got = eh->plt_got.refcount > 0;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local
	  && !resolved_to_zero)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      if (bfd_link_pic (info)
	  || WILL_CALL_FINISH_DYNAMIC_SYMBOL (1, 0, h))
	{
	  asection *s = htab->elf.splt;
	  asection *got_s = htab->plt_got;

	  /* If this is the first .plt entry, make room for the special
	     first entry.  The .plt section is used by prelink to undo
	     prelinking for dynamic relocations.  */
	  if (s->size == 0)
	    s->size = plt_entry_size;

	  if (use_plt_got)
	    eh->plt_got.offset = got_s->size;
	  else
	    h->plt.offset = s->size;

	  /* If this symbol is not defined in a regular file, and we are
	     not generating a shared library, then set the symbol to this
	     location in the .plt.  This is required to make function
	     pointers compare as equal between the normal executable and
	     the shared library.  */
	  if (! bfd_link_pic (info)
	      && !h->def_regular)
	    {
	      if (use_plt_got)
		{
		  /* We need to make a call to the entry of the GOT PLT
		     instead of regular PLT entry.  */
		  h->root.u.def.section = got_s;
		  h->root.u.def.value = eh->plt_got.offset;
		}
	      else
		{
		  h->root.u.def.section = s;
		  h->root.u.def.value = h->plt.offset;
		}
	    }

	  /* Make room for this entry.  */
	  if (use_plt_got)
	    got_s->size += sizeof (elf_i386_got_plt_entry);
	  else
	    {
	      s->size += plt_entry_size;

	      /* We also need to make an entry in the .got.plt section,
		 which will be placed in the .got section by the linker
		 script.  */
	      htab->elf.sgotplt->size += 4;

	      /* There should be no PLT relocation against resolved
		 undefined weak symbol in executable.  */
	      if (!resolved_to_zero)
		{
		  /* We also need to make an entry in the .rel.plt
		     section.  */
		  htab->elf.srelplt->size += sizeof (Elf32_External_Rel);
		  htab->elf.srelplt->reloc_count++;
		}
	    }

	  if (get_elf_i386_backend_data (info->output_bfd)->is_vxworks
              && !bfd_link_pic (info))
	    {
	      /* VxWorks has a second set of relocations for each PLT entry
		 in executables.  They go in a separate relocation section,
		 which is processed by the kernel loader.  */

	      /* There are two relocations for the initial PLT entry: an
		 R_386_32 relocation for _GLOBAL_OFFSET_TABLE_ + 4 and an
		 R_386_32 relocation for _GLOBAL_OFFSET_TABLE_ + 8.  */

	      if (h->plt.offset == plt_entry_size)
		htab->srelplt2->size += (sizeof (Elf32_External_Rel) * 2);

	      /* There are two extra relocations for each subsequent PLT entry:
		 an R_386_32 relocation for the GOT entry, and an R_386_32
		 relocation for the PLT entry.  */

	      htab->srelplt2->size += (sizeof (Elf32_External_Rel) * 2);
	    }
	}
      else
	{
	  eh->plt_got.offset = (bfd_vma) -1;
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
    }
  else
    {
      eh->plt_got.offset = (bfd_vma) -1;
      h->plt.offset = (bfd_vma) -1;
      h->needs_plt = 0;
    }

  eh->tlsdesc_got = (bfd_vma) -1;

  /* If R_386_TLS_{IE_32,IE,GOTIE} symbol is now local to the binary,
     make it a R_386_TLS_LE_32 requiring no TLS entry.  */
  if (h->got.refcount > 0
      && bfd_link_executable (info)
      && h->dynindx == -1
      && (elf_i386_hash_entry(h)->tls_type & GOT_TLS_IE))
    h->got.offset = (bfd_vma) -1;
  else if (h->got.refcount > 0)
    {
      asection *s;
      bfd_boolean dyn;
      int tls_type = elf_i386_hash_entry(h)->tls_type;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local
	  && !resolved_to_zero)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      s = htab->elf.sgot;
      if (GOT_TLS_GDESC_P (tls_type))
	{
	  eh->tlsdesc_got = htab->elf.sgotplt->size
	    - elf_i386_compute_jump_table_size (htab);
	  htab->elf.sgotplt->size += 8;
	  h->got.offset = (bfd_vma) -2;
	}
      if (! GOT_TLS_GDESC_P (tls_type)
	  || GOT_TLS_GD_P (tls_type))
	{
	  h->got.offset = s->size;
	  s->size += 4;
	  /* R_386_TLS_GD needs 2 consecutive GOT slots.  */
	  if (GOT_TLS_GD_P (tls_type) || tls_type == GOT_TLS_IE_BOTH)
	    s->size += 4;
	}
      dyn = htab->elf.dynamic_sections_created;
      /* R_386_TLS_IE_32 needs one dynamic relocation,
	 R_386_TLS_IE resp. R_386_TLS_GOTIE needs one dynamic relocation,
	 (but if both R_386_TLS_IE_32 and R_386_TLS_IE is present, we
	 need two), R_386_TLS_GD needs one if local symbol and two if
	 global.  No dynamic relocation against resolved undefined weak
	 symbol in executable.  */
      if (tls_type == GOT_TLS_IE_BOTH)
	htab->elf.srelgot->size += 2 * sizeof (Elf32_External_Rel);
      else if ((GOT_TLS_GD_P (tls_type) && h->dynindx == -1)
	       || (tls_type & GOT_TLS_IE))
	htab->elf.srelgot->size += sizeof (Elf32_External_Rel);
      else if (GOT_TLS_GD_P (tls_type))
	htab->elf.srelgot->size += 2 * sizeof (Elf32_External_Rel);
      else if (! GOT_TLS_GDESC_P (tls_type)
	       && ((ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
		    && !resolved_to_zero)
		   || h->root.type != bfd_link_hash_undefweak)
	       && (bfd_link_pic (info)
		   || WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, 0, h)))
	htab->elf.srelgot->size += sizeof (Elf32_External_Rel);
      if (GOT_TLS_GDESC_P (tls_type))
	htab->elf.srelplt->size += sizeof (Elf32_External_Rel);
    }
  else
    h->got.offset = (bfd_vma) -1;

  if (eh->dyn_relocs == NULL)
    return TRUE;

  /* In the shared -Bsymbolic case, discard space allocated for
     dynamic pc-relative relocs against symbols which turn out to be
     defined in regular objects.  For the normal shared case, discard
     space for pc-relative relocs that have become local due to symbol
     visibility changes.  */

  if (bfd_link_pic (info))
    {
      /* The only reloc that uses pc_count is R_386_PC32, which will
	 appear on a call or on something like ".long foo - .".  We
	 want calls to protected symbols to resolve directly to the
	 function rather than going via the plt.  If people want
	 function pointer comparisons to work as expected then they
	 should avoid writing assembly like ".long foo - .".  */
      if (SYMBOL_CALLS_LOCAL (info, h))
	{
	  struct elf_dyn_relocs **pp;

	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      p->count -= p->pc_count;
	      p->pc_count = 0;
	      if (p->count == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }
	}

      if (get_elf_i386_backend_data (info->output_bfd)->is_vxworks)
	{
	  struct elf_dyn_relocs **pp;
	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      if (strcmp (p->sec->output_section->name, ".tls_vars") == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }
	}

      /* Also discard relocs on undefined weak syms with non-default
	 visibility or in PIE.  */
      if (eh->dyn_relocs != NULL
	  && h->root.type == bfd_link_hash_undefweak)
	{
	  /* Undefined weak symbol is never bound locally in shared
	     library.  */
	  if (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      || resolved_to_zero)
	    {
	      if (h->non_got_ref)
		{
		  /* Keep dynamic non-GOT/non-PLT relocation so that we
		     can branch to 0 without PLT.  */
		  struct elf_dyn_relocs **pp;

		  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
		    if (p->pc_count == 0)
		      *pp = p->next;
		    else
		      {
			/* Remove non-R_386_PC32 relocation.  */
			p->count = p->pc_count;
			pp = &p->next;
		      }

		  if (eh->dyn_relocs != NULL)
		    {
		      /* Make sure undefined weak symbols are output
			 as dynamic symbols in PIEs for dynamic non-GOT
			 non-PLT reloations.  */
		      if (! bfd_elf_link_record_dynamic_symbol (info, h))
			return FALSE;
		    }
		}
	      else
		eh->dyn_relocs = NULL;
	    }
	  else if (h->dynindx == -1
		   && !h->forced_local)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }
	}
    }
  else if (ELIMINATE_COPY_RELOCS)
    {
      /* For the non-shared case, discard space for relocs against
	 symbols which turn out to need copy relocs or are not
	 dynamic.  Keep dynamic relocations for run-time function
	 pointer initialization.  */

      if ((!h->non_got_ref
	   || eh->func_pointer_refcount > 0
	   || (h->root.type == bfd_link_hash_undefweak
	       && !resolved_to_zero))
	  && ((h->def_dynamic
	       && !h->def_regular)
	      || (htab->elf.dynamic_sections_created
		  && (h->root.type == bfd_link_hash_undefweak
		      || h->root.type == bfd_link_hash_undefined))))
	{
	  /* Make sure this symbol is output as a dynamic symbol.
	     Undefined weak syms won't yet be marked as dynamic.  */
	  if (h->dynindx == -1
	      && !h->forced_local
	      && !resolved_to_zero)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }

	  /* If that succeeded, we know we'll be keeping all the
	     relocs.  */
	  if (h->dynindx != -1)
	    goto keep;
	}

      eh->dyn_relocs = NULL;
      eh->func_pointer_refcount = 0;

    keep: ;
    }

  /* Finally, allocate space.  */
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      asection *sreloc;

      sreloc = elf_section_data (p->sec)->sreloc;

      BFD_ASSERT (sreloc != NULL);
      sreloc->size += p->count * sizeof (Elf32_External_Rel);
    }

  return TRUE;
}

/* Allocate space in .plt, .got and associated reloc sections for
   local dynamic relocs.  */

static bfd_boolean
elf_i386_allocate_local_dynrelocs (void **slot, void *inf)
{
  struct elf_link_hash_entry *h
    = (struct elf_link_hash_entry *) *slot;

  if (h->type != STT_GNU_IFUNC
      || !h->def_regular
      || !h->ref_regular
      || !h->forced_local
      || h->root.type != bfd_link_hash_defined)
    abort ();

  return elf_i386_allocate_dynrelocs (h, inf);
}

/* Find any dynamic relocs that apply to read-only sections.  */

static bfd_boolean
elf_i386_readonly_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct elf_i386_link_hash_entry *eh;
  struct elf_dyn_relocs *p;

  /* Skip local IFUNC symbols. */
  if (h->forced_local && h->type == STT_GNU_IFUNC)
    return TRUE;

  eh = (struct elf_i386_link_hash_entry *) h;
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      asection *s = p->sec->output_section;

      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	{
	  struct bfd_link_info *info = (struct bfd_link_info *) inf;

	  info->flags |= DF_TEXTREL;

	  if ((info->warn_shared_textrel && bfd_link_pic (info))
	      || info->error_textrel)
	    /* xgettext:c-format */
	    info->callbacks->einfo (_("%P: %B: warning: relocation against `%s' in readonly section `%A'\n"),
				    p->sec->owner, h->root.root.string,
				    p->sec);

	  /* Not an error, just cut short the traversal.  */
	  return FALSE;
	}
    }
  return TRUE;
}

/* Convert load via the GOT slot to load immediate.  */

static bfd_boolean
elf_i386_convert_load (bfd *abfd, asection *sec,
		       struct bfd_link_info *link_info)
{
  struct elf_i386_link_hash_table *htab;
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents;
  bfd_boolean changed;
  bfd_signed_vma *local_got_refcounts;

  /* Don't even try to convert non-ELF outputs.  */
  if (!is_elf_hash_table (link_info->hash))
    return FALSE;

  /* Nothing to do if there is no need or no output.  */
  if ((sec->flags & (SEC_CODE | SEC_RELOC)) != (SEC_CODE | SEC_RELOC)
      || sec->need_convert_load == 0
      || bfd_is_abs_section (sec->output_section))
    return TRUE;

  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;

  /* Load the relocations for this section.  */
  internal_relocs = (_bfd_elf_link_read_relocs
		     (abfd, sec, NULL, (Elf_Internal_Rela *) NULL,
		      link_info->keep_memory));
  if (internal_relocs == NULL)
    return FALSE;

  changed = FALSE;
  htab = elf_i386_hash_table (link_info);
  local_got_refcounts = elf_local_got_refcounts (abfd);

  /* Get the section contents.  */
  if (elf_section_data (sec)->this_hdr.contents != NULL)
    contents = elf_section_data (sec)->this_hdr.contents;
  else
    {
      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
	goto error_return;
    }

  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      unsigned int r_type = ELF32_R_TYPE (irel->r_info);
      unsigned int r_symndx;
      struct elf_link_hash_entry *h;
      bfd_boolean converted;

      /* Don't convert R_386_GOT32 since we can't tell if it is applied
	 to "mov $foo@GOT, %reg" which isn't a load via GOT.  */
      if (r_type != R_386_GOT32X)
	continue;

      r_symndx = ELF32_R_SYM (irel->r_info);
      if (r_symndx < symtab_hdr->sh_info)
	h = elf_i386_get_local_sym_hash (htab, sec->owner,
					 (const Elf_Internal_Rela *) irel,
					 FALSE);
      else
	{
	  h = elf_sym_hashes (abfd)[r_symndx - symtab_hdr->sh_info];
	   while (h->root.type == bfd_link_hash_indirect
		  || h->root.type == bfd_link_hash_warning)
	     h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      /* STT_GNU_IFUNC must keep GOT32 relocations.  */
      if (h != NULL && h->type == STT_GNU_IFUNC)
	continue;

      converted = FALSE;
      if (!elf_i386_convert_load_reloc (abfd, symtab_hdr, contents,
					irel, h, &converted, link_info))
	goto error_return;

      if (converted)
	{
	  changed = converted;
	  if (h)
	    {
	      if (h->got.refcount > 0)
		h->got.refcount -= 1;
	    }
	  else
	    {
	      if (local_got_refcounts != NULL
		  && local_got_refcounts[r_symndx] > 0)
		local_got_refcounts[r_symndx] -= 1;
	    }
	}
    }

  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (!changed && !link_info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  if (elf_section_data (sec)->relocs != internal_relocs)
    {
      if (!changed)
	free (internal_relocs);
      else
	elf_section_data (sec)->relocs = internal_relocs;
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

/* Set the sizes of the dynamic sections.  */

static bfd_boolean
elf_i386_size_dynamic_sections (bfd *output_bfd, struct bfd_link_info *info)
{
  struct elf_i386_link_hash_table *htab;
  bfd *dynobj;
  asection *s;
  bfd_boolean relocs;
  bfd *ibfd;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;
  dynobj = htab->elf.dynobj;
  if (dynobj == NULL)
    abort ();

  /* Set up .got offsets for local syms, and space for local dynamic
     relocs.  */
  for (ibfd = info->input_bfds; ibfd != NULL; ibfd = ibfd->link.next)
    {
      bfd_signed_vma *local_got;
      bfd_signed_vma *end_local_got;
      char *local_tls_type;
      bfd_vma *local_tlsdesc_gotent;
      bfd_size_type locsymcount;
      Elf_Internal_Shdr *symtab_hdr;
      asection *srel;

      if (! is_i386_elf (ibfd))
	continue;

      for (s = ibfd->sections; s != NULL; s = s->next)
	{
	  struct elf_dyn_relocs *p;

	  if (!elf_i386_convert_load (ibfd, s, info))
	    return FALSE;

	  for (p = ((struct elf_dyn_relocs *)
		     elf_section_data (s)->local_dynrel);
	       p != NULL;
	       p = p->next)
	    {
	      if (!bfd_is_abs_section (p->sec)
		  && bfd_is_abs_section (p->sec->output_section))
		{
		  /* Input section has been discarded, either because
		     it is a copy of a linkonce section or due to
		     linker script /DISCARD/, so we'll be discarding
		     the relocs too.  */
		}
	      else if (get_elf_i386_backend_data (output_bfd)->is_vxworks
		       && strcmp (p->sec->output_section->name,
				  ".tls_vars") == 0)
		{
		  /* Relocations in vxworks .tls_vars sections are
		     handled specially by the loader.  */
		}
	      else if (p->count != 0)
		{
		  srel = elf_section_data (p->sec)->sreloc;
		  srel->size += p->count * sizeof (Elf32_External_Rel);
		  if ((p->sec->output_section->flags & SEC_READONLY) != 0
		      && (info->flags & DF_TEXTREL) == 0)
		    {
		      info->flags |= DF_TEXTREL;
		      if ((info->warn_shared_textrel && bfd_link_pic (info))
			  || info->error_textrel)
			/* xgettext:c-format */
			info->callbacks->einfo (_("%P: %B: warning: relocation in readonly section `%A'\n"),
						p->sec->owner, p->sec);
		    }
		}
	    }
	}

      local_got = elf_local_got_refcounts (ibfd);
      if (!local_got)
	continue;

      symtab_hdr = &elf_symtab_hdr (ibfd);
      locsymcount = symtab_hdr->sh_info;
      end_local_got = local_got + locsymcount;
      local_tls_type = elf_i386_local_got_tls_type (ibfd);
      local_tlsdesc_gotent = elf_i386_local_tlsdesc_gotent (ibfd);
      s = htab->elf.sgot;
      srel = htab->elf.srelgot;
      for (; local_got < end_local_got;
	   ++local_got, ++local_tls_type, ++local_tlsdesc_gotent)
	{
	  *local_tlsdesc_gotent = (bfd_vma) -1;
	  if (*local_got > 0)
	    {
	      if (GOT_TLS_GDESC_P (*local_tls_type))
		{
		  *local_tlsdesc_gotent = htab->elf.sgotplt->size
		    - elf_i386_compute_jump_table_size (htab);
		  htab->elf.sgotplt->size += 8;
		  *local_got = (bfd_vma) -2;
		}
	      if (! GOT_TLS_GDESC_P (*local_tls_type)
		  || GOT_TLS_GD_P (*local_tls_type))
		{
		  *local_got = s->size;
		  s->size += 4;
		  if (GOT_TLS_GD_P (*local_tls_type)
		      || *local_tls_type == GOT_TLS_IE_BOTH)
		    s->size += 4;
		}
	      if (bfd_link_pic (info)
		  || GOT_TLS_GD_ANY_P (*local_tls_type)
		  || (*local_tls_type & GOT_TLS_IE))
		{
		  if (*local_tls_type == GOT_TLS_IE_BOTH)
		    srel->size += 2 * sizeof (Elf32_External_Rel);
		  else if (GOT_TLS_GD_P (*local_tls_type)
			   || ! GOT_TLS_GDESC_P (*local_tls_type))
		    srel->size += sizeof (Elf32_External_Rel);
		  if (GOT_TLS_GDESC_P (*local_tls_type))
		    htab->elf.srelplt->size += sizeof (Elf32_External_Rel);
		}
	    }
	  else
	    *local_got = (bfd_vma) -1;
	}
    }

  if (htab->tls_ldm_got.refcount > 0)
    {
      /* Allocate 2 got entries and 1 dynamic reloc for R_386_TLS_LDM
	 relocs.  */
      htab->tls_ldm_got.offset = htab->elf.sgot->size;
      htab->elf.sgot->size += 8;
      htab->elf.srelgot->size += sizeof (Elf32_External_Rel);
    }
  else
    htab->tls_ldm_got.offset = -1;

  /* Allocate global sym .plt and .got entries, and space for global
     sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->elf, elf_i386_allocate_dynrelocs, info);

  /* Allocate .plt and .got entries, and space for local symbols.  */
  htab_traverse (htab->loc_hash_table,
		 elf_i386_allocate_local_dynrelocs,
		 info);

  /* For every jump slot reserved in the sgotplt, reloc_count is
     incremented.  However, when we reserve space for TLS descriptors,
     it's not incremented, so in order to compute the space reserved
     for them, it suffices to multiply the reloc count by the jump
     slot size.

     PR ld/13302: We start next_irelative_index at the end of .rela.plt
     so that R_386_IRELATIVE entries come last.  */
  if (htab->elf.srelplt)
    {
      htab->next_tls_desc_index = htab->elf.srelplt->reloc_count;
      htab->sgotplt_jump_table_size = htab->next_tls_desc_index * 4;
      htab->next_irelative_index = htab->elf.srelplt->reloc_count - 1;
    }
  else if (htab->elf.irelplt)
    htab->next_irelative_index = htab->elf.irelplt->reloc_count - 1;


  if (htab->elf.sgotplt)
    {
      /* Don't allocate .got.plt section if there are no GOT nor PLT
         entries and there is no reference to _GLOBAL_OFFSET_TABLE_.  */
      if ((htab->elf.hgot == NULL
	   || !htab->elf.hgot->ref_regular_nonweak)
	  && (htab->elf.sgotplt->size
	      == get_elf_backend_data (output_bfd)->got_header_size)
	  && (htab->elf.splt == NULL
	      || htab->elf.splt->size == 0)
	  && (htab->elf.sgot == NULL
	      || htab->elf.sgot->size == 0)
	  && (htab->elf.iplt == NULL
	      || htab->elf.iplt->size == 0)
	  && (htab->elf.igotplt == NULL
	      || htab->elf.igotplt->size == 0))
	htab->elf.sgotplt->size = 0;
    }

  if (_bfd_elf_eh_frame_present (info))
    {
      if (htab->plt_eh_frame != NULL
	  && htab->elf.splt != NULL
	  && htab->elf.splt->size != 0
	  && !bfd_is_abs_section (htab->elf.splt->output_section))
	htab->plt_eh_frame->size
	  = get_elf_i386_backend_data (output_bfd)->plt->eh_frame_plt_size;

      if (htab->plt_got_eh_frame != NULL
	  && htab->plt_got != NULL
	  && htab->plt_got->size != 0
	  && !bfd_is_abs_section (htab->plt_got->output_section))
	htab->plt_got_eh_frame->size
	  = get_elf_i386_backend_data (output_bfd)->plt->eh_frame_plt_got_size;
    }

  /* We now have determined the sizes of the various dynamic sections.
     Allocate memory for them.  */
  relocs = FALSE;
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      bfd_boolean strip_section = TRUE;

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      if (s == htab->elf.splt
	  || s == htab->elf.sgot)
	{
	  /* Strip this section if we don't need it; see the
	     comment below.  */
	  /* We'd like to strip these sections if they aren't needed, but if
	     we've exported dynamic symbols from them we must leave them.
	     It's too late to tell BFD to get rid of the symbols.  */

	  if (htab->elf.hplt != NULL)
	    strip_section = FALSE;
	}
      else if (s == htab->elf.sgotplt
	       || s == htab->elf.iplt
	       || s == htab->elf.igotplt
	       || s == htab->plt_got
	       || s == htab->plt_eh_frame
	       || s == htab->plt_got_eh_frame
	       || s == htab->elf.sdynbss
	       || s == htab->elf.sdynrelro)
	{
	  /* Strip these too.  */
	}
      else if (CONST_STRNEQ (bfd_get_section_name (dynobj, s), ".rel"))
	{
	  if (s->size != 0
	      && s != htab->elf.srelplt
	      && s != htab->srelplt2)
	    relocs = TRUE;

	  /* We use the reloc_count field as a counter if we need
	     to copy relocs into the output file.  */
	  s->reloc_count = 0;
	}
      else
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

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
	  if (strip_section)
	    s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* Allocate memory for the section contents.  We use bfd_zalloc
	 here in case unused entries are not reclaimed before the
	 section's contents are written out.  This should not happen,
	 but this way if it does, we get a R_386_NONE reloc instead
	 of garbage.  */
      s->contents = (unsigned char *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return FALSE;
    }

  if (htab->plt_eh_frame != NULL
      && htab->plt_eh_frame->contents != NULL)
    {
      memcpy (htab->plt_eh_frame->contents,
	      get_elf_i386_backend_data (output_bfd)->plt->eh_frame_plt,
	      htab->plt_eh_frame->size);
      bfd_put_32 (dynobj, htab->elf.splt->size,
		  htab->plt_eh_frame->contents + PLT_FDE_LEN_OFFSET);
    }

  if (htab->plt_got_eh_frame != NULL
      && htab->plt_got_eh_frame->contents != NULL)
    {
      memcpy (htab->plt_got_eh_frame->contents,
	      get_elf_i386_backend_data (output_bfd)->plt->eh_frame_plt_got,
	      htab->plt_got_eh_frame->size);
      bfd_put_32 (dynobj, htab->plt_got->size,
		  (htab->plt_got_eh_frame->contents
		   + PLT_FDE_LEN_OFFSET));
    }

  if (htab->elf.dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in elf_i386_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  The DT_DEBUG entry is filled in by the
	 dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (bfd_link_executable (info))
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}

      if (htab->elf.splt->size != 0)
	{
	  /* DT_PLTGOT is used by prelink even if there is no PLT
	     relocation.  */
	  if (!add_dynamic_entry (DT_PLTGOT, 0))
	    return FALSE;

	  if (htab->elf.srelplt->size != 0)
	    {
	      if (!add_dynamic_entry (DT_PLTRELSZ, 0)
		  || !add_dynamic_entry (DT_PLTREL, DT_REL)
		  || !add_dynamic_entry (DT_JMPREL, 0))
		return FALSE;
	    }
	}

      if (relocs)
	{
	  if (!add_dynamic_entry (DT_REL, 0)
	      || !add_dynamic_entry (DT_RELSZ, 0)
	      || !add_dynamic_entry (DT_RELENT, sizeof (Elf32_External_Rel)))
	    return FALSE;

	  /* If any dynamic relocs apply to a read-only section,
	     then we need a DT_TEXTREL entry.  */
	  if ((info->flags & DF_TEXTREL) == 0)
	    elf_link_hash_traverse (&htab->elf,
				    elf_i386_readonly_dynrelocs, info);

	  if ((info->flags & DF_TEXTREL) != 0)
	    {
	      if (htab->readonly_dynrelocs_against_ifunc)
		{
		  info->callbacks->einfo
		    (_("%P%X: read-only segment has dynamic IFUNC relocations; recompile with -fPIC\n"));
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}

	      if (!add_dynamic_entry (DT_TEXTREL, 0))
		return FALSE;
	    }
	}
      if (get_elf_i386_backend_data (output_bfd)->is_vxworks
	  && !elf_vxworks_add_dynamic_entries (output_bfd, info))
	return FALSE;
    }
#undef add_dynamic_entry

  return TRUE;
}

static bfd_boolean
elf_i386_always_size_sections (bfd *output_bfd,
			       struct bfd_link_info *info)
{
  asection *tls_sec = elf_hash_table (info)->tls_sec;

  if (tls_sec)
    {
      struct elf_link_hash_entry *tlsbase;

      tlsbase = elf_link_hash_lookup (elf_hash_table (info),
				      "_TLS_MODULE_BASE_",
				      FALSE, FALSE, FALSE);

      if (tlsbase && tlsbase->type == STT_TLS)
	{
	  struct elf_i386_link_hash_table *htab;
	  struct bfd_link_hash_entry *bh = NULL;
	  const struct elf_backend_data *bed
	    = get_elf_backend_data (output_bfd);

	  htab = elf_i386_hash_table (info);
	  if (htab == NULL)
	    return FALSE;

	  if (!(_bfd_generic_link_add_one_symbol
		(info, output_bfd, "_TLS_MODULE_BASE_", BSF_LOCAL,
		 tls_sec, 0, NULL, FALSE,
		 bed->collect, &bh)))
	    return FALSE;

	  htab->tls_module_base = bh;

	  tlsbase = (struct elf_link_hash_entry *)bh;
	  tlsbase->def_regular = 1;
	  tlsbase->other = STV_HIDDEN;
	  tlsbase->root.linker_def = 1;
	  (*bed->elf_backend_hide_symbol) (info, tlsbase, TRUE);
	}
    }

  return TRUE;
}

/* Set the correct type for an x86 ELF section.  We do this by the
   section name, which is a hack, but ought to work.  */

static bfd_boolean
elf_i386_fake_sections (bfd *abfd ATTRIBUTE_UNUSED,
			Elf_Internal_Shdr *hdr,
			asection *sec)
{
  const char *name;

  name = bfd_get_section_name (abfd, sec);

  /* This is an ugly, but unfortunately necessary hack that is
     needed when producing EFI binaries on x86. It tells
     elf.c:elf_fake_sections() not to consider ".reloc" as a section
     containing ELF relocation info.  We need this hack in order to
     be able to generate ELF binaries that can be translated into
     EFI applications (which are essentially COFF objects).  Those
     files contain a COFF ".reloc" section inside an ELFNN object,
     which would normally cause BFD to segfault because it would
     attempt to interpret this section as containing relocation
     entries for section "oc".  With this hack enabled, ".reloc"
     will be treated as a normal data section, which will avoid the
     segfault.  However, you won't be able to create an ELFNN binary
     with a section named "oc" that needs relocations, but that's
     the kind of ugly side-effects you get when detecting section
     types based on their names...  In practice, this limitation is
     unlikely to bite.  */
  if (strcmp (name, ".reloc") == 0)
    hdr->sh_type = SHT_PROGBITS;

  return TRUE;
}

/* _TLS_MODULE_BASE_ needs to be treated especially when linking
   executables.  Rather than setting it to the beginning of the TLS
   section, we have to set it to the end.    This function may be called
   multiple times, it is idempotent.  */

static void
elf_i386_set_tls_module_base (struct bfd_link_info *info)
{
  struct elf_i386_link_hash_table *htab;
  struct bfd_link_hash_entry *base;

  if (!bfd_link_executable (info))
    return;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return;

  base = htab->tls_module_base;
  if (base == NULL)
    return;

  base->u.def.value = htab->elf.tls_size;
}

/* Return the base VMA address which should be subtracted from real addresses
   when resolving @dtpoff relocation.
   This is PT_TLS segment p_vaddr.  */

static bfd_vma
elf_i386_dtpoff_base (struct bfd_link_info *info)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return elf_hash_table (info)->tls_sec->vma;
}

/* Return the relocation value for @tpoff relocation
   if STT_TLS virtual address is ADDRESS.  */

static bfd_vma
elf_i386_tpoff (struct bfd_link_info *info, bfd_vma address)
{
  struct elf_link_hash_table *htab = elf_hash_table (info);
  const struct elf_backend_data *bed = get_elf_backend_data (info->output_bfd);
  bfd_vma static_tls_size;

  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (htab->tls_sec == NULL)
    return 0;

  /* Consider special static TLS alignment requirements.  */
  static_tls_size = BFD_ALIGN (htab->tls_size, bed->static_tls_alignment);
  return static_tls_size + htab->tls_sec->vma - address;
}

/* Relocate an i386 ELF section.  */

static bfd_boolean
elf_i386_relocate_section (bfd *output_bfd,
			   struct bfd_link_info *info,
			   bfd *input_bfd,
			   asection *input_section,
			   bfd_byte *contents,
			   Elf_Internal_Rela *relocs,
			   Elf_Internal_Sym *local_syms,
			   asection **local_sections)
{
  struct elf_i386_link_hash_table *htab;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  bfd_vma *local_got_offsets;
  bfd_vma *local_tlsdesc_gotents;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *wrel;
  Elf_Internal_Rela *relend;
  bfd_boolean is_vxworks_tls;
  unsigned plt_entry_size;

  BFD_ASSERT (is_i386_elf (input_bfd));

  /* Skip if check_relocs failed.  */
  if (input_section->check_relocs_failed)
    return FALSE;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;
  symtab_hdr = &elf_symtab_hdr (input_bfd);
  sym_hashes = elf_sym_hashes (input_bfd);
  local_got_offsets = elf_local_got_offsets (input_bfd);
  local_tlsdesc_gotents = elf_i386_local_tlsdesc_gotent (input_bfd);
  /* We have to handle relocations in vxworks .tls_vars sections
     specially, because the dynamic loader is 'weird'.  */
  is_vxworks_tls = (get_elf_i386_backend_data (output_bfd)->is_vxworks
                    && bfd_link_pic (info)
		    && !strcmp (input_section->output_section->name,
				".tls_vars"));

  elf_i386_set_tls_module_base (info);

  plt_entry_size = GET_PLT_ENTRY_SIZE (output_bfd);

  rel = wrel = relocs;
  relend = relocs + input_section->reloc_count;
  for (; rel < relend; wrel++, rel++)
    {
      unsigned int r_type;
      reloc_howto_type *howto;
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;
      struct elf_i386_link_hash_entry *eh;
      Elf_Internal_Sym *sym;
      asection *sec;
      bfd_vma off, offplt, plt_offset;
      bfd_vma relocation;
      bfd_boolean unresolved_reloc;
      bfd_reloc_status_type r;
      unsigned int indx;
      int tls_type;
      bfd_vma st_size;
      asection *resolved_plt;
      bfd_boolean resolved_to_zero;

      r_type = ELF32_R_TYPE (rel->r_info);
      if (r_type == R_386_GNU_VTINHERIT
	  || r_type == R_386_GNU_VTENTRY)
	{
	  if (wrel != rel)
	    *wrel = *rel;
	  continue;
	}

      if ((indx = r_type) >= R_386_standard
	  && ((indx = r_type - R_386_ext_offset) - R_386_standard
	      >= R_386_ext - R_386_standard)
	  && ((indx = r_type - R_386_tls_offset) - R_386_ext
	      >= R_386_ext2 - R_386_ext))
	{
	  _bfd_error_handler
	    /* xgettext:c-format */
	    (_("%B: unrecognized relocation (0x%x) in section `%A'"),
	     input_bfd, r_type, input_section);
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
      howto = elf_howto_table + indx;

      r_symndx = ELF32_R_SYM (rel->r_info);
      h = NULL;
      sym = NULL;
      sec = NULL;
      unresolved_reloc = FALSE;
      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = (sec->output_section->vma
			+ sec->output_offset
			+ sym->st_value);
	  st_size = sym->st_size;

	  if (ELF_ST_TYPE (sym->st_info) == STT_SECTION
	      && ((sec->flags & SEC_MERGE) != 0
		  || (bfd_link_relocatable (info)
		      && sec->output_offset != 0)))
	    {
	      bfd_vma addend;
	      bfd_byte *where = contents + rel->r_offset;

	      switch (howto->size)
		{
		case 0:
		  addend = bfd_get_8 (input_bfd, where);
		  if (howto->pc_relative)
		    {
		      addend = (addend ^ 0x80) - 0x80;
		      addend += 1;
		    }
		  break;
		case 1:
		  addend = bfd_get_16 (input_bfd, where);
		  if (howto->pc_relative)
		    {
		      addend = (addend ^ 0x8000) - 0x8000;
		      addend += 2;
		    }
		  break;
		case 2:
		  addend = bfd_get_32 (input_bfd, where);
		  if (howto->pc_relative)
		    {
		      addend = (addend ^ 0x80000000) - 0x80000000;
		      addend += 4;
		    }
		  break;
		default:
		  abort ();
		}

	      if (bfd_link_relocatable (info))
		addend += sec->output_offset;
	      else
		{
		  asection *msec = sec;
		  addend = _bfd_elf_rel_local_sym (output_bfd, sym, &msec,
						   addend);
		  addend -= relocation;
		  addend += msec->output_section->vma + msec->output_offset;
		}

	      switch (howto->size)
		{
		case 0:
		  /* FIXME: overflow checks.  */
		  if (howto->pc_relative)
		    addend -= 1;
		  bfd_put_8 (input_bfd, addend, where);
		  break;
		case 1:
		  if (howto->pc_relative)
		    addend -= 2;
		  bfd_put_16 (input_bfd, addend, where);
		  break;
		case 2:
		  if (howto->pc_relative)
		    addend -= 4;
		  bfd_put_32 (input_bfd, addend, where);
		  break;
		}
	    }
	  else if (!bfd_link_relocatable (info)
		   && ELF32_ST_TYPE (sym->st_info) == STT_GNU_IFUNC)
	    {
	      /* Relocate against local STT_GNU_IFUNC symbol.  */
	      h = elf_i386_get_local_sym_hash (htab, input_bfd, rel,
					       FALSE);
	      if (h == NULL)
		abort ();

	      /* Set STT_GNU_IFUNC symbol value.  */
	      h->root.u.def.value = sym->st_value;
	      h->root.u.def.section = sec;
	    }
	}
      else
	{
	  bfd_boolean warned ATTRIBUTE_UNUSED;
	  bfd_boolean ignored ATTRIBUTE_UNUSED;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);
	  st_size = h->size;
	}

      if (sec != NULL && discarded_section (sec))
	{
	  _bfd_clear_contents (howto, input_bfd, input_section,
			       contents + rel->r_offset);
	  wrel->r_offset = rel->r_offset;
	  wrel->r_info = 0;
	  wrel->r_addend = 0;

	  /* For ld -r, remove relocations in debug sections against
	     sections defined in discarded sections.  Not done for
	     eh_frame editing code expects to be present.  */
	   if (bfd_link_relocatable (info)
	       && (input_section->flags & SEC_DEBUGGING))
	     wrel--;

	   continue;
	}

      if (bfd_link_relocatable (info))
	{
	  if (wrel != rel)
	    *wrel = *rel;
	  continue;
	}

      /* Since STT_GNU_IFUNC symbol must go through PLT, we handle
	 it here if it is defined in a non-shared object.  */
      if (h != NULL
	  && h->type == STT_GNU_IFUNC
	  && h->def_regular)
	{
	  asection *plt, *gotplt, *base_got;
	  bfd_vma plt_index;
	  const char *name;

	  if ((input_section->flags & SEC_ALLOC) == 0)
	    {
	      /* Dynamic relocs are not propagated for SEC_DEBUGGING
		 sections because such sections are not SEC_ALLOC and
		 thus ld.so will not process them.  */
	      if ((input_section->flags & SEC_DEBUGGING) != 0)
		continue;
	      abort ();
	    }

	  /* STT_GNU_IFUNC symbol must go through PLT.  */
	  if (htab->elf.splt != NULL)
	    {
	      plt = htab->elf.splt;
	      gotplt = htab->elf.sgotplt;
	    }
	  else
	    {
	      plt = htab->elf.iplt;
	      gotplt = htab->elf.igotplt;
	    }

	  switch (r_type)
	    {
	    default:
	      break;

	    case R_386_GOT32:
	    case R_386_GOT32X:
	      base_got = htab->elf.sgot;
	      off = h->got.offset;

	      if (base_got == NULL)
		abort ();

	      if (off == (bfd_vma) -1)
		{
		  /* We can't use h->got.offset here to save state, or
		     even just remember the offset, as finish_dynamic_symbol
		     would use that as offset into .got.  */

		  if (h->plt.offset == (bfd_vma) -1)
		    abort ();

		  if (htab->elf.splt != NULL)
		    {
		      plt_index = h->plt.offset / plt_entry_size - 1;
		      off = (plt_index + 3) * 4;
		      base_got = htab->elf.sgotplt;
		    }
		  else
		    {
		      plt_index = h->plt.offset / plt_entry_size;
		      off = plt_index * 4;
		      base_got = htab->elf.igotplt;
		    }

		  if (h->dynindx == -1
		      || h->forced_local
		      || info->symbolic)
		    {
		      /* This references the local defitionion.  We must
			 initialize this entry in the global offset table.
			 Since the offset must always be a multiple of 8,
			 we use the least significant bit to record
			 whether we have initialized it already.

			 When doing a dynamic link, we create a .rela.got
			 relocation entry to initialize the value.  This
			 is done in the finish_dynamic_symbol routine.	 */
		      if ((off & 1) != 0)
			off &= ~1;
		      else
			{
			  bfd_put_32 (output_bfd, relocation,
				      base_got->contents + off);
			  h->got.offset |= 1;
			}
		    }

		  relocation = off;
		}
	      else
		relocation = (base_got->output_section->vma
			      + base_got->output_offset + off
			      - gotplt->output_section->vma
			      - gotplt->output_offset);

	      if (rel->r_offset > 1
		  && (*(contents + rel->r_offset - 1) & 0xc7) == 0x5
		  && *(contents + rel->r_offset - 2) != 0x8d)
		{
		  if (bfd_link_pic (info))
		    goto disallow_got32;

		  /* Add the GOT base if there is no base register.  */
		  relocation += (gotplt->output_section->vma
				 + gotplt->output_offset);
		}
	      else if (htab->elf.splt == NULL)
		{
		  /* Adjust for static executables.  */
		  relocation += gotplt->output_offset;
		}

	      goto do_relocation;
	    }

	  if (h->plt.offset == (bfd_vma) -1)
	    {
	      /* Handle static pointers of STT_GNU_IFUNC symbols.  */
	      if (r_type == R_386_32
		  && (input_section->flags & SEC_CODE) == 0)
		goto do_ifunc_pointer;
	      goto bad_ifunc_reloc;
	    }

	  relocation = (plt->output_section->vma
			+ plt->output_offset + h->plt.offset);

	  switch (r_type)
	    {
	    default:
bad_ifunc_reloc:
	      if (h->root.root.string)
		name = h->root.root.string;
	      else
		name = bfd_elf_sym_name (input_bfd, symtab_hdr, sym,
					 NULL);
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B: relocation %s against STT_GNU_IFUNC "
		   "symbol `%s' isn't supported"), input_bfd,
		 howto->name, name);
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;

	    case R_386_32:
	      /* Generate dynamic relcoation only when there is a
		 non-GOT reference in a shared object.  */
	      if ((bfd_link_pic (info) && h->non_got_ref)
		  || h->plt.offset == (bfd_vma) -1)
		{
		  Elf_Internal_Rela outrel;
		  asection *sreloc;
		  bfd_vma offset;

do_ifunc_pointer:
		  /* Need a dynamic relocation to get the real function
		     adddress.  */
		  offset = _bfd_elf_section_offset (output_bfd,
						    info,
						    input_section,
						    rel->r_offset);
		  if (offset == (bfd_vma) -1
		      || offset == (bfd_vma) -2)
		    abort ();

		  outrel.r_offset = (input_section->output_section->vma
				     + input_section->output_offset
				     + offset);

		  if (h->dynindx == -1
		      || h->forced_local
		      || bfd_link_executable (info))
		    {
		      /* This symbol is resolved locally.  */
		      outrel.r_info = ELF32_R_INFO (0, R_386_IRELATIVE);
		      bfd_put_32 (output_bfd,
				  (h->root.u.def.value
				   + h->root.u.def.section->output_section->vma
				   + h->root.u.def.section->output_offset),
				  contents + offset);
		    }
		  else
		    outrel.r_info = ELF32_R_INFO (h->dynindx, r_type);

		  /* Dynamic relocations are stored in
		     1. .rel.ifunc section in PIC object.
		     2. .rel.got section in dynamic executable.
		     3. .rel.iplt section in static executable.  */
		  if (bfd_link_pic (info))
		    sreloc = htab->elf.irelifunc;
		  else if (htab->elf.splt != NULL)
		    sreloc = htab->elf.srelgot;
		  else
		    sreloc = htab->elf.irelplt;
		  elf_append_rel (output_bfd, sreloc, &outrel);

		  /* If this reloc is against an external symbol, we
		     do not want to fiddle with the addend.  Otherwise,
		     we need to include the symbol value so that it
		     becomes an addend for the dynamic reloc.  For an
		     internal symbol, we have updated addend.  */
		  continue;
		}
	      /* FALLTHROUGH */
	    case R_386_PC32:
	    case R_386_PLT32:
	      goto do_relocation;

	    case R_386_GOTOFF:
	      relocation -= (gotplt->output_section->vma
			     + gotplt->output_offset);
	      goto do_relocation;
	    }
	}

      eh = (struct elf_i386_link_hash_entry *) h;
      resolved_to_zero = (eh != NULL
			  && UNDEFINED_WEAK_RESOLVED_TO_ZERO (info,
							      eh->has_got_reloc,
							      eh));

      switch (r_type)
	{
	case R_386_GOT32X:
	  /* Avoid optimizing _DYNAMIC since ld.so may use its
	     link-time address.  */
	  if (h == htab->elf.hdynamic)
	    goto r_386_got32;

	  if (bfd_link_pic (info))
	    {
	      /* It is OK to convert mov to lea and convert indirect
		 branch to direct branch.  It is OK to convert adc,
		 add, and, cmp, or, sbb, sub, test, xor only when PIC
		 is false.   */
	      unsigned int opcode, addend;
	      addend = bfd_get_32 (input_bfd, contents + rel->r_offset);
	      if (addend != 0)
		goto r_386_got32;
	      opcode = bfd_get_8 (input_bfd, contents + rel->r_offset - 2);
	      if (opcode != 0x8b && opcode != 0xff)
		goto r_386_got32;
	    }

	  /* Resolve "mov GOT[(%reg)], %reg",
	     "call/jmp *GOT[(%reg)]", "test %reg, foo@GOT[(%reg)]"
	     and "binop foo@GOT[(%reg)], %reg".  */
	  if (h == NULL
	      || (h->plt.offset == (bfd_vma) -1
		  && h->got.offset == (bfd_vma) -1)
	      || htab->elf.sgotplt == NULL)
	    abort ();

	  offplt = (htab->elf.sgotplt->output_section->vma
		    + htab->elf.sgotplt->output_offset);

	  /* It is relative to .got.plt section.  */
	  if (h->got.offset != (bfd_vma) -1)
	    /* Use GOT entry.  Mask off the least significant bit in
	       GOT offset which may be set by R_386_GOT32 processing
	       below.  */
	    relocation = (htab->elf.sgot->output_section->vma
			  + htab->elf.sgot->output_offset
			  + (h->got.offset & ~1) - offplt);
	  else
	    /* Use GOTPLT entry.  */
	    relocation = (h->plt.offset / plt_entry_size - 1 + 3) * 4;

	  if (!bfd_link_pic (info))
	    {
	      /* If not PIC, add the .got.plt section address for
		 baseless addressing.  */
	      unsigned int modrm;
	      modrm = bfd_get_8 (input_bfd, contents + rel->r_offset - 1);
	      if ((modrm & 0xc7) == 0x5)
		relocation += offplt;
	    }

	  unresolved_reloc = FALSE;
	  break;

	case R_386_GOT32:
r_386_got32:
	  /* Relocation is to the entry for this symbol in the global
	     offset table.  */
	  if (htab->elf.sgot == NULL)
	    abort ();

	  if (h != NULL)
	    {
	      bfd_boolean dyn;

	      off = h->got.offset;
	      dyn = htab->elf.dynamic_sections_created;
	      if (! WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn,
						     bfd_link_pic (info),
						     h)
		  || (bfd_link_pic (info)
		      && SYMBOL_REFERENCES_LOCAL (info, h))
		  || (ELF_ST_VISIBILITY (h->other)
		      && h->root.type == bfd_link_hash_undefweak))
		{
		  /* This is actually a static link, or it is a
		     -Bsymbolic link and the symbol is defined
		     locally, or the symbol was forced to be local
		     because of a version file.  We must initialize
		     this entry in the global offset table.  Since the
		     offset must always be a multiple of 4, we use the
		     least significant bit to record whether we have
		     initialized it already.

		     When doing a dynamic link, we create a .rel.got
		     relocation entry to initialize the value.  This
		     is done in the finish_dynamic_symbol routine.  */
		  if ((off & 1) != 0)
		    off &= ~1;
		  else
		    {
		      bfd_put_32 (output_bfd, relocation,
				  htab->elf.sgot->contents + off);
		      h->got.offset |= 1;
		    }
		}
	      else
		unresolved_reloc = FALSE;
	    }
	  else
	    {
	      if (local_got_offsets == NULL)
		abort ();

	      off = local_got_offsets[r_symndx];

	      /* The offset must always be a multiple of 4.  We use
		 the least significant bit to record whether we have
		 already generated the necessary reloc.  */
	      if ((off & 1) != 0)
		off &= ~1;
	      else
		{
		  bfd_put_32 (output_bfd, relocation,
			      htab->elf.sgot->contents + off);

		  if (bfd_link_pic (info))
		    {
		      asection *s;
		      Elf_Internal_Rela outrel;

		      s = htab->elf.srelgot;
		      if (s == NULL)
			abort ();

		      outrel.r_offset = (htab->elf.sgot->output_section->vma
					 + htab->elf.sgot->output_offset
					 + off);
		      outrel.r_info = ELF32_R_INFO (0, R_386_RELATIVE);
		      elf_append_rel (output_bfd, s, &outrel);
		    }

		  local_got_offsets[r_symndx] |= 1;
		}
	    }

	  if (off >= (bfd_vma) -2)
	    abort ();

	  relocation = (htab->elf.sgot->output_section->vma
			+ htab->elf.sgot->output_offset + off);
	  if (rel->r_offset > 1
	      && (*(contents + rel->r_offset - 1) & 0xc7) == 0x5
	      && *(contents + rel->r_offset - 2) != 0x8d)
	    {
	      if (bfd_link_pic (info))
		{
		  /* For PIC, disallow R_386_GOT32 without a base
		     register, except for "lea foo@GOT, %reg", since
		     we don't know what the GOT base is.  */
		  const char *name;

disallow_got32:
		  if (h == NULL || h->root.root.string == NULL)
		    name = bfd_elf_sym_name (input_bfd, symtab_hdr, sym,
					     NULL);
		  else
		    name = h->root.root.string;

		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B: direct GOT relocation %s against `%s'"
		       " without base register can not be used"
		       " when making a shared object"),
		     input_bfd, howto->name, name);
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}
	    }
	  else
	    {
	      /* Subtract the .got.plt section address only with a base
		 register.  */
	      relocation -= (htab->elf.sgotplt->output_section->vma
			     + htab->elf.sgotplt->output_offset);
	    }

	  break;

	case R_386_GOTOFF:
	  /* Relocation is relative to the start of the global offset
	     table.  */

	  /* Check to make sure it isn't a protected function or data
	     symbol for shared library since it may not be local when
	     used as function address or with copy relocation.  We also
	     need to make sure that a symbol is referenced locally.  */
	  if (!bfd_link_executable (info) && h)
	    {
	      if (!h->def_regular)
		{
		  const char *v;

		  switch (ELF_ST_VISIBILITY (h->other))
		    {
		    case STV_HIDDEN:
		      v = _("hidden symbol");
		      break;
		    case STV_INTERNAL:
		      v = _("internal symbol");
		      break;
		    case STV_PROTECTED:
		      v = _("protected symbol");
		      break;
		    default:
		      v = _("symbol");
		      break;
		    }

		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B: relocation R_386_GOTOFF against undefined %s"
		       " `%s' can not be used when making a shared object"),
		     input_bfd, v, h->root.root.string);
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}
	      else if (!SYMBOL_REFERENCES_LOCAL (info, h)
		       && (h->type == STT_FUNC
			   || h->type == STT_OBJECT)
		       && ELF_ST_VISIBILITY (h->other) == STV_PROTECTED)
		{
		  _bfd_error_handler
		    /* xgettext:c-format */
		    (_("%B: relocation R_386_GOTOFF against protected %s"
		       " `%s' can not be used when making a shared object"),
		     input_bfd,
		     h->type == STT_FUNC ? "function" : "data",
		     h->root.root.string);
		  bfd_set_error (bfd_error_bad_value);
		  return FALSE;
		}
	    }

	  /* Note that sgot is not involved in this
	     calculation.  We always want the start of .got.plt.  If we
	     defined _GLOBAL_OFFSET_TABLE_ in a different way, as is
	     permitted by the ABI, we might have to change this
	     calculation.  */
	  relocation -= htab->elf.sgotplt->output_section->vma
			+ htab->elf.sgotplt->output_offset;
	  break;

	case R_386_GOTPC:
	  /* Use global offset table as symbol value.  */
	  relocation = htab->elf.sgotplt->output_section->vma
		       + htab->elf.sgotplt->output_offset;
	  unresolved_reloc = FALSE;
	  break;

	case R_386_PLT32:
	  /* Relocation is to the entry for this symbol in the
	     procedure linkage table.  */

	  /* Resolve a PLT32 reloc against a local symbol directly,
	     without using the procedure linkage table.  */
	  if (h == NULL)
	    break;

	  if ((h->plt.offset == (bfd_vma) -1
	       && eh->plt_got.offset == (bfd_vma) -1)
	      || htab->elf.splt == NULL)
	    {
	      /* We didn't make a PLT entry for this symbol.  This
		 happens when statically linking PIC code, or when
		 using -Bsymbolic.  */
	      break;
	    }

	  if (h->plt.offset != (bfd_vma) -1)
	    {
	      resolved_plt = htab->elf.splt;
	      plt_offset = h->plt.offset;
	    }
	  else
	    {
	      resolved_plt = htab->plt_got;
	      plt_offset = eh->plt_got.offset;
	    }

	  relocation = (resolved_plt->output_section->vma
			+ resolved_plt->output_offset
			+ plt_offset);
	  unresolved_reloc = FALSE;
	  break;

	case R_386_SIZE32:
	  /* Set to symbol size.  */
	  relocation = st_size;
	  /* Fall through.  */

	case R_386_32:
	case R_386_PC32:
	  if ((input_section->flags & SEC_ALLOC) == 0
	      || is_vxworks_tls)
	    break;

	  /* Copy dynamic function pointer relocations.  Don't generate
	     dynamic relocations against resolved undefined weak symbols
	     in PIE, except for R_386_PC32.  */
	  if ((bfd_link_pic (info)
	       && (h == NULL
		   || ((ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
			&& (!resolved_to_zero
			    || r_type == R_386_PC32))
		       || h->root.type != bfd_link_hash_undefweak))
	       && ((r_type != R_386_PC32 && r_type != R_386_SIZE32)
		   || !SYMBOL_CALLS_LOCAL (info, h)))
	      || (ELIMINATE_COPY_RELOCS
		  && !bfd_link_pic (info)
		  && h != NULL
		  && h->dynindx != -1
		  && (!h->non_got_ref
		      || eh->func_pointer_refcount > 0
		      || (h->root.type == bfd_link_hash_undefweak
			  && !resolved_to_zero))
		  && ((h->def_dynamic && !h->def_regular)
		      /* Undefined weak symbol is bound locally when
			 PIC is false.  */
		      || h->root.type == bfd_link_hash_undefweak)))
	    {
	      Elf_Internal_Rela outrel;
	      bfd_boolean skip, relocate;
	      asection *sreloc;

	      /* When generating a shared object, these relocations
		 are copied into the output file to be resolved at run
		 time.  */

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
		       && (r_type == R_386_PC32
			   || !(bfd_link_executable (info)
				|| SYMBOLIC_BIND (info, h))
			   || !h->def_regular))
		outrel.r_info = ELF32_R_INFO (h->dynindx, r_type);
	      else
		{
		  /* This symbol is local, or marked to become local.  */
		  relocate = TRUE;
		  outrel.r_info = ELF32_R_INFO (0, R_386_RELATIVE);
		}

	      sreloc = elf_section_data (input_section)->sreloc;

	      if (sreloc == NULL || sreloc->contents == NULL)
		{
		  r = bfd_reloc_notsupported;
		  goto check_relocation_error;
		}

	      elf_append_rel (output_bfd, sreloc, &outrel);

	      /* If this reloc is against an external symbol, we do
		 not want to fiddle with the addend.  Otherwise, we
		 need to include the symbol value so that it becomes
		 an addend for the dynamic reloc.  */
	      if (! relocate)
		continue;
	    }
	  break;

	case R_386_TLS_IE:
	  if (!bfd_link_executable (info))
	    {
	      Elf_Internal_Rela outrel;
	      asection *sreloc;

	      outrel.r_offset = rel->r_offset
				+ input_section->output_section->vma
				+ input_section->output_offset;
	      outrel.r_info = ELF32_R_INFO (0, R_386_RELATIVE);
	      sreloc = elf_section_data (input_section)->sreloc;
	      if (sreloc == NULL)
		abort ();
	      elf_append_rel (output_bfd, sreloc, &outrel);
	    }
	  /* Fall through */

	case R_386_TLS_GD:
	case R_386_TLS_GOTDESC:
	case R_386_TLS_DESC_CALL:
	case R_386_TLS_IE_32:
	case R_386_TLS_GOTIE:
	  tls_type = GOT_UNKNOWN;
	  if (h == NULL && local_got_offsets)
	    tls_type = elf_i386_local_got_tls_type (input_bfd) [r_symndx];
	  else if (h != NULL)
	    tls_type = elf_i386_hash_entry(h)->tls_type;
	  if (tls_type == GOT_TLS_IE)
	    tls_type = GOT_TLS_IE_NEG;

	  if (! elf_i386_tls_transition (info, input_bfd,
					 input_section, contents,
					 symtab_hdr, sym_hashes,
					 &r_type, tls_type, rel,
					 relend, h, r_symndx, TRUE))
	    return FALSE;

	  if (r_type == R_386_TLS_LE_32)
	    {
	      BFD_ASSERT (! unresolved_reloc);
	      if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_GD)
		{
		  unsigned int type;
		  bfd_vma roff;

		  /* GD->LE transition.  */
		  type = *(contents + rel->r_offset - 2);
		  if (type == 0x04)
		    {
		      /* Change
				leal foo@tlsgd(,%ebx,1), %eax
				call ___tls_get_addr@PLT
			 into:
				movl %gs:0, %eax
				subl $foo@tpoff, %eax
			 (6 byte form of subl).  */
		      roff = rel->r_offset + 5;
		    }
		  else
		    {
		      /* Change
				leal foo@tlsgd(%ebx), %eax
				call ___tls_get_addr@PLT
				nop
			 or
				leal foo@tlsgd(%reg), %eax
				call *___tls_get_addr@GOT(%reg)
				which may be converted to
				addr32 call ___tls_get_addr
			 into:
				movl %gs:0, %eax; subl $foo@tpoff, %eax
			 (6 byte form of subl).  */
		      roff = rel->r_offset + 6;
		    }
		  memcpy (contents + roff - 8,
			  "\x65\xa1\0\0\0\0\x81\xe8\0\0\0", 12);
		  bfd_put_32 (output_bfd, elf_i386_tpoff (info, relocation),
			      contents + roff);
		  /* Skip R_386_PC32, R_386_PLT32 and R_386_GOT32X.  */
		  rel++;
		  wrel++;
		  continue;
		}
	      else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_GOTDESC)
		{
		  /* GDesc -> LE transition.
		     It's originally something like:
		     leal x@tlsdesc(%ebx), %eax

		     leal x@ntpoff, %eax

		     Registers other than %eax may be set up here.  */

		  unsigned int val;
		  bfd_vma roff;

		  roff = rel->r_offset;
		  val = bfd_get_8 (input_bfd, contents + roff - 1);

		  /* Now modify the instruction as appropriate.  */
		  /* aoliva FIXME: remove the above and xor the byte
		     below with 0x86.  */
		  bfd_put_8 (output_bfd, val ^ 0x86,
			     contents + roff - 1);
		  bfd_put_32 (output_bfd, -elf_i386_tpoff (info, relocation),
			      contents + roff);
		  continue;
		}
	      else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_DESC_CALL)
		{
		  /* GDesc -> LE transition.
		     It's originally:
		     call *(%eax)
		     Turn it into:
		     xchg %ax,%ax  */

		  bfd_vma roff;

		  roff = rel->r_offset;
		  bfd_put_8 (output_bfd, 0x66, contents + roff);
		  bfd_put_8 (output_bfd, 0x90, contents + roff + 1);
		  continue;
		}
	      else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_IE)
		{
		  unsigned int val;

		  /* IE->LE transition:
		     Originally it can be one of:
		     movl foo, %eax
		     movl foo, %reg
		     addl foo, %reg
		     We change it into:
		     movl $foo, %eax
		     movl $foo, %reg
		     addl $foo, %reg.  */
		  val = bfd_get_8 (input_bfd, contents + rel->r_offset - 1);
		  if (val == 0xa1)
		    {
		      /* movl foo, %eax.  */
		      bfd_put_8 (output_bfd, 0xb8,
				 contents + rel->r_offset - 1);
		    }
		  else
		    {
		      unsigned int type;

		      type = bfd_get_8 (input_bfd,
					contents + rel->r_offset - 2);
		      switch (type)
			{
			case 0x8b:
			  /* movl */
			  bfd_put_8 (output_bfd, 0xc7,
				     contents + rel->r_offset - 2);
			  bfd_put_8 (output_bfd,
				     0xc0 | ((val >> 3) & 7),
				     contents + rel->r_offset - 1);
			  break;
			case 0x03:
			  /* addl */
			  bfd_put_8 (output_bfd, 0x81,
				     contents + rel->r_offset - 2);
			  bfd_put_8 (output_bfd,
				     0xc0 | ((val >> 3) & 7),
				     contents + rel->r_offset - 1);
			  break;
			default:
			  BFD_FAIL ();
			  break;
			}
		    }
		  bfd_put_32 (output_bfd, -elf_i386_tpoff (info, relocation),
			      contents + rel->r_offset);
		  continue;
		}
	      else
		{
		  unsigned int val, type;

		  /* {IE_32,GOTIE}->LE transition:
		     Originally it can be one of:
		     subl foo(%reg1), %reg2
		     movl foo(%reg1), %reg2
		     addl foo(%reg1), %reg2
		     We change it into:
		     subl $foo, %reg2
		     movl $foo, %reg2 (6 byte form)
		     addl $foo, %reg2.  */
		  type = bfd_get_8 (input_bfd, contents + rel->r_offset - 2);
		  val = bfd_get_8 (input_bfd, contents + rel->r_offset - 1);
		  if (type == 0x8b)
		    {
		      /* movl */
		      bfd_put_8 (output_bfd, 0xc7,
				 contents + rel->r_offset - 2);
		      bfd_put_8 (output_bfd, 0xc0 | ((val >> 3) & 7),
				 contents + rel->r_offset - 1);
		    }
		  else if (type == 0x2b)
		    {
		      /* subl */
		      bfd_put_8 (output_bfd, 0x81,
				 contents + rel->r_offset - 2);
		      bfd_put_8 (output_bfd, 0xe8 | ((val >> 3) & 7),
				 contents + rel->r_offset - 1);
		    }
		  else if (type == 0x03)
		    {
		      /* addl */
		      bfd_put_8 (output_bfd, 0x81,
				 contents + rel->r_offset - 2);
		      bfd_put_8 (output_bfd, 0xc0 | ((val >> 3) & 7),
				 contents + rel->r_offset - 1);
		    }
		  else
		    BFD_FAIL ();
		  if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_GOTIE)
		    bfd_put_32 (output_bfd, -elf_i386_tpoff (info, relocation),
				contents + rel->r_offset);
		  else
		    bfd_put_32 (output_bfd, elf_i386_tpoff (info, relocation),
				contents + rel->r_offset);
		  continue;
		}
	    }

	  if (htab->elf.sgot == NULL)
	    abort ();

	  if (h != NULL)
	    {
	      off = h->got.offset;
	      offplt = elf_i386_hash_entry (h)->tlsdesc_got;
	    }
	  else
	    {
	      if (local_got_offsets == NULL)
		abort ();

	      off = local_got_offsets[r_symndx];
	      offplt = local_tlsdesc_gotents[r_symndx];
	    }

	  if ((off & 1) != 0)
	    off &= ~1;
	  else
	    {
	      Elf_Internal_Rela outrel;
	      int dr_type;
	      asection *sreloc;

	      if (htab->elf.srelgot == NULL)
		abort ();

	      indx = h && h->dynindx != -1 ? h->dynindx : 0;

	      if (GOT_TLS_GDESC_P (tls_type))
		{
		  bfd_byte *loc;
		  outrel.r_info = ELF32_R_INFO (indx, R_386_TLS_DESC);
		  BFD_ASSERT (htab->sgotplt_jump_table_size + offplt + 8
			      <= htab->elf.sgotplt->size);
		  outrel.r_offset = (htab->elf.sgotplt->output_section->vma
				     + htab->elf.sgotplt->output_offset
				     + offplt
				     + htab->sgotplt_jump_table_size);
		  sreloc = htab->elf.srelplt;
		  loc = sreloc->contents;
		  loc += (htab->next_tls_desc_index++
			  * sizeof (Elf32_External_Rel));
		  BFD_ASSERT (loc + sizeof (Elf32_External_Rel)
			      <= sreloc->contents + sreloc->size);
		  bfd_elf32_swap_reloc_out (output_bfd, &outrel, loc);
		  if (indx == 0)
		    {
		      BFD_ASSERT (! unresolved_reloc);
		      bfd_put_32 (output_bfd,
				  relocation - elf_i386_dtpoff_base (info),
				  htab->elf.sgotplt->contents + offplt
				  + htab->sgotplt_jump_table_size + 4);
		    }
		  else
		    {
		      bfd_put_32 (output_bfd, 0,
				  htab->elf.sgotplt->contents + offplt
				  + htab->sgotplt_jump_table_size + 4);
		    }
		}

	      sreloc = htab->elf.srelgot;

	      outrel.r_offset = (htab->elf.sgot->output_section->vma
				 + htab->elf.sgot->output_offset + off);

	      if (GOT_TLS_GD_P (tls_type))
		dr_type = R_386_TLS_DTPMOD32;
	      else if (GOT_TLS_GDESC_P (tls_type))
		goto dr_done;
	      else if (tls_type == GOT_TLS_IE_POS)
		dr_type = R_386_TLS_TPOFF;
	      else
		dr_type = R_386_TLS_TPOFF32;

	      if (dr_type == R_386_TLS_TPOFF && indx == 0)
		bfd_put_32 (output_bfd,
			    relocation - elf_i386_dtpoff_base (info),
			    htab->elf.sgot->contents + off);
	      else if (dr_type == R_386_TLS_TPOFF32 && indx == 0)
		bfd_put_32 (output_bfd,
			    elf_i386_dtpoff_base (info) - relocation,
			    htab->elf.sgot->contents + off);
	      else if (dr_type != R_386_TLS_DESC)
		bfd_put_32 (output_bfd, 0,
			    htab->elf.sgot->contents + off);
	      outrel.r_info = ELF32_R_INFO (indx, dr_type);

	      elf_append_rel (output_bfd, sreloc, &outrel);

	      if (GOT_TLS_GD_P (tls_type))
		{
		  if (indx == 0)
		    {
	    	      BFD_ASSERT (! unresolved_reloc);
		      bfd_put_32 (output_bfd,
				  relocation - elf_i386_dtpoff_base (info),
				  htab->elf.sgot->contents + off + 4);
		    }
		  else
		    {
		      bfd_put_32 (output_bfd, 0,
				  htab->elf.sgot->contents + off + 4);
		      outrel.r_info = ELF32_R_INFO (indx,
						    R_386_TLS_DTPOFF32);
		      outrel.r_offset += 4;
		      elf_append_rel (output_bfd, sreloc, &outrel);
		    }
		}
	      else if (tls_type == GOT_TLS_IE_BOTH)
		{
		  bfd_put_32 (output_bfd,
			      (indx == 0
			       ? relocation - elf_i386_dtpoff_base (info)
			       : 0),
			      htab->elf.sgot->contents + off + 4);
		  outrel.r_info = ELF32_R_INFO (indx, R_386_TLS_TPOFF);
		  outrel.r_offset += 4;
		  elf_append_rel (output_bfd, sreloc, &outrel);
		}

	    dr_done:
	      if (h != NULL)
		h->got.offset |= 1;
	      else
		local_got_offsets[r_symndx] |= 1;
	    }

	  if (off >= (bfd_vma) -2
	      && ! GOT_TLS_GDESC_P (tls_type))
	    abort ();
	  if (r_type == R_386_TLS_GOTDESC
	      || r_type == R_386_TLS_DESC_CALL)
	    {
	      relocation = htab->sgotplt_jump_table_size + offplt;
	      unresolved_reloc = FALSE;
	    }
	  else if (r_type == ELF32_R_TYPE (rel->r_info))
	    {
	      bfd_vma g_o_t = htab->elf.sgotplt->output_section->vma
			      + htab->elf.sgotplt->output_offset;
	      relocation = htab->elf.sgot->output_section->vma
		+ htab->elf.sgot->output_offset + off - g_o_t;
	      if ((r_type == R_386_TLS_IE || r_type == R_386_TLS_GOTIE)
		  && tls_type == GOT_TLS_IE_BOTH)
		relocation += 4;
	      if (r_type == R_386_TLS_IE)
		relocation += g_o_t;
	      unresolved_reloc = FALSE;
	    }
	  else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_GD)
	    {
	      unsigned int val, type;
	      bfd_vma roff;

	      /* GD->IE transition.  */
	      type = *(contents + rel->r_offset - 2);
	      val = *(contents + rel->r_offset - 1);
	      if (type == 0x04)
		{
		  /* Change
			leal foo@tlsgd(,%ebx,1), %eax
			call ___tls_get_addr@PLT
		     into:
			movl %gs:0, %eax
			subl $foo@gottpoff(%ebx), %eax.  */
		  val >>= 3;
		  roff = rel->r_offset - 3;
		}
	      else
		{
		  /* Change
			leal foo@tlsgd(%ebx), %eax
			call ___tls_get_addr@PLT
			nop
		     or
			leal foo@tlsgd(%reg), %eax
			call *___tls_get_addr@GOT(%reg)
			which may be converted to
			addr32 call ___tls_get_addr
		     into:
			movl %gs:0, %eax;
			subl $foo@gottpoff(%reg), %eax.  */
		  roff = rel->r_offset - 2;
		}
	      memcpy (contents + roff,
		      "\x65\xa1\0\0\0\0\x2b\x80\0\0\0", 12);
	      contents[roff + 7] = 0x80 | (val & 7);
	      /* If foo is used only with foo@gotntpoff(%reg) and
		 foo@indntpoff, but not with foo@gottpoff(%reg), change
		 subl $foo@gottpoff(%reg), %eax
		 into:
		 addl $foo@gotntpoff(%reg), %eax.  */
	      if (tls_type == GOT_TLS_IE_POS)
		contents[roff + 6] = 0x03;
	      bfd_put_32 (output_bfd,
			  htab->elf.sgot->output_section->vma
			  + htab->elf.sgot->output_offset + off
			  - htab->elf.sgotplt->output_section->vma
			  - htab->elf.sgotplt->output_offset,
			  contents + roff + 8);
	      /* Skip R_386_PLT32 and R_386_GOT32X.  */
	      rel++;
	      wrel++;
	      continue;
	    }
	  else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_GOTDESC)
	    {
	      /* GDesc -> IE transition.
		 It's originally something like:
		 leal x@tlsdesc(%ebx), %eax

		 Change it to:
		 movl x@gotntpoff(%ebx), %eax # before xchg %ax,%ax
		 or:
		 movl x@gottpoff(%ebx), %eax # before negl %eax

		 Registers other than %eax may be set up here.  */

	      bfd_vma roff;

	      /* First, make sure it's a leal adding ebx to a 32-bit
		 offset into any register, although it's probably
		 almost always going to be eax.  */
	      roff = rel->r_offset;

	      /* Now modify the instruction as appropriate.  */
	      /* To turn a leal into a movl in the form we use it, it
		 suffices to change the first byte from 0x8d to 0x8b.
		 aoliva FIXME: should we decide to keep the leal, all
		 we have to do is remove the statement below, and
		 adjust the relaxation of R_386_TLS_DESC_CALL.  */
	      bfd_put_8 (output_bfd, 0x8b, contents + roff - 2);

	      if (tls_type == GOT_TLS_IE_BOTH)
		off += 4;

	      bfd_put_32 (output_bfd,
			  htab->elf.sgot->output_section->vma
			  + htab->elf.sgot->output_offset + off
			  - htab->elf.sgotplt->output_section->vma
			  - htab->elf.sgotplt->output_offset,
			  contents + roff);
	      continue;
	    }
	  else if (ELF32_R_TYPE (rel->r_info) == R_386_TLS_DESC_CALL)
	    {
	      /* GDesc -> IE transition.
		 It's originally:
		 call *(%eax)

		 Change it to:
		 xchg %ax,%ax
		 or
		 negl %eax
		 depending on how we transformed the TLS_GOTDESC above.
	      */

	      bfd_vma roff;

	      roff = rel->r_offset;

	      /* Now modify the instruction as appropriate.  */
	      if (tls_type != GOT_TLS_IE_NEG)
		{
		  /* xchg %ax,%ax */
		  bfd_put_8 (output_bfd, 0x66, contents + roff);
		  bfd_put_8 (output_bfd, 0x90, contents + roff + 1);
		}
	      else
		{
		  /* negl %eax */
		  bfd_put_8 (output_bfd, 0xf7, contents + roff);
		  bfd_put_8 (output_bfd, 0xd8, contents + roff + 1);
		}

	      continue;
	    }
	  else
	    BFD_ASSERT (FALSE);
	  break;

	case R_386_TLS_LDM:
	  if (! elf_i386_tls_transition (info, input_bfd,
					 input_section, contents,
					 symtab_hdr, sym_hashes,
					 &r_type, GOT_UNKNOWN, rel,
					 relend, h, r_symndx, TRUE))
	    return FALSE;

	  if (r_type != R_386_TLS_LDM)
	    {
	      /* LD->LE transition.  Change
			leal foo@tlsldm(%ebx) %eax
			call ___tls_get_addr@PLT
		 into:
			movl %gs:0, %eax
			nop
			leal 0(%esi,1), %esi
		 or change
			leal foo@tlsldm(%reg) %eax
			call *___tls_get_addr@GOT(%reg)
			which may be converted to
			addr32 call ___tls_get_addr
		 into:
			movl %gs:0, %eax
			leal 0(%esi), %esi  */
	      BFD_ASSERT (r_type == R_386_TLS_LE_32);
	      if (*(contents + rel->r_offset + 4) == 0xff
		  || *(contents + rel->r_offset + 4) == 0x67)
		memcpy (contents + rel->r_offset - 2,
			"\x65\xa1\0\0\0\0\x8d\xb6\0\0\0", 12);
	      else
		memcpy (contents + rel->r_offset - 2,
			"\x65\xa1\0\0\0\0\x90\x8d\x74\x26", 11);
	      /* Skip R_386_PC32/R_386_PLT32.  */
	      rel++;
	      wrel++;
	      continue;
	    }

	  if (htab->elf.sgot == NULL)
	    abort ();

	  off = htab->tls_ldm_got.offset;
	  if (off & 1)
	    off &= ~1;
	  else
	    {
	      Elf_Internal_Rela outrel;

	      if (htab->elf.srelgot == NULL)
		abort ();

	      outrel.r_offset = (htab->elf.sgot->output_section->vma
				 + htab->elf.sgot->output_offset + off);

	      bfd_put_32 (output_bfd, 0,
			  htab->elf.sgot->contents + off);
	      bfd_put_32 (output_bfd, 0,
			  htab->elf.sgot->contents + off + 4);
	      outrel.r_info = ELF32_R_INFO (0, R_386_TLS_DTPMOD32);
	      elf_append_rel (output_bfd, htab->elf.srelgot, &outrel);
	      htab->tls_ldm_got.offset |= 1;
	    }
	  relocation = htab->elf.sgot->output_section->vma
		       + htab->elf.sgot->output_offset + off
		       - htab->elf.sgotplt->output_section->vma
		       - htab->elf.sgotplt->output_offset;
	  unresolved_reloc = FALSE;
	  break;

	case R_386_TLS_LDO_32:
	  if (!bfd_link_executable (info)
	      || (input_section->flags & SEC_CODE) == 0)
	    relocation -= elf_i386_dtpoff_base (info);
	  else
	    /* When converting LDO to LE, we must negate.  */
	    relocation = -elf_i386_tpoff (info, relocation);
	  break;

	case R_386_TLS_LE_32:
	case R_386_TLS_LE:
	  if (!bfd_link_executable (info))
	    {
	      Elf_Internal_Rela outrel;
	      asection *sreloc;

	      outrel.r_offset = rel->r_offset
				+ input_section->output_section->vma
				+ input_section->output_offset;
	      if (h != NULL && h->dynindx != -1)
		indx = h->dynindx;
	      else
		indx = 0;
	      if (r_type == R_386_TLS_LE_32)
		outrel.r_info = ELF32_R_INFO (indx, R_386_TLS_TPOFF32);
	      else
		outrel.r_info = ELF32_R_INFO (indx, R_386_TLS_TPOFF);
	      sreloc = elf_section_data (input_section)->sreloc;
	      if (sreloc == NULL)
		abort ();
	      elf_append_rel (output_bfd, sreloc, &outrel);
	      if (indx)
		continue;
	      else if (r_type == R_386_TLS_LE_32)
		relocation = elf_i386_dtpoff_base (info) - relocation;
	      else
		relocation -= elf_i386_dtpoff_base (info);
	    }
	  else if (r_type == R_386_TLS_LE_32)
	    relocation = elf_i386_tpoff (info, relocation);
	  else
	    relocation = -elf_i386_tpoff (info, relocation);
	  break;

	default:
	  break;
	}

      /* Dynamic relocs are not propagated for SEC_DEBUGGING sections
	 because such sections are not SEC_ALLOC and thus ld.so will
	 not process them.  */
      if (unresolved_reloc
	  && !((input_section->flags & SEC_DEBUGGING) != 0
	       && h->def_dynamic)
	  && _bfd_elf_section_offset (output_bfd, info, input_section,
				      rel->r_offset) != (bfd_vma) -1)
	{
	  _bfd_error_handler
	    /* xgettext:c-format */
	    (_("%B(%A+0x%lx): unresolvable %s relocation against symbol `%s'"),
	     input_bfd,
	     input_section,
	     (long) rel->r_offset,
	     howto->name,
	     h->root.root.string);
	  return FALSE;
	}

do_relocation:
      r = _bfd_final_link_relocate (howto, input_bfd, input_section,
				    contents, rel->r_offset,
				    relocation, 0);

check_relocation_error:
      if (r != bfd_reloc_ok)
	{
	  const char *name;

	  if (h != NULL)
	    name = h->root.root.string;
	  else
	    {
	      name = bfd_elf_string_from_elf_section (input_bfd,
						      symtab_hdr->sh_link,
						      sym->st_name);
	      if (name == NULL)
		return FALSE;
	      if (*name == '\0')
		name = bfd_section_name (input_bfd, sec);
	    }

	  if (r == bfd_reloc_overflow)
	    (*info->callbacks->reloc_overflow)
	      (info, (h ? &h->root : NULL), name, howto->name,
	       (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	  else
	    {
	      _bfd_error_handler
		/* xgettext:c-format */
		(_("%B(%A+0x%lx): reloc against `%s': error %d"),
		 input_bfd, input_section,
		 (long) rel->r_offset, name, (int) r);
	      return FALSE;
	    }
	}

      if (wrel != rel)
	*wrel = *rel;
    }

  if (wrel != rel)
    {
      Elf_Internal_Shdr *rel_hdr;
      size_t deleted = rel - wrel;

      rel_hdr = _bfd_elf_single_rel_hdr (input_section->output_section);
      rel_hdr->sh_size -= rel_hdr->sh_entsize * deleted;
      if (rel_hdr->sh_size == 0)
	{
	  /* It is too late to remove an empty reloc section.  Leave
	     one NONE reloc.
	     ??? What is wrong with an empty section???  */
	  rel_hdr->sh_size = rel_hdr->sh_entsize;
	  deleted -= 1;
	}
      rel_hdr = _bfd_elf_single_rel_hdr (input_section);
      rel_hdr->sh_size -= rel_hdr->sh_entsize * deleted;
      input_section->reloc_count -= deleted;
    }

  return TRUE;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

static bfd_boolean
elf_i386_finish_dynamic_symbol (bfd *output_bfd,
				struct bfd_link_info *info,
				struct elf_link_hash_entry *h,
				Elf_Internal_Sym *sym)
{
  struct elf_i386_link_hash_table *htab;
  unsigned plt_entry_size;
  const struct elf_i386_backend_data *abed;
  struct elf_i386_link_hash_entry *eh;
  bfd_boolean local_undefweak;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  abed = get_elf_i386_backend_data (output_bfd);
  plt_entry_size = GET_PLT_ENTRY_SIZE (output_bfd);

  eh = (struct elf_i386_link_hash_entry *) h;

  /* We keep PLT/GOT entries without dynamic PLT/GOT relocations for
     resolved undefined weak symbols in executable so that their
     references have value 0 at run-time.  */
  local_undefweak = UNDEFINED_WEAK_RESOLVED_TO_ZERO (info,
						     eh->has_got_reloc,
						     eh);

  if (h->plt.offset != (bfd_vma) -1)
    {
      bfd_vma plt_index;
      bfd_vma got_offset;
      Elf_Internal_Rela rel;
      bfd_byte *loc;
      asection *plt, *gotplt, *relplt;

      /* When building a static executable, use .iplt, .igot.plt and
	 .rel.iplt sections for STT_GNU_IFUNC symbols.  */
      if (htab->elf.splt != NULL)
	{
	  plt = htab->elf.splt;
	  gotplt = htab->elf.sgotplt;
	  relplt = htab->elf.srelplt;
	}
      else
	{
	  plt = htab->elf.iplt;
	  gotplt = htab->elf.igotplt;
	  relplt = htab->elf.irelplt;
	}

      /* This symbol has an entry in the procedure linkage table.  Set
	 it up.  */

      if ((h->dynindx == -1
	   && !local_undefweak
	   && !((h->forced_local || bfd_link_executable (info))
		&& h->def_regular
		&& h->type == STT_GNU_IFUNC))
	  || plt == NULL
	  || gotplt == NULL
	  || relplt == NULL)
	abort ();

      /* Get the index in the procedure linkage table which
	 corresponds to this symbol.  This is the index of this symbol
	 in all the symbols for which we are making plt entries.  The
	 first entry in the procedure linkage table is reserved.

	 Get the offset into the .got table of the entry that
	 corresponds to this function.  Each .got entry is 4 bytes.
	 The first three are reserved.

	 For static executables, we don't reserve anything.  */

      if (plt == htab->elf.splt)
	{
	  got_offset = h->plt.offset / plt_entry_size - 1;
	  got_offset = (got_offset + 3) * 4;
	}
      else
	{
	  got_offset = h->plt.offset / plt_entry_size;
	  got_offset = got_offset * 4;
	}

      /* Fill in the entry in the procedure linkage table.  */
      if (! bfd_link_pic (info))
	{
	  memcpy (plt->contents + h->plt.offset, abed->plt->plt_entry,
		  abed->plt->plt_entry_size);
	  bfd_put_32 (output_bfd,
		      (gotplt->output_section->vma
		       + gotplt->output_offset
		       + got_offset),
		      plt->contents + h->plt.offset
                      + abed->plt->plt_got_offset);

	  if (abed->is_vxworks)
	    {
	      int s, k, reloc_index;

	      /* Create the R_386_32 relocation referencing the GOT
		 for this PLT entry.  */

	      /* S: Current slot number (zero-based).  */
	      s = ((h->plt.offset - abed->plt->plt_entry_size)
                   / abed->plt->plt_entry_size);
	      /* K: Number of relocations for PLTResolve. */
	      if (bfd_link_pic (info))
		k = PLTRESOLVE_RELOCS_SHLIB;
	      else
		k = PLTRESOLVE_RELOCS;
	      /* Skip the PLTresolve relocations, and the relocations for
		 the other PLT slots. */
	      reloc_index = k + s * PLT_NON_JUMP_SLOT_RELOCS;
	      loc = (htab->srelplt2->contents + reloc_index
		     * sizeof (Elf32_External_Rel));

	      rel.r_offset = (htab->elf.splt->output_section->vma
			      + htab->elf.splt->output_offset
			      + h->plt.offset + 2),
	      rel.r_info = ELF32_R_INFO (htab->elf.hgot->indx, R_386_32);
	      bfd_elf32_swap_reloc_out (output_bfd, &rel, loc);

	      /* Create the R_386_32 relocation referencing the beginning of
		 the PLT for this GOT entry.  */
	      rel.r_offset = (htab->elf.sgotplt->output_section->vma
			      + htab->elf.sgotplt->output_offset
			      + got_offset);
	      rel.r_info = ELF32_R_INFO (htab->elf.hplt->indx, R_386_32);
	      bfd_elf32_swap_reloc_out (output_bfd, &rel,
					loc + sizeof (Elf32_External_Rel));
	    }
	}
      else
	{
	  memcpy (plt->contents + h->plt.offset, abed->plt->pic_plt_entry,
		  abed->plt->plt_entry_size);
	  bfd_put_32 (output_bfd, got_offset,
		      plt->contents + h->plt.offset
                      + abed->plt->plt_got_offset);
	}

      /* Fill in the entry in the global offset table.  Leave the entry
	 as zero for undefined weak symbol in PIE.  No PLT relocation
	 against undefined weak symbol in PIE.  */
      if (!local_undefweak)
	{
	  bfd_put_32 (output_bfd,
		      (plt->output_section->vma
		       + plt->output_offset
		       + h->plt.offset
		       + abed->plt->plt_lazy_offset),
		      gotplt->contents + got_offset);

	  /* Fill in the entry in the .rel.plt section.  */
	  rel.r_offset = (gotplt->output_section->vma
			  + gotplt->output_offset
			  + got_offset);
	  if (h->dynindx == -1
	      || ((bfd_link_executable (info)
		   || ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
		  && h->def_regular
		  && h->type == STT_GNU_IFUNC))
	    {
	      /* If an STT_GNU_IFUNC symbol is locally defined, generate
		 R_386_IRELATIVE instead of R_386_JUMP_SLOT.  Store addend
		 in the .got.plt section.  */
	      bfd_put_32 (output_bfd,
			  (h->root.u.def.value
			   + h->root.u.def.section->output_section->vma
			   + h->root.u.def.section->output_offset),
			  gotplt->contents + got_offset);
	      rel.r_info = ELF32_R_INFO (0, R_386_IRELATIVE);
	      /* R_386_IRELATIVE comes last.  */
	      plt_index = htab->next_irelative_index--;
	    }
	  else
	    {
	      rel.r_info = ELF32_R_INFO (h->dynindx, R_386_JUMP_SLOT);
	      plt_index = htab->next_jump_slot_index++;
	    }

	  loc = relplt->contents + plt_index * sizeof (Elf32_External_Rel);
	  bfd_elf32_swap_reloc_out (output_bfd, &rel, loc);

	  /* Don't fill PLT entry for static executables.  */
	  if (plt == htab->elf.splt)
	    {
	      bfd_put_32 (output_bfd,
			  plt_index * sizeof (Elf32_External_Rel),
			  plt->contents + h->plt.offset
			  + abed->plt->plt_reloc_offset);
	      bfd_put_32 (output_bfd, - (h->plt.offset
					 + abed->plt->plt_plt_offset + 4),
			  plt->contents + h->plt.offset
			  + abed->plt->plt_plt_offset);
	    }
	}
    }
  else if (eh->plt_got.offset != (bfd_vma) -1)
    {
      bfd_vma got_offset, plt_offset;
      asection *plt, *got, *gotplt;
      const bfd_byte *got_plt_entry;

      /* Offset of displacement of the indirect jump.  */
      bfd_vma plt_got_offset = 2;

      /* Set the entry in the GOT procedure linkage table.  */
      plt = htab->plt_got;
      got = htab->elf.sgot;
      gotplt = htab->elf.sgotplt;
      got_offset = h->got.offset;

      if (got_offset == (bfd_vma) -1
	  || plt == NULL
	  || got == NULL
	  || gotplt == NULL)
	abort ();

      /* Fill in the entry in the GOT procedure linkage table.  */
      if (! bfd_link_pic (info))
	{
	  got_plt_entry = elf_i386_got_plt_entry;
	  got_offset += got->output_section->vma + got->output_offset;
	}
      else
	{
	  got_plt_entry = elf_i386_pic_got_plt_entry;
	  got_offset += (got->output_section->vma
			 + got->output_offset
			 - gotplt->output_section->vma
			 - gotplt->output_offset);
	}

      plt_offset = eh->plt_got.offset;
      memcpy (plt->contents + plt_offset, got_plt_entry,
	      sizeof (elf_i386_got_plt_entry));
      bfd_put_32 (output_bfd, got_offset,
		  plt->contents + plt_offset + plt_got_offset);
    }

  if (!local_undefweak
      && !h->def_regular
      && (h->plt.offset != (bfd_vma) -1
	  || eh->plt_got.offset != (bfd_vma) -1))
    {
      /* Mark the symbol as undefined, rather than as defined in
	 the .plt section.  Leave the value if there were any
	 relocations where pointer equality matters (this is a clue
	 for the dynamic linker, to make function pointer
	 comparisons work between an application and shared
	 library), otherwise set it to zero.  If a function is only
	 called from a binary, there is no need to slow down
	 shared libraries because of that.  */
      sym->st_shndx = SHN_UNDEF;
      if (!h->pointer_equality_needed)
	sym->st_value = 0;
    }

  /* Don't generate dynamic GOT relocation against undefined weak
     symbol in executable.  */
  if (h->got.offset != (bfd_vma) -1
      && ! GOT_TLS_GD_ANY_P (elf_i386_hash_entry(h)->tls_type)
      && (elf_i386_hash_entry(h)->tls_type & GOT_TLS_IE) == 0
      && !local_undefweak)
    {
      Elf_Internal_Rela rel;
      asection *relgot = htab->elf.srelgot;

      /* This symbol has an entry in the global offset table.  Set it
	 up.  */

      if (htab->elf.sgot == NULL || htab->elf.srelgot == NULL)
	abort ();

      rel.r_offset = (htab->elf.sgot->output_section->vma
		      + htab->elf.sgot->output_offset
		      + (h->got.offset & ~(bfd_vma) 1));

      /* If this is a static link, or it is a -Bsymbolic link and the
	 symbol is defined locally or was forced to be local because
	 of a version file, we just want to emit a RELATIVE reloc.
	 The entry in the global offset table will already have been
	 initialized in the relocate_section function.  */
      if (h->def_regular
	  && h->type == STT_GNU_IFUNC)
	{
	  if (h->plt.offset == (bfd_vma) -1)
	    {
	      /* STT_GNU_IFUNC is referenced without PLT.  */
	      if (htab->elf.splt == NULL)
		{
		  /* use .rel[a].iplt section to store .got relocations
		     in static executable.  */
		  relgot = htab->elf.irelplt;
		}
	      if (SYMBOL_REFERENCES_LOCAL (info, h))
		{
		  bfd_put_32 (output_bfd,
			      (h->root.u.def.value
			       + h->root.u.def.section->output_section->vma
			       + h->root.u.def.section->output_offset),
			      htab->elf.sgot->contents + h->got.offset);
		  rel.r_info = ELF32_R_INFO (0, R_386_IRELATIVE);
		}
	      else
		goto do_glob_dat;
	    }
	  else if (bfd_link_pic (info))
	    {
	      /* Generate R_386_GLOB_DAT.  */
	      goto do_glob_dat;
	    }
	  else
	    {
	      asection *plt;

	      if (!h->pointer_equality_needed)
		abort ();

	      /* For non-shared object, we can't use .got.plt, which
		 contains the real function addres if we need pointer
		 equality.  We load the GOT entry with the PLT entry.  */
	      plt = htab->elf.splt ? htab->elf.splt : htab->elf.iplt;
	      bfd_put_32 (output_bfd,
			  (plt->output_section->vma
			   + plt->output_offset + h->plt.offset),
			  htab->elf.sgot->contents + h->got.offset);
	      return TRUE;
	    }
	}
      else if (bfd_link_pic (info)
	       && SYMBOL_REFERENCES_LOCAL (info, h))
	{
	  BFD_ASSERT((h->got.offset & 1) != 0);
	  rel.r_info = ELF32_R_INFO (0, R_386_RELATIVE);
	}
      else
	{
	  BFD_ASSERT((h->got.offset & 1) == 0);
do_glob_dat:
	  bfd_put_32 (output_bfd, (bfd_vma) 0,
		      htab->elf.sgot->contents + h->got.offset);
	  rel.r_info = ELF32_R_INFO (h->dynindx, R_386_GLOB_DAT);
	}

      elf_append_rel (output_bfd, relgot, &rel);
    }

  if (h->needs_copy)
    {
      Elf_Internal_Rela rel;
      asection *s;

      /* This symbol needs a copy reloc.  Set it up.  */

      if (h->dynindx == -1
	  || (h->root.type != bfd_link_hash_defined
	      && h->root.type != bfd_link_hash_defweak)
	  || htab->elf.srelbss == NULL
	  || htab->elf.sreldynrelro == NULL)
	abort ();

      rel.r_offset = (h->root.u.def.value
		      + h->root.u.def.section->output_section->vma
		      + h->root.u.def.section->output_offset);
      rel.r_info = ELF32_R_INFO (h->dynindx, R_386_COPY);
      if (h->root.u.def.section == htab->elf.sdynrelro)
	s = htab->elf.sreldynrelro;
      else
	s = htab->elf.srelbss;
      elf_append_rel (output_bfd, s, &rel);
    }

  return TRUE;
}

/* Finish up local dynamic symbol handling.  We set the contents of
   various dynamic sections here.  */

static bfd_boolean
elf_i386_finish_local_dynamic_symbol (void **slot, void *inf)
{
  struct elf_link_hash_entry *h
    = (struct elf_link_hash_entry *) *slot;
  struct bfd_link_info *info
    = (struct bfd_link_info *) inf;

  return elf_i386_finish_dynamic_symbol (info->output_bfd, info,
					 h, NULL);
}

/* Finish up undefined weak symbol handling in PIE.  Fill its PLT entry
   here since undefined weak symbol may not be dynamic and may not be
   called for elf_i386_finish_dynamic_symbol.  */

static bfd_boolean
elf_i386_pie_finish_undefweak_symbol (struct bfd_hash_entry *bh,
				      void *inf)
{
  struct elf_link_hash_entry *h = (struct elf_link_hash_entry *) bh;
  struct bfd_link_info *info = (struct bfd_link_info *) inf;

  if (h->root.type != bfd_link_hash_undefweak
      || h->dynindx != -1)
    return TRUE;

  return elf_i386_finish_dynamic_symbol (info->output_bfd,
					     info, h, NULL);
}

/* Used to decide how to sort relocs in an optimal manner for the
   dynamic linker, before writing them out.  */

static enum elf_reloc_type_class
elf_i386_reloc_type_class (const struct bfd_link_info *info,
			   const asection *rel_sec ATTRIBUTE_UNUSED,
			   const Elf_Internal_Rela *rela)
{
  bfd *abfd = info->output_bfd;
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  struct elf_link_hash_table *htab = elf_hash_table (info);

  if (htab->dynsym != NULL
      && htab->dynsym->contents != NULL)
    {
      /* Check relocation against STT_GNU_IFUNC symbol if there are
         dynamic symbols.  */
      unsigned long r_symndx = ELF32_R_SYM (rela->r_info);
      if (r_symndx != STN_UNDEF)
	{
	  Elf_Internal_Sym sym;
	  if (!bed->s->swap_symbol_in (abfd,
				       (htab->dynsym->contents
					+ r_symndx * sizeof (Elf32_External_Sym)),
				       0, &sym))
	    abort ();

	  if (ELF32_ST_TYPE (sym.st_info) == STT_GNU_IFUNC)
	    return reloc_class_ifunc;
	}
    }

  switch (ELF32_R_TYPE (rela->r_info))
    {
    case R_386_IRELATIVE:
      return reloc_class_ifunc;
    case R_386_RELATIVE:
      return reloc_class_relative;
    case R_386_JUMP_SLOT:
      return reloc_class_plt;
    case R_386_COPY:
      return reloc_class_copy;
    default:
      return reloc_class_normal;
    }
}

/* Finish up the dynamic sections.  */

static bfd_boolean
elf_i386_finish_dynamic_sections (bfd *output_bfd,
				  struct bfd_link_info *info)
{
  struct elf_i386_link_hash_table *htab;
  bfd *dynobj;
  asection *sdyn;
  const struct elf_i386_backend_data *abed;

  htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  dynobj = htab->elf.dynobj;
  sdyn = bfd_get_linker_section (dynobj, ".dynamic");
  abed = get_elf_i386_backend_data (output_bfd);

  if (htab->elf.dynamic_sections_created)
    {
      Elf32_External_Dyn *dyncon, *dynconend;

      if (sdyn == NULL || htab->elf.sgot == NULL)
	abort ();

      dyncon = (Elf32_External_Dyn *) sdyn->contents;
      dynconend = (Elf32_External_Dyn *) (sdyn->contents + sdyn->size);
      for (; dyncon < dynconend; dyncon++)
	{
	  Elf_Internal_Dyn dyn;
	  asection *s;

	  bfd_elf32_swap_dyn_in (dynobj, dyncon, &dyn);

	  switch (dyn.d_tag)
	    {
	    default:
	      if (abed->is_vxworks
                  && elf_vxworks_finish_dynamic_entry (output_bfd, &dyn))
		break;
	      continue;

	    case DT_PLTGOT:
	      s = htab->elf.sgotplt;
	      dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	      break;

	    case DT_JMPREL:
	      s = htab->elf.srelplt;
	      dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	      break;

	    case DT_PLTRELSZ:
	      s = htab->elf.srelplt;
	      dyn.d_un.d_val = s->size;
	      break;
	    }

	  bfd_elf32_swap_dyn_out (output_bfd, &dyn, dyncon);
	}

      /* Fill in the first entry in the procedure linkage table.  */
      if (htab->elf.splt && htab->elf.splt->size > 0)
	{
	  if (bfd_link_pic (info))
	    {
	      memcpy (htab->elf.splt->contents, abed->plt->pic_plt0_entry,
		      abed->plt->plt0_entry_size);
	      memset (htab->elf.splt->contents + abed->plt->plt0_entry_size,
		      abed->plt0_pad_byte,
		      abed->plt->plt_entry_size - abed->plt->plt0_entry_size);
	    }
	  else
	    {
	      memcpy (htab->elf.splt->contents, abed->plt->plt0_entry,
		      abed->plt->plt0_entry_size);
	      memset (htab->elf.splt->contents + abed->plt->plt0_entry_size,
		      abed->plt0_pad_byte,
		      abed->plt->plt_entry_size - abed->plt->plt0_entry_size);
	      bfd_put_32 (output_bfd,
			  (htab->elf.sgotplt->output_section->vma
			   + htab->elf.sgotplt->output_offset
			   + 4),
			  htab->elf.splt->contents
                          + abed->plt->plt0_got1_offset);
	      bfd_put_32 (output_bfd,
			  (htab->elf.sgotplt->output_section->vma
			   + htab->elf.sgotplt->output_offset
			   + 8),
			  htab->elf.splt->contents
                          + abed->plt->plt0_got2_offset);

	      if (abed->is_vxworks)
		{
		  Elf_Internal_Rela rel;

		  /* Generate a relocation for _GLOBAL_OFFSET_TABLE_ + 4.
		     On IA32 we use REL relocations so the addend goes in
		     the PLT directly.  */
		  rel.r_offset = (htab->elf.splt->output_section->vma
				  + htab->elf.splt->output_offset
				  + abed->plt->plt0_got1_offset);
		  rel.r_info = ELF32_R_INFO (htab->elf.hgot->indx, R_386_32);
		  bfd_elf32_swap_reloc_out (output_bfd, &rel,
					    htab->srelplt2->contents);
		  /* Generate a relocation for _GLOBAL_OFFSET_TABLE_ + 8.  */
		  rel.r_offset = (htab->elf.splt->output_section->vma
				  + htab->elf.splt->output_offset
				  + abed->plt->plt0_got2_offset);
		  rel.r_info = ELF32_R_INFO (htab->elf.hgot->indx, R_386_32);
		  bfd_elf32_swap_reloc_out (output_bfd, &rel,
					    htab->srelplt2->contents +
					    sizeof (Elf32_External_Rel));
		}
	    }

	  /* UnixWare sets the entsize of .plt to 4, although that doesn't
	     really seem like the right value.  */
	  elf_section_data (htab->elf.splt->output_section)
	    ->this_hdr.sh_entsize = 4;

	  /* Correct the .rel.plt.unloaded relocations.  */
	  if (abed->is_vxworks && !bfd_link_pic (info))
	    {
	      int num_plts = (htab->elf.splt->size
                              / abed->plt->plt_entry_size) - 1;
	      unsigned char *p;

	      p = htab->srelplt2->contents;
	      if (bfd_link_pic (info))
		p += PLTRESOLVE_RELOCS_SHLIB * sizeof (Elf32_External_Rel);
	      else
		p += PLTRESOLVE_RELOCS * sizeof (Elf32_External_Rel);

	      for (; num_plts; num_plts--)
		{
		  Elf_Internal_Rela rel;
		  bfd_elf32_swap_reloc_in (output_bfd, p, &rel);
		  rel.r_info = ELF32_R_INFO (htab->elf.hgot->indx, R_386_32);
		  bfd_elf32_swap_reloc_out (output_bfd, &rel, p);
		  p += sizeof (Elf32_External_Rel);

		  bfd_elf32_swap_reloc_in (output_bfd, p, &rel);
		  rel.r_info = ELF32_R_INFO (htab->elf.hplt->indx, R_386_32);
		  bfd_elf32_swap_reloc_out (output_bfd, &rel, p);
		  p += sizeof (Elf32_External_Rel);
		}
	    }
	}
    }

  if (htab->elf.sgotplt)
    {
      if (bfd_is_abs_section (htab->elf.sgotplt->output_section))
	{
	  _bfd_error_handler
	    (_("discarded output section: `%A'"), htab->elf.sgotplt);
	  return FALSE;
	}

      /* Fill in the first three entries in the global offset table.  */
      if (htab->elf.sgotplt->size > 0)
	{
	  bfd_put_32 (output_bfd,
		      (sdyn == NULL ? 0
		       : sdyn->output_section->vma + sdyn->output_offset),
		      htab->elf.sgotplt->contents);
	  bfd_put_32 (output_bfd, 0, htab->elf.sgotplt->contents + 4);
	  bfd_put_32 (output_bfd, 0, htab->elf.sgotplt->contents + 8);
	}

      elf_section_data (htab->elf.sgotplt->output_section)->this_hdr.sh_entsize = 4;
    }

  /* Adjust .eh_frame for .plt section.  */
  if (htab->plt_eh_frame != NULL
      && htab->plt_eh_frame->contents != NULL)
    {
      if (htab->elf.splt != NULL
	  && htab->elf.splt->size != 0
	  && (htab->elf.splt->flags & SEC_EXCLUDE) == 0
	  && htab->elf.splt->output_section != NULL
	  && htab->plt_eh_frame->output_section != NULL)
	{
	  bfd_vma plt_start = htab->elf.splt->output_section->vma;
	  bfd_vma eh_frame_start = htab->plt_eh_frame->output_section->vma
				   + htab->plt_eh_frame->output_offset
				   + PLT_FDE_START_OFFSET;
	  bfd_put_signed_32 (dynobj, plt_start - eh_frame_start,
			     htab->plt_eh_frame->contents
			     + PLT_FDE_START_OFFSET);
	}
      if (htab->plt_eh_frame->sec_info_type
	  == SEC_INFO_TYPE_EH_FRAME)
	{
	  if (! _bfd_elf_write_section_eh_frame (output_bfd, info,
						 htab->plt_eh_frame,
						 htab->plt_eh_frame->contents))
	    return FALSE;
	}
    }

  /* Adjust .eh_frame for .plt.got section.  */
  if (htab->plt_got_eh_frame != NULL
      && htab->plt_got_eh_frame->contents != NULL)
    {
      if (htab->plt_got != NULL
	  && htab->plt_got->size != 0
	  && (htab->plt_got->flags & SEC_EXCLUDE) == 0
	  && htab->plt_got->output_section != NULL
	  && htab->plt_got_eh_frame->output_section != NULL)
	{
	  bfd_vma plt_start = htab->plt_got->output_section->vma;
	  bfd_vma eh_frame_start = htab->plt_got_eh_frame->output_section->vma
				   + htab->plt_got_eh_frame->output_offset
				   + PLT_FDE_START_OFFSET;
	  bfd_put_signed_32 (dynobj, plt_start - eh_frame_start,
			     htab->plt_got_eh_frame->contents
			     + PLT_FDE_START_OFFSET);
	}
      if (htab->plt_got_eh_frame->sec_info_type == SEC_INFO_TYPE_EH_FRAME)
	{
	  if (! _bfd_elf_write_section_eh_frame (output_bfd, info,
						 htab->plt_got_eh_frame,
						 htab->plt_got_eh_frame->contents))
	    return FALSE;
	}
    }

  if (htab->elf.sgot && htab->elf.sgot->size > 0)
    elf_section_data (htab->elf.sgot->output_section)->this_hdr.sh_entsize = 4;

  /* Fill PLT entries for undefined weak symbols in PIE.  */
  if (bfd_link_pie (info))
    bfd_hash_traverse (&info->hash->table,
		       elf_i386_pie_finish_undefweak_symbol,
		       info);

  return TRUE;
}

/* Fill PLT/GOT entries and allocate dynamic relocations for local
   STT_GNU_IFUNC symbols, which aren't in the ELF linker hash table.
   It has to be done before elf_link_sort_relocs is called so that
   dynamic relocations are properly sorted.  */

static bfd_boolean
elf_i386_output_arch_local_syms
  (bfd *output_bfd ATTRIBUTE_UNUSED,
   struct bfd_link_info *info,
   void *flaginfo ATTRIBUTE_UNUSED,
   int (*func) (void *, const char *,
		Elf_Internal_Sym *,
		asection *,
		struct elf_link_hash_entry *) ATTRIBUTE_UNUSED)
{
  struct elf_i386_link_hash_table *htab = elf_i386_hash_table (info);
  if (htab == NULL)
    return FALSE;

  /* Fill PLT and GOT entries for local STT_GNU_IFUNC symbols.  */
  htab_traverse (htab->loc_hash_table,
		 elf_i386_finish_local_dynamic_symbol,
		 info);

  return TRUE;
}

/* Return an array of PLT entry symbol values.  */

static bfd_vma *
elf_i386_get_plt_sym_val (bfd *abfd, asymbol **dynsyms, asection *plt,
			  asection *relplt)
{
  bfd_boolean (*slurp_relocs) (bfd *, asection *, asymbol **, bfd_boolean);
  arelent *p;
  long count, i;
  bfd_vma *plt_sym_val;
  bfd_vma plt_offset;
  bfd_byte *plt_contents;
  const struct elf_i386_backend_data *bed
    = get_elf_i386_backend_data (abfd);
  Elf_Internal_Shdr *hdr;

  /* Get the .plt section contents.  */
  plt_contents = (bfd_byte *) bfd_malloc (plt->size);
  if (plt_contents == NULL)
    return NULL;
  if (!bfd_get_section_contents (abfd, (asection *) plt,
				 plt_contents, 0, plt->size))
    {
bad_return:
      free (plt_contents);
      return NULL;
    }

  slurp_relocs = get_elf_backend_data (abfd)->s->slurp_reloc_table;
  if (! (*slurp_relocs) (abfd, relplt, dynsyms, TRUE))
    goto bad_return;

  hdr = &elf_section_data (relplt)->this_hdr;
  count = relplt->size / hdr->sh_entsize;

  plt_sym_val = (bfd_vma *) bfd_malloc (sizeof (bfd_vma) * count);
  if (plt_sym_val == NULL)
    goto bad_return;

  for (i = 0; i < count; i++)
    plt_sym_val[i] = -1;

  plt_offset = bed->plt->plt_entry_size;
  p = relplt->relocation;
  for (i = 0; i < count; i++, p++)
    {
      long reloc_index;

      /* Skip unknown relocation.  PR 17512: file: bc9d6cf5.  */
      if (p->howto == NULL)
	continue;

      if (p->howto->type != R_386_JUMP_SLOT
	  && p->howto->type != R_386_IRELATIVE)
	continue;

      reloc_index = H_GET_32 (abfd, (plt_contents + plt_offset
				     + bed->plt->plt_reloc_offset));
      reloc_index /= sizeof (Elf32_External_Rel);
      if (reloc_index < count)
	plt_sym_val[reloc_index] = plt->vma + plt_offset;

      plt_offset += bed->plt->plt_entry_size;

      /* PR binutils/18437: Skip extra relocations in the .rel.plt
	 section.  */
      if (plt_offset >= plt->size)
	break;
    }

  free (plt_contents);

  return plt_sym_val;
}

/* Similar to _bfd_elf_get_synthetic_symtab.  */

static long
elf_i386_get_synthetic_symtab (bfd *abfd,
			       long symcount,
			       asymbol **syms,
			       long dynsymcount,
			       asymbol **dynsyms,
			       asymbol **ret)
{
  asection *plt = bfd_get_section_by_name (abfd, ".plt");
  return _bfd_elf_ifunc_get_synthetic_symtab (abfd, symcount, syms,
					      dynsymcount, dynsyms, ret,
					      plt,
					      elf_i386_get_plt_sym_val);
}

/* Return TRUE if symbol should be hashed in the `.gnu.hash' section.  */

static bfd_boolean
elf_i386_hash_symbol (struct elf_link_hash_entry *h)
{
  if (h->plt.offset != (bfd_vma) -1
      && !h->def_regular
      && !h->pointer_equality_needed)
    return FALSE;

  return _bfd_elf_hash_symbol (h);
}

/* Parse i386 GNU properties.  */

static enum elf_property_kind
elf_i386_parse_gnu_properties (bfd *abfd, unsigned int type,
			       bfd_byte *ptr, unsigned int datasz)
{
  elf_property *prop;

  switch (type)
    {
    case GNU_PROPERTY_X86_ISA_1_USED:
    case GNU_PROPERTY_X86_ISA_1_NEEDED:
      if (datasz != 4)
	{
	  _bfd_error_handler
	    ((type == GNU_PROPERTY_X86_ISA_1_USED
	      ? _("error: %B: <corrupt x86 ISA used size: 0x%x>")
	      : _("error: %B: <corrupt x86 ISA needed size: 0x%x>")),
	     abfd, datasz);
	  return property_corrupt;
	}
      prop = _bfd_elf_get_property (abfd, type, datasz);
      prop->u.number = bfd_h_get_32 (abfd, ptr);
      prop->pr_kind = property_number;
      break;

    default:
      return property_ignored;
    }

  return property_number;
}

/* Merge i386 GNU property BPROP with APROP.  If APROP isn't NULL,
   return TRUE if APROP is updated.  Otherwise, return TRUE if BPROP
   should be merged with ABFD.  */

static bfd_boolean
elf_i386_merge_gnu_properties (bfd *abfd ATTRIBUTE_UNUSED,
			       elf_property *aprop,
			       elf_property *bprop)
{
  unsigned int number;
  bfd_boolean updated = FALSE;
  unsigned int pr_type = aprop != NULL ? aprop->pr_type : bprop->pr_type;

  switch (pr_type)
    {
    case GNU_PROPERTY_X86_ISA_1_USED:
    case GNU_PROPERTY_X86_ISA_1_NEEDED:
      if (aprop != NULL && bprop != NULL)
	{
	  number = aprop->u.number;
	  aprop->u.number = number | bprop->u.number;
	  updated = number != (unsigned int) aprop->u.number;
	}
      else
	{
	  /* Return TRUE if APROP is NULL to indicate that BPROP should
	     be added to ABFD.  */
	  updated = aprop == NULL;
	}
      break;

    default:
      /* Never should happen.  */
      abort ();
    }

  return updated;
}

#define TARGET_LITTLE_SYM		i386_elf32_vec
#define TARGET_LITTLE_NAME		"elf32-i386"
#define ELF_ARCH			bfd_arch_i386
#define ELF_TARGET_ID			I386_ELF_DATA
#define ELF_MACHINE_CODE		EM_386
#define ELF_MAXPAGESIZE			0x1000

#define elf_backend_can_gc_sections	1
#define elf_backend_can_refcount	1
#define elf_backend_want_got_plt	1
#define elf_backend_plt_readonly	1
#define elf_backend_want_plt_sym	0
#define elf_backend_got_header_size	12
#define elf_backend_plt_alignment	4
#define elf_backend_dtrel_excludes_plt	1
#define elf_backend_extern_protected_data 1
#define elf_backend_caches_rawsize	1
#define elf_backend_want_dynrelro	1

/* Support RELA for objdump of prelink objects.  */
#define elf_info_to_howto		      elf_i386_info_to_howto_rel
#define elf_info_to_howto_rel		      elf_i386_info_to_howto_rel

#define bfd_elf32_mkobject		      elf_i386_mkobject

#define bfd_elf32_bfd_is_local_label_name     elf_i386_is_local_label_name
#define bfd_elf32_bfd_link_hash_table_create  elf_i386_link_hash_table_create
#define bfd_elf32_bfd_reloc_type_lookup	      elf_i386_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup	      elf_i386_reloc_name_lookup
#define bfd_elf32_get_synthetic_symtab	      elf_i386_get_synthetic_symtab

#define elf_backend_adjust_dynamic_symbol     elf_i386_adjust_dynamic_symbol
#define elf_backend_relocs_compatible	      _bfd_elf_relocs_compatible
#define elf_backend_check_relocs	      elf_i386_check_relocs
#define elf_backend_copy_indirect_symbol      elf_i386_copy_indirect_symbol
#define elf_backend_create_dynamic_sections   elf_i386_create_dynamic_sections
#define elf_backend_fake_sections	      elf_i386_fake_sections
#define elf_backend_finish_dynamic_sections   elf_i386_finish_dynamic_sections
#define elf_backend_finish_dynamic_symbol     elf_i386_finish_dynamic_symbol
#define elf_backend_output_arch_local_syms     elf_i386_output_arch_local_syms
#define elf_backend_gc_mark_hook	      elf_i386_gc_mark_hook
#define elf_backend_grok_prstatus	      elf_i386_grok_prstatus
#define elf_backend_grok_psinfo		      elf_i386_grok_psinfo
#define elf_backend_reloc_type_class	      elf_i386_reloc_type_class
#define elf_backend_relocate_section	      elf_i386_relocate_section
#define elf_backend_size_dynamic_sections     elf_i386_size_dynamic_sections
#define elf_backend_always_size_sections      elf_i386_always_size_sections
#define elf_backend_omit_section_dynsym \
  ((bfd_boolean (*) (bfd *, struct bfd_link_info *, asection *)) bfd_true)
#define elf_backend_hash_symbol		      elf_i386_hash_symbol
#define elf_backend_fixup_symbol	      elf_i386_fixup_symbol
#define elf_backend_parse_gnu_properties      elf_i386_parse_gnu_properties
#define elf_backend_merge_gnu_properties      elf_i386_merge_gnu_properties

#include "elf32-target.h"

/* FreeBSD support.  */

#undef	TARGET_LITTLE_SYM
#define	TARGET_LITTLE_SYM		i386_elf32_fbsd_vec
#undef	TARGET_LITTLE_NAME
#define	TARGET_LITTLE_NAME		"elf32-i386-freebsd"
#undef	ELF_OSABI
#define	ELF_OSABI			ELFOSABI_FREEBSD

/* The kernel recognizes executables as valid only if they carry a
   "FreeBSD" label in the ELF header.  So we put this label on all
   executables and (for simplicity) also all other object files.  */

static void
elf_i386_fbsd_post_process_headers (bfd *abfd, struct bfd_link_info *info)
{
  _bfd_elf_post_process_headers (abfd, info);

#ifdef OLD_FREEBSD_ABI_LABEL
  {
    /* The ABI label supported by FreeBSD <= 4.0 is quite nonstandard.  */
    Elf_Internal_Ehdr *i_ehdrp = elf_elfheader (abfd);
    memcpy (&i_ehdrp->e_ident[EI_ABIVERSION], "FreeBSD", 8);
  }
#endif
}

#undef	elf_backend_post_process_headers
#define	elf_backend_post_process_headers	elf_i386_fbsd_post_process_headers
#undef	elf32_bed
#define	elf32_bed				elf32_i386_fbsd_bed

#undef elf_backend_add_symbol_hook

#include "elf32-target.h"

/* Solaris 2.  */

#undef	TARGET_LITTLE_SYM
#define	TARGET_LITTLE_SYM		i386_elf32_sol2_vec
#undef	TARGET_LITTLE_NAME
#define	TARGET_LITTLE_NAME		"elf32-i386-sol2"

#undef elf_backend_post_process_headers

/* Restore default: we cannot use ELFOSABI_SOLARIS, otherwise ELFOSABI_NONE
   objects won't be recognized.  */
#undef ELF_OSABI

#undef	elf32_bed
#define	elf32_bed			elf32_i386_sol2_bed

/* The 32-bit static TLS arena size is rounded to the nearest 8-byte
   boundary.  */
#undef  elf_backend_static_tls_alignment
#define elf_backend_static_tls_alignment 8

/* The Solaris 2 ABI requires a plt symbol on all platforms.

   Cf. Linker and Libraries Guide, Ch. 2, Link-Editor, Generating the Output
   File, p.63.  */
#undef  elf_backend_want_plt_sym
#define elf_backend_want_plt_sym	1

#undef  elf_backend_strtab_flags
#define elf_backend_strtab_flags	SHF_STRINGS

/* Called to set the sh_flags, sh_link and sh_info fields of OSECTION which
   has a type >= SHT_LOOS.  Returns TRUE if these fields were initialised 
   FALSE otherwise.  ISECTION is the best guess matching section from the
   input bfd IBFD, but it might be NULL.  */

static bfd_boolean
elf32_i386_copy_solaris_special_section_fields (const bfd *ibfd ATTRIBUTE_UNUSED,
						bfd *obfd ATTRIBUTE_UNUSED,
						const Elf_Internal_Shdr *isection ATTRIBUTE_UNUSED,
						Elf_Internal_Shdr *osection ATTRIBUTE_UNUSED)
{
  /* PR 19938: FIXME: Need to add code for setting the sh_info
     and sh_link fields of Solaris specific section types.  */
  return FALSE;

  /* Based upon Oracle Solaris 11.3 Linkers and Libraries Guide, Ch. 13,
     Object File Format, Table 13-9  ELF sh_link and sh_info Interpretation:

http://docs.oracle.com/cd/E53394_01/html/E54813/chapter6-94076.html#scrolltoc

     The following values should be set:
     
Type                 Link                           Info
-----------------------------------------------------------------------------
SHT_SUNW_ancillary   The section header index of    0
 [0x6fffffee]        the associated string table.
	
SHT_SUNW_capinfo     The section header index of    For a dynamic object, the
 [0x6ffffff0]        the associated symbol table.   section header index of
                                                    the associated
						    SHT_SUNW_capchain table,
						    otherwise 0.

SHT_SUNW_symsort     The section header index of    0
 [0x6ffffff1]        the associated symbol table.

SHT_SUNW_tlssort     The section header index of    0
 [0x6ffffff2]        the associated symbol table.
	
SHT_SUNW_LDYNSYM     The section header index of    One greater than the 
 [0x6ffffff3]        the associated string table.   symbol table index of the
		     This index is the same string  last local symbol, 
		     table used by the SHT_DYNSYM   STB_LOCAL. Since
		     section.                       SHT_SUNW_LDYNSYM only
		                                    contains local symbols,
						    sh_info is equivalent to
						    the number of symbols in
						    the table.

SHT_SUNW_cap         If symbol capabilities exist,  If any capabilities refer
 [0x6ffffff5]        the section header index of    to named strings, the
                     the associated                 section header index of
		     SHT_SUNW_capinfo table,        the associated string 
			  otherwise 0.              table, otherwise 0.

SHT_SUNW_move        The section header index of    0
 [0x6ffffffa]        the associated symbol table.
	
SHT_SUNW_COMDAT      0                              0
 [0x6ffffffb]

SHT_SUNW_syminfo     The section header index of    The section header index
 [0x6ffffffc]        the associated symbol table.   of the associated
		                                    .dynamic section.

SHT_SUNW_verdef      The section header index of    The number of version 
 [0x6ffffffd]        the associated string table.   definitions within the
		                                    section.

SHT_SUNW_verneed     The section header index of    The number of version
 [0x6ffffffe]        the associated string table.   dependencies within the
                                                    section.

SHT_SUNW_versym      The section header index of    0
 [0x6fffffff]        the associated symbol table.  */
}

#undef  elf_backend_copy_special_section_fields
#define elf_backend_copy_special_section_fields elf32_i386_copy_solaris_special_section_fields

#include "elf32-target.h"

/* Intel MCU support.  */

static bfd_boolean
elf32_iamcu_elf_object_p (bfd *abfd)
{
  /* Set the right machine number for an IAMCU elf32 file.  */
  bfd_default_set_arch_mach (abfd, bfd_arch_iamcu, bfd_mach_i386_iamcu);
  return TRUE;
}

#undef  TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM		iamcu_elf32_vec
#undef  TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME		"elf32-iamcu"
#undef  ELF_ARCH
#define ELF_ARCH			bfd_arch_iamcu

#undef	ELF_MACHINE_CODE
#define	ELF_MACHINE_CODE		EM_IAMCU

#undef	ELF_OSABI

#undef  elf32_bed
#define elf32_bed			elf32_iamcu_bed

#undef	elf_backend_object_p
#define elf_backend_object_p		elf32_iamcu_elf_object_p

#undef	elf_backend_static_tls_alignment

#undef	elf_backend_want_plt_sym
#define elf_backend_want_plt_sym	    0

#undef  elf_backend_strtab_flags
#undef  elf_backend_copy_special_section_fields

#include "elf32-target.h"

/* Restore defaults.  */
#undef	ELF_ARCH
#define ELF_ARCH			bfd_arch_i386
#undef	ELF_MACHINE_CODE
#define ELF_MACHINE_CODE		EM_386

/* Native Client support.  */

#undef	TARGET_LITTLE_SYM
#define	TARGET_LITTLE_SYM		i386_elf32_nacl_vec
#undef	TARGET_LITTLE_NAME
#define	TARGET_LITTLE_NAME		"elf32-i386-nacl"
#undef	elf32_bed
#define	elf32_bed			elf32_i386_nacl_bed

#undef	ELF_MAXPAGESIZE
#define	ELF_MAXPAGESIZE			0x10000

/* Restore defaults.  */
#undef	ELF_OSABI
#undef	elf_backend_want_plt_sym
#define elf_backend_want_plt_sym	0
#undef	elf_backend_post_process_headers
#undef	elf_backend_static_tls_alignment

/* NaCl uses substantially different PLT entries for the same effects.  */

#undef	elf_backend_plt_alignment
#define elf_backend_plt_alignment	5
#define NACL_PLT_ENTRY_SIZE		64
#define	NACLMASK			0xe0 /* 32-byte alignment mask.  */

static const bfd_byte elf_i386_nacl_plt0_entry[] =
  {
    0xff, 0x35,			  /* pushl contents of address */
    0, 0, 0, 0,			  /* replaced with address of .got + 4.	 */
    0x8b, 0x0d,                   /* movl contents of address, %ecx */
    0, 0, 0, 0,			  /* replaced with address of .got + 8.	 */
    0x83, 0xe1, NACLMASK,	  /* andl $NACLMASK, %ecx */
    0xff, 0xe1			  /* jmp *%ecx */
  };

static const bfd_byte elf_i386_nacl_plt_entry[NACL_PLT_ENTRY_SIZE] =
  {
    0x8b, 0x0d,				/* movl contents of address, %ecx */
    0, 0, 0, 0,				/* replaced with GOT slot address.  */
    0x83, 0xe1, NACLMASK,		/* andl $NACLMASK, %ecx */
    0xff, 0xe1,				/* jmp *%ecx */

    /* Pad to the next 32-byte boundary with nop instructions.	*/
    0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,

    /* Lazy GOT entries point here (32-byte aligned).  */
    0x68,			       /* pushl immediate */
    0, 0, 0, 0,			       /* replaced with reloc offset.  */
    0xe9,			       /* jmp relative */
    0, 0, 0, 0,			       /* replaced with offset to .plt.	 */

    /* Pad to the next 32-byte boundary with nop instructions.	*/
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90
  };

static const bfd_byte
elf_i386_nacl_pic_plt0_entry[sizeof (elf_i386_nacl_plt0_entry)] =
  {
    0xff, 0x73, 0x04,		/* pushl 4(%ebx) */
    0x8b, 0x4b, 0x08,		/* mov 0x8(%ebx), %ecx */
    0x83, 0xe1, 0xe0,		/* and $NACLMASK, %ecx */
    0xff, 0xe1,			/* jmp *%ecx */

    /* This is expected to be the same size as elf_i386_nacl_plt0_entry,
       so pad to that size with nop instructions.  */
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90
  };

static const bfd_byte elf_i386_nacl_pic_plt_entry[NACL_PLT_ENTRY_SIZE] =
  {
    0x8b, 0x8b,          /* movl offset(%ebx), %ecx */
    0, 0, 0, 0,          /* replaced with offset of this symbol in .got.  */
    0x83, 0xe1, 0xe0,    /* andl $NACLMASK, %ecx */
    0xff, 0xe1,          /* jmp *%ecx */

    /* Pad to the next 32-byte boundary with nop instructions.	*/
    0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,

    /* Lazy GOT entries point here (32-byte aligned).  */
    0x68,                /* pushl immediate */
    0, 0, 0, 0,          /* replaced with offset into relocation table.  */
    0xe9,                /* jmp relative */
    0, 0, 0, 0,          /* replaced with offset to start of .plt.  */

    /* Pad to the next 32-byte boundary with nop instructions.	*/
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90
  };

static const bfd_byte elf_i386_nacl_eh_frame_plt[] =
  {
#if (PLT_CIE_LENGTH != 20                               \
     || PLT_FDE_LENGTH != 36                            \
     || PLT_FDE_START_OFFSET != 4 + PLT_CIE_LENGTH + 8  \
     || PLT_FDE_LEN_OFFSET != 4 + PLT_CIE_LENGTH + 12)
# error "Need elf_i386_backend_data parameters for eh_frame_plt offsets!"
#endif
    PLT_CIE_LENGTH, 0, 0, 0,		/* CIE length */
    0, 0, 0, 0,                         /* CIE ID */
    1,                                  /* CIE version */
    'z', 'R', 0,                        /* Augmentation string */
    1,                                  /* Code alignment factor */
    0x7c,                               /* Data alignment factor: -4 */
    8,                                  /* Return address column */
    1,					/* Augmentation size */
    DW_EH_PE_pcrel | DW_EH_PE_sdata4,	/* FDE encoding */
    DW_CFA_def_cfa, 4, 4,		/* DW_CFA_def_cfa: r4 (esp) ofs 4 */
    DW_CFA_offset + 8, 1,		/* DW_CFA_offset: r8 (eip) at cfa-4 */
    DW_CFA_nop, DW_CFA_nop,

    PLT_FDE_LENGTH, 0, 0, 0,     /* FDE length */
    PLT_CIE_LENGTH + 8, 0, 0, 0, /* CIE pointer */
    0, 0, 0, 0,                  /* R_386_PC32 .plt goes here */
    0, 0, 0, 0,                  /* .plt size goes here */
    0,                           /* Augmentation size */
    DW_CFA_def_cfa_offset, 8,    /* DW_CFA_def_cfa_offset: 8 */
    DW_CFA_advance_loc + 6,      /* DW_CFA_advance_loc: 6 to __PLT__+6 */
    DW_CFA_def_cfa_offset, 12,   /* DW_CFA_def_cfa_offset: 12 */
    DW_CFA_advance_loc + 58,     /* DW_CFA_advance_loc: 58 to __PLT__+64 */
    DW_CFA_def_cfa_expression,   /* DW_CFA_def_cfa_expression */
    13,                          /* Block length */
    DW_OP_breg4, 4,              /* DW_OP_breg4 (esp): 4 */
    DW_OP_breg8, 0,              /* DW_OP_breg8 (eip): 0 */
    DW_OP_const1u, 63, DW_OP_and, DW_OP_const1u, 37, DW_OP_ge,
    DW_OP_lit2, DW_OP_shl, DW_OP_plus,
    DW_CFA_nop, DW_CFA_nop
  };

static const struct elf_i386_plt_layout elf_i386_nacl_plt =
  {
    elf_i386_nacl_plt0_entry,		/* plt0_entry */
    sizeof (elf_i386_nacl_plt0_entry),	/* plt0_entry_size */
    2,					/* plt0_got1_offset */
    8,					/* plt0_got2_offset */
    elf_i386_nacl_plt_entry,		/* plt_entry */
    NACL_PLT_ENTRY_SIZE,		/* plt_entry_size */
    2,					/* plt_got_offset */
    33,					/* plt_reloc_offset */
    38,					/* plt_plt_offset */
    32,					/* plt_lazy_offset */
    elf_i386_nacl_pic_plt0_entry,	/* pic_plt0_entry */
    elf_i386_nacl_pic_plt_entry,	/* pic_plt_entry */
    elf_i386_nacl_eh_frame_plt,		/* eh_frame_plt */
    sizeof (elf_i386_nacl_eh_frame_plt),/* eh_frame_plt_size */
    NULL,				/* eh_frame_plt_got */
    0,					/* eh_frame_plt_got_size */
  };

static const struct elf_i386_backend_data elf_i386_nacl_arch_bed =
  {
    &elf_i386_nacl_plt,                      /* plt */
    0x90,				/* plt0_pad_byte: nop insn */
    0,                                  /* is_vxworks */
  };

static bfd_boolean
elf32_i386_nacl_elf_object_p (bfd *abfd)
{
  /* Set the right machine number for a NaCl i386 ELF32 file.  */
  bfd_default_set_arch_mach (abfd, bfd_arch_i386, bfd_mach_i386_i386_nacl);
  return TRUE;
}

#undef	elf_backend_arch_data
#define elf_backend_arch_data	&elf_i386_nacl_arch_bed

#undef	elf_backend_object_p
#define elf_backend_object_p			elf32_i386_nacl_elf_object_p
#undef	elf_backend_modify_segment_map
#define	elf_backend_modify_segment_map		nacl_modify_segment_map
#undef	elf_backend_modify_program_headers
#define	elf_backend_modify_program_headers	nacl_modify_program_headers
#undef	elf_backend_final_write_processing
#define elf_backend_final_write_processing	nacl_final_write_processing

#include "elf32-target.h"

/* Restore defaults.  */
#undef	elf_backend_object_p
#undef	elf_backend_modify_segment_map
#undef	elf_backend_modify_program_headers
#undef	elf_backend_final_write_processing

/* VxWorks support.  */

#undef	TARGET_LITTLE_SYM
#define TARGET_LITTLE_SYM		i386_elf32_vxworks_vec
#undef	TARGET_LITTLE_NAME
#define TARGET_LITTLE_NAME		"elf32-i386-vxworks"
#undef	ELF_OSABI
#undef	elf_backend_plt_alignment
#define elf_backend_plt_alignment	4

static const struct elf_i386_backend_data elf_i386_vxworks_arch_bed =
  {
    &elf_i386_plt,                      /* plt */
    0x90,                               /* plt0_pad_byte */
    1,                                  /* is_vxworks */
  };

#undef	elf_backend_arch_data
#define	elf_backend_arch_data	&elf_i386_vxworks_arch_bed

#undef elf_backend_relocs_compatible
#undef elf_backend_add_symbol_hook
#define elf_backend_add_symbol_hook \
  elf_vxworks_add_symbol_hook
#undef elf_backend_link_output_symbol_hook
#define elf_backend_link_output_symbol_hook \
  elf_vxworks_link_output_symbol_hook
#undef elf_backend_emit_relocs
#define elf_backend_emit_relocs			elf_vxworks_emit_relocs
#undef elf_backend_final_write_processing
#define elf_backend_final_write_processing \
  elf_vxworks_final_write_processing
#undef elf_backend_static_tls_alignment

/* On VxWorks, we emit relocations against _PROCEDURE_LINKAGE_TABLE_, so
   define it.  */
#undef elf_backend_want_plt_sym
#define elf_backend_want_plt_sym	1

#undef	elf32_bed
#define elf32_bed				elf32_i386_vxworks_bed

#include "elf32-target.h"
