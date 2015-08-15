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

#include "defs.h"
#include "auto-load.h"
#include "progspace.h"
#include "python/python.h"
#include "gdb_regex.h"
#include "ui-out.h"
#include "filenames.h"
#include "command.h"
#include "observer.h"
#include "objfiles.h"
#include "exceptions.h"
#include "cli/cli-script.h"
#include "gdbcmd.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "gdb_vecs.h"
#include "readline/tilde.h"
#include "completer.h"
#include "fnmatch.h"
#include "top.h"
#include "filestuff.h"

/* The section to look in for auto-loaded scripts (in file formats that
   support sections).
   Each entry in this section is a record that begins with a leading byte
   identifying the record type.
   At the moment we only support one record type: A leading byte of 1,
   followed by the path of a python script to load.  */
#define AUTO_SECTION_NAME ".debug_gdb_scripts"

/* The suffix of per-objfile scripts to auto-load as non-Python command files.
   E.g. When the program loads libfoo.so, look for libfoo-gdb.gdb.  */
#define GDB_AUTO_FILE_NAME "-gdb.gdb"

static void source_gdb_script_for_objfile (struct objfile *objfile, FILE *file,
					   const char *filename);

/* Value of the 'set debug auto-load' configuration variable.  */
static int debug_auto_load = 0;

/* "show" command for the debug_auto_load configuration variable.  */

static void
show_debug_auto_load (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging output for files "
			    "of 'set auto-load ...' is %s.\n"),
		    value);
}

/* User-settable option to enable/disable auto-loading of GDB_AUTO_FILE_NAME
   scripts:
   set auto-load gdb-scripts on|off
   This is true if we should auto-load associated scripts when an objfile
   is opened, false otherwise.  */
static int auto_load_gdb_scripts = 1;

/* "show" command for the auto_load_gdb_scripts configuration variable.  */

static void
show_auto_load_gdb_scripts (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Auto-loading of canned sequences of commands "
			    "scripts is %s.\n"),
		    value);
}

/* Return non-zero if auto-loading gdb scripts is enabled.  */

static int
auto_load_gdb_scripts_enabled (void)
{
  return auto_load_gdb_scripts;
}

/* Internal-use flag to enable/disable auto-loading.
   This is true if we should auto-load python code when an objfile is opened,
   false otherwise.

   Both auto_load_scripts && global_auto_load must be true to enable
   auto-loading.

   This flag exists to facilitate deferring auto-loading during start-up
   until after ./.gdbinit has been read; it may augment the search directories
   used to find the scripts.  */
int global_auto_load = 1;

/* Auto-load .gdbinit file from the current directory?  */
int auto_load_local_gdbinit = 1;

/* Absolute pathname to the current directory .gdbinit, if it exists.  */
char *auto_load_local_gdbinit_pathname = NULL;

/* Boolean value if AUTO_LOAD_LOCAL_GDBINIT_PATHNAME has been loaded.  */
int auto_load_local_gdbinit_loaded = 0;

/* "show" command for the auto_load_local_gdbinit configuration variable.  */

static void
show_auto_load_local_gdbinit (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Auto-loading of .gdbinit script from current "
			    "directory is %s.\n"),
		    value);
}

/* Directory list from which to load auto-loaded scripts.  It is not checked
   for absolute paths but they are strongly recommended.  It is initialized by
   _initialize_auto_load.  */
static char *auto_load_dir;

/* "set" command for the auto_load_dir configuration variable.  */

static void
set_auto_load_dir (char *args, int from_tty, struct cmd_list_element *c)
{
  /* Setting the variable to "" resets it to the compile time defaults.  */
  if (auto_load_dir[0] == '\0')
    {
      xfree (auto_load_dir);
      auto_load_dir = xstrdup (AUTO_LOAD_DIR);
    }
}

/* "show" command for the auto_load_dir configuration variable.  */

static void
show_auto_load_dir (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("List of directories from which to load "
			    "auto-loaded scripts is %s.\n"),
		    value);
}

/* Directory list safe to hold auto-loaded files.  It is not checked for
   absolute paths but they are strongly recommended.  It is initialized by
   _initialize_auto_load.  */
static char *auto_load_safe_path;

/* Vector of directory elements of AUTO_LOAD_SAFE_PATH with each one normalized
   by tilde_expand and possibly each entries has added its gdb_realpath
   counterpart.  */
static VEC (char_ptr) *auto_load_safe_path_vec;

/* Expand $datadir and $debugdir in STRING according to the rules of
   substitute_path_component.  Return vector from dirnames_to_char_ptr_vec,
   this vector must be freed by free_char_ptr_vec by the caller.  */

static VEC (char_ptr) *
auto_load_expand_dir_vars (const char *string)
{
  VEC (char_ptr) *dir_vec;
  char *s;

  s = xstrdup (string);
  substitute_path_component (&s, "$datadir", gdb_datadir);
  substitute_path_component (&s, "$debugdir", debug_file_directory);

  if (debug_auto_load && strcmp (s, string) != 0)
    fprintf_unfiltered (gdb_stdlog,
			_("auto-load: Expanded $-variables to \"%s\".\n"), s);

  dir_vec = dirnames_to_char_ptr_vec (s);
  xfree(s);

  return dir_vec;
}

/* Update auto_load_safe_path_vec from current AUTO_LOAD_SAFE_PATH.  */

static void
auto_load_safe_path_vec_update (void)
{
  unsigned len;
  int ix;

  if (debug_auto_load)
    fprintf_unfiltered (gdb_stdlog,
			_("auto-load: Updating directories of \"%s\".\n"),
			auto_load_safe_path);

  free_char_ptr_vec (auto_load_safe_path_vec);

  auto_load_safe_path_vec = auto_load_expand_dir_vars (auto_load_safe_path);
  len = VEC_length (char_ptr, auto_load_safe_path_vec);

  /* Apply tilde_expand and gdb_realpath to each AUTO_LOAD_SAFE_PATH_VEC
     element.  */
  for (ix = 0; ix < len; ix++)
    {
      char *dir = VEC_index (char_ptr, auto_load_safe_path_vec, ix);
      char *expanded = tilde_expand (dir);
      char *real_path = gdb_realpath (expanded);

      /* Ensure the current entry is at least tilde_expand-ed.  */
      VEC_replace (char_ptr, auto_load_safe_path_vec, ix, expanded);

      if (debug_auto_load)
	{
	  if (strcmp (expanded, dir) == 0)
	    fprintf_unfiltered (gdb_stdlog,
				_("auto-load: Using directory \"%s\".\n"),
				expanded);
	  else
	    fprintf_unfiltered (gdb_stdlog,
				_("auto-load: Resolved directory \"%s\" "
				  "as \"%s\".\n"),
				dir, expanded);
	}
      xfree (dir);

      /* If gdb_realpath returns a different content, append it.  */
      if (strcmp (real_path, expanded) == 0)
	xfree (real_path);
      else
	{
	  VEC_safe_push (char_ptr, auto_load_safe_path_vec, real_path);

	  if (debug_auto_load)
	    fprintf_unfiltered (gdb_stdlog,
				_("auto-load: And canonicalized as \"%s\".\n"),
				real_path);
	}
    }
}

/* Variable gdb_datadir has been set.  Update content depending on $datadir.  */

static void
auto_load_gdb_datadir_changed (void)
{
  auto_load_safe_path_vec_update ();
}

/* "set" command for the auto_load_safe_path configuration variable.  */

static void
set_auto_load_safe_path (char *args, int from_tty, struct cmd_list_element *c)
{
  /* Setting the variable to "" resets it to the compile time defaults.  */
  if (auto_load_safe_path[0] == '\0')
    {
      xfree (auto_load_safe_path);
      auto_load_safe_path = xstrdup (AUTO_LOAD_SAFE_PATH);
    }

  auto_load_safe_path_vec_update ();
}

/* "show" command for the auto_load_safe_path configuration variable.  */

static void
show_auto_load_safe_path (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  const char *cs;

  /* Check if user has entered either "/" or for example ":".
     But while more complicate content like ":/foo" would still also
     permit any location do not hide those.  */

  for (cs = value; *cs && (*cs == DIRNAME_SEPARATOR || IS_DIR_SEPARATOR (*cs));
       cs++);
  if (*cs == 0)
    fprintf_filtered (file, _("Auto-load files are safe to load from any "
			      "directory.\n"));
  else
    fprintf_filtered (file, _("List of directories from which it is safe to "
			      "auto-load files is %s.\n"),
		      value);
}

/* "add-auto-load-safe-path" command for the auto_load_safe_path configuration
   variable.  */

static void
add_auto_load_safe_path (char *args, int from_tty)
{
  char *s;

  if (args == NULL || *args == 0)
    error (_("\
Directory argument required.\n\
Use 'set auto-load safe-path /' for disabling the auto-load safe-path security.\
"));

  s = xstrprintf ("%s%c%s", auto_load_safe_path, DIRNAME_SEPARATOR, args);
  xfree (auto_load_safe_path);
  auto_load_safe_path = s;

  auto_load_safe_path_vec_update ();
}

/* Implementation for filename_is_in_pattern overwriting the caller's FILENAME
   and PATTERN.  */

static int
filename_is_in_pattern_1 (char *filename, char *pattern)
{
  size_t pattern_len = strlen (pattern);
  size_t filename_len = strlen (filename);

  if (debug_auto_load)
    fprintf_unfiltered (gdb_stdlog, _("auto-load: Matching file \"%s\" "
				      "to pattern \"%s\"\n"),
			filename, pattern);

  /* Trim trailing slashes ("/") from PATTERN.  Even for "d:\" paths as
     trailing slashes are trimmed also from FILENAME it still matches
     correctly.  */
  while (pattern_len && IS_DIR_SEPARATOR (pattern[pattern_len - 1]))
    pattern_len--;
  pattern[pattern_len] = '\0';

  /* Ensure auto_load_safe_path "/" matches any FILENAME.  On MS-Windows
     platform FILENAME even after gdb_realpath does not have to start with
     IS_DIR_SEPARATOR character, such as the 'C:\x.exe' filename.  */
  if (pattern_len == 0)
    {
      if (debug_auto_load)
	fprintf_unfiltered (gdb_stdlog,
			    _("auto-load: Matched - empty pattern\n"));
      return 1;
    }

  for (;;)
    {
      /* Trim trailing slashes ("/").  PATTERN also has slashes trimmed the
         same way so they will match.  */
      while (filename_len && IS_DIR_SEPARATOR (filename[filename_len - 1]))
	filename_len--;
      filename[filename_len] = '\0';
      if (filename_len == 0)
	{
	  if (debug_auto_load)
	    fprintf_unfiltered (gdb_stdlog,
				_("auto-load: Not matched - pattern \"%s\".\n"),
				pattern);
	  return 0;
	}

      if (gdb_filename_fnmatch (pattern, filename, FNM_FILE_NAME | FNM_NOESCAPE)
	  == 0)
	{
	  if (debug_auto_load)
	    fprintf_unfiltered (gdb_stdlog, _("auto-load: Matched - file "
					      "\"%s\" to pattern \"%s\".\n"),
				filename, pattern);
	  return 1;
	}

      /* Trim trailing FILENAME component.  */
      while (filename_len > 0 && !IS_DIR_SEPARATOR (filename[filename_len - 1]))
	filename_len--;
    }
}

/* Return 1 if FILENAME matches PATTERN or if FILENAME resides in
   a subdirectory of a directory that matches PATTERN.  Return 0 otherwise.
   gdb_realpath normalization is never done here.  */

static ATTRIBUTE_PURE int
filename_is_in_pattern (const char *filename, const char *pattern)
{
  char *filename_copy, *pattern_copy;

  filename_copy = alloca (strlen (filename) + 1);
  strcpy (filename_copy, filename);
  pattern_copy = alloca (strlen (pattern) + 1);
  strcpy (pattern_copy, pattern);

  return filename_is_in_pattern_1 (filename_copy, pattern_copy);
}

/* Return 1 if FILENAME belongs to one of directory components of
   AUTO_LOAD_SAFE_PATH_VEC.  Return 0 otherwise.
   auto_load_safe_path_vec_update is never called.
   *FILENAME_REALP may be updated by gdb_realpath of FILENAME - it has to be
   freed by the caller.  */

static int
filename_is_in_auto_load_safe_path_vec (const char *filename,
					char **filename_realp)
{
  char *pattern;
  int ix;

  for (ix = 0; VEC_iterate (char_ptr, auto_load_safe_path_vec, ix, pattern);
       ++ix)
    if (*filename_realp == NULL && filename_is_in_pattern (filename, pattern))
      break;
  
  if (pattern == NULL)
    {
      if (*filename_realp == NULL)
	{
	  *filename_realp = gdb_realpath (filename);
	  if (debug_auto_load && strcmp (*filename_realp, filename) != 0)
	    fprintf_unfiltered (gdb_stdlog,
				_("auto-load: Resolved "
				  "file \"%s\" as \"%s\".\n"),
				filename, *filename_realp);
	}

      if (strcmp (*filename_realp, filename) != 0)
	for (ix = 0;
	     VEC_iterate (char_ptr, auto_load_safe_path_vec, ix, pattern); ++ix)
	  if (filename_is_in_pattern (*filename_realp, pattern))
	    break;
    }

  if (pattern != NULL)
    {
      if (debug_auto_load)
	fprintf_unfiltered (gdb_stdlog, _("auto-load: File \"%s\" matches "
					  "directory \"%s\".\n"),
			    filename, pattern);
      return 1;
    }

  return 0;
}

/* Return 1 if FILENAME is located in one of the directories of
   AUTO_LOAD_SAFE_PATH.  Otherwise call warning and return 0.  FILENAME does
   not have to be an absolute path.

   Existence of FILENAME is not checked.  Function will still give a warning
   even if the caller would quietly skip non-existing file in unsafe
   directory.  */

int
file_is_auto_load_safe (const char *filename, const char *debug_fmt, ...)
{
  char *filename_real = NULL;
  struct cleanup *back_to;
  static int advice_printed = 0;

  if (debug_auto_load)
    {
      va_list debug_args;

      va_start (debug_args, debug_fmt);
      vfprintf_unfiltered (gdb_stdlog, debug_fmt, debug_args);
      va_end (debug_args);
    }

  back_to = make_cleanup (free_current_contents, &filename_real);

  if (filename_is_in_auto_load_safe_path_vec (filename, &filename_real))
    {
      do_cleanups (back_to);
      return 1;
    }

  auto_load_safe_path_vec_update ();
  if (filename_is_in_auto_load_safe_path_vec (filename, &filename_real))
    {
      do_cleanups (back_to);
      return 1;
    }

  warning (_("File \"%s\" auto-loading has been declined by your "
	     "`auto-load safe-path' set to \"%s\"."),
	   filename_real, auto_load_safe_path);

  if (!advice_printed)
    {
      const char *homedir = getenv ("HOME");
      char *homeinit;

      if (homedir == NULL)
	homedir = "$HOME";
      homeinit = xstrprintf ("%s/%s", homedir, gdbinit);
      make_cleanup (xfree, homeinit);

      printf_filtered (_("\
To enable execution of this file add\n\
\tadd-auto-load-safe-path %s\n\
line to your configuration file \"%s\".\n\
To completely disable this security protection add\n\
\tset auto-load safe-path /\n\
line to your configuration file \"%s\".\n\
For more information about this security protection see the\n\
\"Auto-loading safe path\" section in the GDB manual.  E.g., run from the shell:\n\
\tinfo \"(gdb)Auto-loading safe path\"\n"),
		       filename_real, homeinit, homeinit);
      advice_printed = 1;
    }

  do_cleanups (back_to);
  return 0;
}

/* Definition of script language for GDB canned sequences of commands.  */

static const struct script_language script_language_gdb =
{
  "gdb",
  GDB_AUTO_FILE_NAME,
  auto_load_gdb_scripts_enabled,
  source_gdb_script_for_objfile
};

static void
source_gdb_script_for_objfile (struct objfile *objfile, FILE *file,
			       const char *filename)
{
  volatile struct gdb_exception e;

  TRY_CATCH (e, RETURN_MASK_ALL)
    {
      script_from_file (file, filename);
    }
  exception_print (gdb_stderr, e);
}

/* For scripts specified in .debug_gdb_scripts, multiple objfiles may load
   the same script.  There's no point in loading the script multiple times,
   and there can be a lot of objfiles and scripts, so we keep track of scripts
   loaded this way.  */

struct auto_load_pspace_info
{
  /* For each program space we keep track of loaded scripts.  */
  struct htab *loaded_scripts;

  /* Non-zero if we've issued the warning about an auto-load script not being
     found.  We only want to issue this warning once.  */
  int script_not_found_warning_printed;
};

/* Objects of this type are stored in the loaded script hash table.  */

struct loaded_script
{
  /* Name as provided by the objfile.  */
  const char *name;

  /* Full path name or NULL if script wasn't found (or was otherwise
     inaccessible).  */
  const char *full_path;

  /* Non-zero if this script has been loaded.  */
  int loaded;

  const struct script_language *language;
};

/* Per-program-space data key.  */
static const struct program_space_data *auto_load_pspace_data;

static void
auto_load_pspace_data_cleanup (struct program_space *pspace, void *arg)
{
  struct auto_load_pspace_info *info = arg;

  if (info->loaded_scripts)
    htab_delete (info->loaded_scripts);
  xfree (info);
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
  const struct loaded_script *e = data;

  return htab_hash_string (e->name) ^ htab_hash_pointer (e->language);
}

/* Equality function for the loaded script hash.  */

static int
eq_loaded_script_entry (const void *a, const void *b)
{
  const struct loaded_script *ea = a;
  const struct loaded_script *eb = b;

  return strcmp (ea->name, eb->name) == 0 && ea->language == eb->language;
}

/* Initialize the table to track loaded scripts.
   Each entry is hashed by the full path name.  */

static void
init_loaded_scripts_info (struct auto_load_pspace_info *pspace_info)
{
  /* Choose 31 as the starting size of the hash table, somewhat arbitrarily.
     Space for each entry is obtained with one malloc so we can free them
     easily.  */

  pspace_info->loaded_scripts = htab_create (31,
					     hash_loaded_script_entry,
					     eq_loaded_script_entry,
					     xfree);

  pspace_info->script_not_found_warning_printed = FALSE;
}

/* Wrapper on get_auto_load_pspace_data to also allocate the hash table
   for loading scripts.  */

struct auto_load_pspace_info *
get_auto_load_pspace_data_for_loading (struct program_space *pspace)
{
  struct auto_load_pspace_info *info;

  info = get_auto_load_pspace_data (pspace);
  if (info->loaded_scripts == NULL)
    init_loaded_scripts_info (info);

  return info;
}

/* Add script NAME in LANGUAGE to hash table of PSPACE_INFO.  LOADED 1 if the
   script has been (is going to) be loaded, 0 otherwise (such as if it has not
   been found).  FULL_PATH is NULL if the script wasn't found.  The result is
   true if the script was already in the hash table.  */

int
maybe_add_script (struct auto_load_pspace_info *pspace_info, int loaded,
		  const char *name, const char *full_path,
		  const struct script_language *language)
{
  struct htab *htab = pspace_info->loaded_scripts;
  struct loaded_script **slot, entry;
  int in_hash_table;

  entry.name = name;
  entry.language = language;
  slot = (struct loaded_script **) htab_find_slot (htab, &entry, INSERT);
  in_hash_table = *slot != NULL;

  /* If this script is not in the hash table, add it.  */

  if (! in_hash_table)
    {
      char *p;

      /* Allocate all space in one chunk so it's easier to free.  */
      *slot = xmalloc (sizeof (**slot)
		       + strlen (name) + 1
		       + (full_path != NULL ? (strlen (full_path) + 1) : 0));
      p = ((char*) *slot) + sizeof (**slot);
      strcpy (p, name);
      (*slot)->name = p;
      if (full_path != NULL)
	{
	  p += strlen (p) + 1;
	  strcpy (p, full_path);
	  (*slot)->full_path = p;
	}
      else
	(*slot)->full_path = NULL;
      (*slot)->loaded = loaded;
      (*slot)->language = language;
    }

  return in_hash_table;
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
      info->script_not_found_warning_printed = FALSE;
    }
}

/* Look for the auto-load script in LANGUAGE associated with OBJFILE where
   OBJFILE's gdb_realpath is REALNAME and load it.  Return 1 if we found any
   matching script, return 0 otherwise.  */

static int
auto_load_objfile_script_1 (struct objfile *objfile, const char *realname,
			    const struct script_language *language)
{
  char *filename, *debugfile;
  int len, retval;
  FILE *input;
  struct cleanup *cleanups;

  len = strlen (realname);
  filename = xmalloc (len + strlen (language->suffix) + 1);
  memcpy (filename, realname, len);
  strcpy (filename + len, language->suffix);

  cleanups = make_cleanup (xfree, filename);

  input = gdb_fopen_cloexec (filename, "r");
  debugfile = filename;
  if (debug_auto_load)
    fprintf_unfiltered (gdb_stdlog, _("auto-load: Attempted file \"%s\" %s.\n"),
			debugfile, input ? _("exists") : _("does not exist"));

  if (!input)
    {
      VEC (char_ptr) *vec;
      int ix;
      char *dir;

      /* Also try the same file in a subdirectory of gdb's data
	 directory.  */

      vec = auto_load_expand_dir_vars (auto_load_dir);
      make_cleanup_free_char_ptr_vec (vec);

      if (debug_auto_load)
	fprintf_unfiltered (gdb_stdlog, _("auto-load: Searching 'set auto-load "
					  "scripts-directory' path \"%s\".\n"),
			    auto_load_dir);

      for (ix = 0; VEC_iterate (char_ptr, vec, ix, dir); ++ix)
	{
	  debugfile = xmalloc (strlen (dir) + strlen (filename) + 1);
	  strcpy (debugfile, dir);

	  /* FILENAME is absolute, so we don't need a "/" here.  */
	  strcat (debugfile, filename);

	  make_cleanup (xfree, debugfile);
	  input = gdb_fopen_cloexec (debugfile, "r");
	  if (debug_auto_load)
	    fprintf_unfiltered (gdb_stdlog, _("auto-load: Attempted file "
					      "\"%s\" %s.\n"),
				debugfile,
				input ? _("exists") : _("does not exist"));
	  if (input != NULL)
	    break;
	}
    }

  if (input)
    {
      int is_safe;
      struct auto_load_pspace_info *pspace_info;

      make_cleanup_fclose (input);

      is_safe
	= file_is_auto_load_safe (debugfile,
				  _("auto-load: Loading %s script \"%s\""
				    " by extension for objfile \"%s\".\n"),
				  language->name, debugfile,
				  objfile_name (objfile));

      /* Add this script to the hash table too so
	 "info auto-load ${lang}-scripts" can print it.  */
      pspace_info
	= get_auto_load_pspace_data_for_loading (current_program_space);
      maybe_add_script (pspace_info, is_safe, debugfile, debugfile, language);

      /* To preserve existing behaviour we don't check for whether the
	 script was already in the table, and always load it.
	 It's highly unlikely that we'd ever load it twice,
	 and these scripts are required to be idempotent under multiple
	 loads anyway.  */
      if (is_safe)
	language->source_script_for_objfile (objfile, input, debugfile);

      retval = 1;
    }
  else
    retval = 0;

  do_cleanups (cleanups);
  return retval;
}

/* Look for the auto-load script in LANGUAGE associated with OBJFILE and load
   it.  */

static void
auto_load_objfile_script (struct objfile *objfile,
			  const struct script_language *language)
{
  char *realname;
  struct cleanup *cleanups;

  /* Skip this script if support has not been compiled in or
     auto-loading it has been disabled.  */
  if (language == NULL
      || !language->auto_load_enabled ())
    return;

  realname = gdb_realpath (objfile_name (objfile));
  cleanups = make_cleanup (xfree, realname);

  if (!auto_load_objfile_script_1 (objfile, realname, language))
    {
      /* For Windows/DOS .exe executables, strip the .exe suffix, so that
	 FOO-gdb.gdb could be used for FOO.exe, and try again.  */

      size_t len = strlen (realname);
      const size_t lexe = sizeof (".exe") - 1;

      if (len > lexe && strcasecmp (realname + len - lexe, ".exe") == 0)
	{
	  len -= lexe;
	  realname[len] = '\0';
	  if (debug_auto_load)
	    fprintf_unfiltered (gdb_stdlog, _("auto-load: Stripped .exe suffix, "
					      "retrying with \"%s\".\n"),
				realname);
	  auto_load_objfile_script_1 (objfile, realname, language);
	}
    }

  do_cleanups (cleanups);
}

/* Load scripts specified in OBJFILE.
   START,END delimit a buffer containing a list of nul-terminated
   file names.
   SECTION_NAME is used in error messages.

   Scripts are found per normal "source -s" command processing.
   First the script is looked for in $cwd.  If not found there the
   source search path is used.

   The section contains a list of path names of script files to load.
   Each path is null-terminated.  */

static void
source_section_scripts (struct objfile *objfile, const char *section_name,
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
      /* At the moment we only support python scripts in .debug_gdb_scripts,
	 but that can change.  */
      const struct script_language *language = gdbpy_script_language_defn ();

      if (*p != 1)
	{
	  warning (_("Invalid entry in %s section"), section_name);
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
		   section_name, buf);
	  /* Don't load it.  */
	  break;
	}
      if (p == file)
	{
	  warning (_("Empty path in %s"), section_name);
	  continue;
	}

      /* Skip this script if support has not been compiled in or
	 auto-loading it has been disabled.  */
      if (language == NULL
	  || !language->auto_load_enabled ())
	{
	  /* No message is printed, just skip it.  */
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
				       _("auto-load: Loading %s script "
					 "\"%s\" from section \"%s\" of "
					 "objfile \"%s\".\n"),
				       language->name, full_path, section_name,
				       objfile_name (objfile)))
	    opened = 0;
	}
      else
	{
	  full_path = NULL;

	  /* If one script isn't found it's not uncommon for more to not be
	     found either.  We don't want to print a message for each script,
	     too much noise.  Instead, we print the warning once and tell the
	     user how to find the list of scripts that weren't loaded.
	     We don't throw an error, the program is still debuggable.

	     IWBN if complaints.c were more general-purpose.  */

	  if (script_not_found_warning_print (pspace_info))
	    warning (_("Missing auto-load scripts referenced in section %s\n\
of file %s\n\
Use `info auto-load %s-scripts [REGEXP]' to list them."),
		     section_name, objfile_name (objfile), language->name);
	}

      in_hash_table = maybe_add_script (pspace_info, opened, file, full_path,
					language);

      /* If this file is not currently loaded, load it.  */
      if (opened && !in_hash_table)
	{
	  gdb_assert (language->source_script_for_objfile != NULL);
	  language->source_script_for_objfile (objfile, stream, full_path);
	}

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

/* Load any auto-loaded scripts for OBJFILE.  */

void
load_auto_scripts_for_objfile (struct objfile *objfile)
{
  /* Return immediately if auto-loading has been globally disabled.
     This is to handle sequencing of operations during gdb startup.
     Also return immediately if OBJFILE is not actually a file.  */
  if (!global_auto_load || (objfile->flags & OBJF_NOT_FILENAME) != 0)
    return;

  /* Load any scripts for this objfile. e.g. foo-gdb.gdb, foo-gdb.py.  */
  auto_load_objfile_script (objfile, &script_language_gdb);
  auto_load_objfile_script (objfile, gdbpy_script_language_defn ());

  /* Load any scripts mentioned in AUTO_SECTION_NAME (.debug_gdb_scripts).  */
  auto_load_section_scripts (objfile, AUTO_SECTION_NAME);
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

/* Collect scripts to be printed in a vec.  */

typedef struct loaded_script *loaded_script_ptr;
DEF_VEC_P (loaded_script_ptr);

struct collect_matching_scripts_data
{
  VEC (loaded_script_ptr) **scripts_p;

  const struct script_language *language;
};

/* Traversal function for htab_traverse.
   Collect the entry if it matches the regexp.  */

static int
collect_matching_scripts (void **slot, void *info)
{
  struct loaded_script *script = *slot;
  struct collect_matching_scripts_data *data = info;

  if (script->language == data->language && re_exec (script->name))
    VEC_safe_push (loaded_script_ptr, *data->scripts_p, script);

  return 1;
}

/* Print SCRIPT.  */

static void
print_script (struct loaded_script *script)
{
  struct ui_out *uiout = current_uiout;
  struct cleanup *chain;

  chain = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);

  ui_out_field_string (uiout, "loaded", script->loaded ? "Yes" : "No");
  ui_out_field_string (uiout, "script", script->name);
  ui_out_text (uiout, "\n");

  /* If the name isn't the full path, print it too.  */
  if (script->full_path != NULL
      && strcmp (script->name, script->full_path) != 0)
    {
      ui_out_text (uiout, "\tfull name: ");
      ui_out_field_string (uiout, "full_path", script->full_path);
      ui_out_text (uiout, "\n");
    }

  do_cleanups (chain);
}

/* Helper for info_auto_load_scripts to sort the scripts by name.  */

static int
sort_scripts_by_name (const void *ap, const void *bp)
{
  const struct loaded_script *a = *(const struct loaded_script **) ap;
  const struct loaded_script *b = *(const struct loaded_script **) bp;

  return FILENAME_CMP (a->name, b->name);
}

/* Special internal GDB value of auto_load_info_scripts's PATTERN identify
   the "info auto-load XXX" command has been executed through the general
   "info auto-load" invocation.  Extra newline will be printed if needed.  */
char auto_load_info_scripts_pattern_nl[] = "";

/* Implementation for "info auto-load gdb-scripts"
   (and "info auto-load python-scripts").  List scripts in LANGUAGE matching
   PATTERN.  FROM_TTY is the usual GDB boolean for user interactivity.  */

void
auto_load_info_scripts (char *pattern, int from_tty,
			const struct script_language *language)
{
  struct ui_out *uiout = current_uiout;
  struct auto_load_pspace_info *pspace_info;
  struct cleanup *script_chain;
  VEC (loaded_script_ptr) *scripts;
  int nr_scripts;

  dont_repeat ();

  pspace_info = get_auto_load_pspace_data (current_program_space);

  if (pattern && *pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error (_("Invalid regexp: %s"), re_err);
    }
  else
    {
      re_comp ("");
    }

  /* We need to know the number of rows before we build the table.
     Plus we want to sort the scripts by name.
     So first traverse the hash table collecting the matching scripts.  */

  scripts = VEC_alloc (loaded_script_ptr, 10);
  script_chain = make_cleanup (VEC_cleanup (loaded_script_ptr), &scripts);

  if (pspace_info != NULL && pspace_info->loaded_scripts != NULL)
    {
      struct collect_matching_scripts_data data = { &scripts, language };

      /* Pass a pointer to scripts as VEC_safe_push can realloc space.  */
      htab_traverse_noresize (pspace_info->loaded_scripts,
			      collect_matching_scripts, &data);
    }

  nr_scripts = VEC_length (loaded_script_ptr, scripts);

  /* Table header shifted right by preceding "gdb-scripts:  " would not match
     its columns.  */
  if (nr_scripts > 0 && pattern == auto_load_info_scripts_pattern_nl)
    ui_out_text (uiout, "\n");

  make_cleanup_ui_out_table_begin_end (uiout, 2, nr_scripts,
				       "AutoLoadedScriptsTable");

  ui_out_table_header (uiout, 7, ui_left, "loaded", "Loaded");
  ui_out_table_header (uiout, 70, ui_left, "script", "Script");
  ui_out_table_body (uiout);

  if (nr_scripts > 0)
    {
      int i;
      loaded_script_ptr script;

      qsort (VEC_address (loaded_script_ptr, scripts),
	     VEC_length (loaded_script_ptr, scripts),
	     sizeof (loaded_script_ptr), sort_scripts_by_name);
      for (i = 0; VEC_iterate (loaded_script_ptr, scripts, i, script); ++i)
	print_script (script);
    }

  do_cleanups (script_chain);

  if (nr_scripts == 0)
    {
      if (pattern && *pattern)
	ui_out_message (uiout, 0, "No auto-load scripts matching %s.\n",
			pattern);
      else
	ui_out_message (uiout, 0, "No auto-load scripts.\n");
    }
}

/* Wrapper for "info auto-load gdb-scripts".  */

static void
info_auto_load_gdb_scripts (char *pattern, int from_tty)
{
  auto_load_info_scripts (pattern, from_tty, &script_language_gdb);
}

/* Implement 'info auto-load local-gdbinit'.  */

static void
info_auto_load_local_gdbinit (char *args, int from_tty)
{
  if (auto_load_local_gdbinit_pathname == NULL)
    printf_filtered (_("Local .gdbinit file was not found.\n"));
  else if (auto_load_local_gdbinit_loaded)
    printf_filtered (_("Local .gdbinit file \"%s\" has been loaded.\n"),
		     auto_load_local_gdbinit_pathname);
  else
    printf_filtered (_("Local .gdbinit file \"%s\" has not been loaded.\n"),
		     auto_load_local_gdbinit_pathname);
}

/* Return non-zero if SCRIPT_NOT_FOUND_WARNING_PRINTED of PSPACE_INFO was unset
   before calling this function.  Always set SCRIPT_NOT_FOUND_WARNING_PRINTED
   of PSPACE_INFO.  */

int
script_not_found_warning_print (struct auto_load_pspace_info *pspace_info)
{
  int retval = !pspace_info->script_not_found_warning_printed;

  pspace_info->script_not_found_warning_printed = 1;

  return retval;
}

/* The only valid "set auto-load" argument is off|0|no|disable.  */

static void
set_auto_load_cmd (char *args, int from_tty)
{
  struct cmd_list_element *list;
  size_t length;

  /* See parse_binary_operation in use by the sub-commands.  */

  length = args ? strlen (args) : 0;

  while (length > 0 && (args[length - 1] == ' ' || args[length - 1] == '\t'))
    length--;

  if (length == 0 || (strncmp (args, "off", length) != 0
		      && strncmp (args, "0", length) != 0
		      && strncmp (args, "no", length) != 0
		      && strncmp (args, "disable", length) != 0))
    error (_("Valid is only global 'set auto-load no'; "
	     "otherwise check the auto-load sub-commands."));

  for (list = *auto_load_set_cmdlist_get (); list != NULL; list = list->next)
    if (list->var_type == var_boolean)
      {
	gdb_assert (list->type == set_cmd);
	do_set_command (args, from_tty, list);
      }
}

/* Initialize "set auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_set_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_prefix_cmd ("auto-load", class_maintenance, set_auto_load_cmd, _("\
Auto-loading specific settings.\n\
Configure various auto-load-specific variables such as\n\
automatic loading of Python scripts."),
		    &retval, "set auto-load ",
		    1/*allow-unknown*/, &setlist);

  return &retval;
}

/* Command "show auto-load" displays summary of all the current
   "show auto-load " settings.  */

static void
show_auto_load_cmd (char *args, int from_tty)
{
  cmd_show_list (*auto_load_show_cmdlist_get (), from_tty, "");
}

/* Initialize "show auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_show_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_prefix_cmd ("auto-load", class_maintenance, show_auto_load_cmd, _("\
Show auto-loading specific settings.\n\
Show configuration of various auto-load-specific variables such as\n\
automatic loading of Python scripts."),
		    &retval, "show auto-load ",
		    0/*allow-unknown*/, &showlist);

  return &retval;
}

/* Command "info auto-load" displays whether the various auto-load files have
   been loaded.  This is reimplementation of cmd_show_list which inserts
   newlines at proper places.  */

static void
info_auto_load_cmd (char *args, int from_tty)
{
  struct cmd_list_element *list;
  struct cleanup *infolist_chain;
  struct ui_out *uiout = current_uiout;

  infolist_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "infolist");

  for (list = *auto_load_info_cmdlist_get (); list != NULL; list = list->next)
    {
      struct cleanup *option_chain
	= make_cleanup_ui_out_tuple_begin_end (uiout, "option");

      gdb_assert (!list->prefixlist);
      gdb_assert (list->type == not_set_cmd);

      ui_out_field_string (uiout, "name", list->name);
      ui_out_text (uiout, ":  ");
      cmd_func (list, auto_load_info_scripts_pattern_nl, from_tty);

      /* Close the tuple.  */
      do_cleanups (option_chain);
    }

  /* Close the tuple.  */
  do_cleanups (infolist_chain);
}

/* Initialize "info auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_info_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_prefix_cmd ("auto-load", class_info, info_auto_load_cmd, _("\
Print current status of auto-loaded files.\n\
Print whether various files like Python scripts or .gdbinit files have been\n\
found and/or loaded."),
		    &retval, "info auto-load ",
		    0/*allow-unknown*/, &infolist);

  return &retval;
}

void _initialize_auto_load (void);

void
_initialize_auto_load (void)
{
  struct cmd_list_element *cmd;
  char *scripts_directory_help;

  auto_load_pspace_data
    = register_program_space_data_with_cleanup (NULL,
						auto_load_pspace_data_cleanup);

  observer_attach_new_objfile (auto_load_new_objfile);

  add_setshow_boolean_cmd ("gdb-scripts", class_support,
			   &auto_load_gdb_scripts, _("\
Enable or disable auto-loading of canned sequences of commands scripts."), _("\
Show whether auto-loading of canned sequences of commands scripts is enabled."),
			   _("\
If enabled, canned sequences of commands are loaded when the debugger reads\n\
an executable or shared library.\n\
This options has security implications for untrusted inferiors."),
			   NULL, show_auto_load_gdb_scripts,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_cmd ("gdb-scripts", class_info, info_auto_load_gdb_scripts,
	   _("Print the list of automatically loaded sequences of commands.\n\
Usage: info auto-load gdb-scripts [REGEXP]"),
	   auto_load_info_cmdlist_get ());

  add_setshow_boolean_cmd ("local-gdbinit", class_support,
			   &auto_load_local_gdbinit, _("\
Enable or disable auto-loading of .gdbinit script in current directory."), _("\
Show whether auto-loading .gdbinit script in current directory is enabled."),
			   _("\
If enabled, canned sequences of commands are loaded when debugger starts\n\
from .gdbinit file in current directory.  Such files are deprecated,\n\
use a script associated with inferior executable file instead.\n\
This options has security implications for untrusted inferiors."),
			   NULL, show_auto_load_local_gdbinit,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_cmd ("local-gdbinit", class_info, info_auto_load_local_gdbinit,
	   _("Print whether current directory .gdbinit file has been loaded.\n\
Usage: info auto-load local-gdbinit"),
	   auto_load_info_cmdlist_get ());

  auto_load_dir = xstrdup (AUTO_LOAD_DIR);
  scripts_directory_help = xstrprintf (
#ifdef HAVE_PYTHON
				       _("\
Automatically loaded Python scripts (named OBJFILE%s) and GDB scripts\n\
(named OBJFILE%s) are located in one of the directories listed by this\n\
option.\n\
%s"),
				       GDBPY_AUTO_FILE_NAME,
#else
				       _("\
Automatically loaded GDB scripts (named OBJFILE%s) are located in one\n\
of the directories listed by this option.\n\
%s"),
#endif
				       GDB_AUTO_FILE_NAME,
				       _("\
This option is ignored for the kinds of scripts \
having 'set auto-load ... off'.\n\
Directories listed here need to be present also \
in the 'set auto-load safe-path'\n\
option."));
  add_setshow_optional_filename_cmd ("scripts-directory", class_support,
				     &auto_load_dir, _("\
Set the list of directories from which to load auto-loaded scripts."), _("\
Show the list of directories from which to load auto-loaded scripts."),
				     scripts_directory_help,
				     set_auto_load_dir, show_auto_load_dir,
				     auto_load_set_cmdlist_get (),
				     auto_load_show_cmdlist_get ());
  xfree (scripts_directory_help);

  auto_load_safe_path = xstrdup (AUTO_LOAD_SAFE_PATH);
  auto_load_safe_path_vec_update ();
  add_setshow_optional_filename_cmd ("safe-path", class_support,
				     &auto_load_safe_path, _("\
Set the list of files and directories that are safe for auto-loading."), _("\
Show the list of files and directories that are safe for auto-loading."), _("\
Various files loaded automatically for the 'set auto-load ...' options must\n\
be located in one of the directories listed by this option.  Warning will be\n\
printed and file will not be used otherwise.\n\
You can mix both directory and filename entries.\n\
Setting this parameter to an empty list resets it to its default value.\n\
Setting this parameter to '/' (without the quotes) allows any file\n\
for the 'set auto-load ...' options.  Each path entry can be also shell\n\
wildcard pattern; '*' does not match directory separator.\n\
This option is ignored for the kinds of files having 'set auto-load ... off'.\n\
This options has security implications for untrusted inferiors."),
				     set_auto_load_safe_path,
				     show_auto_load_safe_path,
				     auto_load_set_cmdlist_get (),
				     auto_load_show_cmdlist_get ());
  observer_attach_gdb_datadir_changed (auto_load_gdb_datadir_changed);

  cmd = add_cmd ("add-auto-load-safe-path", class_support,
		 add_auto_load_safe_path,
		 _("Add entries to the list of directories from which it is safe "
		   "to auto-load files.\n\
See the commands 'set auto-load safe-path' and 'show auto-load safe-path' to\n\
access the current full list setting."),
		 &cmdlist);
  set_cmd_completer (cmd, filename_completer);

  add_setshow_boolean_cmd ("auto-load", class_maintenance,
			   &debug_auto_load, _("\
Set auto-load verifications debugging."), _("\
Show auto-load verifications debugging."), _("\
When non-zero, debugging output for files of 'set auto-load ...'\n\
is displayed."),
			    NULL, show_debug_auto_load,
			    &setdebuglist, &showdebuglist);
}
