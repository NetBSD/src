/* Linux bpf specific support for 64-bit ELF
   Copyright (C) 2019-2020 Free Software Foundation, Inc.
   Contributed by Oracle Inc.

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
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/bpf.h"
#include "libiberty.h"

/* In case we're on a 32-bit machine, construct a 64-bit "-1" value.  */
#define MINUS_ONE (~ (bfd_vma) 0)

#define BASEADDR(SEC)	((SEC)->output_section->vma + (SEC)->output_offset)

/* Relocation tables.  */
static reloc_howto_type bpf_elf_howto_table [] =
{
  /* This reloc does nothing.  */
  HOWTO (R_BPF_NONE,		/* type */
	 0,			/* rightshift */
	 3,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64-immediate in LDDW instruction.  */
  HOWTO (R_BPF_INSN_64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_INSN_64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 32-immediate in LDDW instruction.  */
  HOWTO (R_BPF_INSN_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_INSN_32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 16-bit offsets in instructions.  */
  HOWTO (R_BPF_INSN_16,		/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_INSN_16",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0x0000ffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 16-bit PC-relative address in jump instructions.  */
  HOWTO (R_BPF_INSN_DISP16,	/* type */
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 32,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_INSN_DISP16",   /* name */
	 FALSE,			/* partial_inplace */
	 0xffff,		/* src_mask */
	 0xffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_BPF_DATA_8_PCREL,
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_8_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_BPF_DATA_16_PCREL,
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_16_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_BPF_DATA_32_PCREL,
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_32_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_BPF_DATA_8,
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 8,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_DATA_8",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xff,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_BPF_DATA_16,
	 0,			/* rightshift */
	 1,			/* size (0 = byte, 1 = short, 2 = long) */
	 16,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_unsigned, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_DATA_16",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32-bit PC-relative address in call instructions.  */
  HOWTO (R_BPF_INSN_DISP32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_INSN_DISP32",   /* name */
	 FALSE,			/* partial_inplace */
	 0xffffffff,		/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 32-bit data.  */
  HOWTO (R_BPF_DATA_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_DATA_32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 64-bit data.  */
  HOWTO (R_BPF_DATA_64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_DATA_64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_BPF_DATA_64_PCREL,
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_BPF_64_PCREL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 TRUE),			/* pcrel_offset */
};
#undef AHOW

/* Map BFD reloc types to bpf ELF reloc types.  */

static reloc_howto_type *
bpf_reloc_type_lookup (bfd * abfd ATTRIBUTE_UNUSED,
                        bfd_reloc_code_real_type code)
{
  /* Note that the bpf_elf_howto_table is indexed by the R_ constants.
     Thus, the order that the howto records appear in the table *must*
     match the order of the relocation types defined in
     include/elf/bpf.h.  */

  switch (code)
    {
    case BFD_RELOC_NONE:
      return &bpf_elf_howto_table[ (int) R_BPF_NONE];

    case BFD_RELOC_8_PCREL:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_8_PCREL];
    case BFD_RELOC_16_PCREL:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_16_PCREL];
    case BFD_RELOC_32_PCREL:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_32_PCREL];
    case BFD_RELOC_64_PCREL:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_64_PCREL];

    case BFD_RELOC_8:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_8];
    case BFD_RELOC_16:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_16];
    case BFD_RELOC_32:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_32];
    case BFD_RELOC_64:
      return &bpf_elf_howto_table[ (int) R_BPF_DATA_64];

    case BFD_RELOC_BPF_64:
      return &bpf_elf_howto_table[ (int) R_BPF_INSN_64];
    case BFD_RELOC_BPF_32:
      return &bpf_elf_howto_table[ (int) R_BPF_INSN_32];
    case BFD_RELOC_BPF_16:
      return &bpf_elf_howto_table[ (int) R_BPF_INSN_16];
    case BFD_RELOC_BPF_DISP16:
      return &bpf_elf_howto_table[ (int) R_BPF_INSN_DISP16];
    case BFD_RELOC_BPF_DISP32:
      return &bpf_elf_howto_table[ (int) R_BPF_INSN_DISP32];

    default:
      /* Pacify gcc -Wall.  */
      return NULL;
    }
  return NULL;
}

/* Map BFD reloc names to bpf ELF reloc names.  */

static reloc_howto_type *
bpf_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (bpf_elf_howto_table); i++)
    if (bpf_elf_howto_table[i].name != NULL
	&& strcasecmp (bpf_elf_howto_table[i].name, r_name) == 0)
      return &bpf_elf_howto_table[i];

  return NULL;
}

/* Set the howto pointer for a bpf reloc.  */

static bfd_boolean
bpf_info_to_howto (bfd *abfd, arelent *bfd_reloc,
                    Elf_Internal_Rela *elf_reloc)
{
  unsigned int r_type;

  r_type = ELF64_R_TYPE (elf_reloc->r_info);
  if (r_type >= (unsigned int) R_BPF_max)
    {
      /* xgettext:c-format */
      _bfd_error_handler (_("%pB: unsupported relocation type %#x"),
                          abfd, r_type);
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  bfd_reloc->howto = &bpf_elf_howto_table [r_type];
  return TRUE;
}

/* Relocate an eBPF ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures; if the section
   actually uses Rel structures, the r_addend field will always be
   zero.

   This function is responsible for adjusting the section contents as
   necessary, and (if using Rela relocs and generating a relocatable
   output file) adjusting the reloc addend as necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocatable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

#define sec_addr(sec) ((sec)->output_section->vma + (sec)->output_offset)

static bfd_boolean
bpf_elf_relocate_section (bfd *output_bfd ATTRIBUTE_UNUSED,
                          struct bfd_link_info *info,
                          bfd *input_bfd,
                          asection *input_section,
                          bfd_byte *contents,
                          Elf_Internal_Rela *relocs,
                          Elf_Internal_Sym *local_syms,
                          asection **local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;

  symtab_hdr = & elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend     = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel ++)
    {
      reloc_howto_type *	   howto;
      unsigned long		   r_symndx;
      Elf_Internal_Sym *	   sym;
      asection *		   sec;
      struct elf_link_hash_entry * h;
      bfd_vma			   relocation;
      bfd_reloc_status_type	   r;
      const char *		   name = NULL;
      int			   r_type ATTRIBUTE_UNUSED;

      r_type = ELF64_R_TYPE (rel->r_info);
      r_symndx = ELF64_R_SYM (rel->r_info);
      howto  = bpf_elf_howto_table + ELF64_R_TYPE (rel->r_info);
      h      = NULL;
      sym    = NULL;
      sec    = NULL;

      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections [r_symndx];
	  relocation = BASEADDR (sec) + sym->st_value;

	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = name == NULL ? bfd_section_name (sec) : name;
	}
      else
	{
	  bfd_boolean warned ATTRIBUTE_UNUSED;
	  bfd_boolean unresolved_reloc ATTRIBUTE_UNUSED;
	  bfd_boolean ignored ATTRIBUTE_UNUSED;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned, ignored);

	  name = h->root.root.string;
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (bfd_link_relocatable (info))
	continue;

      switch (howto->type)
        {
        case R_BPF_INSN_DISP16:
        case R_BPF_INSN_DISP32:
          {
            bfd_signed_vma addend;
            
            /* Make the relocation PC-relative, and change its unit to
               64-bit words.  */
            relocation -= sec_addr (input_section) + rel->r_offset;
            /* Make it 64-bit words.  */
            relocation = relocation / 8;
            
            /* Get the addend from the instruction and apply it.  */
            addend = bfd_get (howto->bitsize, input_bfd,
                              contents + rel->r_offset
                              + (howto->bitsize == 16 ? 2 : 4));
                              
            if ((addend & (((~howto->src_mask) >> 1) & howto->src_mask)) != 0)
              addend -= (((~howto->src_mask) >> 1) & howto->src_mask) << 1;
            relocation += addend;

            /* Write out the relocated value.  */
            bfd_put (howto->bitsize, input_bfd, relocation,
                     contents + rel->r_offset
                     + (howto->bitsize == 16 ? 2 : 4));

            r = bfd_reloc_ok;
            break;
          }
        default:
          r = _bfd_final_link_relocate (howto, input_bfd, input_section,
                                        contents, rel->r_offset, relocation,
                                        rel->r_addend);
        }

      if (r != bfd_reloc_ok)
	{
	  const char * msg = NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      (*info->callbacks->reloc_overflow)
		(info, (h ? &h->root : NULL), name, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      break;

	    case bfd_reloc_undefined:
	      (*info->callbacks->undefined_symbol)
		(info, name, input_bfd, input_section, rel->r_offset, TRUE);
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	    case bfd_reloc_notsupported:
	      if (sym != NULL) /* Only if it's not an unresolved symbol.  */
                msg = _("internal error: relocation not supported");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    (*info->callbacks->warning) (info, msg, name, input_bfd,
					 input_section, rel->r_offset);
	}
    }

  return TRUE;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

static bfd_boolean
elf64_bpf_merge_private_bfd_data (bfd *ibfd, struct bfd_link_info *info)
{
  /* Check if we have the same endianness.  */
  if (! _bfd_generic_verify_endian_match (ibfd, info))
    return FALSE;

  return TRUE;
}

/* The macros below configure the architecture.  */

#define TARGET_LITTLE_SYM bpf_elf64_le_vec
#define TARGET_LITTLE_NAME "elf64-bpfle"

#define TARGET_BIG_SYM bpf_elf64_be_vec
#define TARGET_BIG_NAME "elf64-bpfbe"

#define ELF_ARCH bfd_arch_bpf
#define ELF_MACHINE_CODE EM_BPF

#define ELF_MAXPAGESIZE 0x100000

#define elf_info_to_howto_rel bpf_info_to_howto
#define elf_info_to_howto bpf_info_to_howto

#define elf_backend_may_use_rel_p		1
#define elf_backend_may_use_rela_p		0
#define elf_backend_default_use_rela_p		0
#define elf_backend_relocate_section		bpf_elf_relocate_section

#define elf_backend_can_gc_sections		0

#define elf_symbol_leading_char			'_'
#define bfd_elf64_bfd_reloc_type_lookup		bpf_reloc_type_lookup
#define bfd_elf64_bfd_reloc_name_lookup		bpf_reloc_name_lookup

#define bfd_elf64_bfd_merge_private_bfd_data elf64_bpf_merge_private_bfd_data

#include "elf64-target.h"
