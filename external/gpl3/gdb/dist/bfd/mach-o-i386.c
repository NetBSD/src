/* Intel i386 Mach-O support for BFD.
   Copyright 2009
   Free Software Foundation, Inc.

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
#include "mach-o.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"

#define bfd_mach_o_object_p bfd_mach_o_i386_object_p
#define bfd_mach_o_core_p bfd_mach_o_i386_core_p
#define bfd_mach_o_mkobject bfd_mach_o_i386_mkobject

static const bfd_target *
bfd_mach_o_i386_object_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0, BFD_MACH_O_CPU_TYPE_I386);
}

static const bfd_target *
bfd_mach_o_i386_core_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd,
                              BFD_MACH_O_MH_CORE, BFD_MACH_O_CPU_TYPE_I386);
}

static bfd_boolean
bfd_mach_o_i386_mkobject (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_mkobject_init (abfd))
    return FALSE;

  mdata = bfd_mach_o_get_data (abfd);
  mdata->header.magic = BFD_MACH_O_MH_MAGIC;
  mdata->header.cputype = BFD_MACH_O_CPU_TYPE_I386;
  mdata->header.cpusubtype = BFD_MACH_O_CPU_SUBTYPE_X86_ALL;
  mdata->header.byteorder = BFD_ENDIAN_LITTLE;
  mdata->header.version = 1;

  return TRUE;
}

static reloc_howto_type i386_howto_table[]=
{
  HOWTO(BFD_RELOC_32, 0, 2, 32, FALSE, 0,
	complain_overflow_bitfield,
	NULL, "32",
	FALSE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(BFD_RELOC_16, 0, 1, 16, FALSE, 0,
	complain_overflow_bitfield,
	NULL, "16",
	FALSE, 0xffff, 0xffff, FALSE),
  HOWTO(BFD_RELOC_8, 0, 0, 8, FALSE, 0,
	complain_overflow_bitfield,
	NULL, "8",
	FALSE, 0xff, 0xff, FALSE),
  HOWTO(BFD_RELOC_32_PCREL, 0, 2, 32, TRUE, 0,
	complain_overflow_bitfield,
	NULL, "DISP32",
	FALSE, 0xffffffff, 0xffffffff, TRUE),
  HOWTO(BFD_RELOC_16_PCREL, 0, 1, 16, TRUE, 0,
	complain_overflow_bitfield,
	NULL, "DISP16",
	FALSE, 0xffff, 0xffff, TRUE),
  HOWTO(BFD_RELOC_MACH_O_SECTDIFF, 0, 2, 32, FALSE, 0,
	complain_overflow_bitfield,
	NULL, "SECTDIFF_32",
	FALSE, 0xffffffff, 0xffffffff, FALSE),
  HOWTO(BFD_RELOC_MACH_O_PAIR, 0, 2, 32, FALSE, 0,
	complain_overflow_bitfield,
	NULL, "PAIR_32",
	FALSE, 0xffffffff, 0xffffffff, FALSE),
};

static bfd_boolean
bfd_mach_o_i386_swap_reloc_in (arelent *res, bfd_mach_o_reloc_info *reloc)
{
  if (reloc->r_scattered)
    {
      switch (reloc->r_type)
        {
        case BFD_MACH_O_GENERIC_RELOC_PAIR:
          if (reloc->r_length != 2)
            return FALSE;
          res->howto = &i386_howto_table[6];
          res->address = res[-1].address;
          return TRUE;
        case BFD_MACH_O_GENERIC_RELOC_SECTDIFF:
        case BFD_MACH_O_GENERIC_RELOC_LOCAL_SECTDIFF:
          if (reloc->r_length != 2)
            return FALSE;
          res->howto = &i386_howto_table[5];
          return TRUE;
        default:
          return FALSE;
        }
    }
  else
    {
      switch (reloc->r_type)
        {
        case BFD_MACH_O_GENERIC_RELOC_VANILLA:
          switch ((reloc->r_length << 1) | reloc->r_pcrel)
            {
            case 0: /* len = 0, pcrel = 0  */
              res->howto = &i386_howto_table[2];
              return TRUE;
            case 2: /* len = 1, pcrel = 0  */
              res->howto = &i386_howto_table[1];
              return TRUE;
            case 3: /* len = 1, pcrel = 1  */
              res->howto = &i386_howto_table[4];
              return TRUE;
            case 4: /* len = 2, pcrel = 0  */
              res->howto = &i386_howto_table[0];
              return TRUE;
            case 5: /* len = 2, pcrel = 1  */
              res->howto = &i386_howto_table[3];
              return TRUE;
            default:
              return FALSE;
            }
          break;
        default:
          return FALSE;
        }
    }
}

static bfd_boolean
bfd_mach_o_i386_swap_reloc_out (arelent *rel, bfd_mach_o_reloc_info *rinfo)
{
  rinfo->r_address = rel->address;
  switch (rel->howto->type)
    {
    case BFD_RELOC_32:
    case BFD_RELOC_32_PCREL:
    case BFD_RELOC_16:
    case BFD_RELOC_16_PCREL:
    case BFD_RELOC_8:
      rinfo->r_scattered = 0;
      rinfo->r_type = BFD_MACH_O_GENERIC_RELOC_VANILLA;
      rinfo->r_pcrel = rel->howto->pc_relative;
      rinfo->r_length = rel->howto->size; /* Correct in practice.  */
      if ((*rel->sym_ptr_ptr)->flags & BSF_SECTION_SYM)
        {
          rinfo->r_extern = 0;
          rinfo->r_value = (*rel->sym_ptr_ptr)->section->target_index;
        }
      else
        {
          rinfo->r_extern = 1;
          rinfo->r_value = (*rel->sym_ptr_ptr)->udata.i;
        }
      break;
    case BFD_RELOC_MACH_O_SECTDIFF:
      rinfo->r_scattered = 1;
      rinfo->r_type = BFD_MACH_O_GENERIC_RELOC_SECTDIFF;
      rinfo->r_pcrel = 0;
      rinfo->r_length = 2;
      rinfo->r_extern = 0;
      rinfo->r_value = (*rel->sym_ptr_ptr)->value 
        + (*rel->sym_ptr_ptr)->section->vma;
      break;
    case BFD_RELOC_MACH_O_PAIR:
      rinfo->r_address = 0;
      rinfo->r_scattered = 1;
      rinfo->r_type = BFD_MACH_O_GENERIC_RELOC_PAIR;
      rinfo->r_pcrel = 0;
      rinfo->r_length = 2;
      rinfo->r_extern = 0;
      rinfo->r_value = (*rel->sym_ptr_ptr)->value 
        + (*rel->sym_ptr_ptr)->section->vma;
      break;
    default:
      return FALSE;
    }
  return TRUE;
}

static reloc_howto_type *
bfd_mach_o_i386_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                                       bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < sizeof (i386_howto_table) / sizeof (*i386_howto_table); i++)
    if (code == i386_howto_table[i].type)
      return &i386_howto_table[i];
  return NULL;
}

static reloc_howto_type *
bfd_mach_o_i386_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
                                       const char *name ATTRIBUTE_UNUSED)
{
  return NULL;
}

static bfd_boolean
bfd_mach_o_i386_print_thread (bfd *abfd, bfd_mach_o_thread_flavour *thread,
                              void *vfile, char *buf)
{
  FILE *file = (FILE *)vfile;

  switch (thread->flavour)
    {
    case BFD_MACH_O_x86_THREAD_STATE:
      if (thread->size < (8 + 16 * 4))
        return FALSE;
      fprintf (file, "   x86_THREAD_STATE:\n");
      fprintf (file, "    flavor: 0x%08lx  count: 0x%08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 0),
               (unsigned long)bfd_get_32 (abfd, buf + 4));
      fprintf (file, "     eax: %08lx  ebx: %08lx  ecx: %08lx  edx: %08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 8),
               (unsigned long)bfd_get_32 (abfd, buf + 12),
               (unsigned long)bfd_get_32 (abfd, buf + 16),
               (unsigned long)bfd_get_32 (abfd, buf + 20));
      fprintf (file, "     edi: %08lx  esi: %08lx  ebp: %08lx  esp: %08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 24),
               (unsigned long)bfd_get_32 (abfd, buf + 28),
               (unsigned long)bfd_get_32 (abfd, buf + 32),
               (unsigned long)bfd_get_32 (abfd, buf + 36));
      fprintf (file, "      ss: %08lx  flg: %08lx  eip: %08lx   cs: %08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 40),
               (unsigned long)bfd_get_32 (abfd, buf + 44),
               (unsigned long)bfd_get_32 (abfd, buf + 48),
               (unsigned long)bfd_get_32 (abfd, buf + 52));
      fprintf (file, "      ds: %08lx   es: %08lx   fs: %08lx   gs: %08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 56),
               (unsigned long)bfd_get_32 (abfd, buf + 60),
               (unsigned long)bfd_get_32 (abfd, buf + 64),
               (unsigned long)bfd_get_32 (abfd, buf + 68));
      return TRUE;
    case BFD_MACH_O_x86_FLOAT_STATE:
      if (thread->size < 8)
        return FALSE;
      fprintf (file, "   x86_FLOAT_STATE:\n");
      fprintf (file, "    flavor: 0x%08lx  count: 0x%08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 0),
               (unsigned long)bfd_get_32 (abfd, buf + 4));
      return TRUE;
    case BFD_MACH_O_x86_EXCEPTION_STATE:
      if (thread->size < 8 + 3 * 4)
        return FALSE;
      fprintf (file, "   x86_EXCEPTION_STATE:\n");
      fprintf (file, "    flavor: 0x%08lx  count: 0x%08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 0),
               (unsigned long)bfd_get_32 (abfd, buf + 4));
      fprintf (file, "    trapno: %08lx  err: %08lx  faultaddr: %08lx\n",
               (unsigned long)bfd_get_32 (abfd, buf + 8),
               (unsigned long)bfd_get_32 (abfd, buf + 12),
               (unsigned long)bfd_get_32 (abfd, buf + 16));
      return TRUE;
    default:
      break;
    }
  return FALSE;
}

#define bfd_mach_o_swap_reloc_in bfd_mach_o_i386_swap_reloc_in
#define bfd_mach_o_swap_reloc_out bfd_mach_o_i386_swap_reloc_out
#define bfd_mach_o_print_thread bfd_mach_o_i386_print_thread

#define bfd_mach_o_bfd_reloc_type_lookup bfd_mach_o_i386_bfd_reloc_type_lookup 
#define bfd_mach_o_bfd_reloc_name_lookup bfd_mach_o_i386_bfd_reloc_name_lookup

#define TARGET_NAME 		mach_o_i386_vec
#define TARGET_STRING 		"mach-o-i386"
#define TARGET_ARCHITECTURE	bfd_arch_i386
#define TARGET_BIG_ENDIAN 	0
#define TARGET_ARCHIVE 		0
#include "mach-o-target.c"
