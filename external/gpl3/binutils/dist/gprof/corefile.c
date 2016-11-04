/* corefile.c

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

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

#include "gprof.h"
#include "libiberty.h"
#include "filenames.h"
#include "search_list.h"
#include "source.h"
#include "symtab.h"
#include "hist.h"
#include "corefile.h"
#include "safe-ctype.h"

#include <stdlib.h>

bfd *core_bfd;
static int core_num_syms;
static asymbol **core_syms;
asection *core_text_sect;
void * core_text_space;

static int min_insn_size;
int offset_to_code;

/* For mapping symbols to specific .o files during file ordering.  */
struct function_map * symbol_map;
unsigned int symbol_map_count;

static void read_function_mappings (const char *);
static int core_sym_class (asymbol *);
static bfd_boolean get_src_info
  (bfd_vma, const char **, const char **, int *);

extern void i386_find_call  (Sym *, bfd_vma, bfd_vma);
extern void alpha_find_call (Sym *, bfd_vma, bfd_vma);
extern void vax_find_call   (Sym *, bfd_vma, bfd_vma);
extern void tahoe_find_call (Sym *, bfd_vma, bfd_vma);
extern void sparc_find_call (Sym *, bfd_vma, bfd_vma);
extern void mips_find_call  (Sym *, bfd_vma, bfd_vma);
extern void aarch64_find_call (Sym *, bfd_vma, bfd_vma);

static void
parse_error (const char *filename)
{
  fprintf (stderr, _("%s: unable to parse mapping file %s.\n"), whoami, filename);
  done (1);
}

/* Compare two function_map structs based on function name.
   We want to sort in ascending order.  */

static int
cmp_symbol_map (const void * l, const void * r)
{
  return strcmp (((struct function_map *) l)->function_name,
		 ((struct function_map *) r)->function_name);
}

static void
read_function_mappings (const char *filename)
{
  FILE * file = fopen (filename, "r");
  char dummy[1024];
  int count = 0;
  unsigned int i;

  if (!file)
    {
      fprintf (stderr, _("%s: could not open %s.\n"), whoami, filename);
      done (1);
    }

  /* First parse the mapping file so we know how big we need to
     make our tables.  We also do some sanity checks at this
     time.  */
  while (!feof (file))
    {
      int matches;

      matches = fscanf (file, "%[^\n:]", dummy);
      if (!matches)
	parse_error (filename);

      /* Just skip messages about files with no symbols.  */
      if (!strncmp (dummy, "No symbols in ", 14))
	{
	  matches = fscanf (file, "\n");
	  if (matches == EOF)
	    parse_error (filename);
	  continue;
	}

      /* Don't care what else is on this line at this point.  */
      matches = fscanf (file, "%[^\n]\n", dummy);
      if (!matches)
	parse_error (filename);
      count++;
    }

  /* Now we know how big we need to make our table.  */
  symbol_map = ((struct function_map *)
		xmalloc (count * sizeof (struct function_map)));

  /* Rewind the input file so we can read it again.  */
  rewind (file);

  /* Read each entry and put it into the table.  */
  count = 0;
  while (!feof (file))
    {
      int matches;
      char *tmp;

      matches = fscanf (file, "%[^\n:]", dummy);
      if (!matches)
	parse_error (filename);

      /* Just skip messages about files with no symbols.  */
      if (!strncmp (dummy, "No symbols in ", 14))
	{
	  matches = fscanf (file, "\n");
	  if (matches == EOF)
	    parse_error (filename);
	  continue;
	}

      /* dummy has the filename, go ahead and copy it.  */
      symbol_map[count].file_name = (char *) xmalloc (strlen (dummy) + 1);
      strcpy (symbol_map[count].file_name, dummy);

      /* Now we need the function name.  */
      matches = fscanf (file, "%[^\n]\n", dummy);
      if (!matches)
	parse_error (filename);
      tmp = strrchr (dummy, ' ') + 1;
      symbol_map[count].function_name = (char *) xmalloc (strlen (tmp) + 1);
      strcpy (symbol_map[count].function_name, tmp);
      count++;
    }

  /* Record the size of the map table for future reference.  */
  symbol_map_count = count;

  for (i = 0; i < symbol_map_count; ++i)
    if (i == 0
        || filename_cmp (symbol_map[i].file_name, symbol_map[i - 1].file_name))
      symbol_map[i].is_first = 1;

  qsort (symbol_map, symbol_map_count, sizeof (struct function_map), cmp_symbol_map);

  fclose (file);
}

void
core_init (const char * aout_name)
{
  int core_sym_bytes;
  asymbol *synthsyms;
  long synth_count;

  core_bfd = bfd_openr (aout_name, 0);

  if (!core_bfd)
    {
      perror (aout_name);
      done (1);
    }

  if (!bfd_check_format (core_bfd, bfd_object))
    {
      fprintf (stderr, _("%s: %s: not in executable format\n"), whoami, aout_name);
      done (1);
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
	  done (1);
	}
    }

  /* Read core's symbol table.  */

  /* This will probably give us more than we need, but that's ok.  */
  core_sym_bytes = bfd_get_symtab_upper_bound (core_bfd);
  if (core_sym_bytes < 0)
    {
      fprintf (stderr, "%s: %s: %s\n", whoami, aout_name,
	       bfd_errmsg (bfd_get_error ()));
      done (1);
    }

  core_syms = (asymbol **) xmalloc (core_sym_bytes);
  core_num_syms = bfd_canonicalize_symtab (core_bfd, core_syms);

  if (core_num_syms < 0)
    {
      fprintf (stderr, "%s: %s: %s\n", whoami, aout_name,
	       bfd_errmsg (bfd_get_error ()));
      done (1);
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

  min_insn_size = 1;
  offset_to_code = 0;

  switch (bfd_get_arch (core_bfd))
    {
    case bfd_arch_vax:
    case bfd_arch_tahoe:
      offset_to_code = 2;
      break;

    case bfd_arch_alpha:
      min_insn_size = 4;
      break;

    default:
      break;
    }

  if (function_mapping_file)
    read_function_mappings (function_mapping_file);
}

/* Read in the text space of an a.out file.  */

void
core_get_text_space (bfd *cbfd)
{
  core_text_space = malloc (bfd_get_section_size (core_text_sect));

  if (!core_text_space)
    {
      fprintf (stderr, _("%s: ran out room for %lu bytes of text space\n"),
	       whoami, (unsigned long) bfd_get_section_size (core_text_sect));
      done (1);
    }

  if (!bfd_get_section_contents (cbfd, core_text_sect, core_text_space,
				 0, bfd_get_section_size (core_text_sect)))
    {
      bfd_perror ("bfd_get_section_contents");
      free (core_text_space);
      core_text_space = 0;
    }

  if (!core_text_space)
    fprintf (stderr, _("%s: can't do -c\n"), whoami);
}


void
find_call (Sym *parent, bfd_vma p_lowpc, bfd_vma p_highpc)
{
  if (core_text_space == 0)
    return;

  hist_clip_symbol_address (&p_lowpc, &p_highpc);

  switch (bfd_get_arch (core_bfd))
    {
    case bfd_arch_i386:
      i386_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_alpha:
      alpha_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_vax:
      vax_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_sparc:
      sparc_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_tahoe:
      tahoe_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_mips:
      mips_find_call (parent, p_lowpc, p_highpc);
      break;

    case bfd_arch_aarch64:
      aarch64_find_call (parent, p_lowpc, p_highpc);
      break;

    default:
      fprintf (stderr, _("%s: -c not supported on architecture %s\n"),
	       whoami, bfd_printable_name(core_bfd));

      /* Don't give the error more than once.  */
      ignore_direct_calls = FALSE;
    }
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

  /* Must be a text symbol, and static text symbols
     don't qualify if ignore_static_funcs set.   */
  if (ignore_static_funcs && (sym->flags & BSF_LOCAL))
    {
      DBG (AOUTDEBUG, printf ("[core_sym_class] %s: not a function\n",
			      sym->name));
      return 0;
    }

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

  /* Do some more filtering on static function-names.  */
  if (ignore_static_funcs)
    return 0;

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

  /* If the object file supports marking of function symbols, then
     we can zap anything that doesn't have BSF_FUNCTION set.  */
  if (ignore_non_functions && (sym->flags & BSF_FUNCTION) == 0)
    return 0;

  return 't';			/* It's a static text symbol.  */
}

/* Get whatever source info we can get regarding address ADDR.  */

static bfd_boolean
get_src_info (bfd_vma addr, const char **filename, const char **name, int *line_num)
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
      return TRUE;
    }
  else
    {
      DBG (AOUTDEBUG, printf ("[get_src_info] no info for 0x%lx (%s:%d,%s)\n",
			      (unsigned long) addr,
			      fname ? fname : "<unknown>", l,
			      func_name ? func_name : "<unknown>"));
      return FALSE;
    }
}

/* Return number of symbols in a symbol-table file.  */

static int
num_of_syms_in (FILE * f)
{
  const int BUFSIZE = 1024;
  char * buf = (char *) xmalloc (BUFSIZE);
  char * address = (char *) xmalloc (BUFSIZE);
  char   type;
  char * name = (char *) xmalloc (BUFSIZE);
  int num = 0;

  while (!feof (f) && fgets (buf, BUFSIZE - 1, f))
    {
      if (sscanf (buf, "%s %c %s", address, &type, name) == 3)
        if (type == 't' || type == 'T')
          ++num;
    }

  free (buf);
  free (address);
  free (name);

  return num;
}

/* Read symbol table from a file.  */

void
core_create_syms_from (const char * sym_table_file)
{
  const int BUFSIZE = 1024;
  char * buf = (char *) xmalloc (BUFSIZE);
  char * address = (char *) xmalloc (BUFSIZE);
  char type;
  char * name = (char *) xmalloc (BUFSIZE);
  bfd_vma min_vma = ~(bfd_vma) 0;
  bfd_vma max_vma = 0;
  FILE * f;

  f = fopen (sym_table_file, "r");
  if (!f)
    {
      fprintf (stderr, _("%s: could not open %s.\n"), whoami, sym_table_file);
      done (1);
    }

  /* Pass 1 - determine upper bound on number of function names.  */
  symtab.len = num_of_syms_in (f);

  if (symtab.len == 0)
    {
      fprintf (stderr, _("%s: file `%s' has no symbols\n"), whoami, sym_table_file);
      done (1);
    }

  symtab.base = (Sym *) xmalloc (symtab.len * sizeof (Sym));

  /* Pass 2 - create symbols.  */
  symtab.limit = symtab.base;

  if (fseek (f, 0, SEEK_SET) != 0)
    {
      perror (sym_table_file);
      done (1);
    }

  while (!feof (f) && fgets (buf, BUFSIZE - 1, f))
    {
      if (sscanf (buf, "%s %c %s", address, &type, name) == 3)
        if (type != 't' && type != 'T')
          continue;

      sym_init (symtab.limit);

      sscanf (address, "%" BFD_VMA_FMT "x", &(symtab.limit->addr) );

      symtab.limit->name = (char *) xmalloc (strlen (name) + 1);
      strcpy ((char *) symtab.limit->name, name);
      symtab.limit->mapped = 0;
      symtab.limit->is_func = TRUE;
      symtab.limit->is_bb_head = TRUE;
      symtab.limit->is_static = (type == 't');
      min_vma = MIN (symtab.limit->addr, min_vma);
      max_vma = MAX (symtab.limit->addr, max_vma);

      ++symtab.limit;
    }
  fclose (f);

  symtab.len = symtab.limit - symtab.base;
  symtab_finalize (&symtab);

  free (buf);
  free (address);
  free (name);
}

static int
search_mapped_symbol (const void * l, const void * r)
{
    return strcmp ((const char *) l, ((const struct function_map *) r)->function_name);
}

/* Read in symbol table from core.
   One symbol per function is entered.  */

void
core_create_function_syms (void)
{
  bfd_vma min_vma = ~ (bfd_vma) 0;
  bfd_vma max_vma = 0;
  int cxxclass;
  long i;
  struct function_map * found = NULL;
  int core_has_func_syms = 0;

  switch (core_bfd->xvec->flavour)
    {
    default:
      break;
    case bfd_target_coff_flavour:
    case bfd_target_ecoff_flavour:
    case bfd_target_xcoff_flavour:
    case bfd_target_elf_flavour:
    case bfd_target_nlm_flavour:
    case bfd_target_som_flavour:
      core_has_func_syms = 1;
    }

  /* Pass 1 - determine upper bound on number of function names.  */
  symtab.len = 0;

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
	++symtab.len;
    }

  if (symtab.len == 0)
    {
      fprintf (stderr, _("%s: file `%s' has no symbols\n"), whoami, a_out_name);
      done (1);
    }

  symtab.base = (Sym *) xmalloc (symtab.len * sizeof (Sym));

  /* Pass 2 - create symbols.  */
  symtab.limit = symtab.base;

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

      sym_init (symtab.limit);

      /* Symbol offsets are always section-relative.  */
      sym_sec = core_syms[i]->section;
      symtab.limit->addr = core_syms[i]->value;
      if (sym_sec)
	symtab.limit->addr += bfd_get_section_vma (sym_sec->owner, sym_sec);

      if (found)
	{
	  symtab.limit->name = found->file_name;
	  symtab.limit->mapped = 1;
	}
      else
	{
	  symtab.limit->name = core_syms[i]->name;
	  symtab.limit->mapped = 0;
	}

      /* Lookup filename and line number, if we can.  */
      {
	const char * filename;
	const char * func_name;

	if (get_src_info (symtab.limit->addr, & filename, & func_name,
			  & symtab.limit->line_num))
	  {
	    symtab.limit->file = source_file_lookup_path (filename);

	    /* FIXME: Checking __osf__ here does not work with a cross
	       gprof.  */
#ifdef __osf__
	    /* Suppress symbols that are not function names.  This is
	       useful to suppress code-labels and aliases.

	       This is known to be useful under DEC's OSF/1.  Under SunOS 4.x,
	       labels do not appear in the symbol table info, so this isn't
	       necessary.  */

	    if (strcmp (symtab.limit->name, func_name) != 0)
	      {
		/* The symbol's address maps to a different name, so
		   it can't be a function-entry point.  This happens
		   for labels, for example.  */
		DBG (AOUTDEBUG,
		     printf ("[core_create_function_syms: rej %s (maps to %s)\n",
			     symtab.limit->name, func_name));
		continue;
	      }
#endif
	  }
      }

      symtab.limit->is_func = (!core_has_func_syms
			       || (core_syms[i]->flags & BSF_FUNCTION) != 0);
      symtab.limit->is_bb_head = TRUE;

      if (cxxclass == 't')
	symtab.limit->is_static = TRUE;

      /* Keep track of the minimum and maximum vma addresses used by all
	 symbols.  When computing the max_vma, use the ending address of the
	 section containing the symbol, if available.  */
      min_vma = MIN (symtab.limit->addr, min_vma);
      if (sym_sec)
	max_vma = MAX (bfd_get_section_vma (sym_sec->owner, sym_sec)
		       + bfd_section_size (sym_sec->owner, sym_sec) - 1,
		       max_vma);
      else
	max_vma = MAX (symtab.limit->addr, max_vma);

      DBG (AOUTDEBUG, printf ("[core_create_function_syms] %ld %s 0x%lx\n",
			      (long) (symtab.limit - symtab.base),
			      symtab.limit->name,
			      (unsigned long) symtab.limit->addr));
      ++symtab.limit;
    }

  symtab.len = symtab.limit - symtab.base;
  symtab_finalize (&symtab);
}

/* Read in symbol table from core.
   One symbol per line of source code is entered.  */

void
core_create_line_syms (void)
{
  char *prev_name, *prev_filename;
  unsigned int prev_name_len, prev_filename_len;
  bfd_vma vma, min_vma = ~(bfd_vma) 0, max_vma = 0;
  Sym *prev, dummy, *sym;
  const char *filename;
  int prev_line_num;
  Sym_Table ltab;
  bfd_vma vma_high;

  /* Create symbols for functions as usual.  This is necessary in
     cases where parts of a program were not compiled with -g.  For
     those parts we still want to get info at the function level.  */
  core_create_function_syms ();

  /* Pass 1: count the number of symbols.  */

  /* To find all line information, walk through all possible
     text-space addresses (one by one!) and get the debugging
     info for each address.  When the debugging info changes,
     it is time to create a new symbol.

     Of course, this is rather slow and it would be better if
     BFD would provide an iterator for enumerating all line infos.  */
  prev_name_len = PATH_MAX;
  prev_filename_len = PATH_MAX;
  prev_name = (char *) xmalloc (prev_name_len);
  prev_filename = (char *) xmalloc (prev_filename_len);
  ltab.len = 0;
  prev_line_num = 0;

  vma_high = core_text_sect->vma + bfd_get_section_size (core_text_sect);
  for (vma = core_text_sect->vma; vma < vma_high; vma += min_insn_size)
    {
      unsigned int len;

      if (!get_src_info (vma, &filename, &dummy.name, &dummy.line_num)
	  || (prev_line_num == dummy.line_num
	      && prev_name != NULL
	      && strcmp (prev_name, dummy.name) == 0
	      && filename_cmp (prev_filename, filename) == 0))
	continue;

      ++ltab.len;
      prev_line_num = dummy.line_num;

      len = strlen (dummy.name);
      if (len >= prev_name_len)
	{
	  prev_name_len = len + 1024;
	  free (prev_name);
	  prev_name = (char *) xmalloc (prev_name_len);
	}

      strcpy (prev_name, dummy.name);
      len = strlen (filename);

      if (len >= prev_filename_len)
	{
	  prev_filename_len = len + 1024;
	  free (prev_filename);
	  prev_filename = (char *) xmalloc (prev_filename_len);
	}

      strcpy (prev_filename, filename);

      min_vma = MIN (vma, min_vma);
      max_vma = MAX (vma, max_vma);
    }

  free (prev_name);
  free (prev_filename);

  /* Make room for function symbols, too.  */
  ltab.len += symtab.len;
  ltab.base = (Sym *) xmalloc (ltab.len * sizeof (Sym));
  ltab.limit = ltab.base;

  /* Pass 2 - create symbols.  */

  /* We now set is_static as we go along, rather than by running
     through the symbol table at the end.

     The old way called symtab_finalize before the is_static pass,
     causing a problem since symtab_finalize uses is_static as part of
     its address conflict resolution algorithm.  Since global symbols
     were prefered over static symbols, and all line symbols were
     global at that point, static function names that conflicted with
     their own line numbers (static, but labeled as global) were
     rejected in favor of the line num.

     This was not the desired functionality.  We always want to keep
     our function symbols and discard any conflicting line symbols.
     Perhaps symtab_finalize should be modified to make this
     distinction as well, but the current fix works and the code is a
     lot cleaner now.  */
  prev = 0;

  for (vma = core_text_sect->vma; vma < vma_high; vma += min_insn_size)
    {
      sym_init (ltab.limit);

      if (!get_src_info (vma, &filename, &ltab.limit->name, &ltab.limit->line_num)
	  || (prev && prev->line_num == ltab.limit->line_num
	      && strcmp (prev->name, ltab.limit->name) == 0
	      && filename_cmp (prev->file->name, filename) == 0))
	continue;

      /* Make name pointer a malloc'ed string.  */
      ltab.limit->name = xstrdup (ltab.limit->name);
      ltab.limit->file = source_file_lookup_path (filename);

      ltab.limit->addr = vma;

      /* Set is_static based on the enclosing function, using either:
	 1) the previous symbol, if it's from the same function, or
	 2) a symtab lookup.  */
      if (prev && ltab.limit->file == prev->file &&
	  strcmp (ltab.limit->name, prev->name) == 0)
	{
	  ltab.limit->is_static = prev->is_static;
	}
      else
	{
	  sym = sym_lookup(&symtab, ltab.limit->addr);
          if (sym)
	    ltab.limit->is_static = sym->is_static;
	}

      prev = ltab.limit;

      DBG (AOUTDEBUG, printf ("[core_create_line_syms] %lu %s 0x%lx\n",
			      (unsigned long) (ltab.limit - ltab.base),
			      ltab.limit->name,
			      (unsigned long) ltab.limit->addr));
      ++ltab.limit;
    }

  /* Copy in function symbols.  */
  memcpy (ltab.limit, symtab.base, symtab.len * sizeof (Sym));
  ltab.limit += symtab.len;

  if ((unsigned int) (ltab.limit - ltab.base) != ltab.len)
    {
      fprintf (stderr,
	       _("%s: somebody miscounted: ltab.len=%d instead of %ld\n"),
	       whoami, ltab.len, (long) (ltab.limit - ltab.base));
      done (1);
    }

  /* Finalize ltab and make it symbol table.  */
  symtab_finalize (&ltab);
  free (symtab.base);
  symtab = ltab;
}
