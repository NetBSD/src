/* Do various things to symbol tables (other than lookup), for GDB.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "readline/tilde.h"

#include "psymtab.h"

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

static int block_depth (const struct block *);

static void print_symbol (struct gdbarch *gdbarch, struct symbol *symbol,
			  int depth, ui_file *outfile);


void
print_symbol_bcache_statistics (void)
{
  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	QUIT;
	printf_filtered (_("Byte cache statistics for '%s':\n"),
			 objfile_name (objfile));
	objfile->partial_symtabs->psymbol_cache.print_statistics
	  ("partial symbol cache");
	objfile->per_bfd->string_cache.print_statistics ("string cache");
      }
}

void
print_objfile_statistics (void)
{
  int i, linetables, blockvectors;

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
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
	i = linetables = 0;
	for (compunit_symtab *cu : objfile->compunits ())
	  {
	    for (symtab *s : compunit_filetabs (cu))
	      {
		i++;
		if (SYMTAB_LINETABLE (s) != NULL)
		  linetables++;
	      }
	  }
	blockvectors = std::distance (objfile->compunits ().begin (),
				      objfile->compunits ().end ());
	printf_filtered (_("  Number of symbol tables: %d\n"), i);
	printf_filtered (_("  Number of symbol tables with line tables: %d\n"),
			 linetables);
	printf_filtered (_("  Number of symbol tables with blockvectors: %d\n"),
			 blockvectors);

	if (OBJSTAT (objfile, sz_strtab) > 0)
	  printf_filtered (_("  Space used by string tables: %d\n"),
			   OBJSTAT (objfile, sz_strtab));
	printf_filtered (_("  Total memory used for objfile obstack: %s\n"),
			 pulongest (obstack_memory_used (&objfile
							 ->objfile_obstack)));
	printf_filtered (_("  Total memory used for BFD obstack: %s\n"),
			 pulongest (obstack_memory_used (&objfile->per_bfd
							 ->storage_obstack)));
	printf_filtered
	  (_("  Total memory used for psymbol cache: %d\n"),
	   objfile->partial_symtabs->psymbol_cache.memory_used ());
	printf_filtered (_("  Total memory used for string cache: %d\n"),
			 objfile->per_bfd->string_cache.memory_used ());
      }
}

static void
dump_objfile (struct objfile *objfile)
{
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
      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *symtab : compunit_filetabs (cu))
	    {
	      printf_filtered ("%s at ",
			       symtab_to_filename_for_display (symtab));
	      gdb_print_host_address (symtab, gdb_stdout);
	      printf_filtered (", ");
	      if (SYMTAB_OBJFILE (symtab) != objfile)
		{
		  printf_filtered ("NOT ON CHAIN!  ");
		}
	      wrap_here ("  ");
	    }
	}
      printf_filtered ("\n\n");
    }
}

/* Print minimal symbols from this objfile.  */

static void
dump_msymbols (struct objfile *objfile, struct ui_file *outfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  int index;
  char ms_type;

  fprintf_filtered (outfile, "\nObject file %s:\n\n", objfile_name (objfile));
  if (objfile->per_bfd->minimal_symbol_count == 0)
    {
      fprintf_filtered (outfile, "No minimal symbols found.\n");
      return;
    }
  index = 0;
  for (minimal_symbol *msymbol : objfile->msymbols ())
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
	case mst_data_gnu_ifunc:
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

      /* Use the relocated address as shown in the symbol here -- do
	 not try to respect copy relocations.  */
      CORE_ADDR addr = (msymbol->value.address
			+ objfile->section_offsets[msymbol->section]);
      fputs_filtered (paddress (gdbarch, addr), outfile);
      fprintf_filtered (outfile, " %s", msymbol->linkage_name ());
      if (section)
	{
	  if (section->the_bfd_section != NULL)
	    fprintf_filtered (outfile, " section %s",
			      bfd_section_name (section->the_bfd_section));
	  else
	    fprintf_filtered (outfile, " spurious section %ld",
			      (long) (section - objfile->sections));
	}
      if (msymbol->demangled_name () != NULL)
	{
	  fprintf_filtered (outfile, "  %s", msymbol->demangled_name ());
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
  struct gdbarch *gdbarch = objfile->arch ();
  int i;
  struct mdict_iterator miter;
  int len;
  struct linetable *l;
  const struct blockvector *bv;
  struct symbol *sym;
  const struct block *b;
  int depth;

  fprintf_filtered (outfile, "\nSymtab for file %s at %s\n",
		    symtab_to_filename_for_display (symtab),
		    host_address_to_string (symtab));

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
	  if (l->item[i].is_stmt)
	    fprintf_filtered (outfile, "\t(stmt)");
	  fprintf_filtered (outfile, "\n");
	}
    }
  /* Now print the block info, but only for compunit symtabs since we will
     print lots of duplicate info otherwise.  */
  if (is_main_symtab_of_compunit_symtab (symtab))
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
			    mdict_size (BLOCK_MULTIDICT (b)));
	  fputs_filtered (paddress (gdbarch, BLOCK_START (b)), outfile);
	  fprintf_filtered (outfile, "..");
	  fputs_filtered (paddress (gdbarch, BLOCK_END (b)), outfile);
	  if (BLOCK_FUNCTION (b))
	    {
	      fprintf_filtered (outfile, ", function %s",
				BLOCK_FUNCTION (b)->linkage_name ());
	      if (BLOCK_FUNCTION (b)->demangled_name () != NULL)
		{
		  fprintf_filtered (outfile, ", %s",
				BLOCK_FUNCTION (b)->demangled_name ());
		}
	    }
	  fprintf_filtered (outfile, "\n");
	  /* Now print each symbol in this block (in no particular order, if
	     we're using a hashtable).  Note that we only want this
	     block, not any blocks from included symtabs.  */
	  ALL_DICT_SYMBOLS (BLOCK_MULTIDICT (b), miter, sym)
	    {
	      try
		{
		  print_symbol (gdbarch, sym, depth + 1, outfile);
		}
	      catch (const gdb_exception_error &ex)
		{
		  exception_fprintf (gdb_stderr, ex,
				     "Error printing symbol:\n");
		}
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

  /* Print info about the user of this compunit_symtab, and the
     compunit_symtabs included by this one. */
  if (is_main_symtab_of_compunit_symtab (symtab))
    {
      struct compunit_symtab *cust = SYMTAB_COMPUNIT (symtab);

      if (cust->user != nullptr)
	{
	  const char *addr
	    = host_address_to_string (COMPUNIT_FILETABS (cust->user));
	  fprintf_filtered (outfile, "Compunit user: %s\n", addr);
	}
      if (cust->includes != nullptr)
	for (i = 0; ; ++i)
	  {
	    struct compunit_symtab *include = cust->includes[i];
	    if (include == nullptr)
	      break;
	    const char *addr
	      = host_address_to_string (COMPUNIT_FILETABS (include));
	    fprintf_filtered (outfile, "Compunit include: %s\n", addr);
	  }
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
      scoped_restore_current_language save_lang;
      set_language (symtab->language);
      dump_symtab_1 (symtab, outfile);
    }
  else
    dump_symtab_1 (symtab, outfile);
}

static void
maintenance_print_symbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *address_arg = NULL, *source_arg = NULL, *objfile_arg = NULL;
  int i, outfile_idx;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-pc") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing pc value"));
	  address_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-source") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing source file"));
	  source_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  if (address_arg != NULL && source_arg != NULL)
    error (_("Must specify at most one of -pc and -source"));

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  if (address_arg != NULL)
    {
      CORE_ADDR pc = parse_and_eval_address (address_arg);
      struct symtab *s = find_pc_line_symtab (pc);

      if (s == NULL)
	error (_("No symtab for address: %s"), address_arg);
      dump_symtab (s, outfile);
    }
  else
    {
      int found = 0;

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  int print_for_objfile = 1;

	  if (objfile_arg != NULL)
	    print_for_objfile
	      = compare_filenames_for_search (objfile_name (objfile),
					      objfile_arg);
	  if (!print_for_objfile)
	    continue;

	  for (compunit_symtab *cu : objfile->compunits ())
	    {
	      for (symtab *s : compunit_filetabs (cu))
		{
		  int print_for_source = 0;

		  QUIT;
		  if (source_arg != NULL)
		    {
		      print_for_source
			= compare_filenames_for_search
			(symtab_to_filename_for_display (s), source_arg);
		      found = 1;
		    }
		  if (source_arg == NULL
		      || print_for_source)
		    dump_symtab (s, outfile);
		}
	    }
	}

      if (source_arg != NULL && !found)
	error (_("No symtab for source file: %s"), source_arg);
    }
}

/* Print symbol SYMBOL on OUTFILE.  DEPTH says how far to indent.  */

static void
print_symbol (struct gdbarch *gdbarch, struct symbol *symbol,
	      int depth, ui_file *outfile)
{
  struct obj_section *section;

  if (SYMBOL_OBJFILE_OWNED (symbol))
    section = SYMBOL_OBJ_SECTION (symbol_objfile (symbol), symbol);
  else
    section = NULL;

  print_spaces (depth, outfile);
  if (SYMBOL_DOMAIN (symbol) == LABEL_DOMAIN)
    {
      fprintf_filtered (outfile, "label %s at ", symbol->print_name ());
      fputs_filtered (paddress (gdbarch, SYMBOL_VALUE_ADDRESS (symbol)),
		      outfile);
      if (section)
	fprintf_filtered (outfile, " section %s\n",
			  bfd_section_name (section->the_bfd_section));
      else
	fprintf_filtered (outfile, "\n");
      return;
    }

  if (SYMBOL_DOMAIN (symbol) == STRUCT_DOMAIN)
    {
      if (SYMBOL_TYPE (symbol)->name ())
	{
	  LA_PRINT_TYPE (SYMBOL_TYPE (symbol), "", outfile, 1, depth,
			 &type_print_raw_options);
	}
      else
	{
	  fprintf_filtered (outfile, "%s %s = ",
			 (SYMBOL_TYPE (symbol)->code () == TYPE_CODE_ENUM
			  ? "enum"
		     : (SYMBOL_TYPE (symbol)->code () == TYPE_CODE_STRUCT
			? "struct" : "union")),
			    symbol->linkage_name ());
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
	  LA_PRINT_TYPE (SYMBOL_TYPE (symbol), symbol->print_name (),
			 outfile,
			 SYMBOL_TYPE (symbol)->code () != TYPE_CODE_ENUM,
			 depth,
			 &type_print_raw_options);
	  fprintf_filtered (outfile, "; ");
	}
      else
	fprintf_filtered (outfile, "%s ", symbol->print_name ());

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

	    fprintf_filtered (outfile, "const %s hex bytes:",
			      pulongest (TYPE_LENGTH (type)));
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
			      bfd_section_name (section->the_bfd_section));
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
			      bfd_section_name (section->the_bfd_section));
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
			      bfd_section_name (section->the_bfd_section));
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
}

static void
maintenance_print_msymbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *objfile_arg = NULL;
  int i, outfile_idx;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      QUIT;
      if (objfile_arg == NULL
	  || compare_filenames_for_search (objfile_name (objfile), objfile_arg))
	dump_msymbols (objfile, outfile);
    }
}

static void
maintenance_print_objfiles (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	QUIT;
	if (! regexp
	    || re_exec (objfile_name (objfile)))
	  dump_objfile (objfile);
      }
}

/* List all the symbol tables whose names match REGEXP (optional).  */

static void
maintenance_info_symtabs (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	/* We don't want to print anything for this objfile until we
	   actually find a symtab whose name matches.  */
	int printed_objfile_start = 0;

	for (compunit_symtab *cust : objfile->compunits ())
	  {
	    int printed_compunit_symtab_start = 0;

	    for (symtab *symtab : compunit_filetabs (cust))
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
			printf_filtered ("    user"
					 " ((struct compunit_symtab *) %s)\n",
					 cust->user != nullptr
					 ? host_address_to_string (cust->user)
					 : "(null)");
			if (cust->includes != nullptr)
			  {
			    printf_filtered ("    ( includes\n");
			    for (int i = 0; ; ++i)
			      {
				struct compunit_symtab *include
				  = cust->includes[i];
				if (include == nullptr)
				  break;
				const char *addr
				  = host_address_to_string (include);
				printf_filtered ("      (%s %s)\n",
						 "(struct compunit_symtab *)",
						 addr);
			      }
			    printf_filtered ("    )\n");
			  }
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
maintenance_check_symtabs (const char *ignore, int from_tty)
{
  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	/* We don't want to print anything for this objfile until we
	   actually find something worth printing.  */
	int printed_objfile_start = 0;

	for (compunit_symtab *cust : objfile->compunits ())
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

/* Expand all symbol tables whose name matches an optional regexp.  */

static void
maintenance_expand_symtabs (const char *args, int from_tty)
{
  char *regexp = NULL;

  /* We use buildargv here so that we handle spaces in the regexp
     in a way that allows adding more arguments later.  */
  gdb_argv argv (args);

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

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	if (objfile->sf)
	  {
	    objfile->sf->qf->expand_symtabs_matching
	      (objfile,
	       [&] (const char *filename, bool basenames)
	       {
		 /* KISS: Only apply the regexp to the complete file name.  */
		 return (!basenames
			 && (regexp == NULL || re_exec (filename)));
	       },
	       NULL,
	       NULL,
	       NULL,
	       ALL_DOMAIN);
	  }
      }
}


/* Return the nexting depth of a block within other blocks in its symtab.  */

static int
block_depth (const struct block *block)
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
      /* Leave space for 6 digits of index and line number.  After that the
	 tables will just not format as well.  */
      struct ui_out *uiout = current_uiout;
      ui_out_emit_table table_emitter (uiout, 4, -1, "line-table");
      uiout->table_header (6, ui_left, "index", _("INDEX"));
      uiout->table_header (6, ui_left, "line", _("LINE"));
      uiout->table_header (18, ui_left, "address", _("ADDRESS"));
      uiout->table_header (1, ui_left, "is-stmt", _("IS-STMT"));
      uiout->table_body ();

      for (int i = 0; i < linetable->nitems; ++i)
	{
	  struct linetable_entry *item;

	  item = &linetable->item [i];
	  ui_out_emit_tuple tuple_emitter (uiout, nullptr);
	  uiout->field_signed ("index", i);
	  if (item->line > 0)
	    uiout->field_signed ("line", item->line);
	  else
	    uiout->field_string ("line", _("END"));
	  uiout->field_core_addr ("address", objfile->arch (),
				  item->pc);
	  uiout->field_string ("is-stmt", item->is_stmt ? "Y" : "");
	  uiout->text ("\n");
	}
    }

  return 0;
}

/* Implement the 'maint info line-table' command.  */

static void
maintenance_info_line_tables (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp != NULL)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	for (compunit_symtab *cust : objfile->compunits ())
	  {
	    for (symtab *symtab : compunit_filetabs (cust))
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

void _initialize_symmisc ();
void
_initialize_symmisc ()
{
  std_in = stdin;
  std_out = stdout;
  std_err = stderr;

  add_cmd ("symbols", class_maintenance, maintenance_print_symbols, _("\
Print dump of current symbol definitions.\n\
Usage: mt print symbols [-pc ADDRESS] [--] [OUTFILE]\n\
       mt print symbols [-objfile OBJFILE] [-source SOURCE] [--] [OUTFILE]\n\
Entries in the full symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If ADDRESS is provided, dump only the file for that address.\n\
If SOURCE is provided, dump only that file's symbols.\n\
If OBJFILE is provided, dump only that file's minimal symbols."),
	   &maintenanceprintlist);

  add_cmd ("msymbols", class_maintenance, maintenance_print_msymbols, _("\
Print dump of current minimal symbol definitions.\n\
Usage: mt print msymbols [-objfile OBJFILE] [--] [OUTFILE]\n\
Entries in the minimal symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If OBJFILE is provided, dump only that file's minimal symbols."),
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
