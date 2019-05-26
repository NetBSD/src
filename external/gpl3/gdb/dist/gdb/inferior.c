/* Multi-process control for GDB, the GNU debugger.

   Copyright (C) 2008-2019 Free Software Foundation, Inc.

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
#include "exec.h"
#include "inferior.h"
#include "target.h"
#include "command.h"
#include "completer.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "ui-out.h"
#include "observable.h"
#include "gdbcore.h"
#include "symfile.h"
#include "common/environ.h"
#include "cli/cli-utils.h"
#include "continuations.h"
#include "arch-utils.h"
#include "target-descriptions.h"
#include "readline/tilde.h"
#include "progspace-and-thread.h"

/* Keep a registry of per-inferior data-pointers required by other GDB
   modules.  */

DEFINE_REGISTRY (inferior, REGISTRY_ACCESS_FIELD)

struct inferior *inferior_list = NULL;
static int highest_inferior_num;

/* See inferior.h.  */
int print_inferior_events = 1;

/* The Current Inferior.  This is a strong reference.  I.e., whenever
   an inferior is the current inferior, its refcount is
   incremented.  */
static struct inferior *current_inferior_ = NULL;

struct inferior*
current_inferior (void)
{
  return current_inferior_;
}

void
set_current_inferior (struct inferior *inf)
{
  /* There's always an inferior.  */
  gdb_assert (inf != NULL);

  inf->incref ();
  current_inferior_->decref ();
  current_inferior_ = inf;
}

private_inferior::~private_inferior () = default;

inferior::~inferior ()
{
  inferior *inf = this;

  discard_all_inferior_continuations (inf);
  inferior_free_data (inf);
  xfree (inf->args);
  xfree (inf->terminal);
  target_desc_info_free (inf->tdesc_info);
}

inferior::inferior (int pid_)
  : num (++highest_inferior_num),
    pid (pid_),
    environment (gdb_environ::from_host_environ ()),
    registry_data ()
{
  inferior_alloc_data (this);
}

struct inferior *
add_inferior_silent (int pid)
{
  inferior *inf = new inferior (pid);

  if (inferior_list == NULL)
    inferior_list = inf;
  else
    {
      inferior *last;

      for (last = inferior_list; last->next != NULL; last = last->next)
	;
      last->next = inf;
    }

  gdb::observers::inferior_added.notify (inf);

  if (pid != 0)
    inferior_appeared (inf, pid);

  return inf;
}

struct inferior *
add_inferior (int pid)
{
  struct inferior *inf = add_inferior_silent (pid);

  if (print_inferior_events)
    {
      if (pid != 0)
	printf_unfiltered (_("[New inferior %d (%s)]\n"),
			   inf->num,
			   target_pid_to_str (ptid_t (pid)));
      else
	printf_unfiltered (_("[New inferior %d]\n"), inf->num);
    }

  return inf;
}

void
delete_inferior (struct inferior *todel)
{
  struct inferior *inf, *infprev;

  infprev = NULL;

  for (inf = inferior_list; inf; infprev = inf, inf = inf->next)
    if (inf == todel)
      break;

  if (!inf)
    return;

  for (thread_info *tp : inf->threads_safe ())
    delete_thread_silent (tp);

  if (infprev)
    infprev->next = inf->next;
  else
    inferior_list = inf->next;

  gdb::observers::inferior_removed.notify (inf);

  /* If this program space is rendered useless, remove it. */
  if (program_space_empty_p (inf->pspace))
    delete_program_space (inf->pspace);

  delete inf;
}

/* If SILENT then be quiet -- don't announce a inferior exit, or the
   exit of its threads.  */

static void
exit_inferior_1 (struct inferior *inftoex, int silent)
{
  struct inferior *inf;

  for (inf = inferior_list; inf; inf = inf->next)
    if (inf == inftoex)
      break;

  if (!inf)
    return;

  for (thread_info *tp : inf->threads_safe ())
    {
      if (silent)
	delete_thread_silent (tp);
      else
	delete_thread (tp);
    }

  gdb::observers::inferior_exit.notify (inf);

  inf->pid = 0;
  inf->fake_pid_p = 0;
  inf->priv = NULL;

  if (inf->vfork_parent != NULL)
    {
      inf->vfork_parent->vfork_child = NULL;
      inf->vfork_parent = NULL;
    }
  if (inf->vfork_child != NULL)
    {
      inf->vfork_child->vfork_parent = NULL;
      inf->vfork_child = NULL;
    }

  inf->pending_detach = 0;
  /* Reset it.  */
  inf->control = inferior_control_state (NO_STOP_QUIETLY);
}

void
exit_inferior (inferior *inf)
{
  exit_inferior_1 (inf, 0);
}

void
exit_inferior_silent (int pid)
{
  struct inferior *inf = find_inferior_pid (pid);

  exit_inferior_1 (inf, 1);
}

void
exit_inferior_silent (inferior *inf)
{
  exit_inferior_1 (inf, 1);
}

/* See inferior.h.  */

void
detach_inferior (inferior *inf)
{
  /* Save the pid, since exit_inferior_1 will reset it.  */
  int pid = inf->pid;

  exit_inferior_1 (inf, 0);

  if (print_inferior_events)
    printf_unfiltered (_("[Inferior %d (%s) detached]\n"),
		       inf->num,
		       target_pid_to_str (ptid_t (pid)));
}

void
inferior_appeared (struct inferior *inf, int pid)
{
  /* If this is the first inferior with threads, reset the global
     thread id.  */
  if (!any_thread_p ())
    init_thread_list ();

  inf->pid = pid;
  inf->has_exit_code = 0;
  inf->exit_code = 0;

  gdb::observers::inferior_appeared.notify (inf);
}

void
discard_all_inferiors (void)
{
  for (inferior *inf : all_non_exited_inferiors ())
    exit_inferior_silent (inf);
}

struct inferior *
find_inferior_id (int num)
{
  for (inferior *inf : all_inferiors ())
    if (inf->num == num)
      return inf;

  return NULL;
}

struct inferior *
find_inferior_pid (int pid)
{
  /* Looking for inferior pid == 0 is always wrong, and indicative of
     a bug somewhere else.  There may be more than one with pid == 0,
     for instance.  */
  gdb_assert (pid != 0);

  for (inferior *inf : all_inferiors ())
    if (inf->pid == pid)
      return inf;

  return NULL;
}

/* See inferior.h */

struct inferior *
find_inferior_ptid (ptid_t ptid)
{
  return find_inferior_pid (ptid.pid ());
}

/* See inferior.h.  */

struct inferior *
find_inferior_for_program_space (struct program_space *pspace)
{
  struct inferior *cur_inf = current_inferior ();

  if (cur_inf->pspace == pspace)
    return cur_inf;

  for (inferior *inf : all_inferiors ())
    if (inf->pspace == pspace)
      return inf;

  return NULL;
}

struct inferior *
iterate_over_inferiors (int (*callback) (struct inferior *, void *),
			void *data)
{
  for (inferior *inf : all_inferiors_safe ())
    if ((*callback) (inf, data))
      return inf;

  return NULL;
}

int
have_inferiors (void)
{
  for (inferior *inf ATTRIBUTE_UNUSED : all_non_exited_inferiors ())
    return 1;

  return 0;
}

/* Return the number of live inferiors.  We account for the case
   where an inferior might have a non-zero pid but no threads, as
   in the middle of a 'mourn' operation.  */

int
number_of_live_inferiors (void)
{
  int num_inf = 0;

  for (inferior *inf : all_non_exited_inferiors ())
    if (target_has_execution_1 (ptid_t (inf->pid)))
      for (thread_info *tp ATTRIBUTE_UNUSED : inf->non_exited_threads ())
	{
	  /* Found a live thread in this inferior, go to the next
	     inferior.  */
	  ++num_inf;
	  break;
	}

  return num_inf;
}

/* Return true if there is at least one live inferior.  */

int
have_live_inferiors (void)
{
  return number_of_live_inferiors () > 0;
}

/* Prune away any unused inferiors, and then prune away no longer used
   program spaces.  */

void
prune_inferiors (void)
{
  struct inferior *ss, **ss_link;

  ss = inferior_list;
  ss_link = &inferior_list;
  while (ss)
    {
      if (!ss->deletable ()
	  || !ss->removable
	  || ss->pid != 0)
	{
	  ss_link = &ss->next;
	  ss = *ss_link;
	  continue;
	}

      *ss_link = ss->next;
      delete_inferior (ss);
      ss = *ss_link;
    }
}

/* Simply returns the count of inferiors.  */

int
number_of_inferiors (void)
{
  auto rng = all_inferiors ();
  return std::distance (rng.begin (), rng.end ());
}

/* Converts an inferior process id to a string.  Like
   target_pid_to_str, but special cases the null process.  */

static const char *
inferior_pid_to_str (int pid)
{
  if (pid != 0)
    return target_pid_to_str (ptid_t (pid));
  else
    return _("<null>");
}

/* See inferior.h.  */

void
print_selected_inferior (struct ui_out *uiout)
{
  struct inferior *inf = current_inferior ();
  const char *filename = inf->pspace->pspace_exec_filename;

  if (filename == NULL)
    filename = _("<noexec>");

  uiout->message (_("[Switching to inferior %d [%s] (%s)]\n"),
		  inf->num, inferior_pid_to_str (inf->pid), filename);
}

/* Prints the list of inferiors and their details on UIOUT.  This is a
   version of 'info_inferior_command' suitable for use from MI.

   If REQUESTED_INFERIORS is not NULL, it's a list of GDB ids of the
   inferiors that should be printed.  Otherwise, all inferiors are
   printed.  */

static void
print_inferior (struct ui_out *uiout, const char *requested_inferiors)
{
  int inf_count = 0;

  /* Compute number of inferiors we will print.  */
  for (inferior *inf : all_inferiors ())
    {
      if (!number_is_in_list (requested_inferiors, inf->num))
	continue;

      ++inf_count;
    }

  if (inf_count == 0)
    {
      uiout->message ("No inferiors.\n");
      return;
    }

  ui_out_emit_table table_emitter (uiout, 4, inf_count, "inferiors");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (4, ui_left, "number", "Num");
  uiout->table_header (17, ui_left, "target-id", "Description");
  uiout->table_header (17, ui_left, "exec", "Executable");

  uiout->table_body ();
  for (inferior *inf : all_inferiors ())
    {
      if (!number_is_in_list (requested_inferiors, inf->num))
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      if (inf == current_inferior ())
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      uiout->field_int ("number", inf->num);

      uiout->field_string ("target-id", inferior_pid_to_str (inf->pid));

      if (inf->pspace->pspace_exec_filename != NULL)
	uiout->field_string ("exec", inf->pspace->pspace_exec_filename);
      else
	uiout->field_skip ("exec");

      /* Print extra info that isn't really fit to always present in
	 tabular form.  Currently we print the vfork parent/child
	 relationships, if any.  */
      if (inf->vfork_parent)
	{
	  uiout->text (_("\n\tis vfork child of inferior "));
	  uiout->field_int ("vfork-parent", inf->vfork_parent->num);
	}
      if (inf->vfork_child)
	{
	  uiout->text (_("\n\tis vfork parent of inferior "));
	  uiout->field_int ("vfork-child", inf->vfork_child->num);
	}

      uiout->text ("\n");
    }
}

static void
detach_inferior_command (const char *args, int from_tty)
{
  if (!args || !*args)
    error (_("Requires argument (inferior id(s) to detach)"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      inferior *inf = find_inferior_id (num);
      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (inf->pid == 0)
	{
	  warning (_("Inferior ID %d is not running."), num);
	  continue;
	}

      thread_info *tp = any_thread_of_inferior (inf);
      if (tp == NULL)
	{
	  warning (_("Inferior ID %d has no threads."), num);
	  continue;
	}

      switch_to_thread (tp);

      detach_command (NULL, from_tty);
    }
}

static void
kill_inferior_command (const char *args, int from_tty)
{
  if (!args || !*args)
    error (_("Requires argument (inferior id(s) to kill)"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      inferior *inf = find_inferior_id (num);
      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (inf->pid == 0)
	{
	  warning (_("Inferior ID %d is not running."), num);
	  continue;
	}

      thread_info *tp = any_thread_of_inferior (inf);
      if (tp == NULL)
	{
	  warning (_("Inferior ID %d has no threads."), num);
	  continue;
	}

      switch_to_thread (tp);

      target_kill ();
    }

  bfd_cache_close_all ();
}

static void
inferior_command (const char *args, int from_tty)
{
  struct inferior *inf;
  int num;

  num = parse_and_eval_long (args);

  inf = find_inferior_id (num);
  if (inf == NULL)
    error (_("Inferior ID %d not known."), num);

  if (inf->pid != 0)
    {
      if (inf != current_inferior ())
	{
	  thread_info *tp = any_thread_of_inferior (inf);
	  if (tp == NULL)
	    error (_("Inferior has no threads."));

	  switch_to_thread (tp);
	}

      gdb::observers::user_selected_context_changed.notify
	(USER_SELECTED_INFERIOR
	 | USER_SELECTED_THREAD
	 | USER_SELECTED_FRAME);
    }
  else
    {
      set_current_inferior (inf);
      switch_to_no_thread ();
      set_current_program_space (inf->pspace);

      gdb::observers::user_selected_context_changed.notify
	(USER_SELECTED_INFERIOR);
    }
}

/* Print information about currently known inferiors.  */

static void
info_inferiors_command (const char *args, int from_tty)
{
  print_inferior (current_uiout, args);
}

/* remove-inferior ID */

static void
remove_inferior_command (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    error (_("Requires an argument (inferior id(s) to remove)"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();
      struct inferior *inf = find_inferior_id (num);

      if (inf == NULL)
	{
	  warning (_("Inferior ID %d not known."), num);
	  continue;
	}

      if (!inf->deletable ())
	{
	  warning (_("Can not remove current inferior %d."), num);
	  continue;
	}
    
      if (inf->pid != 0)
	{
	  warning (_("Can not remove active inferior %d."), num);
	  continue;
	}

      delete_inferior (inf);
    }
}

struct inferior *
add_inferior_with_spaces (void)
{
  struct address_space *aspace;
  struct program_space *pspace;
  struct inferior *inf;
  struct gdbarch_info info;

  /* If all inferiors share an address space on this system, this
     doesn't really return a new address space; otherwise, it
     really does.  */
  aspace = maybe_new_address_space ();
  pspace = new program_space (aspace);
  inf = add_inferior (0);
  inf->pspace = pspace;
  inf->aspace = pspace->aspace;

  /* Setup the inferior's initial arch, based on information obtained
     from the global "set ..." options.  */
  gdbarch_info_init (&info);
  inf->gdbarch = gdbarch_find_by_info (info);
  /* The "set ..." options reject invalid settings, so we should
     always have a valid arch by now.  */
  gdb_assert (inf->gdbarch != NULL);

  return inf;
}

/* add-inferior [-copies N] [-exec FILENAME]  */

static void
add_inferior_command (const char *args, int from_tty)
{
  int i, copies = 1;
  gdb::unique_xmalloc_ptr<char> exec;
  symfile_add_flags add_flags = 0;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  if (args)
    {
      gdb_argv built_argv (args);

      for (char **argv = built_argv.get (); *argv != NULL; argv++)
	{
	  if (**argv == '-')
	    {
	      if (strcmp (*argv, "-copies") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -copies"));
		  copies = parse_and_eval_long (*argv);
		}
	      else if (strcmp (*argv, "-exec") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -exec"));
		  exec.reset (tilde_expand (*argv));
		}
	    }
	  else
	    error (_("Invalid argument"));
	}
    }

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (i = 0; i < copies; ++i)
    {
      struct inferior *inf = add_inferior_with_spaces ();

      printf_filtered (_("Added inferior %d\n"), inf->num);

      if (exec != NULL)
	{
	  /* Switch over temporarily, while reading executable and
	     symbols.q.  */
	  set_current_program_space (inf->pspace);
	  set_current_inferior (inf);
	  switch_to_no_thread ();

	  exec_file_attach (exec.get (), from_tty);
	  symbol_file_add_main (exec.get (), add_flags);
	}
    }
}

/* clone-inferior [-copies N] [ID] */

static void
clone_inferior_command (const char *args, int from_tty)
{
  int i, copies = 1;
  struct inferior *orginf = NULL;

  if (args)
    {
      gdb_argv built_argv (args);

      char **argv = built_argv.get ();
      for (; *argv != NULL; argv++)
	{
	  if (**argv == '-')
	    {
	      if (strcmp (*argv, "-copies") == 0)
		{
		  ++argv;
		  if (!*argv)
		    error (_("No argument to -copies"));
		  copies = parse_and_eval_long (*argv);

		  if (copies < 0)
		    error (_("Invalid copies number"));
		}
	    }
	  else
	    {
	      if (orginf == NULL)
		{
		  int num;

		  /* The first non-option (-) argument specified the
		     program space ID.  */
		  num = parse_and_eval_long (*argv);
		  orginf = find_inferior_id (num);

		  if (orginf == NULL)
		    error (_("Inferior ID %d not known."), num);
		  continue;
		}
	      else
		error (_("Invalid argument"));
	    }
	}
    }

  /* If no inferior id was specified, then the user wants to clone the
     current inferior.  */
  if (orginf == NULL)
    orginf = current_inferior ();

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  for (i = 0; i < copies; ++i)
    {
      struct address_space *aspace;
      struct program_space *pspace;
      struct inferior *inf;

      /* If all inferiors share an address space on this system, this
	 doesn't really return a new address space; otherwise, it
	 really does.  */
      aspace = maybe_new_address_space ();
      pspace = new program_space (aspace);
      inf = add_inferior (0);
      inf->pspace = pspace;
      inf->aspace = pspace->aspace;
      inf->gdbarch = orginf->gdbarch;

      /* If the original inferior had a user specified target
	 description, make the clone use it too.  */
      if (target_desc_info_from_user_p (inf->tdesc_info))
	copy_inferior_target_desc_info (inf, orginf);

      printf_filtered (_("Added inferior %d.\n"), inf->num);

      set_current_inferior (inf);
      switch_to_no_thread ();
      clone_program_space (pspace, orginf->pspace);
    }
}

/* Print notices when new inferiors are created and die.  */
static void
show_print_inferior_events (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of inferior events is %s.\n"), value);
}

/* Return a new value for the selected inferior's id.  */

static struct value *
inferior_id_make_value (struct gdbarch *gdbarch, struct internalvar *var,
			void *ignore)
{
  struct inferior *inf = current_inferior ();

  return value_from_longest (builtin_type (gdbarch)->builtin_int, inf->num);
}

/* Implementation of `$_inferior' variable.  */

static const struct internalvar_funcs inferior_funcs =
{
  inferior_id_make_value,
  NULL,
  NULL
};



void
initialize_inferiors (void)
{
  struct cmd_list_element *c = NULL;

  /* There's always one inferior.  Note that this function isn't an
     automatic _initialize_foo function, since other _initialize_foo
     routines may need to install their per-inferior data keys.  We
     can only allocate an inferior when all those modules have done
     that.  Do this after initialize_progspace, due to the
     current_program_space reference.  */
  current_inferior_ = add_inferior_silent (0);
  current_inferior_->incref ();
  current_inferior_->pspace = current_program_space;
  current_inferior_->aspace = current_program_space->aspace;
  /* The architecture will be initialized shortly, by
     initialize_current_architecture.  */

  add_info ("inferiors", info_inferiors_command,
	    _("Print a list of inferiors being managed.\n\
Usage: info inferiors [ID]...\n\
If IDs are specified, the list is limited to just those inferiors.\n\
By default all inferiors are displayed."));

  c = add_com ("add-inferior", no_class, add_inferior_command, _("\
Add a new inferior.\n\
Usage: add-inferior [-copies N] [-exec FILENAME]\n\
N is the optional number of inferiors to add, default is 1.\n\
FILENAME is the file name of the executable to use\n\
as main program."));
  set_cmd_completer (c, filename_completer);

  add_com ("remove-inferiors", no_class, remove_inferior_command, _("\
Remove inferior ID (or list of IDs).\n\
Usage: remove-inferiors ID..."));

  add_com ("clone-inferior", no_class, clone_inferior_command, _("\
Clone inferior ID.\n\
Usage: clone-inferior [-copies N] [ID]\n\
Add N copies of inferior ID.  The new inferior has the same\n\
executable loaded as the copied inferior.  If -copies is not specified,\n\
adds 1 copy.  If ID is not specified, it is the current inferior\n\
that is cloned."));

  add_cmd ("inferiors", class_run, detach_inferior_command, _("\
Detach from inferior ID (or list of IDS).\n\
Usage; detach inferiors ID..."),
	   &detachlist);

  add_cmd ("inferiors", class_run, kill_inferior_command, _("\
Kill inferior ID (or list of IDs).\n\
Usage: kill inferiors ID..."),
	   &killlist);

  add_cmd ("inferior", class_run, inferior_command, _("\
Use this command to switch between inferiors.\n\
Usage: inferior ID\n\
The new inferior ID must be currently known."),
	   &cmdlist);

  add_setshow_boolean_cmd ("inferior-events", no_class,
         &print_inferior_events, _("\
Set printing of inferior events (e.g., inferior start and exit)."), _("\
Show printing of inferior events (e.g., inferior start and exit)."), NULL,
         NULL,
         show_print_inferior_events,
         &setprintlist, &showprintlist);

  create_internalvar_type_lazy ("_inferior", &inferior_funcs, NULL);
}
