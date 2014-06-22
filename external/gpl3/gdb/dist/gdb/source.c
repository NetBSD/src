/* List lines of source files for GDB, the GNU debugger.
   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "symtab.h"
#include "expression.h"
#include "language.h"
#include "command.h"
#include "source.h"
#include "gdbcmd.h"
#include "frame.h"
#include "value.h"
#include "gdb_assert.h"
#include "filestuff.h"

#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gdbcore.h"
#include "gdb_regex.h"
#include "symfile.h"
#include "objfiles.h"
#include "annotate.h"
#include "gdbtypes.h"
#include "linespec.h"
#include "filenames.h"		/* for DOSish file names */
#include "completer.h"
#include "ui-out.h"
#include "readline/readline.h"

#define OPEN_MODE (O_RDONLY | O_BINARY)
#define FDOPEN_MODE FOPEN_RB

/* Prototypes for exported functions.  */

void _initialize_source (void);

/* Prototypes for local functions.  */

static int get_filename_and_charpos (struct symtab *, char **);

static void reverse_search_command (char *, int);

static void forward_search_command (char *, int);

static void line_info (char *, int);

static void source_info (char *, int);

/* Path of directories to search for source files.
   Same format as the PATH environment variable's value.  */

char *source_path;

/* Support for source path substitution commands.  */

struct substitute_path_rule
{
  char *from;
  char *to;
  struct substitute_path_rule *next;
};

static struct substitute_path_rule *substitute_path_rules = NULL;

/* Symtab of default file for listing lines of.  */

static struct symtab *current_source_symtab;

/* Default next line to list.  */

static int current_source_line;

static struct program_space *current_source_pspace;

/* Default number of lines to print with commands like "list".
   This is based on guessing how many long (i.e. more than chars_per_line
   characters) lines there will be.  To be completely correct, "list"
   and friends should be rewritten to count characters and see where
   things are wrapping, but that would be a fair amount of work.  */

int lines_to_list = 10;
static void
show_lines_to_list (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Number of source lines gdb "
		      "will list by default is %s.\n"),
		    value);
}

/* Possible values of 'set filename-display'.  */
static const char filename_display_basename[] = "basename";
static const char filename_display_relative[] = "relative";
static const char filename_display_absolute[] = "absolute";

static const char *const filename_display_kind_names[] = {
  filename_display_basename,
  filename_display_relative,
  filename_display_absolute,
  NULL
};

static const char *filename_display_string = filename_display_relative;

static void
show_filename_display_string (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Filenames are displayed as \"%s\".\n"), value);
}
 
/* Line number of last line printed.  Default for various commands.
   current_source_line is usually, but not always, the same as this.  */

static int last_line_listed;

/* First line number listed by last listing command.  */

static int first_line_listed;

/* Saves the name of the last source file visited and a possible error code.
   Used to prevent repeating annoying "No such file or directories" msgs.  */

static struct symtab *last_source_visited = NULL;
static int last_source_error = 0;

/* Return the first line listed by print_source_lines.
   Used by command interpreters to request listing from
   a previous point.  */

int
get_first_line_listed (void)
{
  return first_line_listed;
}

/* Return the default number of lines to print with commands like the
   cli "list".  The caller of print_source_lines must use this to
   calculate the end line and use it in the call to print_source_lines
   as it does not automatically use this value.  */

int
get_lines_to_list (void)
{
  return lines_to_list;
}

/* Return the current source file for listing and next line to list.
   NOTE: The returned sal pc and end fields are not valid.  */
   
struct symtab_and_line
get_current_source_symtab_and_line (void)
{
  struct symtab_and_line cursal = { 0 };

  cursal.pspace = current_source_pspace;
  cursal.symtab = current_source_symtab;
  cursal.line = current_source_line;
  cursal.pc = 0;
  cursal.end = 0;
  
  return cursal;
}

/* If the current source file for listing is not set, try and get a default.
   Usually called before get_current_source_symtab_and_line() is called.
   It may err out if a default cannot be determined.
   We must be cautious about where it is called, as it can recurse as the
   process of determining a new default may call the caller!
   Use get_current_source_symtab_and_line only to get whatever
   we have without erroring out or trying to get a default.  */
   
void
set_default_source_symtab_and_line (void)
{
  if (!have_full_symbols () && !have_partial_symbols ())
    error (_("No symbol table is loaded.  Use the \"file\" command."));

  /* Pull in a current source symtab if necessary.  */
  if (current_source_symtab == 0)
    select_source_symtab (0);
}

/* Return the current default file for listing and next line to list
   (the returned sal pc and end fields are not valid.)
   and set the current default to whatever is in SAL.
   NOTE: The returned sal pc and end fields are not valid.  */
   
struct symtab_and_line
set_current_source_symtab_and_line (const struct symtab_and_line *sal)
{
  struct symtab_and_line cursal = { 0 };

  cursal.pspace = current_source_pspace;
  cursal.symtab = current_source_symtab;
  cursal.line = current_source_line;
  cursal.pc = 0;
  cursal.end = 0;

  current_source_pspace = sal->pspace;
  current_source_symtab = sal->symtab;
  current_source_line = sal->line;

  return cursal;
}

/* Reset any information stored about a default file and line to print.  */

void
clear_current_source_symtab_and_line (void)
{
  current_source_symtab = 0;
  current_source_line = 0;
}

/* Set the source file default for the "list" command to be S.

   If S is NULL, and we don't have a default, find one.  This
   should only be called when the user actually tries to use the
   default, since we produce an error if we can't find a reasonable
   default.  Also, since this can cause symbols to be read, doing it
   before we need to would make things slower than necessary.  */

void
select_source_symtab (struct symtab *s)
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  struct objfile *ofp;

  if (s)
    {
      current_source_symtab = s;
      current_source_line = 1;
      current_source_pspace = SYMTAB_PSPACE (s);
      return;
    }

  if (current_source_symtab)
    return;

  /* Make the default place to list be the function `main'
     if one exists.  */
  if (lookup_symbol (main_name (), 0, VAR_DOMAIN, 0))
    {
      sals = decode_line_with_current_source (main_name (),
					      DECODE_LINE_FUNFIRSTLINE);
      sal = sals.sals[0];
      xfree (sals.sals);
      current_source_pspace = sal.pspace;
      current_source_symtab = sal.symtab;
      current_source_line = max (sal.line - (lines_to_list - 1), 1);
      if (current_source_symtab)
	return;
    }

  /* Alright; find the last file in the symtab list (ignoring .h's
     and namespace symtabs).  */

  current_source_line = 1;

  ALL_OBJFILES (ofp)
    {
      for (s = ofp->symtabs; s; s = s->next)
	{
	  const char *name = s->filename;
	  int len = strlen (name);

	  if (!(len > 2 && (strcmp (&name[len - 2], ".h") == 0
	      || strcmp (name, "<<C++-namespaces>>") == 0)))
	    {
	      current_source_pspace = current_program_space;
	      current_source_symtab = s;
	    }
	}
    }

  if (current_source_symtab)
    return;

  ALL_OBJFILES (ofp)
  {
    if (ofp->sf)
      s = ofp->sf->qf->find_last_source_symtab (ofp);
    if (s)
      current_source_symtab = s;
  }
  if (current_source_symtab)
    return;

  error (_("Can't find a default source file"));
}

/* Handler for "set directories path-list" command.
   "set dir mumble" doesn't prepend paths, it resets the entire
   path list.  The theory is that set(show(dir)) should be a no-op.  */

static void
set_directories_command (char *args, int from_tty, struct cmd_list_element *c)
{
  /* This is the value that was set.
     It needs to be processed to maintain $cdir:$cwd and remove dups.  */
  char *set_path = source_path;

  /* We preserve the invariant that $cdir:$cwd begins life at the end of
     the list by calling init_source_path.  If they appear earlier in
     SET_PATH then mod_path will move them appropriately.
     mod_path will also remove duplicates.  */
  init_source_path ();
  if (*set_path != '\0')
    mod_path (set_path, &source_path);

  xfree (set_path);
}

/* Print the list of source directories.
   This is used by the "ld" command, so it has the signature of a command
   function.  */

static void
show_directories_1 (char *ignore, int from_tty)
{
  puts_filtered ("Source directories searched: ");
  puts_filtered (source_path);
  puts_filtered ("\n");
}

/* Handler for "show directories" command.  */

static void
show_directories_command (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  show_directories_1 (NULL, from_tty);
}

/* Forget line positions and file names for the symtabs in a
   particular objfile.  */

void
forget_cached_source_info_for_objfile (struct objfile *objfile)
{
  struct symtab *s;

  ALL_OBJFILE_SYMTABS (objfile, s)
    {
      if (s->line_charpos != NULL)
	{
	  xfree (s->line_charpos);
	  s->line_charpos = NULL;
	}
      if (s->fullname != NULL)
	{
	  xfree (s->fullname);
	  s->fullname = NULL;
	}
    }

  if (objfile->sf)
    objfile->sf->qf->forget_cached_source_info (objfile);
}

/* Forget what we learned about line positions in source files, and
   which directories contain them; must check again now since files
   may be found in a different directory now.  */

void
forget_cached_source_info (void)
{
  struct program_space *pspace;
  struct objfile *objfile;

  ALL_PSPACES (pspace)
    ALL_PSPACE_OBJFILES (pspace, objfile)
    {
      forget_cached_source_info_for_objfile (objfile);
    }

  last_source_visited = NULL;
}

void
init_source_path (void)
{
  char buf[20];

  xsnprintf (buf, sizeof (buf), "$cdir%c$cwd", DIRNAME_SEPARATOR);
  source_path = xstrdup (buf);
  forget_cached_source_info ();
}

/* Add zero or more directories to the front of the source path.  */

static void
directory_command (char *dirname, int from_tty)
{
  dont_repeat ();
  /* FIXME, this goes to "delete dir"...  */
  if (dirname == 0)
    {
      if (!from_tty || query (_("Reinitialize source path to empty? ")))
	{
	  xfree (source_path);
	  init_source_path ();
	}
    }
  else
    {
      mod_path (dirname, &source_path);
      forget_cached_source_info ();
    }
  if (from_tty)
    show_directories_1 ((char *) 0, from_tty);
}

/* Add a path given with the -d command line switch.
   This will not be quoted so we must not treat spaces as separators.  */

void
directory_switch (char *dirname, int from_tty)
{
  add_path (dirname, &source_path, 0);
}

/* Add zero or more directories to the front of an arbitrary path.  */

void
mod_path (char *dirname, char **which_path)
{
  add_path (dirname, which_path, 1);
}

/* Workhorse of mod_path.  Takes an extra argument to determine
   if dirname should be parsed for separators that indicate multiple
   directories.  This allows for interfaces that pre-parse the dirname
   and allow specification of traditional separator characters such
   as space or tab.  */

void
add_path (char *dirname, char **which_path, int parse_separators)
{
  char *old = *which_path;
  int prefix = 0;
  VEC (char_ptr) *dir_vec = NULL;
  struct cleanup *back_to;
  int ix;
  char *name;

  if (dirname == 0)
    return;

  if (parse_separators)
    {
      char **argv, **argvp;

      /* This will properly parse the space and tab separators
	 and any quotes that may exist.  */
      argv = gdb_buildargv (dirname);

      for (argvp = argv; *argvp; argvp++)
	dirnames_to_char_ptr_vec_append (&dir_vec, *argvp);

      freeargv (argv);
    }
  else
    VEC_safe_push (char_ptr, dir_vec, xstrdup (dirname));
  back_to = make_cleanup_free_char_ptr_vec (dir_vec);

  for (ix = 0; VEC_iterate (char_ptr, dir_vec, ix, name); ++ix)
    {
      char *p;
      struct stat st;

      /* Spaces and tabs will have been removed by buildargv().
         NAME is the start of the directory.
	 P is the '\0' following the end.  */
      p = name + strlen (name);

      while (!(IS_DIR_SEPARATOR (*name) && p <= name + 1)	/* "/" */
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
      /* On MS-DOS and MS-Windows, h:\ is different from h: */
	     && !(p == name + 3 && name[1] == ':')		/* "d:/" */
#endif
	     && IS_DIR_SEPARATOR (p[-1]))
	/* Sigh.  "foo/" => "foo" */
	--p;
      *p = '\0';

      while (p > name && p[-1] == '.')
	{
	  if (p - name == 1)
	    {
	      /* "." => getwd ().  */
	      name = current_directory;
	      goto append;
	    }
	  else if (p > name + 1 && IS_DIR_SEPARATOR (p[-2]))
	    {
	      if (p - name == 2)
		{
		  /* "/." => "/".  */
		  *--p = '\0';
		  goto append;
		}
	      else
		{
		  /* "...foo/." => "...foo".  */
		  p -= 2;
		  *p = '\0';
		  continue;
		}
	    }
	  else
	    break;
	}

      if (name[0] == '~')
	name = tilde_expand (name);
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
      else if (IS_ABSOLUTE_PATH (name) && p == name + 2) /* "d:" => "d:." */
	name = concat (name, ".", (char *)NULL);
#endif
      else if (!IS_ABSOLUTE_PATH (name) && name[0] != '$')
	name = concat (current_directory, SLASH_STRING, name, (char *)NULL);
      else
	name = savestring (name, p - name);
      make_cleanup (xfree, name);

      /* Unless it's a variable, check existence.  */
      if (name[0] != '$')
	{
	  /* These are warnings, not errors, since we don't want a
	     non-existent directory in a .gdbinit file to stop processing
	     of the .gdbinit file.

	     Whether they get added to the path is more debatable.  Current
	     answer is yes, in case the user wants to go make the directory
	     or whatever.  If the directory continues to not exist/not be
	     a directory/etc, then having them in the path should be
	     harmless.  */
	  if (stat (name, &st) < 0)
	    {
	      int save_errno = errno;

	      fprintf_unfiltered (gdb_stderr, "Warning: ");
	      print_sys_errmsg (name, save_errno);
	    }
	  else if ((st.st_mode & S_IFMT) != S_IFDIR)
	    warning (_("%s is not a directory."), name);
	}

    append:
      {
	unsigned int len = strlen (name);
	char tinybuf[2];

	p = *which_path;
	while (1)
	  {
	    /* FIXME: we should use realpath() or its work-alike
	       before comparing.  Then all the code above which
	       removes excess slashes and dots could simply go away.  */
	    if (!filename_ncmp (p, name, len)
		&& (p[len] == '\0' || p[len] == DIRNAME_SEPARATOR))
	      {
		/* Found it in the search path, remove old copy.  */
		if (p > *which_path)
		  {
		    /* Back over leading separator.  */
		    p--;
		  }
		if (prefix > p - *which_path)
		  {
		    /* Same dir twice in one cmd.  */
		    goto skip_dup;
		  }
		/* Copy from next '\0' or ':'.  */
		memmove (p, &p[len + 1], strlen (&p[len + 1]) + 1);
	      }
	    p = strchr (p, DIRNAME_SEPARATOR);
	    if (p != 0)
	      ++p;
	    else
	      break;
	  }

	tinybuf[0] = DIRNAME_SEPARATOR;
	tinybuf[1] = '\0';

	/* If we have already tacked on a name(s) in this command,
	   be sure they stay on the front as we tack on some
	   more.  */
	if (prefix)
	  {
	    char *temp, c;

	    c = old[prefix];
	    old[prefix] = '\0';
	    temp = concat (old, tinybuf, name, (char *)NULL);
	    old[prefix] = c;
	    *which_path = concat (temp, "", &old[prefix], (char *) NULL);
	    prefix = strlen (temp);
	    xfree (temp);
	  }
	else
	  {
	    *which_path = concat (name, (old[0] ? tinybuf : old),
				  old, (char *)NULL);
	    prefix = strlen (name);
	  }
	xfree (old);
	old = *which_path;
      }
    skip_dup:
      ;
    }

  do_cleanups (back_to);
}


static void
source_info (char *ignore, int from_tty)
{
  struct symtab *s = current_source_symtab;

  if (!s)
    {
      printf_filtered (_("No current source file.\n"));
      return;
    }
  printf_filtered (_("Current source file is %s\n"), s->filename);
  if (s->dirname)
    printf_filtered (_("Compilation directory is %s\n"), s->dirname);
  if (s->fullname)
    printf_filtered (_("Located in %s\n"), s->fullname);
  if (s->nlines)
    printf_filtered (_("Contains %d line%s.\n"), s->nlines,
		     s->nlines == 1 ? "" : "s");

  printf_filtered (_("Source language is %s.\n"), language_str (s->language));
  printf_filtered (_("Compiled with %s debugging format.\n"), s->debugformat);
  printf_filtered (_("%s preprocessor macro info.\n"),
                   s->macro_table ? "Includes" : "Does not include");
}


/* Return True if the file NAME exists and is a regular file.  */
static int
is_regular_file (const char *name)
{
  struct stat st;
  const int status = stat (name, &st);

  /* Stat should never fail except when the file does not exist.
     If stat fails, analyze the source of error and return True
     unless the file does not exist, to avoid returning false results
     on obscure systems where stat does not work as expected.  */

  if (status != 0)
    return (errno != ENOENT);

  return S_ISREG (st.st_mode);
}

/* Open a file named STRING, searching path PATH (dir names sep by some char)
   using mode MODE in the calls to open.  You cannot use this function to
   create files (O_CREAT).

   OPTS specifies the function behaviour in specific cases.

   If OPF_TRY_CWD_FIRST, try to open ./STRING before searching PATH.
   (ie pretend the first element of PATH is ".").  This also indicates
   that, unless OPF_SEARCH_IN_PATH is also specified, a slash in STRING
   disables searching of the path (this is so that "exec-file ./foo" or
   "symbol-file ./foo" insures that you get that particular version of
   foo or an error message).

   If OPTS has OPF_SEARCH_IN_PATH set, absolute names will also be
   searched in path (we usually want this for source files but not for
   executables).

   If FILENAME_OPENED is non-null, set it to a newly allocated string naming
   the actual file opened (this string will always start with a "/").  We
   have to take special pains to avoid doubling the "/" between the directory
   and the file, sigh!  Emacs gets confuzzed by this when we print the
   source file name!!! 

   If OPTS has OPF_RETURN_REALPATH set return FILENAME_OPENED resolved by
   gdb_realpath.  Even without OPF_RETURN_REALPATH this function still returns
   filename starting with "/".  If FILENAME_OPENED is NULL this option has no
   effect.

   If a file is found, return the descriptor.
   Otherwise, return -1, with errno set for the last name we tried to open.  */

/*  >>>> This should only allow files of certain types,
    >>>>  eg executable, non-directory.  */
int
openp (const char *path, int opts, const char *string,
       int mode, char **filename_opened)
{
  int fd;
  char *filename;
  int alloclen;
  VEC (char_ptr) *dir_vec;
  struct cleanup *back_to;
  int ix;
  char *dir;

  /* The open syscall MODE parameter is not specified.  */
  gdb_assert ((mode & O_CREAT) == 0);
  gdb_assert (string != NULL);

  /* A file with an empty name cannot possibly exist.  Report a failure
     without further checking.

     This is an optimization which also defends us against buggy
     implementations of the "stat" function.  For instance, we have
     noticed that a MinGW debugger built on Windows XP 32bits crashes
     when the debugger is started with an empty argument.  */
  if (string[0] == '\0')
    {
      errno = ENOENT;
      return -1;
    }

  if (!path)
    path = ".";

  mode |= O_BINARY;

  if ((opts & OPF_TRY_CWD_FIRST) || IS_ABSOLUTE_PATH (string))
    {
      int i;

      if (is_regular_file (string))
	{
	  filename = alloca (strlen (string) + 1);
	  strcpy (filename, string);
	  fd = gdb_open_cloexec (filename, mode, 0);
	  if (fd >= 0)
	    goto done;
	}
      else
	{
	  filename = NULL;
	  fd = -1;
	}

      if (!(opts & OPF_SEARCH_IN_PATH))
	for (i = 0; string[i]; i++)
	  if (IS_DIR_SEPARATOR (string[i]))
	    goto done;
    }

  /* For dos paths, d:/foo -> /foo, and d:foo -> foo.  */
  if (HAS_DRIVE_SPEC (string))
    string = STRIP_DRIVE_SPEC (string);

  /* /foo => foo, to avoid multiple slashes that Emacs doesn't like.  */
  while (IS_DIR_SEPARATOR(string[0]))
    string++;

  /* ./foo => foo */
  while (string[0] == '.' && IS_DIR_SEPARATOR (string[1]))
    string += 2;

  alloclen = strlen (path) + strlen (string) + 2;
  filename = alloca (alloclen);
  fd = -1;

  dir_vec = dirnames_to_char_ptr_vec (path);
  back_to = make_cleanup_free_char_ptr_vec (dir_vec);

  for (ix = 0; VEC_iterate (char_ptr, dir_vec, ix, dir); ++ix)
    {
      size_t len = strlen (dir);

      if (strcmp (dir, "$cwd") == 0)
	{
	  /* Name is $cwd -- insert current directory name instead.  */
	  int newlen;

	  /* First, realloc the filename buffer if too short.  */
	  len = strlen (current_directory);
	  newlen = len + strlen (string) + 2;
	  if (newlen > alloclen)
	    {
	      alloclen = newlen;
	      filename = alloca (alloclen);
	    }
	  strcpy (filename, current_directory);
	}
      else if (strchr(dir, '~'))
	{
	 /* See whether we need to expand the tilde.  */
	  int newlen;
	  char *tilde_expanded;

	  tilde_expanded  = tilde_expand (dir);

	  /* First, realloc the filename buffer if too short.  */
	  len = strlen (tilde_expanded);
	  newlen = len + strlen (string) + 2;
	  if (newlen > alloclen)
	    {
	      alloclen = newlen;
	      filename = alloca (alloclen);
	    }
	  strcpy (filename, tilde_expanded);
	  xfree (tilde_expanded);
	}
      else
	{
	  /* Normal file name in path -- just use it.  */
	  strcpy (filename, dir);

	  /* Don't search $cdir.  It's also a magic path like $cwd, but we
	     don't have enough information to expand it.  The user *could*
	     have an actual directory named '$cdir' but handling that would
	     be confusing, it would mean different things in different
	     contexts.  If the user really has '$cdir' one can use './$cdir'.
	     We can get $cdir when loading scripts.  When loading source files
	     $cdir must have already been expanded to the correct value.  */
	  if (strcmp (dir, "$cdir") == 0)
	    continue;
	}

      /* Remove trailing slashes.  */
      while (len > 0 && IS_DIR_SEPARATOR (filename[len - 1]))
	filename[--len] = 0;

      strcat (filename + len, SLASH_STRING);
      strcat (filename, string);

      if (is_regular_file (filename))
	{
	  fd = gdb_open_cloexec (filename, mode, 0);
	  if (fd >= 0)
	    break;
	}
    }

  do_cleanups (back_to);

done:
  if (filename_opened)
    {
      /* If a file was opened, canonicalize its filename.  */
      if (fd < 0)
	*filename_opened = NULL;
      else if ((opts & OPF_RETURN_REALPATH) != 0)
	*filename_opened = gdb_realpath (filename);
      else
	*filename_opened = gdb_abspath (filename);
    }

  return fd;
}


/* This is essentially a convenience, for clients that want the behaviour
   of openp, using source_path, but that really don't want the file to be
   opened but want instead just to know what the full pathname is (as
   qualified against source_path).

   The current working directory is searched first.

   If the file was found, this function returns 1, and FULL_PATHNAME is
   set to the fully-qualified pathname.

   Else, this functions returns 0, and FULL_PATHNAME is set to NULL.  */
int
source_full_path_of (const char *filename, char **full_pathname)
{
  int fd;

  fd = openp (source_path,
	      OPF_TRY_CWD_FIRST | OPF_SEARCH_IN_PATH | OPF_RETURN_REALPATH,
	      filename, O_RDONLY, full_pathname);
  if (fd < 0)
    {
      *full_pathname = NULL;
      return 0;
    }

  close (fd);
  return 1;
}

/* Return non-zero if RULE matches PATH, that is if the rule can be
   applied to PATH.  */

static int
substitute_path_rule_matches (const struct substitute_path_rule *rule,
                              const char *path)
{
  const int from_len = strlen (rule->from);
  const int path_len = strlen (path);
  char *path_start;

  if (path_len < from_len)
    return 0;

  /* The substitution rules are anchored at the start of the path,
     so the path should start with rule->from.  There is no filename
     comparison routine, so we need to extract the first FROM_LEN
     characters from PATH first and use that to do the comparison.  */

  path_start = alloca (from_len + 1);
  strncpy (path_start, path, from_len);
  path_start[from_len] = '\0';

  if (FILENAME_CMP (path_start, rule->from) != 0)
    return 0;

  /* Make sure that the region in the path that matches the substitution
     rule is immediately followed by a directory separator (or the end of
     string character).  */
  
  if (path[from_len] != '\0' && !IS_DIR_SEPARATOR (path[from_len]))
    return 0;

  return 1;
}

/* Find the substitute-path rule that applies to PATH and return it.
   Return NULL if no rule applies.  */

static struct substitute_path_rule *
get_substitute_path_rule (const char *path)
{
  struct substitute_path_rule *rule = substitute_path_rules;

  while (rule != NULL && !substitute_path_rule_matches (rule, path))
    rule = rule->next;

  return rule;
}

/* If the user specified a source path substitution rule that applies
   to PATH, then apply it and return the new path.  This new path must
   be deallocated afterwards.
   
   Return NULL if no substitution rule was specified by the user,
   or if no rule applied to the given PATH.  */
   
char *
rewrite_source_path (const char *path)
{
  const struct substitute_path_rule *rule = get_substitute_path_rule (path);
  char *new_path;
  int from_len;
  
  if (rule == NULL)
    return NULL;

  from_len = strlen (rule->from);

  /* Compute the rewritten path and return it.  */

  new_path =
    (char *) xmalloc (strlen (path) + 1 + strlen (rule->to) - from_len);
  strcpy (new_path, rule->to);
  strcat (new_path, path + from_len);

  return new_path;
}

int
find_and_open_source (const char *filename,
		      const char *dirname,
		      char **fullname)
{
  char *path = source_path;
  const char *p;
  int result;
  struct cleanup *cleanup;

  /* Quick way out if we already know its full name.  */

  if (*fullname)
    {
      /* The user may have requested that source paths be rewritten
         according to substitution rules he provided.  If a substitution
         rule applies to this path, then apply it.  */
      char *rewritten_fullname = rewrite_source_path (*fullname);

      if (rewritten_fullname != NULL)
        {
          xfree (*fullname);
          *fullname = rewritten_fullname;
        }

      result = gdb_open_cloexec (*fullname, OPEN_MODE, 0);
      if (result >= 0)
	{
	  char *lpath = gdb_realpath (*fullname);

	  xfree (*fullname);
	  *fullname = lpath;
	  return result;
	}

      /* Didn't work -- free old one, try again.  */
      xfree (*fullname);
      *fullname = NULL;
    }

  cleanup = make_cleanup (null_cleanup, NULL);

  if (dirname != NULL)
    {
      /* If necessary, rewrite the compilation directory name according
         to the source path substitution rules specified by the user.  */

      char *rewritten_dirname = rewrite_source_path (dirname);

      if (rewritten_dirname != NULL)
        {
          make_cleanup (xfree, rewritten_dirname);
          dirname = rewritten_dirname;
        }
      
      /* Replace a path entry of $cdir with the compilation directory
	 name.  */
#define	cdir_len	5
      /* We cast strstr's result in case an ANSIhole has made it const,
         which produces a "required warning" when assigned to a nonconst.  */
      p = (char *) strstr (source_path, "$cdir");
      if (p && (p == path || p[-1] == DIRNAME_SEPARATOR)
	  && (p[cdir_len] == DIRNAME_SEPARATOR || p[cdir_len] == '\0'))
	{
	  int len;

	  path = (char *)
	    alloca (strlen (source_path) + 1 + strlen (dirname) + 1);
	  len = p - source_path;
	  strncpy (path, source_path, len);	/* Before $cdir */
	  strcpy (path + len, dirname);		/* new stuff */
	  strcat (path + len, source_path + len + cdir_len);	/* After
								   $cdir */
	}
    }

  if (IS_ABSOLUTE_PATH (filename))
    {
      /* If filename is absolute path, try the source path
	 substitution on it.  */
      char *rewritten_filename = rewrite_source_path (filename);

      if (rewritten_filename != NULL)
        {
          make_cleanup (xfree, rewritten_filename);
          filename = rewritten_filename;
        }
    }

  result = openp (path, OPF_SEARCH_IN_PATH | OPF_RETURN_REALPATH, filename,
		  OPEN_MODE, fullname);
  if (result < 0)
    {
      /* Didn't work.  Try using just the basename.  */
      p = lbasename (filename);
      if (p != filename)
	result = openp (path, OPF_SEARCH_IN_PATH | OPF_RETURN_REALPATH, p,
			OPEN_MODE, fullname);
    }

  do_cleanups (cleanup);
  return result;
}

/* Open a source file given a symtab S.  Returns a file descriptor or
   negative number for error.  
   
   This function is a convience function to find_and_open_source.  */

int
open_source_file (struct symtab *s)
{
  if (!s)
    return -1;

  return find_and_open_source (s->filename, s->dirname, &s->fullname);
}

/* Finds the fullname that a symtab represents.

   This functions finds the fullname and saves it in s->fullname.
   It will also return the value.

   If this function fails to find the file that this symtab represents,
   the expected fullname is used.  Therefore the files does not have to
   exist.  */

const char *
symtab_to_fullname (struct symtab *s)
{
  /* Use cached copy if we have it.
     We rely on forget_cached_source_info being called appropriately
     to handle cases like the file being moved.  */
  if (s->fullname == NULL)
    {
      int fd = find_and_open_source (s->filename, s->dirname, &s->fullname);

      if (fd >= 0)
	close (fd);
      else
	{
	  char *fullname;
	  struct cleanup *back_to;

	  /* rewrite_source_path would be applied by find_and_open_source, we
	     should report the pathname where GDB tried to find the file.  */

	  if (s->dirname == NULL || IS_ABSOLUTE_PATH (s->filename))
	    fullname = xstrdup (s->filename);
	  else
	    fullname = concat (s->dirname, SLASH_STRING, s->filename, NULL);

	  back_to = make_cleanup (xfree, fullname);
	  s->fullname = rewrite_source_path (fullname);
	  if (s->fullname == NULL)
	    s->fullname = xstrdup (fullname);
	  do_cleanups (back_to);
	}
    } 

  return s->fullname;
}

/* See commentary in source.h.  */

const char *
symtab_to_filename_for_display (struct symtab *symtab)
{
  if (filename_display_string == filename_display_basename)
    return lbasename (symtab->filename);
  else if (filename_display_string == filename_display_absolute)
    return symtab_to_fullname (symtab);
  else if (filename_display_string == filename_display_relative)
    return symtab->filename;
  else
    internal_error (__FILE__, __LINE__, _("invalid filename_display_string"));
}

/* Create and initialize the table S->line_charpos that records
   the positions of the lines in the source file, which is assumed
   to be open on descriptor DESC.
   All set S->nlines to the number of such lines.  */

void
find_source_lines (struct symtab *s, int desc)
{
  struct stat st;
  char *data, *p, *end;
  int nlines = 0;
  int lines_allocated = 1000;
  int *line_charpos;
  long mtime = 0;
  int size;

  gdb_assert (s);
  line_charpos = (int *) xmalloc (lines_allocated * sizeof (int));
  if (fstat (desc, &st) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  if (s->objfile && s->objfile->obfd)
    mtime = s->objfile->mtime;
  else if (exec_bfd)
    mtime = exec_bfd_mtime;

  if (mtime && mtime < st.st_mtime)
    warning (_("Source file is more recent than executable."));

  {
    struct cleanup *old_cleanups;

    /* st_size might be a large type, but we only support source files whose 
       size fits in an int.  */
    size = (int) st.st_size;

    /* Use malloc, not alloca, because this may be pretty large, and we may
       run into various kinds of limits on stack size.  */
    data = (char *) xmalloc (size);
    old_cleanups = make_cleanup (xfree, data);

    /* Reassign `size' to result of read for systems where \r\n -> \n.  */
    size = myread (desc, data, size);
    if (size < 0)
      perror_with_name (symtab_to_filename_for_display (s));
    end = data + size;
    p = data;
    line_charpos[0] = 0;
    nlines = 1;
    while (p != end)
      {
	if (*p++ == '\n'
	/* A newline at the end does not start a new line.  */
	    && p != end)
	  {
	    if (nlines == lines_allocated)
	      {
		lines_allocated *= 2;
		line_charpos =
		  (int *) xrealloc ((char *) line_charpos,
				    sizeof (int) * lines_allocated);
	      }
	    line_charpos[nlines++] = p - data;
	  }
      }
    do_cleanups (old_cleanups);
  }

  s->nlines = nlines;
  s->line_charpos =
    (int *) xrealloc ((char *) line_charpos, nlines * sizeof (int));

}



/* Get full pathname and line number positions for a symtab.
   Return nonzero if line numbers may have changed.
   Set *FULLNAME to actual name of the file as found by `openp',
   or to 0 if the file is not found.  */

static int
get_filename_and_charpos (struct symtab *s, char **fullname)
{
  int desc, linenums_changed = 0;
  struct cleanup *cleanups;

  desc = open_source_file (s);
  if (desc < 0)
    {
      if (fullname)
	*fullname = NULL;
      return 0;
    }
  cleanups = make_cleanup_close (desc);
  if (fullname)
    *fullname = s->fullname;
  if (s->line_charpos == 0)
    linenums_changed = 1;
  if (linenums_changed)
    find_source_lines (s, desc);
  do_cleanups (cleanups);
  return linenums_changed;
}

/* Print text describing the full name of the source file S
   and the line number LINE and its corresponding character position.
   The text starts with two Ctrl-z so that the Emacs-GDB interface
   can easily find it.

   MID_STATEMENT is nonzero if the PC is not at the beginning of that line.

   Return 1 if successful, 0 if could not find the file.  */

int
identify_source_line (struct symtab *s, int line, int mid_statement,
		      CORE_ADDR pc)
{
  if (s->line_charpos == 0)
    get_filename_and_charpos (s, (char **) NULL);
  if (s->fullname == 0)
    return 0;
  if (line > s->nlines)
    /* Don't index off the end of the line_charpos array.  */
    return 0;
  annotate_source (s->fullname, line, s->line_charpos[line - 1],
		   mid_statement, get_objfile_arch (s->objfile), pc);

  current_source_line = line;
  first_line_listed = line;
  last_line_listed = line;
  current_source_symtab = s;
  return 1;
}


/* Print source lines from the file of symtab S,
   starting with line number LINE and stopping before line number STOPLINE.  */

static void
print_source_lines_base (struct symtab *s, int line, int stopline,
			 enum print_source_lines_flags flags)
{
  int c;
  int desc;
  int noprint = 0;
  FILE *stream;
  int nlines = stopline - line;
  struct cleanup *cleanup;
  struct ui_out *uiout = current_uiout;

  /* Regardless of whether we can open the file, set current_source_symtab.  */
  current_source_symtab = s;
  current_source_line = line;
  first_line_listed = line;

  /* If printing of source lines is disabled, just print file and line
     number.  */
  if (ui_out_test_flags (uiout, ui_source_list))
    {
      /* Only prints "No such file or directory" once.  */
      if ((s != last_source_visited) || (!last_source_error))
	{
	  last_source_visited = s;
	  desc = open_source_file (s);
	}
      else
	{
	  desc = last_source_error;
	  flags |= PRINT_SOURCE_LINES_NOERROR;
	}
    }
  else
    {
      desc = last_source_error;
	  flags |= PRINT_SOURCE_LINES_NOERROR;
      noprint = 1;
    }

  if (desc < 0 || noprint)
    {
      last_source_error = desc;

      if (!(flags & PRINT_SOURCE_LINES_NOERROR))
	{
	  const char *filename = symtab_to_filename_for_display (s);
	  int len = strlen (filename) + 100;
	  char *name = alloca (len);

	  xsnprintf (name, len, "%d\t%s", line, filename);
	  print_sys_errmsg (name, errno);
	}
      else
	{
	  ui_out_field_int (uiout, "line", line);
	  ui_out_text (uiout, "\tin ");

	  /* CLI expects only the "file" field.  TUI expects only the
	     "fullname" field (and TUI does break if "file" is printed).
	     MI expects both fields.  ui_source_list is set only for CLI,
	     not for TUI.  */
	  if (ui_out_is_mi_like_p (uiout)
	      || ui_out_test_flags (uiout, ui_source_list))
	    ui_out_field_string (uiout, "file",
				 symtab_to_filename_for_display (s));
	  if (ui_out_is_mi_like_p (uiout)
	      || !ui_out_test_flags (uiout, ui_source_list))
 	    {
	      const char *s_fullname = symtab_to_fullname (s);
	      char *local_fullname;

	      /* ui_out_field_string may free S_FULLNAME by calling
		 open_source_file for it again.  See e.g.,
		 tui_field_string->tui_show_source.  */
	      local_fullname = alloca (strlen (s_fullname) + 1);
	      strcpy (local_fullname, s_fullname);

	      ui_out_field_string (uiout, "fullname", local_fullname);
 	    }

	  ui_out_text (uiout, "\n");
	}

      return;
    }

  last_source_error = 0;

  if (s->line_charpos == 0)
    find_source_lines (s, desc);

  if (line < 1 || line > s->nlines)
    {
      close (desc);
      error (_("Line number %d out of range; %s has %d lines."),
	     line, symtab_to_filename_for_display (s), s->nlines);
    }

  if (lseek (desc, s->line_charpos[line - 1], 0) < 0)
    {
      close (desc);
      perror_with_name (symtab_to_filename_for_display (s));
    }

  stream = fdopen (desc, FDOPEN_MODE);
  clearerr (stream);
  cleanup = make_cleanup_fclose (stream);

  while (nlines-- > 0)
    {
      char buf[20];

      c = fgetc (stream);
      if (c == EOF)
	break;
      last_line_listed = current_source_line;
      if (flags & PRINT_SOURCE_LINES_FILENAME)
        {
          ui_out_text (uiout, symtab_to_filename_for_display (s));
          ui_out_text (uiout, ":");
        }
      xsnprintf (buf, sizeof (buf), "%d\t", current_source_line++);
      ui_out_text (uiout, buf);
      do
	{
	  if (c < 040 && c != '\t' && c != '\n' && c != '\r')
	    {
	      xsnprintf (buf, sizeof (buf), "^%c", c + 0100);
	      ui_out_text (uiout, buf);
	    }
	  else if (c == 0177)
	    ui_out_text (uiout, "^?");
	  else if (c == '\r')
	    {
	      /* Skip a \r character, but only before a \n.  */
	      int c1 = fgetc (stream);

	      if (c1 != '\n')
		printf_filtered ("^%c", c + 0100);
	      if (c1 != EOF)
		ungetc (c1, stream);
	    }
	  else
	    {
	      xsnprintf (buf, sizeof (buf), "%c", c);
	      ui_out_text (uiout, buf);
	    }
	}
      while (c != '\n' && (c = fgetc (stream)) >= 0);
    }

  do_cleanups (cleanup);
}

/* Show source lines from the file of symtab S, starting with line
   number LINE and stopping before line number STOPLINE.  If this is
   not the command line version, then the source is shown in the source
   window otherwise it is simply printed.  */

void
print_source_lines (struct symtab *s, int line, int stopline,
		    enum print_source_lines_flags flags)
{
  print_source_lines_base (s, line, stopline, flags);
}

/* Print info on range of pc's in a specified line.  */

static void
line_info (char *arg, int from_tty)
{
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;
  CORE_ADDR start_pc, end_pc;
  int i;
  struct cleanup *cleanups;

  init_sal (&sal);		/* initialize to zeroes */

  if (arg == 0)
    {
      sal.symtab = current_source_symtab;
      sal.pspace = current_program_space;
      sal.line = last_line_listed;
      sals.nelts = 1;
      sals.sals = (struct symtab_and_line *)
	xmalloc (sizeof (struct symtab_and_line));
      sals.sals[0] = sal;
    }
  else
    {
      sals = decode_line_with_last_displayed (arg, DECODE_LINE_LIST_MODE);

      dont_repeat ();
    }

  cleanups = make_cleanup (xfree, sals.sals);

  /* C++  More than one line may have been specified, as when the user
     specifies an overloaded function name.  Print info on them all.  */
  for (i = 0; i < sals.nelts; i++)
    {
      sal = sals.sals[i];
      if (sal.pspace != current_program_space)
	continue;

      if (sal.symtab == 0)
	{
	  struct gdbarch *gdbarch = get_current_arch ();

	  printf_filtered (_("No line number information available"));
	  if (sal.pc != 0)
	    {
	      /* This is useful for "info line *0x7f34".  If we can't tell the
	         user about a source line, at least let them have the symbolic
	         address.  */
	      printf_filtered (" for address ");
	      wrap_here ("  ");
	      print_address (gdbarch, sal.pc, gdb_stdout);
	    }
	  else
	    printf_filtered (".");
	  printf_filtered ("\n");
	}
      else if (sal.line > 0
	       && find_line_pc_range (sal, &start_pc, &end_pc))
	{
	  struct gdbarch *gdbarch = get_objfile_arch (sal.symtab->objfile);

	  if (start_pc == end_pc)
	    {
	      printf_filtered ("Line %d of \"%s\"",
			       sal.line,
			       symtab_to_filename_for_display (sal.symtab));
	      wrap_here ("  ");
	      printf_filtered (" is at address ");
	      print_address (gdbarch, start_pc, gdb_stdout);
	      wrap_here ("  ");
	      printf_filtered (" but contains no code.\n");
	    }
	  else
	    {
	      printf_filtered ("Line %d of \"%s\"",
			       sal.line,
			       symtab_to_filename_for_display (sal.symtab));
	      wrap_here ("  ");
	      printf_filtered (" starts at address ");
	      print_address (gdbarch, start_pc, gdb_stdout);
	      wrap_here ("  ");
	      printf_filtered (" and ends at ");
	      print_address (gdbarch, end_pc, gdb_stdout);
	      printf_filtered (".\n");
	    }

	  /* x/i should display this line's code.  */
	  set_next_address (gdbarch, start_pc);

	  /* Repeating "info line" should do the following line.  */
	  last_line_listed = sal.line + 1;

	  /* If this is the only line, show the source code.  If it could
	     not find the file, don't do anything special.  */
	  if (annotation_level && sals.nelts == 1)
	    identify_source_line (sal.symtab, sal.line, 0, start_pc);
	}
      else
	/* Is there any case in which we get here, and have an address
	   which the user would want to see?  If we have debugging symbols
	   and no line numbers?  */
	printf_filtered (_("Line number %d is out of range for \"%s\".\n"),
			 sal.line, symtab_to_filename_for_display (sal.symtab));
    }
  do_cleanups (cleanups);
}

/* Commands to search the source file for a regexp.  */

static void
forward_search_command (char *regex, int from_tty)
{
  int c;
  int desc;
  FILE *stream;
  int line;
  char *msg;
  struct cleanup *cleanups;

  line = last_line_listed + 1;

  msg = (char *) re_comp (regex);
  if (msg)
    error (("%s"), msg);

  if (current_source_symtab == 0)
    select_source_symtab (0);

  desc = open_source_file (current_source_symtab);
  if (desc < 0)
    perror_with_name (symtab_to_filename_for_display (current_source_symtab));
  cleanups = make_cleanup_close (desc);

  if (current_source_symtab->line_charpos == 0)
    find_source_lines (current_source_symtab, desc);

  if (line < 1 || line > current_source_symtab->nlines)
    error (_("Expression not found"));

  if (lseek (desc, current_source_symtab->line_charpos[line - 1], 0) < 0)
    perror_with_name (symtab_to_filename_for_display (current_source_symtab));

  discard_cleanups (cleanups);
  stream = fdopen (desc, FDOPEN_MODE);
  clearerr (stream);
  cleanups = make_cleanup_fclose (stream);
  while (1)
    {
      static char *buf = NULL;
      char *p;
      int cursize, newsize;

      cursize = 256;
      buf = xmalloc (cursize);
      p = buf;

      c = fgetc (stream);
      if (c == EOF)
	break;
      do
	{
	  *p++ = c;
	  if (p - buf == cursize)
	    {
	      newsize = cursize + cursize / 2;
	      buf = xrealloc (buf, newsize);
	      p = buf + cursize;
	      cursize = newsize;
	    }
	}
      while (c != '\n' && (c = fgetc (stream)) >= 0);

      /* Remove the \r, if any, at the end of the line, otherwise
         regular expressions that end with $ or \n won't work.  */
      if (p - buf > 1 && p[-2] == '\r')
	{
	  p--;
	  p[-1] = '\n';
	}

      /* We now have a source line in buf, null terminate and match.  */
      *p = 0;
      if (re_exec (buf) > 0)
	{
	  /* Match!  */
	  do_cleanups (cleanups);
	  print_source_lines (current_source_symtab, line, line + 1, 0);
	  set_internalvar_integer (lookup_internalvar ("_"), line);
	  current_source_line = max (line - lines_to_list / 2, 1);
	  return;
	}
      line++;
    }

  printf_filtered (_("Expression not found\n"));
  do_cleanups (cleanups);
}

static void
reverse_search_command (char *regex, int from_tty)
{
  int c;
  int desc;
  FILE *stream;
  int line;
  char *msg;
  struct cleanup *cleanups;

  line = last_line_listed - 1;

  msg = (char *) re_comp (regex);
  if (msg)
    error (("%s"), msg);

  if (current_source_symtab == 0)
    select_source_symtab (0);

  desc = open_source_file (current_source_symtab);
  if (desc < 0)
    perror_with_name (symtab_to_filename_for_display (current_source_symtab));
  cleanups = make_cleanup_close (desc);

  if (current_source_symtab->line_charpos == 0)
    find_source_lines (current_source_symtab, desc);

  if (line < 1 || line > current_source_symtab->nlines)
    error (_("Expression not found"));

  if (lseek (desc, current_source_symtab->line_charpos[line - 1], 0) < 0)
    perror_with_name (symtab_to_filename_for_display (current_source_symtab));

  discard_cleanups (cleanups);
  stream = fdopen (desc, FDOPEN_MODE);
  clearerr (stream);
  cleanups = make_cleanup_fclose (stream);
  while (line > 1)
    {
/* FIXME!!!  We walk right off the end of buf if we get a long line!!!  */
      char buf[4096];		/* Should be reasonable???  */
      char *p = buf;

      c = fgetc (stream);
      if (c == EOF)
	break;
      do
	{
	  *p++ = c;
	}
      while (c != '\n' && (c = fgetc (stream)) >= 0);

      /* Remove the \r, if any, at the end of the line, otherwise
         regular expressions that end with $ or \n won't work.  */
      if (p - buf > 1 && p[-2] == '\r')
	{
	  p--;
	  p[-1] = '\n';
	}

      /* We now have a source line in buf; null terminate and match.  */
      *p = 0;
      if (re_exec (buf) > 0)
	{
	  /* Match!  */
	  do_cleanups (cleanups);
	  print_source_lines (current_source_symtab, line, line + 1, 0);
	  set_internalvar_integer (lookup_internalvar ("_"), line);
	  current_source_line = max (line - lines_to_list / 2, 1);
	  return;
	}
      line--;
      if (fseek (stream, current_source_symtab->line_charpos[line - 1], 0) < 0)
	{
	  const char *filename;

	  do_cleanups (cleanups);
	  filename = symtab_to_filename_for_display (current_source_symtab);
	  perror_with_name (filename);
	}
    }

  printf_filtered (_("Expression not found\n"));
  do_cleanups (cleanups);
  return;
}

/* If the last character of PATH is a directory separator, then strip it.  */

static void
strip_trailing_directory_separator (char *path)
{
  const int last = strlen (path) - 1;

  if (last < 0)
    return;  /* No stripping is needed if PATH is the empty string.  */

  if (IS_DIR_SEPARATOR (path[last]))
    path[last] = '\0';
}

/* Return the path substitution rule that matches FROM.
   Return NULL if no rule matches.  */

static struct substitute_path_rule *
find_substitute_path_rule (const char *from)
{
  struct substitute_path_rule *rule = substitute_path_rules;

  while (rule != NULL)
    {
      if (FILENAME_CMP (rule->from, from) == 0)
        return rule;
      rule = rule->next;
    }

  return NULL;
}

/* Add a new substitute-path rule at the end of the current list of rules.
   The new rule will replace FROM into TO.  */

void
add_substitute_path_rule (char *from, char *to)
{
  struct substitute_path_rule *rule;
  struct substitute_path_rule *new_rule;

  new_rule = xmalloc (sizeof (struct substitute_path_rule));
  new_rule->from = xstrdup (from);
  new_rule->to = xstrdup (to);
  new_rule->next = NULL;

  /* If the list of rules are empty, then insert the new rule
     at the head of the list.  */

  if (substitute_path_rules == NULL)
    {
      substitute_path_rules = new_rule;
      return;
    }

  /* Otherwise, skip to the last rule in our list and then append
     the new rule.  */

  rule = substitute_path_rules;
  while (rule->next != NULL)
    rule = rule->next;

  rule->next = new_rule;
}

/* Remove the given source path substitution rule from the current list
   of rules.  The memory allocated for that rule is also deallocated.  */

static void
delete_substitute_path_rule (struct substitute_path_rule *rule)
{
  if (rule == substitute_path_rules)
    substitute_path_rules = rule->next;
  else
    {
      struct substitute_path_rule *prev = substitute_path_rules;

      while (prev != NULL && prev->next != rule)
        prev = prev->next;

      gdb_assert (prev != NULL);

      prev->next = rule->next;
    }

  xfree (rule->from);
  xfree (rule->to);
  xfree (rule);
}

/* Implement the "show substitute-path" command.  */

static void
show_substitute_path_command (char *args, int from_tty)
{
  struct substitute_path_rule *rule = substitute_path_rules;
  char **argv;
  char *from = NULL;
  struct cleanup *cleanup;
  
  argv = gdb_buildargv (args);
  cleanup = make_cleanup_freeargv (argv);

  /* We expect zero or one argument.  */

  if (argv != NULL && argv[0] != NULL && argv[1] != NULL)
    error (_("Too many arguments in command"));

  if (argv != NULL && argv[0] != NULL)
    from = argv[0];

  /* Print the substitution rules.  */

  if (from != NULL)
    printf_filtered
      (_("Source path substitution rule matching `%s':\n"), from);
  else
    printf_filtered (_("List of all source path substitution rules:\n"));

  while (rule != NULL)
    {
      if (from == NULL || FILENAME_CMP (rule->from, from) == 0)
        printf_filtered ("  `%s' -> `%s'.\n", rule->from, rule->to);
      rule = rule->next;
    }

  do_cleanups (cleanup);
}

/* Implement the "unset substitute-path" command.  */

static void
unset_substitute_path_command (char *args, int from_tty)
{
  struct substitute_path_rule *rule = substitute_path_rules;
  char **argv = gdb_buildargv (args);
  char *from = NULL;
  int rule_found = 0;
  struct cleanup *cleanup;

  /* This function takes either 0 or 1 argument.  */

  cleanup = make_cleanup_freeargv (argv);
  if (argv != NULL && argv[0] != NULL && argv[1] != NULL)
    error (_("Incorrect usage, too many arguments in command"));

  if (argv != NULL && argv[0] != NULL)
    from = argv[0];

  /* If the user asked for all the rules to be deleted, ask him
     to confirm and give him a chance to abort before the action
     is performed.  */

  if (from == NULL
      && !query (_("Delete all source path substitution rules? ")))
    error (_("Canceled"));

  /* Delete the rule matching the argument.  No argument means that
     all rules should be deleted.  */

  while (rule != NULL)
    {
      struct substitute_path_rule *next = rule->next;

      if (from == NULL || FILENAME_CMP (from, rule->from) == 0)
        {
          delete_substitute_path_rule (rule);
          rule_found = 1;
        }

      rule = next;
    }
  
  /* If the user asked for a specific rule to be deleted but
     we could not find it, then report an error.  */

  if (from != NULL && !rule_found)
    error (_("No substitution rule defined for `%s'"), from);

  forget_cached_source_info ();

  do_cleanups (cleanup);
}

/* Add a new source path substitution rule.  */

static void
set_substitute_path_command (char *args, int from_tty)
{
  char **argv;
  struct substitute_path_rule *rule;
  struct cleanup *cleanup;
  
  argv = gdb_buildargv (args);
  cleanup = make_cleanup_freeargv (argv);

  if (argv == NULL || argv[0] == NULL || argv [1] == NULL)
    error (_("Incorrect usage, too few arguments in command"));

  if (argv[2] != NULL)
    error (_("Incorrect usage, too many arguments in command"));

  if (*(argv[0]) == '\0')
    error (_("First argument must be at least one character long"));

  /* Strip any trailing directory separator character in either FROM
     or TO.  The substitution rule already implicitly contains them.  */
  strip_trailing_directory_separator (argv[0]);
  strip_trailing_directory_separator (argv[1]);

  /* If a rule with the same "from" was previously defined, then
     delete it.  This new rule replaces it.  */

  rule = find_substitute_path_rule (argv[0]);
  if (rule != NULL)
    delete_substitute_path_rule (rule);
      
  /* Insert the new substitution rule.  */

  add_substitute_path_rule (argv[0], argv[1]);
  forget_cached_source_info ();

  do_cleanups (cleanup);
}


void
_initialize_source (void)
{
  struct cmd_list_element *c;

  current_source_symtab = 0;
  init_source_path ();

  /* The intention is to use POSIX Basic Regular Expressions.
     Always use the GNU regex routine for consistency across all hosts.
     Our current GNU regex.c does not have all the POSIX features, so this is
     just an approximation.  */
  re_set_syntax (RE_SYNTAX_GREP);

  c = add_cmd ("directory", class_files, directory_command, _("\
Add directory DIR to beginning of search path for source files.\n\
Forget cached info on source file locations and line positions.\n\
DIR can also be $cwd for the current working directory, or $cdir for the\n\
directory in which the source file was compiled into object code.\n\
With no argument, reset the search path to $cdir:$cwd, the default."),
	       &cmdlist);

  if (dbx_commands)
    add_com_alias ("use", "directory", class_files, 0);

  set_cmd_completer (c, filename_completer);

  add_setshow_optional_filename_cmd ("directories",
				     class_files,
				     &source_path,
				     _("\
Set the search path for finding source files."),
				     _("\
Show the search path for finding source files."),
				     _("\
$cwd in the path means the current working directory.\n\
$cdir in the path means the compilation directory of the source file.\n\
GDB ensures the search path always ends with $cdir:$cwd by\n\
appending these directories if necessary.\n\
Setting the value to an empty string sets it to $cdir:$cwd, the default."),
			    set_directories_command,
			    show_directories_command,
			    &setlist, &showlist);

  if (xdb_commands)
    {
      add_com_alias ("D", "directory", class_files, 0);
      add_cmd ("ld", no_class, show_directories_1, _("\
Current search path for finding source files.\n\
$cwd in the path means the current working directory.\n\
$cdir in the path means the compilation directory of the source file."),
	       &cmdlist);
    }

  add_info ("source", source_info,
	    _("Information about the current source file."));

  add_info ("line", line_info, _("\
Core addresses of the code for a source line.\n\
Line can be specified as\n\
  LINENUM, to list around that line in current file,\n\
  FILE:LINENUM, to list around that line in that file,\n\
  FUNCTION, to list around beginning of that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
Default is to describe the last source line that was listed.\n\n\
This sets the default address for \"x\" to the line's first instruction\n\
so that \"x/i\" suffices to start examining the machine code.\n\
The address is also stored as the value of \"$_\"."));

  add_com ("forward-search", class_files, forward_search_command, _("\
Search for regular expression (see regex(3)) from last line listed.\n\
The matching line number is also stored as the value of \"$_\"."));
  add_com_alias ("search", "forward-search", class_files, 0);
  add_com_alias ("fo", "forward-search", class_files, 1);

  add_com ("reverse-search", class_files, reverse_search_command, _("\
Search backward for regular expression (see regex(3)) from last line listed.\n\
The matching line number is also stored as the value of \"$_\"."));
  add_com_alias ("rev", "reverse-search", class_files, 1);

  if (xdb_commands)
    {
      add_com_alias ("/", "forward-search", class_files, 0);
      add_com_alias ("?", "reverse-search", class_files, 0);
    }

  add_setshow_integer_cmd ("listsize", class_support, &lines_to_list, _("\
Set number of source lines gdb will list by default."), _("\
Show number of source lines gdb will list by default."), _("\
Use this to choose how many source lines the \"list\" displays (unless\n\
the \"list\" argument explicitly specifies some other number).\n\
A value of \"unlimited\", or zero, means there's no limit."),
			    NULL,
			    show_lines_to_list,
			    &setlist, &showlist);

  add_cmd ("substitute-path", class_files, set_substitute_path_command,
           _("\
Usage: set substitute-path FROM TO\n\
Add a substitution rule replacing FROM into TO in source file names.\n\
If a substitution rule was previously set for FROM, the old rule\n\
is replaced by the new one."),
           &setlist);

  add_cmd ("substitute-path", class_files, unset_substitute_path_command,
           _("\
Usage: unset substitute-path [FROM]\n\
Delete the rule for substituting FROM in source file names.  If FROM\n\
is not specified, all substituting rules are deleted.\n\
If the debugger cannot find a rule for FROM, it will display a warning."),
           &unsetlist);

  add_cmd ("substitute-path", class_files, show_substitute_path_command,
           _("\
Usage: show substitute-path [FROM]\n\
Print the rule for substituting FROM in source file names. If FROM\n\
is not specified, print all substitution rules."),
           &showlist);

  add_setshow_enum_cmd ("filename-display", class_files,
			filename_display_kind_names,
			&filename_display_string, _("\
Set how to display filenames."), _("\
Show how to display filenames."), _("\
filename-display can be:\n\
  basename - display only basename of a filename\n\
  relative - display a filename relative to the compilation directory\n\
  absolute - display an absolute filename\n\
By default, relative filenames are displayed."),
			NULL,
			show_filename_display_string,
			&setlist, &showlist);

}
