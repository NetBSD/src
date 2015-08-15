/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2010-2014 Free Software Foundation, Inc.

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
#include <string.h>
#include "top.h"
#include "exceptions.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "python.h"
#include "auto-load.h"

#ifdef HAVE_PYTHON

#include "python-internal.h"

/* User-settable option to enable/disable auto-loading of Python scripts:
   set auto-load python-scripts on|off
   This is true if we should auto-load associated Python scripts when an
   objfile is opened, false otherwise.  */
static int auto_load_python_scripts = 1;

/* "show" command for the auto_load_python_scripts configuration variable.  */

static void
show_auto_load_python_scripts (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Auto-loading of Python scripts is %s.\n"), value);
}

/* Return non-zero if auto-loading Python scripts is enabled.  */

static int
auto_load_python_scripts_enabled (void)
{
  return auto_load_python_scripts;
}

/* Definition of script language for Python scripts.  */

static const struct script_language script_language_python =
{
  "python",
  GDBPY_AUTO_FILE_NAME,
  auto_load_python_scripts_enabled,
  source_python_script_for_objfile
};

/* Return the Python script language definition.  */

const struct script_language *
gdbpy_script_language_defn (void)
{
  return &script_language_python;
}

/* Wrapper for "info auto-load python-scripts".  */

static void
info_auto_load_python_scripts (char *pattern, int from_tty)
{
  auto_load_info_scripts (pattern, from_tty, &script_language_python);
}

int
gdbpy_initialize_auto_load (void)
{
  struct cmd_list_element *cmd;
  const char *cmd_name;

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

  return 0;
}

#else /* ! HAVE_PYTHON */

/* Return the Python script language definition.
   Since support isn't compiled in, return NULL.  */

const struct script_language *
gdbpy_script_language_defn (void)
{
  return NULL;
}

#endif /* ! HAVE_PYTHON */
