/* Do various things to symbol tables (other than lookup), for GDB.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "bfd.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "breakpoint.h"
#include "command.h"
#include "gdb_obstack.h"
#include "language.h"
#include "bcache.h"
#include "block.h"
#include "gdb_regex.h"
#include <sys/stat.h>
#include "dictionary.h"
#include "typeprint.h"
#include "gdbcmd.h"
#include "source.h"
#include "readline/readline.h"

#include "psymtab.h"

#ifndef DEV_TTY
#define DEV_TTY "/dev/tty"
#endif

/* Unfortunately for debugging, stderr is usually a macro.  This is painful
   when calling functions that take FILE *'s from the debugger.
   So we make a variable which has the same value and which is accessible when
   debugging GDB with itself.  Because stdin et al need not be constants,
   we initialize them in the _initialize_symmisc function at the bottom
   of the file.  */
FILE *std_in;
FILE *std_out;
FILE *std_err;

/* Prototypes for local functions */

static int block_depth (struct block *);

void _initialize_symmisc (void);

struct print_symbol_args
  {
    struct gdbarch *gdbarch;
    struct symbol *symbol;
    int depth;
    struct ui_file *outfile;
  };

static int print_symbol (void *);


void
print_symbol_bcache_statistics (void)
{
  struct program_space *pspace;
  struct objfile *objfile;

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
  {
    QUIT;
    printf_filtered (_("Byte cache statistics for '%s':\n"),
		     objfile_name (objfile));
    print_bcache_statistics (psymbol_bcache_get_bcache (objfile->psymbol_cache),
                             "partial symbol cache");
    print_bcache_statistics (objfile->per_bfd->macro_cache,
			     "preprocessor macro cache");
    print_bcache_statistics (objfile->per_bfd->filename_cache,
			     "file name cache");
  }
}

void
print_objfile_statistics (void)
{
  struct program_space *pspace;
  struct objfile *objfile;
  struct compunit_symtab *cu;
  struct symtab *s;
  int i, linetables, blockvectors;

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
  {
    QUIT;
    printf_filtered (_("Statistics for '%s':\n"), objfile_name (objfile));
    if (OBJSTAT (objfile, n_stabs) > 0)
      printf_filtered (_("  Number of \"stab\" symbols read: %d\n"),
		       OBJSTAT (objfile, n_stabs));
    if (objfile->per_bfd->n_minsyms > 0)
      printf_filtered (_("  Number of \"minimal\" symbols read: %d\n"),
		       objfile->per_bfd->n_minsyms);
    if (OBJSTAT (objfile, n_psyms) > 0)
      printf_filtered (_("  Number of \"partial\" symbols read: %d\n"),
		       OBJSTAT (objfile, n_psyms));
    if (OBJSTAT (objfile, n_syms) > 0)
      printf_filtered (_("  Number of \"full\" symbols read: %d\n"),
		       OBJSTAT (objfile, n_syms));
    if (OBJSTAT (objfile, n_types) > 0)
      printf_filtered (_("  Number of \"types\" defined: %d\n"),
		       OBJSTAT (objfile, n_types));
    if (objfile->sf)
      objfile->sf->qf->print_stats (objfile);
    i = linetables = blockvectors = 0;
    ALL_OBJFILE_FILETABS (objfile, cu, s)
      {
        i++;
        if (SYMTAB_LINETABLE (s) != NULL)
          linetables++;
      }
    ALL_OBJFILE_COMPUNITS (objfile, cu)
      blockvectors++;
    printf_filtered (_("  Number of symbol tables: %d\n"), i);
    printf_filtered (_("  Number of symbol tables with line tables: %d\n"),
                     linetables);
    printf_filtered (_("  Number of symbol tables with blockvectors: %d\n"),
                     blockvectors);

    if (OBJSTAT (objfile, sz_strtab) > 0)
      printf_filtered (_("  Space used by a.out string tables: %d\n"),
		       OBJSTAT (objfile, sz_strtab));
    printf_filtered (_("  Total memory used for objfile obstack: %s\n"),
		     pulongest (obstack_memory_used (&objfile
						     ->objfile_obstack)));
    printf_filtered (_("  Total memory used for BFD obstack: %s\n"),
		     pulongest (obstack_memory_used (&objfile->per_bfd
						     ->storage_obstack)));
    printf_filtered (_("  Total memory used for psymbol cache: %d\n"),
		     bcache_memory_used (psymbol_bcache_get_bcache
		                          (objfile->psymbol_cache)));
    printf_filtered (_("  Total memory used for macro cache: %d\n"),
		     bcache_memory_used (objfile->per_bfd->macro_cache));
    printf_filtered (_("  Total memory used for file name cache: %d\n"),
		     bcache_memory_used (objfile->per_bfd->filename_cache));
  }
}

static void
dump_objfile (struct objfile *objfile)
{
  struct compunit_symtab *cust;
  struct symtab *symtab;

  printf_filtered ("\nObject file %s:  ", objfile_name (objfile));
  printf_filtered ("Objfile at ");
  gdb_print_host_address (objfile, gdb_stdout);
  printf_filtered (", bfd at ");
  gdb_print_host_address (objfile->obfd, gdb_stdout);
  printf_filtered (", %d minsyms\n\n",
		   objfile->per_bfd->minimal_symbol_count);

  if (objfile->sf)
    objfile->sf->qf->dump (objfile);

  if (objfile->compunit_symtabs != NULL)
    {
      printf_filtered ("Symtabs:\n");
      ALL_OBJFILE_FILETABS (objfile, cust, symtab)
	{
	  printf_filtered ("%s at ", symtab_to_filename_for_display (symtab));
	  gdb_print_host_address (symtab, gdb_stdout);
	  printf_filtered (", ");
	  if (SYMTAB_OBJFILE (symtab) != objfile)
	    {
	      printf_filtered ("NOT ON CHAIN!  ");
	    }
	  wrap_here ("  ");
	}
      printf_filtered ("\n\n");
    }
}

/* Print minimal symbols from this objfile.  */

static void
dump_msymbols (struct objfile *objfile, struct ui_file *outfile)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct minimal_symbol *msymbol;
  int index;
  char ms_type;

  fprintf_filtered (outfile, "\nObject file %s:\n\n", objfile_name (objfile));
  if (objfile->per_bfd->minimal_symbol_count == 0)
    {
      fprintf_filtered (outfile, "No minimal symbols found.\n");
      return;
    }
  index = 0;
  ALL_OBJFILE_MSYMBOLS (objfile, msymbol)
    {
      struct obj_section *section = MSYMBOL_OBJ_SECTION (objfile, msymbol);

      switch (MSYMBOL_TYPE (msymbol))
	{
	case mst_unknown:
	  ms_type = 'u';
	  break;
	case mst_text:
	  ms_type = 'T';
	  break;
	case mst_text_gnu_ifunc:
	  ms_type = 'i';
	  break;
	case mst_solib_trampoline:
	  ms_type = 'S';
	  break;
	case mst_data:
	  ms_type = 'D';
	  break;
	case mst_bss:
	  ms_type = 'B';
	  break;
	case mst_abs:
	  ms_type = 'A';
	  break;
	case mst_file_text:
	  ms_type = 't';
	  break;
	case mst_file_data:
	  ms_type = 'd';
	  break;
	case mst_file_bss:
	  ms_type = 'b';
	  break;
	default:
	  ms_type = '?';
	  break;
	}
      fprintf_filtered (outfile, "[%2d] %c ", index, ms_type);
      fputs_filtered (paddress (gdbarch, MSYMBOL_VALUE_ADDRESS (objfile,
								msymbol)),
		      outfile);
      fprintf_filtered (outfile, " %s", MSYMBOL_LINKAGE_NAME (msymbol));
      if (section)
	{
	  if (section->the_bfd_section != NULL)
	    fprintf_filtered (outfile, " section %s",
			      bfd_section_name (objfile->obfd,
						section->the_bfd_section));
	  else
	    fprintf_filtered (outfile, " spurious section %ld",
			      (long) (section - objfile->sections));
	}
      if (MSYMBOL_DEMANGLED_NAME (msymbol) != NULL)
	{
	  fprintf_filtered (outfile, "  %s", MSYMBOL_DEMANGLED_NAME (msymbol));
	}
      if (msymbol->filename)
	fprintf_filtered (outfile, "  %s", msymbol->filename);
      fputs_filtered ("\n", outfile);
      index++;
    }
  if (objfile->per_bfd->minimal_symbol_count != index)
    {
      warning (_("internal error:  minimal symbol count %d != %d"),
	       objfile->per_bfd->minimal_symbol_count, index);
    }
  fprintf_filtered (outfile, "\n");
}

static void
dump_symtab_1 (struct symtab *symtab, struct ui_file *outfile)
{
  struct objfile *objfile = SYMTAB_OBJFILE (symtab);
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  int i;
  struct dict_iterator iter;
  int len;
  struct linetable *l;
  const struct blockvector *bv;
  struct symbol *sym;
  struct block *b;
  int depth;

  fprintf_filtered (outfile, "\nSymtab for file %s\n",
		    symtab_to_filename_for_display (symtab));
  if (SYMTAB_DIRNAME (symtab) != NULL)
    fprintf_filtered (outfile, "Compilation directory is %s\n",
		      SYMTAB_DIRNAME (symtab));
  fprintf_filtered (outfile, "Read from object file %s (",
		    objfile_name (objfile));
  gdb_print_host_address (objfile, outfile);
  fprintf_filtered (outfile, ")\n");
  fprintf_filtered (outfile, "Language: %s\n",
		    language_str (symtab->language));

  /* First print the line table.  */
  l = SYMTAB_LINETABLE (symtab);
  if (l)
    {
      fprintf_filtered (outfile, "\nLine table:\n\n");
      len = l->nitems;
      for (i = 0; i < len; i++)
	{
	  fprintf_filtered (outfile, " line %d at ", l->item[i].line);
	  fputs_filtered (paddress (gdbarch, l->item[i].pc), outfile);
	  fprintf_filtered (outfile, "\n");
	}
    }
  /* Now print the block info, but only for compunit symtabs since we will
     print lots of duplicate info otherwise.  */
  if (symtab == COMPUNIT_FILETABS (SYMTAB_COMPUNIT (symtab)))
    {
      fprintf_filtered (outfile, "\nBlockvector:\n\n");
      bv = SYMTAB_BLOCKVECTOR (symtab);
      len = BLOCKVECTOR_NBLOCKS (bv);
      for (i = 0; i < len; i++)
	{
	  b = BLOCKVECTOR_BLOCK (bv, i);
	  depth = block_depth (b) * 2;
	  print_spaces (depth, outfile);
	  fprintf_filtered (outfile, "block #%03d, object at ", i);
	  gdb_print_host_address (b, outfile);
	  if (BLOCK_SUPERBLOCK (b))
	    {
	      fprintf_filtered (outfile, " under ");
	      gdb_print_host_address (BLOCK_SUPERBLOCK (b), outfile);
	    }
	  /* drow/2002-07-10: We could save the total symbols count
	     even if we're using a hashtable, but nothing else but this message
	     wants it.  */
	  fprintf_filtered (outfile, ", %d syms/buckets in ",
			    dict_size (BLOCK_DICT (b)));
	  fputs_filtered (paddress (gdbarch, BLOCK_START (b)), outfile);
	  fprintf_filtered (outfile, "..");
	  fputs_filtered (paddress (gdbarch, BLOCK_END (b)), outfile);
	  if (BLOCK_FUNCTION (b))
	    {
	      fprintf_filtered (outfile, ", function %s",
				SYMBOL_LINKAGE_NAME (BLOCK_FUNCTION (b)));
	      if (SYMBOL_DEMANGLED_NAME (BLOCK_FUNCTION (b)) != NULL)
		{
		  fprintf_filtered (outfile, ", %s",
				SYMBOL_DEMANGLED_NAME (BLOCK_FUNCTION (b)));
		}
	    }
	  fprintf_filtered (outfile, "\n");
	  /* Now print each symbol in this block (in no particular order, if
	     we're using a hashtable).  Note that we only want this
	     block, not any blocks from included symtabs.  */
	  ALL_DICT_SYMBOLS (BLOCK_DICT (b), iter, sym)
	    {
	      struct print_symbol_args s;

	      s.gdbarch = gdbarch;
	      s.symbol = sym;
	      s.depth = depth + 1;
	      s.outfile = outfile;
	      catch_errors (print_symbol, &s, "Error printing symbol:\n",
			    RETURN_MASK_ERROR);
	    }
	}
      fprintf_filtered (outfile, "\n");
    }
  else
    {
      const char *compunit_filename
	= symtab_to_filename_for_display (COMPUNIT_FILETABS (SYMTAB_COMPUNIT (symtab)));

      fprintf_filtered (outfile,
			"\nBlockvector same as owning compunit: %s\n\n",
			compunit_filename);
    }
}

static void
dump_symtab (struct symtab *symtab, struct ui_file *outfile)
{
  /* Set the current language to the language of the symtab we're dumping
     because certain routines used during dump_symtab() use the current
     language to print an image of the symbol.  We'll restore it later.
     But use only real languages, not placeholders.  */
  if (symtab->language != language_unknown
      && symtab->language != language_auto)
    {
      enum language saved_lang;

      saved_lang = set_language (symtab->language);

      dump_symtab_1 (symtab, outfile);

      set_language (saved_lang);
    }
  else
    dump_symtab_1 (symtab, outfile);
}

static void
maintenance_print_symbols (char *args, int from_tty)
{
  char **argv;
  struct ui_file *outfile;
  struct cleanup *cleanups;
  char *symname = NULL;
  char *filename = DEV_TTY;
  struct objfile *objfile;
  struct compunit_symtab *cu;
  struct symtab *s;

  dont_repeat ();

  if (args == NULL)
    {
      error (_("Arguments missing: an output file name "
	       "and an optional symbol file name"));
    }
  argv = gdb_buildargv (args);
  cleanups = make_cleanup_freeargv (argv);

  if (argv[0] != NULL)
    {
      filename = argv[0];
      /* If a second arg is supplied, it is a source file name to match on.  */
      if (argv[1] != NULL)
	{
	  symname = argv[1];
	}
    }

  filename = tilde_expand (filename);
  make_cleanup (xfree, filename);

  outfile = gdb_fopen (filename, FOPEN_WT);
  if (outfile == 0)
    perror_with_name (filename);
  make_cleanup_ui_file_delete (outfile);

  ALL_FILETABS (objfile, cu, s)
    {
      QUIT;
      if (symname == NULL
	  || filename_cmp (symname, symtab_to_filename_for_display (s)) == 0)
	dump_symtab (s, outfile);
    }
  do_cleanups (cleanups);
}

/* Print symbol ARGS->SYMBOL on ARGS->OUTFILE.  ARGS->DEPTH says how
   far to indent.  ARGS is really a struct print_symbol_args *, but is
   declared as char * to get it past catch_errors.  Returns 0 for error,
   1 for success.  */

static int
print_symbol (void *args)
{
  struct gdbarch *gdbarch = ((struct print_symbol_args *) args)->gdbarch;
  struct symbol *symbol = ((struct print_symbol_args *) args)->symbol;
  int depth = ((struct print_symbol_args *) args)->depth;
  struct ui_file *outfile = ((struct print_symbol_args *) args)->outfile;
  struct obj_section *section;

  if (SYMBOL_OBJFILE_OWNED (symbol))
    section = SYMBOL_OBJ_SECTION (symbol_objfile (symbol), symbol);
  else
    section = NULL;

  print_spaces (depth, outfile);
  if (SYMBOL_DOMAIN (symbol) == LABEL_DOMAIN)
    {
      fprintf_filtered (outfile, "label %s at ", SYMBOL_PRINT_NAME (symbol));
      fputs_filtered (paddress (gdbarch, SYMBOL_VALUE_ADDRESS (symbol)),
		      outfile);
      if (section)
	fprintf_filtered (outfile, " section %s\n",
			  bfd_section_name (section->the_bfd_section->owner,
					    section->the_bfd_section));
      else
	fprintf_filtered (outfile, "\n");
      return 1;
    }
  if (SYMBOL_DOMAIN (symbol) == STRUCT_DOMAIN)
    {
      if (TYPE_TAG_NAME (SYMBOL_TYPE (symbol)))
	{
	  LA_PRINT_TYPE (SYMBOL_TYPE (symbol), "", outfile, 1, depth,
			 &type_print_raw_options);
	}
      else
	{
	  fprintf_filtered (outfile, "%s %s = ",
			 (TYPE_CODE (SYMBOL_TYPE (symbol)) == TYPE_CODE_ENUM
			  ? "enum"
		     : (TYPE_CODE (SYMBOL_TYPE (symbol)) == TYPE_CODE_STRUCT
			? "struct" : "union")),
			    SYMBOL_LINKAGE_NAME (symbol));
	  LA_PRINT_TYPE (SYMBOL_TYPE (symbol), "", outfile, 1, depth,
			 &type_print_raw_options);
	}
      fprintf_filtered (outfile, ";\n");
    }
  else
    {
      if (SYMBOL_CLASS (symbol) == LOC_TYPEDEF)
	fprintf_filtered (outfile, "typedef ");
      if (SYMBOL_TYPE (symbol))
	{
	  /* Print details of types, except for enums where it's clutter.  */
	  LA_PRINT_TYPE (SYMBOL_TYPE (symbol), SYMBOL_PRINT_NAME (symbol),
			 outfile,
			 TYPE_CODE (SYMBOL_TYPE (symbol)) != TYPE_CODE_ENUM,
			 depth,
			 &type_print_raw_options);
	  fprintf_filtered (outfile, "; ");
	}
      else
	fprintf_filtered (outfile, "%s ", SYMBOL_PRINT_NAME (symbol));

      switch (SYMBOL_CLASS (symbol))
	{
	case LOC_CONST:
	  fprintf_filtered (outfile, "const %s (%s)",
			    plongest (SYMBOL_VALUE (symbol)),
			    hex_string (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_CONST_BYTES:
	  {
	    unsigned i;
	    struct type *type = check_typedef (SYMBOL_TYPE (symbol));

	    fprintf_filtered (outfile, "const %u hex bytes:",
			      TYPE_LENGTH (type));
	    for (i = 0; i < TYPE_LENGTH (type); i++)
	      fprintf_filtered (outfile, " %02x",
				(unsigned) SYMBOL_VALUE_BYTES (symbol)[i]);
	  }
	  break;

	case LOC_STATIC:
	  fprintf_filtered (outfile, "static at ");
	  fputs_filtered (paddress (gdbarch, SYMBOL_VALUE_ADDRESS (symbol)),
			  outfile);
	  if (section)
	    fprintf_filtered (outfile, " section %s",
			      bfd_section_name (section->the_bfd_section->owner,
						section->the_bfd_section));
	  break;

	case LOC_REGISTER:
	  if (SYMBOL_IS_ARGUMENT (symbol))
	    fprintf_filtered (outfile, "parameter register %s",
			      plongest (SYMBOL_VALUE (symbol)));
	  else
	    fprintf_filtered (outfile, "register %s",
			      plongest (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_ARG:
	  fprintf_filtered (outfile, "arg at offset %s",
			    hex_string (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_REF_ARG:
	  fprintf_filtered (outfile, "reference arg at %s",
			    hex_string (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_REGPARM_ADDR:
	  fprintf_filtered (outfile, "address parameter register %s",
			    plongest (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_LOCAL:
	  fprintf_filtered (outfile, "local at offset %s",
			    hex_string (SYMBOL_VALUE (symbol)));
	  break;

	case LOC_TYPEDEF:
	  break;

	case LOC_LABEL:
	  fprintf_filtered (outfile, "label at ");
	  fputs_filtered (paddress (gdbarch, SYMBOL_VALUE_ADDRESS (symbol)),
			  outfile);
	  if (section)
	    fprintf_filtered (outfile, " section %s",
			      bfd_section_name (section->the_bfd_section->owner,
						section->the_bfd_section));
	  break;

	case LOC_BLOCK:
	  fprintf_filtered (outfile, "block object ");
	  gdb_print_host_address (SYMBOL_BLOCK_VALUE (symbol), outfile);
	  fprintf_filtered (outfile, ", ");
	  fputs_filtered (paddress (gdbarch,
				    BLOCK_START (SYMBOL_BLOCK_VALUE (symbol))),
			  outfile);
	  fprintf_filtered (outfile, "..");
	  fputs_filtered (paddress (gdbarch,
				    BLOCK_END (SYMBOL_BLOCK_VALUE (symbol))),
			  outfile);
	  if (section)
	    fprintf_filtered (outfile, " section %s",
			      bfd_section_name (section->the_bfd_section->owner,
						section->the_bfd_section));
	  break;

	case LOC_COMPUTED:
	  fprintf_filtered (outfile, "computed at runtime");
	  break;

	case LOC_UNRESOLVED:
	  fprintf_filtered (outfile, "unresolved");
	  break;

	case LOC_OPTIMIZED_OUT:
	  fprintf_filtered (outfile, "optimized out");
	  break;

	default:
	  fprintf_filtered (outfile, "botched symbol class %x",
			    SYMBOL_CLASS (symbol));
	  break;
	}
    }
  fprintf_filtered (outfile, "\n");
  return 1;
}

static void
maintenance_print_msymbols (char *args, int from_tty)
{
  char **argv;
  struct ui_file *outfile;
  struct cleanup *cleanups;
  char *filename = DEV_TTY;
  char *symname = NULL;
  struct program_space *pspace;
  struct objfile *objfile;

  struct stat sym_st, obj_st;

  dont_repeat ();

  if (args == NULL)
    {
      error (_("print-msymbols takes an output file "
	       "name and optional symbol file name"));
    }
  argv = gdb_buildargv (args);
  cleanups = make_cleanup_freeargv (argv);

  if (argv[0] != NULL)
    {
      filename = argv[0];
      /* If a second arg is supplied, it is a source file name to match on.  */
      if (argv[1] != NULL)
	{
	  symname = gdb_realpath (argv[1]);
	  make_cleanup (xfree, symname);
	  if (symname && stat (symname, &sym_st))
	    perror_with_name (symname);
	}
    }

  filename = tilde_expand (filename);
  make_cleanup (xfree, filename);

  outfile = gdb_fopen (filename, FOPEN_WT);
  if (outfile == 0)
    perror_with_name (filename);
  make_cleanup_ui_file_delete (outfile);

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
      {
	QUIT;
	if (symname == NULL || (!stat (objfile_name (objfile), &obj_st)
				&& sym_st.st_dev == obj_st.st_dev
				&& sym_st.st_ino == obj_st.st_ino))
	  dump_msymbols (objfile, outfile);
      }
  fprintf_filtered (outfile, "\n\n");
  do_cleanups (cleanups);
}

static void
maintenance_print_objfiles (char *regexp, int from_tty)
{
  struct program_space *pspace;
  struct objfile *objfile;

  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
      {
	QUIT;
	if (! regexp
	    || re_exec (objfile_name (objfile)))
	  dump_objfile (objfile);
      }
}

/* List all the symbol tables whose names match REGEXP (optional).  */

static void
maintenance_info_symtabs (char *regexp, int from_tty)
{
  struct program_space *pspace;
  struct objfile *objfile;

  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      struct compunit_symtab *cust;
      struct symtab *symtab;

      /* We don't want to print anything for this objfile until we
         actually find a symtab whose name matches.  */
      int printed_objfile_start = 0;

      ALL_OBJFILE_COMPUNITS (objfile, cust)
	{
	  int printed_compunit_symtab_start = 0;

	  ALL_COMPUNIT_FILETABS (cust, symtab)
	    {
	      QUIT;

	      if (! regexp
		  || re_exec (symtab_to_filename_for_display (symtab)))
		{
		  if (! printed_objfile_start)
		    {
		      printf_filtered ("{ objfile %s ", objfile_name (objfile));
		      wrap_here ("  ");
		      printf_filtered ("((struct objfile *) %s)\n",
				       host_address_to_string (objfile));
		      printed_objfile_start = 1;
		    }
		  if (! printed_compunit_symtab_start)
		    {
		      printf_filtered ("  { ((struct compunit_symtab *) %s)\n",
				       host_address_to_string (cust));
		      printf_filtered ("    debugformat %s\n",
				       COMPUNIT_DEBUGFORMAT (cust));
		      printf_filtered ("    producer %s\n",
				       COMPUNIT_PRODUCER (cust) != NULL
				       ? COMPUNIT_PRODUCER (cust)
				       : "(null)");
		      printf_filtered ("    dirname %s\n",
				       COMPUNIT_DIRNAME (cust) != NULL
				       ? COMPUNIT_DIRNAME (cust)
				       : "(null)");
		      printf_filtered ("    blockvector"
				       " ((struct blockvector *) %s)\n",
				       host_address_to_string
				         (COMPUNIT_BLOCKVECTOR (cust)));
		      printed_compunit_symtab_start = 1;
		    }

		  printf_filtered ("\t{ symtab %s ",
				   symtab_to_filename_for_display (symtab));
		  wrap_here ("    ");
		  printf_filtered ("((struct symtab *) %s)\n",
				   host_address_to_string (symtab));
		  printf_filtered ("\t  fullname %s\n",
				   symtab->fullname != NULL
				   ? symtab->fullname
				   : "(null)");
		  printf_filtered ("\t  "
				   "linetable ((struct linetable *) %s)\n",
				   host_address_to_string (symtab->linetable));
		  printf_filtered ("\t}\n");
		}
	    }

	  if (printed_compunit_symtab_start)
	    printf_filtered ("  }\n");
	}

      if (printed_objfile_start)
        printf_filtered ("}\n");
    }
}

/* Check consistency of symtabs.
   An example of what this checks for is NULL blockvectors.
   They can happen if there's a bug during debug info reading.
   GDB assumes they are always non-NULL.

   Note: This does not check for psymtab vs symtab consistency.
   Use "maint check-psymtabs" for that.  */

static void
maintenance_check_symtabs (char *ignore, int from_tty)
{
  struct program_space *pspace;
  struct objfile *objfile;

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      struct compunit_symtab *cust;

      /* We don't want to print anything for this objfile until we
         actually find something worth printing.  */
      int printed_objfile_start = 0;

      ALL_OBJFILE_COMPUNITS (objfile, cust)
	{
	  int found_something = 0;
	  struct symtab *symtab = compunit_primary_filetab (cust);

	  QUIT;

	  if (COMPUNIT_BLOCKVECTOR (cust) == NULL)
	    found_something = 1;
	  /* Add more checks here.  */

	  if (found_something)
	    {
	      if (! printed_objfile_start)
		{
		  printf_filtered ("{ objfile %s ", objfile_name (objfile));
		  wrap_here ("  ");
		  printf_filtered ("((struct objfile *) %s)\n",
				   host_address_to_string (objfile));
		  printed_objfile_start = 1;
		}
	      printf_filtered ("  { symtab %s\n",
			       symtab_to_filename_for_display (symtab));
	      if (COMPUNIT_BLOCKVECTOR (cust) == NULL)
		printf_filtered ("    NULL blockvector\n");
	      printf_filtered ("  }\n");
	    }
	}

      if (printed_objfile_start)
        printf_filtered ("}\n");
    }
}

/* Helper function for maintenance_expand_symtabs.
   This is the name_matcher function for expand_symtabs_matching.  */

static int
maintenance_expand_name_matcher (const char *symname, void *data)
{
  /* Since we're not searching on symbols, just return TRUE.  */
  return 1;
}

/* Helper function for maintenance_expand_symtabs.
   This is the file_matcher function for expand_symtabs_matching.  */

static int
maintenance_expand_file_matcher (const char *filename, void *data,
				 int basenames)
{
  const char *regexp = (const char *) data;

  QUIT;

  /* KISS: Only apply the regexp to the complete file name.  */
  if (basenames)
    return 0;

  if (regexp == NULL || re_exec (filename))
    return 1;

  return 0;
}

/* Expand all symbol tables whose name matches an optional regexp.  */

static void
maintenance_expand_symtabs (char *args, int from_tty)
{
  struct program_space *pspace;
  struct objfile *objfile;
  struct cleanup *cleanups;
  char **argv;
  char *regexp = NULL;

  /* We use buildargv here so that we handle spaces in the regexp
     in a way that allows adding more arguments later.  */
  argv = gdb_buildargv (args);
  cleanups = make_cleanup_freeargv (argv);

  if (argv != NULL)
    {
      if (argv[0] != NULL)
	{
	  regexp = argv[0];
	  if (argv[1] != NULL)
	    error (_("Extra arguments after regexp."));
	}
    }

  if (regexp)
    re_comp (regexp);

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      if (objfile->sf)
	{
	  objfile->sf->qf->expand_symtabs_matching
	    (objfile, maintenance_expand_file_matcher,
	     maintenance_expand_name_matcher, NULL, ALL_DOMAIN, regexp);
	}
    }

  do_cleanups (cleanups);
}


/* Return the nexting depth of a block within other blocks in its symtab.  */

static int
block_depth (struct block *block)
{
  int i = 0;

  while ((block = BLOCK_SUPERBLOCK (block)) != NULL)
    {
      i++;
    }
  return i;
}


/* Used by MAINTENANCE_INFO_LINE_TABLES to print the information about a
   single line table.  */

static int
maintenance_print_one_line_table (struct symtab *symtab, void *data)
{
  struct linetable *linetable;
  struct objfile *objfile;

  objfile = symtab->compunit_symtab->objfile;
  printf_filtered (_("objfile: %s ((struct objfile *) %s)\n"),
		   objfile_name (objfile),
		   host_address_to_string (objfile));
  printf_filtered (_("compunit_symtab: ((struct compunit_symtab *) %s)\n"),
		   host_address_to_string (symtab->compunit_symtab));
  printf_filtered (_("symtab: %s ((struct symtab *) %s)\n"),
		   symtab_to_fullname (symtab),
		   host_address_to_string (symtab));
  linetable = SYMTAB_LINETABLE (symtab);
  printf_filtered (_("linetable: ((struct linetable *) %s):\n"),
		   host_address_to_string (linetable));

  if (linetable == NULL)
    printf_filtered (_("No line table.\n"));
  else if (linetable->nitems <= 0)
    printf_filtered (_("Line table has no lines.\n"));
  else
    {
      int i;

      /* Leave space for 6 digits of index and line number.  After that the
	 tables will just not format as well.  */
      printf_filtered (_("%-6s %6s %s\n"),
		       _("INDEX"), _("LINE"), _("ADDRESS"));

      for (i = 0; i < linetable->nitems; ++i)
	{
	  struct linetable_entry *item;

	  item = &linetable->item [i];
	  printf_filtered (_("%-6d %6d %s\n"), i, item->line,
			   core_addr_to_string (item->pc));
	}
    }

  return 0;
}

/* Implement the 'maint info line-table' command.  */

static void
maintenance_info_line_tables (char *regexp, int from_tty)
{
  struct program_space *pspace;
  struct objfile *objfile;

  dont_repeat ();

  if (regexp != NULL)
    re_comp (regexp);

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      struct compunit_symtab *cust;
      struct symtab *symtab;

      ALL_OBJFILE_COMPUNITS (objfile, cust)
	{
	  ALL_COMPUNIT_FILETABS (cust, symtab)
	    {
	      QUIT;

	      if (regexp == NULL
		  || re_exec (symtab_to_filename_for_display (symtab)))
		maintenance_print_one_line_table (symtab, NULL);
	    }
	}
    }
}



/* Do early runtime initializations.  */

void
_initialize_symmisc (void)
{
  std_in = stdin;
  std_out = stdout;
  std_err = stderr;

  add_cmd ("symbols", class_maintenance, maintenance_print_symbols, _("\
Print dump of current symbol definitions.\n\
Entries in the full symbol table are dumped to file OUTFILE.\n\
If a SOURCE file is specified, dump only that file's symbols."),
	   &maintenanceprintlist);

  add_cmd ("msymbols", class_maintenance, maintenance_print_msymbols, _("\
Print dump of current minimal symbol definitions.\n\
Entries in the minimal symbol table are dumped to file OUTFILE.\n\
If a SOURCE file is specified, dump only that file's minimal symbols."),
	   &maintenanceprintlist);

  add_cmd ("objfiles", class_maintenance, maintenance_print_objfiles,
	   _("Print dump of current object file definitions.\n\
With an argument REGEXP, list the object files with matching names."),
	   &maintenanceprintlist);

  add_cmd ("symtabs", class_maintenance, maintenance_info_symtabs, _("\
List the full symbol tables for all object files.\n\
This does not include information about individual symbols, blocks, or\n\
linetables --- just the symbol table structures themselves.\n\
With an argument REGEXP, list the symbol tables with matching names."),
	   &maintenanceinfolist);

  add_cmd ("line-table", class_maintenance, maintenance_info_line_tables, _("\
List the contents of all line tables, from all symbol tables.\n\
With an argument REGEXP, list just the line tables for the symbol\n\
tables with matching names."),
	   &maintenanceinfolist);

  add_cmd ("check-symtabs", class_maintenance, maintenance_check_symtabs,
	   _("\
Check consistency of currently expanded symtabs."),
	   &maintenancelist);

  add_cmd ("expand-symtabs", class_maintenance, maintenance_expand_symtabs,
	   _("Expand symbol tables.\n\
With an argument REGEXP, only expand the symbol tables with matching names."),
	   &maintenancelist);
}
