/* od-macho.c -- dump information about an Mach-O object file.
   Copyright 2011, 2012 Free Software Foundation, Inc.
   Written by Tristan Gingold, Adacore.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stddef.h>
#include <time.h>
#include "safe-ctype.h"
#include "bfd.h"
#include "objdump.h"
#include "bucomm.h"
#include "bfdlink.h"
#include "libbfd.h"
#include "mach-o.h"
#include "mach-o/external.h"
#include "mach-o/codesign.h"

/* Index of the options in the options[] array.  */
#define OPT_HEADER 0
#define OPT_SECTION 1
#define OPT_MAP 2
#define OPT_LOAD 3
#define OPT_DYSYMTAB 4
#define OPT_CODESIGN 5
#define OPT_SEG_SPLIT_INFO 6

/* List of actions.  */
static struct objdump_private_option options[] =
  {
    { "header", 0 },
    { "section", 0 },
    { "map", 0 },
    { "load", 0 },
    { "dysymtab", 0 },
    { "codesign", 0 },
    { "seg_split_info", 0 },
    { NULL, 0 }
  };

/* Display help.  */

static void
mach_o_help (FILE *stream)
{
  fprintf (stream, _("\
For Mach-O files:\n\
  header         Display the file header\n\
  section        Display the segments and sections commands\n\
  map            Display the section map\n\
  load           Display the load commands\n\
  dysymtab       Display the dynamic symbol table\n\
  codesign       Display code signature\n\
  seg_split_info Display segment split info\n\
"));
}

/* Return TRUE if ABFD is handled.  */

static int
mach_o_filter (bfd *abfd)
{
  return bfd_get_flavour (abfd) == bfd_target_mach_o_flavour;
}

static const bfd_mach_o_xlat_name bfd_mach_o_cpu_name[] =
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

static const bfd_mach_o_xlat_name bfd_mach_o_filetype_name[] =
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

static const bfd_mach_o_xlat_name bfd_mach_o_header_flags_name[] =
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

static const bfd_mach_o_xlat_name bfd_mach_o_load_command_name[] =
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
  { "load_upward_lib", BFD_MACH_O_LC_LOAD_UPWARD_DYLIB},
  { "version_min_macosx", BFD_MACH_O_LC_VERSION_MIN_MACOSX},
  { "version_min_iphoneos", BFD_MACH_O_LC_VERSION_MIN_IPHONEOS},
  { "function_starts", BFD_MACH_O_LC_FUNCTION_STARTS},
  { "dyld_environment", BFD_MACH_O_LC_DYLD_ENVIRONMENT},
  { NULL, 0}
};

static const bfd_mach_o_xlat_name bfd_mach_o_thread_x86_name[] =
{
  { "thread_state32", BFD_MACH_O_x86_THREAD_STATE32},
  { "float_state32", BFD_MACH_O_x86_FLOAT_STATE32},
  { "exception_state32", BFD_MACH_O_x86_EXCEPTION_STATE32},
  { "thread_state64", BFD_MACH_O_x86_THREAD_STATE64},
  { "float_state64", BFD_MACH_O_x86_FLOAT_STATE64},
  { "exception_state64", BFD_MACH_O_x86_EXCEPTION_STATE64},
  { "thread_state", BFD_MACH_O_x86_THREAD_STATE},
  { "float_state", BFD_MACH_O_x86_FLOAT_STATE},
  { "exception_state", BFD_MACH_O_x86_EXCEPTION_STATE},
  { "debug_state32", BFD_MACH_O_x86_DEBUG_STATE32},
  { "debug_state64", BFD_MACH_O_x86_DEBUG_STATE64},
  { "debug_state", BFD_MACH_O_x86_DEBUG_STATE},
  { "state_none", BFD_MACH_O_x86_THREAD_STATE_NONE},
  { NULL, 0 }
};

static void
bfd_mach_o_print_flags (const bfd_mach_o_xlat_name *table,
                        unsigned long val)
{
  int first = 1;

  for (; table->name; table++)
    {
      if (table->val & val)
        {
          if (!first)
            printf ("+");
          printf ("%s", table->name);
          val &= ~table->val;
          first = 0;
        }
    }
  if (val)
    {
      if (!first)
        printf ("+");
      printf ("0x%lx", val);
      return;
    }
  if (first)
    printf ("-");
}

static const char *
bfd_mach_o_get_name_or_null (const bfd_mach_o_xlat_name *table,
                             unsigned long val)
{
  for (; table->name; table++)
    if (table->val == val)
      return table->name;
  return NULL;
}

static const char *
bfd_mach_o_get_name (const bfd_mach_o_xlat_name *table, unsigned long val)
{
  const char *res = bfd_mach_o_get_name_or_null (table, val);

  if (res == NULL)
    return "*UNKNOWN*";
  else
    return res;
}

static void
dump_header (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_header *h = &mdata->header;

  fputs (_("Mach-O header:\n"), stdout);
  printf (_(" magic     : %08lx\n"), h->magic);
  printf (_(" cputype   : %08lx (%s)\n"), h->cputype,
          bfd_mach_o_get_name (bfd_mach_o_cpu_name, h->cputype));
  printf (_(" cpusubtype: %08lx\n"), h->cpusubtype);
  printf (_(" filetype  : %08lx (%s)\n"),
          h->filetype,
          bfd_mach_o_get_name (bfd_mach_o_filetype_name, h->filetype));
  printf (_(" ncmds     : %08lx (%lu)\n"), h->ncmds, h->ncmds);
  printf (_(" sizeofcmds: %08lx\n"), h->sizeofcmds);
  printf (_(" flags     : %08lx ("), h->flags);
  bfd_mach_o_print_flags (bfd_mach_o_header_flags_name, h->flags);
  fputs (_(")\n"), stdout);
  printf (_(" reserved  : %08x\n"), h->reserved);
}

static void
dump_section_map (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i;
  unsigned int sec_nbr = 0;

  fputs (_("Segments and Sections:\n"), stdout);
  fputs (_(" #: Segment name     Section name     Address\n"), stdout);

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_segment_command *seg;
      bfd_mach_o_section *sec;

      if (mdata->commands[i].type != BFD_MACH_O_LC_SEGMENT
	  && mdata->commands[i].type != BFD_MACH_O_LC_SEGMENT_64)
	continue;

      seg = &mdata->commands[i].command.segment;

      printf ("[Segment %-16s ", seg->segname);
      printf_vma (seg->vmaddr);
      putchar ('-');
      printf_vma  (seg->vmaddr + seg->vmsize - 1);
      putchar (' ');
      putchar (seg->initprot & BFD_MACH_O_PROT_READ ? 'r' : '-');
      putchar (seg->initprot & BFD_MACH_O_PROT_WRITE ? 'w' : '-');
      putchar (seg->initprot & BFD_MACH_O_PROT_EXECUTE ? 'x' : '-');
      printf ("]\n");

      for (sec = seg->sect_head; sec != NULL; sec = sec->next)
	{
	  printf ("%02u: %-16s %-16s ", ++sec_nbr,
                  sec->segname, sec->sectname);
	  printf_vma (sec->addr);
	  putchar (' ');
	  printf_vma  (sec->size);
	  printf (" %08lx\n", sec->flags);
	}
    }
}

static void
dump_section (bfd *abfd ATTRIBUTE_UNUSED, bfd_mach_o_section *sec)
{
  printf (" Section: %-16s %-16s (bfdname: %s)\n",
           sec->sectname, sec->segname, sec->bfdsection->name);
  printf ("  addr: ");
  printf_vma (sec->addr);
  printf (" size: ");
  printf_vma (sec->size);
  printf (" offset: ");
  printf_vma (sec->offset);
  printf ("\n");
  printf ("  align: %ld", sec->align);
  printf ("  nreloc: %lu  reloff: ", sec->nreloc);
  printf_vma (sec->reloff);
  printf ("\n");
  printf ("  flags: %08lx (type: %s", sec->flags,
          bfd_mach_o_get_name (bfd_mach_o_section_type_name,
                               sec->flags & BFD_MACH_O_SECTION_TYPE_MASK));
  printf (" attr: ");
  bfd_mach_o_print_flags (bfd_mach_o_section_attribute_name,
                          sec->flags & BFD_MACH_O_SECTION_ATTRIBUTES_MASK);
  printf (")\n");
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_SYMBOL_STUBS:
      printf ("  first indirect sym: %lu", sec->reserved1);
      printf (" (%u entries)",
               bfd_mach_o_section_get_nbr_indirect (abfd, sec));
      break;
    default:
      printf ("  reserved1: 0x%lx", sec->reserved1);
      break;
    }
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_SYMBOL_STUBS:
      printf ("  stub size: %lu", sec->reserved2);
      break;
    default:
      printf ("  reserved2: 0x%lx", sec->reserved2);
      break;
    }
  printf ("  reserved3: 0x%lx\n", sec->reserved3);
}

static void
dump_segment (bfd *abfd ATTRIBUTE_UNUSED, bfd_mach_o_load_command *cmd)
{
  bfd_mach_o_segment_command *seg = &cmd->command.segment;
  bfd_mach_o_section *sec;

  printf (" name: %s\n", *seg->segname ? seg->segname : "*none*");
  printf ("    vmaddr: ");
  printf_vma (seg->vmaddr);
  printf ("   vmsize: ");
  printf_vma  (seg->vmsize);
  printf ("\n");
  printf ("   fileoff: ");
  printf_vma (seg->fileoff);
  printf (" filesize: ");
  printf_vma ((bfd_vma)seg->filesize);
  printf (" endoff: ");
  printf_vma ((bfd_vma)(seg->fileoff + seg->filesize));
  printf ("\n");
  printf ("   nsects: %lu  ", seg->nsects);
  printf (" flags: %lx\n", seg->flags);
  for (sec = seg->sect_head; sec != NULL; sec = sec->next)
    dump_section (abfd, sec);
}

static void
dump_dysymtab (bfd *abfd, bfd_mach_o_load_command *cmd, bfd_boolean verbose)
{
  bfd_mach_o_dysymtab_command *dysymtab = &cmd->command.dysymtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i;

  printf ("              local symbols: idx: %10lu  num: %-8lu",
          dysymtab->ilocalsym, dysymtab->nlocalsym);
  printf (" (nxtidx: %lu)\n",
          dysymtab->ilocalsym + dysymtab->nlocalsym);
  printf ("           external symbols: idx: %10lu  num: %-8lu",
          dysymtab->iextdefsym, dysymtab->nextdefsym);
  printf (" (nxtidx: %lu)\n",
          dysymtab->iextdefsym + dysymtab->nextdefsym);
  printf ("          undefined symbols: idx: %10lu  num: %-8lu",
          dysymtab->iundefsym, dysymtab->nundefsym);
  printf (" (nxtidx: %lu)\n",
          dysymtab->iundefsym + dysymtab->nundefsym);
  printf ("           table of content: off: 0x%08lx  num: %-8lu",
          dysymtab->tocoff, dysymtab->ntoc);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->tocoff + dysymtab->ntoc * BFD_MACH_O_TABLE_OF_CONTENT_SIZE);
  printf ("               module table: off: 0x%08lx  num: %-8lu",
          dysymtab->modtaboff, dysymtab->nmodtab);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->modtaboff + dysymtab->nmodtab
          * (mdata->header.version == 2 ?
             BFD_MACH_O_DYLIB_MODULE_64_SIZE : BFD_MACH_O_DYLIB_MODULE_SIZE));
  printf ("   external reference table: off: 0x%08lx  num: %-8lu",
          dysymtab->extrefsymoff, dysymtab->nextrefsyms);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->extrefsymoff
          + dysymtab->nextrefsyms * BFD_MACH_O_REFERENCE_SIZE);
  printf ("      indirect symbol table: off: 0x%08lx  num: %-8lu",
          dysymtab->indirectsymoff, dysymtab->nindirectsyms);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->indirectsymoff
          + dysymtab->nindirectsyms * BFD_MACH_O_INDIRECT_SYMBOL_SIZE);
  printf ("  external relocation table: off: 0x%08lx  num: %-8lu",
          dysymtab->extreloff, dysymtab->nextrel);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->extreloff + dysymtab->nextrel * BFD_MACH_O_RELENT_SIZE);
  printf ("     local relocation table: off: 0x%08lx  num: %-8lu",
          dysymtab->locreloff, dysymtab->nlocrel);
  printf (" (endoff: 0x%08lx)\n",
          dysymtab->locreloff + dysymtab->nlocrel * BFD_MACH_O_RELENT_SIZE);

  if (!verbose)
    return;

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
      printf ("  module %u:\n", i);
      printf ("   name: %lu", module->module_name_idx);
      if (mdata->symtab && mdata->symtab->strtab)
        printf (": %s",
                 mdata->symtab->strtab + module->module_name_idx);
      printf ("\n");
      printf ("   extdefsym: idx: %8lu  num: %lu\n",
               module->iextdefsym, module->nextdefsym);
      printf ("      refsym: idx: %8lu  num: %lu\n",
               module->irefsym, module->nrefsym);
      printf ("    localsym: idx: %8lu  num: %lu\n",
               module->ilocalsym, module->nlocalsym);
      printf ("      extrel: idx: %8lu  num: %lu\n",
               module->iextrel, module->nextrel);
      printf ("        init: idx: %8u  num: %u\n",
               module->iinit, module->ninit);
      printf ("        term: idx: %8u  num: %u\n",
               module->iterm, module->nterm);
      printf ("   objc_module_info: addr: ");
      printf_vma (module->objc_module_info_addr);
      printf ("  size: %lu\n", module->objc_module_info_size);
    }

  if (dysymtab->ntoc > 0)
    {
      bfd_mach_o_symtab_command *symtab = mdata->symtab;

      printf ("  table of content: (symbol/module)\n");
      for (i = 0; i < dysymtab->ntoc; i++)
        {
          bfd_mach_o_dylib_table_of_content *toc = &dysymtab->dylib_toc[i];

          printf ("   %4u: ", i);
          if (symtab && symtab->symbols && toc->symbol_index < symtab->nsyms)
            {
              const char *name = symtab->symbols[toc->symbol_index].symbol.name;
              printf ("%s (%lu)", name ? name : "*invalid*",
                       toc->symbol_index);
            }
          else
            printf ("%lu", toc->symbol_index);

          printf (" / ");
          if (symtab && symtab->strtab
              && toc->module_index < dysymtab->nmodtab)
            {
              bfd_mach_o_dylib_module *mod;
              mod = &dysymtab->dylib_module[toc->module_index];
              printf ("%s (%lu)",
                       symtab->strtab + mod->module_name_idx,
                       toc->module_index);
            }
          else
            printf ("%lu", toc->module_index);

          printf ("\n");
        }
    }

  if (dysymtab->nindirectsyms != 0)
    {
      printf ("  indirect symbols:\n");

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
              printf ("  for section %s.%s:\n",
                       sec->segname, sec->sectname);
              for (j = first; j < last; j++)
                {
                  unsigned int isym = dysymtab->indirect_syms[j];

                  printf ("   ");
                  printf_vma (addr);
                  printf (" %5u: 0x%08x", j, isym);
                  if (isym & BFD_MACH_O_INDIRECT_SYMBOL_LOCAL)
                    printf (" LOCAL");
                  if (isym & BFD_MACH_O_INDIRECT_SYMBOL_ABS)
                    printf (" ABSOLUTE");
                  if (symtab && symtab->symbols
                      && isym < symtab->nsyms
                      && symtab->symbols[isym].symbol.name)
                    printf (" %s", symtab->symbols[isym].symbol.name);
                  printf ("\n");
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

      printf ("  external reference table: (symbol flags)\n");
      for (i = 0; i < dysymtab->nextrefsyms; i++)
        {
          bfd_mach_o_dylib_reference *ref = &dysymtab->ext_refs[i];

          printf ("   %4u: %5lu 0x%02lx", i, ref->isym, ref->flags);
          if (symtab && symtab->symbols
              && ref->isym < symtab->nsyms
              && symtab->symbols[ref->isym].symbol.name)
            printf (" %s", symtab->symbols[ref->isym].symbol.name);
          printf ("\n");
        }
    }

}

static void
dump_dyld_info (bfd *abfd ATTRIBUTE_UNUSED, bfd_mach_o_load_command *cmd)
{
  bfd_mach_o_dyld_info_command *info = &cmd->command.dyld_info;

  printf ("       rebase: off: 0x%08x  size: %-8u\n",
           info->rebase_off, info->rebase_size);
  printf ("         bind: off: 0x%08x  size: %-8u\n",
           info->bind_off, info->bind_size);
  printf ("    weak bind: off: 0x%08x  size: %-8u\n",
           info->weak_bind_off, info->weak_bind_size);
  printf ("    lazy bind: off: 0x%08x  size: %-8u\n",
           info->lazy_bind_off, info->lazy_bind_size);
  printf ("       export: off: 0x%08x  size: %-8u\n",
           info->export_off, info->export_size);
}

static void
dump_thread (bfd *abfd, bfd_mach_o_load_command *cmd)
{
  bfd_mach_o_thread_command *thread = &cmd->command.thread;
  unsigned int j;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  printf (" nflavours: %lu\n", thread->nflavours);
  for (j = 0; j < thread->nflavours; j++)
    {
      bfd_mach_o_thread_flavour *flavour = &thread->flavours[j];
      const bfd_mach_o_xlat_name *name_table;

      printf ("  %2u: flavour: 0x%08lx", j, flavour->flavour);
      switch (mdata->header.cputype)
        {
        case BFD_MACH_O_CPU_TYPE_I386:
        case BFD_MACH_O_CPU_TYPE_X86_64:
          name_table = bfd_mach_o_thread_x86_name;
          break;
        default:
          name_table = NULL;
          break;
        }
      if (name_table != NULL)
        printf (": %s", bfd_mach_o_get_name (name_table, flavour->flavour));
      putchar ('\n');

      printf ("       offset: 0x%08lx  size: 0x%08lx\n",
              flavour->offset, flavour->size);
      if (bed->_bfd_mach_o_print_thread)
        {
          char *buf = xmalloc (flavour->size);

          if (bfd_seek (abfd, flavour->offset, SEEK_SET) == 0
              && bfd_bread (buf, flavour->size, abfd) == flavour->size)
            (*bed->_bfd_mach_o_print_thread)(abfd, flavour, stdout, buf);

          free (buf);
        }
    }
}

static const bfd_mach_o_xlat_name bfd_mach_o_cs_magic[] =
{
  { "embedded signature", BFD_MACH_O_CS_MAGIC_EMBEDDED_SIGNATURE },
  { "requirement", BFD_MACH_O_CS_MAGIC_REQUIREMENT },
  { "requirements", BFD_MACH_O_CS_MAGIC_REQUIREMENTS },
  { "code directory", BFD_MACH_O_CS_MAGIC_CODEDIRECTORY },
  { "embedded entitlements", BFD_MACH_O_CS_MAGIC_EMBEDDED_ENTITLEMENTS },
  { "blob wrapper", BFD_MACH_O_CS_MAGIC_BLOB_WRAPPER },
  { NULL, 0 }
};

static const bfd_mach_o_xlat_name bfd_mach_o_cs_hash_type[] =
{
  { "no-hash", BFD_MACH_O_CS_NO_HASH },
  { "sha1", BFD_MACH_O_CS_HASH_SHA1 },
  { "sha256", BFD_MACH_O_CS_HASH_SHA256 },
  { "skein 160", BFD_MACH_O_CS_HASH_PRESTANDARD_SKEIN_160x256 },
  { "skein 256", BFD_MACH_O_CS_HASH_PRESTANDARD_SKEIN_256x512 },
  { NULL, 0 }
};

static unsigned int
dump_code_signature_blob (bfd *abfd, const unsigned char *buf, unsigned int len);

static void
dump_code_signature_superblob (bfd *abfd ATTRIBUTE_UNUSED,
                               const unsigned char *buf, unsigned int len)
{
  unsigned int count;
  unsigned int i;

  if (len < 12)
    {
      printf (_("  [bad block length]\n"));
      return;
    }
  count = bfd_getb32 (buf + 8);
  printf (_("  %u index entries:\n"), count);
  if (len < 12 + 8 * count)
    {
      printf (_("  [bad block length]\n"));
      return;
    }
  for (i = 0; i < count; i++)
    {
      unsigned int type;
      unsigned int off;

      type = bfd_getb32 (buf + 12 + 8 * i);
      off = bfd_getb32 (buf + 12 + 8 * i + 4);
      printf (_("  index entry %u: type: %08x, offset: %08x\n"),
              i, type, off);

      dump_code_signature_blob (abfd, buf + off, len - off);
    }
}

static void
swap_code_codedirectory_v1_in
  (const struct mach_o_codesign_codedirectory_external_v1 *src,
   struct mach_o_codesign_codedirectory_v1 *dst)
{
  dst->version = bfd_getb32 (src->version);
  dst->flags = bfd_getb32 (src->flags);
  dst->hash_offset = bfd_getb32 (src->hash_offset);
  dst->ident_offset = bfd_getb32 (src->ident_offset);
  dst->nbr_special_slots = bfd_getb32 (src->nbr_special_slots);
  dst->nbr_code_slots = bfd_getb32 (src->nbr_code_slots);
  dst->code_limit = bfd_getb32 (src->code_limit);
  dst->hash_size = src->hash_size[0];
  dst->hash_type = src->hash_type[0];
  dst->spare1 = src->spare1[0];
  dst->page_size = src->page_size[0];
  dst->spare2 = bfd_getb32 (src->spare2);
}

static void
hexdump (unsigned int start, unsigned int len,
         const unsigned char *buf)
{
  unsigned int i, j;

  for (i = 0; i < len; i += 16)
    {
      printf ("%08x:", start + i);
      for (j = 0; j < 16; j++)
        {
          fputc (j == 8 ? '-' : ' ', stdout);
          if (i + j < len)
            printf ("%02x", buf[i + j]);
          else
            fputs ("  ", stdout);
        }
      fputc (' ', stdout);
      for (j = 0; j < 16; j++)
        {
          if (i + j < len)
            fputc (ISPRINT (buf[i + j]) ? buf[i + j] : '.', stdout);
          else
            fputc (' ', stdout);
        }
      fputc ('\n', stdout);
    }
}

static void
dump_code_signature_codedirectory (bfd *abfd ATTRIBUTE_UNUSED,
                                   const unsigned char *buf, unsigned int len)
{
  struct mach_o_codesign_codedirectory_v1 cd;
  const char *id;

  if (len < sizeof (struct mach_o_codesign_codedirectory_external_v1))
    {
      printf (_("  [bad block length]\n"));
      return;
    }

  swap_code_codedirectory_v1_in
    ((const struct mach_o_codesign_codedirectory_external_v1 *) (buf + 8), &cd);

  printf (_("  version:           %08x\n"), cd.version);
  printf (_("  flags:             %08x\n"), cd.flags);
  printf (_("  hash offset:       %08x\n"), cd.hash_offset);
  id = (const char *) buf + cd.ident_offset;
  printf (_("  ident offset:      %08x (- %08x)\n"),
          cd.ident_offset, cd.ident_offset + (unsigned) strlen (id) + 1);
  printf (_("   identity: %s\n"), id);
  printf (_("  nbr special slots: %08x (at offset %08x)\n"),
          cd.nbr_special_slots,
          cd.hash_offset - cd.nbr_special_slots * cd.hash_size);
  printf (_("  nbr code slots:    %08x\n"), cd.nbr_code_slots);
  printf (_("  code limit:        %08x\n"), cd.code_limit);
  printf (_("  hash size:         %02x\n"), cd.hash_size);
  printf (_("  hash type:         %02x (%s)\n"),
          cd.hash_type,
          bfd_mach_o_get_name (bfd_mach_o_cs_hash_type, cd.hash_type));
  printf (_("  spare1:            %02x\n"), cd.spare1);
  printf (_("  page size:         %02x\n"), cd.page_size);
  printf (_("  spare2:            %08x\n"), cd.spare2);
  if (cd.version >= 0x20100)
    printf (_("  scatter offset:    %08x\n"),
            (unsigned) bfd_getb32 (buf + 44));
}

static unsigned int
dump_code_signature_blob (bfd *abfd, const unsigned char *buf, unsigned int len)
{
  unsigned int magic;
  unsigned int length;

  if (len < 8)
    {
      printf (_("  [truncated block]\n"));
      return 0;
    }
  magic = bfd_getb32 (buf);
  length = bfd_getb32 (buf + 4);
  if (magic == 0 || length == 0)
    return 0;

  printf (_(" magic : %08x (%s)\n"), magic,
          bfd_mach_o_get_name (bfd_mach_o_cs_magic, magic));
  printf (_(" length: %08x\n"), length);
  if (length > len)
    {
      printf (_("  [bad block length]\n"));
      return 0;
    }

  switch (magic)
    {
    case BFD_MACH_O_CS_MAGIC_EMBEDDED_SIGNATURE:
      dump_code_signature_superblob (abfd, buf, length);
      break;
    case BFD_MACH_O_CS_MAGIC_CODEDIRECTORY:
      dump_code_signature_codedirectory (abfd, buf, length);
      break;
    default:
      hexdump (0, length - 8, buf + 8);
      break;
    }
  return length;
}

static void
dump_code_signature (bfd *abfd, bfd_mach_o_linkedit_command *cmd)
{
  unsigned char *buf = xmalloc (cmd->datasize);
  unsigned int off;

  if (bfd_seek (abfd, cmd->dataoff, SEEK_SET) != 0
      || bfd_bread (buf, cmd->datasize, abfd) != cmd->datasize)
    {
      non_fatal (_("cannot read code signature data"));
      free (buf);
      return;
    }
  for (off = 0; off < cmd->datasize;)
    {
      unsigned int len;

      len = dump_code_signature_blob (abfd, buf + off, cmd->datasize - off);

      if (len == 0)
        break;
      off += len;
    }
  free (buf);
}

static void
dump_segment_split_info (bfd *abfd, bfd_mach_o_linkedit_command *cmd)
{
  unsigned char *buf = xmalloc (cmd->datasize);
  unsigned char *p;
  unsigned int len;
  bfd_vma addr = 0;

  if (bfd_seek (abfd, cmd->dataoff, SEEK_SET) != 0
      || bfd_bread (buf, cmd->datasize, abfd) != cmd->datasize)
    {
      non_fatal (_("cannot read segment split info"));
      free (buf);
      return;
    }
  if (buf[cmd->datasize - 1] != 0)
    {
      non_fatal (_("segment split info is not nul terminated"));
      free (buf);
      return;
    }

  switch (buf[0])
    {
    case 0:
      printf (_("  32 bit pointers:\n"));
      break;
    case 1:
      printf (_("  64 bit pointers:\n"));
      break;
    case 2:
      printf (_("  PPC hi-16:\n"));
      break;
    default:
      printf (_("  Unhandled location type %u\n"), buf[0]);
      break;
    }
  for (p = buf + 1; *p != 0; p += len)
    {
      addr += read_unsigned_leb128 (abfd, p, &len);
      fputs ("    ", stdout);
      bfd_printf_vma (abfd, addr);
      putchar ('\n');
    }
  free (buf);
}

static void
dump_load_command (bfd *abfd, bfd_mach_o_load_command *cmd,
                   bfd_boolean verbose)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  const char *cmd_name;

  cmd_name = bfd_mach_o_get_name_or_null
    (bfd_mach_o_load_command_name, cmd->type);
  printf ("Load command ");
  if (cmd_name == NULL)
    printf ("0x%02x:", cmd->type);
  else
    printf ("%s:", cmd_name);

  switch (cmd->type)
    {
    case BFD_MACH_O_LC_SEGMENT:
    case BFD_MACH_O_LC_SEGMENT_64:
      dump_segment (abfd, cmd);
      break;
    case BFD_MACH_O_LC_UUID:
      {
        bfd_mach_o_uuid_command *uuid = &cmd->command.uuid;
        unsigned int j;

        for (j = 0; j < sizeof (uuid->uuid); j ++)
          printf (" %02x", uuid->uuid[j]);
        putchar ('\n');
      }
      break;
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
    case BFD_MACH_O_LC_REEXPORT_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB:
    case BFD_MACH_O_LC_LOAD_UPWARD_DYLIB:
      {
        bfd_mach_o_dylib_command *dylib = &cmd->command.dylib;
        printf (" %s\n", dylib->name_str);
        printf ("            time stamp: 0x%08lx\n",
                dylib->timestamp);
        printf ("       current version: 0x%08lx\n",
                dylib->current_version);
        printf ("  comptibility version: 0x%08lx\n",
                dylib->compatibility_version);
      }
      break;
    case BFD_MACH_O_LC_LOAD_DYLINKER:
    case BFD_MACH_O_LC_ID_DYLINKER:
      printf (" %s\n", cmd->command.dylinker.name_str);
      break;
    case BFD_MACH_O_LC_SYMTAB:
      {
        bfd_mach_o_symtab_command *symtab = &cmd->command.symtab;
        printf ("\n"
                "   symoff: 0x%08x    nsyms: %8u  (endoff: 0x%08x)\n",
                symtab->symoff, symtab->nsyms,
                symtab->symoff + symtab->nsyms
                * (mdata->header.version == 2
                   ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE));
        printf ("   stroff: 0x%08x  strsize: %8u  (endoff: 0x%08x)\n",
                symtab->stroff, symtab->strsize,
                symtab->stroff + symtab->strsize);
        break;
      }
    case BFD_MACH_O_LC_DYSYMTAB:
      putchar ('\n');
      dump_dysymtab (abfd, cmd, verbose);
      break;
    case BFD_MACH_O_LC_LOADFVMLIB:
    case BFD_MACH_O_LC_IDFVMLIB:
      {
        bfd_mach_o_fvmlib_command *fvmlib = &cmd->command.fvmlib;
        printf (" %s\n", fvmlib->name_str);
        printf ("         minor version: 0x%08x\n", fvmlib->minor_version);
        printf ("        header address: 0x%08x\n", fvmlib->header_addr);
      }
      break;
    case BFD_MACH_O_LC_CODE_SIGNATURE:
    case BFD_MACH_O_LC_SEGMENT_SPLIT_INFO:
    case BFD_MACH_O_LC_FUNCTION_STARTS:
      {
        bfd_mach_o_linkedit_command *linkedit = &cmd->command.linkedit;
        printf
          ("\n"
           "  dataoff: 0x%08lx  datasize: 0x%08lx  (endoff: 0x%08lx)\n",
           linkedit->dataoff, linkedit->datasize,
           linkedit->dataoff + linkedit->datasize);

        if (verbose && cmd->type == BFD_MACH_O_LC_CODE_SIGNATURE)
          dump_code_signature (abfd, linkedit);
        else if (verbose && cmd->type == BFD_MACH_O_LC_SEGMENT_SPLIT_INFO)
          dump_segment_split_info (abfd, linkedit);
        break;
      }
    case BFD_MACH_O_LC_SUB_FRAMEWORK:
    case BFD_MACH_O_LC_SUB_UMBRELLA:
    case BFD_MACH_O_LC_SUB_LIBRARY:
    case BFD_MACH_O_LC_SUB_CLIENT:
    case BFD_MACH_O_LC_RPATH:
      {
        bfd_mach_o_str_command *str = &cmd->command.str;
        printf (" %s\n", str->str);
        break;
      }
    case BFD_MACH_O_LC_THREAD:
    case BFD_MACH_O_LC_UNIXTHREAD:
      dump_thread (abfd, cmd);
      break;
    case BFD_MACH_O_LC_ENCRYPTION_INFO:
      {
        bfd_mach_o_encryption_info_command *cryp =
          &cmd->command.encryption_info;
        printf
          ("\n"
           "  cryptoff: 0x%08x  cryptsize: 0x%08x (endoff 0x%08x)"
           " cryptid: %u\n",
           cryp->cryptoff, cryp->cryptsize,
           cryp->cryptoff + cryp->cryptsize,
           cryp->cryptid);
      }
      break;
    case BFD_MACH_O_LC_DYLD_INFO:
      putchar ('\n');
      dump_dyld_info (abfd, cmd);
      break;
    case BFD_MACH_O_LC_VERSION_MIN_MACOSX:
    case BFD_MACH_O_LC_VERSION_MIN_IPHONEOS:
      {
        bfd_mach_o_version_min_command *ver = &cmd->command.version_min;

        printf (" %u.%u.%u\n", ver->rel, ver->maj, ver->min);
      }
      break;
    default:
      putchar ('\n');
      printf ("  offset: 0x%08lx\n", (unsigned long)cmd->offset);
      printf ("    size: 0x%08lx\n", (unsigned long)cmd->len);
      break;
    }
  putchar ('\n');
}

static void
dump_load_commands (bfd *abfd, unsigned int cmd32, unsigned int cmd64)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int i;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_load_command *cmd = &mdata->commands[i];

      if (cmd32 == 0)
        dump_load_command (abfd, cmd, FALSE);
      else if (cmd->type == cmd32 || cmd->type == cmd64)
        dump_load_command (abfd, cmd, TRUE);
    }
}

/* Dump ABFD (according to the options[] array).  */

static void
mach_o_dump (bfd *abfd)
{
  if (options[OPT_HEADER].selected)
    dump_header (abfd);
  if (options[OPT_SECTION].selected)
    dump_load_commands (abfd, BFD_MACH_O_LC_SEGMENT, BFD_MACH_O_LC_SEGMENT_64);
  if (options[OPT_MAP].selected)
    dump_section_map (abfd);
  if (options[OPT_LOAD].selected)
    dump_load_commands (abfd, 0, 0);
  if (options[OPT_DYSYMTAB].selected)
    dump_load_commands (abfd, BFD_MACH_O_LC_DYSYMTAB, 0);
  if (options[OPT_CODESIGN].selected)
    dump_load_commands (abfd, BFD_MACH_O_LC_CODE_SIGNATURE, 0);
  if (options[OPT_SEG_SPLIT_INFO].selected)
    dump_load_commands (abfd, BFD_MACH_O_LC_SEGMENT_SPLIT_INFO, 0);
}

/* Vector for Mach-O.  */

const struct objdump_private_desc objdump_private_desc_mach_o =
  {
    mach_o_help,
    mach_o_filter,
    mach_o_dump,
    options
  };
