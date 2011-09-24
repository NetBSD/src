/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "gdb_regex.h"
#include "top.h"
#include "exceptions.h"
#include "command.h"
#include "gdbcmd.h"
#include "observer.h"
#include "progspace.h"
#include "objfiles.h"
#include "python.h"
#include "cli/cli-cmds.h"

/* Internal-use flag to enable/disable auto-loading.
   This is true if we should auto-load python code when an objfile is opened,
   false otherwise.

   Both auto_load_scripts && gdbpy_global_auto_load must be true to enable
   auto-loading.

   This flag exists to facilitate deferring auto-loading during start-up
   until after ./.gdbinit has been read; it may augment the search directories
   used to find the scripts.  */
int gdbpy_global_auto_load = 1;

#ifdef HAVE_PYTHON

#include "python-internal.h"

/* NOTE: It's trivial to also support auto-loading normal gdb scripts.
   There has yet to be a need so it's not implemented.  */

/* The suffix of per-objfile scripts to auto-load.
   E.g. When the program loads libfoo.so, look for libfoo-gdb.py.  */
#define GDBPY_AUTO_FILE_NAME "-gdb.py"

/* The section to look for scripts (in file formats that support sections).
   Each entry in this section is a byte of value 1, and then the nul-terminated
   name of the script.  The script name may include a directory.
   The leading byte is to allow upward compatible extensions.  */
#define GDBPY_AUTO_SECTION_NAME ".debug_gdb_scripts"

/* For scripts specified in .debug_gdb_scripts, multiple objfiles may load
   the same script.  There's no point in loading the script multiple times,
   and there can be a lot of objfiles and scripts, so we keep track of scripts
   loaded this way.  */

struct auto_load_pspace_info
{
  /* For each program space we keep track of loaded scripts.  */
  struct htab *loaded_scripts;
};

/* Objects of this type are stored in the loaded script hash table.  */

struct loaded_script_entry
{
  /* Name as provided by the objfile.  */
  const char *name;
  /* Full path name or NULL if script wasn't found (or was otherwise
     inaccessible).  */
  const char *full_path;
};

/* User-settable option to enable/disable auto-loading:
   set auto-load-scripts on|off
   This is true if we should auto-load associated scripts when an objfile
   is opened, false otherwise.
   At the moment, this only affects python scripts, but there's no reason
   one couldn't also have other kinds of auto-loaded scripts, and there's
   no reason to have them each controlled by a separate flag.
   So we elide "python" from the name here and in the option.
   The fact that it lives here is just an implementation detail.  */
static int auto_load_scripts = 1;

/* Per-program-space data key.  */
static const struct program_space_data *auto_load_pspace_data;

static void
auto_load_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  struct auto_load_pspace_info *info;

  info = program_space_data (pspace, auto_load_pspace_data);
  if (info != NULL)
    {
      if (info->loaded_scripts)
	htab_delete (info->loaded_scripts);
      xfree (info);
    }
}

/* Get the current autoload data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct auto_load_pspace_info *
get_auto_load_pspace_data (struct program_space *pspace)
{
  struct auto_load_pspace_info *info;

  info = program_space_data (pspace, auto_load_pspace_data);
  if (info == NULL)
    {
      info = XZALLOC (struct auto_load_pspace_info);
      set_program_space_data (pspace, auto_load_pspace_data, info);
    }

  return info;
}

/* Hash function for the loaded script hash.  */

static hashval_t
hash_loaded_script_entry (const void *data)
{
  const struct loaded_script_entry *e = data;

  return htab_hash_string (e->name);
}

/* Equality function for the loaded script hash.  */

static int
eq_loaded_script_entry (const void *a, const void *b)
{
  const struct loaded_script_entry *ea = a;
  const struct loaded_script_entry *eb = b;

  return strcmp (ea->name, eb->name) == 0;
}

/* Create the hash table used for loaded scripts.
   Each entry is hashed by the full path name.  */

static void
create_loaded_scripts_hash (struct auto_load_pspace_info *pspace_info)
{
  /* Choose 31 as the starting size of the hash table, somewhat arbitrarily.
     Space for each entry is obtained with one malloc so we can free them
     easily.  */

  pspace_info->loaded_scripts = htab_create (31,
					     hash_loaded_script_entry,
					     eq_loaded_script_entry,
					     xfree);
}

/* Load scripts specified in OBJFILE.
   START,END delimit a buffer containing a list of nul-terminated
   file names.
   SOURCE_NAME is used in error messages.

   Scripts are found per normal "source -s" command processing.
   First the script is looked for in $cwd.  If not found there the
   source search path is used.

   The section contains a list of path names of files containing
   python code to load.  Each path is null-terminated.  */

static void
source_section_scripts (struct objfile *objfile, const char *source_name,
			const char *start, const char *end)
{
  const char *p;
  struct auto_load_pspace_info *pspace_info;
  struct loaded_script_entry **slot, entry;

  pspace_info = get_auto_load_pspace_data (current_program_space);
  if (pspace_info->loaded_scripts == NULL)
    create_loaded_scripts_hash (pspace_info);

  for (p = start; p < end; ++p)
    {
      const char *file;
      FILE *stream;
      char *full_path;
      int opened, in_hash_table;

      if (*p != 1)
	{
	  warning (_("Invalid entry in %s section"), GDBPY_AUTO_SECTION_NAME);
	  /* We could try various heuristics to find the next valid entry,
	     but it's safer to just punt.  */
	  break;
	}
      file = ++p;

      while (p < end && *p != '\0')
	++p;
      if (p == end)
	{
	  char *buf = alloca (p - file + 1);

	  memcpy (buf, file, p - file);
	  buf[p - file] = '\0';
	  warning (_("Non-null-terminated path in %s: %s"),
		   source_name, buf);
	  /* Don't load it.  */
	  break;
	}
      if (p == file)
	{
	  warning (_("Empty path in %s"), source_name);
	  continue;
	}

      opened = find_and_open_script (file, 1 /*search_path*/,
				     &stream, &full_path);

      /* If the file is not found, we still record the file in the hash table,
	 we only want to print an error message once.
	 IWBN if complaints.c were more general-purpose.  */

      entry.name = file;
      if (opened)
	entry.full_path = full_path;
      else
	entry.full_path = NULL;
      slot = ((struct loaded_script_entry **)
	      htab_find_slot (pspace_info->loaded_scripts,
			      &entry, INSERT));
      in_hash_table = *slot != NULL;

      /* If this file is not in the hash table, add it.  */
      if (! in_hash_table)
	{
	  char *p;

	  *slot = xmalloc (sizeof (**slot)
			   + strlen (file) + 1
			   + (opened ? (strlen (full_path) + 1) : 0));
	  p = ((char*) *slot) + sizeof (**slot);
	  strcpy (p, file);
	  (*slot)->name = p;
	  if (opened)
	    {
	      p += strlen (p) + 1;
	      strcpy (p, full_path);
	      (*slot)->full_path = p;
	    }
	  else
	    (*slot)->full_path = NULL;
	}

      if (opened)
	free (full_path);

      if (! opened)
	{
	  /* We don't throw an error, the program is still debuggable.
	     Check in_hash_table to only print the warning once.  */
	  if (! in_hash_table)
	    warning (_("%s (referenced in %s): %s"),
		     file, GDBPY_AUTO_SECTION_NAME, safe_strerror (errno));
	  continue;
	}

      /* If this file is not currently loaded, load it.  */
      if (! in_hash_table)
	source_python_script_for_objfile (objfile, stream, file);
    }
}

/* Load scripts specified in section SECTION_NAME of OBJFILE.  */

static void
auto_load_section_scripts (struct objfile *objfile, const char *section_name)
{
  bfd *abfd = objfile->obfd;
  asection *scripts_sect;
  bfd_size_type size;
  char *p;
  struct cleanup *cleanups;

  scripts_sect = bfd_get_section_by_name (abfd, section_name);
  if (scripts_sect == NULL)
    return;

  size = bfd_get_section_size (scripts_sect);
  p = xmalloc (size);
  
  cleanups = make_cleanup (xfree, p);

  if (bfd_get_section_contents (abfd, scripts_sect, p, (file_ptr) 0, size))
    source_section_scripts (objfile, section_name, p, p + size);
  else
    warning (_("Couldn't read %s section of %s"),
	     section_name, bfd_get_filename (abfd));

  do_cleanups (cleanups);
}

/* Clear the table of loaded section scripts.  */

static void
clear_section_scripts (void)
{
  struct program_space *pspace = current_program_space;
  struct auto_load_pspace_info *info;

  info = program_space_data (pspace, auto_load_pspace_data);
  if (info != NULL && info->loaded_scripts != NULL)
    {
      htab_delete (info->loaded_scripts);
      info->loaded_scripts = NULL;
    }
}

/* Look for the auto-load script associated with OBJFILE and load it.  */

static void
auto_load_objfile_script (struct objfile *objfile, const char *suffix)
{
  char *realname;
  char *filename, *debugfile;
  int len;
  FILE *input;
  struct cleanup *cleanups;

  realname = gdb_realpath (objfile->name);
  len = strlen (realname);
  filename = xmalloc (len + strlen (suffix) + 1);
  memcpy (filename, realname, len);
  strcpy (filename + len, suffix);

  cleanups = make_cleanup (xfree, filename);
  make_cleanup (xfree, realname);

  input = fopen (filename, "r");
  debugfile = filename;

  if (!input && debug_file_directory)
    {
      /* Also try the same file in the separate debug info directory.  */
      debugfile = xmalloc (strlen (filename)
			   + strlen (debug_file_directory) + 1);
      strcpy (debugfile, debug_file_directory);
      /* FILENAME is absolute, so we don't need a "/" here.  */
      strcat (debugfile, filename);

      make_cleanup (xfree, debugfile);
      input = fopen (debugfile, "r");
    }

  if (!input && gdb_datadir)
    {
      /* Also try the same file in a subdirectory of gdb's data
	 directory.  */
      debugfile = xmalloc (strlen (gdb_datadir) + strlen (filename)
			   + strlen ("/auto-load") + 1);
      strcpy (debugfile, gdb_datadir);
      strcat (debugfile, "/auto-load");
      /* FILENAME is absolute, so we don't need a "/" here.  */
      strcat (debugfile, filename);

      make_cleanup (xfree, debugfile);
      input = fopen (debugfile, "r");
    }

  if (input)
    {
      source_python_script_for_objfile (objfile, input, debugfile);
      fclose (input);
    }

  do_cleanups (cleanups);
}

/* This is a new_objfile observer callback to auto-load scripts.

   Two flavors of auto-loaded scripts are supported.
   1) based on the path to the objfile
   2) from .debug_gdb_scripts section  */

static void
auto_load_new_objfile (struct objfile *objfile)
{
  if (!objfile)
    {
      /* OBJFILE is NULL when loading a new "main" symbol-file.  */
      clear_section_scripts ();
      return;
    }

  load_auto_scripts_for_objfile (objfile);
}

/* Load any auto-loaded scripts for OBJFILE.  */

void
load_auto_scripts_for_objfile (struct objfile *objfile)
{
  if (auto_load_scripts && gdbpy_global_auto_load)
    {
      auto_load_objfile_script (objfile, GDBPY_AUTO_FILE_NAME);
      auto_load_section_scripts (objfile, GDBPY_AUTO_SECTION_NAME);
    }
}

/* Traversal function for htab_traverse.
   Print the entry if specified in the regex.  */

static int
maybe_print_section_script (void **slot, void *info)
{
  struct loaded_script_entry *entry = *slot;

  if (re_exec (entry->name))
    {
      printf_filtered (_("Script name: %s\n"), entry->name);
      printf_filtered (_("  Full name: %s\n"),
		       entry->full_path ? entry->full_path : _("unknown"));
    }

  return 1;
}

/* "maint print section-scripts" command.  */

static void
maintenance_print_section_scripts (char *pattern, int from_tty)
{
  struct auto_load_pspace_info *pspace_info;

  dont_repeat ();

  if (pattern && *pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error (_("Invalid regexp: %s"), re_err);

      printf_filtered (_("Objfile scripts matching %s:\n"), pattern);
    }
  else
    {
      re_comp ("");
      printf_filtered (_("Objfile scripts:\n"));
    }

  pspace_info = get_auto_load_pspace_data (current_program_space);
  if (pspace_info == NULL || pspace_info->loaded_scripts == NULL)
    return;

  immediate_quit++;
  htab_traverse_noresize (pspace_info->loaded_scripts,
			  maybe_print_section_script, NULL);
  immediate_quit--;
}

void
gdbpy_initialize_auto_load (void)
{
  auto_load_pspace_data
    = register_program_space_data_with_cleanup (auto_load_pspace_data_cleanup);

  observer_attach_new_objfile (auto_load_new_objfile);

  add_setshow_boolean_cmd ("auto-load-scripts", class_support,
			   &auto_load_scripts, _("\
Set the debugger's behaviour regarding auto-loaded scripts."), _("\
Show the debugger's behaviour regarding auto-loaded scripts."), _("\
If enabled, auto-loaded scripts are loaded when the debugger reads\n\
an executable or shared library."),
			   NULL, NULL,
			   &setlist,
			   &showlist);

  add_cmd ("section-scripts", class_maintenance,
	   maintenance_print_section_scripts,
	   _("Print dump of auto-loaded section scripts matching REGEXP."),
	   &maintenanceprintlist);
}

#else /* ! HAVE_PYTHON */

void
load_auto_scripts_for_objfile (struct objfile *objfile)
{
}

#endif /* ! HAVE_PYTHON */
