/* corefile.c

   Copyright (C) 1999-2026 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "config.h"
#include "util.h"
#include "bfd.h"
#include "gp-gmon.h"

#include "source.h"
#include "symtab.h"

#include "safe-ctype.h"
#include <limits.h>    /* For UINT_MAX.  */
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include "gp-experiment.h"
#include <filenames.h>

#define _(String) (String)

bfd *core_bfd;
static int core_num_syms;
static asymbol **core_syms;
asection *core_text_sect;
void * core_text_space;

/* Greatest common divisor of instruction sizes and alignments.  */
static int insn_boundary;
int offset_to_code;

/* For mapping symbols to specific .o files during file ordering.  */
static struct function_map * symbol_map;
static unsigned int symbol_map_count;

static int core_sym_class (asymbol *);
static bool get_src_info
  (bfd_vma, const char **, const char **, int *);


#define BUFSIZE      (1024)
/* This is BUFSIZE - 1 as a string.  Suitable for use in fprintf/sscanf format strings.  */
#define STR_BUFSIZE  "1023"


int
core_init (const char * aout_name, const char *whoami)
{
  int core_sym_bytes;
  asymbol *synthsyms;
  long synth_count;

  core_bfd = bfd_openr (aout_name, 0);

  if (!core_bfd)
    {
      perror (aout_name);
      return -1;
    }

  core_bfd->flags |= BFD_DECOMPRESS;

  if (!bfd_check_format (core_bfd, bfd_object))
    {
      fprintf (stderr, _("%s: %s: not in executable format\n"), whoami, aout_name);
      return -1;
    }

  /* Get core's text section.  */
  core_text_sect = bfd_get_section_by_name (core_bfd, ".text");
  if (!core_text_sect)
    {
      core_text_sect = bfd_get_section_by_name (core_bfd, "$CODE$");
      if (!core_text_sect)
	{
	  fprintf (stderr, _("%s: can't find .text section in %s\n"),
		   whoami, aout_name);
	  return -1;
	}
    }

  /* Read core's symbol table.  */

  /* This will probably give us more than we need, but that's ok.  */
  core_sym_bytes = bfd_get_symtab_upper_bound (core_bfd);
  if (core_sym_bytes < 0)
    {
      fprintf (stderr, "%s: %s: %s\n", whoami, aout_name,
	       bfd_errmsg (bfd_get_error ()));
      return -1;
    }

  core_syms = (asymbol **) xmalloc (core_sym_bytes);
  core_num_syms = bfd_canonicalize_symtab (core_bfd, core_syms);

  if (core_num_syms < 0)
    {
      fprintf (stderr, "%s: %s: %s\n", whoami, aout_name,
	       bfd_errmsg (bfd_get_error ()));
      return -1;
    }

  synth_count = bfd_get_synthetic_symtab (core_bfd, core_num_syms, core_syms,
					  0, NULL, &synthsyms);
  if (synth_count > 0)
    {
      asymbol **symp;
      long new_size;
      long i;

      new_size = (core_num_syms + synth_count + 1) * sizeof (*core_syms);
      core_syms = (asymbol **) xrealloc (core_syms, new_size);
      symp = core_syms + core_num_syms;
      core_num_syms += synth_count;
      for (i = 0; i < synth_count; i++)
	*symp++ = synthsyms + i;
      *symp = 0;
    }

  insn_boundary = 1;
  offset_to_code = 0;

  switch (bfd_get_arch (core_bfd))
    {
    case bfd_arch_vax:
      offset_to_code = 2;
      break;

    case bfd_arch_mips:/* and microMIPS */
    case bfd_arch_powerpc:/* and VLE */
    case bfd_arch_riscv:/* and RVC */
    case bfd_arch_sh:
      insn_boundary = 2;
      break;

    case bfd_arch_alpha:
      insn_boundary = 4;
      break;

    default:
      break;
    }

  return 0;
}


/* Return class of symbol SYM.  The returned class can be any of:
	0   -> symbol is not interesting to us
	'T' -> symbol is a global name
	't' -> symbol is a local (static) name.  */

static int
core_sym_class (asymbol *sym)
{
  symbol_info syminfo;
  const char *name;
  char sym_prefix;
  int i;

  if (sym->section == NULL || (sym->flags & BSF_DEBUGGING) != 0)
    return 0;

  bfd_get_symbol_info (core_bfd, sym, &syminfo);
  i = syminfo.type;

  if (i == 'T')
    return i;			/* It's a global symbol.  */

  if (i == 'W')
    /* Treat weak symbols as text symbols.  FIXME: a weak symbol may
       also be a data symbol.  */
    return 'T';

  if (i != 't')
    {
      /* Not a static text symbol.  */
      DBG (AOUTDEBUG, printf ("[core_sym_class] %s is of class %c\n",
			      sym->name, i));
      return 0;
    }


  /* Can't zero-length name or funny characters in name, where
     `funny' includes: `.' (.o file names) and `$' (Pascal labels).  */
  if (!sym->name || sym->name[0] == '\0')
    return 0;

  for (name = sym->name; *name; ++name)
    {
      if (*name == '$')
        return 0;

      while (*name == '.')
	{
	  /* Allow both nested subprograms (which end with ".NNN", where N is
	     a digit) and GCC cloned functions (which contain ".clone").
	     Allow for multiple iterations of both - apparently GCC can clone
	     clones and subprograms.  */
	  int digit_seen = 0;
#define CLONE_NAME	    ".clone."
#define CLONE_NAME_LEN	    strlen (CLONE_NAME)
#define CONSTPROP_NAME	    ".constprop."
#define CONSTPROP_NAME_LEN  strlen (CONSTPROP_NAME)

	  if (strlen (name) > CLONE_NAME_LEN
	      && strncmp (name, CLONE_NAME, CLONE_NAME_LEN) == 0)
	    name += CLONE_NAME_LEN - 1;

	  else if (strlen (name) > CONSTPROP_NAME_LEN
	      && strncmp (name, CONSTPROP_NAME, CONSTPROP_NAME_LEN) == 0)
	    name += CONSTPROP_NAME_LEN - 1;

	  for (name++; *name; name++)
	    if (digit_seen && *name == '.')
	      break;
	    else if (ISDIGIT (*name))
	      digit_seen = 1;
	    else
	      return 0;
	}
    }

  /* On systems where the C compiler adds an underscore to all
     names, static names without underscores seem usually to be
     labels in hand written assembler in the library.  We don't want
     these names.  This is certainly necessary on a Sparc running
     SunOS 4.1 (try profiling a program that does a lot of
     division). I don't know whether it has harmful side effects on
     other systems.  Perhaps it should be made configurable.  */
  sym_prefix = bfd_get_symbol_leading_char (core_bfd);

  if ((sym_prefix && sym_prefix != sym->name[0])
      /* GCC may add special symbols to help gdb figure out the file
	language.  We want to ignore these, since sometimes they mask
	the real function.  (dj@ctron)  */
      || !strncmp (sym->name, "__gnu_compiled", 14)
      || !strncmp (sym->name, "___gnu_compiled", 15))
    {
      return 0;
    }

  return 't';			/* It's a static text symbol.  */
}

/* Get whatever source info we can get regarding address ADDR.  */

static bool
get_src_info (bfd_vma addr, const char **filename, const char **name,
	      int *line_num)
{
  const char *fname = 0, *func_name = 0;
  int l = 0;

  if (bfd_find_nearest_line (core_bfd, core_text_sect, core_syms,
			     addr - core_text_sect->vma,
			     &fname, &func_name, (unsigned int *) &l)
      && fname && func_name && l)
    {
      DBG (AOUTDEBUG, printf ("[get_src_info] 0x%lx -> %s:%d (%s)\n",
			      (unsigned long) addr, fname, l, func_name));
      *filename = fname;
      *name = func_name;
      *line_num = l;
      return true;
    }
  else
    {
      DBG (AOUTDEBUG, printf ("[get_src_info] no info for 0x%lx (%s:%d,%s)\n",
			      (unsigned long) addr,
			      fname ? fname : "<unknown>", l,
			      func_name ? func_name : "<unknown>"));
      return false;
    }
}

static int
search_mapped_symbol (const void * l, const void * r)
{
    return strcmp ((const char *) l, ((const struct function_map *) r)->function_name);
}

Source_File *first_src_file = 0;

Source_File *
source_file_lookup_path (const char *path)
{
  Source_File *sf;

  for (sf = first_src_file; sf; sf = sf->next)
    {
      if (FILENAME_CMP (path, sf->name) == 0)
	break;
    }

  if (!sf)
    {
      /* Create a new source file descriptor.  */
      sf = (Source_File *) xmalloc (sizeof (*sf));

      memset (sf, 0, sizeof (*sf));

      sf->name = xstrdup (path);
      sf->next = first_src_file;
      first_src_file = sf;
    }

  return sf;
}

/* Read in symbol table from core.
   One symbol per function is entered.  */

static void
core_create_function_syms (const char *whoami)
{
  int cxxclass;
  long i;
  struct function_map * found = NULL;
  int core_has_func_syms = 0;
  Sym_Table *symtab = get_symtab_direct ();

  switch (core_bfd->xvec->flavour)
    {
    default:
      break;
    case bfd_target_coff_flavour:
    case bfd_target_ecoff_flavour:
    case bfd_target_xcoff_flavour:
    case bfd_target_elf_flavour:
    case bfd_target_som_flavour:
      core_has_func_syms = 1;
    }

  /* Pass 1 - determine upper bound on number of function names.  */
  symtab->len = 0;

  for (i = 0; i < core_num_syms; ++i)
    {
      if (!core_sym_class (core_syms[i]))
	continue;

      /* Don't create a symtab entry for a function that has
	 a mapping to a file, unless it's the first function
	 in the file.  */
      if (symbol_map_count != 0)
	{
	  /* Note: some systems (SunOS 5.8) crash if bsearch base argument
	     is NULL.  */
	  found = (struct function_map *) bsearch
	    (core_syms[i]->name, symbol_map, symbol_map_count,
	     sizeof (struct function_map), search_mapped_symbol);
	}
      if (found == NULL || found->is_first)
	++symtab->len;
    }

  if (symtab->len == 0)
    {
      fprintf (stderr, _("%s: file has no symbols\n"), whoami);
      done (1);
    }

  symtab->base = (Sym *) xmalloc (symtab->len * sizeof (Sym));

  /* Pass 2 - create symbols.  */
  symtab->limit = symtab->base;

  for (i = 0; i < core_num_syms; ++i)
    {
      asection *sym_sec;

      cxxclass = core_sym_class (core_syms[i]);

      if (!cxxclass)
	{
	  DBG (AOUTDEBUG,
	       printf ("[core_create_function_syms] rejecting: 0x%lx %s\n",
		       (unsigned long) core_syms[i]->value,
		       core_syms[i]->name));
	  continue;
	}

      if (symbol_map_count != 0)
	{
	  /* Note: some systems (SunOS 5.8) crash if bsearch base argument
	     is NULL.  */
	  found = (struct function_map *) bsearch
	    (core_syms[i]->name, symbol_map, symbol_map_count,
	     sizeof (struct function_map), search_mapped_symbol);
	}
      if (found && ! found->is_first)
	continue;

      sym_init (symtab->limit);

      /* Symbol offsets are always section-relative.  */
      sym_sec = core_syms[i]->section;
      symtab->limit->addr = core_syms[i]->value;
      if (sym_sec)
	symtab->limit->addr += bfd_section_vma (sym_sec);

      if (found)
	{
	  symtab->limit->name = found->file_name;
	  symtab->limit->mapped = 1;
	}
      else
	{
	  symtab->limit->name = core_syms[i]->name;
	  symtab->limit->mapped = 0;
	}

      /* Lookup filename and line number, if we can.  */
      {
	const char * filename;
	const char * func_name;

	if (get_src_info (symtab->limit->addr, & filename, & func_name,
			  & symtab->limit->line_num))
	  {
	    symtab->limit->file = source_file_lookup_path (filename);

	    /* FIXME: Checking __osf__ here does not work with a cross
	       gprof.  */
#ifdef __osf__
	    /* Suppress symbols that are not function names.  This is
	       useful to suppress code-labels and aliases.

	       This is known to be useful under DEC's OSF/1.  Under SunOS 4.x,
	       labels do not appear in the symbol table info, so this isn't
	       necessary.  */

	    if (strcmp (symtab->limit->name, func_name) != 0)
	      {
		/* The symbol's address maps to a different name, so
		   it can't be a function-entry point.  This happens
		   for labels, for example.  */
		DBG (AOUTDEBUG,
		     printf ("[core_create_function_syms: rej %s (maps to %s)\n",
			     symtab->limit->name, func_name));
		continue;
	      }
#endif
	  }
      }

      symtab->limit->is_func = (!core_has_func_syms
			       || (core_syms[i]->flags & BSF_FUNCTION) != 0);
      symtab->limit->is_bb_head = true;

      if (cxxclass == 't')
	symtab->limit->is_static = true;

      DBG (AOUTDEBUG, printf ("[core_create_function_syms] %ld %s 0x%lx\n",
			      (long) (symtab->limit - symtab->base),
			      symtab->limit->name,
			      (unsigned long) symtab->limit->addr));
      ++symtab->limit;
    }

  symtab->len = symtab->limit - symtab->base;
  symtab_finalize (symtab);
}


/* Initialize the symbol table.  */

void
symtab_init (const char *whoami)
{
  core_create_function_syms (whoami);
}
