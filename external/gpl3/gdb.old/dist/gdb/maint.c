/* Support for GDB maintenance commands.

   Copyright (C) 1992-2016 Free Software Foundation, Inc.

   Written by Fred Fish at Cygnus Support.

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
#include <ctype.h>
#include <signal.h>
#include "gdb_sys_time.h"
#include <time.h>
#include "command.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "block.h"
#include "gdbtypes.h"
#include "demangle.h"
#include "gdbcore.h"
#include "expression.h"		/* For language.h */
#include "language.h"
#include "symfile.h"
#include "objfiles.h"
#include "value.h"
#include "top.h"
#include "timeval-utils.h"
#include "maint.h"
#include "selftest.h"

#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "cli/cli-setshow.h"

extern void _initialize_maint_cmds (void);

static void maintenance_command (char *, int);

static void maintenance_internal_error (char *args, int from_tty);

static void maintenance_demangle (char *, int);

static void maintenance_time_display (char *, int);

static void maintenance_space_display (char *, int);

static void maintenance_info_command (char *, int);

static void maintenance_info_sections (char *, int);

static void maintenance_print_command (char *, int);

static void maintenance_do_deprecate (char *, int);

/* Set this to the maximum number of seconds to wait instead of waiting forever
   in target_wait().  If this timer times out, then it generates an error and
   the command is aborted.  This replaces most of the need for timeouts in the
   GDB test suite, and makes it possible to distinguish between a hung target
   and one with slow communications.  */

int watchdog = 0;
static void
show_watchdog (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Watchdog timer is %s.\n"), value);
}

/* Access the maintenance subcommands.  */

static void
maintenance_command (char *args, int from_tty)
{
  printf_unfiltered (_("\"maintenance\" must be followed by "
		       "the name of a maintenance command.\n"));
  help_list (maintenancelist, "maintenance ", all_commands, gdb_stdout);
}

#ifndef _WIN32
static void
maintenance_dump_me (char *args, int from_tty)
{
  if (query (_("Should GDB dump core? ")))
    {
#ifdef __DJGPP__
      /* SIGQUIT by default is ignored, so use SIGABRT instead.  */
      signal (SIGABRT, SIG_DFL);
      kill (getpid (), SIGABRT);
#else
      signal (SIGQUIT, SIG_DFL);
      kill (getpid (), SIGQUIT);
#endif
    }
}
#endif

/* Stimulate the internal error mechanism that GDB uses when an
   internal problem is detected.  Allows testing of the mechanism.
   Also useful when the user wants to drop a core file but not exit
   GDB.  */

static void
maintenance_internal_error (char *args, int from_tty)
{
  internal_error (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   internal problem is detected.  Allows testing of the mechanism.
   Also useful when the user wants to drop a core file but not exit
   GDB.  */

static void
maintenance_internal_warning (char *args, int from_tty)
{
  internal_warning (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   demangler problem is detected.  Allows testing of the mechanism.  */

static void
maintenance_demangler_warning (char *args, int from_tty)
{
  demangler_warning (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Old command to demangle a string.  The command has been moved to "demangle".
   It is kept for now because otherwise "mt demangle" gets interpreted as
   "mt demangler-warning" which artificially creates an internal gdb error.  */

static void
maintenance_demangle (char *args, int from_tty)
{
  printf_filtered (_("This command has been moved to \"demangle\".\n"));
}

static void
maintenance_time_display (char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    printf_unfiltered (_("\"maintenance time\" takes a numeric argument.\n"));
  else
    set_per_command_time (strtol (args, NULL, 10));
}

static void
maintenance_space_display (char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    printf_unfiltered ("\"maintenance space\" takes a numeric argument.\n");
  else
    set_per_command_space (strtol (args, NULL, 10));
}

/* The "maintenance info" command is defined as a prefix, with
   allow_unknown 0.  Therefore, its own definition is called only for
   "maintenance info" with no args.  */

static void
maintenance_info_command (char *arg, int from_tty)
{
  printf_unfiltered (_("\"maintenance info\" must be followed "
		       "by the name of an info command.\n"));
  help_list (maintenanceinfolist, "maintenance info ", all_commands,
	     gdb_stdout);
}

/* Mini tokenizing lexer for 'maint info sections' command.  */

static int
match_substring (const char *string, const char *substr)
{
  int substr_len = strlen(substr);
  const char *tok;

  while ((tok = strstr (string, substr)) != NULL)
    {
      /* Got a partial match.  Is it a whole word?  */
      if (tok == string
	  || tok[-1] == ' '
	  || tok[-1] == '\t')
      {
	/* Token is delimited at the front...  */
	if (tok[substr_len] == ' '
	    || tok[substr_len] == '\t'
	    || tok[substr_len] == '\0')
	{
	  /* Token is delimited at the rear.  Got a whole-word match.  */
	  return 1;
	}
      }
      /* Token didn't match as a whole word.  Advance and try again.  */
      string = tok + 1;
    }
  return 0;
}

static int 
match_bfd_flags (const char *string, flagword flags)
{
  if (flags & SEC_ALLOC)
    if (match_substring (string, "ALLOC"))
      return 1;
  if (flags & SEC_LOAD)
    if (match_substring (string, "LOAD"))
      return 1;
  if (flags & SEC_RELOC)
    if (match_substring (string, "RELOC"))
      return 1;
  if (flags & SEC_READONLY)
    if (match_substring (string, "READONLY"))
      return 1;
  if (flags & SEC_CODE)
    if (match_substring (string, "CODE"))
      return 1;
  if (flags & SEC_DATA)
    if (match_substring (string, "DATA"))
      return 1;
  if (flags & SEC_ROM)
    if (match_substring (string, "ROM"))
      return 1;
  if (flags & SEC_CONSTRUCTOR)
    if (match_substring (string, "CONSTRUCTOR"))
      return 1;
  if (flags & SEC_HAS_CONTENTS)
    if (match_substring (string, "HAS_CONTENTS"))
      return 1;
  if (flags & SEC_NEVER_LOAD)
    if (match_substring (string, "NEVER_LOAD"))
      return 1;
  if (flags & SEC_COFF_SHARED_LIBRARY)
    if (match_substring (string, "COFF_SHARED_LIBRARY"))
      return 1;
  if (flags & SEC_IS_COMMON)
    if (match_substring (string, "IS_COMMON"))
      return 1;

  return 0;
}

static void
print_bfd_flags (flagword flags)
{
  if (flags & SEC_ALLOC)
    printf_filtered (" ALLOC");
  if (flags & SEC_LOAD)
    printf_filtered (" LOAD");
  if (flags & SEC_RELOC)
    printf_filtered (" RELOC");
  if (flags & SEC_READONLY)
    printf_filtered (" READONLY");
  if (flags & SEC_CODE)
    printf_filtered (" CODE");
  if (flags & SEC_DATA)
    printf_filtered (" DATA");
  if (flags & SEC_ROM)
    printf_filtered (" ROM");
  if (flags & SEC_CONSTRUCTOR)
    printf_filtered (" CONSTRUCTOR");
  if (flags & SEC_HAS_CONTENTS)
    printf_filtered (" HAS_CONTENTS");
  if (flags & SEC_NEVER_LOAD)
    printf_filtered (" NEVER_LOAD");
  if (flags & SEC_COFF_SHARED_LIBRARY)
    printf_filtered (" COFF_SHARED_LIBRARY");
  if (flags & SEC_IS_COMMON)
    printf_filtered (" IS_COMMON");
}

static void
maint_print_section_info (const char *name, flagword flags, 
			  CORE_ADDR addr, CORE_ADDR endaddr, 
			  unsigned long filepos, int addr_size)
{
  printf_filtered ("    %s", hex_string_custom (addr, addr_size));
  printf_filtered ("->%s", hex_string_custom (endaddr, addr_size));
  printf_filtered (" at %s",
		   hex_string_custom ((unsigned long) filepos, 8));
  printf_filtered (": %s", name);
  print_bfd_flags (flags);
  printf_filtered ("\n");
}

static void
print_bfd_section_info (bfd *abfd, 
			asection *asect, 
			void *datum)
{
  flagword flags = bfd_get_section_flags (abfd, asect);
  const char *name = bfd_section_name (abfd, asect);
  const char *arg = (const char *) datum;

  if (arg == NULL || *arg == '\0'
      || match_substring (arg, name)
      || match_bfd_flags (arg, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;
      CORE_ADDR addr, endaddr;

      addr = bfd_section_vma (abfd, asect);
      endaddr = addr + bfd_section_size (abfd, asect);
      printf_filtered (" [%d] ", gdb_bfd_section_index (abfd, asect));
      maint_print_section_info (name, flags, addr, endaddr,
				asect->filepos, addr_size);
    }
}

static void
print_objfile_section_info (bfd *abfd, 
			    struct obj_section *asect, 
			    const char *string)
{
  flagword flags = bfd_get_section_flags (abfd, asect->the_bfd_section);
  const char *name = bfd_section_name (abfd, asect->the_bfd_section);

  if (string == NULL || *string == '\0'
      || match_substring (string, name)
      || match_bfd_flags (string, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;

      maint_print_section_info (name, flags,
				obj_section_addr (asect),
				obj_section_endaddr (asect),
				asect->the_bfd_section->filepos,
				addr_size);
    }
}

static void
maintenance_info_sections (char *arg, int from_tty)
{
  if (exec_bfd)
    {
      printf_filtered (_("Exec file:\n"));
      printf_filtered ("    `%s', ", bfd_get_filename (exec_bfd));
      wrap_here ("        ");
      printf_filtered (_("file type %s.\n"), bfd_get_target (exec_bfd));
      if (arg && *arg && match_substring (arg, "ALLOBJ"))
	{
	  struct objfile *ofile;
	  struct obj_section *osect;

	  /* Only this function cares about the 'ALLOBJ' argument; 
	     if 'ALLOBJ' is the only argument, discard it rather than
	     passing it down to print_objfile_section_info (which 
	     wouldn't know how to handle it).  */
	  if (strcmp (arg, "ALLOBJ") == 0)
	    arg = NULL;

	  ALL_OBJFILES (ofile)
	    {
	      printf_filtered (_("  Object file: %s\n"), 
			       bfd_get_filename (ofile->obfd));
	      ALL_OBJFILE_OSECTIONS (ofile, osect)
		{
		  print_objfile_section_info (ofile->obfd, osect, arg);
		}
	    }
	}
      else 
	bfd_map_over_sections (exec_bfd, print_bfd_section_info, arg);
    }

  if (core_bfd)
    {
      printf_filtered (_("Core file:\n"));
      printf_filtered ("    `%s', ", bfd_get_filename (core_bfd));
      wrap_here ("        ");
      printf_filtered (_("file type %s.\n"), bfd_get_target (core_bfd));
      bfd_map_over_sections (core_bfd, print_bfd_section_info, arg);
    }
}

static void
maintenance_print_statistics (char *args, int from_tty)
{
  print_objfile_statistics ();
  print_symbol_bcache_statistics ();
}

static void
maintenance_print_architecture (char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (args == NULL)
    gdbarch_dump (gdbarch, gdb_stdout);
  else
    {
      struct cleanup *cleanups;
      struct ui_file *file = gdb_fopen (args, "w");

      if (file == NULL)
	perror_with_name (_("maintenance print architecture"));
      cleanups = make_cleanup_ui_file_delete (file);
      gdbarch_dump (gdbarch, file);
      do_cleanups (cleanups);
    }
}

/* The "maintenance print" command is defined as a prefix, with
   allow_unknown 0.  Therefore, its own definition is called only for
   "maintenance print" with no args.  */

static void
maintenance_print_command (char *arg, int from_tty)
{
  printf_unfiltered (_("\"maintenance print\" must be followed "
		       "by the name of a print command.\n"));
  help_list (maintenanceprintlist, "maintenance print ", all_commands,
	     gdb_stdout);
}

/* The "maintenance translate-address" command converts a section and address
   to a symbol.  This can be called in two ways:
   maintenance translate-address <secname> <addr>
   or   maintenance translate-address <addr>.  */

static void
maintenance_translate_address (char *arg, int from_tty)
{
  CORE_ADDR address;
  struct obj_section *sect;
  char *p;
  struct bound_minimal_symbol sym;
  struct objfile *objfile;

  if (arg == NULL || *arg == 0)
    error (_("requires argument (address or section + address)"));

  sect = NULL;
  p = arg;

  if (!isdigit (*p))
    {				/* See if we have a valid section name.  */
      while (*p && !isspace (*p))	/* Find end of section name.  */
	p++;
      if (*p == '\000')		/* End of command?  */
	error (_("Need to specify <section-name> and <address>"));
      *p++ = '\000';
      p = skip_spaces (p);

      ALL_OBJSECTIONS (objfile, sect)
      {
	if (strcmp (sect->the_bfd_section->name, arg) == 0)
	  break;
      }

      if (!objfile)
	error (_("Unknown section %s."), arg);
    }

  address = parse_and_eval_address (p);

  if (sect)
    sym = lookup_minimal_symbol_by_pc_section (address, sect);
  else
    sym = lookup_minimal_symbol_by_pc (address);

  if (sym.minsym)
    {
      const char *symbol_name = MSYMBOL_PRINT_NAME (sym.minsym);
      const char *symbol_offset
	= pulongest (address - BMSYMBOL_VALUE_ADDRESS (sym));

      sect = MSYMBOL_OBJ_SECTION(sym.objfile, sym.minsym);
      if (sect != NULL)
	{
	  const char *section_name;
	  const char *obj_name;

	  gdb_assert (sect->the_bfd_section && sect->the_bfd_section->name);
	  section_name = sect->the_bfd_section->name;

	  gdb_assert (sect->objfile && objfile_name (sect->objfile));
	  obj_name = objfile_name (sect->objfile);

	  if (MULTI_OBJFILE_P ())
	    printf_filtered (_("%s + %s in section %s of %s\n"),
			     symbol_name, symbol_offset,
			     section_name, obj_name);
	  else
	    printf_filtered (_("%s + %s in section %s\n"),
			     symbol_name, symbol_offset, section_name);
	}
      else
	printf_filtered (_("%s + %s\n"), symbol_name, symbol_offset);
    }
  else if (sect)
    printf_filtered (_("no symbol at %s:%s\n"),
		     sect->the_bfd_section->name, hex_string (address));
  else
    printf_filtered (_("no symbol at %s\n"), hex_string (address));

  return;
}


/* When a command is deprecated the user will be warned the first time
   the command is used.  If possible, a replacement will be
   offered.  */

static void
maintenance_deprecate (char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    {
      printf_unfiltered (_("\"maintenance deprecate\" takes an argument,\n\
the command you want to deprecate, and optionally the replacement command\n\
enclosed in quotes.\n"));
    }

  maintenance_do_deprecate (args, 1);

}


static void
maintenance_undeprecate (char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    {
      printf_unfiltered (_("\"maintenance undeprecate\" takes an argument, \n\
the command you want to undeprecate.\n"));
    }

  maintenance_do_deprecate (args, 0);

}

/* You really shouldn't be using this.  It is just for the testsuite.
   Rather, you should use deprecate_cmd() when the command is created
   in _initialize_blah().

   This function deprecates a command and optionally assigns it a
   replacement.  */

static void
maintenance_do_deprecate (char *text, int deprecate)
{
  struct cmd_list_element *alias = NULL;
  struct cmd_list_element *prefix_cmd = NULL;
  struct cmd_list_element *cmd = NULL;

  char *start_ptr = NULL;
  char *end_ptr = NULL;
  int len;
  char *replacement = NULL;

  if (text == NULL)
    return;

  if (!lookup_cmd_composition (text, &alias, &prefix_cmd, &cmd))
    {
      printf_filtered (_("Can't find command '%s' to deprecate.\n"), text);
      return;
    }

  if (deprecate)
    {
      /* Look for a replacement command.  */
      start_ptr = strchr (text, '\"');
      if (start_ptr != NULL)
	{
	  start_ptr++;
	  end_ptr = strrchr (start_ptr, '\"');
	  if (end_ptr != NULL)
	    {
	      len = end_ptr - start_ptr;
	      start_ptr[len] = '\0';
	      replacement = xstrdup (start_ptr);
	    }
	}
    }

  if (!start_ptr || !end_ptr)
    replacement = NULL;


  /* If they used an alias, we only want to deprecate the alias.

     Note the MALLOCED_REPLACEMENT test.  If the command's replacement
     string was allocated at compile time we don't want to free the
     memory.  */
  if (alias)
    {
      if (alias->malloced_replacement)
	xfree ((char *) alias->replacement);

      if (deprecate)
	{
	  alias->deprecated_warn_user = 1;
	  alias->cmd_deprecated = 1;
	}
      else
	{
	  alias->deprecated_warn_user = 0;
	  alias->cmd_deprecated = 0;
	}
      alias->replacement = replacement;
      alias->malloced_replacement = 1;
      return;
    }
  else if (cmd)
    {
      if (cmd->malloced_replacement)
	xfree ((char *) cmd->replacement);

      if (deprecate)
	{
	  cmd->deprecated_warn_user = 1;
	  cmd->cmd_deprecated = 1;
	}
      else
	{
	  cmd->deprecated_warn_user = 0;
	  cmd->cmd_deprecated = 0;
	}
      cmd->replacement = replacement;
      cmd->malloced_replacement = 1;
      return;
    }
  xfree (replacement);
}

/* Maintenance set/show framework.  */

struct cmd_list_element *maintenance_set_cmdlist;
struct cmd_list_element *maintenance_show_cmdlist;

static void
maintenance_set_cmd (char *args, int from_tty)
{
  printf_unfiltered (_("\"maintenance set\" must be followed "
		       "by the name of a set command.\n"));
  help_list (maintenance_set_cmdlist, "maintenance set ", all_commands,
	     gdb_stdout);
}

static void
maintenance_show_cmd (char *args, int from_tty)
{
  cmd_show_list (maintenance_show_cmdlist, from_tty, "");
}

/* Profiling support.  */

static int maintenance_profile_p;
static void
show_maintenance_profile_p (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Internal profiling is %s.\n"), value);
}

#ifdef HAVE__ETEXT
extern char _etext;
#define TEXTEND &_etext
#elif defined (HAVE_ETEXT)
extern char etext;
#define TEXTEND &etext
#endif

#if defined (HAVE_MONSTARTUP) && defined (HAVE__MCLEANUP) && defined (TEXTEND)

static int profiling_state;

EXTERN_C void _mcleanup (void);

static void
mcleanup_wrapper (void)
{
  if (profiling_state)
    _mcleanup ();
}

EXTERN_C void monstartup (unsigned long, unsigned long);
extern int main ();

static void
maintenance_set_profile_cmd (char *args, int from_tty,
			     struct cmd_list_element *c)
{
  if (maintenance_profile_p == profiling_state)
    return;

  profiling_state = maintenance_profile_p;

  if (maintenance_profile_p)
    {
      static int profiling_initialized;

      if (!profiling_initialized)
	{
	  atexit (mcleanup_wrapper);
	  profiling_initialized = 1;
	}

      /* "main" is now always the first function in the text segment, so use
	 its address for monstartup.  */
      monstartup ((unsigned long) &main, (unsigned long) TEXTEND);
    }
  else
    {
      extern void _mcleanup (void);

      _mcleanup ();
    }
}
#else
static void
maintenance_set_profile_cmd (char *args, int from_tty,
			     struct cmd_list_element *c)
{
  error (_("Profiling support is not available on this system."));
}
#endif

/* If nonzero, display time usage both at startup and for each command.  */

static int per_command_time;

/* If nonzero, display space usage both at startup and for each command.  */

static int per_command_space;

/* If nonzero, display basic symtab stats for each command.  */

static int per_command_symtab;

/* mt per-command commands.  */

static struct cmd_list_element *per_command_setlist;
static struct cmd_list_element *per_command_showlist;

/* Records a run time and space usage to be used as a base for
   reporting elapsed time or change in space.  */

struct cmd_stats 
{
  /* Zero if the saved time is from the beginning of GDB execution.
     One if from the beginning of an individual command execution.  */
  int msg_type;
  /* Track whether the stat was enabled at the start of the command
     so that we can avoid printing anything if it gets turned on by
     the current command.  */
  int time_enabled : 1;
  int space_enabled : 1;
  int symtab_enabled : 1;
  long start_cpu_time;
  struct timeval start_wall_time;
  long start_space;
  /* Total number of symtabs (over all objfiles).  */
  int start_nr_symtabs;
  /* A count of the compunits.  */
  int start_nr_compunit_symtabs;
  /* Total number of blocks.  */
  int start_nr_blocks;
};

/* Set whether to display time statistics to NEW_VALUE
   (non-zero means true).  */

void
set_per_command_time (int new_value)
{
  per_command_time = new_value;
}

/* Set whether to display space statistics to NEW_VALUE
   (non-zero means true).  */

void
set_per_command_space (int new_value)
{
  per_command_space = new_value;
}

/* Count the number of symtabs and blocks.  */

static void
count_symtabs_and_blocks (int *nr_symtabs_ptr, int *nr_compunit_symtabs_ptr,
			  int *nr_blocks_ptr)
{
  struct objfile *o;
  struct compunit_symtab *cu;
  struct symtab *s;
  int nr_symtabs = 0;
  int nr_compunit_symtabs = 0;
  int nr_blocks = 0;

  /* When collecting statistics during startup, this is called before
     pretty much anything in gdb has been initialized, and thus
     current_program_space may be NULL.  */
  if (current_program_space != NULL)
    {
      ALL_COMPUNITS (o, cu)
	{
	  ++nr_compunit_symtabs;
	  nr_blocks += BLOCKVECTOR_NBLOCKS (COMPUNIT_BLOCKVECTOR (cu));
	  ALL_COMPUNIT_FILETABS (cu, s)
	    ++nr_symtabs;
	}
    }

  *nr_symtabs_ptr = nr_symtabs;
  *nr_compunit_symtabs_ptr = nr_compunit_symtabs;
  *nr_blocks_ptr = nr_blocks;
}

/* As indicated by display_time and display_space, report GDB's elapsed time
   and space usage from the base time and space provided in ARG, which
   must be a pointer to a struct cmd_stat.  This function is intended
   to be called as a cleanup.  */

static void
report_command_stats (void *arg)
{
  struct cmd_stats *start_stats = (struct cmd_stats *) arg;
  int msg_type = start_stats->msg_type;

  if (start_stats->time_enabled && per_command_time)
    {
      long cmd_time = get_run_time () - start_stats->start_cpu_time;
      struct timeval now_wall_time, delta_wall_time, wait_time;

      gettimeofday (&now_wall_time, NULL);
      timeval_sub (&delta_wall_time,
		   &now_wall_time, &start_stats->start_wall_time);

      /* Subtract time spend in prompt_for_continue from walltime.  */
      wait_time = get_prompt_for_continue_wait_time ();
      timeval_sub (&delta_wall_time, &delta_wall_time, &wait_time);

      printf_unfiltered (msg_type == 0
			 ? _("Startup time: %ld.%06ld (cpu), %ld.%06ld (wall)\n")
			 : _("Command execution time: %ld.%06ld (cpu), %ld.%06ld (wall)\n"),
			 cmd_time / 1000000, cmd_time % 1000000,
			 (long) delta_wall_time.tv_sec,
			 (long) delta_wall_time.tv_usec);
    }

  if (start_stats->space_enabled && per_command_space)
    {
#ifdef HAVE_SBRK
      char *lim = (char *) sbrk (0);

      long space_now = lim - lim_at_start;
      long space_diff = space_now - start_stats->start_space;

      printf_unfiltered (msg_type == 0
			 ? _("Space used: %ld (%s%ld during startup)\n")
			 : _("Space used: %ld (%s%ld for this command)\n"),
			 space_now,
			 (space_diff >= 0 ? "+" : ""),
			 space_diff);
#endif
    }

  if (start_stats->symtab_enabled && per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      printf_unfiltered (_("#symtabs: %d (+%d),"
			   " #compunits: %d (+%d),"
			   " #blocks: %d (+%d)\n"),
			 nr_symtabs,
			 nr_symtabs - start_stats->start_nr_symtabs,
			 nr_compunit_symtabs,
			 (nr_compunit_symtabs
			  - start_stats->start_nr_compunit_symtabs),
			 nr_blocks,
			 nr_blocks - start_stats->start_nr_blocks);
    }
}

/* Create a cleanup that reports time and space used since its creation.
   MSG_TYPE is zero for gdb startup, otherwise it is one(1) to report
   data for individual commands.  */

struct cleanup *
make_command_stats_cleanup (int msg_type)
{
  struct cmd_stats *new_stat;

  /* Early exit if we're not reporting any stats.  It can be expensive to
     compute the pre-command values so don't collect them at all if we're
     not reporting stats.  Alas this doesn't work in the startup case because
     we don't know yet whether we will be reporting the stats.  For the
     startup case collect the data anyway (it should be cheap at this point),
     and leave it to the reporter to decide whether to print them.  */
  if (msg_type != 0
      && !per_command_time
      && !per_command_space
      && !per_command_symtab)
    return make_cleanup (null_cleanup, 0);

  new_stat = XCNEW (struct cmd_stats);

  new_stat->msg_type = msg_type;

  if (msg_type == 0 || per_command_space)
    {
#ifdef HAVE_SBRK
      char *lim = (char *) sbrk (0);
      new_stat->start_space = lim - lim_at_start;
      new_stat->space_enabled = 1;
#endif
    }

  if (msg_type == 0 || per_command_time)
    {
      new_stat->start_cpu_time = get_run_time ();
      gettimeofday (&new_stat->start_wall_time, NULL);
      new_stat->time_enabled = 1;
    }

  if (msg_type == 0 || per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      new_stat->start_nr_symtabs = nr_symtabs;
      new_stat->start_nr_compunit_symtabs = nr_compunit_symtabs;
      new_stat->start_nr_blocks = nr_blocks;
      new_stat->symtab_enabled = 1;
    }

  /* Initalize timer to keep track of how long we waited for the user.  */
  reset_prompt_for_continue_wait_time ();

  return make_cleanup_dtor (report_command_stats, new_stat, xfree);
}

/* Handle unknown "mt set per-command" arguments.
   In this case have "mt set per-command on|off" affect every setting.  */

static void
set_per_command_cmd (char *args, int from_tty)
{
  struct cmd_list_element *list;
  int val;

  val = parse_cli_boolean_value (args);
  if (val < 0)
    error (_("Bad value for 'mt set per-command no'."));

  for (list = per_command_setlist; list != NULL; list = list->next)
    if (list->var_type == var_boolean)
      {
	gdb_assert (list->type == set_cmd);
	do_set_command (args, from_tty, list);
      }
}

/* Command "show per-command" displays summary of all the current
   "show per-command " settings.  */

static void
show_per_command_cmd (char *args, int from_tty)
{
  cmd_show_list (per_command_showlist, from_tty, "");
}


/* The "maintenance selftest" command.  */

static void
maintenance_selftest (char *args, int from_tty)
{
  run_self_tests ();
}


void
_initialize_maint_cmds (void)
{
  struct cmd_list_element *cmd;

  add_prefix_cmd ("maintenance", class_maintenance, maintenance_command, _("\
Commands for use by GDB maintainers.\n\
Includes commands to dump specific internal GDB structures in\n\
a human readable form, to cause GDB to deliberately dump core, etc."),
		  &maintenancelist, "maintenance ", 0,
		  &cmdlist);

  add_com_alias ("mt", "maintenance", class_maintenance, 1);

  add_prefix_cmd ("info", class_maintenance, maintenance_info_command, _("\
Commands for showing internal info about the program being debugged."),
		  &maintenanceinfolist, "maintenance info ", 0,
		  &maintenancelist);
  add_alias_cmd ("i", "info", class_maintenance, 1, &maintenancelist);

  add_cmd ("sections", class_maintenance, maintenance_info_sections, _("\
List the BFD sections of the exec and core files. \n\
Arguments may be any combination of:\n\
	[one or more section names]\n\
	ALLOC LOAD RELOC READONLY CODE DATA ROM CONSTRUCTOR\n\
	HAS_CONTENTS NEVER_LOAD COFF_SHARED_LIBRARY IS_COMMON\n\
Sections matching any argument will be listed (no argument\n\
implies all sections).  In addition, the special argument\n\
	ALLOBJ\n\
lists all sections from all object files, including shared libraries."),
	   &maintenanceinfolist);

  add_prefix_cmd ("print", class_maintenance, maintenance_print_command,
		  _("Maintenance command for printing GDB internal state."),
		  &maintenanceprintlist, "maintenance print ", 0,
		  &maintenancelist);

  add_prefix_cmd ("set", class_maintenance, maintenance_set_cmd, _("\
Set GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
		  &maintenance_set_cmdlist, "maintenance set ",
		  0/*allow-unknown*/,
		  &maintenancelist);

  add_prefix_cmd ("show", class_maintenance, maintenance_show_cmd, _("\
Show GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
		  &maintenance_show_cmdlist, "maintenance show ",
		  0/*allow-unknown*/,
		  &maintenancelist);

#ifndef _WIN32
  add_cmd ("dump-me", class_maintenance, maintenance_dump_me, _("\
Get fatal error; make debugger dump its core.\n\
GDB sets its handling of SIGQUIT back to SIG_DFL and then sends\n\
itself a SIGQUIT signal."),
	   &maintenancelist);
#endif

  add_cmd ("internal-error", class_maintenance,
	   maintenance_internal_error, _("\
Give GDB an internal error.\n\
Cause GDB to behave as if an internal error was detected."),
	   &maintenancelist);

  add_cmd ("internal-warning", class_maintenance,
	   maintenance_internal_warning, _("\
Give GDB an internal warning.\n\
Cause GDB to behave as if an internal warning was reported."),
	   &maintenancelist);

  add_cmd ("demangler-warning", class_maintenance,
	   maintenance_demangler_warning, _("\
Give GDB a demangler warning.\n\
Cause GDB to behave as if a demangler warning was reported."),
	   &maintenancelist);

  cmd = add_cmd ("demangle", class_maintenance, maintenance_demangle, _("\
This command has been moved to \"demangle\"."),
		 &maintenancelist);
  deprecate_cmd (cmd, "demangle");

  add_prefix_cmd ("per-command", class_maintenance, set_per_command_cmd, _("\
Per-command statistics settings."),
		    &per_command_setlist, "set per-command ",
		    1/*allow-unknown*/, &maintenance_set_cmdlist);

  add_prefix_cmd ("per-command", class_maintenance, show_per_command_cmd, _("\
Show per-command statistics settings."),
		    &per_command_showlist, "show per-command ",
		    0/*allow-unknown*/, &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("time", class_maintenance,
			   &per_command_time, _("\
Set whether to display per-command execution time."), _("\
Show whether to display per-command execution time."),
			   _("\
If enabled, the execution time for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  add_setshow_boolean_cmd ("space", class_maintenance,
			   &per_command_space, _("\
Set whether to display per-command space usage."), _("\
Show whether to display per-command space usage."),
			   _("\
If enabled, the space usage for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  add_setshow_boolean_cmd ("symtab", class_maintenance,
			   &per_command_symtab, _("\
Set whether to display per-command symtab statistics."), _("\
Show whether to display per-command symtab statistics."),
			   _("\
If enabled, the basic symtab statistics for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  /* This is equivalent to "mt set per-command time on".
     Kept because some people are used to typing "mt time 1".  */
  add_cmd ("time", class_maintenance, maintenance_time_display, _("\
Set the display of time usage.\n\
If nonzero, will cause the execution time for each command to be\n\
displayed, following the command's output."),
	   &maintenancelist);

  /* This is equivalent to "mt set per-command space on".
     Kept because some people are used to typing "mt space 1".  */
  add_cmd ("space", class_maintenance, maintenance_space_display, _("\
Set the display of space usage.\n\
If nonzero, will cause the execution space for each command to be\n\
displayed, following the command's output."),
	   &maintenancelist);

  add_cmd ("type", class_maintenance, maintenance_print_type, _("\
Print a type chain for a given symbol.\n\
For each node in a type chain, print the raw data for each member of\n\
the type structure, and the interpretation of the data."),
	   &maintenanceprintlist);

  add_cmd ("statistics", class_maintenance, maintenance_print_statistics,
	   _("Print statistics about internal gdb state."),
	   &maintenanceprintlist);

  add_cmd ("architecture", class_maintenance,
	   maintenance_print_architecture, _("\
Print the internal architecture configuration.\n\
Takes an optional file parameter."),
	   &maintenanceprintlist);

  add_cmd ("translate-address", class_maintenance,
	   maintenance_translate_address,
	   _("Translate a section name and address to a symbol."),
	   &maintenancelist);

  add_cmd ("deprecate", class_maintenance, maintenance_deprecate, _("\
Deprecate a command.  Note that this is just in here so the \n\
testsuite can check the command deprecator. You probably shouldn't use this,\n\
rather you should use the C function deprecate_cmd().  If you decide you \n\
want to use it: maintenance deprecate 'commandname' \"replacement\". The \n\
replacement is optional."), &maintenancelist);

  add_cmd ("undeprecate", class_maintenance, maintenance_undeprecate, _("\
Undeprecate a command.  Note that this is just in here so the \n\
testsuite can check the command deprecator. You probably shouldn't use this,\n\
If you decide you want to use it: maintenance undeprecate 'commandname'"),
	   &maintenancelist);

  add_cmd ("selftest", class_maintenance, maintenance_selftest, _("\
Run gdb's unit tests.\n\
Usage: maintenance selftest\n\
This will run any unit tests that were built in to gdb.\n\
gdb will abort if any test fails."),
	   &maintenancelist);

  add_setshow_zinteger_cmd ("watchdog", class_maintenance, &watchdog, _("\
Set watchdog timer."), _("\
Show watchdog timer."), _("\
When non-zero, this timeout is used instead of waiting forever for a target\n\
to finish a low-level step or continue operation.  If the specified amount\n\
of time passes without a response from the target, an error occurs."),
			    NULL,
			    show_watchdog,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("profile", class_maintenance,
			   &maintenance_profile_p, _("\
Set internal profiling."), _("\
Show internal profiling."), _("\
When enabled GDB is profiled."),
			   maintenance_set_profile_cmd,
			   show_maintenance_profile_p,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}
