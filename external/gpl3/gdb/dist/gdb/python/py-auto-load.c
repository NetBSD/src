/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2010-2013 Free Software Foundation, Inc.

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
#include "top.h"
#include "exceptions.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "python.h"
#include "cli/cli-cmds.h"
#include "auto-load.h"

#ifdef HAVE_PYTHON

#include "python-internal.h"

/* The section to look for Python auto-loaded scripts (in file formats that
   support sections).
   Each entry in this section is a byte of value 1, and then the nul-terminated
   name of the script.  The script name may include a directory.
   The leading byte is to allow upward compatible extensions.  */
#define GDBPY_AUTO_SECTION_NAME ".debug_gdb_scripts"

/* User-settable option to enable/disable auto-loading of Python scripts:
   set auto-load python-scripts on|off
   This is true if we should auto-load associated Python scripts when an
   objfile is opened, false otherwise.  */
static int auto_load_python_scripts = 1;

static void gdbpy_load_auto_script_for_objfile (struct objfile *objfile,
						FILE *file,
						const char *filename);

/* "show" command for the auto_load_python_scripts configuration variable.  */

static void
show_auto_load_python_scripts (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Auto-loading of Python scripts is %s.\n"), value);
}

/* Definition of script language for Python scripts.  */

static const struct script_language script_language_python
  = { GDBPY_AUTO_FILE_NAME, gdbpy_load_auto_script_for_objfile };

/* Wrapper of source_python_script_for_objfile for script_language_python.  */

static void
gdbpy_load_auto_script_for_objfile (struct objfile *objfile, FILE *file,
				    const char *filename)
{
  int is_safe;
  struct auto_load_pspace_info *pspace_info;

  is_safe = file_is_auto_load_safe (filename,
				    _("auto-load: Loading Python script \"%s\" "
				      "by extension for objfile \"%s\".\n"),
				    filename, objfile->name);

  /* Add this script to the hash table too so "info auto-load python-scripts"
     can print it.  */
  pspace_info = get_auto_load_pspace_data_for_loading (current_program_space);
  maybe_add_script (pspace_info, is_safe, filename, filename,
		    &script_language_python);

  if (is_safe)
    source_python_script_for_objfile (objfile, file, filename);
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

  pspace_info = get_auto_load_pspace_data_for_loading (current_program_space);

  for (p = start; p < end; ++p)
    {
      const char *file;
      FILE *stream;
      char *full_path;
      int opened, in_hash_table;
      struct cleanup *back_to;

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

      back_to = make_cleanup (null_cleanup, NULL);
      if (opened)
	{
	  make_cleanup_fclose (stream);
	  make_cleanup (xfree, full_path);

	  if (!file_is_auto_load_safe (full_path,
				       _("auto-load: Loading Python script "
					 "\"%s\" from section \"%s\" of "
					 "objfile \"%s\".\n"),
				       full_path, GDBPY_AUTO_SECTION_NAME,
				       objfile->name))
	    opened = 0;
	}
      else
	{
	  full_path = NULL;

	  /* We don't throw an error, the program is still debuggable.  */
	  if (script_not_found_warning_print (pspace_info))
	    warning (_("Missing auto-load scripts referenced in section %s\n\
of file %s\n\
Use `info auto-load python [REGEXP]' to list them."),
		     GDBPY_AUTO_SECTION_NAME, objfile->name);
	}

      /* If one script isn't found it's not uncommon for more to not be
	 found either.  We don't want to print an error message for each
	 script, too much noise.  Instead, we print the warning once and tell
	 the user how to find the list of scripts that weren't loaded.

	 IWBN if complaints.c were more general-purpose.  */

      in_hash_table = maybe_add_script (pspace_info, opened, file, full_path,
					&script_language_python);

      /* If this file is not currently loaded, load it.  */
      if (opened && !in_hash_table)
	source_python_script_for_objfile (objfile, stream, full_path);

      do_cleanups (back_to);
    }
}

/* Load scripts specified in section SECTION_NAME of OBJFILE.  */

static void
auto_load_section_scripts (struct objfile *objfile, const char *section_name)
{
  bfd *abfd = objfile->obfd;
  asection *scripts_sect;
  bfd_byte *data = NULL;

  scripts_sect = bfd_get_section_by_name (abfd, section_name);
  if (scripts_sect == NULL)
    return;

  if (!bfd_get_full_section_contents (abfd, scripts_sect, &data))
    warning (_("Couldn't read %s section of %s"),
	     section_name, bfd_get_filename (abfd));
  else
    {
      struct cleanup *cleanups;
      char *p = (char *) data;

      cleanups = make_cleanup (xfree, p);
      source_section_scripts (objfile, section_name, p,
			      p + bfd_get_section_size (scripts_sect));
      do_cleanups (cleanups);
    }
}

/* Load any Python auto-loaded scripts for OBJFILE.  */

void
gdbpy_load_auto_scripts_for_objfile (struct objfile *objfile)
{
  if (auto_load_python_scripts)
    {
      auto_load_objfile_script (objfile, &script_language_python);
      auto_load_section_scripts (objfile, GDBPY_AUTO_SECTION_NAME);
    }
}

/* Wrapper for "info auto-load python-scripts".  */

static void
info_auto_load_python_scripts (char *pattern, int from_tty)
{
  auto_load_info_scripts (pattern, from_tty, &script_language_python);
}

void
gdbpy_initialize_auto_load (void)
{
  struct cmd_list_element *cmd;
  char *cmd_name;

  add_setshow_boolean_cmd ("python-scripts", class_support,
			   &auto_load_python_scripts, _("\
Set the debugger's behaviour regarding auto-loaded Python scripts."), _("\
Show the debugger's behaviour regarding auto-loaded Python scripts."), _("\
If enabled, auto-loaded Python scripts are loaded when the debugger reads\n\
an executable or shared library.\n\
This options has security implications for untrusted inferiors."),
			   NULL, show_auto_load_python_scripts,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_setshow_boolean_cmd ("auto-load-scripts", class_support,
			   &auto_load_python_scripts, _("\
Set the debugger's behaviour regarding auto-loaded Python scripts, "
								 "deprecated."),
			   _("\
Show the debugger's behaviour regarding auto-loaded Python scripts, "
								 "deprecated."),
			   NULL, NULL, show_auto_load_python_scripts,
			   &setlist, &showlist);
  cmd_name = "auto-load-scripts";
  cmd = lookup_cmd (&cmd_name, setlist, "", -1, 1);
  deprecate_cmd (cmd, "set auto-load python-scripts");

  /* It is needed because lookup_cmd updates the CMD_NAME pointer.  */
  cmd_name = "auto-load-scripts";
  cmd = lookup_cmd (&cmd_name, showlist, "", -1, 1);
  deprecate_cmd (cmd, "show auto-load python-scripts");

  add_cmd ("python-scripts", class_info, info_auto_load_python_scripts,
	   _("Print the list of automatically loaded Python scripts.\n\
Usage: info auto-load python-scripts [REGEXP]"),
	   auto_load_info_cmdlist_get ());

  cmd = add_info ("auto-load-scripts", info_auto_load_python_scripts, _("\
Print the list of automatically loaded Python scripts, deprecated."));
  deprecate_cmd (cmd, "info auto-load python-scripts");
}

#else /* ! HAVE_PYTHON */

void
gdbpy_load_auto_scripts_for_objfile (struct objfile *objfile)
{
}

#endif /* ! HAVE_PYTHON */
