/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2012-2014 Free Software Foundation, Inc.

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

#ifndef AUTO_LOAD_H
#define AUTO_LOAD_H 1

struct program_space;

struct script_language
{
  /* The name of the language, lowercase.  */
  const char *name;

  /* The suffix added to objfiles to get their auto-load script.
     E.g., "-gdb.py".  */
  const char *suffix;

  /* Returns non-zero if auto-loading scripts in this language is enabled.  */
  int (*auto_load_enabled) (void);

  /* Worker routine to load the script.  It has already been opened and
     deemed safe to load.  */
  void (*source_script_for_objfile) (struct objfile *objfile, FILE *file,
				     const char *filename);
};

extern int global_auto_load;

extern int auto_load_local_gdbinit;
extern char *auto_load_local_gdbinit_pathname;
extern int auto_load_local_gdbinit_loaded;

extern struct auto_load_pspace_info *
  get_auto_load_pspace_data_for_loading (struct program_space *pspace);
extern int maybe_add_script (struct auto_load_pspace_info *pspace_info,
			     int loaded, const char *name,
			     const char *full_path,
			     const struct script_language *language);
extern void load_auto_scripts_for_objfile (struct objfile *objfile);
extern int
  script_not_found_warning_print (struct auto_load_pspace_info *pspace_info);
extern char auto_load_info_scripts_pattern_nl[];
extern void auto_load_info_scripts (char *pattern, int from_tty,
				    const struct script_language *language);

extern struct cmd_list_element **auto_load_set_cmdlist_get (void);
extern struct cmd_list_element **auto_load_show_cmdlist_get (void);
extern struct cmd_list_element **auto_load_info_cmdlist_get (void);

extern int file_is_auto_load_safe (const char *filename,
				   const char *debug_fmt, ...);

#endif /* AUTO_LOAD_H */
