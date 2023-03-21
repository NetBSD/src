/* Top level stuff for GDB, the GNU debugger.

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
#include "top.h"
#include "target.h"
#include "inferior.h"
#include "symfile.h"
#include "gdbcore.h"
#include "getopt.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "gdbsupport/event-loop.h"
#include "ui-out.h"

#include "interps.h"
#include "main.h"
#include "source.h"
#include "cli/cli-cmds.h"
#include "objfiles.h"
#include "auto-load.h"
#include "maint.h"

#include "filenames.h"
#include "gdbsupport/filestuff.h"
#include <signal.h>
#include "event-top.h"
#include "infrun.h"
#include "gdbsupport/signals-state-save-restore.h"
#include <algorithm>
#include <vector>
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"
#ifdef GDBTK
#include "gdbtk/generic/gdbtk.h"
#endif
#include "gdbsupport/alt-stack.h"
#include "observable.h"

/* The selected interpreter.  This will be used as a set command
   variable, so it should always be malloc'ed - since
   do_setshow_command will free it.  */
char *interpreter_p;

/* Whether dbx commands will be handled.  */
int dbx_commands = 0;

/* System root path, used to find libraries etc.  */
char *gdb_sysroot = 0;

/* GDB datadir, used to store data files.  */
std::string gdb_datadir;

/* Non-zero if GDB_DATADIR was provided on the command line.
   This doesn't track whether data-directory is set later from the
   command line, but we don't reread system.gdbinit when that happens.  */
static int gdb_datadir_provided = 0;

/* If gdb was configured with --with-python=/path,
   the possibly relocated path to python's lib directory.  */
std::string python_libdir;

/* Target IO streams.  */
struct ui_file *gdb_stdtargin;
struct ui_file *gdb_stdtarg;
struct ui_file *gdb_stdtargerr;

/* True if --batch or --batch-silent was seen.  */
int batch_flag = 0;

/* Support for the --batch-silent option.  */
int batch_silent = 0;

/* Support for --return-child-result option.
   Set the default to -1 to return error in the case
   that the program does not run or does not complete.  */
int return_child_result = 0;
int return_child_result_value = -1;


/* GDB as it has been invoked from the command line (i.e. argv[0]).  */
static char *gdb_program_name;

/* Return read only pointer to GDB_PROGRAM_NAME.  */
const char *
get_gdb_program_name (void)
{
  return gdb_program_name;
}

static void print_gdb_help (struct ui_file *);

/* Set the data-directory parameter to NEW_DATADIR.
   If NEW_DATADIR is not a directory then a warning is printed.
   We don't signal an error for backward compatibility.  */

void
set_gdb_data_directory (const char *new_datadir)
{
  struct stat st;

  if (stat (new_datadir, &st) < 0)
    {
      int save_errno = errno;

      fprintf_unfiltered (gdb_stderr, "Warning: ");
      print_sys_errmsg (new_datadir, save_errno);
    }
  else if (!S_ISDIR (st.st_mode))
    warning (_("%ps is not a directory."),
	     styled_string (file_name_style.style (), new_datadir));

  gdb_datadir = gdb_realpath (new_datadir).get ();

  /* gdb_realpath won't return an absolute path if the path doesn't exist,
     but we still want to record an absolute path here.  If the user entered
     "../foo" and "../foo" doesn't exist then we'll record $(pwd)/../foo which
     isn't canonical, but that's ok.  */
  if (!IS_ABSOLUTE_PATH (gdb_datadir.c_str ()))
    {
      gdb::unique_xmalloc_ptr<char> abs_datadir
        = gdb_abspath (gdb_datadir.c_str ());

      gdb_datadir = abs_datadir.get ();
    }
}

/* Relocate a file or directory.  PROGNAME is the name by which gdb
   was invoked (i.e., argv[0]).  INITIAL is the default value for the
   file or directory.  RELOCATABLE is true if the value is relocatable,
   false otherwise.  This may return an empty string under the same
   conditions as make_relative_prefix returning NULL.  */

static std::string
relocate_path (const char *progname, const char *initial, bool relocatable)
{
  if (relocatable)
    {
      gdb::unique_xmalloc_ptr<char> str (make_relative_prefix (progname,
							       BINDIR,
							       initial));
      if (str != nullptr)
	return str.get ();
      return std::string ();
    }
  return initial;
}

/* Like relocate_path, but specifically checks for a directory.
   INITIAL is relocated according to the rules of relocate_path.  If
   the result is a directory, it is used; otherwise, INITIAL is used.
   The chosen directory is then canonicalized using lrealpath.  */

std::string
relocate_gdb_directory (const char *initial, bool relocatable)
{
  std::string dir = relocate_path (gdb_program_name, initial, relocatable);
  if (!dir.empty ())
    {
      struct stat s;

      if (stat (dir.c_str (), &s) != 0 || !S_ISDIR (s.st_mode))
	{
	  dir.clear ();
	}
    }
  if (dir.empty ())
    dir = initial;

  /* Canonicalize the directory.  */
  if (!dir.empty ())
    {
      gdb::unique_xmalloc_ptr<char> canon_sysroot (lrealpath (dir.c_str ()));

      if (canon_sysroot)
	dir = canon_sysroot.get ();
    }

  return dir;
}

/* Given a gdbinit path in FILE, adjusts it according to the gdb_datadir
   parameter if it is in the data dir, or passes it through relocate_path
   otherwise.  */

static std::string
relocate_gdbinit_path_maybe_in_datadir (const std::string &file,
					bool relocatable)
{
  size_t datadir_len = strlen (GDB_DATADIR);

  std::string relocated_path;

  /* If SYSTEM_GDBINIT lives in data-directory, and data-directory
     has been provided, search for SYSTEM_GDBINIT there.  */
  if (gdb_datadir_provided
      && datadir_len < file.length ()
      && filename_ncmp (file.c_str (), GDB_DATADIR, datadir_len) == 0
      && IS_DIR_SEPARATOR (file[datadir_len]))
    {
      /* Append the part of SYSTEM_GDBINIT that follows GDB_DATADIR
	 to gdb_datadir.  */

      size_t start = datadir_len;
      for (; IS_DIR_SEPARATOR (file[start]); ++start)
	;
      relocated_path = gdb_datadir + SLASH_STRING + file.substr (start);
    }
  else
    {
      relocated_path = relocate_path (gdb_program_name, file.c_str (),
				      relocatable);
    }
    return relocated_path;
}

/* Compute the locations of init files that GDB should source and
   return them in SYSTEM_GDBINIT, HOME_GDBINIT, LOCAL_GDBINIT.  If
   there is no system gdbinit (resp. home gdbinit and local gdbinit)
   to be loaded, then SYSTEM_GDBINIT (resp. HOME_GDBINIT and
   LOCAL_GDBINIT) is set to the empty string.  */
static void
get_init_files (std::vector<std::string> *system_gdbinit,
		std::string *home_gdbinit,
		std::string *local_gdbinit)
{
  static std::vector<std::string> sysgdbinit;
  static std::string homeinit;
  static std::string localinit;
  static int initialized = 0;

  if (!initialized)
    {
      struct stat homebuf, cwdbuf, s;

      if (SYSTEM_GDBINIT[0])
	{
	  std::string relocated_sysgdbinit
	    = relocate_gdbinit_path_maybe_in_datadir
		(SYSTEM_GDBINIT, SYSTEM_GDBINIT_RELOCATABLE);
	  if (!relocated_sysgdbinit.empty ()
	      && stat (relocated_sysgdbinit.c_str (), &s) == 0)
	    sysgdbinit.push_back (relocated_sysgdbinit);
	}
      if (SYSTEM_GDBINIT_DIR[0])
	{
	  std::string relocated_gdbinit_dir
	    = relocate_gdbinit_path_maybe_in_datadir
		(SYSTEM_GDBINIT_DIR, SYSTEM_GDBINIT_DIR_RELOCATABLE);
	  if (!relocated_gdbinit_dir.empty ()) {
	    gdb_dir_up dir (opendir (relocated_gdbinit_dir.c_str ()));
	    if (dir != nullptr)
	      {
		std::vector<std::string> files;
		for (;;)
		  {
		    struct dirent *ent = readdir (dir.get ());
		    if (ent == nullptr)
		      break;
		    std::string name (ent->d_name);
		    if (name == "." || name == "..")
		      continue;
		    /* ent->d_type is not available on all systems (e.g. mingw,
		       Solaris), so we have to call stat().  */
		    std::string filename
		      = relocated_gdbinit_dir + SLASH_STRING + name;
		    if (stat (filename.c_str (), &s) != 0
			|| !S_ISREG (s.st_mode))
		      continue;
		    const struct extension_language_defn *extlang
		      = get_ext_lang_of_file (filename.c_str ());
		    /* We effectively don't support "set script-extension
		       off/soft", because we are loading system init files here,
		       so it does not really make sense to depend on a
		       setting.  */
		    if (extlang != nullptr && ext_lang_present_p (extlang))
		      files.push_back (std::move (filename));
		  }
		std::sort (files.begin (), files.end ());
		sysgdbinit.insert (sysgdbinit.end (),
				   files.begin (), files.end ());
	      }
	  }
	}

      const char *homedir = getenv ("HOME");

      /* If the .gdbinit file in the current directory is the same as
	 the $HOME/.gdbinit file, it should not be sourced.  homebuf
	 and cwdbuf are used in that purpose.  Make sure that the stats
	 are zero in case one of them fails (this guarantees that they
	 won't match if either exists).  */

      memset (&homebuf, 0, sizeof (struct stat));
      memset (&cwdbuf, 0, sizeof (struct stat));

      if (homedir)
	{
	  homeinit = std::string (homedir) + SLASH_STRING + GDBINIT;
	  if (stat (homeinit.c_str (), &homebuf) != 0)
	    {
	      homeinit = "";
	    }
	}

      if (stat (GDBINIT, &cwdbuf) == 0)
	{
	  if (homeinit.empty ()
	      || memcmp ((char *) &homebuf, (char *) &cwdbuf,
			 sizeof (struct stat)))
	    localinit = GDBINIT;
	}
      
      initialized = 1;
    }

  *system_gdbinit = sysgdbinit;
  *home_gdbinit = homeinit;
  *local_gdbinit = localinit;
}

/* Start up the event loop.  This is the entry point to the event loop
   from the command loop.  */

static void
start_event_loop ()
{
  /* Loop until there is nothing to do.  This is the entry point to
     the event loop engine.  gdb_do_one_event will process one event
     for each invocation.  It blocks waiting for an event and then
     processes it.  */
  while (1)
    {
      int result = 0;

      try
	{
	  result = gdb_do_one_event ();
	}
      catch (const gdb_exception &ex)
	{
	  exception_print (gdb_stderr, ex);

	  /* If any exception escaped to here, we better enable
	     stdin.  Otherwise, any command that calls async_disable_stdin,
	     and then throws, will leave stdin inoperable.  */
	  SWITCH_THRU_ALL_UIS ()
	    {
	      async_enable_stdin ();
	    }
	  /* If we long-jumped out of do_one_event, we probably didn't
	     get around to resetting the prompt, which leaves readline
	     in a messed-up state.  Reset it here.  */
	  current_ui->prompt_state = PROMPT_NEEDED;
	  gdb::observers::command_error.notify ();
	  /* This call looks bizarre, but it is required.  If the user
	     entered a command that caused an error,
	     after_char_processing_hook won't be called from
	     rl_callback_read_char_wrapper.  Using a cleanup there
	     won't work, since we want this function to be called
	     after a new prompt is printed.  */
	  if (after_char_processing_hook)
	    (*after_char_processing_hook) ();
	  /* Maybe better to set a flag to be checked somewhere as to
	     whether display the prompt or not.  */
	}

      if (result < 0)
	break;
    }

  /* We are done with the event loop.  There are no more event sources
     to listen to.  So we exit GDB.  */
  return;
}

/* Call command_loop.  */

/* Prevent inlining this function for the benefit of GDB's selftests
   in the testsuite.  Those tests want to run GDB under GDB and stop
   here.  */
static void captured_command_loop () __attribute__((noinline));

static void
captured_command_loop ()
{
  struct ui *ui = current_ui;

  /* Top-level execution commands can be run in the background from
     here on.  */
  current_ui->async = 1;

  /* Give the interpreter a chance to print a prompt, if necessary  */
  if (ui->prompt_state != PROMPT_BLOCKED)
    interp_pre_command_loop (top_level_interpreter ());

  /* Now it's time to start the event loop.  */
  start_event_loop ();

  /* If the command_loop returned, normally (rather than threw an
     error) we try to quit.  If the quit is aborted, our caller
     catches the signal and restarts the command loop.  */
  quit_command (NULL, ui->instream == ui->stdin_stream);
}

/* Handle command errors thrown from within catch_command_errors.  */

static int
handle_command_errors (const struct gdb_exception &e)
{
  if (e.reason < 0)
    {
      exception_print (gdb_stderr, e);

      /* If any exception escaped to here, we better enable stdin.
	 Otherwise, any command that calls async_disable_stdin, and
	 then throws, will leave stdin inoperable.  */
      async_enable_stdin ();
      return 0;
    }
  return 1;
}

/* Type of the command callback passed to the const
   catch_command_errors.  */

typedef void (catch_command_errors_const_ftype) (const char *, int);

/* Wrap calls to commands run before the event loop is started.  */

static int
catch_command_errors (catch_command_errors_const_ftype command,
		      const char *arg, int from_tty)
{
  try
    {
      int was_sync = current_ui->prompt_state == PROMPT_BLOCKED;

      command (arg, from_tty);

      maybe_wait_sync_command_done (was_sync);
    }
  catch (const gdb_exception &e)
    {
      return handle_command_errors (e);
    }

  return 1;
}

/* Adapter for symbol_file_add_main that translates 'from_tty' to a
   symfile_add_flags.  */

static void
symbol_file_add_main_adapter (const char *arg, int from_tty)
{
  symfile_add_flags add_flags = 0;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  symbol_file_add_main (arg, add_flags);
}

/* Perform validation of the '--readnow' and '--readnever' flags.  */

static void
validate_readnow_readnever ()
{
  if (readnever_symbol_files && readnow_symbol_files)
    {
      error (_("%s: '--readnow' and '--readnever' cannot be "
	       "specified simultaneously"),
	     gdb_program_name);
    }
}

/* Type of this option.  */
enum cmdarg_kind
{
  /* Option type -x.  */
  CMDARG_FILE,

  /* Option type -ex.  */
  CMDARG_COMMAND,

  /* Option type -ix.  */
  CMDARG_INIT_FILE,
    
  /* Option type -iex.  */
  CMDARG_INIT_COMMAND
};

/* Arguments of --command option and its counterpart.  */
struct cmdarg
{
  cmdarg (cmdarg_kind type_, char *string_)
    : type (type_), string (string_)
  {}

  /* Type of this option.  */
  enum cmdarg_kind type;

  /* Value of this option - filename or the GDB command itself.  String memory
     is not owned by this structure despite it is 'const'.  */
  char *string;
};

static void
captured_main_1 (struct captured_main_args *context)
{
  int argc = context->argc;
  char **argv = context->argv;

  static int quiet = 0;
  static int set_args = 0;
  static int inhibit_home_gdbinit = 0;

  /* Pointers to various arguments from command line.  */
  char *symarg = NULL;
  char *execarg = NULL;
  char *pidarg = NULL;
  char *corearg = NULL;
  char *pid_or_core_arg = NULL;
  char *cdarg = NULL;
  char *ttyarg = NULL;

  /* These are static so that we can take their address in an
     initializer.  */
  static int print_help;
  static int print_version;
  static int print_configuration;

  /* Pointers to all arguments of --command option.  */
  std::vector<struct cmdarg> cmdarg_vec;

  /* All arguments of --directory option.  */
  std::vector<char *> dirarg;

  int i;
  int save_auto_load;
  int ret = 1;

#ifdef HAVE_USEFUL_SBRK
  /* Set this before constructing scoped_command_stats.  */
  lim_at_start = (char *) sbrk (0);
#endif

  scoped_command_stats stat_reporter (false);

#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  notice_open_fds ();

#ifdef __MINGW32__
  /* Ensure stderr is unbuffered.  A Cygwin pty or pipe is implemented
     as a Windows pipe, and Windows buffers on pipes.  */
  setvbuf (stderr, NULL, _IONBF, BUFSIZ);
#endif

  /* Note: `error' cannot be called before this point, because the
     caller will crash when trying to print the exception.  */
  main_ui = new ui (stdin, stdout, stderr);
  current_ui = main_ui;

  gdb_stdtargerr = gdb_stderr;	/* for moment */
  gdb_stdtargin = gdb_stdin;	/* for moment */

  if (bfd_init () != BFD_INIT_MAGIC)
    error (_("fatal error: libbfd ABI mismatch"));

#ifdef __MINGW32__
  /* On Windows, argv[0] is not necessarily set to absolute form when
     GDB is found along PATH, without which relocation doesn't work.  */
  gdb_program_name = windows_get_absolute_argv0 (argv[0]);
#else
  gdb_program_name = xstrdup (argv[0]);
#endif

  /* Prefix warning messages with the command name.  */
  gdb::unique_xmalloc_ptr<char> tmp_warn_preprint
    (xstrprintf ("%s: warning: ", gdb_program_name));
  warning_pre_print = tmp_warn_preprint.get ();

  current_directory = getcwd (NULL, 0);
  if (current_directory == NULL)
    perror_warning_with_name (_("error finding working directory"));

  /* Set the sysroot path.  */
  gdb_sysroot
    = xstrdup (relocate_gdb_directory (TARGET_SYSTEM_ROOT,
				     TARGET_SYSTEM_ROOT_RELOCATABLE).c_str ());

  if (*gdb_sysroot == '\0')
    {
      xfree (gdb_sysroot);
      gdb_sysroot = xstrdup (TARGET_SYSROOT_PREFIX);
    }

  debug_file_directory
    = xstrdup (relocate_gdb_directory (DEBUGDIR,
				     DEBUGDIR_RELOCATABLE).c_str ());

  gdb_datadir = relocate_gdb_directory (GDB_DATADIR,
					GDB_DATADIR_RELOCATABLE);

#ifdef WITH_PYTHON_LIBDIR
  python_libdir = relocate_gdb_directory (WITH_PYTHON_LIBDIR,
					  PYTHON_LIBDIR_RELOCATABLE);
#endif

#ifdef RELOC_SRCDIR
  add_substitute_path_rule (RELOC_SRCDIR,
			    make_relative_prefix (gdb_program_name, BINDIR,
						  RELOC_SRCDIR));
#endif

  /* There will always be an interpreter.  Either the one passed into
     this captured main, or one specified by the user at start up, or
     the console.  Initialize the interpreter to the one requested by 
     the application.  */
  interpreter_p = xstrdup (context->interpreter_p);

  /* Parse arguments and options.  */
  {
    int c;
    /* When var field is 0, use flag field to record the equivalent
       short option (or arbitrary numbers starting at 10 for those
       with no equivalent).  */
    enum {
      OPT_SE = 10,
      OPT_CD,
      OPT_ANNOTATE,
      OPT_STATISTICS,
      OPT_TUI,
      OPT_NOWINDOWS,
      OPT_WINDOWS,
      OPT_IX,
      OPT_IEX,
      OPT_READNOW,
      OPT_READNEVER
    };
    /* This struct requires int* in the struct, but write_files is a bool.
       So use this temporary int that we write back after argument parsing.  */
    int write_files_1 = 0;
    static struct option long_options[] =
    {
      {"tui", no_argument, 0, OPT_TUI},
      {"dbx", no_argument, &dbx_commands, 1},
      {"readnow", no_argument, NULL, OPT_READNOW},
      {"readnever", no_argument, NULL, OPT_READNEVER},
      {"r", no_argument, NULL, OPT_READNOW},
      {"quiet", no_argument, &quiet, 1},
      {"q", no_argument, &quiet, 1},
      {"silent", no_argument, &quiet, 1},
      {"nh", no_argument, &inhibit_home_gdbinit, 1},
      {"nx", no_argument, &inhibit_gdbinit, 1},
      {"n", no_argument, &inhibit_gdbinit, 1},
      {"batch-silent", no_argument, 0, 'B'},
      {"batch", no_argument, &batch_flag, 1},

    /* This is a synonym for "--annotate=1".  --annotate is now
       preferred, but keep this here for a long time because people
       will be running emacses which use --fullname.  */
      {"fullname", no_argument, 0, 'f'},
      {"f", no_argument, 0, 'f'},

      {"annotate", required_argument, 0, OPT_ANNOTATE},
      {"help", no_argument, &print_help, 1},
      {"se", required_argument, 0, OPT_SE},
      {"symbols", required_argument, 0, 's'},
      {"s", required_argument, 0, 's'},
      {"exec", required_argument, 0, 'e'},
      {"e", required_argument, 0, 'e'},
      {"core", required_argument, 0, 'c'},
      {"c", required_argument, 0, 'c'},
      {"pid", required_argument, 0, 'p'},
      {"p", required_argument, 0, 'p'},
      {"command", required_argument, 0, 'x'},
      {"eval-command", required_argument, 0, 'X'},
      {"version", no_argument, &print_version, 1},
      {"configuration", no_argument, &print_configuration, 1},
      {"x", required_argument, 0, 'x'},
      {"ex", required_argument, 0, 'X'},
      {"init-command", required_argument, 0, OPT_IX},
      {"init-eval-command", required_argument, 0, OPT_IEX},
      {"ix", required_argument, 0, OPT_IX},
      {"iex", required_argument, 0, OPT_IEX},
#ifdef GDBTK
      {"tclcommand", required_argument, 0, 'z'},
      {"enable-external-editor", no_argument, 0, 'y'},
      {"editor-command", required_argument, 0, 'w'},
#endif
      {"ui", required_argument, 0, 'i'},
      {"interpreter", required_argument, 0, 'i'},
      {"i", required_argument, 0, 'i'},
      {"directory", required_argument, 0, 'd'},
      {"d", required_argument, 0, 'd'},
      {"data-directory", required_argument, 0, 'D'},
      {"D", required_argument, 0, 'D'},
      {"cd", required_argument, 0, OPT_CD},
      {"tty", required_argument, 0, 't'},
      {"baud", required_argument, 0, 'b'},
      {"b", required_argument, 0, 'b'},
      {"nw", no_argument, NULL, OPT_NOWINDOWS},
      {"nowindows", no_argument, NULL, OPT_NOWINDOWS},
      {"w", no_argument, NULL, OPT_WINDOWS},
      {"windows", no_argument, NULL, OPT_WINDOWS},
      {"statistics", no_argument, 0, OPT_STATISTICS},
      {"write", no_argument, &write_files_1, 1},
      {"args", no_argument, &set_args, 1},
      {"l", required_argument, 0, 'l'},
      {"return-child-result", no_argument, &return_child_result, 1},
      {0, no_argument, 0, 0}
    };

    while (1)
      {
	int option_index;

	c = getopt_long_only (argc, argv, "",
			      long_options, &option_index);
	if (c == EOF || set_args)
	  break;

	/* Long option that takes an argument.  */
	if (c == 0 && long_options[option_index].flag == 0)
	  c = long_options[option_index].val;

	switch (c)
	  {
	  case 0:
	    /* Long option that just sets a flag.  */
	    break;
	  case OPT_SE:
	    symarg = optarg;
	    execarg = optarg;
	    break;
	  case OPT_CD:
	    cdarg = optarg;
	    break;
	  case OPT_ANNOTATE:
	    /* FIXME: what if the syntax is wrong (e.g. not digits)?  */
	    annotation_level = atoi (optarg);
	    break;
	  case OPT_STATISTICS:
	    /* Enable the display of both time and space usage.  */
	    set_per_command_time (1);
	    set_per_command_space (1);
	    break;
	  case OPT_TUI:
	    /* --tui is equivalent to -i=tui.  */
#ifdef TUI
	    xfree (interpreter_p);
	    interpreter_p = xstrdup (INTERP_TUI);
#else
	    error (_("%s: TUI mode is not supported"), gdb_program_name);
#endif
	    break;
	  case OPT_WINDOWS:
	    /* FIXME: cagney/2003-03-01: Not sure if this option is
               actually useful, and if it is, what it should do.  */
#ifdef GDBTK
	    /* --windows is equivalent to -i=insight.  */
	    xfree (interpreter_p);
	    interpreter_p = xstrdup (INTERP_INSIGHT);
#endif
	    break;
	  case OPT_NOWINDOWS:
	    /* -nw is equivalent to -i=console.  */
	    xfree (interpreter_p);
	    interpreter_p = xstrdup (INTERP_CONSOLE);
	    break;
	  case 'f':
	    annotation_level = 1;
	    break;
	  case 's':
	    symarg = optarg;
	    break;
	  case 'e':
	    execarg = optarg;
	    break;
	  case 'c':
	    corearg = optarg;
	    break;
	  case 'p':
	    pidarg = optarg;
	    break;
	  case 'x':
	    cmdarg_vec.emplace_back (CMDARG_FILE, optarg);
	    break;
	  case 'X':
	    cmdarg_vec.emplace_back (CMDARG_COMMAND, optarg);
	    break;
	  case OPT_IX:
	    cmdarg_vec.emplace_back (CMDARG_INIT_FILE, optarg);
	    break;
	  case OPT_IEX:
	    cmdarg_vec.emplace_back (CMDARG_INIT_COMMAND, optarg);
	    break;
	  case 'B':
	    batch_flag = batch_silent = 1;
	    gdb_stdout = new null_file ();
	    break;
	  case 'D':
	    if (optarg[0] == '\0')
	      error (_("%s: empty path for `--data-directory'"),
		     gdb_program_name);
	    set_gdb_data_directory (optarg);
	    gdb_datadir_provided = 1;
	    break;
#ifdef GDBTK
	  case 'z':
	    {
	      if (!gdbtk_test (optarg))
		error (_("%s: unable to load tclcommand file \"%s\""),
		       gdb_program_name, optarg);
	      break;
	    }
	  case 'y':
	    /* Backwards compatibility only.  */
	    break;
	  case 'w':
	    {
	      /* Set the external editor commands when gdb is farming out files
		 to be edited by another program.  */
	      external_editor_command = xstrdup (optarg);
	      break;
	    }
#endif /* GDBTK */
	  case 'i':
	    xfree (interpreter_p);
	    interpreter_p = xstrdup (optarg);
	    break;
	  case 'd':
	    dirarg.push_back (optarg);
	    break;
	  case 't':
	    ttyarg = optarg;
	    break;
	  case 'q':
	    quiet = 1;
	    break;
	  case 'b':
	    {
	      int rate;
	      char *p;

	      rate = strtol (optarg, &p, 0);
	      if (rate == 0 && p == optarg)
		warning (_("could not set baud rate to `%s'."),
			 optarg);
	      else
		baud_rate = rate;
	    }
            break;
	  case 'l':
	    {
	      int timeout;
	      char *p;

	      timeout = strtol (optarg, &p, 0);
	      if (timeout == 0 && p == optarg)
		warning (_("could not set timeout limit to `%s'."),
			 optarg);
	      else
		remote_timeout = timeout;
	    }
	    break;

	  case OPT_READNOW:
	    {
	      readnow_symbol_files = 1;
	      validate_readnow_readnever ();
	    }
	    break;

	  case OPT_READNEVER:
	    {
	      readnever_symbol_files = 1;
	      validate_readnow_readnever ();
	    }
	    break;

	  case '?':
	    error (_("Use `%s --help' for a complete list of options."),
		   gdb_program_name);
	  }
      }
    write_files = (write_files_1 != 0);

    if (batch_flag)
      {
	quiet = 1;

	/* Disable all output styling when running in batch mode.  */
	cli_styling = 0;
      }
  }

  save_original_signals_state (quiet);

  /* Try to set up an alternate signal stack for SIGSEGV handlers.  */
  gdb::alternate_signal_stack signal_stack;

  /* Initialize all files.  */
  gdb_init (gdb_program_name);

  /* Now that gdb_init has created the initial inferior, we're in
     position to set args for that inferior.  */
  if (set_args)
    {
      /* The remaining options are the command-line options for the
	 inferior.  The first one is the sym/exec file, and the rest
	 are arguments.  */
      if (optind >= argc)
	error (_("%s: `--args' specified but no program specified"),
	       gdb_program_name);

      symarg = argv[optind];
      execarg = argv[optind];
      ++optind;
      set_inferior_args_vector (argc - optind, &argv[optind]);
    }
  else
    {
      /* OK, that's all the options.  */

      /* The first argument, if specified, is the name of the
	 executable.  */
      if (optind < argc)
	{
	  symarg = argv[optind];
	  execarg = argv[optind];
	  optind++;
	}

      /* If the user hasn't already specified a PID or the name of a
	 core file, then a second optional argument is allowed.  If
	 present, this argument should be interpreted as either a
	 PID or a core file, whichever works.  */
      if (pidarg == NULL && corearg == NULL && optind < argc)
	{
	  pid_or_core_arg = argv[optind];
	  optind++;
	}

      /* Any argument left on the command line is unexpected and
	 will be ignored.  Inform the user.  */
      if (optind < argc)
	fprintf_unfiltered (gdb_stderr,
			    _("Excess command line "
			      "arguments ignored. (%s%s)\n"),
			    argv[optind],
			    (optind == argc - 1) ? "" : " ...");
    }

  /* Lookup gdbinit files.  Note that the gdbinit file name may be
     overridden during file initialization, so get_init_files should be
     called after gdb_init.  */
  std::vector<std::string> system_gdbinit;
  std::string home_gdbinit;
  std::string local_gdbinit;
  get_init_files (&system_gdbinit, &home_gdbinit, &local_gdbinit);

  /* Do these (and anything which might call wrap_here or *_filtered)
     after initialize_all_files() but before the interpreter has been
     installed.  Otherwize the help/version messages will be eaten by
     the interpreter's output handler.  */

  if (print_version)
    {
      print_gdb_version (gdb_stdout, false);
      wrap_here ("");
      printf_filtered ("\n");
      exit (0);
    }

  if (print_help)
    {
      print_gdb_help (gdb_stdout);
      exit (0);
    }

  if (print_configuration)
    {
      print_gdb_configuration (gdb_stdout);
      wrap_here ("");
      printf_filtered ("\n");
      exit (0);
    }

  /* FIXME: cagney/2003-02-03: The big hack (part 1 of 2) that lets
     GDB retain the old MI1 interpreter startup behavior.  Output the
     copyright message before the interpreter is installed.  That way
     it isn't encapsulated in MI output.  */
  if (!quiet && strcmp (interpreter_p, INTERP_MI1) == 0)
    {
      /* Print all the junk at the top, with trailing "..." if we are
         about to read a symbol file (possibly slowly).  */
      print_gdb_version (gdb_stdout, true);
      if (symarg)
	printf_filtered ("..");
      wrap_here ("");
      printf_filtered ("\n");
      gdb_flush (gdb_stdout);	/* Force to screen during slow
				   operations.  */
    }

  /* Install the default UI.  All the interpreters should have had a
     look at things by now.  Initialize the default interpreter.  */
  set_top_level_interpreter (interpreter_p);

  /* FIXME: cagney/2003-02-03: The big hack (part 2 of 2) that lets
     GDB retain the old MI1 interpreter startup behavior.  Output the
     copyright message after the interpreter is installed when it is
     any sane interpreter.  */
  if (!quiet && !current_interp_named_p (INTERP_MI1))
    {
      /* Print all the junk at the top, with trailing "..." if we are
         about to read a symbol file (possibly slowly).  */
      print_gdb_version (gdb_stdout, true);
      if (symarg)
	printf_filtered ("..");
      wrap_here ("");
      printf_filtered ("\n");
      gdb_flush (gdb_stdout);	/* Force to screen during slow
				   operations.  */
    }

  /* Set off error and warning messages with a blank line.  */
  tmp_warn_preprint.reset ();
  warning_pre_print = _("\nwarning: ");

  /* Read and execute the system-wide gdbinit file, if it exists.
     This is done *before* all the command line arguments are
     processed; it sets global parameters, which are independent of
     what file you are debugging or what directory you are in.  */
  if (!system_gdbinit.empty () && !inhibit_gdbinit)
    {
      for (const std::string &file : system_gdbinit)
	ret = catch_command_errors (source_script, file.c_str (), 0);
    }

  /* Read and execute $HOME/.gdbinit file, if it exists.  This is done
     *before* all the command line arguments are processed; it sets
     global parameters, which are independent of what file you are
     debugging or what directory you are in.  */

  if (!home_gdbinit.empty () && !inhibit_gdbinit && !inhibit_home_gdbinit)
    ret = catch_command_errors (source_script, home_gdbinit.c_str (), 0);

  /* Process '-ix' and '-iex' options early.  */
  for (i = 0; i < cmdarg_vec.size (); i++)
    {
      const struct cmdarg &cmdarg_p = cmdarg_vec[i];

      switch (cmdarg_p.type)
	{
	case CMDARG_INIT_FILE:
	  ret = catch_command_errors (source_script, cmdarg_p.string,
				      !batch_flag);
	  break;
	case CMDARG_INIT_COMMAND:
	  ret = catch_command_errors (execute_command, cmdarg_p.string,
				      !batch_flag);
	  break;
	}
    }

  /* Now perform all the actions indicated by the arguments.  */
  if (cdarg != NULL)
    {
      ret = catch_command_errors (cd_command, cdarg, 0);
    }

  for (i = 0; i < dirarg.size (); i++)
    ret = catch_command_errors (directory_switch, dirarg[i], 0);

  /* Skip auto-loading section-specified scripts until we've sourced
     local_gdbinit (which is often used to augment the source search
     path).  */
  save_auto_load = global_auto_load;
  global_auto_load = 0;

  if (execarg != NULL
      && symarg != NULL
      && strcmp (execarg, symarg) == 0)
    {
      /* The exec file and the symbol-file are the same.  If we can't
         open it, better only print one error message.
         catch_command_errors returns non-zero on success!  */
      ret = catch_command_errors (exec_file_attach, execarg,
				  !batch_flag);
      if (ret != 0)
	ret = catch_command_errors (symbol_file_add_main_adapter,
				    symarg, !batch_flag);
    }
  else
    {
      if (execarg != NULL)
	ret = catch_command_errors (exec_file_attach, execarg,
				    !batch_flag);
      if (symarg != NULL)
	ret = catch_command_errors (symbol_file_add_main_adapter,
				    symarg, !batch_flag);
    }

  if (corearg && pidarg)
    error (_("Can't attach to process and specify "
	     "a core file at the same time."));

  if (corearg != NULL)
    {
      ret = catch_command_errors (core_file_command, corearg,
				  !batch_flag);
    }
  else if (pidarg != NULL)
    {
      ret = catch_command_errors (attach_command, pidarg, !batch_flag);
    }
  else if (pid_or_core_arg)
    {
      /* The user specified 'gdb program pid' or gdb program core'.
	 If pid_or_core_arg's first character is a digit, try attach
	 first and then corefile.  Otherwise try just corefile.  */

      if (isdigit (pid_or_core_arg[0]))
	{
	  ret = catch_command_errors (attach_command, pid_or_core_arg,
				      !batch_flag);
	  if (ret == 0)
	    ret = catch_command_errors (core_file_command,
					pid_or_core_arg,
					!batch_flag);
	}
      else
	{
	  /* Can't be a pid, better be a corefile.  */
	  ret = catch_command_errors (core_file_command,
				      pid_or_core_arg,
				      !batch_flag);
	}
    }

  if (ttyarg != NULL)
    current_inferior ()->set_tty (ttyarg);

  /* Error messages should no longer be distinguished with extra output.  */
  warning_pre_print = _("warning: ");

  /* Read the .gdbinit file in the current directory, *if* it isn't
     the same as the $HOME/.gdbinit file (it should exist, also).  */
  if (!local_gdbinit.empty ())
    {
      auto_load_local_gdbinit_pathname
	= gdb_realpath (local_gdbinit.c_str ()).release ();

      if (!inhibit_gdbinit && auto_load_local_gdbinit
	  && file_is_auto_load_safe (local_gdbinit.c_str (),
				     _("auto-load: Loading .gdbinit "
				       "file \"%s\".\n"),
				     local_gdbinit.c_str ()))
	{
	  auto_load_local_gdbinit_loaded = 1;

	  ret = catch_command_errors (source_script, local_gdbinit.c_str (), 0);
	}
    }

  /* Now that all .gdbinit's have been read and all -d options have been
     processed, we can read any scripts mentioned in SYMARG.
     We wait until now because it is common to add to the source search
     path in local_gdbinit.  */
  global_auto_load = save_auto_load;
  for (objfile *objfile : current_program_space->objfiles ())
    load_auto_scripts_for_objfile (objfile);

  /* Process '-x' and '-ex' options.  */
  for (i = 0; i < cmdarg_vec.size (); i++)
    {
      const struct cmdarg &cmdarg_p = cmdarg_vec[i];

      switch (cmdarg_p.type)
	{
	case CMDARG_FILE:
	  ret = catch_command_errors (source_script, cmdarg_p.string,
				      !batch_flag);
	  break;
	case CMDARG_COMMAND:
	  ret = catch_command_errors (execute_command, cmdarg_p.string,
				      !batch_flag);
	  break;
	}
    }

  /* Read in the old history after all the command files have been
     read.  */
  init_history ();

  if (batch_flag)
    {
      int error_status = EXIT_FAILURE;
      int *exit_arg = ret == 0 ? &error_status : NULL;

      /* We have hit the end of the batch file.  */
      quit_force (exit_arg, 0);
    }
}

static void
captured_main (void *data)
{
  struct captured_main_args *context = (struct captured_main_args *) data;

  captured_main_1 (context);

  /* NOTE: cagney/1999-11-07: There is probably no reason for not
     moving this loop and the code found in captured_command_loop()
     into the command_loop() proper.  The main thing holding back that
     change - SET_TOP_LEVEL() - has been eliminated.  */
  while (1)
    {
      try
	{
	  captured_command_loop ();
	}
      catch (const gdb_exception &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
    }
  /* No exit -- exit is through quit_command.  */
}

int
gdb_main (struct captured_main_args *args)
{
  try
    {
      captured_main (args);
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);
    }

  /* The only way to end up here is by an error (normal exit is
     handled by quit_force()), hence always return an error status.  */
  return 1;
}


/* Don't use *_filtered for printing help.  We don't want to prompt
   for continue no matter how small the screen or how much we're going
   to print.  */

static void
print_gdb_help (struct ui_file *stream)
{
  std::vector<std::string> system_gdbinit;
  std::string home_gdbinit;
  std::string local_gdbinit;

  get_init_files (&system_gdbinit, &home_gdbinit, &local_gdbinit);

  /* Note: The options in the list below are only approximately sorted
     in the alphabetical order, so as to group closely related options
     together.  */
  fputs_unfiltered (_("\
This is the GNU debugger.  Usage:\n\n\
    gdb [options] [executable-file [core-file or process-id]]\n\
    gdb [options] --args executable-file [inferior-arguments ...]\n\n\
"), stream);
  fputs_unfiltered (_("\
Selection of debuggee and its files:\n\n\
  --args             Arguments after executable-file are passed to inferior\n\
  --core=COREFILE    Analyze the core dump COREFILE.\n\
  --exec=EXECFILE    Use EXECFILE as the executable.\n\
  --pid=PID          Attach to running process PID.\n\
  --directory=DIR    Search for source files in DIR.\n\
  --se=FILE          Use FILE as symbol file and executable file.\n\
  --symbols=SYMFILE  Read symbols from SYMFILE.\n\
  --readnow          Fully read symbol files on first access.\n\
  --readnever        Do not read symbol files.\n\
  --write            Set writing into executable and core files.\n\n\
"), stream);
  fputs_unfiltered (_("\
Initial commands and command files:\n\n\
  --command=FILE, -x Execute GDB commands from FILE.\n\
  --init-command=FILE, -ix\n\
                     Like -x but execute commands before loading inferior.\n\
  --eval-command=COMMAND, -ex\n\
                     Execute a single GDB command.\n\
                     May be used multiple times and in conjunction\n\
                     with --command.\n\
  --init-eval-command=COMMAND, -iex\n\
                     Like -ex but before loading inferior.\n\
  --nh               Do not read ~/.gdbinit.\n\
  --nx               Do not read any .gdbinit files in any directory.\n\n\
"), stream);
  fputs_unfiltered (_("\
Output and user interface control:\n\n\
  --fullname         Output information used by emacs-GDB interface.\n\
  --interpreter=INTERP\n\
                     Select a specific interpreter / user interface\n\
  --tty=TTY          Use TTY for input/output by the program being debugged.\n\
  -w                 Use the GUI interface.\n\
  --nw               Do not use the GUI interface.\n\
"), stream);
#if defined(TUI)
  fputs_unfiltered (_("\
  --tui              Use a terminal user interface.\n\
"), stream);
#endif
  fputs_unfiltered (_("\
  --dbx              DBX compatibility mode.\n\
  -q, --quiet, --silent\n\
                     Do not print version number on startup.\n\n\
"), stream);
  fputs_unfiltered (_("\
Operating modes:\n\n\
  --batch            Exit after processing options.\n\
  --batch-silent     Like --batch, but suppress all gdb stdout output.\n\
  --return-child-result\n\
                     GDB exit code will be the child's exit code.\n\
  --configuration    Print details about GDB configuration and then exit.\n\
  --help             Print this message and then exit.\n\
  --version          Print version information and then exit.\n\n\
Remote debugging options:\n\n\
  -b BAUDRATE        Set serial port baud rate used for remote debugging.\n\
  -l TIMEOUT         Set timeout in seconds for remote debugging.\n\n\
Other options:\n\n\
  --cd=DIR           Change current directory to DIR.\n\
  --data-directory=DIR, -D\n\
                     Set GDB's data-directory to DIR.\n\
"), stream);
  fputs_unfiltered (_("\n\
At startup, GDB reads the following init files and executes their commands:\n\
"), stream);
  if (!system_gdbinit.empty ())
    {
      std::string output;
      for (size_t idx = 0; idx < system_gdbinit.size (); ++idx)
        {
	  output += system_gdbinit[idx];
	  if (idx < system_gdbinit.size () - 1)
	    output += ", ";
	}
      fprintf_unfiltered (stream, _("\
   * system-wide init files: %s\n\
"), output.c_str ());
    }
  if (!home_gdbinit.empty ())
    fprintf_unfiltered (stream, _("\
   * user-specific init file: %s\n\
"), home_gdbinit.c_str ());
  if (!local_gdbinit.empty ())
    fprintf_unfiltered (stream, _("\
   * local init file (see also 'set auto-load local-gdbinit'): ./%s\n\
"), local_gdbinit.c_str ());
  fputs_unfiltered (_("\n\
For more information, type \"help\" from within GDB, or consult the\n\
GDB manual (available as on-line info or a printed manual).\n\
"), stream);
  if (REPORT_BUGS_TO[0] && stream == gdb_stdout)
    fprintf_unfiltered (stream, _("\n\
Report bugs to %s.\n\
"), REPORT_BUGS_TO);
  if (stream == gdb_stdout)
    fprintf_unfiltered (stream, _("\n\
You can ask GDB-related questions on the GDB users mailing list\n\
(gdb@sourceware.org) or on GDB's IRC channel (#gdb on Freenode).\n"));
}
