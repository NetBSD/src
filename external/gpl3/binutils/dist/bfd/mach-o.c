/* Mach-O support for BFD.
   Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009, 2010, 2011
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
#include "aout/stab_gnu.h"
#include <ctype.h>

#define bfd_mach_o_object_p bfd_mach_o_gen_object_p
#define bfd_mach_o_core_p bfd_mach_o_gen_core_p
#define bfd_mach_o_mkobject bfd_mach_o_gen_mkobject

#define FILE_ALIGN(off, algn) \
  (((off) + ((file_ptr) 1 << (algn)) - 1) & ((file_ptr) -1 << (algn)))

static int bfd_mach_o_read_symtab_symbols (bfd *);

unsigned int
bfd_mach_o_version (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = NULL;

  BFD_ASSERT (bfd_mach_o_valid (abfd));
  mdata = bfd_mach_o_get_data (abfd);

  return mdata->header.version;
}

bfd_boolean
bfd_mach_o_valid (bfd *abfd)
{
  if (abfd == NULL || abfd->xvec == NULL)
    return FALSE;

  if (abfd->xvec->flavour != bfd_target_mach_o_flavour)
    return FALSE;

  if (bfd_mach_o_get_data (abfd) == NULL)
    return FALSE;
  return TRUE;
}

static INLINE bfd_boolean
mach_o_wide_p (bfd_mach_o_header *header)
{
  switch (header->version)
    {
    case 1:
      return FALSE;
    case 2:
      return TRUE;
    default:
      BFD_FAIL ();
      return FALSE;
    }
}

static INLINE bfd_boolean
bfd_mach_o_wide_p (bfd *abfd)
{
  return mach_o_wide_p (&bfd_mach_o_get_data (abfd)->header);
}
      
/* Tables to translate well known Mach-O segment/section names to bfd
   names.  Use of canonical names (such as .text or .debug_frame) is required
   by gdb.  */

struct mach_o_section_name_xlat
{
  const char *bfd_name;
  const char *mach_o_name;
  flagword flags;
};

static const struct mach_o_section_name_xlat dwarf_section_names_xlat[] =
  {
    { ".debug_frame",    "__debug_frame",    SEC_DEBUGGING },
    { ".debug_info",     "__debug_info",     SEC_DEBUGGING },
    { ".debug_abbrev",   "__debug_abbrev",   SEC_DEBUGGING },
    { ".debug_aranges",  "__debug_aranges",  SEC_DEBUGGING },
    { ".debug_macinfo",  "__debug_macinfo",  SEC_DEBUGGING },
    { ".debug_line",     "__debug_line",     SEC_DEBUGGING },
    { ".debug_loc",      "__debug_loc",      SEC_DEBUGGING },
    { ".debug_pubnames", "__debug_pubnames", SEC_DEBUGGING },
    { ".debug_pubtypes", "__debug_pubtypes", SEC_DEBUGGING },
    { ".debug_str",      "__debug_str",      SEC_DEBUGGING },
    { ".debug_ranges",   "__debug_ranges",   SEC_DEBUGGING },
    { NULL, NULL, 0}
  };

static const struct mach_o_section_name_xlat text_section_names_xlat[] =
  {
    { ".text",     "__text",      SEC_CODE | SEC_LOAD },
    { ".const",    "__const",     SEC_READONLY | SEC_DATA | SEC_LOAD },
    { ".cstring",  "__cstring",   SEC_READONLY | SEC_DATA | SEC_LOAD },
    { ".eh_frame", "__eh_frame",  SEC_READONLY | SEC_LOAD },
    { NULL, NULL, 0}
  };

static const struct mach_o_section_name_xlat data_section_names_xlat[] =
  {
    { ".data",                "__data",          SEC_DATA | SEC_LOAD },
    { ".const_data",          "__const",         SEC_DATA | SEC_LOAD },
    { ".dyld",                "__dyld",          SEC_DATA | SEC_LOAD },
    { ".lazy_symbol_ptr",     "__la_symbol_ptr", SEC_DATA | SEC_LOAD },
    { ".non_lazy_symbol_ptr", "__nl_symbol_ptr", SEC_DATA | SEC_LOAD },
    { ".bss",                 "__bss",           SEC_NO_FLAGS },
    { NULL, NULL, 0}
  };

struct mach_o_segment_name_xlat
{
  const char *segname;
  const struct mach_o_section_name_xlat *sections;
};

static const struct mach_o_segment_name_xlat segsec_names_xlat[] =
  {
    { "__DWARF", dwarf_section_names_xlat },
    { "__TEXT", text_section_names_xlat },
    { "__DATA", data_section_names_xlat },
    { NULL, NULL }
  };


/* Mach-O to bfd names.  */

static void
bfd_mach_o_convert_section_name_to_bfd (bfd *abfd, bfd_mach_o_section *section,
                                        char **name, flagword *flags)
{
  const struct mach_o_segment_name_xlat *seg;
  char *res;
  unsigned int len;
  const char *pfx = "";

  *name = NULL;
  *flags = SEC_NO_FLAGS;

  for (seg = segsec_names_xlat; seg->segname; seg++)
    {
      if (strcmp (seg->segname, section->segname) == 0)
        {
          const struct mach_o_section_name_xlat *sec;

          for (sec = seg->sections; sec->mach_o_name; sec++)
            {
              if (strcmp (sec->mach_o_name, section->sectname) == 0)
                {
                  len = strlen (sec->bfd_name);
                  res = bfd_alloc (abfd, len + 1);

                  if (res == NULL)
                    return;
                  strcpy (res, sec->bfd_name);
                  *name = res;
                  *flags = sec->flags;
                  return;
                }
            }
        }
    }

  len = strlen (section->segname) + 1
    + strlen (section->sectname) + 1;

  /* Put "LC_SEGMENT." prefix if the segment name is weird (ie doesn't start
     with an underscore.  */
  if (section->segname[0] != '_')
    {
      static const char seg_pfx[] = "LC_SEGMENT.";

      pfx = seg_pfx;
      len += sizeof (seg_pfx) - 1;
    }

  res = bfd_alloc (abfd, len);
  if (res == NULL)
    return;
  snprintf (res, len, "%s%s.%s", pfx, section->segname, section->sectname);
  *name = res;
}

/* Convert a bfd section name to a Mach-O segment + section name.  */

static void
bfd_mach_o_convert_section_name_to_mach_o (bfd *abfd ATTRIBUTE_UNUSED,
                                           asection *sect,
                                           bfd_mach_o_section *section)
{
  const struct mach_o_segment_name_xlat *seg;
  const char *name = bfd_get_section_name (abfd, sect);
  const char *dot;
  unsigned int len;
  unsigned int seglen;
  unsigned int seclen;

  /* List of well known names.  They all start with a dot.  */
  if (name[0] == '.')
    for (seg = segsec_names_xlat; seg->segname; seg++)
      {
        const struct mach_o_section_name_xlat *sec;

        for (sec = seg->sections; sec->mach_o_name; sec++)
          {
            if (strcmp (sec->bfd_name, name) == 0)
              {
                strcpy (section->segname, seg->segname);
                strcpy (section->sectname, sec->mach_o_name);
                return;
              }
          }
      }

  /* Strip LC_SEGMENT. prefix.  */
  if (strncmp (name, "LC_SEGMENT.", 11) == 0)
    name += 11;

  /* Find a dot.  */
  dot = strchr (name, '.');
  len = strlen (name);

  /* Try to split name into segment and section names.  */
  if (dot && dot != name)
    {
      seglen = dot - name;
      seclen = len - (dot + 1 - name);

      if (seglen < 16 && seclen < 16)
        {
          memcpy (section->segname, name, seglen);
          section->segname[seglen] = 0;
          memcpy (section->sectname, dot + 1, seclen);
          section->sectname[seclen] = 0;
          return;
        }
    }

  if (len > 16)
    len = 16;
  memcpy (section->segname, name, len);
  section->segname[len] = 0;
  memcpy (section->sectname, name, len);
  section->sectname[len] = 0;
}

/* Return the size of an entry for section SEC.
   Must be called only for symbol pointer section and symbol stubs
   sections.  */

static unsigned int
bfd_mach_o_section_get_entry_size (bfd *abfd, bfd_mach_o_section *sec)
{
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
      return bfd_mach_o_wide_p (abfd) ? 8 : 4;
    case BFD_MACH_O_S_SYMBOL_STUBS:
      return sec->reserved2;
    default:
      BFD_FAIL ();
      return 0;
    }
}

/* Return the number of indirect symbols for a section.
   Must be called only for symbol pointer section and symbol stubs
   sections.  */

static unsigned int
bfd_mach_o_section_get_nbr_indirect (bfd *abfd, bfd_mach_o_section *sec)
{
  unsigned int elsz;

  elsz = bfd_mach_o_section_get_entry_size (abfd, sec);
  if (elsz == 0)
    return 0;
  else
    return sec->size / elsz;
}


/* Copy any private info we understand from the input symbol
   to the output symbol.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_symbol_data (bfd *ibfd ATTRIBUTE_UNUSED,
					 asymbol *isymbol ATTRIBUTE_UNUSED,
					 bfd *obfd ATTRIBUTE_UNUSED,
					 asymbol *osymbol ATTRIBUTE_UNUSED)
{
  return TRUE;
}

/* Copy any private info we understand from the input section
   to the output section.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_section_data (bfd *ibfd ATTRIBUTE_UNUSED,
					  asection *isection ATTRIBUTE_UNUSED,
					  bfd *obfd ATTRIBUTE_UNUSED,
					  asection *osection ATTRIBUTE_UNUSED)
{
  return TRUE;
}

/* Copy any private info we understand from the input bfd
   to the output bfd.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  if (bfd_get_flavour (ibfd) != bfd_target_mach_o_flavour
      || bfd_get_flavour (obfd) != bfd_target_mach_o_flavour)
    return TRUE;

  BFD_ASSERT (bfd_mach_o_valid (ibfd));
  BFD_ASSERT (bfd_mach_o_valid (obfd));

  /* FIXME: copy commands.  */

  return TRUE;
}

/* Count the total number of symbols.  */

static long
bfd_mach_o_count_symbols (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (mdata->symtab == NULL)
    return 0;
  return mdata->symtab->nsyms;
}

long
bfd_mach_o_get_symtab_upper_bound (bfd *abfd)
{
  long nsyms = bfd_mach_o_count_symbols (abfd);

  return ((nsyms + 1) * sizeof (asymbol *));
}

long
bfd_mach_o_canonicalize_symtab (bfd *abfd, asymbol **alocation)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  long nsyms = bfd_mach_o_count_symbols (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;
  unsigned long j;

  if (nsyms < 0)
    return nsyms;

  if (bfd_mach_o_read_symtab_symbols (abfd) != 0)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_canonicalize_symtab: unable to load symbols"));
      return 0;
    }

  BFD_ASSERT (sym->symbols != NULL);

  for (j = 0; j < sym->nsyms; j++)
    alocation[j] = &sym->symbols[j].symbol;

  alocation[j] = NULL;

  return nsyms;
}

long
bfd_mach_o_get_synthetic_symtab (bfd *abfd,
                                 long symcount ATTRIBUTE_UNUSED,
                                 asymbol **syms ATTRIBUTE_UNUSED,
                                 long dynsymcount ATTRIBUTE_UNUSED,
                                 asymbol **dynsyms ATTRIBUTE_UNUSED,
                                 asymbol **ret)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_dysymtab_command *dysymtab = mdata->dysymtab;
  bfd_mach_o_symtab_command *symtab = mdata->symtab;
  asymbol *s;
  unsigned long count, i, j, n;
  size_t size;
  char *names;
  char *nul_name;

  *ret = NULL;

  if (dysymtab == NULL || symtab == NULL || symtab->symbols == NULL)
    return 0;

  if (dysymtab->nindirectsyms == 0)
    return 0;

  count = dysymtab->nindirectsyms;
  size = count * sizeof (asymbol) + 1;

  for (j = 0; j < count; j++)
    {
      unsigned int isym = dysymtab->indirect_syms[j];
              
      if (isym < symtab->nsyms && symtab->symbols[isym].symbol.name)
        size += strlen (symtab->symbols[isym].symbol.name) + sizeof ("$stub");
    }

  s = *ret = (asymbol *) bfd_malloc (size);
  if (s == NULL)
    return -1;
  names = (char *) (s + count);
  nul_name = names;
  *names++ = 0;
  
  n = 0;
  for (i = 0; i < mdata->nsects; i++)
    {
      bfd_mach_o_section *sec = mdata->sections[i];
      unsigned int first, last;
      bfd_vma addr;
      bfd_vma entry_size;
      
      switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
        {
        case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
        case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
        case BFD_MACH_O_S_SYMBOL_STUBS:
          first = sec->reserved1;
          last = first + bfd_mach_o_section_get_nbr_indirect (abfd, sec);
          addr = sec->addr;
          entry_size = bfd_mach_o_section_get_entry_size (abfd, sec);
          for (j = first; j < last; j++)
            {
              unsigned int isym = dysymtab->indirect_syms[j];

              s->flags = BSF_GLOBAL | BSF_SYNTHETIC;
              s->section = sec->bfdsection;
              s->value = addr - sec->addr;
              s->udata.p = NULL;
              
              if (isym < symtab->nsyms
                  && symtab->symbols[isym].symbol.name)
                {
                  const char *sym = symtab->symbols[isym].symbol.name;
                  size_t len;

                  s->name = names;
                  len = strlen (sym);
                  memcpy (names, sym, len);
                  names += len;
                  memcpy (names, "$stub", sizeof ("$stub"));
                  names += sizeof ("$stub");
                }
              else
                s->name = nul_name;

              addr += entry_size;
              s++;
              n++;
            }
          break;
        default:
          break;
        }
    }

  return n;
}

void
bfd_mach_o_get_symbol_info (bfd *abfd ATTRIBUTE_UNUSED,
			    asymbol *symbol,
			    symbol_info *ret)
{
  bfd_symbol_info (symbol, ret);
}

void
bfd_mach_o_print_symbol (bfd *abfd,
			 void * afile,
			 asymbol *symbol,
			 bfd_print_symbol_type how)
{
  FILE *file = (FILE *) afile;
  const char *name;
  bfd_mach_o_asymbol *asym = (bfd_mach_o_asymbol *)symbol;

  switch (how)
    {
    case bfd_print_symbol_name:
      fprintf (file, "%s", symbol->name);
      break;
    default:
      bfd_print_symbol_vandf (abfd, (void *) file, symbol);
      if (asym->n_type & BFD_MACH_O_N_STAB)
	name = bfd_get_stab_name (asym->n_type);
      else
	switch (asym->n_type & BFD_MACH_O_N_TYPE)
	  {
	  case BFD_MACH_O_N_UNDF:
	    name = "UND";
	    break;
	  case BFD_MACH_O_N_ABS:
	    name = "ABS";
	    break;
	  case BFD_MACH_O_N_INDR:
	    name = "INDR";
	    break;
	  case BFD_MACH_O_N_PBUD:
	    name = "PBUD";
	    break;
	  case BFD_MACH_O_N_SECT:
	    name = "SECT";
	    break;
	  default:
	    name = "???";
	    break;
	  }
      if (name == NULL)
	name = "";
      fprintf (file, " %02x %-6s %02x %04x",
               asym->n_type, name, asym->n_sect, asym->n_desc);
      if ((asym->n_type & BFD_MACH_O_N_STAB) == 0
	  && (asym->n_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_SECT)
	fprintf (file, " %-5s", symbol->section->name);
      fprintf (file, " %s", symbol->name);
    }
}

static void
bfd_mach_o_convert_architecture (bfd_mach_o_cpu_type mtype,
				 bfd_mach_o_cpu_subtype msubtype ATTRIBUTE_UNUSED,
				 enum bfd_architecture *type,
				 unsigned long *subtype)
{
  *subtype = bfd_arch_unknown;

  switch (mtype)
    {
    case BFD_MACH_O_CPU_TYPE_VAX: *type = bfd_arch_vax; break;
    case BFD_MACH_O_CPU_TYPE_MC680x0: *type = bfd_arch_m68k; break;
    case BFD_MACH_O_CPU_TYPE_I386:
      *type = bfd_arch_i386;
      *subtype = bfd_mach_i386_i386;
      break;
    case BFD_MACH_O_CPU_TYPE_X86_64:
      *type = bfd_arch_i386;
      *subtype = bfd_mach_x86_64;
      break;
    case BFD_MACH_O_CPU_TYPE_MIPS: *type = bfd_arch_mips; break;
    case BFD_MACH_O_CPU_TYPE_MC98000: *type = bfd_arch_m98k; break;
    case BFD_MACH_O_CPU_TYPE_HPPA: *type = bfd_arch_hppa; break;
    case BFD_MACH_O_CPU_TYPE_ARM: *type = bfd_arch_arm; break;
    case BFD_MACH_O_CPU_TYPE_MC88000: *type = bfd_arch_m88k; break;
    case BFD_MACH_O_CPU_TYPE_SPARC:
      *type = bfd_arch_sparc;
      *subtype = bfd_mach_sparc;
      break;
    case BFD_MACH_O_CPU_TYPE_I860: *type = bfd_arch_i860; break;
    case BFD_MACH_O_CPU_TYPE_ALPHA: *type = bfd_arch_alpha; break;
    case BFD_MACH_O_CPU_TYPE_POWERPC:
      *type = bfd_arch_powerpc;
      *subtype = bfd_mach_ppc;
      break;
    case BFD_MACH_O_CPU_TYPE_POWERPC_64:
      *type = bfd_arch_powerpc;
      *subtype = bfd_mach_ppc64;
      break;
    default:
      *type = bfd_arch_unknown;
      break;
    }
}

static bfd_boolean
bfd_mach_o_write_header (bfd *abfd, bfd_mach_o_header *header)
{
  unsigned char buf[32];
  unsigned int size;

  size = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  bfd_h_put_32 (abfd, header->magic, buf + 0);
  bfd_h_put_32 (abfd, header->cputype, buf + 4);
  bfd_h_put_32 (abfd, header->cpusubtype, buf + 8);
  bfd_h_put_32 (abfd, header->filetype, buf + 12);
  bfd_h_put_32 (abfd, header->ncmds, buf + 16);
  bfd_h_put_32 (abfd, header->sizeofcmds, buf + 20);
  bfd_h_put_32 (abfd, header->flags, buf + 24);

  if (mach_o_wide_p (header))
    bfd_h_put_32 (abfd, header->reserved, buf + 28);

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bwrite ((void *) buf, size, abfd) != size)
    return FALSE;

  return TRUE;
}

static int
bfd_mach_o_write_thread (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_thread_command *cmd = &command->command.thread;
  unsigned int i;
  unsigned char buf[8];
  unsigned int offset;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_THREAD)
	      || (command->type == BFD_MACH_O_LC_UNIXTHREAD));

  offset = 8;
  for (i = 0; i < cmd->nflavours; i++)
    {
      BFD_ASSERT ((cmd->flavours[i].size % 4) == 0);
      BFD_ASSERT (cmd->flavours[i].offset == (command->offset + offset + 8));

      bfd_h_put_32 (abfd, cmd->flavours[i].flavour, buf);
      bfd_h_put_32 (abfd, (cmd->flavours[i].size / 4), buf + 4);

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bwrite ((void *) buf, 8, abfd) != 8)
	return -1;

      offset += cmd->flavours[i].size + 8;
    }

  return 0;
}

long
bfd_mach_o_get_reloc_upper_bound (bfd *abfd ATTRIBUTE_UNUSED,
                                  asection *asect)
{
  return (asect->reloc_count + 1) * sizeof (arelent *);
}

static int
bfd_mach_o_canonicalize_one_reloc (bfd *abfd, char *buf,
                                   arelent *res, asymbol **syms)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  bfd_mach_o_reloc_info reloc;
  bfd_vma addr;
  bfd_vma symnum;
  asymbol **sym;

  addr = bfd_get_32 (abfd, buf + 0);
  symnum = bfd_get_32 (abfd, buf + 4);
  
  if (addr & BFD_MACH_O_SR_SCATTERED)
    {
      unsigned int j;

      /* Scattered relocation.
         Extract section and offset from r_value.  */
      res->sym_ptr_ptr = NULL;
      res->addend = 0;
      for (j = 0; j < mdata->nsects; j++)
        {
          bfd_mach_o_section *sect = mdata->sections[j];
          if (symnum >= sect->addr && symnum < sect->addr + sect->size)
            {
              res->sym_ptr_ptr = sect->bfdsection->symbol_ptr_ptr;
              res->addend = symnum - sect->addr;
              break;
            }
        }
      res->address = BFD_MACH_O_GET_SR_ADDRESS (addr);
      reloc.r_type = BFD_MACH_O_GET_SR_TYPE (addr);
      reloc.r_length = BFD_MACH_O_GET_SR_LENGTH (addr);
      reloc.r_pcrel = addr & BFD_MACH_O_SR_PCREL;
      reloc.r_scattered = 1;
    }
  else
    {
      unsigned int num = BFD_MACH_O_GET_R_SYMBOLNUM (symnum);
      res->addend = 0;
      res->address = addr;
      if (symnum & BFD_MACH_O_R_EXTERN)
        {
          sym = syms + num;
          reloc.r_extern = 1;
        }
      else
        {
          BFD_ASSERT (num != 0);
          BFD_ASSERT (num <= mdata->nsects);
          sym = mdata->sections[num - 1]->bfdsection->symbol_ptr_ptr;
          /* For a symbol defined in section S, the addend (stored in the
             binary) contains the address of the section.  To comply with
             bfd conventio, substract the section address.
             Use the address from the header, so that the user can modify
             the vma of the section.  */
          res->addend = -mdata->sections[num - 1]->addr;
          reloc.r_extern = 0;
        }
      res->sym_ptr_ptr = sym;
      reloc.r_type = BFD_MACH_O_GET_R_TYPE (symnum);
      reloc.r_length = BFD_MACH_O_GET_R_LENGTH (symnum);
      reloc.r_pcrel = (symnum & BFD_MACH_O_R_PCREL) ? 1 : 0;
      reloc.r_scattered = 0;
    }
  
  if (!(*bed->_bfd_mach_o_swap_reloc_in)(res, &reloc))
    return -1;
  return 0;
}

static int
bfd_mach_o_canonicalize_relocs (bfd *abfd, unsigned long filepos,
                                unsigned long count,
                                arelent *res, asymbol **syms)
{
  unsigned long i;
  char *native_relocs;
  bfd_size_type native_size;

  /* Allocate and read relocs.  */
  native_size = count * BFD_MACH_O_RELENT_SIZE;
  native_relocs = bfd_malloc (native_size);
  if (native_relocs == NULL)
    return -1;

  if (bfd_seek (abfd, filepos, SEEK_SET) != 0
      || bfd_bread (native_relocs, native_size, abfd) != native_size)
    goto err;

  for (i = 0; i < count; i++)
    {
      char *buf = native_relocs + BFD_MACH_O_RELENT_SIZE * i;

      if (bfd_mach_o_canonicalize_one_reloc (abfd, buf, &res[i], syms) < 0)
        goto err;
    }
  free (native_relocs);
  return i;
 err:
  free (native_relocs);
  return -1;
}

long
bfd_mach_o_canonicalize_reloc (bfd *abfd, asection *asect,
                               arelent **rels, asymbol **syms)
{
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  unsigned long i;
  arelent *res;

  if (asect->reloc_count == 0)
    return 0;

  /* No need to go further if we don't know how to read relocs.  */
  if (bed->_bfd_mach_o_swap_reloc_in == NULL)
    return 0;

  res = bfd_malloc (asect->reloc_count * sizeof (arelent));
  if (res == NULL)
    return -1;

  if (bfd_mach_o_canonicalize_relocs (abfd, asect->rel_filepos,
                                      asect->reloc_count, res, syms) < 0)
    {
      free (res);
      return -1;
    }

  for (i = 0; i < asect->reloc_count; i++)
    rels[i] = &res[i];
  rels[i] = NULL;
  asect->relocation = res;

  return i;
}

long
bfd_mach_o_get_dynamic_reloc_upper_bound (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (mdata->dysymtab == NULL)
    return 1;
  return (mdata->dysymtab->nextrel + mdata->dysymtab->nlocrel)
    * sizeof (arelent *);
}

long
bfd_mach_o_canonicalize_dynamic_reloc (bfd *abfd, arelent **rels,
                                       struct bfd_symbol **syms)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_dysymtab_command *dysymtab = mdata->dysymtab;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  unsigned long i;
  arelent *res;

  if (dysymtab == NULL)
    return 0;
  if (dysymtab->nextrel == 0 && dysymtab->nlocrel == 0)
    return 0;

  /* No need to go further if we don't know how to read relocs.  */
  if (bed->_bfd_mach_o_swap_reloc_in == NULL)
    return 0;

  res = bfd_malloc ((dysymtab->nextrel + dysymtab->nlocrel) * sizeof (arelent));
  if (res == NULL)
    return -1;

  if (bfd_mach_o_canonicalize_relocs (abfd, dysymtab->extreloff,
                                      dysymtab->nextrel, res, syms) < 0)
    {
      free (res);
      return -1;
    }

  if (bfd_mach_o_canonicalize_relocs (abfd, dysymtab->locreloff,
                                      dysymtab->nlocrel,
                                      res + dysymtab->nextrel, syms) < 0)
    {
      free (res);
      return -1;
    }

  for (i = 0; i < dysymtab->nextrel + dysymtab->nlocrel; i++)
    rels[i] = &res[i];
  rels[i] = NULL;
  return i;
}

static bfd_boolean
bfd_mach_o_write_relocs (bfd *abfd, bfd_mach_o_section *section)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i;
  arelent **entries;
  asection *sec;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  sec = section->bfdsection;
  if (sec->reloc_count == 0)
    return TRUE;

  if (bed->_bfd_mach_o_swap_reloc_out == NULL)
    return TRUE;

  /* Allocate relocation room.  */
  mdata->filelen = FILE_ALIGN(mdata->filelen, 2);
  section->nreloc = sec->reloc_count;
  sec->rel_filepos = mdata->filelen;
  section->reloff = sec->rel_filepos;
  mdata->filelen += sec->reloc_count * BFD_MACH_O_RELENT_SIZE;

  if (bfd_seek (abfd, section->reloff, SEEK_SET) != 0)
    return FALSE;

  /* Convert and write.  */
  entries = section->bfdsection->orelocation;
  for (i = 0; i < section->nreloc; i++)
    {
      arelent *rel = entries[i];
      char buf[8];
      bfd_mach_o_reloc_info info, *pinfo = &info;

      /* Convert relocation to an intermediate representation.  */
      if (!(*bed->_bfd_mach_o_swap_reloc_out) (rel, pinfo))
        return FALSE;

      /* Lower the relocation info.  */
      if (pinfo->r_scattered)
        {
          unsigned long v;

          v = BFD_MACH_O_SR_SCATTERED
            | (pinfo->r_pcrel ? BFD_MACH_O_SR_PCREL : 0)
            | BFD_MACH_O_SET_SR_LENGTH(pinfo->r_length)
            | BFD_MACH_O_SET_SR_TYPE(pinfo->r_type)
            | BFD_MACH_O_SET_SR_ADDRESS(pinfo->r_address);
          bfd_put_32 (abfd, v, buf);
          bfd_put_32 (abfd, pinfo->r_value, buf + 4);
        }
      else
        {
          unsigned long v;

          bfd_put_32 (abfd, pinfo->r_address, buf);
          v = BFD_MACH_O_SET_R_SYMBOLNUM (pinfo->r_value)
            | (pinfo->r_pcrel ? BFD_MACH_O_R_PCREL : 0)
            | BFD_MACH_O_SET_R_LENGTH (pinfo->r_length)
            | (pinfo->r_extern ? BFD_MACH_O_R_EXTERN : 0)
            | BFD_MACH_O_SET_R_TYPE (pinfo->r_type);
          bfd_put_32 (abfd, v, buf + 4);
        }

      if (bfd_bwrite ((void *) buf, BFD_MACH_O_RELENT_SIZE, abfd)
          != BFD_MACH_O_RELENT_SIZE)
        return FALSE;
    }
  return TRUE;
}

static int
bfd_mach_o_write_section_32 (bfd *abfd, bfd_mach_o_section *section)
{
  unsigned char buf[BFD_MACH_O_SECTION_SIZE];

  memcpy (buf, section->sectname, 16);
  memcpy (buf + 16, section->segname, 16);
  bfd_h_put_32 (abfd, section->addr, buf + 32);
  bfd_h_put_32 (abfd, section->size, buf + 36);
  bfd_h_put_32 (abfd, section->offset, buf + 40);
  bfd_h_put_32 (abfd, section->align, buf + 44);
  bfd_h_put_32 (abfd, section->reloff, buf + 48);
  bfd_h_put_32 (abfd, section->nreloc, buf + 52);
  bfd_h_put_32 (abfd, section->flags, buf + 56);
  bfd_h_put_32 (abfd, section->reserved1, buf + 60);
  bfd_h_put_32 (abfd, section->reserved2, buf + 64);

  if (bfd_bwrite ((void *) buf, BFD_MACH_O_SECTION_SIZE, abfd)
      != BFD_MACH_O_SECTION_SIZE)
    return -1;

  return 0;
}

static int
bfd_mach_o_write_section_64 (bfd *abfd, bfd_mach_o_section *section)
{
  unsigned char buf[BFD_MACH_O_SECTION_64_SIZE];

  memcpy (buf, section->sectname, 16);
  memcpy (buf + 16, section->segname, 16);
  bfd_h_put_64 (abfd, section->addr, buf + 32);
  bfd_h_put_64 (abfd, section->size, buf + 40);
  bfd_h_put_32 (abfd, section->offset, buf + 48);
  bfd_h_put_32 (abfd, section->align, buf + 52);
  bfd_h_put_32 (abfd, section->reloff, buf + 56);
  bfd_h_put_32 (abfd, section->nreloc, buf + 60);
  bfd_h_put_32 (abfd, section->flags, buf + 64);
  bfd_h_put_32 (abfd, section->reserved1, buf + 68);
  bfd_h_put_32 (abfd, section->reserved2, buf + 72);
  bfd_h_put_32 (abfd, section->reserved3, buf + 76);

  if (bfd_bwrite ((void *) buf, BFD_MACH_O_SECTION_64_SIZE, abfd)
      != BFD_MACH_O_SECTION_64_SIZE)
    return -1;

  return 0;
}

static int
bfd_mach_o_write_segment_32 (bfd *abfd, bfd_mach_o_load_command *command)
{
  unsigned char buf[BFD_MACH_O_LC_SEGMENT_SIZE];
  bfd_mach_o_segment_command *seg = &command->command.segment;
  unsigned long i;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT);

  for (i = 0; i < seg->nsects; i++)
    if (!bfd_mach_o_write_relocs (abfd, &seg->sections[i]))
      return -1;

  memcpy (buf, seg->segname, 16);
  bfd_h_put_32 (abfd, seg->vmaddr, buf + 16);
  bfd_h_put_32 (abfd, seg->vmsize, buf + 20);
  bfd_h_put_32 (abfd, seg->fileoff, buf + 24);
  bfd_h_put_32 (abfd, seg->filesize, buf + 28);
  bfd_h_put_32 (abfd, seg->maxprot, buf + 32);
  bfd_h_put_32 (abfd, seg->initprot, buf + 36);
  bfd_h_put_32 (abfd, seg->nsects, buf + 40);
  bfd_h_put_32 (abfd, seg->flags, buf + 44);
  
  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || (bfd_bwrite ((void *) buf, BFD_MACH_O_LC_SEGMENT_SIZE - 8, abfd) 
          != BFD_MACH_O_LC_SEGMENT_SIZE - 8))
    return -1;

  for (i = 0; i < seg->nsects; i++)
    if (bfd_mach_o_write_section_32 (abfd, &seg->sections[i]))
      return -1;

  return 0;
}

static int
bfd_mach_o_write_segment_64 (bfd *abfd, bfd_mach_o_load_command *command)
{
  unsigned char buf[BFD_MACH_O_LC_SEGMENT_64_SIZE];
  bfd_mach_o_segment_command *seg = &command->command.segment;
  unsigned long i;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT_64);

  for (i = 0; i < seg->nsects; i++)
    if (!bfd_mach_o_write_relocs (abfd, &seg->sections[i]))
      return -1;

  memcpy (buf, seg->segname, 16);
  bfd_h_put_64 (abfd, seg->vmaddr, buf + 16);
  bfd_h_put_64 (abfd, seg->vmsize, buf + 24);
  bfd_h_put_64 (abfd, seg->fileoff, buf + 32);
  bfd_h_put_64 (abfd, seg->filesize, buf + 40);
  bfd_h_put_32 (abfd, seg->maxprot, buf + 48);
  bfd_h_put_32 (abfd, seg->initprot, buf + 52);
  bfd_h_put_32 (abfd, seg->nsects, buf + 56);
  bfd_h_put_32 (abfd, seg->flags, buf + 60);

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || (bfd_bwrite ((void *) buf, BFD_MACH_O_LC_SEGMENT_64_SIZE - 8, abfd)
          != BFD_MACH_O_LC_SEGMENT_64_SIZE - 8))
    return -1;

  for (i = 0; i < seg->nsects; i++)
    if (bfd_mach_o_write_section_64 (abfd, &seg->sections[i]))
      return -1;

  return 0;
}

static bfd_boolean
bfd_mach_o_write_symtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = &command->command.symtab;
  unsigned char buf[16];
  unsigned long i;
  unsigned int wide = bfd_mach_o_wide_p (abfd);
  unsigned int symlen = wide ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE;
  struct bfd_strtab_hash *strtab;
  asymbol **symbols = bfd_get_outsymbols (abfd);

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SYMTAB);

  /* Write the symbols first.  */
  mdata->filelen = FILE_ALIGN(mdata->filelen, wide ? 3 : 2);
  sym->symoff = mdata->filelen;
  if (bfd_seek (abfd, sym->symoff, SEEK_SET) != 0)
    return FALSE;

  sym->nsyms = bfd_get_symcount (abfd);
  mdata->filelen += sym->nsyms * symlen;

  strtab = _bfd_stringtab_init ();
  if (strtab == NULL)
    return FALSE;

  for (i = 0; i < sym->nsyms; i++)
    {
      bfd_size_type str_index;
      bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];

      /* Compute name index.  */
      /* An index of 0 always means the empty string.  */
      if (s->symbol.name == 0 || s->symbol.name[0] == '\0')
        str_index = 0;
      else
        {
          str_index = _bfd_stringtab_add (strtab, s->symbol.name, TRUE, FALSE);
          if (str_index == (bfd_size_type) -1)
            goto err;
        }
      bfd_h_put_32 (abfd, str_index, buf);
      bfd_h_put_8 (abfd, s->n_type, buf + 4);
      bfd_h_put_8 (abfd, s->n_sect, buf + 5);
      bfd_h_put_16 (abfd, s->n_desc, buf + 6);
      if (wide)
        bfd_h_put_64 (abfd, s->symbol.section->vma + s->symbol.value, buf + 8);
      else
        bfd_h_put_32 (abfd, s->symbol.section->vma + s->symbol.value, buf + 8);

      if (bfd_bwrite ((void *) buf, symlen, abfd) != symlen)
        goto err;
    }
  sym->strsize = _bfd_stringtab_size (strtab);
  sym->stroff = mdata->filelen;
  mdata->filelen += sym->strsize;

  if (_bfd_stringtab_emit (abfd, strtab) != TRUE)
    goto err;
  _bfd_stringtab_free (strtab);

  /* The command.  */
  bfd_h_put_32 (abfd, sym->symoff, buf);
  bfd_h_put_32 (abfd, sym->nsyms, buf + 4);
  bfd_h_put_32 (abfd, sym->stroff, buf + 8);
  bfd_h_put_32 (abfd, sym->strsize, buf + 12);

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bwrite ((void *) buf, 16, abfd) != 16)
    return FALSE;

  return TRUE;

 err:
  _bfd_stringtab_free (strtab);
  return FALSE;
}

/* Process the symbols and generate Mach-O specific fields.
   Number them.  */

static bfd_boolean
bfd_mach_o_mangle_symbols (bfd *abfd)
{
  unsigned long i;
  asymbol **symbols = bfd_get_outsymbols (abfd);

  for (i = 0; i < bfd_get_symcount (abfd); i++)
    {
      bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];

      if (s->n_type == BFD_MACH_O_N_UNDF && !(s->symbol.flags & BSF_DEBUGGING))
        {
          /* As genuine Mach-O symbols type shouldn't be N_UNDF (undefined
             symbols should be N_UNDEF | N_EXT), we suppose the back-end
             values haven't been set.  */
          if (s->symbol.section == bfd_abs_section_ptr)
            s->n_type = BFD_MACH_O_N_ABS;
          else if (s->symbol.section == bfd_und_section_ptr)
            {
              s->n_type = BFD_MACH_O_N_UNDF;
              if (s->symbol.flags & BSF_WEAK)
                s->n_desc |= BFD_MACH_O_N_WEAK_REF;
            }
          else if (s->symbol.section == bfd_com_section_ptr)
            s->n_type = BFD_MACH_O_N_UNDF | BFD_MACH_O_N_EXT;
          else
            s->n_type = BFD_MACH_O_N_SECT;
          
          if (s->symbol.flags & BSF_GLOBAL)
            s->n_type |= BFD_MACH_O_N_EXT;
        }

      /* Compute section index.  */
      if (s->symbol.section != bfd_abs_section_ptr
          && s->symbol.section != bfd_und_section_ptr
          && s->symbol.section != bfd_com_section_ptr)
        s->n_sect = s->symbol.section->target_index;

      /* Number symbols.  */
      s->symbol.udata.i = i;
    }
  return TRUE;
}

bfd_boolean
bfd_mach_o_write_contents (bfd *abfd)
{
  unsigned int i;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (mdata->header.ncmds == 0)
    if (!bfd_mach_o_build_commands (abfd))
      return FALSE;

  /* Now write header information.  */
  if (mdata->header.filetype == 0)
    {
      if (abfd->flags & EXEC_P)
        mdata->header.filetype = BFD_MACH_O_MH_EXECUTE;
      else if (abfd->flags & DYNAMIC)
        mdata->header.filetype = BFD_MACH_O_MH_DYLIB;
      else
        mdata->header.filetype = BFD_MACH_O_MH_OBJECT;
    }
  if (!bfd_mach_o_write_header (abfd, &mdata->header))
    return FALSE;

  /* Assign a number to each symbols.  */
  if (!bfd_mach_o_mangle_symbols (abfd))
    return FALSE;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      unsigned char buf[8];
      bfd_mach_o_load_command *cur = &mdata->commands[i];
      unsigned long typeflag;

      typeflag = cur->type | (cur->type_required ? BFD_MACH_O_LC_REQ_DYLD : 0);

      bfd_h_put_32 (abfd, typeflag, buf);
      bfd_h_put_32 (abfd, cur->len, buf + 4);

      if (bfd_seek (abfd, cur->offset, SEEK_SET) != 0
          || bfd_bwrite ((void *) buf, 8, abfd) != 8)
	return FALSE;

      switch (cur->type)
	{
	case BFD_MACH_O_LC_SEGMENT:
	  if (bfd_mach_o_write_segment_32 (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SEGMENT_64:
	  if (bfd_mach_o_write_segment_64 (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SYMTAB:
	  if (!bfd_mach_o_write_symtab (abfd, cur))
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SYMSEG:
	  break;
	case BFD_MACH_O_LC_THREAD:
	case BFD_MACH_O_LC_UNIXTHREAD:
	  if (bfd_mach_o_write_thread (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_LOADFVMLIB:
	case BFD_MACH_O_LC_IDFVMLIB:
	case BFD_MACH_O_LC_IDENT:
	case BFD_MACH_O_LC_FVMFILE:
	case BFD_MACH_O_LC_PREPAGE:
	case BFD_MACH_O_LC_DYSYMTAB:
	case BFD_MACH_O_LC_LOAD_DYLIB:
	case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
	case BFD_MACH_O_LC_ID_DYLIB:
	case BFD_MACH_O_LC_REEXPORT_DYLIB:
	case BFD_MACH_O_LC_LOAD_DYLINKER:
	case BFD_MACH_O_LC_ID_DYLINKER:
	case BFD_MACH_O_LC_PREBOUND_DYLIB:
	case BFD_MACH_O_LC_ROUTINES:
	case BFD_MACH_O_LC_SUB_FRAMEWORK:
	  break;
	default:
	  (*_bfd_error_handler) (_("unable to write unknown load command 0x%lx"),
				 (unsigned long) cur->type);
	  return FALSE;
	}
    }

  return TRUE;
}

/* Build Mach-O load commands from the sections.  */

bfd_boolean
bfd_mach_o_build_commands (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int wide = mach_o_wide_p (&mdata->header);
  bfd_mach_o_segment_command *seg;
  bfd_mach_o_section *sections;
  asection *sec;
  bfd_mach_o_load_command *cmd;
  bfd_mach_o_load_command *symtab_cmd;
  int target_index;

  /* Return now if commands are already built.  */
  if (mdata->header.ncmds)
    return FALSE;

  /* Very simple version: 1 command (segment) containing all sections.  */
  mdata->header.ncmds = 2;
  mdata->commands = bfd_alloc (abfd, mdata->header.ncmds
                               * sizeof (bfd_mach_o_load_command));
  if (mdata->commands == NULL)
    return FALSE;
  cmd = &mdata->commands[0];
  seg = &cmd->command.segment;

  seg->nsects = bfd_count_sections (abfd);
  sections = bfd_alloc (abfd, seg->nsects * sizeof (bfd_mach_o_section));
  if (sections == NULL)
    return FALSE;
  seg->sections = sections;

  /* Set segment command.  */
  if (wide)
    {
      cmd->type = BFD_MACH_O_LC_SEGMENT_64;
      cmd->offset = BFD_MACH_O_HEADER_64_SIZE;
      cmd->len = BFD_MACH_O_LC_SEGMENT_64_SIZE
        + BFD_MACH_O_SECTION_64_SIZE * seg->nsects;
    }
  else
    {
      cmd->type = BFD_MACH_O_LC_SEGMENT;
      cmd->offset = BFD_MACH_O_HEADER_SIZE;
      cmd->len = BFD_MACH_O_LC_SEGMENT_SIZE
        + BFD_MACH_O_SECTION_SIZE * seg->nsects;
    }
  cmd->type_required = FALSE;
  mdata->header.sizeofcmds = cmd->len;
  mdata->filelen = cmd->offset + cmd->len;

  /* Set symtab command.  */
  symtab_cmd = &mdata->commands[1];
  
  symtab_cmd->type = BFD_MACH_O_LC_SYMTAB;
  symtab_cmd->offset = cmd->offset + cmd->len;
  symtab_cmd->len = 6 * 4;
  symtab_cmd->type_required = FALSE;
  
  mdata->header.sizeofcmds += symtab_cmd->len;
  mdata->filelen += symtab_cmd->len;

  /* Fill segment command.  */
  memset (seg->segname, 0, sizeof (seg->segname));
  seg->vmaddr = 0;
  seg->fileoff = mdata->filelen;
  seg->filesize = 0;
  seg->maxprot = BFD_MACH_O_PROT_READ | BFD_MACH_O_PROT_WRITE
    | BFD_MACH_O_PROT_EXECUTE;
  seg->initprot = seg->maxprot;
  seg->flags = 0;

  /* Create Mach-O sections.  */
  target_index = 0;
  for (sec = abfd->sections; sec; sec = sec->next)
    {
      sections->bfdsection = sec;
      bfd_mach_o_convert_section_name_to_mach_o (abfd, sec, sections);
      sections->addr = bfd_get_section_vma (abfd, sec);
      sections->size = bfd_get_section_size (sec);
      sections->align = bfd_get_section_alignment (abfd, sec);

      if (sections->size != 0)
        {
          mdata->filelen = FILE_ALIGN (mdata->filelen, sections->align);
          sections->offset = mdata->filelen;
        }
      else
        sections->offset = 0;
      sections->reloff = 0;
      sections->nreloc = 0;
      sections->reserved1 = 0;
      sections->reserved2 = 0;
      sections->reserved3 = 0;

      sec->filepos = sections->offset;
      sec->target_index = ++target_index;

      mdata->filelen += sections->size;
      sections++;
    }
  seg->filesize = mdata->filelen - seg->fileoff;
  seg->vmsize = seg->filesize;

  return TRUE;
}

/* Set the contents of a section.  */

bfd_boolean
bfd_mach_o_set_section_contents (bfd *abfd,
				 asection *section,
				 const void * location,
				 file_ptr offset,
				 bfd_size_type count)
{
  file_ptr pos;

  /* This must be done first, because bfd_set_section_contents is
     going to set output_has_begun to TRUE.  */
  if (! abfd->output_has_begun && ! bfd_mach_o_build_commands (abfd))
    return FALSE;

  if (count == 0)
    return TRUE;

  pos = section->filepos + offset;
  if (bfd_seek (abfd, pos, SEEK_SET) != 0
      || bfd_bwrite (location, count, abfd) != count)
    return FALSE;

  return TRUE;
}

int
bfd_mach_o_sizeof_headers (bfd *a ATTRIBUTE_UNUSED,
			   struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  return 0;
}

/* Make an empty symbol.  This is required only because
   bfd_make_section_anyway wants to create a symbol for the section.  */

asymbol *
bfd_mach_o_make_empty_symbol (bfd *abfd)
{
  asymbol *new_symbol;

  new_symbol = bfd_zalloc (abfd, sizeof (bfd_mach_o_asymbol));
  if (new_symbol == NULL)
    return new_symbol;
  new_symbol->the_bfd = abfd;
  new_symbol->udata.i = 0;
  return new_symbol;
}

static bfd_boolean
bfd_mach_o_read_header (bfd *abfd, bfd_mach_o_header *header)
{
  unsigned char buf[32];
  unsigned int size;
  bfd_vma (*get32) (const void *) = NULL;

  /* Just read the magic number.  */
  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 4, abfd) != 4)
    return FALSE;

  if (bfd_getb32 (buf) == BFD_MACH_O_MH_MAGIC)
    {
      header->byteorder = BFD_ENDIAN_BIG;
      header->magic = BFD_MACH_O_MH_MAGIC;
      header->version = 1;
      get32 = bfd_getb32;
    }
  else if (bfd_getl32 (buf) == BFD_MACH_O_MH_MAGIC)
    {
      header->byteorder = BFD_ENDIAN_LITTLE;
      header->magic = BFD_MACH_O_MH_MAGIC;
      header->version = 1;
      get32 = bfd_getl32;
    }
  else if (bfd_getb32 (buf) == BFD_MACH_O_MH_MAGIC_64)
    {
      header->byteorder = BFD_ENDIAN_BIG;
      header->magic = BFD_MACH_O_MH_MAGIC_64;
      header->version = 2;
      get32 = bfd_getb32;
    }
  else if (bfd_getl32 (buf) == BFD_MACH_O_MH_MAGIC_64)
    {
      header->byteorder = BFD_ENDIAN_LITTLE;
      header->magic = BFD_MACH_O_MH_MAGIC_64;
      header->version = 2;
      get32 = bfd_getl32;
    }
  else
    {
      header->byteorder = BFD_ENDIAN_UNKNOWN;
      return FALSE;
    }

  /* Once the size of the header is known, read the full header.  */
  size = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread ((void *) buf, size, abfd) != size)
    return FALSE;

  header->cputype = (*get32) (buf + 4);
  header->cpusubtype = (*get32) (buf + 8);
  header->filetype = (*get32) (buf + 12);
  header->ncmds = (*get32) (buf + 16);
  header->sizeofcmds = (*get32) (buf + 20);
  header->flags = (*get32) (buf + 24);

  if (mach_o_wide_p (header))
    header->reserved = (*get32) (buf + 28);

  return TRUE;
}

static asection *
bfd_mach_o_make_bfd_section (bfd *abfd, bfd_mach_o_section *section,
			     unsigned long prot)
{
  asection *bfdsec;
  char *sname;
  flagword flags;

  bfd_mach_o_convert_section_name_to_bfd (abfd, section, &sname, &flags);
  if (sname == NULL)
    return NULL;

  if (flags == SEC_NO_FLAGS)
    {
      /* Try to guess flags.  */
      if (section->flags & BFD_MACH_O_S_ATTR_DEBUG)
        flags = SEC_DEBUGGING;
      else
        {
          flags = SEC_ALLOC;
          if ((section->flags & BFD_MACH_O_SECTION_TYPE_MASK)
              != BFD_MACH_O_S_ZEROFILL)
            {
              flags |= SEC_LOAD;
              if (prot & BFD_MACH_O_PROT_EXECUTE)
                flags |= SEC_CODE;
              if (prot & BFD_MACH_O_PROT_WRITE)
                flags |= SEC_DATA;
              else if (prot & BFD_MACH_O_PROT_READ)
                flags |= SEC_READONLY;
            }
        }
    }
  else
    {
      if ((flags & SEC_DEBUGGING) == 0)
        flags |= SEC_ALLOC;
    }

  if (section->offset != 0)
    flags |= SEC_HAS_CONTENTS;
  if (section->nreloc != 0)
    flags |= SEC_RELOC;

  bfdsec = bfd_make_section_anyway_with_flags (abfd, sname, flags);
  if (bfdsec == NULL)
    return NULL;

  bfdsec->vma = section->addr;
  bfdsec->lma = section->addr;
  bfdsec->size = section->size;
  bfdsec->filepos = section->offset;
  bfdsec->alignment_power = section->align;
  bfdsec->segment_mark = 0;
  bfdsec->reloc_count = section->nreloc;
  bfdsec->rel_filepos = section->reloff;

  return bfdsec;
}

static int
bfd_mach_o_read_section_32 (bfd *abfd,
                            bfd_mach_o_section *section,
                            unsigned int offset,
                            unsigned long prot)
{
  unsigned char buf[BFD_MACH_O_SECTION_SIZE];

  if (bfd_seek (abfd, offset, SEEK_SET) != 0
      || (bfd_bread ((void *) buf, BFD_MACH_O_SECTION_SIZE, abfd)
          != BFD_MACH_O_SECTION_SIZE))
    return -1;

  memcpy (section->sectname, buf, 16);
  section->sectname[16] = '\0';
  memcpy (section->segname, buf + 16, 16);
  section->segname[16] = '\0';
  section->addr = bfd_h_get_32 (abfd, buf + 32);
  section->size = bfd_h_get_32 (abfd, buf + 36);
  section->offset = bfd_h_get_32 (abfd, buf + 40);
  section->align = bfd_h_get_32 (abfd, buf + 44);
  section->reloff = bfd_h_get_32 (abfd, buf + 48);
  section->nreloc = bfd_h_get_32 (abfd, buf + 52);
  section->flags = bfd_h_get_32 (abfd, buf + 56);
  section->reserved1 = bfd_h_get_32 (abfd, buf + 60);
  section->reserved2 = bfd_h_get_32 (abfd, buf + 64);
  section->reserved3 = 0;
  section->bfdsection = bfd_mach_o_make_bfd_section (abfd, section, prot);

  if (section->bfdsection == NULL)
    return -1;

  return 0;
}

static int
bfd_mach_o_read_section_64 (bfd *abfd,
                            bfd_mach_o_section *section,
                            unsigned int offset,
                            unsigned long prot)
{
  unsigned char buf[BFD_MACH_O_SECTION_64_SIZE];

  if (bfd_seek (abfd, offset, SEEK_SET) != 0
      || (bfd_bread ((void *) buf, BFD_MACH_O_SECTION_64_SIZE, abfd)
          != BFD_MACH_O_SECTION_64_SIZE))
    return -1;

  memcpy (section->sectname, buf, 16);
  section->sectname[16] = '\0';
  memcpy (section->segname, buf + 16, 16);
  section->segname[16] = '\0';
  section->addr = bfd_h_get_64 (abfd, buf + 32);
  section->size = bfd_h_get_64 (abfd, buf + 40);
  section->offset = bfd_h_get_32 (abfd, buf + 48);
  section->align = bfd_h_get_32 (abfd, buf + 52);
  section->reloff = bfd_h_get_32 (abfd, buf + 56);
  section->nreloc = bfd_h_get_32 (abfd, buf + 60);
  section->flags = bfd_h_get_32 (abfd, buf + 64);
  section->reserved1 = bfd_h_get_32 (abfd, buf + 68);
  section->reserved2 = bfd_h_get_32 (abfd, buf + 72);
  section->reserved3 = bfd_h_get_32 (abfd, buf + 76);
  section->bfdsection = bfd_mach_o_make_bfd_section (abfd, section, prot);

  if (section->bfdsection == NULL)
    return -1;

  return 0;
}

static int
bfd_mach_o_read_section (bfd *abfd,
                         bfd_mach_o_section *section,
                         unsigned int offset,
                         unsigned long prot,
                         unsigned int wide)
{
  if (wide)
    return bfd_mach_o_read_section_64 (abfd, section, offset, prot);
  else
    return bfd_mach_o_read_section_32 (abfd, section, offset, prot);
}

static int
bfd_mach_o_read_symtab_symbol (bfd *abfd,
                               bfd_mach_o_symtab_command *sym,
                               bfd_mach_o_asymbol *s,
                               unsigned long i)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int wide = mach_o_wide_p (&mdata->header);
  unsigned int symwidth =
    wide ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE;
  unsigned int symoff = sym->symoff + (i * symwidth);
  unsigned char buf[16];
  unsigned char type = -1;
  unsigned char section = -1;
  short desc = -1;
  symvalue value = -1;
  unsigned long stroff = -1;
  unsigned int symtype = -1;

  BFD_ASSERT (sym->strtab != NULL);

  if (bfd_seek (abfd, symoff, SEEK_SET) != 0
      || bfd_bread ((void *) buf, symwidth, abfd) != symwidth)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: unable to read %d bytes at %lu"),
			     symwidth, (unsigned long) symoff);
      return -1;
    }

  stroff = bfd_h_get_32 (abfd, buf);
  type = bfd_h_get_8 (abfd, buf + 4);
  symtype = type & BFD_MACH_O_N_TYPE;
  section = bfd_h_get_8 (abfd, buf + 5);
  desc = bfd_h_get_16 (abfd, buf + 6);
  if (wide)
    value = bfd_h_get_64 (abfd, buf + 8);
  else
    value = bfd_h_get_32 (abfd, buf + 8);

  if (stroff >= sym->strsize)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: symbol name out of range (%lu >= %lu)"),
			     (unsigned long) stroff,
			     (unsigned long) sym->strsize);
      return -1;
    }

  s->symbol.the_bfd = abfd;
  s->symbol.name = sym->strtab + stroff;
  s->symbol.value = value;
  s->symbol.flags = 0x0;
  s->symbol.udata.i = 0;
  s->n_type = type;
  s->n_sect = section;
  s->n_desc = desc;

  if (type & BFD_MACH_O_N_STAB)
    {
      s->symbol.flags |= BSF_DEBUGGING;
      s->symbol.section = bfd_und_section_ptr;
      switch (type)
	{
	case N_FUN:
	case N_STSYM:
	case N_LCSYM:
	case N_BNSYM:
	case N_SLINE:
	case N_ENSYM:
	case N_ECOMM:
	case N_ECOML:
	case N_GSYM:
	  if ((section > 0) && (section <= mdata->nsects))
	    {
	      s->symbol.section = mdata->sections[section - 1]->bfdsection;
	      s->symbol.value =
                s->symbol.value - mdata->sections[section - 1]->addr;
	    }
	  break;
	}
    }
  else
    {
      if (type & BFD_MACH_O_N_PEXT)
	s->symbol.flags |= BSF_GLOBAL;

      if (type & BFD_MACH_O_N_EXT)
	s->symbol.flags |= BSF_GLOBAL;

      if (!(type & (BFD_MACH_O_N_PEXT | BFD_MACH_O_N_EXT)))
	s->symbol.flags |= BSF_LOCAL;

      switch (symtype)
	{
	case BFD_MACH_O_N_UNDF:
          if (type == (BFD_MACH_O_N_UNDF | BFD_MACH_O_N_EXT)
              && s->symbol.value != 0)
            {
              /* A common symbol.  */
              s->symbol.section = bfd_com_section_ptr;
              s->symbol.flags = BSF_NO_FLAGS;
            }
          else
            {
              s->symbol.section = bfd_und_section_ptr;
              if (s->n_desc & BFD_MACH_O_N_WEAK_REF)
                s->symbol.flags |= BSF_WEAK;
            }
	  break;
	case BFD_MACH_O_N_PBUD:
	  s->symbol.section = bfd_und_section_ptr;
	  break;
	case BFD_MACH_O_N_ABS:
	  s->symbol.section = bfd_abs_section_ptr;
	  break;
	case BFD_MACH_O_N_SECT:
	  if ((section > 0) && (section <= mdata->nsects))
	    {
	      s->symbol.section = mdata->sections[section - 1]->bfdsection;
	      s->symbol.value =
                s->symbol.value - mdata->sections[section - 1]->addr;
	    }
	  else
	    {
	      /* Mach-O uses 0 to mean "no section"; not an error.  */
	      if (section != 0)
		{
		  (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: "
					   "symbol \"%s\" specified invalid section %d (max %lu): setting to undefined"),
					 s->symbol.name, section, mdata->nsects);
		}
	      s->symbol.section = bfd_und_section_ptr;
	    }
	  break;
	case BFD_MACH_O_N_INDR:
	  (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: "
				   "symbol \"%s\" is unsupported 'indirect' reference: setting to undefined"),
				 s->symbol.name);
	  s->symbol.section = bfd_und_section_ptr;
	  break;
	default:
	  (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: "
				   "symbol \"%s\" specified invalid type field 0x%x: setting to undefined"),
				 s->symbol.name, symtype);
	  s->symbol.section = bfd_und_section_ptr;
	  break;
	}
    }

  return 0;
}

static int
bfd_mach_o_read_symtab_strtab (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;

  /* Fail if there is no symtab.  */
  if (sym == NULL)
    return -1;

  /* Success if already loaded.  */
  if (sym->strtab)
    return 0;

  if (abfd->flags & BFD_IN_MEMORY)
    {
      struct bfd_in_memory *b;

      b = (struct bfd_in_memory *) abfd->iostream;

      if ((sym->stroff + sym->strsize) > b->size)
	{
	  bfd_set_error (bfd_error_file_truncated);
	  return -1;
	}
      sym->strtab = (char *) b->buffer + sym->stroff;
    }
  else
    {
      sym->strtab = bfd_alloc (abfd, sym->strsize);
      if (sym->strtab == NULL)
        return -1;

      if (bfd_seek (abfd, sym->stroff, SEEK_SET) != 0
          || bfd_bread ((void *) sym->strtab, sym->strsize, abfd) != sym->strsize)
        {
          bfd_set_error (bfd_error_file_truncated);
          return -1;
        }
    }

  return 0;
}

static int
bfd_mach_o_read_symtab_symbols (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;
  unsigned long i;
  int ret;

  if (sym->symbols)
    return 0;

  sym->symbols = bfd_alloc (abfd, sym->nsyms * sizeof (bfd_mach_o_asymbol));

  if (sym->symbols == NULL)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbols: unable to allocate memory for symbols"));
      return -1;
    }

  ret = bfd_mach_o_read_symtab_strtab (abfd);
  if (ret != 0)
    return ret;

  for (i = 0; i < sym->nsyms; i++)
    {
      ret = bfd_mach_o_read_symtab_symbol (abfd, sym, &sym->symbols[i], i);
      if (ret != 0)
	return ret;
    }

  return 0;
}

int
bfd_mach_o_read_dysymtab_symbol (bfd *abfd,
                                 bfd_mach_o_dysymtab_command *dysym,
                                 bfd_mach_o_symtab_command *sym,
                                 bfd_mach_o_asymbol *s,
                                 unsigned long i)
{
  unsigned long isymoff = dysym->indirectsymoff + (i * 4);
  unsigned long sym_index;
  unsigned char buf[4];

  BFD_ASSERT (i < dysym->nindirectsyms);

  if (bfd_seek (abfd, isymoff, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 4, abfd) != 4)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_read_dysymtab_symbol: unable to read %lu bytes at %lu"),
			       (unsigned long) 4, isymoff);
      return -1;
    }
  sym_index = bfd_h_get_32 (abfd, buf);

  return bfd_mach_o_read_symtab_symbol (abfd, sym, s, sym_index);
}

static const char *
bfd_mach_o_i386_flavour_string (unsigned int flavour)
{
  switch ((int) flavour)
    {
    case BFD_MACH_O_x86_THREAD_STATE32:    return "x86_THREAD_STATE32";
    case BFD_MACH_O_x86_FLOAT_STATE32:     return "x86_FLOAT_STATE32";
    case BFD_MACH_O_x86_EXCEPTION_STATE32: return "x86_EXCEPTION_STATE32";
    case BFD_MACH_O_x86_THREAD_STATE64:    return "x86_THREAD_STATE64";
    case BFD_MACH_O_x86_FLOAT_STATE64:     return "x86_FLOAT_STATE64";
    case BFD_MACH_O_x86_EXCEPTION_STATE64: return "x86_EXCEPTION_STATE64";
    case BFD_MACH_O_x86_THREAD_STATE:      return "x86_THREAD_STATE";
    case BFD_MACH_O_x86_FLOAT_STATE:       return "x86_FLOAT_STATE";
    case BFD_MACH_O_x86_EXCEPTION_STATE:   return "x86_EXCEPTION_STATE";
    case BFD_MACH_O_x86_DEBUG_STATE32:     return "x86_DEBUG_STATE32";
    case BFD_MACH_O_x86_DEBUG_STATE64:     return "x86_DEBUG_STATE64";
    case BFD_MACH_O_x86_DEBUG_STATE:       return "x86_DEBUG_STATE";
    case BFD_MACH_O_x86_THREAD_STATE_NONE: return "x86_THREAD_STATE_NONE";
    default: return "UNKNOWN";
    }
}

static const char *
bfd_mach_o_ppc_flavour_string (unsigned int flavour)
{
  switch ((int) flavour)
    {
    case BFD_MACH_O_PPC_THREAD_STATE:      return "PPC_THREAD_STATE";
    case BFD_MACH_O_PPC_FLOAT_STATE:       return "PPC_FLOAT_STATE";
    case BFD_MACH_O_PPC_EXCEPTION_STATE:   return "PPC_EXCEPTION_STATE";
    case BFD_MACH_O_PPC_VECTOR_STATE:      return "PPC_VECTOR_STATE";
    case BFD_MACH_O_PPC_THREAD_STATE64:    return "PPC_THREAD_STATE64";
    case BFD_MACH_O_PPC_EXCEPTION_STATE64: return "PPC_EXCEPTION_STATE64";
    default: return "UNKNOWN";
    }
}

static int
bfd_mach_o_read_dylinker (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dylinker_command *cmd = &command->command.dylinker;
  unsigned char buf[4];
  unsigned int nameoff;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_ID_DYLINKER)
	      || (command->type == BFD_MACH_O_LC_LOAD_DYLINKER));

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 4, abfd) != 4)
    return -1;

  nameoff = bfd_h_get_32 (abfd, buf + 0);

  cmd->name_offset = command->offset + nameoff;
  cmd->name_len = command->len - nameoff;
  cmd->name_str = bfd_alloc (abfd, cmd->name_len);
  if (cmd->name_str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->name_offset, SEEK_SET) != 0
      || bfd_bread (cmd->name_str, cmd->name_len, abfd) != cmd->name_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_dylib (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dylib_command *cmd = &command->command.dylib;
  unsigned char buf[16];
  unsigned int nameoff;

  switch (command->type)
    {
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB:
    case BFD_MACH_O_LC_REEXPORT_DYLIB:
      break;
    default:
      BFD_FAIL ();
      return -1;
    }

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 16, abfd) != 16)
    return -1;

  nameoff = bfd_h_get_32 (abfd, buf + 0);
  cmd->timestamp = bfd_h_get_32 (abfd, buf + 4);
  cmd->current_version = bfd_h_get_32 (abfd, buf + 8);
  cmd->compatibility_version = bfd_h_get_32 (abfd, buf + 12);

  cmd->name_offset = command->offset + nameoff;
  cmd->name_len = command->len - nameoff;
  cmd->name_str = bfd_alloc (abfd, cmd->name_len);
  if (cmd->name_str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->name_offset, SEEK_SET) != 0
      || bfd_bread (cmd->name_str, cmd->name_len, abfd) != cmd->name_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_prebound_dylib (bfd *abfd ATTRIBUTE_UNUSED,
                                bfd_mach_o_load_command *command ATTRIBUTE_UNUSED)
{
  /* bfd_mach_o_prebound_dylib_command *cmd = &command->command.prebound_dylib; */

  BFD_ASSERT (command->type == BFD_MACH_O_LC_PREBOUND_DYLIB);
  return 0;
}

static int
bfd_mach_o_read_thread (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_thread_command *cmd = &command->command.thread;
  unsigned char buf[8];
  unsigned int offset;
  unsigned int nflavours;
  unsigned int i;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_THREAD)
	      || (command->type == BFD_MACH_O_LC_UNIXTHREAD));

  /* Count the number of threads.  */
  offset = 8;
  nflavours = 0;
  while (offset != command->len)
    {
      if (offset >= command->len)
	return -1;

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bread ((void *) buf, 8, abfd) != 8)
	return -1;

      offset += 8 + bfd_h_get_32 (abfd, buf + 4) * 4;
      nflavours++;
    }

  /* Allocate threads.  */
  cmd->flavours = bfd_alloc
    (abfd, nflavours * sizeof (bfd_mach_o_thread_flavour));
  if (cmd->flavours == NULL)
    return -1;
  cmd->nflavours = nflavours;

  offset = 8;
  nflavours = 0;
  while (offset != command->len)
    {
      if (offset >= command->len)
	return -1;

      if (nflavours >= cmd->nflavours)
	return -1;

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bread ((void *) buf, 8, abfd) != 8)
	return -1;

      cmd->flavours[nflavours].flavour = bfd_h_get_32 (abfd, buf);
      cmd->flavours[nflavours].offset = command->offset + offset + 8;
      cmd->flavours[nflavours].size = bfd_h_get_32 (abfd, buf + 4) * 4;
      offset += cmd->flavours[nflavours].size + 8;
      nflavours++;
    }

  for (i = 0; i < nflavours; i++)
    {
      asection *bfdsec;
      unsigned int snamelen;
      char *sname;
      const char *flavourstr;
      const char *prefix = "LC_THREAD";
      unsigned int j = 0;

      switch (mdata->header.cputype)
	{
	case BFD_MACH_O_CPU_TYPE_POWERPC:
	case BFD_MACH_O_CPU_TYPE_POWERPC_64:
	  flavourstr = bfd_mach_o_ppc_flavour_string (cmd->flavours[i].flavour);
	  break;
	case BFD_MACH_O_CPU_TYPE_I386:
	case BFD_MACH_O_CPU_TYPE_X86_64:
	  flavourstr = bfd_mach_o_i386_flavour_string (cmd->flavours[i].flavour);
	  break;
	default:
	  flavourstr = "UNKNOWN_ARCHITECTURE";
	  break;
	}

      snamelen = strlen (prefix) + 1 + 20 + 1 + strlen (flavourstr) + 1;
      sname = bfd_alloc (abfd, snamelen);
      if (sname == NULL)
	return -1;

      for (;;)
	{
	  sprintf (sname, "%s.%s.%u", prefix, flavourstr, j);
	  if (bfd_get_section_by_name (abfd, sname) == NULL)
	    break;
	  j++;
	}

      bfdsec = bfd_make_section_with_flags (abfd, sname, SEC_HAS_CONTENTS);

      bfdsec->vma = 0;
      bfdsec->lma = 0;
      bfdsec->size = cmd->flavours[i].size;
      bfdsec->filepos = cmd->flavours[i].offset;
      bfdsec->alignment_power = 0x0;

      cmd->section = bfdsec;
    }

  return 0;
}

static int
bfd_mach_o_read_dysymtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dysymtab_command *cmd = &command->command.dysymtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned char buf[72];

  BFD_ASSERT (command->type == BFD_MACH_O_LC_DYSYMTAB);

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 72, abfd) != 72)
    return -1;

  cmd->ilocalsym = bfd_h_get_32 (abfd, buf + 0);
  cmd->nlocalsym = bfd_h_get_32 (abfd, buf + 4);
  cmd->iextdefsym = bfd_h_get_32 (abfd, buf + 8);
  cmd->nextdefsym = bfd_h_get_32 (abfd, buf + 12);
  cmd->iundefsym = bfd_h_get_32 (abfd, buf + 16);
  cmd->nundefsym = bfd_h_get_32 (abfd, buf + 20);
  cmd->tocoff = bfd_h_get_32 (abfd, buf + 24);
  cmd->ntoc = bfd_h_get_32 (abfd, buf + 28);
  cmd->modtaboff = bfd_h_get_32 (abfd, buf + 32);
  cmd->nmodtab = bfd_h_get_32 (abfd, buf + 36);
  cmd->extrefsymoff = bfd_h_get_32 (abfd, buf + 40);
  cmd->nextrefsyms = bfd_h_get_32 (abfd, buf + 44);
  cmd->indirectsymoff = bfd_h_get_32 (abfd, buf + 48);
  cmd->nindirectsyms = bfd_h_get_32 (abfd, buf + 52);
  cmd->extreloff = bfd_h_get_32 (abfd, buf + 56);
  cmd->nextrel = bfd_h_get_32 (abfd, buf + 60);
  cmd->locreloff = bfd_h_get_32 (abfd, buf + 64);
  cmd->nlocrel = bfd_h_get_32 (abfd, buf + 68);

  if (cmd->nmodtab != 0)
    {
      unsigned int i;
      int wide = bfd_mach_o_wide_p (abfd);
      unsigned int module_len = wide ? 56 : 52;

      cmd->dylib_module =
        bfd_alloc (abfd, cmd->nmodtab * sizeof (bfd_mach_o_dylib_module));
      if (cmd->dylib_module == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->modtaboff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nmodtab; i++)
        {
          bfd_mach_o_dylib_module *module = &cmd->dylib_module[i];
          unsigned long v;

          if (bfd_bread ((void *) buf, module_len, abfd) != module_len)
            return -1;

          module->module_name_idx = bfd_h_get_32 (abfd, buf + 0);
          module->iextdefsym = bfd_h_get_32 (abfd, buf + 4);
          module->nextdefsym = bfd_h_get_32 (abfd, buf + 8);
          module->irefsym = bfd_h_get_32 (abfd, buf + 12);
          module->nrefsym = bfd_h_get_32 (abfd, buf + 16);
          module->ilocalsym = bfd_h_get_32 (abfd, buf + 20);
          module->nlocalsym = bfd_h_get_32 (abfd, buf + 24);
          module->iextrel = bfd_h_get_32 (abfd, buf + 28);
          module->nextrel = bfd_h_get_32 (abfd, buf + 32);
          v = bfd_h_get_32 (abfd, buf +36);
          module->iinit = v & 0xffff;
          module->iterm = (v >> 16) & 0xffff;
          v = bfd_h_get_32 (abfd, buf + 40);
          module->ninit = v & 0xffff;
          module->nterm = (v >> 16) & 0xffff;
          if (wide)
            {
              module->objc_module_info_size = bfd_h_get_32 (abfd, buf + 44);
              module->objc_module_info_addr = bfd_h_get_64 (abfd, buf + 48);
            }
          else
            {
              module->objc_module_info_addr = bfd_h_get_32 (abfd, buf + 44);
              module->objc_module_info_size = bfd_h_get_32 (abfd, buf + 48);
            }
        }
    }
  
  if (cmd->ntoc != 0)
    {
      unsigned int i;

      cmd->dylib_toc = bfd_alloc
        (abfd, cmd->ntoc * sizeof (bfd_mach_o_dylib_table_of_content));
      if (cmd->dylib_toc == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->tocoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->ntoc; i++)
        {
          bfd_mach_o_dylib_table_of_content *toc = &cmd->dylib_toc[i];

          if (bfd_bread ((void *) buf, 8, abfd) != 8)
            return -1;

          toc->symbol_index = bfd_h_get_32 (abfd, buf + 0);
          toc->module_index = bfd_h_get_32 (abfd, buf + 4);
        }
    }

  if (cmd->nindirectsyms != 0)
    {
      unsigned int i;

      cmd->indirect_syms = bfd_alloc
        (abfd, cmd->nindirectsyms * sizeof (unsigned int));
      if (cmd->indirect_syms == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->indirectsymoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nindirectsyms; i++)
        {
          unsigned int *is = &cmd->indirect_syms[i];

          if (bfd_bread ((void *) buf, 4, abfd) != 4)
            return -1;

          *is = bfd_h_get_32 (abfd, buf + 0);
        }
    }

  if (cmd->nextrefsyms != 0)
    {
      unsigned long v;
      unsigned int i;

      cmd->ext_refs = bfd_alloc
        (abfd, cmd->nextrefsyms * sizeof (bfd_mach_o_dylib_reference));
      if (cmd->ext_refs == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->extrefsymoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nextrefsyms; i++)
        {
          bfd_mach_o_dylib_reference *ref = &cmd->ext_refs[i];

          if (bfd_bread ((void *) buf, 4, abfd) != 4)
            return -1;

          /* Fields isym and flags are written as bit-fields, thus we need
             a specific processing for endianness.  */
          v = bfd_h_get_32 (abfd, buf + 0);
          if (bfd_big_endian (abfd))
            {
              ref->isym = (v >> 8) & 0xffffff;
              ref->flags = v & 0xff;
            }
          else
            {
              ref->isym = v & 0xffffff;
              ref->flags = (v >> 24) & 0xff;
            }
        }
    }

  if (mdata->dysymtab)
    return -1;
  mdata->dysymtab = cmd;

  return 0;
}

static int
bfd_mach_o_read_symtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_symtab_command *symtab = &command->command.symtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned char buf[16];

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SYMTAB);

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 16, abfd) != 16)
    return -1;

  symtab->symoff = bfd_h_get_32 (abfd, buf);
  symtab->nsyms = bfd_h_get_32 (abfd, buf + 4);
  symtab->stroff = bfd_h_get_32 (abfd, buf + 8);
  symtab->strsize = bfd_h_get_32 (abfd, buf + 12);
  symtab->symbols = NULL;
  symtab->strtab = NULL;

  if (symtab->nsyms != 0)
    abfd->flags |= HAS_SYMS;

  if (mdata->symtab)
    return -1;
  mdata->symtab = symtab;
  return 0;
}

static int
bfd_mach_o_read_uuid (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_uuid_command *cmd = &command->command.uuid;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_UUID);

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) cmd->uuid, 16, abfd) != 16)
    return -1;

  return 0;
}

static int
bfd_mach_o_read_linkedit (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_linkedit_command *cmd = &command->command.linkedit;
  char buf[8];

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 8, abfd) != 8)
    return -1;

  cmd->dataoff = bfd_get_32 (abfd, buf + 0);
  cmd->datasize = bfd_get_32 (abfd, buf + 4);
  return 0;
}

static int
bfd_mach_o_read_str (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_str_command *cmd = &command->command.str;
  char buf[4];
  unsigned long off;

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 4, abfd) != 4)
    return -1;

  off = bfd_get_32 (abfd, buf + 0);
  cmd->stroff = command->offset + off;
  cmd->str_len = command->len - off;
  cmd->str = bfd_alloc (abfd, cmd->str_len);
  if (cmd->str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->stroff, SEEK_SET) != 0
      || bfd_bread ((void *) cmd->str, cmd->str_len, abfd) != cmd->str_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_dyld_info (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dyld_info_command *cmd = &command->command.dyld_info;
  char buf[40];

  if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
      || bfd_bread ((void *) buf, sizeof (buf), abfd) != sizeof (buf))
    return -1;

  cmd->rebase_off = bfd_get_32 (abfd, buf + 0);
  cmd->rebase_size = bfd_get_32 (abfd, buf + 4);
  cmd->bind_off = bfd_get_32 (abfd, buf + 8);
  cmd->bind_size = bfd_get_32 (abfd, buf + 12);
  cmd->weak_bind_off = bfd_get_32 (abfd, buf + 16);
  cmd->weak_bind_size = bfd_get_32 (abfd, buf + 20);
  cmd->lazy_bind_off = bfd_get_32 (abfd, buf + 24);
  cmd->lazy_bind_size = bfd_get_32 (abfd, buf + 28);
  cmd->export_off = bfd_get_32 (abfd, buf + 32);
  cmd->export_size = bfd_get_32 (abfd, buf + 36);
  return 0;
}

static int
bfd_mach_o_read_segment (bfd *abfd,
                         bfd_mach_o_load_command *command,
                         unsigned int wide)
{
  unsigned char buf[64];
  bfd_mach_o_segment_command *seg = &command->command.segment;
  unsigned long i;

  if (wide)
    {
      BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT_64);

      if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
          || bfd_bread ((void *) buf, 64, abfd) != 64)
	return -1;

      memcpy (seg->segname, buf, 16);
      seg->segname[16] = '\0';

      seg->vmaddr = bfd_h_get_64 (abfd, buf + 16);
      seg->vmsize = bfd_h_get_64 (abfd, buf + 24);
      seg->fileoff = bfd_h_get_64 (abfd, buf + 32);
      seg->filesize = bfd_h_get_64 (abfd, buf + 40);
      seg->maxprot = bfd_h_get_32 (abfd, buf + 48);
      seg->initprot = bfd_h_get_32 (abfd, buf + 52);
      seg->nsects = bfd_h_get_32 (abfd, buf + 56);
      seg->flags = bfd_h_get_32 (abfd, buf + 60);
    }
  else
    {
      BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT);

      if (bfd_seek (abfd, command->offset + 8, SEEK_SET) != 0
          || bfd_bread ((void *) buf, 48, abfd) != 48)
	return -1;

      memcpy (seg->segname, buf, 16);
      seg->segname[16] = '\0';

      seg->vmaddr = bfd_h_get_32 (abfd, buf + 16);
      seg->vmsize = bfd_h_get_32 (abfd, buf + 20);
      seg->fileoff = bfd_h_get_32 (abfd, buf + 24);
      seg->filesize = bfd_h_get_32 (abfd, buf +  28);
      seg->maxprot = bfd_h_get_32 (abfd, buf + 32);
      seg->initprot = bfd_h_get_32 (abfd, buf + 36);
      seg->nsects = bfd_h_get_32 (abfd, buf + 40);
      seg->flags = bfd_h_get_32 (abfd, buf + 44);
    }

  if (seg->nsects != 0)
    {
      seg->sections = bfd_alloc (abfd, seg->nsects
                                 * sizeof (bfd_mach_o_section));
      if (seg->sections == NULL)
	return -1;

      for (i = 0; i < seg->nsects; i++)
	{
	  bfd_vma segoff;
          if (wide)
            segoff = command->offset + BFD_MACH_O_LC_SEGMENT_64_SIZE
              + (i * BFD_MACH_O_SECTION_64_SIZE);
          else
            segoff = command->offset + BFD_MACH_O_LC_SEGMENT_SIZE
              + (i * BFD_MACH_O_SECTION_SIZE);

	  if (bfd_mach_o_read_section
	      (abfd, &seg->sections[i], segoff, seg->initprot, wide) != 0)
	    return -1;
	}
    }

  return 0;
}

static int
bfd_mach_o_read_segment_32 (bfd *abfd, bfd_mach_o_load_command *command)
{
  return bfd_mach_o_read_segment (abfd, command, 0);
}

static int
bfd_mach_o_read_segment_64 (bfd *abfd, bfd_mach_o_load_command *command)
{
  return bfd_mach_o_read_segment (abfd, command, 1);
}

static int
bfd_mach_o_read_command (bfd *abfd, bfd_mach_o_load_command *command)
{
  unsigned char buf[8];

  /* Read command type and length.  */
  if (bfd_seek (abfd, command->offset, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 8, abfd) != 8)
    return -1;

  command->type = bfd_h_get_32 (abfd, buf) & ~BFD_MACH_O_LC_REQ_DYLD;
  command->type_required = (bfd_h_get_32 (abfd, buf) & BFD_MACH_O_LC_REQ_DYLD
			    ? TRUE : FALSE);
  command->len = bfd_h_get_32 (abfd, buf + 4);

  switch (command->type)
    {
    case BFD_MACH_O_LC_SEGMENT:
      if (bfd_mach_o_read_segment_32 (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SEGMENT_64:
      if (bfd_mach_o_read_segment_64 (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SYMTAB:
      if (bfd_mach_o_read_symtab (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SYMSEG:
      break;
    case BFD_MACH_O_LC_THREAD:
    case BFD_MACH_O_LC_UNIXTHREAD:
      if (bfd_mach_o_read_thread (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOAD_DYLINKER:
    case BFD_MACH_O_LC_ID_DYLINKER:
      if (bfd_mach_o_read_dylinker (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB:
    case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
    case BFD_MACH_O_LC_REEXPORT_DYLIB:
      if (bfd_mach_o_read_dylib (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_PREBOUND_DYLIB:
      if (bfd_mach_o_read_prebound_dylib (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOADFVMLIB:
    case BFD_MACH_O_LC_IDFVMLIB:
    case BFD_MACH_O_LC_IDENT:
    case BFD_MACH_O_LC_FVMFILE:
    case BFD_MACH_O_LC_PREPAGE:
    case BFD_MACH_O_LC_ROUTINES:
      break;
    case BFD_MACH_O_LC_SUB_FRAMEWORK:
    case BFD_MACH_O_LC_SUB_UMBRELLA:
    case BFD_MACH_O_LC_SUB_LIBRARY:
    case BFD_MACH_O_LC_SUB_CLIENT:
    case BFD_MACH_O_LC_RPATH:
      if (bfd_mach_o_read_str (abfd, command) != 0)
        return -1;
      break;
    case BFD_MACH_O_LC_DYSYMTAB:
      if (bfd_mach_o_read_dysymtab (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_TWOLEVEL_HINTS:
    case BFD_MACH_O_LC_PREBIND_CKSUM:
      break;
    case BFD_MACH_O_LC_UUID:
      if (bfd_mach_o_read_uuid (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_CODE_SIGNATURE:
    case BFD_MACH_O_LC_SEGMENT_SPLIT_INFO:
      if (bfd_mach_o_read_linkedit (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_DYLD_INFO:
      if (bfd_mach_o_read_dyld_info (abfd, command) != 0)
	return -1;
      break;
    default:
      (*_bfd_error_handler) (_("unable to read unknown load command 0x%lx"),
			     (unsigned long) command->type);
      break;
    }

  return 0;
}

static void
bfd_mach_o_flatten_sections (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  long csect = 0;
  unsigned long i, j;

  /* Count total number of sections.  */
  mdata->nsects = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      if (mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT
	  || mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT_64)
	{
	  bfd_mach_o_segment_command *seg;

	  seg = &mdata->commands[i].command.segment;
	  mdata->nsects += seg->nsects;
	}
    }

  /* Allocate sections array.  */
  mdata->sections = bfd_alloc (abfd,
			       mdata->nsects * sizeof (bfd_mach_o_section *));

  /* Fill the array.  */
  csect = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      if (mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT
	  || mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT_64)
	{
	  bfd_mach_o_segment_command *seg;

	  seg = &mdata->commands[i].command.segment;
	  BFD_ASSERT (csect + seg->nsects <= mdata->nsects);

	  for (j = 0; j < seg->nsects; j++)
	    mdata->sections[csect++] = &seg->sections[j];
	}
    }
}

int
bfd_mach_o_scan_start_address (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_thread_command *cmd = NULL;
  unsigned long i;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      if ((mdata->commands[i].type == BFD_MACH_O_LC_THREAD) ||
	  (mdata->commands[i].type == BFD_MACH_O_LC_UNIXTHREAD))
	{
	  if (cmd == NULL)
	    cmd = &mdata->commands[i].command.thread;
	  else
	    return 0;
	}
    }

  if (cmd == NULL)
    return 0;

  for (i = 0; i < cmd->nflavours; i++)
    {
      if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_I386)
	  && (cmd->flavours[i].flavour
	      == (unsigned long) BFD_MACH_O_x86_THREAD_STATE32))
	{
	  unsigned char buf[4];

	  if (bfd_seek (abfd, cmd->flavours[i].offset + 40, SEEK_SET) != 0
              || bfd_bread (buf, 4, abfd) != 4)
	    return -1;

	  abfd->start_address = bfd_h_get_32 (abfd, buf);
	}
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_POWERPC)
	       && (cmd->flavours[i].flavour == BFD_MACH_O_PPC_THREAD_STATE))
	{
	  unsigned char buf[4];

	  if (bfd_seek (abfd, cmd->flavours[i].offset + 0, SEEK_SET) != 0
              || bfd_bread (buf, 4, abfd) != 4)
	    return -1;

	  abfd->start_address = bfd_h_get_32 (abfd, buf);
	}
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_POWERPC_64)
               && (cmd->flavours[i].flavour == BFD_MACH_O_PPC_THREAD_STATE64))
        {
          unsigned char buf[8];

          if (bfd_seek (abfd, cmd->flavours[i].offset + 0, SEEK_SET) != 0
              || bfd_bread (buf, 8, abfd) != 8)
            return -1;

          abfd->start_address = bfd_h_get_64 (abfd, buf);
        }
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_X86_64)
               && (cmd->flavours[i].flavour == BFD_MACH_O_x86_THREAD_STATE64))
        {
          unsigned char buf[8];

          if (bfd_seek (abfd, cmd->flavours[i].offset + (16 * 8), SEEK_SET) != 0
              || bfd_bread (buf, 8, abfd) != 8)
            return -1;

          abfd->start_address = bfd_h_get_64 (abfd, buf);
        }
    }

  return 0;
}

bfd_boolean
bfd_mach_o_set_arch_mach (bfd *abfd,
                          enum bfd_architecture arch,
                          unsigned long machine)
{
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  /* If this isn't the right architecture for this backend, and this
     isn't the generic backend, fail.  */
  if (arch != bed->arch
      && arch != bfd_arch_unknown
      && bed->arch != bfd_arch_unknown)
    return FALSE;

  return bfd_default_set_arch_mach (abfd, arch, machine);
}

int
bfd_mach_o_scan (bfd *abfd,
		 bfd_mach_o_header *header,
		 bfd_mach_o_data_struct *mdata)
{
  unsigned int i;
  enum bfd_architecture cputype;
  unsigned long cpusubtype;
  unsigned int hdrsize;

  hdrsize = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  mdata->header = *header;

  abfd->flags = abfd->flags & BFD_IN_MEMORY;
  switch (header->filetype)
    {
    case BFD_MACH_O_MH_OBJECT:
      abfd->flags |= HAS_RELOC;
      break;
    case BFD_MACH_O_MH_EXECUTE:
      abfd->flags |= EXEC_P;
      break;
    case BFD_MACH_O_MH_DYLIB:
    case BFD_MACH_O_MH_BUNDLE:
      abfd->flags |= DYNAMIC;
      break;
    }

  abfd->tdata.mach_o_data = mdata;

  bfd_mach_o_convert_architecture (header->cputype, header->cpusubtype,
				   &cputype, &cpusubtype);
  if (cputype == bfd_arch_unknown)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_scan: unknown architecture 0x%lx/0x%lx"),
			     header->cputype, header->cpusubtype);
      return -1;
    }

  bfd_set_arch_mach (abfd, cputype, cpusubtype);

  if (header->ncmds != 0)
    {
      mdata->commands = bfd_alloc
        (abfd, header->ncmds * sizeof (bfd_mach_o_load_command));
      if (mdata->commands == NULL)
	return -1;

      for (i = 0; i < header->ncmds; i++)
	{
	  bfd_mach_o_load_command *cur = &mdata->commands[i];

	  if (i == 0)
	    cur->offset = hdrsize;
	  else
	    {
	      bfd_mach_o_load_command *prev = &mdata->commands[i - 1];
	      cur->offset = prev->offset + prev->len;
	    }

	  if (bfd_mach_o_read_command (abfd, cur) < 0)
	    return -1;
	}
    }

  if (bfd_mach_o_scan_start_address (abfd) < 0)
    return -1;

  bfd_mach_o_flatten_sections (abfd);
  return 0;
}

bfd_boolean
bfd_mach_o_mkobject_init (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = NULL;

  mdata = bfd_alloc (abfd, sizeof (bfd_mach_o_data_struct));
  if (mdata == NULL)
    return FALSE;
  abfd->tdata.mach_o_data = mdata;

  mdata->header.magic = 0;
  mdata->header.cputype = 0;
  mdata->header.cpusubtype = 0;
  mdata->header.filetype = 0;
  mdata->header.ncmds = 0;
  mdata->header.sizeofcmds = 0;
  mdata->header.flags = 0;
  mdata->header.byteorder = BFD_ENDIAN_UNKNOWN;
  mdata->commands = NULL;
  mdata->nsects = 0;
  mdata->sections = NULL;

  return TRUE;
}

static bfd_boolean
bfd_mach_o_gen_mkobject (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_mkobject_init (abfd))
    return FALSE;

  mdata = bfd_mach_o_get_data (abfd);
  mdata->header.magic = BFD_MACH_O_MH_MAGIC;
  mdata->header.cputype = 0;
  mdata->header.cpusubtype = 0;
  mdata->header.byteorder = abfd->xvec->byteorder;
  mdata->header.version = 1;

  return TRUE;
}

const bfd_target *
bfd_mach_o_header_p (bfd *abfd,
                     bfd_mach_o_filetype filetype,
                     bfd_mach_o_cpu_type cputype)
{
  struct bfd_preserve preserve;
  bfd_mach_o_header header;

  preserve.marker = NULL;
  if (!bfd_mach_o_read_header (abfd, &header))
    goto wrong;

  if (! (header.byteorder == BFD_ENDIAN_BIG
	 || header.byteorder == BFD_ENDIAN_LITTLE))
    {
      (*_bfd_error_handler) (_("unknown header byte-order value 0x%lx"),
			     (unsigned long) header.byteorder);
      goto wrong;
    }

  if (! ((header.byteorder == BFD_ENDIAN_BIG
	  && abfd->xvec->byteorder == BFD_ENDIAN_BIG
	  && abfd->xvec->header_byteorder == BFD_ENDIAN_BIG)
	 || (header.byteorder == BFD_ENDIAN_LITTLE
	     && abfd->xvec->byteorder == BFD_ENDIAN_LITTLE
	     && abfd->xvec->header_byteorder == BFD_ENDIAN_LITTLE)))
    goto wrong;

  /* Check cputype and filetype.
     In case of wildcard, do not accept magics that are handled by existing
     targets.  */
  if (cputype)
    {
      if (header.cputype != cputype)
        goto wrong;
    }
  else
    {
      switch (header.cputype)
        {
        case BFD_MACH_O_CPU_TYPE_I386:
          /* Handled by mach-o-i386 */
          goto wrong;
        default:
          break;
        }
    }
  if (filetype)
    {
      if (header.filetype != filetype)
        goto wrong;
    }
  else
    {
      switch (header.filetype)
        {
        case BFD_MACH_O_MH_CORE:
          /* Handled by core_p */
          goto wrong;
        default:
          break;
        }
    }

  preserve.marker = bfd_zalloc (abfd, sizeof (bfd_mach_o_data_struct));
  if (preserve.marker == NULL
      || !bfd_preserve_save (abfd, &preserve))
    goto fail;

  if (bfd_mach_o_scan (abfd, &header,
		       (bfd_mach_o_data_struct *) preserve.marker) != 0)
    goto wrong;

  bfd_preserve_finish (abfd, &preserve);
  return abfd->xvec;

 wrong:
  bfd_set_error (bfd_error_wrong_format);

 fail:
  if (preserve.marker != NULL)
    bfd_preserve_restore (abfd, &preserve);
  return NULL;
}

static const bfd_target *
bfd_mach_o_gen_object_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0, 0);
}

static const bfd_target *
bfd_mach_o_gen_core_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, BFD_MACH_O_MH_CORE, 0);
}

typedef struct mach_o_fat_archentry
{
  unsigned long cputype;
  unsigned long cpusubtype;
  unsigned long offset;
  unsigned long size;
  unsigned long align;
} mach_o_fat_archentry;

typedef struct mach_o_fat_data_struct
{
  unsigned long magic;
  unsigned long nfat_arch;
  mach_o_fat_archentry *archentries;
} mach_o_fat_data_struct;

const bfd_target *
bfd_mach_o_archive_p (bfd *abfd)
{
  mach_o_fat_data_struct *adata = NULL;
  unsigned char buf[20];
  unsigned long i;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread ((void *) buf, 8, abfd) != 8)
    goto error;

  adata = bfd_alloc (abfd, sizeof (mach_o_fat_data_struct));
  if (adata == NULL)
    goto error;

  adata->magic = bfd_getb32 (buf);
  adata->nfat_arch = bfd_getb32 (buf + 4);
  if (adata->magic != 0xcafebabe)
    goto error;
  /* Avoid matching Java bytecode files, which have the same magic number.
     In the Java bytecode file format this field contains the JVM version,
     which starts at 43.0.  */
  if (adata->nfat_arch > 30)
    goto error;

  adata->archentries =
    bfd_alloc (abfd, adata->nfat_arch * sizeof (mach_o_fat_archentry));
  if (adata->archentries == NULL)
    goto error;

  for (i = 0; i < adata->nfat_arch; i++)
    {
      if (bfd_seek (abfd, 8 + 20 * i, SEEK_SET) != 0
          || bfd_bread ((void *) buf, 20, abfd) != 20)
	goto error;
      adata->archentries[i].cputype = bfd_getb32 (buf);
      adata->archentries[i].cpusubtype = bfd_getb32 (buf + 4);
      adata->archentries[i].offset = bfd_getb32 (buf + 8);
      adata->archentries[i].size = bfd_getb32 (buf + 12);
      adata->archentries[i].align = bfd_getb32 (buf + 16);
    }

  abfd->tdata.mach_o_fat_data = adata;
  return abfd->xvec;

 error:
  if (adata != NULL)
    bfd_release (abfd, adata);
  bfd_set_error (bfd_error_wrong_format);
  return NULL;
}

bfd *
bfd_mach_o_openr_next_archived_file (bfd *archive, bfd *prev)
{
  mach_o_fat_data_struct *adata;
  mach_o_fat_archentry *entry = NULL;
  unsigned long i;
  bfd *nbfd;
  enum bfd_architecture arch_type;
  unsigned long arch_subtype;

  adata = (mach_o_fat_data_struct *) archive->tdata.mach_o_fat_data;
  BFD_ASSERT (adata != NULL);

  /* Find index of previous entry.  */
  if (prev == NULL)
    i = 0;	/* Start at first one.  */
  else
    {
      for (i = 0; i < adata->nfat_arch; i++)
	{
	  if (adata->archentries[i].offset == prev->origin)
	    break;
	}

      if (i == adata->nfat_arch)
	{
	  /* Not found.  */
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}
    i++;	/* Get next entry.  */
  }

  if (i >= adata->nfat_arch)
    {
      bfd_set_error (bfd_error_no_more_archived_files);
      return NULL;
    }

  entry = &adata->archentries[i];
  nbfd = _bfd_new_bfd_contained_in (archive);
  if (nbfd == NULL)
    return NULL;

  nbfd->origin = entry->offset;

  bfd_mach_o_convert_architecture (entry->cputype, entry->cpusubtype,
				   &arch_type, &arch_subtype);
  /* Create the member filename.
     Use FILENAME:ARCH_NAME.  */
  {
    char *s = NULL;
    const char *arch_name;
    size_t arch_file_len = strlen (bfd_get_filename (archive));

    arch_name = bfd_printable_arch_mach (arch_type, arch_subtype);
    s = bfd_malloc (arch_file_len + 1 + strlen (arch_name) + 1);
    if (s == NULL)
      return NULL;
    memcpy (s, bfd_get_filename (archive), arch_file_len);
    s[arch_file_len] = ':';
    strcpy (s + arch_file_len + 1, arch_name);
    nbfd->filename = s;
  }
  nbfd->iostream = NULL;
  bfd_set_arch_mach (nbfd, arch_type, arch_subtype);

  return nbfd;
}

/* If ABFD format is FORMAT and architecture is ARCH, return it.
   If ABFD is a fat image containing a member that corresponds to FORMAT
   and ARCH, returns it.
   In other case, returns NULL.
   This function allows transparent uses of fat images.  */
bfd *
bfd_mach_o_fat_extract (bfd *abfd,
			bfd_format format,
			const bfd_arch_info_type *arch)
{
  bfd *res;
  mach_o_fat_data_struct *adata;
  unsigned int i;

  if (bfd_check_format (abfd, format))
    {
      if (bfd_get_arch_info (abfd) == arch)
	return abfd;
      return NULL;
    }
  if (!bfd_check_format (abfd, bfd_archive)
      || abfd->xvec != &mach_o_fat_vec)
    return NULL;

  /* This is a Mach-O fat image.  */
  adata = (mach_o_fat_data_struct *) abfd->tdata.mach_o_fat_data;
  BFD_ASSERT (adata != NULL);

  for (i = 0; i < adata->nfat_arch; i++)
    {
      struct mach_o_fat_archentry *e = &adata->archentries[i];
      enum bfd_architecture cpu_type;
      unsigned long cpu_subtype;

      bfd_mach_o_convert_architecture (e->cputype, e->cpusubtype,
				       &cpu_type, &cpu_subtype);
      if (cpu_type != arch->arch || cpu_subtype != arch->mach)
	continue;

      /* The architecture is found.  */
      res = _bfd_new_bfd_contained_in (abfd);
      if (res == NULL)
	return NULL;

      res->origin = e->offset;

      res->filename = strdup (abfd->filename);
      res->iostream = NULL;

      if (bfd_check_format (res, format))
	{
	  BFD_ASSERT (bfd_get_arch_info (res) == arch);
	  return res;
	}
      bfd_close (res);
      return NULL;
    }

  return NULL;
}

int
bfd_mach_o_lookup_section (bfd *abfd,
			   asection *section,
			   bfd_mach_o_load_command **mcommand,
			   bfd_mach_o_section **msection)
{
  struct mach_o_data_struct *md = bfd_mach_o_get_data (abfd);
  unsigned int i, j, num;

  bfd_mach_o_load_command *ncmd = NULL;
  bfd_mach_o_section *nsect = NULL;

  BFD_ASSERT (mcommand != NULL);
  BFD_ASSERT (msection != NULL);

  num = 0;
  for (i = 0; i < md->header.ncmds; i++)
    {
      struct bfd_mach_o_load_command *cmd = &md->commands[i];
      struct bfd_mach_o_segment_command *seg = NULL;

      if (cmd->type != BFD_MACH_O_LC_SEGMENT
	  || cmd->type != BFD_MACH_O_LC_SEGMENT_64)
	continue;
      seg = &cmd->command.segment;

      for (j = 0; j < seg->nsects; j++)
	{
	  struct bfd_mach_o_section *sect = &seg->sections[j];

	  if (sect->bfdsection == section)
	    {
	      if (num == 0)
                {
                  nsect = sect;
                  ncmd = cmd;
                }
	      num++;
	    }
	}
    }

  *mcommand = ncmd;
  *msection = nsect;
  return num;
}

int
bfd_mach_o_lookup_command (bfd *abfd,
			   bfd_mach_o_load_command_type type,
			   bfd_mach_o_load_command **mcommand)
{
  struct mach_o_data_struct *md = bfd_mach_o_get_data (abfd);
  bfd_mach_o_load_command *ncmd = NULL;
  unsigned int i, num;

  BFD_ASSERT (md != NULL);
  BFD_ASSERT (mcommand != NULL);

  num = 0;
  for (i = 0; i < md->header.ncmds; i++)
    {
      struct bfd_mach_o_load_command *cmd = &md->commands[i];

      if (cmd->type != type)
	continue;

      if (num == 0)
	ncmd = cmd;
      num++;
    }

  *mcommand = ncmd;
  return num;
}

unsigned long
bfd_mach_o_stack_addr (enum bfd_mach_o_cpu_type type)
{
  switch (type)
    {
    case BFD_MACH_O_CPU_TYPE_MC680x0:
      return 0x04000000;
    case BFD_MACH_O_CPU_TYPE_MC88000:
      return 0xffffe000;
    case BFD_MACH_O_CPU_TYPE_POWERPC:
      return 0xc0000000;
    case BFD_MACH_O_CPU_TYPE_I386:
      return 0xc0000000;
    case BFD_MACH_O_CPU_TYPE_SPARC:
      return 0xf0000000;
    case BFD_MACH_O_CPU_TYPE_I860:
      return 0;
    case BFD_MACH_O_CPU_TYPE_HPPA:
      return 0xc0000000 - 0x04000000;
    default:
      return 0;
    }
}

typedef struct bfd_mach_o_xlat_name
{
  const char *name;
  unsigned long val;
}
bfd_mach_o_xlat_name;

static void
bfd_mach_o_print_flags (const bfd_mach_o_xlat_name *table,
                        unsigned long val,
                        FILE *file)
{
  int first = 1;

  for (; table->name; table++)
    {
      if (table->val & val)
        {
          if (!first)
            fprintf (file, "+");
          fprintf (file, "%s", table->name);
          val &= ~table->val;
          first = 0;
        }
    }
  if (val)
    {
      if (!first)
        fprintf (file, "+");
      fprintf (file, "0x%lx", val);
      return;
    }
  if (first)
    fprintf (file, "-");
}

static const char *
bfd_mach_o_get_name (const bfd_mach_o_xlat_name *table, unsigned long val)
{
  for (; table->name; table++)
    if (table->val == val)
      return table->name;
  return "*UNKNOWN*";
}

static bfd_mach_o_xlat_name bfd_mach_o_cpu_name[] =
{
  { "vax", BFD_MACH_O_CPU_TYPE_VAX },
  { "mc680x0", BFD_MACH_O_CPU_TYPE_MC680x0 },
  { "i386", BFD_MACH_O_CPU_TYPE_I386 },
  { "mips", BFD_MACH_O_CPU_TYPE_MIPS },
  { "mc98000", BFD_MACH_O_CPU_TYPE_MC98000 },
  { "hppa", BFD_MACH_O_CPU_TYPE_HPPA },
  { "arm", BFD_MACH_O_CPU_TYPE_ARM },
  { "mc88000", BFD_MACH_O_CPU_TYPE_MC88000 },
  { "sparc", BFD_MACH_O_CPU_TYPE_SPARC },
  { "i860", BFD_MACH_O_CPU_TYPE_I860 },
  { "alpha", BFD_MACH_O_CPU_TYPE_ALPHA },
  { "powerpc", BFD_MACH_O_CPU_TYPE_POWERPC },
  { "powerpc_64", BFD_MACH_O_CPU_TYPE_POWERPC_64 },
  { "x86_64", BFD_MACH_O_CPU_TYPE_X86_64 },
  { NULL, 0}
};

static bfd_mach_o_xlat_name bfd_mach_o_filetype_name[] = 
{
  { "object", BFD_MACH_O_MH_OBJECT },
  { "execute", BFD_MACH_O_MH_EXECUTE },
  { "fvmlib", BFD_MACH_O_MH_FVMLIB },
  { "core", BFD_MACH_O_MH_CORE },
  { "preload", BFD_MACH_O_MH_PRELOAD },
  { "dylib", BFD_MACH_O_MH_DYLIB },
  { "dylinker", BFD_MACH_O_MH_DYLINKER },
  { "bundle", BFD_MACH_O_MH_BUNDLE },
  { "dylib_stub", BFD_MACH_O_MH_DYLIB_STUB },
  { "dym", BFD_MACH_O_MH_DSYM },
  { "kext_bundle", BFD_MACH_O_MH_KEXT_BUNDLE },
  { NULL, 0}
};

static bfd_mach_o_xlat_name bfd_mach_o_header_flags_name[] = 
{
  { "noundefs", BFD_MACH_O_MH_NOUNDEFS },
  { "incrlink", BFD_MACH_O_MH_INCRLINK },
  { "dyldlink", BFD_MACH_O_MH_DYLDLINK },
  { "bindatload", BFD_MACH_O_MH_BINDATLOAD },
  { "prebound", BFD_MACH_O_MH_PREBOUND },
  { "split_segs", BFD_MACH_O_MH_SPLIT_SEGS },
  { "lazy_init", BFD_MACH_O_MH_LAZY_INIT },
  { "twolevel", BFD_MACH_O_MH_TWOLEVEL },
  { "force_flat", BFD_MACH_O_MH_FORCE_FLAT },
  { "nomultidefs", BFD_MACH_O_MH_NOMULTIDEFS },
  { "nofixprebinding", BFD_MACH_O_MH_NOFIXPREBINDING },
  { "prebindable", BFD_MACH_O_MH_PREBINDABLE },
  { "allmodsbound", BFD_MACH_O_MH_ALLMODSBOUND },
  { "subsections_via_symbols", BFD_MACH_O_MH_SUBSECTIONS_VIA_SYMBOLS },
  { "canonical", BFD_MACH_O_MH_CANONICAL },
  { "weak_defines", BFD_MACH_O_MH_WEAK_DEFINES },
  { "binds_to_weak", BFD_MACH_O_MH_BINDS_TO_WEAK },
  { "allow_stack_execution", BFD_MACH_O_MH_ALLOW_STACK_EXECUTION },
  { "root_safe", BFD_MACH_O_MH_ROOT_SAFE },
  { "setuid_safe", BFD_MACH_O_MH_SETUID_SAFE },
  { "no_reexported_dylibs", BFD_MACH_O_MH_NO_REEXPORTED_DYLIBS },
  { "pie", BFD_MACH_O_MH_PIE },
  { NULL, 0}
};

static bfd_mach_o_xlat_name bfd_mach_o_section_type_name[] = 
{
  { "regular", BFD_MACH_O_S_REGULAR},
  { "zerofill", BFD_MACH_O_S_ZEROFILL},
  { "cstring_literals", BFD_MACH_O_S_CSTRING_LITERALS},
  { "4byte_literals", BFD_MACH_O_S_4BYTE_LITERALS},
  { "8byte_literals", BFD_MACH_O_S_8BYTE_LITERALS},
  { "literal_pointers", BFD_MACH_O_S_LITERAL_POINTERS},
  { "non_lazy_symbol_pointers", BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS},
  { "lazy_symbol_pointers", BFD_MACH_O_S_LAZY_SYMBOL_POINTERS},
  { "symbol_stubs", BFD_MACH_O_S_SYMBOL_STUBS},
  { "mod_init_func_pointers", BFD_MACH_O_S_MOD_INIT_FUNC_POINTERS},
  { "mod_fini_func_pointers", BFD_MACH_O_S_MOD_FINI_FUNC_POINTERS},
  { "coalesced", BFD_MACH_O_S_COALESCED},
  { "gb_zerofill", BFD_MACH_O_S_GB_ZEROFILL},
  { "interposing", BFD_MACH_O_S_INTERPOSING},
  { "16byte_literals", BFD_MACH_O_S_16BYTE_LITERALS},
  { "dtrace_dof", BFD_MACH_O_S_DTRACE_DOF},
  { "lazy_dylib_symbol_pointers", BFD_MACH_O_S_LAZY_DYLIB_SYMBOL_POINTERS},
  { NULL, 0}
};

static bfd_mach_o_xlat_name bfd_mach_o_section_attribute_name[] = 
{
  { "loc_reloc", BFD_MACH_O_S_ATTR_LOC_RELOC },
  { "ext_reloc", BFD_MACH_O_S_ATTR_EXT_RELOC },
  { "some_instructions", BFD_MACH_O_S_ATTR_SOME_INSTRUCTIONS },
  { "debug", BFD_MACH_O_S_ATTR_DEBUG },
  { "modifying_code", BFD_MACH_O_S_SELF_MODIFYING_CODE },
  { "live_support", BFD_MACH_O_S_ATTR_LIVE_SUPPORT },
  { "no_dead_strip", BFD_MACH_O_S_ATTR_NO_DEAD_STRIP },
  { "strip_static_syms", BFD_MACH_O_S_ATTR_STRIP_STATIC_SYMS },
  { "no_toc", BFD_MACH_O_S_ATTR_NO_TOC },
  { "pure_instructions", BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS },
  { NULL, 0}
};

static bfd_mach_o_xlat_name bfd_mach_o_load_command_name[] = 
{
  { "segment", BFD_MACH_O_LC_SEGMENT},
  { "symtab", BFD_MACH_O_LC_SYMTAB},
  { "symseg", BFD_MACH_O_LC_SYMSEG},
  { "thread", BFD_MACH_O_LC_THREAD},
  { "unixthread", BFD_MACH_O_LC_UNIXTHREAD},
  { "loadfvmlib", BFD_MACH_O_LC_LOADFVMLIB},
  { "idfvmlib", BFD_MACH_O_LC_IDFVMLIB},
  { "ident", BFD_MACH_O_LC_IDENT},
  { "fvmfile", BFD_MACH_O_LC_FVMFILE},
  { "prepage", BFD_MACH_O_LC_PREPAGE},
  { "dysymtab", BFD_MACH_O_LC_DYSYMTAB},
  { "load_dylib", BFD_MACH_O_LC_LOAD_DYLIB},
  { "id_dylib", BFD_MACH_O_LC_ID_DYLIB},
  { "load_dylinker", BFD_MACH_O_LC_LOAD_DYLINKER},
  { "id_dylinker", BFD_MACH_O_LC_ID_DYLINKER},
  { "prebound_dylib", BFD_MACH_O_LC_PREBOUND_DYLIB},
  { "routines", BFD_MACH_O_LC_ROUTINES},
  { "sub_framework", BFD_MACH_O_LC_SUB_FRAMEWORK},
  { "sub_umbrella", BFD_MACH_O_LC_SUB_UMBRELLA},
  { "sub_client", BFD_MACH_O_LC_SUB_CLIENT},
  { "sub_library", BFD_MACH_O_LC_SUB_LIBRARY},
  { "twolevel_hints", BFD_MACH_O_LC_TWOLEVEL_HINTS},
  { "prebind_cksum", BFD_MACH_O_LC_PREBIND_CKSUM},
  { "load_weak_dylib", BFD_MACH_O_LC_LOAD_WEAK_DYLIB},
  { "segment_64", BFD_MACH_O_LC_SEGMENT_64},
  { "routines_64", BFD_MACH_O_LC_ROUTINES_64},
  { "uuid", BFD_MACH_O_LC_UUID},
  { "rpath", BFD_MACH_O_LC_RPATH},
  { "code_signature", BFD_MACH_O_LC_CODE_SIGNATURE},
  { "segment_split_info", BFD_MACH_O_LC_SEGMENT_SPLIT_INFO},
  { "reexport_dylib", BFD_MACH_O_LC_REEXPORT_DYLIB},
  { "lazy_load_dylib", BFD_MACH_O_LC_LAZY_LOAD_DYLIB},
  { "encryption_info", BFD_MACH_O_LC_ENCRYPTION_INFO},
  { "dyld_info", BFD_MACH_O_LC_DYLD_INFO},
  { NULL, 0}
};

static void
bfd_mach_o_print_private_header (bfd *abfd, FILE *file)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_header *h = &mdata->header;

  fputs (_("Mach-O header:\n"), file);
  fprintf (file, _(" magic     : %08lx\n"), h->magic);
  fprintf (file, _(" cputype   : %08lx (%s)\n"), h->cputype,
           bfd_mach_o_get_name (bfd_mach_o_cpu_name, h->cputype));
  fprintf (file, _(" cpusubtype: %08lx\n"), h->cpusubtype);
  fprintf (file, _(" filetype  : %08lx (%s)\n"),
           h->filetype,
           bfd_mach_o_get_name (bfd_mach_o_filetype_name, h->filetype));
  fprintf (file, _(" ncmds     : %08lx (%lu)\n"), h->ncmds, h->ncmds);
  fprintf (file, _(" sizeofcmds: %08lx\n"), h->sizeofcmds);
  fprintf (file, _(" flags     : %08lx ("), h->flags);
  bfd_mach_o_print_flags (bfd_mach_o_header_flags_name, h->flags, file);
  fputs (_(")\n"), file);
  fprintf (file, _(" reserved  : %08x\n"), h->reserved);
}

static void
bfd_mach_o_print_section_map (bfd *abfd, FILE *file)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i, j;
  unsigned int sec_nbr = 0;

  fputs (_("Segments and Sections:\n"), file);
  fputs (_(" #: Segment name     Section name     Address\n"), file);

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_segment_command *seg;

      if (mdata->commands[i].type != BFD_MACH_O_LC_SEGMENT
	  && mdata->commands[i].type != BFD_MACH_O_LC_SEGMENT_64)
	continue;

      seg = &mdata->commands[i].command.segment;

      fprintf (file, "[Segment %-16s ", seg->segname);
      fprintf_vma (file, seg->vmaddr);
      fprintf (file, "-");
      fprintf_vma  (file, seg->vmaddr + seg->vmsize - 1);
      fputc (' ', file);
      fputc (seg->initprot & BFD_MACH_O_PROT_READ ? 'r' : '-', file);
      fputc (seg->initprot & BFD_MACH_O_PROT_WRITE ? 'w' : '-', file);
      fputc (seg->initprot & BFD_MACH_O_PROT_EXECUTE ? 'x' : '-', file);
      fprintf (file, "]\n");
      for (j = 0; j < seg->nsects; j++)
	{
	  bfd_mach_o_section *sec = &seg->sections[j];
	  fprintf (file, "%02u: %-16s %-16s ", ++sec_nbr,
		   sec->segname, sec->sectname);
	  fprintf_vma (file, sec->addr);
	  fprintf (file, " ");
	  fprintf_vma  (file, sec->size);
	  fprintf (file, " %08lx\n", sec->flags);
	}
    }
}

static void
bfd_mach_o_print_section (bfd *abfd ATTRIBUTE_UNUSED,
                          bfd_mach_o_section *sec, FILE *file)
{
  fprintf (file, " Section: %-16s %-16s (bfdname: %s)\n",
           sec->sectname, sec->segname, sec->bfdsection->name);
  fprintf (file, "  addr: ");
  fprintf_vma (file, sec->addr);
  fprintf (file, " size: ");
  fprintf_vma  (file, sec->size);
  fprintf (file, " offset: ");
  fprintf_vma (file, sec->offset);
  fprintf (file, "\n");
  fprintf (file, "  align: %ld", sec->align);
  fprintf (file, "  nreloc: %lu  reloff: ", sec->nreloc);
  fprintf_vma (file, sec->reloff);
  fprintf (file, "\n");
  fprintf (file, "  flags: %08lx (type: %s", sec->flags,
           bfd_mach_o_get_name (bfd_mach_o_section_type_name,
                                sec->flags & BFD_MACH_O_SECTION_TYPE_MASK));
  fprintf (file, " attr: ");
  bfd_mach_o_print_flags (bfd_mach_o_section_attribute_name,
                          sec->flags & BFD_MACH_O_SECTION_ATTRIBUTES_MASK,
                          file);
  fprintf (file, ")\n");
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_SYMBOL_STUBS:
      fprintf (file, "  first indirect sym: %lu", sec->reserved1);
      fprintf (file, " (%u entries)",
               bfd_mach_o_section_get_nbr_indirect (abfd, sec));
      break;
    default:
      fprintf (file, "  reserved1: 0x%lx", sec->reserved1);
      break;
    }
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_SYMBOL_STUBS:
      fprintf (file, "  stub size: %lu", sec->reserved2);
      break;
    default:
      fprintf (file, "  reserved2: 0x%lx", sec->reserved2);
      break;
    }
  fprintf (file, "  reserved3: 0x%lx\n", sec->reserved3);
}

static void
bfd_mach_o_print_segment (bfd *abfd ATTRIBUTE_UNUSED,
                          bfd_mach_o_load_command *cmd, FILE *file)
{
  bfd_mach_o_segment_command *seg = &cmd->command.segment;
  unsigned int i;

  fprintf (file, " name: %s\n", *seg->segname ? seg->segname : "*none*");
  fprintf (file, "    vmaddr: ");
  fprintf_vma (file, seg->vmaddr);
  fprintf (file, "   vmsize: ");
  fprintf_vma  (file, seg->vmsize);
  fprintf (file, "\n");
  fprintf (file, "   fileoff: ");
  fprintf_vma (file, seg->fileoff);
  fprintf (file, " filesize: ");
  fprintf_vma (file, (bfd_vma)seg->filesize);
  fprintf (file, " endoff: ");
  fprintf_vma (file, (bfd_vma)(seg->fileoff + seg->filesize));
  fprintf (file, "\n");
  fprintf (file, "   nsects: %lu  ", seg->nsects);
  fprintf (file, " flags: %lx\n", seg->flags);
  for (i = 0; i < seg->nsects; i++)
    bfd_mach_o_print_section (abfd, &seg->sections[i], file);
}

static void
bfd_mach_o_print_dysymtab (bfd *abfd ATTRIBUTE_UNUSED,
                           bfd_mach_o_load_command *cmd, FILE *file)
{
  bfd_mach_o_dysymtab_command *dysymtab = &cmd->command.dysymtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i;

  fprintf (file, "              local symbols: idx: %10lu  num: %-8lu",
           dysymtab->ilocalsym, dysymtab->nlocalsym);
  fprintf (file, " (nxtidx: %lu)\n",
           dysymtab->ilocalsym + dysymtab->nlocalsym);
  fprintf (file, "           external symbols: idx: %10lu  num: %-8lu",
           dysymtab->iextdefsym, dysymtab->nextdefsym);
  fprintf (file, " (nxtidx: %lu)\n",
           dysymtab->iextdefsym + dysymtab->nextdefsym);
  fprintf (file, "          undefined symbols: idx: %10lu  num: %-8lu",
           dysymtab->iundefsym, dysymtab->nundefsym);
  fprintf (file, " (nxtidx: %lu)\n",
           dysymtab->iundefsym + dysymtab->nundefsym);
  fprintf (file, "           table of content: off: 0x%08lx  num: %-8lu",
           dysymtab->tocoff, dysymtab->ntoc);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->tocoff 
           + dysymtab->ntoc * BFD_MACH_O_TABLE_OF_CONTENT_SIZE); 
  fprintf (file, "               module table: off: 0x%08lx  num: %-8lu",
           dysymtab->modtaboff, dysymtab->nmodtab);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->modtaboff + dysymtab->nmodtab 
           * (mach_o_wide_p (&mdata->header) ? 
              BFD_MACH_O_DYLIB_MODULE_64_SIZE : BFD_MACH_O_DYLIB_MODULE_SIZE));
  fprintf (file, "   external reference table: off: 0x%08lx  num: %-8lu",
           dysymtab->extrefsymoff, dysymtab->nextrefsyms);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->extrefsymoff 
           + dysymtab->nextrefsyms * BFD_MACH_O_REFERENCE_SIZE);
  fprintf (file, "      indirect symbol table: off: 0x%08lx  num: %-8lu",
           dysymtab->indirectsymoff, dysymtab->nindirectsyms);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->indirectsymoff 
           + dysymtab->nindirectsyms * BFD_MACH_O_INDIRECT_SYMBOL_SIZE);
  fprintf (file, "  external relocation table: off: 0x%08lx  num: %-8lu",
           dysymtab->extreloff, dysymtab->nextrel);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->extreloff + dysymtab->nextrel * BFD_MACH_O_RELENT_SIZE);
  fprintf (file, "     local relocation table: off: 0x%08lx  num: %-8lu",
           dysymtab->locreloff, dysymtab->nlocrel);
  fprintf (file, " (endoff: 0x%08lx)\n",
           dysymtab->locreloff + dysymtab->nlocrel * BFD_MACH_O_RELENT_SIZE);
  
  if (dysymtab->ntoc > 0
      || dysymtab->nindirectsyms > 0
      || dysymtab->nextrefsyms > 0)
    {
      /* Try to read the symbols to display the toc or indirect symbols.  */
      bfd_mach_o_read_symtab_symbols (abfd);
    }
  else if (dysymtab->nmodtab > 0)
    {
      /* Try to read the strtab to display modules name.  */
      bfd_mach_o_read_symtab_strtab (abfd);
    }
  
  for (i = 0; i < dysymtab->nmodtab; i++)
    {
      bfd_mach_o_dylib_module *module = &dysymtab->dylib_module[i];
      fprintf (file, "  module %u:\n", i);
      fprintf (file, "   name: %lu", module->module_name_idx);
      if (mdata->symtab && mdata->symtab->strtab)
        fprintf (file, ": %s",
                 mdata->symtab->strtab + module->module_name_idx);
      fprintf (file, "\n");
      fprintf (file, "   extdefsym: idx: %8lu  num: %lu\n",
               module->iextdefsym, module->nextdefsym);
      fprintf (file, "      refsym: idx: %8lu  num: %lu\n",
               module->irefsym, module->nrefsym);
      fprintf (file, "    localsym: idx: %8lu  num: %lu\n",
               module->ilocalsym, module->nlocalsym);
      fprintf (file, "      extrel: idx: %8lu  num: %lu\n",
               module->iextrel, module->nextrel);
      fprintf (file, "        init: idx: %8u  num: %u\n",
               module->iinit, module->ninit);
      fprintf (file, "        term: idx: %8u  num: %u\n",
               module->iterm, module->nterm);
      fprintf (file, "   objc_module_info: addr: ");
      fprintf_vma (file, module->objc_module_info_addr);
      fprintf (file, "  size: %lu\n", module->objc_module_info_size);
    }

  if (dysymtab->ntoc > 0)
    {
      bfd_mach_o_symtab_command *symtab = mdata->symtab;
      
      fprintf (file, "  table of content: (symbol/module)\n");
      for (i = 0; i < dysymtab->ntoc; i++)
        {
          bfd_mach_o_dylib_table_of_content *toc = &dysymtab->dylib_toc[i];
          
          fprintf (file, "   %4u: ", i);
          if (symtab && symtab->symbols && toc->symbol_index < symtab->nsyms)
            {
              const char *name = symtab->symbols[toc->symbol_index].symbol.name;
              fprintf (file, "%s (%lu)", name ? name : "*invalid*",
                       toc->symbol_index);
            }
          else
            fprintf (file, "%lu", toc->symbol_index);
          
          fprintf (file, " / ");
          if (symtab && symtab->strtab
              && toc->module_index < dysymtab->nmodtab)
            {
              bfd_mach_o_dylib_module *mod;
              mod = &dysymtab->dylib_module[toc->module_index];
              fprintf (file, "%s (%lu)",
                       symtab->strtab + mod->module_name_idx,
                       toc->module_index);
            }
          else
            fprintf (file, "%lu", toc->module_index);
          
          fprintf (file, "\n");
        }
    }

  if (dysymtab->nindirectsyms != 0)
    {
      fprintf (file, "  indirect symbols:\n");

      for (i = 0; i < mdata->nsects; i++)
        {
          bfd_mach_o_section *sec = mdata->sections[i];
          unsigned int j, first, last;
          bfd_mach_o_symtab_command *symtab = mdata->symtab;
          bfd_vma addr;
          bfd_vma entry_size;
      
          switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
            {
            case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
            case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
            case BFD_MACH_O_S_SYMBOL_STUBS:
              first = sec->reserved1;
              last = first + bfd_mach_o_section_get_nbr_indirect (abfd, sec);
              addr = sec->addr;
              entry_size = bfd_mach_o_section_get_entry_size (abfd, sec);
              fprintf (file, "  for section %s.%s:\n",
                       sec->segname, sec->sectname);
              for (j = first; j < last; j++)
                {
                  unsigned int isym = dysymtab->indirect_syms[j];
                  
                  fprintf (file, "   ");
                  fprintf_vma (file, addr);
                  fprintf (file, " %5u: 0x%08x", j, isym);
                  if (isym & BFD_MACH_O_INDIRECT_SYMBOL_LOCAL)
                    fprintf (file, " LOCAL");
                  if (isym & BFD_MACH_O_INDIRECT_SYMBOL_ABS)
                    fprintf (file, " ABSOLUTE");
                  if (symtab && symtab->symbols
                      && isym < symtab->nsyms
                      && symtab->symbols[isym].symbol.name)
                    fprintf (file, " %s", symtab->symbols[isym].symbol.name);
                  fprintf (file, "\n");
                  addr += entry_size;
                }
              break;
            default:
              break;
            }
        }
    }
  if (dysymtab->nextrefsyms > 0)
    {
      bfd_mach_o_symtab_command *symtab = mdata->symtab;
      
      fprintf (file, "  external reference table: (symbol flags)\n");
      for (i = 0; i < dysymtab->nextrefsyms; i++)
        {
          bfd_mach_o_dylib_reference *ref = &dysymtab->ext_refs[i];
          
          fprintf (file, "   %4u: %5lu 0x%02lx", i, ref->isym, ref->flags);
          if (symtab && symtab->symbols
              && ref->isym < symtab->nsyms
              && symtab->symbols[ref->isym].symbol.name)
            fprintf (file, " %s", symtab->symbols[ref->isym].symbol.name);
          fprintf (file, "\n");
        }
    }

}

static void
bfd_mach_o_print_dyld_info (bfd *abfd ATTRIBUTE_UNUSED,
                            bfd_mach_o_load_command *cmd, FILE *file)
{
  bfd_mach_o_dyld_info_command *info = &cmd->command.dyld_info;

  fprintf (file, "       rebase: off: 0x%08x  size: %-8u\n",
           info->rebase_off, info->rebase_size);
  fprintf (file, "         bind: off: 0x%08x  size: %-8u\n",
           info->bind_off, info->bind_size);
  fprintf (file, "    weak bind: off: 0x%08x  size: %-8u\n",
           info->weak_bind_off, info->weak_bind_size);
  fprintf (file, "    lazy bind: off: 0x%08x  size: %-8u\n",
           info->lazy_bind_off, info->lazy_bind_size);
  fprintf (file, "       export: off: 0x%08x  size: %-8u\n",
           info->export_off, info->export_size);
}

bfd_boolean
bfd_mach_o_bfd_print_private_bfd_data (bfd *abfd, void * ptr)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  FILE *file = (FILE *) ptr;
  unsigned int i;

  bfd_mach_o_print_private_header (abfd, file);
  fputc ('\n', file);

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_load_command *cmd = &mdata->commands[i];
      
      fprintf (file, "Load command %s:",
               bfd_mach_o_get_name (bfd_mach_o_load_command_name, cmd->type));
      switch (cmd->type)
	{
	case BFD_MACH_O_LC_SEGMENT:
	case BFD_MACH_O_LC_SEGMENT_64:
          bfd_mach_o_print_segment (abfd, cmd, file);
	  break;
	case BFD_MACH_O_LC_UUID:
	  {
	    bfd_mach_o_uuid_command *uuid = &cmd->command.uuid;
	    unsigned int j;

	    for (j = 0; j < sizeof (uuid->uuid); j ++)
	      fprintf (file, " %02x", uuid->uuid[j]);
	    fputc ('\n', file);
	  }
	  break;
	case BFD_MACH_O_LC_LOAD_DYLIB:
	case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
	case BFD_MACH_O_LC_REEXPORT_DYLIB:
	case BFD_MACH_O_LC_ID_DYLIB:
	  {
	    bfd_mach_o_dylib_command *dylib = &cmd->command.dylib;
	    fprintf (file, " %s\n", dylib->name_str);
	    fprintf (file, "            time stamp: 0x%08lx\n",
		     dylib->timestamp);
	    fprintf (file, "       current version: 0x%08lx\n",
		     dylib->current_version);
	    fprintf (file, "  comptibility version: 0x%08lx\n",
		     dylib->compatibility_version);
	    break;
	  }
	case BFD_MACH_O_LC_LOAD_DYLINKER:
	case BFD_MACH_O_LC_ID_DYLINKER:
          fprintf (file, " %s\n", cmd->command.dylinker.name_str);
          break;
	case BFD_MACH_O_LC_SYMTAB:
	  {
	    bfd_mach_o_symtab_command *symtab = &cmd->command.symtab;
	    fprintf (file,
                     "\n"
		     "   symoff: 0x%08x    nsyms: %8u  (endoff: 0x%08x)\n",
                     symtab->symoff, symtab->nsyms,
                     symtab->symoff + symtab->nsyms 
                     * (mach_o_wide_p (&mdata->header) 
                        ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE));
	    fprintf (file,
		     "   stroff: 0x%08x  strsize: %8u  (endoff: 0x%08x)\n",
		     symtab->stroff, symtab->strsize,
                     symtab->stroff + symtab->strsize);
	    break;
	  }
	case BFD_MACH_O_LC_DYSYMTAB:
          fprintf (file, "\n");
          bfd_mach_o_print_dysymtab (abfd, cmd, file);
          break;
        case BFD_MACH_O_LC_CODE_SIGNATURE:
        case BFD_MACH_O_LC_SEGMENT_SPLIT_INFO:
	  {
	    bfd_mach_o_linkedit_command *linkedit = &cmd->command.linkedit;
	    fprintf
              (file, "\n"
               "  dataoff: 0x%08lx  datasize: 0x%08lx  (endoff: 0x%08lx)\n",
               linkedit->dataoff, linkedit->datasize,
               linkedit->dataoff + linkedit->datasize);
            break;
          }
        case BFD_MACH_O_LC_SUB_FRAMEWORK:
        case BFD_MACH_O_LC_SUB_UMBRELLA:
        case BFD_MACH_O_LC_SUB_LIBRARY:
        case BFD_MACH_O_LC_SUB_CLIENT:
        case BFD_MACH_O_LC_RPATH:
	  {
	    bfd_mach_o_str_command *str = &cmd->command.str;
	    fprintf (file, " %s\n", str->str);
            break;
          }
        case BFD_MACH_O_LC_THREAD:
        case BFD_MACH_O_LC_UNIXTHREAD:
          {
            bfd_mach_o_thread_command *thread = &cmd->command.thread;
            unsigned int j;
            bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

            fprintf (file, " nflavours: %lu\n", thread->nflavours);
            for (j = 0; j < thread->nflavours; j++)
              {
                bfd_mach_o_thread_flavour *flavour = &thread->flavours[j];

                fprintf (file, "  %2u: flavour: 0x%08lx  offset: 0x%08lx"
                         "  size: 0x%08lx\n",
                         j, flavour->flavour, flavour->offset,
                         flavour->size);
                if (bed->_bfd_mach_o_print_thread)
                  {
                    char *buf = bfd_malloc (flavour->size);

                    if (buf
                        && bfd_seek (abfd, flavour->offset, SEEK_SET) == 0
                        && (bfd_bread (buf, flavour->size, abfd) 
                            == flavour->size))
                      (*bed->_bfd_mach_o_print_thread)(abfd, flavour,
                                                       file, buf);
                    free (buf);
                  }
              }
            break;
          }
	case BFD_MACH_O_LC_DYLD_INFO:
          fprintf (file, "\n");
          bfd_mach_o_print_dyld_info (abfd, cmd, file);
          break;
	default:
	  fprintf (file, "\n");
	  break;
	}
      fputc ('\n', file);
    }

  bfd_mach_o_print_section_map (abfd, file);

  return TRUE;
}

int
bfd_mach_o_core_fetch_environment (bfd *abfd,
				   unsigned char **rbuf,
				   unsigned int *rlen)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned long stackaddr = bfd_mach_o_stack_addr (mdata->header.cputype);
  unsigned int i = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_load_command *cur = &mdata->commands[i];
      bfd_mach_o_segment_command *seg = NULL;

      if (cur->type != BFD_MACH_O_LC_SEGMENT)
	continue;

      seg = &cur->command.segment;

      if ((seg->vmaddr + seg->vmsize) == stackaddr)
	{
	  unsigned long start = seg->fileoff;
	  unsigned long end = seg->fileoff + seg->filesize;
	  unsigned char *buf = bfd_malloc (1024);
	  unsigned long size = 1024;

	  for (;;)
	    {
	      bfd_size_type nread = 0;
	      unsigned long offset;
	      int found_nonnull = 0;

	      if (size > (end - start))
		size = (end - start);

	      buf = bfd_realloc_or_free (buf, size);
	      if (buf == NULL)
		return -1;

	      if (bfd_seek (abfd, end - size, SEEK_SET) != 0)
                {
                  free (buf);
                  return -1;
                }

	      nread = bfd_bread (buf, size, abfd);

	      if (nread != size)
		{
		  free (buf);
		  return -1;
		}

	      for (offset = 4; offset <= size; offset += 4)
		{
		  unsigned long val;

		  val = *((unsigned long *) (buf + size - offset));
		  if (! found_nonnull)
		    {
		      if (val != 0)
			found_nonnull = 1;
		    }
		  else if (val == 0x0)
		    {
		      unsigned long bottom;
		      unsigned long top;

		      bottom = seg->fileoff + seg->filesize - offset;
		      top = seg->fileoff + seg->filesize - 4;
		      *rbuf = bfd_malloc (top - bottom);
		      *rlen = top - bottom;

		      memcpy (*rbuf, buf + size - *rlen, *rlen);
		      free (buf);
		      return 0;
		    }
		}

	      if (size == (end - start))
		break;

	      size *= 2;
	    }

	  free (buf);
	}
    }

  return -1;
}

char *
bfd_mach_o_core_file_failing_command (bfd *abfd)
{
  unsigned char *buf = NULL;
  unsigned int len = 0;
  int ret = -1;

  ret = bfd_mach_o_core_fetch_environment (abfd, &buf, &len);
  if (ret < 0)
    return NULL;

  return (char *) buf;
}

int
bfd_mach_o_core_file_failing_signal (bfd *abfd ATTRIBUTE_UNUSED)
{
  return 0;
}

#define bfd_mach_o_bfd_reloc_type_lookup _bfd_norelocs_bfd_reloc_type_lookup 
#define bfd_mach_o_bfd_reloc_name_lookup _bfd_norelocs_bfd_reloc_name_lookup

#define bfd_mach_o_swap_reloc_in NULL
#define bfd_mach_o_swap_reloc_out NULL
#define bfd_mach_o_print_thread NULL

#define TARGET_NAME 		mach_o_be_vec
#define TARGET_STRING     	"mach-o-be"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	1
#define TARGET_ARCHIVE 		0
#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE

#define TARGET_NAME 		mach_o_le_vec
#define TARGET_STRING 		"mach-o-le"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	0
#define TARGET_ARCHIVE 		0

#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE

/* Not yet handled: creating an archive.  */
#define bfd_mach_o_mkarchive                      _bfd_noarchive_mkarchive

/* Not used.  */
#define bfd_mach_o_read_ar_hdr                    _bfd_noarchive_read_ar_hdr
#define bfd_mach_o_write_ar_hdr                   _bfd_noarchive_write_ar_hdr
#define bfd_mach_o_slurp_armap                    _bfd_noarchive_slurp_armap
#define bfd_mach_o_slurp_extended_name_table      _bfd_noarchive_slurp_extended_name_table
#define bfd_mach_o_construct_extended_name_table  _bfd_noarchive_construct_extended_name_table
#define bfd_mach_o_truncate_arname                _bfd_noarchive_truncate_arname
#define bfd_mach_o_write_armap                    _bfd_noarchive_write_armap
#define bfd_mach_o_get_elt_at_index               _bfd_noarchive_get_elt_at_index
#define bfd_mach_o_generic_stat_arch_elt          _bfd_noarchive_generic_stat_arch_elt
#define bfd_mach_o_update_armap_timestamp         _bfd_noarchive_update_armap_timestamp

#define TARGET_NAME 		mach_o_fat_vec
#define TARGET_STRING 		"mach-o-fat"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	1
#define TARGET_ARCHIVE 		1

#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE
