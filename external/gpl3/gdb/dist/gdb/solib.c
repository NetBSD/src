/* Handle shared libraries for GDB, the GNU Debugger.

   Copyright (C) 1990-2014 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "exceptions.h"
#include "gdbcore.h"
#include "command.h"
#include "target.h"
#include "frame.h"
#include "gdb_regex.h"
#include "inferior.h"
#include "environ.h"
#include "language.h"
#include "gdbcmd.h"
#include "completer.h"
#include "filenames.h"		/* for DOSish file names */
#include "exec.h"
#include "solist.h"
#include "observer.h"
#include "readline/readline.h"
#include "remote.h"
#include "solib.h"
#include "interps.h"
#include "filesystem.h"
#include "gdb_bfd.h"
#include "filestuff.h"

/* Architecture-specific operations.  */

/* Per-architecture data key.  */
static struct gdbarch_data *solib_data;

static void *
solib_init (struct obstack *obstack)
{
  struct target_so_ops **ops;

  ops = OBSTACK_ZALLOC (obstack, struct target_so_ops *);
  *ops = current_target_so_ops;
  return ops;
}

static const struct target_so_ops *
solib_ops (struct gdbarch *gdbarch)
{
  const struct target_so_ops **ops = gdbarch_data (gdbarch, solib_data);

  return *ops;
}

/* Set the solib operations for GDBARCH to NEW_OPS.  */

void
set_solib_ops (struct gdbarch *gdbarch, const struct target_so_ops *new_ops)
{
  const struct target_so_ops **ops = gdbarch_data (gdbarch, solib_data);

  *ops = new_ops;
}


/* external data declarations */

/* FIXME: gdbarch needs to control this variable, or else every
   configuration needs to call set_solib_ops.  */
struct target_so_ops *current_target_so_ops;

/* List of known shared objects */
#define so_list_head current_program_space->so_list

/* Local function prototypes */

/* If non-empty, this is a search path for loading non-absolute shared library
   symbol files.  This takes precedence over the environment variables PATH
   and LD_LIBRARY_PATH.  */
static char *solib_search_path = NULL;
static void
show_solib_search_path (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The search path for loading non-absolute "
			    "shared library symbol files is %s.\n"),
		    value);
}

/* Same as HAVE_DOS_BASED_FILE_SYSTEM, but useable as an rvalue.  */
#if (HAVE_DOS_BASED_FILE_SYSTEM)
#  define DOS_BASED_FILE_SYSTEM 1
#else
#  define DOS_BASED_FILE_SYSTEM 0
#endif

/* Returns the full pathname of the shared library file, or NULL if
   not found.  (The pathname is malloc'ed; it needs to be freed by the
   caller.)  *FD is set to either -1 or an open file handle for the
   library.

   Global variable GDB_SYSROOT is used as a prefix directory
   to search for shared libraries if they have an absolute path.

   Global variable SOLIB_SEARCH_PATH is used as a prefix directory
   (or set of directories, as in LD_LIBRARY_PATH) to search for all
   shared libraries if not found in GDB_SYSROOT.

   Search algorithm:
   * If there is a gdb_sysroot and path is absolute:
   *   Search for gdb_sysroot/path.
   * else
   *   Look for it literally (unmodified).
   * Look in SOLIB_SEARCH_PATH.
   * If available, use target defined search function.
   * If gdb_sysroot is NOT set, perform the following two searches:
   *   Look in inferior's $PATH.
   *   Look in inferior's $LD_LIBRARY_PATH.
   *
   * The last check avoids doing this search when targetting remote
   * machines since gdb_sysroot will almost always be set.
*/

char *
solib_find (char *in_pathname, int *fd)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());
  int found_file = -1;
  char *temp_pathname = NULL;
  int gdb_sysroot_is_empty;
  const char *solib_symbols_extension
    = gdbarch_solib_symbols_extension (target_gdbarch ());
  const char *fskind = effective_target_file_system_kind ();
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  char *sysroot = NULL;

  /* If solib_symbols_extension is set, replace the file's
     extension.  */
  if (solib_symbols_extension)
    {
      char *p = in_pathname + strlen (in_pathname);

      while (p > in_pathname && *p != '.')
	p--;

      if (*p == '.')
	{
	  char *new_pathname;

	  new_pathname = alloca (p - in_pathname + 1
				 + strlen (solib_symbols_extension) + 1);
	  memcpy (new_pathname, in_pathname, p - in_pathname + 1);
	  strcpy (new_pathname + (p - in_pathname) + 1,
		  solib_symbols_extension);

	  in_pathname = new_pathname;
	}
    }

  gdb_sysroot_is_empty = (gdb_sysroot == NULL || *gdb_sysroot == 0);

  if (!gdb_sysroot_is_empty)
    {
      int prefix_len = strlen (gdb_sysroot);

      /* Remove trailing slashes from absolute prefix.  */
      while (prefix_len > 0
	     && IS_DIR_SEPARATOR (gdb_sysroot[prefix_len - 1]))
	prefix_len--;

      sysroot = savestring (gdb_sysroot, prefix_len);
      make_cleanup (xfree, sysroot);
    }

  /* If we're on a non-DOS-based system, backslashes won't be
     understood as directory separator, so, convert them to forward
     slashes, iff we're supposed to handle DOS-based file system
     semantics for target paths.  */
  if (!DOS_BASED_FILE_SYSTEM && fskind == file_system_kind_dos_based)
    {
      char *p;

      /* Avoid clobbering our input.  */
      p = alloca (strlen (in_pathname) + 1);
      strcpy (p, in_pathname);
      in_pathname = p;

      for (; *p; p++)
	{
	  if (*p == '\\')
	    *p = '/';
	}
    }

  /* Note, we're interested in IS_TARGET_ABSOLUTE_PATH, not
     IS_ABSOLUTE_PATH.  The latter is for host paths only, while
     IN_PATHNAME is a target path.  For example, if we're supposed to
     be handling DOS-like semantics we want to consider a
     'c:/foo/bar.dll' path as an absolute path, even on a Unix box.
     With such a path, before giving up on the sysroot, we'll try:

       1st attempt, c:/foo/bar.dll ==> /sysroot/c:/foo/bar.dll
       2nd attempt, c:/foo/bar.dll ==> /sysroot/c/foo/bar.dll
       3rd attempt, c:/foo/bar.dll ==> /sysroot/foo/bar.dll
  */

  if (!IS_TARGET_ABSOLUTE_PATH (fskind, in_pathname) || gdb_sysroot_is_empty)
    temp_pathname = xstrdup (in_pathname);
  else
    {
      int need_dir_separator;

      /* Concatenate the sysroot and the target reported filename.  We
	 may need to glue them with a directory separator.  Cases to
	 consider:

        | sysroot         | separator | in_pathname    |
        |-----------------+-----------+----------------|
        | /some/dir       | /         | c:/foo/bar.dll |
        | /some/dir       |           | /foo/bar.dll   |
        | remote:         |           | c:/foo/bar.dll |
        | remote:         |           | /foo/bar.dll   |
        | remote:some/dir | /         | c:/foo/bar.dll |
        | remote:some/dir |           | /foo/bar.dll   |

	IOW, we don't need to add a separator if IN_PATHNAME already
	has one, or when the the sysroot is exactly "remote:".
	There's no need to check for drive spec explicitly, as we only
	get here if IN_PATHNAME is considered an absolute path.  */
      need_dir_separator = !(IS_DIR_SEPARATOR (in_pathname[0])
			     || strcmp (REMOTE_SYSROOT_PREFIX, sysroot) == 0);

      /* Cat the prefixed pathname together.  */
      temp_pathname = concat (sysroot,
			      need_dir_separator ? SLASH_STRING : "",
			      in_pathname, (char *) NULL);
    }

  /* Handle remote files.  */
  if (remote_filename_p (temp_pathname))
    {
      *fd = -1;
      do_cleanups (old_chain);
      return temp_pathname;
    }

  /* Now see if we can open it.  */
  found_file = gdb_open_cloexec (temp_pathname, O_RDONLY | O_BINARY, 0);
  if (found_file < 0)
    xfree (temp_pathname);

  /* If the search in gdb_sysroot failed, and the path name has a
     drive spec (e.g, c:/foo), try stripping ':' from the drive spec,
     and retrying in the sysroot:
       c:/foo/bar.dll ==> /sysroot/c/foo/bar.dll.  */

  if (found_file < 0
      && !gdb_sysroot_is_empty
      && HAS_TARGET_DRIVE_SPEC (fskind, in_pathname))
    {
      int need_dir_separator = !IS_DIR_SEPARATOR (in_pathname[2]);
      char *drive = savestring (in_pathname, 1);

      temp_pathname = concat (sysroot,
			      SLASH_STRING,
			      drive,
			      need_dir_separator ? SLASH_STRING : "",
			      in_pathname + 2, (char *) NULL);
      xfree (drive);

      found_file = gdb_open_cloexec (temp_pathname, O_RDONLY | O_BINARY, 0);
      if (found_file < 0)
	{
	  xfree (temp_pathname);

	  /* If the search in gdb_sysroot still failed, try fully
	     stripping the drive spec, and trying once more in the
	     sysroot before giving up.

	     c:/foo/bar.dll ==> /sysroot/foo/bar.dll.  */

	  temp_pathname = concat (sysroot,
				  need_dir_separator ? SLASH_STRING : "",
				  in_pathname + 2, (char *) NULL);

	  found_file = gdb_open_cloexec (temp_pathname, O_RDONLY | O_BINARY, 0);
	  if (found_file < 0)
	    xfree (temp_pathname);
	}
    }

  do_cleanups (old_chain);

  /* We try to find the library in various ways.  After each attempt,
     either found_file >= 0 and temp_pathname is a malloc'd string, or
     found_file < 0 and temp_pathname does not point to storage that
     needs to be freed.  */

  if (found_file < 0)
    temp_pathname = NULL;

  /* If the search in gdb_sysroot failed, and the path name is
     absolute at this point, make it relative.  (openp will try and open the
     file according to its absolute path otherwise, which is not what we want.)
     Affects subsequent searches for this solib.  */
  if (found_file < 0 && IS_TARGET_ABSOLUTE_PATH (fskind, in_pathname))
    {
      /* First, get rid of any drive letters etc.  */
      while (!IS_TARGET_DIR_SEPARATOR (fskind, *in_pathname))
	in_pathname++;

      /* Next, get rid of all leading dir separators.  */
      while (IS_TARGET_DIR_SEPARATOR (fskind, *in_pathname))
	in_pathname++;
    }

  /* If not found, search the solib_search_path (if any).  */
  if (found_file < 0 && solib_search_path != NULL)
    found_file = openp (solib_search_path,
			OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH,
			in_pathname, O_RDONLY | O_BINARY, &temp_pathname);

  /* If not found, next search the solib_search_path (if any) for the basename
     only (ignoring the path).  This is to allow reading solibs from a path
     that differs from the opened path.  */
  if (found_file < 0 && solib_search_path != NULL)
    found_file = openp (solib_search_path,
			OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH,
			target_lbasename (fskind, in_pathname),
			O_RDONLY | O_BINARY, &temp_pathname);

  /* If not found, try to use target supplied solib search method.  */
  if (found_file < 0 && ops->find_and_open_solib)
    found_file = ops->find_and_open_solib (in_pathname, O_RDONLY | O_BINARY,
					   &temp_pathname);

  /* If not found, next search the inferior's $PATH environment variable.  */
  if (found_file < 0 && gdb_sysroot_is_empty)
    found_file = openp (get_in_environ (current_inferior ()->environment,
					"PATH"),
			OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH, in_pathname,
			O_RDONLY | O_BINARY, &temp_pathname);

  /* If not found, next search the inferior's $LD_LIBRARY_PATH
     environment variable.  */
  if (found_file < 0 && gdb_sysroot_is_empty)
    found_file = openp (get_in_environ (current_inferior ()->environment,
					"LD_LIBRARY_PATH"),
			OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH, in_pathname,
			O_RDONLY | O_BINARY, &temp_pathname);

  *fd = found_file;
  return temp_pathname;
}

/* Open and return a BFD for the shared library PATHNAME.  If FD is not -1,
   it is used as file handle to open the file.  Throws an error if the file
   could not be opened.  Handles both local and remote file access.

   PATHNAME must be malloc'ed by the caller.  It will be freed by this
   function.  If unsuccessful, the FD will be closed (unless FD was
   -1).  */

bfd *
solib_bfd_fopen (char *pathname, int fd)
{
  bfd *abfd;

  if (remote_filename_p (pathname))
    {
      gdb_assert (fd == -1);
      abfd = remote_bfd_open (pathname, gnutarget);
    }
  else
    {
      abfd = gdb_bfd_open (pathname, gnutarget, fd);

      if (abfd)
	bfd_set_cacheable (abfd, 1);
    }

  if (!abfd)
    {
      make_cleanup (xfree, pathname);
      error (_("Could not open `%s' as an executable file: %s"),
	     pathname, bfd_errmsg (bfd_get_error ()));
    }

  xfree (pathname);

  return abfd;
}

/* Find shared library PATHNAME and open a BFD for it.  */

bfd *
solib_bfd_open (char *pathname)
{
  char *found_pathname;
  int found_file;
  bfd *abfd;
  const struct bfd_arch_info *b;

  /* Search for shared library file.  */
  found_pathname = solib_find (pathname, &found_file);
  if (found_pathname == NULL)
    {
      /* Return failure if the file could not be found, so that we can
	 accumulate messages about missing libraries.  */
      if (errno == ENOENT)
	return NULL;

      perror_with_name (pathname);
    }

  /* Open bfd for shared library.  */
  abfd = solib_bfd_fopen (found_pathname, found_file);

  /* Check bfd format.  */
  if (!bfd_check_format (abfd, bfd_object))
    {
      make_cleanup_bfd_unref (abfd);
      error (_("`%s': not in executable format: %s"),
	     bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
    }

  /* Check bfd arch.  */
  b = gdbarch_bfd_arch_info (target_gdbarch ());
  if (!b->compatible (b, bfd_get_arch_info (abfd)))
    warning (_("`%s': Shared library architecture %s is not compatible "
               "with target architecture %s."), bfd_get_filename (abfd),
             bfd_get_arch_info (abfd)->printable_name, b->printable_name);

  return abfd;
}

/* Given a pointer to one of the shared objects in our list of mapped
   objects, use the recorded name to open a bfd descriptor for the
   object, build a section table, relocate all the section addresses
   by the base address at which the shared object was mapped, and then
   add the sections to the target's section table.

   FIXME: In most (all?) cases the shared object file name recorded in
   the dynamic linkage tables will be a fully qualified pathname.  For
   cases where it isn't, do we really mimic the systems search
   mechanism correctly in the below code (particularly the tilde
   expansion stuff?).  */

static int
solib_map_sections (struct so_list *so)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());
  char *filename;
  struct target_section *p;
  struct cleanup *old_chain;
  bfd *abfd;

  filename = tilde_expand (so->so_name);
  old_chain = make_cleanup (xfree, filename);
  abfd = ops->bfd_open (filename);
  do_cleanups (old_chain);

  if (abfd == NULL)
    return 0;

  /* Leave bfd open, core_xfer_memory and "info files" need it.  */
  so->abfd = abfd;

  /* Copy the full path name into so_name, allowing symbol_file_add
     to find it later.  This also affects the =library-loaded GDB/MI
     event, and in particular the part of that notification providing
     the library's host-side path.  If we let the target dictate
     that objfile's path, and the target is different from the host,
     GDB/MI will not provide the correct host-side path.  */
  if (strlen (bfd_get_filename (abfd)) >= SO_NAME_MAX_PATH_SIZE)
    error (_("Shared library file name is too long."));
  strcpy (so->so_name, bfd_get_filename (abfd));

  if (build_section_table (abfd, &so->sections, &so->sections_end))
    {
      error (_("Can't find the file sections in `%s': %s"),
	     bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
    }

  for (p = so->sections; p < so->sections_end; p++)
    {
      /* Relocate the section binding addresses as recorded in the shared
         object's file by the base address to which the object was actually
         mapped.  */
      ops->relocate_section_addresses (so, p);

      /* If the target didn't provide information about the address
	 range of the shared object, assume we want the location of
	 the .text section.  */
      if (so->addr_low == 0 && so->addr_high == 0
	  && strcmp (p->the_bfd_section->name, ".text") == 0)
	{
	  so->addr_low = p->addr;
	  so->addr_high = p->endaddr;
	}
    }

  /* Add the shared object's sections to the current set of file
     section tables.  Do this immediately after mapping the object so
     that later nodes in the list can query this object, as is needed
     in solib-osf.c.  */
  add_target_sections (so, so->sections, so->sections_end);

  return 1;
}

/* Free symbol-file related contents of SO and reset for possible reloading
   of SO.  If we have opened a BFD for SO, close it.  If we have placed SO's
   sections in some target's section table, the caller is responsible for
   removing them.

   This function doesn't mess with objfiles at all.  If there is an
   objfile associated with SO that needs to be removed, the caller is
   responsible for taking care of that.  */

static void
clear_so (struct so_list *so)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  if (so->sections)
    {
      xfree (so->sections);
      so->sections = so->sections_end = NULL;
    }

  gdb_bfd_unref (so->abfd);
  so->abfd = NULL;

  /* Our caller closed the objfile, possibly via objfile_purge_solibs.  */
  so->symbols_loaded = 0;
  so->objfile = NULL;

  so->addr_low = so->addr_high = 0;

  /* Restore the target-supplied file name.  SO_NAME may be the path
     of the symbol file.  */
  strcpy (so->so_name, so->so_original_name);

  /* Do the same for target-specific data.  */
  if (ops->clear_so != NULL)
    ops->clear_so (so);
}

/* Free the storage associated with the `struct so_list' object SO.
   If we have opened a BFD for SO, close it.

   The caller is responsible for removing SO from whatever list it is
   a member of.  If we have placed SO's sections in some target's
   section table, the caller is responsible for removing them.

   This function doesn't mess with objfiles at all.  If there is an
   objfile associated with SO that needs to be removed, the caller is
   responsible for taking care of that.  */

void
free_so (struct so_list *so)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  clear_so (so);
  ops->free_so (so);

  xfree (so);
}


/* Return address of first so_list entry in master shared object list.  */
struct so_list *
master_so_list (void)
{
  return so_list_head;
}

/* Read in symbols for shared object SO.  If SYMFILE_VERBOSE is set in FLAGS,
   be chatty about it.  Return non-zero if any symbols were actually
   loaded.  */

int
solib_read_symbols (struct so_list *so, int flags)
{
  const int from_tty = flags & SYMFILE_VERBOSE;

  if (so->symbols_loaded)
    {
      /* If needed, we've already warned in our caller.  */
    }
  else if (so->abfd == NULL)
    {
      /* We've already warned about this library, when trying to open
	 it.  */
    }
  else
    {
      volatile struct gdb_exception e;

      flags |= current_inferior ()->symfile_flags;

      TRY_CATCH (e, RETURN_MASK_ERROR)
	{
	  struct section_addr_info *sap;

	  /* Have we already loaded this shared object?  */
	  ALL_OBJFILES (so->objfile)
	    {
	      if (filename_cmp (objfile_name (so->objfile), so->so_name) == 0
		  && so->objfile->addr_low == so->addr_low)
		break;
	    }
	  if (so->objfile != NULL)
	    break;

	  sap = build_section_addr_info_from_section_table (so->sections,
							    so->sections_end);
	  so->objfile = symbol_file_add_from_bfd (so->abfd, so->so_name,
						  flags, sap, OBJF_SHARED,
						  NULL);
	  so->objfile->addr_low = so->addr_low;
	  free_section_addr_info (sap);
	}

      if (e.reason < 0)
	exception_fprintf (gdb_stderr, e, _("Error while reading shared"
					    " library symbols for %s:\n"),
			   so->so_name);
      else
	{
	  if (from_tty || info_verbose)
	    printf_unfiltered (_("Loaded symbols for %s\n"), so->so_name);
	  so->symbols_loaded = 1;
	}
      return 1;
    }

  return 0;
}

/* Return 1 if KNOWN->objfile is used by any other so_list object in the
   SO_LIST_HEAD list.  Return 0 otherwise.  */

static int
solib_used (const struct so_list *const known)
{
  const struct so_list *pivot;

  for (pivot = so_list_head; pivot != NULL; pivot = pivot->next)
    if (pivot != known && pivot->objfile == known->objfile)
      return 1;
  return 0;
}

/* Synchronize GDB's shared object list with inferior's.

   Extract the list of currently loaded shared objects from the
   inferior, and compare it with the list of shared objects currently
   in GDB's so_list_head list.  Edit so_list_head to bring it in sync
   with the inferior's new list.

   If we notice that the inferior has unloaded some shared objects,
   free any symbolic info GDB had read about those shared objects.

   Don't load symbolic info for any new shared objects; just add them
   to the list, and leave their symbols_loaded flag clear.

   If FROM_TTY is non-null, feel free to print messages about what
   we're doing.

   If TARGET is non-null, add the sections of all new shared objects
   to TARGET's section table.  Note that this doesn't remove any
   sections for shared objects that have been unloaded, and it
   doesn't check to see if the new shared objects are already present in
   the section table.  But we only use this for core files and
   processes we've just attached to, so that's okay.  */

static void
update_solib_list (int from_tty, struct target_ops *target)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());
  struct so_list *inferior = ops->current_sos();
  struct so_list *gdb, **gdb_link;

  /* We can reach here due to changing solib-search-path or the
     sysroot, before having any inferior.  */
  if (target_has_execution && !ptid_equal (inferior_ptid, null_ptid))
    {
      struct inferior *inf = current_inferior ();

      /* If we are attaching to a running process for which we
	 have not opened a symbol file, we may be able to get its
	 symbols now!  */
      if (inf->attach_flag && symfile_objfile == NULL)
	catch_errors (ops->open_symbol_file_object, &from_tty,
		      "Error reading attached process's symbol file.\n",
		      RETURN_MASK_ALL);
    }

  /* GDB and the inferior's dynamic linker each maintain their own
     list of currently loaded shared objects; we want to bring the
     former in sync with the latter.  Scan both lists, seeing which
     shared objects appear where.  There are three cases:

     - A shared object appears on both lists.  This means that GDB
     knows about it already, and it's still loaded in the inferior.
     Nothing needs to happen.

     - A shared object appears only on GDB's list.  This means that
     the inferior has unloaded it.  We should remove the shared
     object from GDB's tables.

     - A shared object appears only on the inferior's list.  This
     means that it's just been loaded.  We should add it to GDB's
     tables.

     So we walk GDB's list, checking each entry to see if it appears
     in the inferior's list too.  If it does, no action is needed, and
     we remove it from the inferior's list.  If it doesn't, the
     inferior has unloaded it, and we remove it from GDB's list.  By
     the time we're done walking GDB's list, the inferior's list
     contains only the new shared objects, which we then add.  */

  gdb = so_list_head;
  gdb_link = &so_list_head;
  while (gdb)
    {
      struct so_list *i = inferior;
      struct so_list **i_link = &inferior;

      /* Check to see whether the shared object *gdb also appears in
	 the inferior's current list.  */
      while (i)
	{
	  if (ops->same)
	    {
	      if (ops->same (gdb, i))
		break;
	    }
	  else
	    {
	      if (! filename_cmp (gdb->so_original_name, i->so_original_name))
		break;	      
	    }

	  i_link = &i->next;
	  i = *i_link;
	}

      /* If the shared object appears on the inferior's list too, then
         it's still loaded, so we don't need to do anything.  Delete
         it from the inferior's list, and leave it on GDB's list.  */
      if (i)
	{
	  *i_link = i->next;
	  free_so (i);
	  gdb_link = &gdb->next;
	  gdb = *gdb_link;
	}

      /* If it's not on the inferior's list, remove it from GDB's tables.  */
      else
	{
	  /* Notify any observer that the shared object has been
	     unloaded before we remove it from GDB's tables.  */
	  observer_notify_solib_unloaded (gdb);

	  VEC_safe_push (char_ptr, current_program_space->deleted_solibs,
			 xstrdup (gdb->so_name));

	  *gdb_link = gdb->next;

	  /* Unless the user loaded it explicitly, free SO's objfile.  */
	  if (gdb->objfile && ! (gdb->objfile->flags & OBJF_USERLOADED)
	      && !solib_used (gdb))
	    free_objfile (gdb->objfile);

	  /* Some targets' section tables might be referring to
	     sections from so->abfd; remove them.  */
	  remove_target_sections (gdb);

	  free_so (gdb);
	  gdb = *gdb_link;
	}
    }

  /* Now the inferior's list contains only shared objects that don't
     appear in GDB's list --- those that are newly loaded.  Add them
     to GDB's shared object list.  */
  if (inferior)
    {
      int not_found = 0;
      const char *not_found_filename = NULL;

      struct so_list *i;

      /* Add the new shared objects to GDB's list.  */
      *gdb_link = inferior;

      /* Fill in the rest of each of the `struct so_list' nodes.  */
      for (i = inferior; i; i = i->next)
	{
	  volatile struct gdb_exception e;

	  i->pspace = current_program_space;
	  VEC_safe_push (so_list_ptr, current_program_space->added_solibs, i);

	  TRY_CATCH (e, RETURN_MASK_ERROR)
	    {
	      /* Fill in the rest of the `struct so_list' node.  */
	      if (!solib_map_sections (i))
		{
		  not_found++;
		  if (not_found_filename == NULL)
		    not_found_filename = i->so_original_name;
		}
	    }

	  if (e.reason < 0)
	    exception_fprintf (gdb_stderr, e,
			       _("Error while mapping shared "
				 "library sections:\n"));

	  /* Notify any observer that the shared object has been
	     loaded now that we've added it to GDB's tables.  */
	  observer_notify_solib_loaded (i);
	}

      /* If a library was not found, issue an appropriate warning
	 message.  We have to use a single call to warning in case the
	 front end does something special with warnings, e.g., pop up
	 a dialog box.  It Would Be Nice if we could get a "warning: "
	 prefix on each line in the CLI front end, though - it doesn't
	 stand out well.  */

      if (not_found == 1)
	warning (_("Could not load shared library symbols for %s.\n"
		   "Do you need \"set solib-search-path\" "
		   "or \"set sysroot\"?"),
		 not_found_filename);
      else if (not_found > 1)
	warning (_("\
Could not load shared library symbols for %d libraries, e.g. %s.\n\
Use the \"info sharedlibrary\" command to see the complete listing.\n\
Do you need \"set solib-search-path\" or \"set sysroot\"?"),
		 not_found, not_found_filename);
    }
}


/* Return non-zero if NAME is the libpthread shared library.

   Uses a fairly simplistic heuristic approach where we check
   the file name against "/libpthread".  This can lead to false
   positives, but this should be good enough in practice.  */

int
libpthread_name_p (const char *name)
{
  return (strstr (name, "/libpthread") != NULL);
}

/* Return non-zero if SO is the libpthread shared library.  */

static int
libpthread_solib_p (struct so_list *so)
{
  return libpthread_name_p (so->so_name);
}

/* Read in symbolic information for any shared objects whose names
   match PATTERN.  (If we've already read a shared object's symbol
   info, leave it alone.)  If PATTERN is zero, read them all.

   If READSYMS is 0, defer reading symbolic information until later
   but still do any needed low level processing.

   FROM_TTY and TARGET are as described for update_solib_list, above.  */

void
solib_add (char *pattern, int from_tty,
	   struct target_ops *target, int readsyms)
{
  struct so_list *gdb;

  current_program_space->solib_add_generation++;

  if (pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error (_("Invalid regexp: %s"), re_err);
    }

  update_solib_list (from_tty, target);

  /* Walk the list of currently loaded shared libraries, and read
     symbols for any that match the pattern --- or any whose symbols
     aren't already loaded, if no pattern was given.  */
  {
    int any_matches = 0;
    int loaded_any_symbols = 0;
    const int flags =
        SYMFILE_DEFER_BP_RESET | (from_tty ? SYMFILE_VERBOSE : 0);

    for (gdb = so_list_head; gdb; gdb = gdb->next)
      if (! pattern || re_exec (gdb->so_name))
	{
          /* Normally, we would read the symbols from that library
             only if READSYMS is set.  However, we're making a small
             exception for the pthread library, because we sometimes
             need the library symbols to be loaded in order to provide
             thread support (x86-linux for instance).  */
          const int add_this_solib =
            (readsyms || libpthread_solib_p (gdb));

	  any_matches = 1;
	  if (add_this_solib)
	    {
	      if (gdb->symbols_loaded)
		{
		  /* If no pattern was given, be quiet for shared
		     libraries we have already loaded.  */
		  if (pattern && (from_tty || info_verbose))
		    printf_unfiltered (_("Symbols already loaded for %s\n"),
				       gdb->so_name);
		}
	      else if (solib_read_symbols (gdb, flags))
		loaded_any_symbols = 1;
	    }
	}

    if (loaded_any_symbols)
      breakpoint_re_set ();

    if (from_tty && pattern && ! any_matches)
      printf_unfiltered
	("No loaded shared libraries match the pattern `%s'.\n", pattern);

    if (loaded_any_symbols)
      {
	const struct target_so_ops *ops = solib_ops (target_gdbarch ());

	/* Getting new symbols may change our opinion about what is
	   frameless.  */
	reinit_frame_cache ();

	ops->special_symbol_handling ();
      }
  }
}

/* Implement the "info sharedlibrary" command.  Walk through the
   shared library list and print information about each attached
   library matching PATTERN.  If PATTERN is elided, print them
   all.  */

static void
info_sharedlibrary_command (char *pattern, int from_tty)
{
  struct so_list *so = NULL;	/* link map state variable */
  int so_missing_debug_info = 0;
  int addr_width;
  int nr_libs;
  struct cleanup *table_cleanup;
  struct gdbarch *gdbarch = target_gdbarch ();
  struct ui_out *uiout = current_uiout;

  if (pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error (_("Invalid regexp: %s"), re_err);
    }

  /* "0x", a little whitespace, and two hex digits per byte of pointers.  */
  addr_width = 4 + (gdbarch_ptr_bit (gdbarch) / 4);

  update_solib_list (from_tty, 0);

  /* make_cleanup_ui_out_table_begin_end needs to know the number of
     rows, so we need to make two passes over the libs.  */

  for (nr_libs = 0, so = so_list_head; so; so = so->next)
    {
      if (so->so_name[0])
	{
	  if (pattern && ! re_exec (so->so_name))
	    continue;
	  ++nr_libs;
	}
    }

  table_cleanup =
    make_cleanup_ui_out_table_begin_end (uiout, 4, nr_libs,
					 "SharedLibraryTable");

  /* The "- 1" is because ui_out adds one space between columns.  */
  ui_out_table_header (uiout, addr_width - 1, ui_left, "from", "From");
  ui_out_table_header (uiout, addr_width - 1, ui_left, "to", "To");
  ui_out_table_header (uiout, 12 - 1, ui_left, "syms-read", "Syms Read");
  ui_out_table_header (uiout, 0, ui_noalign,
		       "name", "Shared Object Library");

  ui_out_table_body (uiout);

  for (so = so_list_head; so; so = so->next)
    {
      struct cleanup *lib_cleanup;

      if (! so->so_name[0])
	continue;
      if (pattern && ! re_exec (so->so_name))
	continue;

      lib_cleanup = make_cleanup_ui_out_tuple_begin_end (uiout, "lib");

      if (so->addr_high != 0)
	{
	  ui_out_field_core_addr (uiout, "from", gdbarch, so->addr_low);
	  ui_out_field_core_addr (uiout, "to", gdbarch, so->addr_high);
	}
      else
	{
	  ui_out_field_skip (uiout, "from");
	  ui_out_field_skip (uiout, "to");
	}

      if (! ui_out_is_mi_like_p (interp_ui_out (top_level_interpreter ()))
	  && so->symbols_loaded
	  && !objfile_has_symbols (so->objfile))
	{
	  so_missing_debug_info = 1;
	  ui_out_field_string (uiout, "syms-read", "Yes (*)");
	}
      else
	ui_out_field_string (uiout, "syms-read", 
			     so->symbols_loaded ? "Yes" : "No");

      ui_out_field_string (uiout, "name", so->so_name);

      ui_out_text (uiout, "\n");

      do_cleanups (lib_cleanup);
    }

  do_cleanups (table_cleanup);

  if (nr_libs == 0)
    {
      if (pattern)
	ui_out_message (uiout, 0,
			_("No shared libraries matched.\n"));
      else
	ui_out_message (uiout, 0,
			_("No shared libraries loaded at this time.\n"));
    }
  else
    {
      if (so_missing_debug_info)
	ui_out_message (uiout, 0,
			_("(*): Shared library is missing "
			  "debugging information.\n"));
    }
}

/* Return 1 if ADDRESS lies within SOLIB.  */

int
solib_contains_address_p (const struct so_list *const solib,
			  CORE_ADDR address)
{
  struct target_section *p;

  for (p = solib->sections; p < solib->sections_end; p++)
    if (p->addr <= address && address < p->endaddr)
      return 1;

  return 0;
}

/* If ADDRESS is in a shared lib in program space PSPACE, return its
   name.

   Provides a hook for other gdb routines to discover whether or not a
   particular address is within the mapped address space of a shared
   library.

   For example, this routine is called at one point to disable
   breakpoints which are in shared libraries that are not currently
   mapped in.  */

char *
solib_name_from_address (struct program_space *pspace, CORE_ADDR address)
{
  struct so_list *so = NULL;

  for (so = pspace->so_list; so; so = so->next)
    if (solib_contains_address_p (so, address))
      return (so->so_name);

  return (0);
}

/* Return whether the data starting at VADDR, size SIZE, must be kept
   in a core file for shared libraries loaded before "gcore" is used
   to be handled correctly when the core file is loaded.  This only
   applies when the section would otherwise not be kept in the core
   file (in particular, for readonly sections).  */

int
solib_keep_data_in_core (CORE_ADDR vaddr, unsigned long size)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  if (ops->keep_data_in_core)
    return ops->keep_data_in_core (vaddr, size);
  else
    return 0;
}

/* Called by free_all_symtabs */

void
clear_solib (void)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  /* This function is expected to handle ELF shared libraries.  It is
     also used on Solaris, which can run either ELF or a.out binaries
     (for compatibility with SunOS 4), both of which can use shared
     libraries.  So we don't know whether we have an ELF executable or
     an a.out executable until the user chooses an executable file.

     ELF shared libraries don't get mapped into the address space
     until after the program starts, so we'd better not try to insert
     breakpoints in them immediately.  We have to wait until the
     dynamic linker has loaded them; we'll hit a bp_shlib_event
     breakpoint (look for calls to create_solib_event_breakpoint) when
     it's ready.

     SunOS shared libraries seem to be different --- they're present
     as soon as the process begins execution, so there's no need to
     put off inserting breakpoints.  There's also nowhere to put a
     bp_shlib_event breakpoint, so if we put it off, we'll never get
     around to it.

     So: disable breakpoints only if we're using ELF shared libs.  */
  if (exec_bfd != NULL
      && bfd_get_flavour (exec_bfd) != bfd_target_aout_flavour)
    disable_breakpoints_in_shlibs ();

  while (so_list_head)
    {
      struct so_list *so = so_list_head;

      so_list_head = so->next;
      observer_notify_solib_unloaded (so);
      remove_target_sections (so);
      free_so (so);
    }

  ops->clear_solib ();
}

/* Shared library startup support.  When GDB starts up the inferior,
   it nurses it along (through the shell) until it is ready to execute
   its first instruction.  At this point, this function gets
   called.  */

void
solib_create_inferior_hook (int from_tty)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  ops->solib_create_inferior_hook (from_tty);
}

/* Check to see if an address is in the dynamic loader's dynamic
   symbol resolution code.  Return 1 if so, 0 otherwise.  */

int
in_solib_dynsym_resolve_code (CORE_ADDR pc)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  return ops->in_dynsym_resolve_code (pc);
}

/* Implements the "sharedlibrary" command.  */

static void
sharedlibrary_command (char *args, int from_tty)
{
  dont_repeat ();
  solib_add (args, from_tty, (struct target_ops *) 0, 1);
}

/* Implements the command "nosharedlibrary", which discards symbols
   that have been auto-loaded from shared libraries.  Symbols from
   shared libraries that were added by explicit request of the user
   are not discarded.  Also called from remote.c.  */

void
no_shared_libraries (char *ignored, int from_tty)
{
  /* The order of the two routines below is important: clear_solib notifies
     the solib_unloaded observers, and some of these observers might need
     access to their associated objfiles.  Therefore, we can not purge the
     solibs' objfiles before clear_solib has been called.  */

  clear_solib ();
  objfile_purge_solibs ();
}

/* See solib.h.  */

void
update_solib_breakpoints (void)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  if (ops->update_breakpoints != NULL)
    ops->update_breakpoints ();
}

/* See solib.h.  */

void
handle_solib_event (void)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  if (ops->handle_event != NULL)
    ops->handle_event ();

  clear_program_space_solib_cache (current_inferior ()->pspace);

  /* Check for any newly added shared libraries if we're supposed to
     be adding them automatically.  Switch terminal for any messages
     produced by breakpoint_re_set.  */
  target_terminal_ours_for_output ();
  solib_add (NULL, 0, &current_target, auto_solib_add);
  target_terminal_inferior ();
}

/* Reload shared libraries, but avoid reloading the same symbol file
   we already have loaded.  */

static void
reload_shared_libraries_1 (int from_tty)
{
  struct so_list *so;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);

  for (so = so_list_head; so != NULL; so = so->next)
    {
      char *filename, *found_pathname = NULL;
      bfd *abfd;
      int was_loaded = so->symbols_loaded;
      const int flags =
	SYMFILE_DEFER_BP_RESET | (from_tty ? SYMFILE_VERBOSE : 0);

      filename = tilde_expand (so->so_original_name);
      make_cleanup (xfree, filename);
      abfd = solib_bfd_open (filename);
      if (abfd != NULL)
	{
	  found_pathname = xstrdup (bfd_get_filename (abfd));
	  make_cleanup (xfree, found_pathname);
	  gdb_bfd_unref (abfd);
	}

      /* If this shared library is no longer associated with its previous
	 symbol file, close that.  */
      if ((found_pathname == NULL && was_loaded)
	  || (found_pathname != NULL
	      && filename_cmp (found_pathname, so->so_name) != 0))
	{
	  if (so->objfile && ! (so->objfile->flags & OBJF_USERLOADED)
	      && !solib_used (so))
	    free_objfile (so->objfile);
	  remove_target_sections (so);
	  clear_so (so);
	}

      /* If this shared library is now associated with a new symbol
	 file, open it.  */
      if (found_pathname != NULL
	  && (!was_loaded
	      || filename_cmp (found_pathname, so->so_name) != 0))
	{
	  volatile struct gdb_exception e;

	  TRY_CATCH (e, RETURN_MASK_ERROR)
	    solib_map_sections (so);

	  if (e.reason < 0)
	    exception_fprintf (gdb_stderr, e,
			       _("Error while mapping "
				 "shared library sections:\n"));
	  else if (auto_solib_add || was_loaded || libpthread_solib_p (so))
	    solib_read_symbols (so, flags);
	}
    }

  do_cleanups (old_chain);
}

static void
reload_shared_libraries (char *ignored, int from_tty,
			 struct cmd_list_element *e)
{
  const struct target_so_ops *ops;

  reload_shared_libraries_1 (from_tty);

  ops = solib_ops (target_gdbarch ());

  /* Creating inferior hooks here has two purposes.  First, if we reload 
     shared libraries then the address of solib breakpoint we've computed
     previously might be no longer valid.  For example, if we forgot to set
     solib-absolute-prefix and are setting it right now, then the previous
     breakpoint address is plain wrong.  Second, installing solib hooks
     also implicitly figures were ld.so is and loads symbols for it.
     Absent this call, if we've just connected to a target and set 
     solib-absolute-prefix or solib-search-path, we'll lose all information
     about ld.so.  */
  if (target_has_execution)
    {
      /* Reset or free private data structures not associated with
	 so_list entries.  */
      ops->clear_solib ();

      /* Remove any previous solib event breakpoint.  This is usually
	 done in common code, at breakpoint_init_inferior time, but
	 we're not really starting up the inferior here.  */
      remove_solib_event_breakpoints ();

      solib_create_inferior_hook (from_tty);
    }

  /* Sometimes the platform-specific hook loads initial shared
     libraries, and sometimes it doesn't.  If it doesn't FROM_TTY will be
     incorrectly 0 but such solib targets should be fixed anyway.  If we
     made all the inferior hook methods consistent, this call could be
     removed.  Call it only after the solib target has been initialized by
     solib_create_inferior_hook.  */

  solib_add (NULL, 0, NULL, auto_solib_add);

  breakpoint_re_set ();

  /* We may have loaded or unloaded debug info for some (or all)
     shared libraries.  However, frames may still reference them.  For
     example, a frame's unwinder might still point at DWARF FDE
     structures that are now freed.  Also, getting new symbols may
     change our opinion about what is frameless.  */
  reinit_frame_cache ();

  ops->special_symbol_handling ();
}

static void
show_auto_solib_add (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Autoloading of shared library symbols is %s.\n"),
		    value);
}


/* Handler for library-specific lookup of global symbol NAME in OBJFILE.  Call
   the library-specific handler if it is installed for the current target.  */

struct symbol *
solib_global_lookup (const struct objfile *objfile,
		     const char *name,
		     const domain_enum domain)
{
  const struct target_so_ops *ops = solib_ops (target_gdbarch ());

  if (ops->lookup_lib_global_symbol != NULL)
    return ops->lookup_lib_global_symbol (objfile, name, domain);
  return NULL;
}

/* Lookup the value for a specific symbol from dynamic symbol table.  Look
   up symbol from ABFD.  MATCH_SYM is a callback function to determine
   whether to pick up a symbol.  DATA is the input of this callback
   function.  Return NULL if symbol is not found.  */

CORE_ADDR
gdb_bfd_lookup_symbol_from_symtab (bfd *abfd,
				   int (*match_sym) (asymbol *, void *),
				   void *data)
{
  long storage_needed = bfd_get_symtab_upper_bound (abfd);
  CORE_ADDR symaddr = 0;

  if (storage_needed > 0)
    {
      unsigned int i;

      asymbol **symbol_table = (asymbol **) xmalloc (storage_needed);
      struct cleanup *back_to = make_cleanup (xfree, symbol_table);
      unsigned int number_of_symbols =
	bfd_canonicalize_symtab (abfd, symbol_table);

      for (i = 0; i < number_of_symbols; i++)
	{
	  asymbol *sym  = *symbol_table++;

	  if (match_sym (sym, data))
	    {
	      /* BFD symbols are section relative.  */
	      symaddr = sym->value + sym->section->vma;
	      break;
	    }
	}
      do_cleanups (back_to);
    }

  return symaddr;
}

/* Lookup the value for a specific symbol from symbol table.  Look up symbol
   from ABFD.  MATCH_SYM is a callback function to determine whether to pick
   up a symbol.  DATA is the input of this callback function.  Return NULL
   if symbol is not found.  */

static CORE_ADDR
bfd_lookup_symbol_from_dyn_symtab (bfd *abfd,
				   int (*match_sym) (asymbol *, void *),
				   void *data)
{
  long storage_needed = bfd_get_dynamic_symtab_upper_bound (abfd);
  CORE_ADDR symaddr = 0;

  if (storage_needed > 0)
    {
      unsigned int i;
      asymbol **symbol_table = (asymbol **) xmalloc (storage_needed);
      struct cleanup *back_to = make_cleanup (xfree, symbol_table);
      unsigned int number_of_symbols =
	bfd_canonicalize_dynamic_symtab (abfd, symbol_table);

      for (i = 0; i < number_of_symbols; i++)
	{
	  asymbol *sym = *symbol_table++;

	  if (match_sym (sym, data))
	    {
	      /* BFD symbols are section relative.  */
	      symaddr = sym->value + sym->section->vma;
	      break;
	    }
	}
      do_cleanups (back_to);
    }
  return symaddr;
}

/* Lookup the value for a specific symbol from symbol table and dynamic
   symbol table.  Look up symbol from ABFD.  MATCH_SYM is a callback
   function to determine whether to pick up a symbol.  DATA is the
   input of this callback function.  Return NULL if symbol is not
   found.  */

CORE_ADDR
gdb_bfd_lookup_symbol (bfd *abfd,
		       int (*match_sym) (asymbol *, void *),
		       void *data)
{
  CORE_ADDR symaddr = gdb_bfd_lookup_symbol_from_symtab (abfd, match_sym, data);

  /* On FreeBSD, the dynamic linker is stripped by default.  So we'll
     have to check the dynamic string table too.  */
  if (symaddr == 0)
    symaddr = bfd_lookup_symbol_from_dyn_symtab (abfd, match_sym, data);

  return symaddr;
}

/* SO_LIST_HEAD may contain user-loaded object files that can be removed
   out-of-band by the user.  So upon notification of free_objfile remove
   all references to any user-loaded file that is about to be freed.  */

static void
remove_user_added_objfile (struct objfile *objfile)
{
  struct so_list *so;

  if (objfile != 0 && objfile->flags & OBJF_USERLOADED)
    {
      for (so = so_list_head; so != NULL; so = so->next)
	if (so->objfile == objfile)
	  so->objfile = NULL;
    }
}

extern initialize_file_ftype _initialize_solib; /* -Wmissing-prototypes */

void
_initialize_solib (void)
{
  solib_data = gdbarch_data_register_pre_init (solib_init);

  observer_attach_free_objfile (remove_user_added_objfile);

  add_com ("sharedlibrary", class_files, sharedlibrary_command,
	   _("Load shared object library symbols for files matching REGEXP."));
  add_info ("sharedlibrary", info_sharedlibrary_command,
	    _("Status of loaded shared object libraries."));
  add_com ("nosharedlibrary", class_files, no_shared_libraries,
	   _("Unload all shared object library symbols."));

  add_setshow_boolean_cmd ("auto-solib-add", class_support,
			   &auto_solib_add, _("\
Set autoloading of shared library symbols."), _("\
Show autoloading of shared library symbols."), _("\
If \"on\", symbols from all shared object libraries will be loaded\n\
automatically when the inferior begins execution, when the dynamic linker\n\
informs gdb that a new library has been loaded, or when attaching to the\n\
inferior.  Otherwise, symbols must be loaded manually, using \
`sharedlibrary'."),
			   NULL,
			   show_auto_solib_add,
			   &setlist, &showlist);

  add_setshow_filename_cmd ("sysroot", class_support,
			    &gdb_sysroot, _("\
Set an alternate system root."), _("\
Show the current system root."), _("\
The system root is used to load absolute shared library symbol files.\n\
For other (relative) files, you can add directories using\n\
`set solib-search-path'."),
			    reload_shared_libraries,
			    NULL,
			    &setlist, &showlist);

  add_alias_cmd ("solib-absolute-prefix", "sysroot", class_support, 0,
		 &setlist);
  add_alias_cmd ("solib-absolute-prefix", "sysroot", class_support, 0,
		 &showlist);

  add_setshow_optional_filename_cmd ("solib-search-path", class_support,
				     &solib_search_path, _("\
Set the search path for loading non-absolute shared library symbol files."),
				     _("\
Show the search path for loading non-absolute shared library symbol files."),
				     _("\
This takes precedence over the environment variables \
PATH and LD_LIBRARY_PATH."),
				     reload_shared_libraries,
				     show_solib_search_path,
				     &setlist, &showlist);
}
