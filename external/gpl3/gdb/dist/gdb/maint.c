/* Support for GDB maintenance commands.

   Copyright (C) 1992-2020 Free Software Foundation, Inc.

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
#include <cmath>
#include <signal.h>
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
#include "maint.h"
#include "gdbsupport/selftest.h"

#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "cli/cli-setshow.h"
#include "cli/cli-cmds.h"

#if CXX_STD_THREAD
#include "gdbsupport/thread-pool.h"
#endif

static void maintenance_do_deprecate (const char *, int);

#ifndef _WIN32
static void
maintenance_dump_me (const char *args, int from_tty)
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
maintenance_internal_error (const char *args, int from_tty)
{
  internal_error (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   internal problem is detected.  Allows testing of the mechanism.
   Also useful when the user wants to drop a core file but not exit
   GDB.  */

static void
maintenance_internal_warning (const char *args, int from_tty)
{
  internal_warning (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   demangler problem is detected.  Allows testing of the mechanism.  */

static void
maintenance_demangler_warning (const char *args, int from_tty)
{
  demangler_warning (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Old command to demangle a string.  The command has been moved to "demangle".
   It is kept for now because otherwise "mt demangle" gets interpreted as
   "mt demangler-warning" which artificially creates an internal gdb error.  */

static void
maintenance_demangle (const char *args, int from_tty)
{
  printf_filtered (_("This command has been moved to \"demangle\".\n"));
}

static void
maintenance_time_display (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    printf_unfiltered (_("\"maintenance time\" takes a numeric argument.\n"));
  else
    set_per_command_time (strtol (args, NULL, 10));
}

static void
maintenance_space_display (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    printf_unfiltered ("\"maintenance space\" takes a numeric argument.\n");
  else
    set_per_command_space (strtol (args, NULL, 10));
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

/* Return the number of digits required to display COUNT in decimal.

   Used when pretty printing index numbers to ensure all of the indexes line
   up.*/

static int
index_digits (int count)
{
  return ((int) log10 ((float) count)) + 1;
}

/* Helper function to pretty-print the section index of ASECT from ABFD.
   The INDEX_DIGITS is the number of digits in the largest index that will
   be printed, and is used to pretty-print the resulting string.  */

static void
print_section_index (bfd *abfd,
		     asection *asect,
		     int index_digits)
{
  std::string result
    = string_printf (" [%d] ", gdb_bfd_section_index (abfd, asect));
  /* The '+ 4' for the leading and trailing characters.  */
  printf_filtered ("%-*s", (index_digits + 4), result.c_str ());
}

/* Print information about ASECT from ABFD.  The section will be printed using
   the VMA's from the bfd, which will not be the relocated addresses for bfds
   that should be relocated.  The information must be printed with the same
   layout as PRINT_OBJFILE_SECTION_INFO below.

   ARG is the argument string passed by the user to the top level maintenance
   info sections command.  Used for filtering which sections are printed.  */

static void
print_bfd_section_info (bfd *abfd, asection *asect, const char *arg,
			int index_digits)
{
  flagword flags = bfd_section_flags (asect);
  const char *name = bfd_section_name (asect);

  if (arg == NULL || *arg == '\0'
      || match_substring (arg, name)
      || match_bfd_flags (arg, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;
      CORE_ADDR addr, endaddr;

      addr = bfd_section_vma (asect);
      endaddr = addr + bfd_section_size (asect);
      print_section_index (abfd, asect, index_digits);
      maint_print_section_info (name, flags, addr, endaddr,
				asect->filepos, addr_size);
    }
}

/* Print information about ASECT which is GDB's wrapper around a section
   from ABFD.  The information must be printed with the same layout as
   PRINT_BFD_SECTION_INFO above.  PRINT_DATA holds information used to
   filter which sections are printed, and for formatting the output.

   ARG is the argument string passed by the user to the top level maintenance
   info sections command.  Used for filtering which sections are printed.  */

static void
print_objfile_section_info (bfd *abfd, struct obj_section *asect,
			    const char *arg, int index_digits)
{
  flagword flags = bfd_section_flags (asect->the_bfd_section);
  const char *name = bfd_section_name (asect->the_bfd_section);

  if (arg == NULL || *arg == '\0'
      || match_substring (arg, name)
      || match_bfd_flags (arg, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;

      print_section_index (abfd, asect->the_bfd_section, index_digits);
      maint_print_section_info (name, flags,
				obj_section_addr (asect),
				obj_section_endaddr (asect),
				asect->the_bfd_section->filepos,
				addr_size);
    }
}

/* Find an obj_section, GDB's wrapper around a bfd section for ASECTION
   from ABFD.  It might be that no such wrapper exists (for example debug
   sections don't have such wrappers) in which case nullptr is returned.  */

static obj_section *
maint_obj_section_from_bfd_section (bfd *abfd,
				    asection *asection,
				    objfile *ofile)
{
  if (ofile->sections == nullptr)
    return nullptr;

  obj_section *osect
    = &ofile->sections[gdb_bfd_section_index (abfd, asection)];

  if (osect >= ofile->sections_end)
    return nullptr;

  return osect;
}

/* Print information about ASECT from ABFD.  Where possible the information for
   ASECT will print the relocated addresses of the section.

   ARG is the argument string passed by the user to the top level maintenance
   info sections command.  Used for filtering which sections are printed.  */

static void
print_bfd_section_info_maybe_relocated (bfd *abfd, asection *asect,
					objfile *objfile, const char *arg,
					int index_digits)
{
  gdb_assert (objfile->sections != NULL);
  obj_section *osect
    = maint_obj_section_from_bfd_section (abfd, asect, objfile);

  if (osect->the_bfd_section == NULL)
    print_bfd_section_info (abfd, asect, arg, index_digits);
  else
    print_objfile_section_info (abfd, osect, arg, index_digits);
}

/* Implement the "maintenance info sections" command.  */

static void
maintenance_info_sections (const char *arg, int from_tty)
{
  if (exec_bfd)
    {
      bool allobj = false;

      printf_filtered (_("Exec file:\n"));
      printf_filtered ("    `%s', ", bfd_get_filename (exec_bfd));
      wrap_here ("        ");
      printf_filtered (_("file type %s.\n"), bfd_get_target (exec_bfd));

      /* Only this function cares about the 'ALLOBJ' argument;
	 if 'ALLOBJ' is the only argument, discard it rather than
	 passing it down to print_objfile_section_info (which
	 wouldn't know how to handle it).  */
      if (arg && strcmp (arg, "ALLOBJ") == 0)
	{
	  arg = NULL;
	  allobj = true;
	}

      for (objfile *ofile : current_program_space->objfiles ())
	{
	  if (allobj)
	    printf_filtered (_("  Object file: %s\n"),
			     bfd_get_filename (ofile->obfd));
	  else if (ofile->obfd != exec_bfd)
	    continue;

	  int section_count = gdb_bfd_count_sections (ofile->obfd);

	  for (asection *sect : gdb_bfd_sections (ofile->obfd))
	    print_bfd_section_info_maybe_relocated
	      (ofile->obfd, sect, ofile, arg, index_digits (section_count));
	}
    }

  if (core_bfd)
    {
      printf_filtered (_("Core file:\n"));
      printf_filtered ("    `%s', ", bfd_get_filename (core_bfd));
      wrap_here ("        ");
      printf_filtered (_("file type %s.\n"), bfd_get_target (core_bfd));

      int section_count = gdb_bfd_count_sections (core_bfd);

      for (asection *sect : gdb_bfd_sections (core_bfd))
	print_bfd_section_info (core_bfd, sect, arg,
				index_digits (section_count));
    }
}

static void
maintenance_print_statistics (const char *args, int from_tty)
{
  print_objfile_statistics ();
  print_symbol_bcache_statistics ();
}

static void
maintenance_print_architecture (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (args == NULL)
    gdbarch_dump (gdbarch, gdb_stdout);
  else
    {
      stdio_file file;

      if (!file.open (args, "w"))
	perror_with_name (_("maintenance print architecture"));
      gdbarch_dump (gdbarch, &file);
    }
}

/* The "maintenance translate-address" command converts a section and address
   to a symbol.  This can be called in two ways:
   maintenance translate-address <secname> <addr>
   or   maintenance translate-address <addr>.  */

static void
maintenance_translate_address (const char *arg, int from_tty)
{
  CORE_ADDR address;
  struct obj_section *sect;
  const char *p;
  struct bound_minimal_symbol sym;

  if (arg == NULL || *arg == 0)
    error (_("requires argument (address or section + address)"));

  sect = NULL;
  p = arg;

  if (!isdigit (*p))
    {				/* See if we have a valid section name.  */
      while (*p && !isspace (*p))	/* Find end of section name.  */
	p++;
      if (*p == '\000')		/* End of command?  */
	error (_("Need to specify section name and address"));

      int arg_len = p - arg;
      p = skip_spaces (p + 1);

      for (objfile *objfile : current_program_space->objfiles ())
	ALL_OBJFILE_OSECTIONS (objfile, sect)
	  {
	    if (strncmp (sect->the_bfd_section->name, arg, arg_len) == 0)
	      goto found;
	  }

      error (_("Unknown section %s."), arg);
    found: ;
    }

  address = parse_and_eval_address (p);

  if (sect)
    sym = lookup_minimal_symbol_by_pc_section (address, sect);
  else
    sym = lookup_minimal_symbol_by_pc (address);

  if (sym.minsym)
    {
      const char *symbol_name = sym.minsym->print_name ();
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

	  if (current_program_space->multi_objfile_p ())
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
maintenance_deprecate (const char *args, int from_tty)
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
maintenance_undeprecate (const char *args, int from_tty)
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
maintenance_do_deprecate (const char *text, int deprecate)
{
  struct cmd_list_element *alias = NULL;
  struct cmd_list_element *prefix_cmd = NULL;
  struct cmd_list_element *cmd = NULL;

  const char *start_ptr = NULL;
  const char *end_ptr = NULL;
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
	      replacement = savestring (start_ptr, len);
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

/* "maintenance with" command.  */

static void
maintenance_with_cmd (const char *args, int from_tty)
{
  with_command_1 ("maintenance set ", maintenance_set_cmdlist, args, from_tty);
}

/* "maintenance with" command completer.  */

static void
maintenance_with_cmd_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char * /*word*/)
{
  with_command_completer_1 ("maintenance set ", tracker,  text);
}

/* Profiling support.  */

static bool maintenance_profile_p;
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
maintenance_set_profile_cmd (const char *args, int from_tty,
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
maintenance_set_profile_cmd (const char *args, int from_tty,
			     struct cmd_list_element *c)
{
  error (_("Profiling support is not available on this system."));
}
#endif

static int n_worker_threads = -1;

/* Update the thread pool for the desired number of threads.  */
static void
update_thread_pool_size ()
{
#if CXX_STD_THREAD
  int n_threads = n_worker_threads;

  if (n_threads < 0)
    n_threads = std::thread::hardware_concurrency ();

  gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);
#endif
}

static void
maintenance_set_worker_threads (const char *args, int from_tty,
				struct cmd_list_element *c)
{
  update_thread_pool_size ();
}


/* If true, display time usage both at startup and for each command.  */

static bool per_command_time;

/* If true, display space usage both at startup and for each command.  */

static bool per_command_space;

/* If true, display basic symtab stats for each command.  */

static bool per_command_symtab;

/* mt per-command commands.  */

static struct cmd_list_element *per_command_setlist;
static struct cmd_list_element *per_command_showlist;

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
  int nr_symtabs = 0;
  int nr_compunit_symtabs = 0;
  int nr_blocks = 0;

  /* When collecting statistics during startup, this is called before
     pretty much anything in gdb has been initialized, and thus
     current_program_space may be NULL.  */
  if (current_program_space != NULL)
    {
      for (objfile *o : current_program_space->objfiles ())
	{
	  for (compunit_symtab *cu : o->compunits ())
	    {
	      ++nr_compunit_symtabs;
	      nr_blocks += BLOCKVECTOR_NBLOCKS (COMPUNIT_BLOCKVECTOR (cu));
	      nr_symtabs += std::distance (compunit_filetabs (cu).begin (),
					   compunit_filetabs (cu).end ());
	    }
	}
    }

  *nr_symtabs_ptr = nr_symtabs;
  *nr_compunit_symtabs_ptr = nr_compunit_symtabs;
  *nr_blocks_ptr = nr_blocks;
}

/* As indicated by display_time and display_space, report GDB's
   elapsed time and space usage from the base time and space recorded
   in this object.  */

scoped_command_stats::~scoped_command_stats ()
{
  /* Early exit if we're not reporting any stats.  It can be expensive to
     compute the pre-command values so don't collect them at all if we're
     not reporting stats.  Alas this doesn't work in the startup case because
     we don't know yet whether we will be reporting the stats.  For the
     startup case collect the data anyway (it should be cheap at this point),
     and leave it to the reporter to decide whether to print them.  */
  if (m_msg_type
      && !per_command_time
      && !per_command_space
      && !per_command_symtab)
    return;

  if (m_time_enabled && per_command_time)
    {
      print_time (_("command finished"));

      using namespace std::chrono;

      run_time_clock::duration cmd_time
	= run_time_clock::now () - m_start_cpu_time;

      steady_clock::duration wall_time
	= steady_clock::now () - m_start_wall_time;
      /* Subtract time spend in prompt_for_continue from walltime.  */
      wall_time -= get_prompt_for_continue_wait_time ();

      printf_unfiltered (!m_msg_type
			 ? _("Startup time: %.6f (cpu), %.6f (wall)\n")
			 : _("Command execution time: %.6f (cpu), %.6f (wall)\n"),
			 duration<double> (cmd_time).count (),
			 duration<double> (wall_time).count ());
    }

  if (m_space_enabled && per_command_space)
    {
#ifdef HAVE_USEFUL_SBRK
      char *lim = (char *) sbrk (0);

      long space_now = lim - lim_at_start;
      long space_diff = space_now - m_start_space;

      printf_unfiltered (!m_msg_type
			 ? _("Space used: %ld (%s%ld during startup)\n")
			 : _("Space used: %ld (%s%ld for this command)\n"),
			 space_now,
			 (space_diff >= 0 ? "+" : ""),
			 space_diff);
#endif
    }

  if (m_symtab_enabled && per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      printf_unfiltered (_("#symtabs: %d (+%d),"
			   " #compunits: %d (+%d),"
			   " #blocks: %d (+%d)\n"),
			 nr_symtabs,
			 nr_symtabs - m_start_nr_symtabs,
			 nr_compunit_symtabs,
			 (nr_compunit_symtabs
			  - m_start_nr_compunit_symtabs),
			 nr_blocks,
			 nr_blocks - m_start_nr_blocks);
    }
}

scoped_command_stats::scoped_command_stats (bool msg_type)
: m_msg_type (msg_type)
{
  if (!m_msg_type || per_command_space)
    {
#ifdef HAVE_USEFUL_SBRK
      char *lim = (char *) sbrk (0);
      m_start_space = lim - lim_at_start;
      m_space_enabled = 1;
#endif
    }
  else
    m_space_enabled = 0;

  if (msg_type == 0 || per_command_time)
    {
      using namespace std::chrono;

      m_start_cpu_time = run_time_clock::now ();
      m_start_wall_time = steady_clock::now ();
      m_time_enabled = 1;

      if (per_command_time)
	print_time (_("command started"));
    }
  else
    m_time_enabled = 0;

  if (msg_type == 0 || per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      m_start_nr_symtabs = nr_symtabs;
      m_start_nr_compunit_symtabs = nr_compunit_symtabs;
      m_start_nr_blocks = nr_blocks;
      m_symtab_enabled = 1;
    }
  else
    m_symtab_enabled = 0;

  /* Initialize timer to keep track of how long we waited for the user.  */
  reset_prompt_for_continue_wait_time ();
}

/* See maint.h.  */

void
scoped_command_stats::print_time (const char *msg)
{
  using namespace std::chrono;

  auto now = system_clock::now ();
  auto ticks = now.time_since_epoch ().count () / (1000 * 1000);
  auto millis = ticks % 1000;

  std::time_t as_time = system_clock::to_time_t (now);
  struct tm tm;
  localtime_r (&as_time, &tm);

  char out[100];
  strftime (out, sizeof (out), "%F %H:%M:%S", &tm);

  printf_unfiltered ("%s.%03d - %s\n", out, (int) millis, msg);
}

/* Handle unknown "mt set per-command" arguments.
   In this case have "mt set per-command on|off" affect every setting.  */

static void
set_per_command_cmd (const char *args, int from_tty)
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



/* The "maintenance selftest" command.  */

static void
maintenance_selftest (const char *args, int from_tty)
{
#if GDB_SELF_TEST
  gdb_argv argv (args);
  selftests::run_tests (argv.as_array_view ());
#else
  printf_filtered (_("\
Selftests have been disabled for this build.\n"));
#endif
}

static void
maintenance_info_selftests (const char *arg, int from_tty)
{
#if GDB_SELF_TEST
  printf_filtered ("Registered selftests:\n");
  selftests::for_each_selftest ([] (const std::string &name) {
    printf_filtered (" - %s\n", name.c_str ());
  });
#else
  printf_filtered (_("\
Selftests have been disabled for this build.\n"));
#endif
}


void _initialize_maint_cmds ();
void
_initialize_maint_cmds ()
{
  struct cmd_list_element *cmd;

  add_basic_prefix_cmd ("maintenance", class_maintenance, _("\
Commands for use by GDB maintainers.\n\
Includes commands to dump specific internal GDB structures in\n\
a human readable form, to cause GDB to deliberately dump core, etc."),
			&maintenancelist, "maintenance ", 0,
			&cmdlist);

  add_com_alias ("mt", "maintenance", class_maintenance, 1);

  add_basic_prefix_cmd ("info", class_maintenance, _("\
Commands for showing internal info about the program being debugged."),
			&maintenanceinfolist, "maintenance info ", 0,
			&maintenancelist);
  add_alias_cmd ("i", "info", class_maintenance, 1, &maintenancelist);

  add_cmd ("sections", class_maintenance, maintenance_info_sections, _("\
List the BFD sections of the exec and core files.\n\
Arguments may be any combination of:\n\
	[one or more section names]\n\
	ALLOC LOAD RELOC READONLY CODE DATA ROM CONSTRUCTOR\n\
	HAS_CONTENTS NEVER_LOAD COFF_SHARED_LIBRARY IS_COMMON\n\
Sections matching any argument will be listed (no argument\n\
implies all sections).  In addition, the special argument\n\
	ALLOBJ\n\
lists all sections from all object files, including shared libraries."),
	   &maintenanceinfolist);

  add_basic_prefix_cmd ("print", class_maintenance,
			_("Maintenance command for printing GDB internal state."),
			&maintenanceprintlist, "maintenance print ", 0,
			&maintenancelist);

  add_basic_prefix_cmd ("set", class_maintenance, _("\
Set GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
			&maintenance_set_cmdlist, "maintenance set ",
			0/*allow-unknown*/,
			&maintenancelist);

  add_show_prefix_cmd ("show", class_maintenance, _("\
Show GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
		       &maintenance_show_cmdlist, "maintenance show ",
		       0/*allow-unknown*/,
		       &maintenancelist);

  cmd = add_cmd ("with", class_maintenance, maintenance_with_cmd, _("\
Like \"with\", but works with \"maintenance set\" variables.\n\
Usage: maintenance with SETTING [VALUE] [-- COMMAND]\n\
With no COMMAND, repeats the last executed command.\n\
SETTING is any setting you can change with the \"maintenance set\"\n\
subcommands."),
		 &maintenancelist);
  set_cmd_completer_handle_brkchars (cmd, maintenance_with_cmd_completer);

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
		    &per_command_setlist, "maintenance set per-command ",
		    1/*allow-unknown*/, &maintenance_set_cmdlist);

  add_show_prefix_cmd ("per-command", class_maintenance, _("\
Show per-command statistics settings."),
		       &per_command_showlist, "maintenance show per-command ",
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

  add_basic_prefix_cmd ("check", class_maintenance, _("\
Commands for checking internal gdb state."),
			&maintenancechecklist, "maintenance check ", 0,
			&maintenancelist);

  add_cmd ("translate-address", class_maintenance,
	   maintenance_translate_address,
	   _("Translate a section name and address to a symbol."),
	   &maintenancelist);

  add_cmd ("deprecate", class_maintenance, maintenance_deprecate, _("\
Deprecate a command (for testing purposes).\n\
Usage: maintenance deprecate COMMANDNAME [\"REPLACEMENT\"]\n\
This is used by the testsuite to check the command deprecator.\n\
You probably shouldn't use this,\n\
rather you should use the C function deprecate_cmd()."), &maintenancelist);

  add_cmd ("undeprecate", class_maintenance, maintenance_undeprecate, _("\
Undeprecate a command (for testing purposes).\n\
Usage: maintenance undeprecate COMMANDNAME\n\
This is used by the testsuite to check the command deprecator.\n\
You probably shouldn't use this."),
	   &maintenancelist);

  add_cmd ("selftest", class_maintenance, maintenance_selftest, _("\
Run gdb's unit tests.\n\
Usage: maintenance selftest [FILTER]\n\
This will run any unit tests that were built in to gdb.\n\
If a filter is given, only the tests with that value in their name will ran."),
	   &maintenancelist);

  add_cmd ("selftests", class_maintenance, maintenance_info_selftests,
	 _("List the registered selftests."), &maintenanceinfolist);

  add_setshow_boolean_cmd ("profile", class_maintenance,
			   &maintenance_profile_p, _("\
Set internal profiling."), _("\
Show internal profiling."), _("\
When enabled GDB is profiled."),
			   maintenance_set_profile_cmd,
			   show_maintenance_profile_p,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_zuinteger_unlimited_cmd ("worker-threads",
				       class_maintenance,
				       &n_worker_threads, _("\
Set the number of worker threads GDB can use."), _("\
Show the number of worker threads GDB can use."), _("\
GDB may use multiple threads to speed up certain CPU-intensive operations,\n\
such as demangling symbol names."),
				       maintenance_set_worker_threads, NULL,
				       &maintenance_set_cmdlist,
				       &maintenance_show_cmdlist);

  update_thread_pool_size ();
}
