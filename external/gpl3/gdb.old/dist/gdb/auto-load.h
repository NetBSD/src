/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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

struct objfile;
struct program_space;
struct auto_load_pspace_info;
struct extension_language_defn;

extern int global_auto_load;

extern int auto_load_local_gdbinit;
extern char *auto_load_local_gdbinit_pathname;
extern int auto_load_local_gdbinit_loaded;

extern struct auto_load_pspace_info *
  get_auto_load_pspace_data_for_loading (struct program_space *pspace);
extern void auto_load_objfile_script (struct objfile *objfile,
				      const struct extension_language_defn *);
extern void load_auto_scripts_for_objfile (struct objfile *objfile);
extern char auto_load_info_scripts_pattern_nl[];
extern void auto_load_info_scripts (char *pattern, int from_tty,
				    const struct extension_language_defn *);

extern struct cmd_list_element **auto_load_set_cmdlist_get (void);
extern struct cmd_list_element **auto_load_show_cmdlist_get (void);
extern struct cmd_list_element **auto_load_info_cmdlist_get (void);

extern int file_is_auto_load_safe (const char *filename,
				   const char *debug_fmt, ...)
  ATTRIBUTE_PRINTF (2, 3);

extern int auto_load_gdb_scripts_enabled
  (const struct extension_language_defn *extlang);

#endif /* AUTO_LOAD_H */
