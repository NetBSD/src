/* Debug logging for the symbol file functions for the GNU debugger, GDB.

   Copyright (C) 2013-2014 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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

/* Note: Be careful with functions that can throw errors.
   We want to see a logging message regardless of whether an error was thrown.
   This typically means printing a message before calling the real function
   and then if the function returns a result printing a message after it
   returns.  */

#include "defs.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "observer.h"
#include "source.h"
#include "symtab.h"
#include "symfile.h"

/* We need to save a pointer to the real symbol functions.
   Plus, the debug versions are malloc'd because we have to NULL out the
   ones that are NULL in the real copy.  */

struct debug_sym_fns_data
{
  const struct sym_fns *real_sf;
  struct sym_fns debug_sf;
};

/* We need to record a pointer to the real set of functions for each
   objfile.  */
static const struct objfile_data *symfile_debug_objfile_data_key;

/* If non-zero all calls to the symfile functions are logged.  */
static int debug_symfile = 0;

/* Return non-zero if symfile debug logging is installed.  */

static int
symfile_debug_installed (struct objfile *objfile)
{
  return (objfile->sf != NULL
	  && objfile_data (objfile, symfile_debug_objfile_data_key) != NULL);
}

/* Utility to return the name to print for OBJFILE.  */

static const char *
debug_objfile_name (const struct objfile *objfile)
{
  return lbasename (objfile->original_name);
}

/* Utility return the name to print for SYMTAB.  */

static const char *
debug_symtab_name (struct symtab *symtab)
{
  return symtab_to_filename_for_display (symtab);
}

/* Debugging version of struct quick_symbol_functions.  */

static int
debug_qf_has_symbols (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  int retval;

  retval = debug_data->real_sf->qf->has_symbols (objfile);

  fprintf_filtered (gdb_stdlog, "qf->has_symbols (%s) = %d\n",
		    debug_objfile_name (objfile), retval);

  return retval;
}

static struct symtab *
debug_qf_find_last_source_symtab (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  struct symtab *retval;

  fprintf_filtered (gdb_stdlog, "qf->find_last_source_symtab (%s)\n",
		    debug_objfile_name (objfile));

  retval = debug_data->real_sf->qf->find_last_source_symtab (objfile);

  fprintf_filtered (gdb_stdlog, "qf->find_last_source_symtab (...) = %s\n",
		    retval ? debug_symtab_name (retval) : "NULL");

  return retval;
}

static void
debug_qf_forget_cached_source_info (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "qf->forget_cached_source_info (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->qf->forget_cached_source_info (objfile);
}

static int
debug_qf_map_symtabs_matching_filename (struct objfile *objfile,
					const char *name,
					const char *real_path,
					int (*callback) (struct symtab *,
							 void *),
					void *data)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  int retval;

  fprintf_filtered (gdb_stdlog,
		    "qf->map_symtabs_matching_filename (%s, \"%s\", \"%s\", %s, %s)\n",
		    debug_objfile_name (objfile), name,
		    real_path ? real_path : NULL,
		    host_address_to_string (callback),
		    host_address_to_string (data));

  retval = debug_data->real_sf->qf->map_symtabs_matching_filename
    (objfile, name, real_path, callback, data);

  fprintf_filtered (gdb_stdlog,
		    "qf->map_symtabs_matching_filename (...) = %d\n",
		    retval);

  return retval;
}

static struct symtab *
debug_qf_lookup_symbol (struct objfile *objfile, int kind, const char *name,
			domain_enum domain)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  struct symtab *retval;

  fprintf_filtered (gdb_stdlog,
		    "qf->lookup_symbol (%s, %d, \"%s\", %s)\n",
		    debug_objfile_name (objfile), kind, name,
		    domain_name (domain));

  retval = debug_data->real_sf->qf->lookup_symbol (objfile, kind, name,
						   domain);

  fprintf_filtered (gdb_stdlog, "qf->lookup_symbol (...) = %s\n",
		    retval ? debug_symtab_name (retval) : "NULL");

  return retval;
}

static void
debug_qf_print_stats (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "qf->print_stats (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->qf->print_stats (objfile);
}

static void
debug_qf_dump (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "qf->dump (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->qf->dump (objfile);
}

static void
debug_qf_relocate (struct objfile *objfile,
		   const struct section_offsets *new_offsets,
		   const struct section_offsets *delta)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "qf->relocate (%s, %s, %s)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (new_offsets),
		    host_address_to_string (delta));

  debug_data->real_sf->qf->relocate (objfile, new_offsets, delta);
}

static void
debug_qf_expand_symtabs_for_function (struct objfile *objfile,
				      const char *func_name)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog,
		    "qf->expand_symtabs_for_function (%s, \"%s\")\n",
		    debug_objfile_name (objfile), func_name);

  debug_data->real_sf->qf->expand_symtabs_for_function (objfile, func_name);
}

static void
debug_qf_expand_all_symtabs (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "qf->expand_all_symtabs (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->qf->expand_all_symtabs (objfile);
}

static void
debug_qf_expand_symtabs_with_fullname (struct objfile *objfile,
				       const char *fullname)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog,
		    "qf->expand_symtabs_with_fullname (%s, \"%s\")\n",
		    debug_objfile_name (objfile), fullname);

  debug_data->real_sf->qf->expand_symtabs_with_fullname (objfile, fullname);
}

static void
debug_qf_map_matching_symbols (struct objfile *objfile,
			       const char *name, domain_enum namespace,
			       int global,
			       int (*callback) (struct block *,
						struct symbol *, void *),
			       void *data,
			       symbol_compare_ftype *match,
			       symbol_compare_ftype *ordered_compare)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog,
		    "qf->map_matching_symbols (%s, \"%s\", %s, %d, %s, %s, %s, %s)\n",
		    debug_objfile_name (objfile), name,
		    domain_name (namespace), global,
		    host_address_to_string (callback),
		    host_address_to_string (data),
		    host_address_to_string (match),
		    host_address_to_string (ordered_compare));

  debug_data->real_sf->qf->map_matching_symbols (objfile, name,
						 namespace, global,
						 callback, data,
						 match,
						 ordered_compare);
}

static void
debug_qf_expand_symtabs_matching (struct objfile *objfile,
				  int (*file_matcher) (const char *, void *,
						       int basenames),
				  int (*name_matcher) (const char *, void *),
				  enum search_domain kind,
				  void *data)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog,
		    "qf->expand_symtabs_matching (%s, %s, %s, %s, %s)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (file_matcher),
		    host_address_to_string (name_matcher),
		    search_domain_name (kind),
		    host_address_to_string (data));

  debug_data->real_sf->qf->expand_symtabs_matching (objfile,
						    file_matcher,
						    name_matcher,
						    kind, data);
}

static struct symtab *
debug_qf_find_pc_sect_symtab (struct objfile *objfile,
			      struct minimal_symbol *msymbol,
			      CORE_ADDR pc,
			      struct obj_section *section,
			      int warn_if_readin)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  struct symtab *retval;

  fprintf_filtered (gdb_stdlog,
		    "qf->find_pc_sect_symtab (%s, %s, %s, %s, %d)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (msymbol),
		    hex_string (pc),
		    host_address_to_string (section),
		    warn_if_readin);

  retval = debug_data->real_sf->qf->find_pc_sect_symtab (objfile, msymbol,
							 pc, section,
							 warn_if_readin);

  fprintf_filtered (gdb_stdlog, "qf->find_pc_sect_symtab (...) = %s\n",
		    retval ? debug_symtab_name (retval) : "NULL");

  return retval;
}

static void
debug_qf_map_symbol_filenames (struct objfile *objfile,
			       symbol_filename_ftype *fun, void *data,
			       int need_fullname)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  fprintf_filtered (gdb_stdlog,
		    "qf->map_symbol_filenames (%s, %s, %s, %d)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (fun),
		    host_address_to_string (data),
		    need_fullname);

  debug_data->real_sf->qf->map_symbol_filenames (objfile, fun, data,
						 need_fullname);
}

static const struct quick_symbol_functions debug_sym_quick_functions =
{
  debug_qf_has_symbols,
  debug_qf_find_last_source_symtab,
  debug_qf_forget_cached_source_info,
  debug_qf_map_symtabs_matching_filename,
  debug_qf_lookup_symbol,
  debug_qf_print_stats,
  debug_qf_dump,
  debug_qf_relocate,
  debug_qf_expand_symtabs_for_function,
  debug_qf_expand_all_symtabs,
  debug_qf_expand_symtabs_with_fullname,
  debug_qf_map_matching_symbols,
  debug_qf_expand_symtabs_matching,
  debug_qf_find_pc_sect_symtab,
  debug_qf_map_symbol_filenames
};

/* Debugging version of struct sym_probe_fns.  */

static VEC (probe_p) *
debug_sym_get_probes (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  VEC (probe_p) *retval;

  retval = debug_data->real_sf->sym_probe_fns->sym_get_probes (objfile);

  fprintf_filtered (gdb_stdlog,
		    "probes->sym_get_probes (%s) = %s\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (retval));

  return retval;
}

static void
debug_sym_relocate_probe (struct objfile *objfile,
			  const struct section_offsets *new_offsets,
			  const struct section_offsets *delta)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog,
		    "probes->sym_relocate_probe (%s, %s, %s)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (new_offsets),
		    host_address_to_string (delta));

  debug_data->real_sf->sym_probe_fns->sym_relocate_probe
    (objfile, new_offsets, delta);
}

static const struct sym_probe_fns debug_sym_probe_fns =
{
  debug_sym_get_probes,
  debug_sym_relocate_probe
};

/* Debugging version of struct sym_fns.  */

static void
debug_sym_new_init (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_new_init (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->sym_new_init (objfile);
}

static void
debug_sym_init (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_init (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->sym_init (objfile);
}

static void
debug_sym_read (struct objfile *objfile, int symfile_flags)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_read (%s, 0x%x)\n",
		    debug_objfile_name (objfile), symfile_flags);

  debug_data->real_sf->sym_read (objfile, symfile_flags);
}

static void
debug_sym_read_psymbols (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_read_psymbols (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->sym_read_psymbols (objfile);
}

static void
debug_sym_finish (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_finish (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->sym_finish (objfile);
}

static void
debug_sym_offsets (struct objfile *objfile,
		   const struct section_addr_info *info)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_offsets (%s, %s)\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (info));

  debug_data->real_sf->sym_offsets (objfile, info);
}

static struct symfile_segment_data *
debug_sym_segments (bfd *abfd)
{
  /* This API function is annoying, it doesn't take a "this" pointer.
     Fortunately it is only used in one place where we (re-)lookup the
     sym_fns table to use.  Thus we will never be called.  */
  gdb_assert_not_reached ("debug_sym_segments called");
}

static void
debug_sym_read_linetable (struct objfile *objfile)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);

  fprintf_filtered (gdb_stdlog, "sf->sym_read_linetable (%s)\n",
		    debug_objfile_name (objfile));

  debug_data->real_sf->sym_read_linetable (objfile);
}

static bfd_byte *
debug_sym_relocate (struct objfile *objfile, asection *sectp, bfd_byte *buf)
{
  const struct debug_sym_fns_data *debug_data =
    objfile_data (objfile, symfile_debug_objfile_data_key);
  bfd_byte *retval;

  retval = debug_data->real_sf->sym_relocate (objfile, sectp, buf);

  fprintf_filtered (gdb_stdlog,
		    "sf->sym_relocate (%s, %s, %s) = %s\n",
		    debug_objfile_name (objfile),
		    host_address_to_string (sectp),
		    host_address_to_string (buf),
		    host_address_to_string (retval));

  return retval;
}

/* Template of debugging version of struct sym_fns.
   A copy is made, with sym_flavour updated, and a pointer to the real table
   installed in real_sf, and then a pointer to the copy is installed in the
   objfile.  */

static const struct sym_fns debug_sym_fns =
{
  debug_sym_new_init,
  debug_sym_init,
  debug_sym_read,
  debug_sym_read_psymbols,
  debug_sym_finish,
  debug_sym_offsets,
  debug_sym_segments,
  debug_sym_read_linetable,
  debug_sym_relocate,
  &debug_sym_probe_fns,
  &debug_sym_quick_functions
};

/* Free the copy of sym_fns recorded in the registry.  */

static void
symfile_debug_free_objfile (struct objfile *objfile, void *datum)
{
  xfree (datum);
}

/* Install the debugging versions of the symfile functions for OBJFILE.
   Do not call this if the debug versions are already installed.  */

static void
install_symfile_debug_logging (struct objfile *objfile)
{
  const struct sym_fns *real_sf;
  struct debug_sym_fns_data *debug_data;

  /* The debug versions should not already be installed.  */
  gdb_assert (!symfile_debug_installed (objfile));

  real_sf = objfile->sf;

  /* Alas we have to preserve NULL entries in REAL_SF.  */
  debug_data = XZALLOC (struct debug_sym_fns_data);

#define COPY_SF_PTR(from, to, name, func)	\
  do {						\
    if ((from)->name)				\
      (to)->debug_sf.name = func;		\
  } while (0)

  COPY_SF_PTR (real_sf, debug_data, sym_new_init, debug_sym_new_init);
  COPY_SF_PTR (real_sf, debug_data, sym_init, debug_sym_init);
  COPY_SF_PTR (real_sf, debug_data, sym_read, debug_sym_read);
  COPY_SF_PTR (real_sf, debug_data, sym_read_psymbols,
	       debug_sym_read_psymbols);
  COPY_SF_PTR (real_sf, debug_data, sym_finish, debug_sym_finish);
  COPY_SF_PTR (real_sf, debug_data, sym_offsets, debug_sym_offsets);
  COPY_SF_PTR (real_sf, debug_data, sym_segments, debug_sym_segments);
  COPY_SF_PTR (real_sf, debug_data, sym_read_linetable,
	       debug_sym_read_linetable);
  COPY_SF_PTR (real_sf, debug_data, sym_relocate, debug_sym_relocate);
  if (real_sf->sym_probe_fns)
    debug_data->debug_sf.sym_probe_fns = &debug_sym_probe_fns;
  debug_data->debug_sf.qf = &debug_sym_quick_functions;

#undef COPY_SF_PTR

  debug_data->real_sf = real_sf;
  set_objfile_data (objfile, symfile_debug_objfile_data_key, debug_data);
  objfile->sf = &debug_data->debug_sf;
}

/* Uninstall the debugging versions of the symfile functions for OBJFILE.
   Do not call this if the debug versions are not installed.  */

static void
uninstall_symfile_debug_logging (struct objfile *objfile)
{
  struct debug_sym_fns_data *debug_data;

  /* The debug versions should be currently installed.  */
  gdb_assert (symfile_debug_installed (objfile));

  debug_data = objfile_data (objfile, symfile_debug_objfile_data_key);

  objfile->sf = debug_data->real_sf;
  xfree (debug_data);
  set_objfile_data (objfile, symfile_debug_objfile_data_key, NULL);
}

/* Call this function to set OBJFILE->SF.
   Do not set OBJFILE->SF directly.  */

void
objfile_set_sym_fns (struct objfile *objfile, const struct sym_fns *sf)
{
  if (symfile_debug_installed (objfile))
    {
      gdb_assert (debug_symfile);
      /* Remove the current one, and reinstall a new one later.  */
      uninstall_symfile_debug_logging (objfile);
    }

  /* Assume debug logging is disabled.  */
  objfile->sf = sf;

  /* Turn debug logging on if enabled.  */
  if (debug_symfile)
    install_symfile_debug_logging (objfile);
}

static void
set_debug_symfile (char *args, int from_tty, struct cmd_list_element *c)
{
  struct program_space *pspace;
  struct objfile *objfile;

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      if (debug_symfile)
	{
	  if (!symfile_debug_installed (objfile))
	    install_symfile_debug_logging (objfile);
	}
      else
	{
	  if (symfile_debug_installed (objfile))
	    uninstall_symfile_debug_logging (objfile);
	}
    }
}

static void
show_debug_symfile (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Symfile debugging is %s.\n"), value);
}

initialize_file_ftype _initialize_symfile_debug;

void
_initialize_symfile_debug (void)
{
  symfile_debug_objfile_data_key
    = register_objfile_data_with_cleanup (NULL, symfile_debug_free_objfile);

  add_setshow_boolean_cmd ("symfile", no_class, &debug_symfile, _("\
Set debugging of the symfile functions."), _("\
Show debugging of the symfile functions."), _("\
When enabled, all calls to the symfile functions are logged."),
			   set_debug_symfile, show_debug_symfile,
			   &setdebuglist, &showdebuglist);

  /* Note: We don't need a new-objfile observer because debug logging
     will be installed when objfile init'n calls objfile_set_sym_fns.  */
}
