/* Linker file opening and searching.
   Copyright 1991, 1992, 1993, 1994, 1995, 1998, 1999, 2000, 2001, 2002,
   2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2012
   Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "bfdlink.h"
#include "safe-ctype.h"
#include "ld.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldmain.h"
#include <ldgram.h>
#include "ldlex.h"
#include "ldemul.h"
#include "libiberty.h"
#include "filenames.h"
#ifdef ENABLE_PLUGINS
#include "plugin-api.h"
#include "plugin.h"
#endif /* ENABLE_PLUGINS */

bfd_boolean  ldfile_assumed_script = FALSE;
const char * ldfile_output_machine_name = "";
unsigned long ldfile_output_machine;
enum bfd_architecture ldfile_output_architecture;
search_dirs_type * search_head;

#ifdef VMS
static char * slash = "";
#else
#if defined (_WIN32) && ! defined (__CYGWIN32__)
static char * slash = "\\";
#else
static char * slash = "/";
#endif
#endif

typedef struct search_arch
{
  char *name;
  struct search_arch *next;
} search_arch_type;

static search_dirs_type **search_tail_ptr = &search_head;
static search_arch_type *search_arch_head;
static search_arch_type **search_arch_tail_ptr = &search_arch_head;

/* Test whether a pathname, after canonicalization, is the same or a
   sub-directory of the sysroot directory.  */

static bfd_boolean
is_sysrooted_pathname (const char *name)
{
  char *realname;
  int len;
  bfd_boolean result;

  if (ld_canon_sysroot == NULL)
    return FALSE;

  realname = lrealpath (name);
  len = strlen (realname);
  result = FALSE;
  if (len > ld_canon_sysroot_len
      && IS_DIR_SEPARATOR (realname[ld_canon_sysroot_len]))
    {
      realname[ld_canon_sysroot_len] = '\0';
      result = FILENAME_CMP (ld_canon_sysroot, realname) == 0;
    }

  free (realname);
  return result;
}

/* Adds NAME to the library search path.
   Makes a copy of NAME using xmalloc().  */

void
ldfile_add_library_path (const char *name, bfd_boolean cmdline)
{
  search_dirs_type *new_dirs;

  if (!cmdline && config.only_cmd_line_lib_dirs)
    return;

  new_dirs = (search_dirs_type *) xmalloc (sizeof (search_dirs_type));
  new_dirs->next = NULL;
  new_dirs->cmdline = cmdline;
  *search_tail_ptr = new_dirs;
  search_tail_ptr = &new_dirs->next;

  /* If a directory is marked as honoring sysroot, prepend the sysroot path
     now.  */
  if (name[0] == '=')
    new_dirs->name = concat (ld_sysroot, name + 1, (const char *) NULL);
  else
    new_dirs->name = xstrdup (name);
}

/* Try to open a BFD for a lang_input_statement.  */

bfd_boolean
ldfile_try_open_bfd (const char *attempt,
		     lang_input_statement_type *entry)
{
  entry->the_bfd = bfd_openr (attempt, entry->target);

  if (verbose)
    {
      if (entry->the_bfd == NULL)
	info_msg (_("attempt to open %s failed\n"), attempt);
      else
	info_msg (_("attempt to open %s succeeded\n"), attempt);
    }

  if (entry->the_bfd == NULL)
    {
      if (bfd_get_error () == bfd_error_invalid_target)
	einfo (_("%F%P: invalid BFD target `%s'\n"), entry->target);
      return FALSE;
    }

  /* Linker needs to decompress sections.  */
  entry->the_bfd->flags |= BFD_DECOMPRESS;

  /* If we are searching for this file, see if the architecture is
     compatible with the output file.  If it isn't, keep searching.
     If we can't open the file as an object file, stop the search
     here.  If we are statically linking, ensure that we don't link
     a dynamic object.

     In the code below, it's OK to exit early if the check fails,
     closing the checked BFD and returning FALSE, but if the BFD
     checks out compatible, do not exit early returning TRUE, or
     the plugins will not get a chance to claim the file.  */

  if (entry->flags.search_dirs || !entry->flags.dynamic)
    {
      bfd *check;

      if (bfd_check_format (entry->the_bfd, bfd_archive))
	check = bfd_openr_next_archived_file (entry->the_bfd, NULL);
      else
	check = entry->the_bfd;

      if (check != NULL)
	{
	  if (! bfd_check_format (check, bfd_object))
	    {
	      if (check == entry->the_bfd
		  && entry->flags.search_dirs
		  && bfd_get_error () == bfd_error_file_not_recognized
		  && ! ldemul_unrecognized_file (entry))
		{
		  int token, skip = 0;
		  char *arg, *arg1, *arg2, *arg3;
		  extern FILE *yyin;

		  /* Try to interpret the file as a linker script.  */
		  ldfile_open_command_file (attempt);

		  ldfile_assumed_script = TRUE;
		  parser_input = input_selected;
		  ldlex_both ();
		  token = INPUT_SCRIPT;
		  while (token != 0)
		    {
		      switch (token)
			{
			case OUTPUT_FORMAT:
			  if ((token = yylex ()) != '(')
			    continue;
			  if ((token = yylex ()) != NAME)
			    continue;
			  arg1 = yylval.name;
			  arg2 = NULL;
			  arg3 = NULL;
			  token = yylex ();
			  if (token == ',')
			    {
			      if ((token = yylex ()) != NAME)
				{
				  free (arg1);
				  continue;
				}
			      arg2 = yylval.name;
			      if ((token = yylex ()) != ','
				  || (token = yylex ()) != NAME)
				{
				  free (arg1);
				  free (arg2);
				  continue;
				}
			      arg3 = yylval.name;
			      token = yylex ();
			    }
			  if (token == ')')
			    {
			      switch (command_line.endian)
				{
				default:
				case ENDIAN_UNSET:
				  arg = arg1; break;
				case ENDIAN_BIG:
				  arg = arg2 ? arg2 : arg1; break;
				case ENDIAN_LITTLE:
				  arg = arg3 ? arg3 : arg1; break;
				}
			      if (strcmp (arg, lang_get_output_target ()) != 0)
				skip = 1;
			    }
			  free (arg1);
			  if (arg2) free (arg2);
			  if (arg3) free (arg3);
			  break;
			case NAME:
			case LNAME:
			case VERS_IDENTIFIER:
			case VERS_TAG:
			  free (yylval.name);
			  break;
			case INT:
			  if (yylval.bigint.str)
			    free (yylval.bigint.str);
			  break;
			}
		      token = yylex ();
		    }
		  ldlex_popstate ();
		  ldfile_assumed_script = FALSE;
		  fclose (yyin);
		  yyin = NULL;
		  if (skip)
		    {
		      if (command_line.warn_search_mismatch)
			einfo (_("%P: skipping incompatible %s "
				 "when searching for %s\n"),
			       attempt, entry->local_sym_name);
		      bfd_close (entry->the_bfd);
		      entry->the_bfd = NULL;
		      return FALSE;
		    }
		}
	      goto success;
	    }

	  if (!entry->flags.dynamic && (entry->the_bfd->flags & DYNAMIC) != 0)
	    {
	      einfo (_("%F%P: attempted static link of dynamic object `%s'\n"),
		     attempt);
	      bfd_close (entry->the_bfd);
	      entry->the_bfd = NULL;
	      return FALSE;
	    }

	  if (entry->flags.search_dirs
	      && !bfd_arch_get_compatible (check, link_info.output_bfd,
					   command_line.accept_unknown_input_arch)
	      /* XCOFF archives can have 32 and 64 bit objects.  */
	      && ! (bfd_get_flavour (check) == bfd_target_xcoff_flavour
		    && bfd_get_flavour (link_info.output_bfd) == bfd_target_xcoff_flavour
		    && bfd_check_format (entry->the_bfd, bfd_archive)))
	    {
	      if (command_line.warn_search_mismatch)
		einfo (_("%P: skipping incompatible %s "
			 "when searching for %s\n"),
		       attempt, entry->local_sym_name);
	      bfd_close (entry->the_bfd);
	      entry->the_bfd = NULL;
	      return FALSE;
	    }
	}
    }
success:
#ifdef ENABLE_PLUGINS
  /* If plugins are active, they get first chance to claim
     any successfully-opened input file.  We skip archives
     here; the plugin wants us to offer it the individual
     members when we enumerate them, not the whole file.  We
     also ignore corefiles, because that's just weird.  It is
     a needed side-effect of calling  bfd_check_format with
     bfd_object that it sets the bfd's arch and mach, which
     will be needed when and if we want to bfd_create a new
     one using this one as a template.  */
  if (bfd_check_format (entry->the_bfd, bfd_object)
      && plugin_active_plugins_p ()
      && !no_more_claiming)
    {
      int fd = open (attempt, O_RDONLY | O_BINARY);
      if (fd >= 0)
	{
	  struct ld_plugin_input_file file;

	  file.name = attempt;
	  file.offset = 0;
	  file.filesize = lseek (fd, 0, SEEK_END);
	  file.fd = fd;
	  plugin_maybe_claim (&file, entry);
	}
    }
#endif /* ENABLE_PLUGINS */

  /* It opened OK, the format checked out, and the plugins have had
     their chance to claim it, so this is success.  */
  return TRUE;
}

/* Search for and open the file specified by ENTRY.  If it is an
   archive, use ARCH, LIB and SUFFIX to modify the file name.  */

bfd_boolean
ldfile_open_file_search (const char *arch,
			 lang_input_statement_type *entry,
			 const char *lib,
			 const char *suffix)
{
  search_dirs_type *search;

  /* If this is not an archive, try to open it in the current
     directory first.  */
  if (! entry->flags.maybe_archive)
    {
      if (entry->flags.sysrooted && IS_ABSOLUTE_PATH (entry->filename))
	{
	  char *name = concat (ld_sysroot, entry->filename,
			       (const char *) NULL);
	  if (ldfile_try_open_bfd (name, entry))
	    {
	      entry->filename = name;
	      return TRUE;
	    }
	  free (name);
	}
      else if (ldfile_try_open_bfd (entry->filename, entry))
	return TRUE;

      if (IS_ABSOLUTE_PATH (entry->filename))
	return FALSE;
    }

  for (search = search_head; search != NULL; search = search->next)
    {
      char *string;

      if (entry->flags.dynamic && ! link_info.relocatable)
	{
	  if (ldemul_open_dynamic_archive (arch, search, entry))
	    return TRUE;
	}

      if (entry->flags.maybe_archive)
	string = concat (search->name, slash, lib, entry->filename,
			 arch, suffix, (const char *) NULL);
      else
	string = concat (search->name, slash, entry->filename,
			 (const char *) 0);

      if (ldfile_try_open_bfd (string, entry))
	{
	  entry->filename = string;
	  return TRUE;
	}

      free (string);
    }

  return FALSE;
}

/* Open the input file specified by ENTRY.
   PR 4437: Do not stop on the first missing file, but
   continue processing other input files in case there
   are more errors to report.  */

void
ldfile_open_file (lang_input_statement_type *entry)
{
  if (entry->the_bfd != NULL)
    return;

  if (! entry->flags.search_dirs)
    {
      if (ldfile_try_open_bfd (entry->filename, entry))
	return;

      if (filename_cmp (entry->filename, entry->local_sym_name) != 0)
	einfo (_("%P: cannot find %s (%s): %E\n"),
	       entry->filename, entry->local_sym_name);
      else
	einfo (_("%P: cannot find %s: %E\n"), entry->local_sym_name);

      entry->flags.missing_file = TRUE;
      input_flags.missing_file = TRUE;
    }
  else
    {
      search_arch_type *arch;
      bfd_boolean found = FALSE;

      /* Try to open <filename><suffix> or lib<filename><suffix>.a */
      for (arch = search_arch_head; arch != NULL; arch = arch->next)
	{
	  found = ldfile_open_file_search (arch->name, entry, "lib", ".a");
	  if (found)
	    break;
#ifdef VMS
	  found = ldfile_open_file_search (arch->name, entry, ":lib", ".a");
	  if (found)
	    break;
#endif
	  found = ldemul_find_potential_libraries (arch->name, entry);
	  if (found)
	    break;
	}

      /* If we have found the file, we don't need to search directories
	 again.  */
      if (found)
	entry->flags.search_dirs = FALSE;
      else
	{
	  if (entry->flags.sysrooted
	       && ld_sysroot
	       && IS_ABSOLUTE_PATH (entry->local_sym_name))
	    einfo (_("%P: cannot find %s inside %s\n"),
		   entry->local_sym_name, ld_sysroot);
	  else
	    einfo (_("%P: cannot find %s\n"), entry->local_sym_name);
	  entry->flags.missing_file = TRUE;
	  input_flags.missing_file = TRUE;
	}
    }
}

/* Try to open NAME.  */

static FILE *
try_open (const char *name, bfd_boolean *sysrooted)
{
  FILE *result;

  result = fopen (name, "r");

  if (result != NULL)
    *sysrooted = is_sysrooted_pathname (name);

  if (verbose)
    {
      if (result == NULL)
	info_msg (_("cannot find script file %s\n"), name);
      else
	info_msg (_("opened script file %s\n"), name);
    }

  return result;
}

/* Return TRUE iff directory DIR contains an "ldscripts" subdirectory.  */

static bfd_boolean
check_for_scripts_dir (char *dir)
{
  char *buf;
  struct stat s;
  bfd_boolean res;

  buf = concat (dir, "/ldscripts", (const char *) NULL);
  res = stat (buf, &s) == 0 && S_ISDIR (s.st_mode);
  free (buf);
  return res;
}

/* Return the default directory for finding script files.
   We look for the "ldscripts" directory in:

   SCRIPTDIR (passed from Makefile)
	     (adjusted according to the current location of the binary)
   the dir where this program is (for using it from the build tree).  */

static char *
find_scripts_dir (void)
{
  char *dir;

  dir = make_relative_prefix (program_name, BINDIR, SCRIPTDIR);
  if (dir)
    {
      if (check_for_scripts_dir (dir))
	return dir;
      free (dir);
    }

  dir = make_relative_prefix (program_name, TOOLBINDIR, SCRIPTDIR);
  if (dir)
    {
      if (check_for_scripts_dir (dir))
	return dir;
      free (dir);
    }

  /* Look for "ldscripts" in the dir where our binary is.  */
  dir = make_relative_prefix (program_name, ".", ".");
  if (dir)
    {
      if (check_for_scripts_dir (dir))
	return dir;
      free (dir);
    }

  return NULL;
}

/* If DEFAULT_ONLY is false, try to open NAME; if that fails, look for
   it in directories specified with -L, then in the default script
   directory.  If DEFAULT_ONLY is true, the search is restricted to
   the default script location.  */

static FILE *
ldfile_find_command_file (const char *name,
			  bfd_boolean default_only,
			  bfd_boolean *sysrooted)
{
  search_dirs_type *search;
  FILE *result = NULL;
  char *path;
  static search_dirs_type *script_search;

  if (!default_only)
    {
      /* First try raw name.  */
      result = try_open (name, sysrooted);
      if (result != NULL)
	return result;
    }

  if (!script_search)
    {
      char *script_dir = find_scripts_dir ();
      if (script_dir)
	{
	  search_dirs_type **save_tail_ptr = search_tail_ptr;
	  search_tail_ptr = &script_search;
	  ldfile_add_library_path (script_dir, TRUE);
	  search_tail_ptr = save_tail_ptr;
	}
    }

  /* Temporarily append script_search to the path list so that the
     paths specified with -L will be searched first.  */
  *search_tail_ptr = script_search;

  /* Try now prefixes.  */
  for (search = default_only ? script_search : search_head;
       search != NULL;
       search = search->next)
    {
      path = concat (search->name, slash, name, (const char *) NULL);
      result = try_open (path, sysrooted);
      free (path);
      if (result)
	break;
    }

  /* Restore the original path list.  */
  *search_tail_ptr = NULL;

  return result;
}

/* Open command file NAME.  */

static void
ldfile_open_command_file_1 (const char *name, bfd_boolean default_only)
{
  FILE *ldlex_input_stack;
  bfd_boolean sysrooted;

  ldlex_input_stack = ldfile_find_command_file (name, default_only, &sysrooted);

  if (ldlex_input_stack == NULL)
    {
      bfd_set_error (bfd_error_system_call);
      einfo (_("%P%F: cannot open linker script file %s: %E\n"), name);
    }

  lex_push_file (ldlex_input_stack, name, sysrooted);

  lineno = 1;

  saved_script_handle = ldlex_input_stack;
}

/* Open command file NAME in the current directory, -L directories,
   the default script location, in that order.  */

void
ldfile_open_command_file (const char *name)
{
  ldfile_open_command_file_1 (name, FALSE);
}

/* Open command file NAME at the default script location.  */

void
ldfile_open_default_command_file (const char *name)
{
  ldfile_open_command_file_1 (name, TRUE);
}

void
ldfile_add_arch (const char *in_name)
{
  char *name = xstrdup (in_name);
  search_arch_type *new_arch = (search_arch_type *)
      xmalloc (sizeof (search_arch_type));

  ldfile_output_machine_name = in_name;

  new_arch->name = name;
  new_arch->next = NULL;
  while (*name)
    {
      *name = TOLOWER (*name);
      name++;
    }
  *search_arch_tail_ptr = new_arch;
  search_arch_tail_ptr = &new_arch->next;

}

/* Set the output architecture.  */

void
ldfile_set_output_arch (const char *string, enum bfd_architecture defarch)
{
  const bfd_arch_info_type *arch = bfd_scan_arch (string);

  if (arch)
    {
      ldfile_output_architecture = arch->arch;
      ldfile_output_machine = arch->mach;
      ldfile_output_machine_name = arch->printable_name;
    }
  else if (defarch != bfd_arch_unknown)
    ldfile_output_architecture = defarch;
  else
    einfo (_("%P%F: cannot represent machine `%s'\n"), string);
}
