/* General Compile and inject code

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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
#include "ui-out.h"
#include "command.h"
#include "cli/cli-script.h"
#include "cli/cli-utils.h"
#include "completer.h"
#include "gdbcmd.h"
#include "compile.h"
#include "compile-internal.h"
#include "compile-object-load.h"
#include "compile-object-run.h"
#include "language.h"
#include "frame.h"
#include "source.h"
#include "block.h"
#include "arch-utils.h"
#include "filestuff.h"
#include "target.h"
#include "osabi.h"
#include "gdb_wait.h"
#include "valprint.h"



/* Initial filename for temporary files.  */

#define TMP_PREFIX "/tmp/gdbobj-"

/* Hold "compile" commands.  */

static struct cmd_list_element *compile_command_list;

/* Debug flag for "compile" commands.  */

int compile_debug;

/* Implement "show debug compile".  */

static void
show_compile_debug (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Compile debugging is %s.\n"), value);
}



/* Check *ARG for a "-raw" or "-r" argument.  Return 0 if not seen.
   Return 1 if seen and update *ARG.  */

static int
check_raw_argument (char **arg)
{
  *arg = skip_spaces (*arg);

  if (arg != NULL
      && (check_for_argument (arg, "-raw", sizeof ("-raw") - 1)
	  || check_for_argument (arg, "-r", sizeof ("-r") - 1)))
      return 1;
  return 0;
}

/* Handle the input from the 'compile file' command.  The "compile
   file" command is used to evaluate an expression contained in a file
   that may contain calls to the GCC compiler.  */

static void
compile_file_command (char *arg, int from_tty)
{
  enum compile_i_scope_types scope = COMPILE_I_SIMPLE_SCOPE;
  char *buffer;
  struct cleanup *cleanup;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  /* Check the user did not just <enter> after command.  */
  if (arg == NULL)
    error (_("You must provide a filename for this command."));

  /* Check if a raw (-r|-raw) argument is provided.  */
  if (arg != NULL && check_raw_argument (&arg))
    {
      scope = COMPILE_I_RAW_SCOPE;
      arg = skip_spaces (arg);
    }

  /* After processing arguments, check there is a filename at the end
     of the command.  */
  if (arg[0] == '\0')
    error (_("You must provide a filename with the raw option set."));

  if (arg[0] == '-')
    error (_("Unknown argument specified."));

  arg = skip_spaces (arg);
  arg = gdb_abspath (arg);
  cleanup = make_cleanup (xfree, arg);
  buffer = xstrprintf ("#include \"%s\"\n", arg);
  make_cleanup (xfree, buffer);
  eval_compile_command (NULL, buffer, scope, NULL);
  do_cleanups (cleanup);
}

/* Handle the input from the 'compile code' command.  The
   "compile code" command is used to evaluate an expression that may
   contain calls to the GCC compiler.  The language expected in this
   compile command is the language currently set in GDB.  */

static void
compile_code_command (char *arg, int from_tty)
{
  enum compile_i_scope_types scope = COMPILE_I_SIMPLE_SCOPE;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  if (arg != NULL && check_raw_argument (&arg))
    {
      scope = COMPILE_I_RAW_SCOPE;
      arg = skip_spaces (arg);
    }

  arg = skip_spaces (arg);

  if (arg != NULL && !check_for_argument (&arg, "--", sizeof ("--") - 1))
    {
      if (arg[0] == '-')
	error (_("Unknown argument specified."));
    }

  if (arg && *arg)
    eval_compile_command (NULL, arg, scope, NULL);
  else
    {
      command_line_up l = get_command_line (compile_control, "");

      l->control_u.compile.scope = scope;
      execute_control_command_untraced (l.get ());
    }
}

/* Callback for compile_print_command.  */

void
compile_print_value (struct value *val, void *data_voidp)
{
  const struct format_data *fmtp = (const struct format_data *) data_voidp;

  print_value (val, fmtp);
}

/* Handle the input from the 'compile print' command.  The "compile
   print" command is used to evaluate and print an expression that may
   contain calls to the GCC compiler.  The language expected in this
   compile command is the language currently set in GDB.  */

static void
compile_print_command (char *arg_param, int from_tty)
{
  const char *arg = arg_param;
  enum compile_i_scope_types scope = COMPILE_I_PRINT_ADDRESS_SCOPE;
  struct format_data fmt;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  /* Passing &FMT as SCOPE_DATA is safe as do_module_cleanup will not
     touch the stale pointer if compile_object_run has already quit.  */
  print_command_parse_format (&arg, "compile print", &fmt);

  if (arg && *arg)
    eval_compile_command (NULL, arg, scope, &fmt);
  else
    {
      command_line_up l = get_command_line (compile_control, "");

      l->control_u.compile.scope = scope;
      l->control_u.compile.scope_data = &fmt;
      execute_control_command_untraced (l.get ());
    }
}

/* A cleanup function to remove a directory and all its contents.  */

static void
do_rmdir (void *arg)
{
  const char *dir = (const char *) arg;
  char *zap;
  int wstat;

  gdb_assert (startswith (dir, TMP_PREFIX));
  zap = concat ("rm -rf ", dir, (char *) NULL);
  wstat = system (zap);
  if (wstat == -1 || !WIFEXITED (wstat) || WEXITSTATUS (wstat) != 0)
    warning (_("Could not remove temporary directory %s"), dir);
  XDELETEVEC (zap);
}

/* Return the name of the temporary directory to use for .o files, and
   arrange for the directory to be removed at shutdown.  */

static const char *
get_compile_file_tempdir (void)
{
  static char *tempdir_name;

#define TEMPLATE TMP_PREFIX "XXXXXX"
  char tname[sizeof (TEMPLATE)];

  if (tempdir_name != NULL)
    return tempdir_name;

  strcpy (tname, TEMPLATE);
#undef TEMPLATE
#ifdef HAVE_MKDTEMP
  tempdir_name = mkdtemp (tname);
#else
  error (_("Command not supported on this host."));
#endif
  if (tempdir_name == NULL)
    perror_with_name (_("Could not make temporary directory"));

  tempdir_name = xstrdup (tempdir_name);
  make_final_cleanup (do_rmdir, tempdir_name);
  return tempdir_name;
}

/* Compute the names of source and object files to use.  */

static compile_file_names
get_new_file_names ()
{
  static int seq;
  const char *dir = get_compile_file_tempdir ();

  ++seq;

  return compile_file_names (string_printf ("%s%sout%d.c",
					    dir, SLASH_STRING, seq),
			     string_printf ("%s%sout%d.o",
					    dir, SLASH_STRING, seq));
}

/* Get the block and PC at which to evaluate an expression.  */

static const struct block *
get_expr_block_and_pc (CORE_ADDR *pc)
{
  const struct block *block = get_selected_block (pc);

  if (block == NULL)
    {
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();

      if (cursal.symtab)
	block = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (cursal.symtab),
				   STATIC_BLOCK);
      if (block != NULL)
	*pc = BLOCK_START (block);
    }
  else
    *pc = BLOCK_START (block);

  return block;
}

/* Call gdb_buildargv, set its result for S into *ARGVP but calculate also the
   number of parsed arguments into *ARGCP.  If gdb_buildargv has returned NULL
   then *ARGCP is set to zero.  */

static void
build_argc_argv (const char *s, int *argcp, char ***argvp)
{
  *argvp = gdb_buildargv (s);
  *argcp = countargv (*argvp);
}

/* String for 'set compile-args' and 'show compile-args'.  */
static char *compile_args;

/* Parsed form of COMPILE_ARGS.  COMPILE_ARGS_ARGV is NULL terminated.  */
static int compile_args_argc;
static char **compile_args_argv;

/* Implement 'set compile-args'.  */

static void
set_compile_args (char *args, int from_tty, struct cmd_list_element *c)
{
  freeargv (compile_args_argv);
  build_argc_argv (compile_args, &compile_args_argc, &compile_args_argv);
}

/* Implement 'show compile-args'.  */

static void
show_compile_args (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Compile command command-line arguments "
			    "are \"%s\".\n"),
		    value);
}

/* Append ARGC and ARGV (as parsed by build_argc_argv) to *ARGCP and *ARGVP.
   ARGCP+ARGVP can be zero+NULL and also ARGC+ARGV can be zero+NULL.  */

static void
append_args (int *argcp, char ***argvp, int argc, char **argv)
{
  int argi;

  *argvp = XRESIZEVEC (char *, *argvp, (*argcp + argc + 1));

  for (argi = 0; argi < argc; argi++)
    (*argvp)[(*argcp)++] = xstrdup (argv[argi]);
  (*argvp)[(*argcp)] = NULL;
}

/* Return DW_AT_producer parsed for get_selected_frame () (if any).
   Return NULL otherwise.

   GCC already filters its command-line arguments only for the suitable ones to
   put into DW_AT_producer - see GCC function gen_producer_string.  */

static const char *
get_selected_pc_producer_options (void)
{
  CORE_ADDR pc = get_frame_pc (get_selected_frame (NULL));
  struct compunit_symtab *symtab = find_pc_compunit_symtab (pc);
  const char *cs;

  if (symtab == NULL || symtab->producer == NULL
      || !startswith (symtab->producer, "GNU "))
    return NULL;

  cs = symtab->producer;
  while (*cs != 0 && *cs != '-')
    cs = skip_spaces_const (skip_to_space_const (cs));
  if (*cs != '-')
    return NULL;
  return cs;
}

/* Filter out unwanted options from *ARGCP and ARGV.  */

static void
filter_args (int *argcp, char **argv)
{
  char **destv;

  for (destv = argv; *argv != NULL; argv++)
    {
      /* -fpreprocessed may get in commonly from ccache.  */
      if (strcmp (*argv, "-fpreprocessed") == 0)
	{
	  xfree (*argv);
	  (*argcp)--;
	  continue;
	}
      *destv++ = *argv;
    }
  *destv = NULL;
}

/* Produce final vector of GCC compilation options.  First element is target
   size ("-m64", "-m32" etc.), optionally followed by DW_AT_producer options
   and then compile-args string GDB variable.  */

static void
get_args (const struct compile_instance *compiler, struct gdbarch *gdbarch,
	  int *argcp, char ***argvp)
{
  const char *cs_producer_options;
  int argc_compiler;
  char **argv_compiler;

  build_argc_argv (gdbarch_gcc_target_options (gdbarch),
		   argcp, argvp);

  cs_producer_options = get_selected_pc_producer_options ();
  if (cs_producer_options != NULL)
    {
      int argc_producer;
      char **argv_producer;

      build_argc_argv (cs_producer_options, &argc_producer, &argv_producer);
      filter_args (&argc_producer, argv_producer);
      append_args (argcp, argvp, argc_producer, argv_producer);
      freeargv (argv_producer);
    }

  build_argc_argv (compiler->gcc_target_options,
		   &argc_compiler, &argv_compiler);
  append_args (argcp, argvp, argc_compiler, argv_compiler);
  freeargv (argv_compiler);

  append_args (argcp, argvp, compile_args_argc, compile_args_argv);
}

/* A cleanup function to destroy a gdb_gcc_instance.  */

static void
cleanup_compile_instance (void *arg)
{
  struct compile_instance *inst = (struct compile_instance *) arg;

  inst->destroy (inst);
}

/* A cleanup function to unlink a file.  */

static void
cleanup_unlink_file (void *arg)
{
  const char *filename = (const char *) arg;

  unlink (filename);
}

/* A helper function suitable for use as the "print_callback" in the
   compiler object.  */

static void
print_callback (void *ignore, const char *message)
{
  fputs_filtered (message, gdb_stderr);
}

/* Process the compilation request.  On success it returns the object
   and source file names.  On an error condition, error () is
   called.  */

static compile_file_names
compile_to_object (struct command_line *cmd, const char *cmd_string,
		   enum compile_i_scope_types scope)
{
  struct compile_instance *compiler;
  struct cleanup *cleanup, *inner_cleanup;
  const struct block *expr_block;
  CORE_ADDR trash_pc, expr_pc;
  int argc;
  char **argv;
  int ok;
  FILE *src;
  struct gdbarch *gdbarch = get_current_arch ();
  const char *os_rx;
  const char *arch_rx;
  char *triplet_rx;
  char *error_message;

  if (!target_has_execution)
    error (_("The program must be running for the compile command to "\
	     "work."));

  expr_block = get_expr_block_and_pc (&trash_pc);
  expr_pc = get_frame_address_in_block (get_selected_frame (NULL));

  /* Set up instance and context for the compiler.  */
  if (current_language->la_get_compile_instance == NULL)
    error (_("No compiler support for language %s."),
	   current_language->la_name);
  compiler = current_language->la_get_compile_instance ();
  cleanup = make_cleanup (cleanup_compile_instance, compiler);

  compiler->fe->ops->set_print_callback (compiler->fe, print_callback, NULL);

  compiler->scope = scope;
  compiler->block = expr_block;

  /* From the provided expression, build a scope to pass to the
     compiler.  */

  string_file input_buf;
  const char *input;

  if (cmd != NULL)
    {
      struct command_line *iter;

      for (iter = cmd->body_list[0]; iter; iter = iter->next)
	{
	  input_buf.puts (iter->line);
	  input_buf.puts ("\n");
	}

      input = input_buf.c_str ();
    }
  else if (cmd_string != NULL)
    input = cmd_string;
  else
    error (_("Neither a simple expression, or a multi-line specified."));

  std::string code
    = current_language->la_compute_program (compiler, input, gdbarch,
					    expr_block, expr_pc);
  if (compile_debug)
    fprintf_unfiltered (gdb_stdlog, "debug output:\n\n%s", code.c_str ());

  os_rx = osabi_triplet_regexp (gdbarch_osabi (gdbarch));
  arch_rx = gdbarch_gnu_triplet_regexp (gdbarch);

  /* Allow triplets with or without vendor set.  */
  triplet_rx = concat (arch_rx, "(-[^-]*)?-", os_rx, (char *) NULL);
  make_cleanup (xfree, triplet_rx);

  /* Set compiler command-line arguments.  */
  get_args (compiler, gdbarch, &argc, &argv);
  make_cleanup_freeargv (argv);

  error_message = compiler->fe->ops->set_arguments (compiler->fe, triplet_rx,
						    argc, argv);
  if (error_message != NULL)
    {
      make_cleanup (xfree, error_message);
      error ("%s", error_message);
    }

  if (compile_debug)
    {
      int argi;

      fprintf_unfiltered (gdb_stdlog, "Passing %d compiler options:\n", argc);
      for (argi = 0; argi < argc; argi++)
	fprintf_unfiltered (gdb_stdlog, "Compiler option %d: <%s>\n",
			    argi, argv[argi]);
    }

  compile_file_names fnames = get_new_file_names ();

  src = gdb_fopen_cloexec (fnames.source_file (), "w");
  if (src == NULL)
    perror_with_name (_("Could not open source file for writing"));
  inner_cleanup = make_cleanup (cleanup_unlink_file,
				(void *) fnames.source_file ());
  if (fputs (code.c_str (), src) == EOF)
    perror_with_name (_("Could not write to source file"));
  fclose (src);

  if (compile_debug)
    fprintf_unfiltered (gdb_stdlog, "source file produced: %s\n\n",
			fnames.source_file ());

  /* Call the compiler and start the compilation process.  */
  compiler->fe->ops->set_source_file (compiler->fe, fnames.source_file ());

  if (!compiler->fe->ops->compile (compiler->fe, fnames.object_file (),
				   compile_debug))
    error (_("Compilation failed."));

  if (compile_debug)
    fprintf_unfiltered (gdb_stdlog, "object file produced: %s\n\n",
			fnames.object_file ());

  discard_cleanups (inner_cleanup);
  do_cleanups (cleanup);

  return fnames;
}

/* The "compile" prefix command.  */

static void
compile_command (char *args, int from_tty)
{
  /* If a sub-command is not specified to the compile prefix command,
     assume it is a direct code compilation.  */
  compile_code_command (args, from_tty);
}

/* See compile.h.  */

void
eval_compile_command (struct command_line *cmd, const char *cmd_string,
		      enum compile_i_scope_types scope, void *scope_data)
{
  struct cleanup *cleanup_unlink;
  struct compile_module *compile_module;

  compile_file_names fnames = compile_to_object (cmd, cmd_string, scope);

  cleanup_unlink = make_cleanup (cleanup_unlink_file,
				 (void *) fnames.object_file ());
  make_cleanup (cleanup_unlink_file, (void *) fnames.source_file ());
  compile_module = compile_object_load (fnames, scope, scope_data);
  if (compile_module == NULL)
    {
      gdb_assert (scope == COMPILE_I_PRINT_ADDRESS_SCOPE);
      eval_compile_command (cmd, cmd_string,
			    COMPILE_I_PRINT_VALUE_SCOPE, scope_data);
      return;
    }
  discard_cleanups (cleanup_unlink);
  compile_object_run (compile_module);
}

/* See compile/compile-internal.h.  */

char *
compile_register_name_mangled (struct gdbarch *gdbarch, int regnum)
{
  const char *regname = gdbarch_register_name (gdbarch, regnum);

  return xstrprintf ("__%s", regname);
}

/* See compile/compile-internal.h.  */

int
compile_register_name_demangle (struct gdbarch *gdbarch,
				 const char *regname)
{
  int regnum;

  if (regname[0] != '_' || regname[1] != '_')
    error (_("Invalid register name \"%s\"."), regname);
  regname += 2;

  for (regnum = 0; regnum < gdbarch_num_regs (gdbarch); regnum++)
    if (strcmp (regname, gdbarch_register_name (gdbarch, regnum)) == 0)
      return regnum;

  error (_("Cannot find gdbarch register \"%s\"."), regname);
}

extern initialize_file_ftype _initialize_compile;

void
_initialize_compile (void)
{
  struct cmd_list_element *c = NULL;

  add_prefix_cmd ("compile", class_obscure, compile_command,
		  _("\
Command to compile source code and inject it into the inferior."),
		  &compile_command_list, "compile ", 1, &cmdlist);
  add_com_alias ("expression", "compile", class_obscure, 0);

  add_cmd ("code", class_obscure, compile_code_command,
	   _("\
Compile, inject, and execute code.\n\
\n\
Usage: compile code [-r|-raw] [--] [CODE]\n\
-r|-raw: Suppress automatic 'void _gdb_expr () { CODE }' wrapping.\n\
--: Do not parse any options beyond this delimiter.  All text to the\n\
    right will be treated as source code.\n\
\n\
The source code may be specified as a simple one line expression, e.g.:\n\
\n\
    compile code printf(\"Hello world\\n\");\n\
\n\
Alternatively, you can type a multiline expression by invoking\n\
this command with no argument.  GDB will then prompt for the\n\
expression interactively; type a line containing \"end\" to\n\
indicate the end of the expression."),
	   &compile_command_list);

  c = add_cmd ("file", class_obscure, compile_file_command,
	       _("\
Evaluate a file containing source code.\n\
\n\
Usage: compile file [-r|-raw] [filename]\n\
-r|-raw: Suppress automatic 'void _gdb_expr () { CODE }' wrapping."),
	       &compile_command_list);
  set_cmd_completer (c, filename_completer);

  add_cmd ("print", class_obscure, compile_print_command,
	   _("\
Evaluate EXPR by using the compiler and print result.\n\
\n\
Usage: compile print[/FMT] [EXPR]\n\
\n\
The expression may be specified on the same line as the command, e.g.:\n\
\n\
    compile print i\n\
\n\
Alternatively, you can type a multiline expression by invoking\n\
this command with no argument.  GDB will then prompt for the\n\
expression interactively; type a line containing \"end\" to\n\
indicate the end of the expression.\n\
\n\
EXPR may be preceded with /FMT, where FMT is a format letter\n\
but no count or size letter (see \"x\" command)."),
	   &compile_command_list);

  add_setshow_boolean_cmd ("compile", class_maintenance, &compile_debug, _("\
Set compile command debugging."), _("\
Show compile command debugging."), _("\
When on, compile command debugging is enabled."),
			   NULL, show_compile_debug,
			   &setdebuglist, &showdebuglist);

  add_setshow_string_cmd ("compile-args", class_support,
			  &compile_args,
			  _("Set compile command GCC command-line arguments"),
			  _("Show compile command GCC command-line arguments"),
			  _("\
Use options like -I (include file directory) or ABI settings.\n\
String quoting is parsed like in shell, for example:\n\
  -mno-align-double \"-I/dir with a space/include\""),
			  set_compile_args, show_compile_args, &setlist, &showlist);

  /* Override flags possibly coming from DW_AT_producer.  */
  compile_args = xstrdup ("-O0 -gdwarf-4"
  /* We use -fPIE Otherwise GDB would need to reserve space large enough for
     any object file in the inferior in advance to get the final address when
     to link the object file to and additionally the default system linker
     script would need to be modified so that one can specify there the
     absolute target address.
     -fPIC is not used at is would require from GDB to generate .got.  */
			 " -fPIE"
  /* We want warnings, except for some commonly happening for GDB commands.  */
			 " -Wall "
			 " -Wno-implicit-function-declaration"
			 " -Wno-unused-but-set-variable"
			 " -Wno-unused-variable"
  /* Override CU's possible -fstack-protector-strong.  */
			 " -fno-stack-protector"
  );
  set_compile_args (compile_args, 0, NULL);
}
