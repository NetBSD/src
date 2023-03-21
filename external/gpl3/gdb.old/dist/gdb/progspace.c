/* Program and address space management, for GDB, the GNU debugger.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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
#include "gdbcmd.h"
#include "objfiles.h"
#include "arch-utils.h"
#include "gdbcore.h"
#include "solib.h"
#include "solist.h"
#include "gdbthread.h"
#include "inferior.h"
#include <algorithm>

/* The last program space number assigned.  */
int last_program_space_num = 0;

/* The head of the program spaces list.  */
std::vector<struct program_space *> program_spaces;

/* Pointer to the current program space.  */
struct program_space *current_program_space;

/* The last address space number assigned.  */
static int highest_address_space_num;



/* Keep a registry of per-program_space data-pointers required by other GDB
   modules.  */

DEFINE_REGISTRY (program_space, REGISTRY_ACCESS_FIELD)

/* Keep a registry of per-address_space data-pointers required by other GDB
   modules.  */

DEFINE_REGISTRY (address_space, REGISTRY_ACCESS_FIELD)



/* Create a new address space object, and add it to the list.  */

struct address_space *
new_address_space (void)
{
  struct address_space *aspace;

  aspace = XCNEW (struct address_space);
  aspace->num = ++highest_address_space_num;
  address_space_alloc_data (aspace);

  return aspace;
}

/* Maybe create a new address space object, and add it to the list, or
   return a pointer to an existing address space, in case inferiors
   share an address space on this target system.  */

struct address_space *
maybe_new_address_space (void)
{
  int shared_aspace = gdbarch_has_shared_address_space (target_gdbarch ());

  if (shared_aspace)
    {
      /* Just return the first in the list.  */
      return program_spaces[0]->aspace;
    }

  return new_address_space ();
}

static void
free_address_space (struct address_space *aspace)
{
  address_space_free_data (aspace);
  xfree (aspace);
}

int
address_space_num (struct address_space *aspace)
{
  return aspace->num;
}

/* Start counting over from scratch.  */

static void
init_address_spaces (void)
{
  highest_address_space_num = 0;
}



/* Remove a program space from the program spaces list.  */

static void
remove_program_space (program_space *pspace)
{
  gdb_assert (pspace != NULL);

  auto iter = std::find (program_spaces.begin (), program_spaces.end (),
			 pspace);
  gdb_assert (iter != program_spaces.end ());
  program_spaces.erase (iter);
}

/* See progspace.h.  */

program_space::program_space (address_space *aspace_)
  : num (++last_program_space_num),
    aspace (aspace_)
{
  program_space_alloc_data (this);

  program_spaces.push_back (this);
}

/* See progspace.h.  */

program_space::~program_space ()
{
  gdb_assert (this != current_program_space);

  remove_program_space (this);

  scoped_restore_current_program_space restore_pspace;

  set_current_program_space (this);

  breakpoint_program_space_exit (this);
  no_shared_libraries (NULL, 0);
  exec_close ();
  free_all_objfiles ();
  /* Defer breakpoint re-set because we don't want to create new
     locations for this pspace which we're tearing down.  */
  clear_symtab_users (SYMFILE_DEFER_BP_RESET);
  if (!gdbarch_has_shared_address_space (target_gdbarch ()))
    free_address_space (this->aspace);
  clear_section_table (&this->target_sections);
  clear_program_space_solib_cache (this);
    /* Discard any data modules have associated with the PSPACE.  */
  program_space_free_data (this);
}

/* See progspace.h.  */

void
program_space::free_all_objfiles ()
{
  /* Any objfile reference would become stale.  */
  for (struct so_list *so : current_program_space->solibs ())
    gdb_assert (so->objfile == NULL);

  while (!objfiles_list.empty ())
    objfiles_list.front ()->unlink ();
}

/* See progspace.h.  */

void
program_space::add_objfile (std::shared_ptr<objfile> &&objfile,
			    struct objfile *before)
{
  if (before == nullptr)
    objfiles_list.push_back (std::move (objfile));
  else
    {
      auto iter = std::find_if (objfiles_list.begin (), objfiles_list.end (),
				[=] (const std::shared_ptr<::objfile> &objf)
				{
				  return objf.get () == before;
				});
      gdb_assert (iter != objfiles_list.end ());
      objfiles_list.insert (iter, std::move (objfile));
    }
}

/* See progspace.h.  */

void
program_space::remove_objfile (struct objfile *objfile)
{
  /* Removing an objfile from the objfile list invalidates any frame
     that was built using frame info found in the objfile.  Reinit the
     frame cache to get rid of any frame that might otherwise
     reference stale info.  */
  reinit_frame_cache ();

  auto iter = std::find_if (objfiles_list.begin (), objfiles_list.end (),
			    [=] (const std::shared_ptr<::objfile> &objf)
			    {
			      return objf.get () == objfile;
			    });
  gdb_assert (iter != objfiles_list.end ());
  objfiles_list.erase (iter);

  if (objfile == symfile_object_file)
    symfile_object_file = NULL;
}

/* See progspace.h.  */

next_adapter<struct so_list>
program_space::solibs () const
{
  return next_adapter<struct so_list> (this->so_list);
}

/* Copies program space SRC to DEST.  Copies the main executable file,
   and the main symbol file.  Returns DEST.  */

struct program_space *
clone_program_space (struct program_space *dest, struct program_space *src)
{
  scoped_restore_current_program_space restore_pspace;

  set_current_program_space (dest);

  if (src->pspace_exec_filename != NULL)
    exec_file_attach (src->pspace_exec_filename, 0);

  if (src->symfile_object_file != NULL)
    symbol_file_add_main (objfile_name (src->symfile_object_file),
			  SYMFILE_DEFER_BP_RESET);

  return dest;
}

/* Sets PSPACE as the current program space.  It is the caller's
   responsibility to make sure that the currently selected
   inferior/thread matches the selected program space.  */

void
set_current_program_space (struct program_space *pspace)
{
  if (current_program_space == pspace)
    return;

  gdb_assert (pspace != NULL);

  current_program_space = pspace;

  /* Different symbols change our view of the frame chain.  */
  reinit_frame_cache ();
}

/* Returns true iff there's no inferior bound to PSPACE.  */

int
program_space_empty_p (struct program_space *pspace)
{
  if (find_inferior_for_program_space (pspace) != NULL)
      return 0;

  return 1;
}

/* Prints the list of program spaces and their details on UIOUT.  If
   REQUESTED is not -1, it's the ID of the pspace that should be
   printed.  Otherwise, all spaces are printed.  */

static void
print_program_space (struct ui_out *uiout, int requested)
{
  int count = 0;

  /* Compute number of pspaces we will print.  */
  for (struct program_space *pspace : program_spaces)
    {
      if (requested != -1 && pspace->num != requested)
	continue;

      ++count;
    }

  /* There should always be at least one.  */
  gdb_assert (count > 0);

  ui_out_emit_table table_emitter (uiout, 3, count, "pspaces");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (4, ui_left, "id", "Id");
  uiout->table_header (17, ui_left, "exec", "Executable");
  uiout->table_body ();

  for (struct program_space *pspace : program_spaces)
    {
      int printed_header;

      if (requested != -1 && requested != pspace->num)
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      if (pspace == current_program_space)
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      uiout->field_signed ("id", pspace->num);

      if (pspace->pspace_exec_filename)
	uiout->field_string ("exec", pspace->pspace_exec_filename);
      else
	uiout->field_skip ("exec");

      /* Print extra info that doesn't really fit in tabular form.
	 Currently, we print the list of inferiors bound to a pspace.
	 There can be more than one inferior bound to the same pspace,
	 e.g., both parent/child inferiors in a vfork, or, on targets
	 that share pspaces between inferiors.  */
      printed_header = 0;

      /* We're going to switch inferiors.  */
      scoped_restore_current_thread restore_thread;

      for (inferior *inf : all_inferiors ())
	if (inf->pspace == pspace)
	  {
	    /* Switch to inferior in order to call target methods.  */
	    switch_to_inferior_no_thread (inf);

	    if (!printed_header)
	      {
		printed_header = 1;
		printf_filtered ("\n\tBound inferiors: ID %d (%s)",
				 inf->num,
				 target_pid_to_str (ptid_t (inf->pid)).c_str ());
	      }
	    else
	      printf_filtered (", ID %d (%s)",
			       inf->num,
			       target_pid_to_str (ptid_t (inf->pid)).c_str ());
	  }

      uiout->text ("\n");
    }
}

/* Boolean test for an already-known program space id.  */

static int
valid_program_space_id (int num)
{
  for (struct program_space *pspace : program_spaces)
    if (pspace->num == num)
      return 1;

  return 0;
}

/* If ARGS is NULL or empty, print information about all program
   spaces.  Otherwise, ARGS is a text representation of a LONG
   indicating which the program space to print information about.  */

static void
maintenance_info_program_spaces_command (const char *args, int from_tty)
{
  int requested = -1;

  if (args && *args)
    {
      requested = parse_and_eval_long (args);
      if (!valid_program_space_id (requested))
	error (_("program space ID %d not known."), requested);
    }

  print_program_space (current_uiout, requested);
}

/* Update all program spaces matching to address spaces.  The user may
   have created several program spaces, and loaded executables into
   them before connecting to the target interface that will create the
   inferiors.  All that happens before GDB has a chance to know if the
   inferiors will share an address space or not.  Call this after
   having connected to the target interface and having fetched the
   target description, to fixup the program/address spaces mappings.

   It is assumed that there are no bound inferiors yet, otherwise,
   they'd be left with stale referenced to released aspaces.  */

void
update_address_spaces (void)
{
  int shared_aspace = gdbarch_has_shared_address_space (target_gdbarch ());
  struct inferior *inf;

  init_address_spaces ();

  if (shared_aspace)
    {
      struct address_space *aspace = new_address_space ();

      free_address_space (current_program_space->aspace);
      for (struct program_space *pspace : program_spaces)
	pspace->aspace = aspace;
    }
  else
    for (struct program_space *pspace : program_spaces)
      {
	free_address_space (pspace->aspace);
	pspace->aspace = new_address_space ();
      }

  for (inf = inferior_list; inf; inf = inf->next)
    if (gdbarch_has_global_solist (target_gdbarch ()))
      inf->aspace = maybe_new_address_space ();
    else
      inf->aspace = inf->pspace->aspace;
}



/* See progspace.h.  */

void
clear_program_space_solib_cache (struct program_space *pspace)
{
  pspace->added_solibs.clear ();
  pspace->deleted_solibs.clear ();
}



void
initialize_progspace (void)
{
  add_cmd ("program-spaces", class_maintenance,
	   maintenance_info_program_spaces_command,
	   _("Info about currently known program spaces."),
	   &maintenanceinfolist);

  /* There's always one program space.  Note that this function isn't
     an automatic _initialize_foo function, since other
     _initialize_foo routines may need to install their per-pspace
     data keys.  We can only allocate a progspace when all those
     modules have done that.  Do this before
     initialize_current_architecture, because that accesses exec_bfd,
     which in turn dereferences current_program_space.  */
  current_program_space = new program_space (new_address_space ());
}
