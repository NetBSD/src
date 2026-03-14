/* GNU/Linux native-dependent code for debugging multiple forks.

   Copyright (C) 2005-2025 Free Software Foundation, Inc.

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

#include "arch-utils.h"
#include "event-top.h"
#include "inferior.h"
#include "infrun.h"
#include "regcache.h"
#include "cli/cli-cmds.h"
#include "infcall.h"
#include "objfiles.h"
#include "linux-fork.h"
#include "linux-nat.h"
#include "gdbthread.h"
#include "source.h"
#include "progspace-and-thread.h"
#include "cli/cli-style.h"

#include "nat/gdb_ptrace.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbsupport/eintr.h"
#include "target/waitstatus.h"
#include <dirent.h>
#include <ctype.h>

#include <list>

/* Fork list data structure:  */

struct fork_info
{
  explicit fork_info (pid_t pid, int fork_num)
    : ptid (pid, pid), num (fork_num)
  {
  }

  ~fork_info ()
  {
    /* Notes on step-resume breakpoints: since this is a concern for
       threads, let's convince ourselves that it's not a concern for
       forks.  There are two ways for a fork_info to be created.
       First, by the checkpoint command, in which case we're at a gdb
       prompt and there can't be any step-resume breakpoint.  Second,
       by a fork in the user program, in which case we *may* have
       stepped into the fork call, but regardless of whether we follow
       the parent or the child, we will return to the same place and
       the step-resume breakpoint, if any, will take care of itself as
       usual.  And unlike threads, we do not save a private copy of
       the step-resume breakpoint -- so we're OK.  */

    if (savedregs)
      delete savedregs;

    xfree (filepos);
  }

  ptid_t ptid = null_ptid;
  ptid_t parent_ptid = null_ptid;

  /* Convenient handle (GDB fork id).  */
  int num;

  /* Convenient for info fork, saves having to actually switch
     contexts.  */
  readonly_detached_regcache *savedregs = nullptr;

  CORE_ADDR pc = 0;

  /* Set of open file descriptors' offsets.  */
  off_t *filepos = nullptr;

  int maxfd = 0;
};

/* Per-inferior checkpoint data.  */

struct checkpoint_inferior_data
{
  /* List of forks (checkpoints) in particular inferior.  Once a
     checkpoint has been created, fork_list will contain at least two
     items, the first in the list will be the original (or, if not
     original, then the oldest) fork.  */
  std::list<fork_info> fork_list;

  /* Most recently assigned fork number; when 0, no checkpoints have
     been created yet.  */
  int highest_fork_num = 0;
};

/* Per-inferior data key.  */

static const registry<inferior>::key<checkpoint_inferior_data>
  checkpoint_inferior_data_key;

/* Fetch per-inferior checkpoint data.  It always returns a valid pointer
   to a checkpoint_inferior_info struct.  */

static struct checkpoint_inferior_data *
get_checkpoint_inferior_data (struct inferior *inf)
{
  struct checkpoint_inferior_data *data;

  data = checkpoint_inferior_data_key.get (inf);
  if (data == nullptr)
    data = checkpoint_inferior_data_key.emplace (inf);

  return data;
}

/* Return a reference to the per-inferior fork list.  */

static std::list<fork_info> &
fork_list (inferior *inf)
{
  return get_checkpoint_inferior_data (inf)->fork_list;
}

/* Increment the highest fork number for inferior INF, returning
   the new value.  */

static int
increment_highest_fork_num (inferior *inf)
{
  return ++get_checkpoint_inferior_data (inf)->highest_fork_num;
}

/* Reset the highest fork number for inferior INF.  */

static void
reset_highest_fork_num (inferior *inf)
{
  get_checkpoint_inferior_data (inf)->highest_fork_num = 0;
}

/* Fork list methods:  */

/* Predicate which returns true if checkpoint(s) exist in the inferior
   INF, false otherwise.  */

bool
forks_exist_p (inferior *inf)
{
  /* Avoid allocating checkpoint_inferior_data storage by checking
     to see if such storage exists prior to calling fork_list.
     If we just call fork_list alone, then that call will create
     this storage, even for inferiors which don't need it.  */
  return (checkpoint_inferior_data_key.get (inf) != nullptr
	  && !fork_list (inf).empty ());
}

/* Return the last fork in the list for inferior INF.  */

static struct fork_info *
find_last_fork (inferior *inf)
{
  auto &fork_list = ::fork_list (inf);

  if (fork_list.empty ())
    return NULL;

  return &fork_list.back ();
}

/* Return true iff there's one fork in the list for inferior INF.  */

static bool
one_fork_p (inferior *inf)
{
  return fork_list (inf).size () == 1;
}

/* Add a new fork to the internal fork list.  */

void
add_fork (pid_t pid, inferior *inf)
{
  fork_list (inf).emplace_back (pid, increment_highest_fork_num (inf));
}

/* Delete a fork for PTID in inferior INF.  When the last fork is
   deleted, HIGHEST_FORK_NUM for the given inferior is reset to 0.
   The fork list may also be made to be empty when only one fork
   remains.  */

static void
delete_fork (ptid_t ptid, inferior *inf)
{
  linux_target->low_forget_process (ptid.pid ());

  auto &fork_list = ::fork_list (inf);
  for (auto it = fork_list.begin (); it != fork_list.end (); ++it)
    if (it->ptid == ptid)
      {
	fork_list.erase (it);

	if (fork_list.empty ())
	  reset_highest_fork_num (inf);

	/* Special case: if there is now only one process in the list,
	   and if it is (hopefully!) the current inferior_ptid, then
	   remove it, leaving the list empty -- we're now down to the
	   default case of debugging a single process.  */
	if (one_fork_p (inf) && fork_list.front ().ptid == inferior_ptid)
	  {
	    /* Last fork -- delete from list and handle as solo
	       process (should be a safe recursion).  */
	    delete_fork (inferior_ptid, inf);
	  }
	return;
      }
}

/* Find a fork_info and inferior by matching PTID.  */

static std::pair<fork_info *, inferior *>
find_fork_ptid (ptid_t ptid)
{
  for (inferior *inf : all_inferiors (linux_target))
    {
      for (fork_info &fi : fork_list (inf))
	if (fi.ptid == ptid)
	  return { &fi, inf };
    }

  return { nullptr, nullptr };
}

/* Find a fork_info by matching NUM in inferior INF.  */

static fork_info *
find_fork_id (inferior *inf, int num)
{
  for (fork_info &fi : fork_list (inf))
    if (fi.num == num)
      return &fi;

  return nullptr;
}

/* Find a fork_info and inferior by matching pid.  */

extern std::pair<fork_info *, inferior *>
find_fork_pid (pid_t pid)
{
  for (inferior *inf : all_inferiors (linux_target))
    {
      for (fork_info &fi : fork_list (inf))
	if (pid == fi.ptid.pid ())
	  return { &fi, inf };
    }

  return { nullptr, nullptr };
}

/* Parse a command argument representing a checkpoint id.  This
   can take one of two forms:

   Num

   -or-

   Inf.Num

   where Num is a non-negative decimal integer and Inf, if present, is
   a positive decimal integer.

   Return a pair with a pointer to the fork_info struct and pointer
   to the inferior.  This function will throw an error if there's
   a problem with the parsing or if either the inferior or checkpoint
   id does not exist. */

static std::pair<fork_info *, inferior *>
parse_checkpoint_id (const char *ckptstr)
{
  const char *number = ckptstr;
  const char *p1;
  struct inferior *inf;

  const char *dot = strchr (number, '.');

  if (dot != nullptr)
    {
      /* Parse number to the left of the dot.  */
      int inf_num;

      p1 = number;
      inf_num = get_number_trailer (&p1, '.');
      if (inf_num <= 0)
	error (_("Inferior number must be a positive integer"));

      inf = find_inferior_id (inf_num);
      if (inf == NULL)
	error (_("No inferior number '%d'"), inf_num);

      p1 = dot + 1;
    }
  else
    {
      inf = current_inferior ();
      p1 = number;
    }

  int fork_num = get_number_trailer (&p1, 0);
  if (fork_num < 0)
    error (_("Checkpoint number must be a non-negative integer"));

  if (!forks_exist_p (inf))
    error (_("Inferior %d has no checkpoints"), inf->num);

  fork_info *fork_ptr = find_fork_id (inf, fork_num);
  if (fork_ptr == nullptr)
    error (_("Invalid checkpoint number %d for inferior %d"),
	   fork_num, inf->num);

  return { fork_ptr, inf };
}

/* Fork list <-> gdb interface.  */

/* Utility function for fork_load/fork_save.
   Calls lseek in the (current) inferior process.  */

static off_t
call_lseek (int fd, off_t offset, int whence)
{
  char exp[80];

  snprintf (&exp[0], sizeof (exp), "(long) lseek (%d, %ld, %d)",
	    fd, (long) offset, whence);
  return (off_t) parse_and_eval_long (&exp[0]);
}

/* Load infrun state for the fork PTID.  */

static void
fork_load_infrun_state (struct fork_info *fp)
{
  int i;

  linux_nat_switch_fork (fp->ptid);

  if (fp->savedregs)
    get_thread_regcache (inferior_thread ())->restore (fp->savedregs);

  registers_changed ();
  reinit_frame_cache ();

  inferior_thread ()->set_stop_pc
    (regcache_read_pc (get_thread_regcache (inferior_thread ())));
  inferior_thread ()->set_executing (false);
  inferior_thread ()->set_resumed (false);
  nullify_last_target_wait_ptid ();

  /* Now restore the file positions of open file descriptors.  */
  if (fp->filepos)
    {
      for (i = 0; i <= fp->maxfd; i++)
	if (fp->filepos[i] != (off_t) -1)
	  call_lseek (i, fp->filepos[i], SEEK_SET);
      /* NOTE: I can get away with using SEEK_SET and SEEK_CUR because
	 this is native-only.  If it ever has to be cross, we'll have
	 to rethink this.  */
    }
}

/* Save infrun state for the fork FP.  */

static void
fork_save_infrun_state (struct fork_info *fp)
{
  char path[PATH_MAX];
  struct dirent *de;
  DIR *d;

  if (fp->savedregs)
    delete fp->savedregs;

  fp->savedregs = new readonly_detached_regcache
    (*get_thread_regcache (inferior_thread ()));
  fp->pc = regcache_read_pc (get_thread_regcache (inferior_thread ()));

  /* Now save the 'state' (file position) of all open file descriptors.
     Unfortunately fork does not take care of that for us...  */
  snprintf (path, PATH_MAX, "/proc/%ld/fd", (long) fp->ptid.pid ());
  if ((d = opendir (path)) != NULL)
    {
      long tmp;

      fp->maxfd = 0;
      while ((de = readdir (d)) != NULL)
	{
	  /* Count open file descriptors (actually find highest
	     numbered).  */
	  tmp = strtol (&de->d_name[0], NULL, 10);
	  if (fp->maxfd < tmp)
	    fp->maxfd = tmp;
	}
      /* Allocate array of file positions.  */
      fp->filepos = XRESIZEVEC (off_t, fp->filepos, fp->maxfd + 1);

      /* Initialize to -1 (invalid).  */
      for (tmp = 0; tmp <= fp->maxfd; tmp++)
	fp->filepos[tmp] = -1;

      /* Now find actual file positions.  */
      rewinddir (d);
      while ((de = readdir (d)) != NULL)
	if (isdigit (de->d_name[0]))
	  {
	    tmp = strtol (&de->d_name[0], NULL, 10);
	    fp->filepos[tmp] = call_lseek (tmp, 0, SEEK_CUR);
	  }
      closedir (d);
    }
}

/* Given a ptid, return a "process ptid" in which only the pid member
   is present.  This is used in calls to target_pid_to_str() to ensure
   that only process ptids are printed by this file.  */

static inline ptid_t
proc_ptid (ptid_t ptid)
{
  ptid_t process_ptid (ptid.pid ());
  return process_ptid;
}

/* Kill 'em all, let God sort 'em out...  */

void
linux_fork_killall (inferior *inf)
{
  /* Walk list and kill every pid.  No need to treat the
     current inferior_ptid as special (we do not return a
     status for it) -- however any process may be a child
     or a parent, so may get a SIGCHLD from a previously
     killed child.  Wait them all out.  */

  auto &fork_list = ::fork_list (inf);
  for (fork_info &fi : fork_list)
    {
      pid_t pid = fi.ptid.pid ();
      int status;
      pid_t ret;
      do {
	/* Use SIGKILL instead of PTRACE_KILL because the former works even
	   if the thread is running, while the later doesn't.  */
	kill (pid, SIGKILL);
	ret = gdb::waitpid (pid, &status, 0);
	/* We might get a SIGCHLD instead of an exit status.  This is
	 aggravated by the first kill above - a child has just
	 died.  MVS comment cut-and-pasted from linux-nat.  */
      } while (ret == pid && WIFSTOPPED (status));
    }

  /* Clear list, prepare to start fresh.  */
  fork_list.clear ();
  reset_highest_fork_num (inf);
}

/* The current inferior_ptid has exited, but there are other viable
   forks to debug.  Delete the exiting one and context-switch to the
   first available.  */

void
linux_fork_mourn_inferior ()
{
  struct fork_info *last;
  int status;
  inferior *inf = current_inferior ();

  /* Wait just one more time to collect the inferior's exit status.
     Do not check whether this succeeds though, since we may be
     dealing with a process that we attached to.  Such a process will
     only report its exit status to its original parent.  */
  gdb::waitpid (inferior_ptid.pid (), &status, 0);

  /* OK, presumably inferior_ptid is the one who has exited.
     We need to delete that one from the fork list, and switch
     to the next available fork.  */
  delete_fork (inferior_ptid, inf);

  /* There should still be a fork - if there's only one left,
     delete_fork won't remove it, because we haven't updated
     inferior_ptid yet.  */
  gdb_assert (!fork_list (inf).empty ());

  last = find_last_fork (inf);
  fork_load_infrun_state (last);
  gdb_printf (_("[Switching to %s]\n"),
	      target_pid_to_str (proc_ptid (inferior_ptid)).c_str ());

  /* If there's only one fork, switch back to non-fork mode.  */
  if (one_fork_p (inf))
    delete_fork (inferior_ptid, inf);
}

/* The current inferior_ptid is being detached, but there are other
   viable forks to debug.  Detach and delete it and context-switch to
   the first available.  */

void
linux_fork_detach (int from_tty, lwp_info *lp, inferior *inf)
{
  gdb_assert (lp != nullptr);
  gdb_assert (lp->ptid == inferior_ptid);

  /* OK, inferior_ptid is the one we are detaching from.  We need to
     delete it from the fork list, and switch to the next available
     fork.  But before doing the detach, do make sure that the lwp
     hasn't exited or been terminated first.  */

  if (lp->waitstatus.kind () != TARGET_WAITKIND_EXITED
      && lp->waitstatus.kind () != TARGET_WAITKIND_THREAD_EXITED
      && lp->waitstatus.kind () != TARGET_WAITKIND_SIGNALLED)
    {
      if (ptrace (PTRACE_DETACH, inferior_ptid.pid (), 0, 0))
	error (_("Unable to detach %s"),
	       target_pid_to_str (proc_ptid (inferior_ptid)).c_str ());
    }

  delete_fork (inferior_ptid, inf);

  /* There should still be a fork - if there's only one left,
     delete_fork won't remove it, because we haven't updated
     inferior_ptid yet.  */
  auto &fork_list = ::fork_list (inf);
  gdb_assert (!fork_list.empty ());

  fork_load_infrun_state (&fork_list.front ());

  if (from_tty)
    gdb_printf (_("[Switching to %s]\n"),
		target_pid_to_str (proc_ptid (inferior_ptid)).c_str ());

  /* If there's only one fork, switch back to non-fork mode.  */
  if (one_fork_p (inf))
    delete_fork (inferior_ptid, inf);
}

/* Temporarily switch to the infrun state stored on the fork_info
   identified by a given ptid_t.  When this object goes out of scope,
   restore the currently selected infrun state.   */

class scoped_switch_fork_info
{
public:
  /* Switch to the infrun state held on the fork_info identified by
     PPTID.  If PPTID is the current inferior then no switch is done.  */
  explicit scoped_switch_fork_info (ptid_t pptid)
    : m_oldfp (nullptr), m_oldinf (nullptr)
  {
    if (pptid != inferior_ptid)
      {
	/* Switch to pptid.  */
	auto [oldfp, oldinf] = find_fork_ptid (inferior_ptid);
	m_oldfp = oldfp;
	gdb_assert (m_oldfp != nullptr);
	auto [newfp, newinf]  = find_fork_ptid (pptid);
	gdb_assert (newfp != nullptr);
	fork_save_infrun_state (m_oldfp);
	remove_breakpoints ();

	if (oldinf != newinf)
	  {
	    thread_info *tp = any_thread_of_inferior (newinf);
	    switch_to_thread (tp);
	    m_oldinf = oldinf;
	  }

	fork_load_infrun_state (newfp);
	insert_breakpoints ();
      }
  }

  /* Restore the previously selected infrun state.  If the constructor
     didn't need to switch states, then nothing is done here either.  */
  ~scoped_switch_fork_info ()
  {
    if (m_oldinf != nullptr || m_oldfp != nullptr)
      {
	/* Switch back to inferior_ptid.  */
	try
	  {
	    remove_breakpoints ();
	    if (m_oldinf != nullptr)
	      {
		thread_info *tp = any_thread_of_inferior (m_oldinf);
		switch_to_thread (tp);
	      }
	    fork_load_infrun_state (m_oldfp);
	    insert_breakpoints ();
	  }
	catch (const gdb_exception_quit &ex)
	  {
	    /* We can't throw from a destructor, so re-set the quit flag
	      for later QUIT checking.  */
	    set_quit_flag ();
	  }
	catch (const gdb_exception_forced_quit &ex)
	  {
	    /* Like above, but (eventually) cause GDB to terminate by
	       setting sync_quit_force_run.  */
	    set_force_quit_flag ();
	  }
	catch (const gdb_exception &ex)
	  {
	    warning (_("Couldn't restore checkpoint state in %s: %s"),
		     target_pid_to_str (proc_ptid (m_oldfp->ptid)).c_str (),
		     ex.what ());
	  }
      }
  }

  DISABLE_COPY_AND_ASSIGN (scoped_switch_fork_info);

private:
  /* The fork_info for the previously selected infrun state, or nullptr if
     we were already in the desired state, and nothing needs to be
     restored.  */
  struct fork_info *m_oldfp;

  /* When switching to a different fork, this is the inferior for the
     fork that we're switching from, and to which we'll switch back once
     end-of-scope is reached.  It may also be nullptr if no switching
     is required.  */
  inferior *m_oldinf;
};

/* Call waitpid() by making an inferior function call.  */

static int
inferior_call_waitpid (ptid_t pptid, int pid)
{
  struct objfile *waitpid_objf;
  struct value *waitpid_fn = NULL;
  int ret = -1;

  scoped_switch_fork_info switch_fork_info (pptid);

  /* Get the waitpid_fn.  */
  if (lookup_minimal_symbol (current_program_space, "waitpid").minsym
      != nullptr)
    waitpid_fn = find_function_in_inferior ("waitpid", &waitpid_objf);
  if (!waitpid_fn
      && (lookup_minimal_symbol (current_program_space, "_waitpid").minsym
	  != nullptr))
    waitpid_fn = find_function_in_inferior ("_waitpid", &waitpid_objf);
  if (waitpid_fn != nullptr)
    {
      struct gdbarch *gdbarch = get_current_arch ();
      struct value *argv[3], *retv;

      /* Get the argv.  */
      argv[0] = value_from_longest (builtin_type (gdbarch)->builtin_int, pid);
      argv[1] = value_from_pointer (builtin_type (gdbarch)->builtin_data_ptr, 0);
      argv[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);

      retv = call_function_by_hand (waitpid_fn, NULL, argv);

      if (value_as_long (retv) >= 0)
	ret = 0;
    }

  return ret;
}

/* Fork list <-> user interface.  */

static void
delete_checkpoint_command (const char *args, int from_tty)
{
  ptid_t ptid, pptid;

  if (!args || !*args)
    error (_("Requires argument (checkpoint id to delete)"));

  auto [fi, inf] = parse_checkpoint_id (args);
  ptid = fi->ptid;
  gdb_assert (fi != nullptr);
  pptid = fi->parent_ptid;

  if (ptid.pid () == inf->pid)
    error (_("Cannot delete active checkpoint"));

  if (ptrace (PTRACE_KILL, ptid.pid (), 0, 0))
    error (_("Unable to kill pid %s"),
	   target_pid_to_str (proc_ptid (ptid)).c_str ());

  if (from_tty)
    gdb_printf (_("Killed %s\n"),
		target_pid_to_str (proc_ptid (ptid)).c_str ());

  delete_fork (ptid, inf);

  if (pptid == null_ptid)
    {
      int status;
      /* Wait to collect the inferior's exit status.  Do not check whether
	 this succeeds though, since we may be dealing with a process that we
	 attached to.  Such a process will only report its exit status to its
	 original parent.  */
      gdb::waitpid (ptid.pid (), &status, 0);
      return;
    }

  /* If fi->parent_ptid is not a part of lwp but it's a part of checkpoint
     list, waitpid the ptid.
     If fi->parent_ptid is a part of lwp and it is stopped, waitpid the
     ptid.  */
  thread_info *parent = linux_target->find_thread (pptid);
  if ((parent == NULL && find_fork_ptid (pptid).first != nullptr)
      || (parent != NULL && parent->state == THREAD_STOPPED))
    {
      if (inferior_call_waitpid (pptid, ptid.pid ()))
	warning (_("Unable to wait pid %s"),
		 target_pid_to_str (proc_ptid (ptid)).c_str ());
    }
}

static void
detach_checkpoint_command (const char *args, int from_tty)
{
  ptid_t ptid;

  if (!args || !*args)
    error (_("Requires argument (checkpoint id to detach)"));

  auto fi = parse_checkpoint_id (args).first;
  ptid = fi->ptid;

  if (ptid == inferior_ptid)
    error (_("\
Please switch to another checkpoint before detaching the current one"));

  if (ptrace (PTRACE_DETACH, ptid.pid (), 0, 0))
    error (_("Unable to detach %s"),
	   target_pid_to_str (proc_ptid (ptid)).c_str ());

  if (from_tty)
    gdb_printf (_("Detached %s\n"),
		target_pid_to_str (proc_ptid (ptid)).c_str ());

  delete_fork (ptid, current_inferior ());
}

/* Helper for info_checkpoints_command.  */

static void
print_checkpoints (struct ui_out *uiout, inferior *req_inf, fork_info *req_fi)
{
  struct inferior *cur_inf = current_inferior ();
  bool will_print_something = false;

  /* Figure out whether to print the inferior number in the
     checkpoint list.  */
  bool print_inf = (number_of_inferiors () > 1);

  /* Compute widths of some of the table components.  */
  size_t inf_width = 0;
  size_t num_width = 0;
  size_t targid_width = 0;
  for (inferior *inf : all_inferiors (linux_target))
    {
      if (req_inf != nullptr && req_inf != inf)
	continue;

      scoped_restore_current_pspace_and_thread restore_pspace_thread;
      switch_to_program_space_and_thread (inf->pspace);

      for (const fork_info &fi : fork_list (inf))
	{
	  if (req_fi != nullptr && req_fi != &fi)
	    continue;

	  will_print_something = true;

	  inf_width
	    = std::max (inf_width,
			string_printf ("%d", inf->num).size ());
	  num_width
	    = std::max (num_width,
			string_printf ("%d", fi.num).size ()
			  + (print_inf ? 1 : 0));
	  targid_width
	    = std::max (targid_width,
			target_pid_to_str (proc_ptid (fi.ptid)).size ());
	}
    }

  /* Return early if there are no checkpoints to print.  */
  if (!will_print_something)
    {
      gdb_printf (_("No checkpoints.\n"));
      return;
    }

  /* Ensure that column header width doesn't exceed that of the column data
     for the Id field.  */
  if (!print_inf && num_width < 2)
    num_width = 2;

  ui_out_emit_table table_emitter (uiout, 5, -1, "checkpoints");

  /* Define the columns / headers...  */
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header ((print_inf ? (int) inf_width : 0) + (int) num_width,
		       ui_right, "id", "Id");
  uiout->table_header (6, ui_left, "active", "Active");
  uiout->table_header (targid_width, ui_left, "target-id", "Target Id");
  uiout->table_header (1, ui_left, "frame", "Frame");
  uiout->table_body ();

  for (inferior *inf : all_inferiors (linux_target))
    {
      /* If asked to print a partciular inferior, skip all of
	 those which don't match.  */
      if (req_inf != nullptr && req_inf != inf)
	continue;

      scoped_restore_current_pspace_and_thread restore_pspace_thread;
      switch_to_program_space_and_thread (inf->pspace);

      for (const fork_info &fi : fork_list (inf))
	{
	  /* If asked to print a particular checkpoint, skip all
	     which don't match.  */
	  if (req_fi != nullptr && req_fi != &fi)
	    continue;

	  thread_info *t = any_thread_of_inferior (inf);
	  bool is_current = fi.ptid.pid () == inf->pid;

	  ui_out_emit_tuple tuple_emitter (uiout, nullptr);

	  if (is_current && cur_inf == inf)
	    uiout->field_string ("current", "*");
	  else
	    uiout->field_skip ("current");

	  if (print_inf)
	    uiout->field_fmt ("id", "%d.%d", inf->num, fi.num);
	  else
	    uiout->field_fmt ("id", "%d", fi.num);

	  /* Print out 'y' or 'n' for whether the checkpoint is current.  */
	  uiout->field_string ("active", is_current ? "y" : "n");

	  /* Print target id.  */
	  uiout->field_string
	    ("target-id", target_pid_to_str (proc_ptid (fi.ptid)).c_str ());

	  if (t->state == THREAD_RUNNING && is_current)
	    uiout->text ("(running)");
	  else
	    {
	      /* Print frame info for the checkpoint under
		 consideration.

		 Ideally, we'd call print_stack_frame() here in order
		 to have consistency (with regard to how frames are
		 printed) with other parts of GDB as well as to reduce
		 the amount of code required here.

		 However, we can't simply print the frame without
		 switching checkpoint contexts.	 To do that, we could
		 first call scoped_switch_fork_info() - that mostly
		 works - except when the active fork/checkpoint is
		 running, i.e. when t->state == THREAD_RUNNING.
		 Switching context away from a running fork has certain
		 problems associated with it.  Certainly, the
		 fork_info struct would need some new fields, but
		 work would also need to be done to do something
		 reasonable should the state of the running fork
		 have changed when switching back to it.

		 Note: If scoped_switch_fork_info() is someday
		 changed to allow switching from a running
		 fork/checkpoint, then it might also be possible to
		 allow a restart from a running checkpoint to some
		 other checkpoint.  */

	      ui_out_emit_tuple frame_tuple_emitter (uiout, "frame");
	      uiout->text ("at ");

	      ULONGEST pc
		= (is_current
		   ? regcache_read_pc (get_thread_regcache (t))
		   : fi.pc);
	      uiout->field_core_addr ("addr", get_current_arch (), pc);

	      symtab_and_line sal = find_pc_line (pc, 0);
	      if (sal.symtab)
		{
		  uiout->text (", file ");
		  uiout->field_string ("file",
		    symtab_to_filename_for_display (sal.symtab),
		    file_name_style.style ());
		}
	      if (sal.line)
		{
		  uiout->text (", line ");
		  uiout->field_signed ("line", sal.line,
				       line_number_style.style ());
		}
	      if (!sal.symtab && !sal.line)
		{
		  bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (pc);
		  if (msym.minsym)
		    {
		      uiout->text (", <");
		      uiout->field_string ("linkage-name",
					   msym.minsym->linkage_name (),
					   function_name_style.style ());
		      uiout->text (">");
		    }
		}
	    }

	  uiout->text ("\n");
	}
    }
}

/* Print information about currently known checkpoints.  */

static void
info_checkpoints_command (const char *arg, int from_tty)
{
  inferior *req_inf = nullptr;
  fork_info *req_fi = nullptr;

  if (arg && *arg)
    std::tie (req_fi, req_inf) = parse_checkpoint_id (arg);

  print_checkpoints (current_uiout, req_inf, req_fi);

}

/* The PID of the process we're checkpointing.  */
static int checkpointing_pid = 0;

bool
linux_fork_checkpointing_p (int pid)
{
  return (checkpointing_pid == pid);
}

/* Return true if the current inferior is multi-threaded.  */

static bool
inf_has_multiple_threads ()
{
  int count = 0;

  /* Return true as soon as we see the second thread of the current
     inferior.  */
  for (thread_info *tp ATTRIBUTE_UNUSED : current_inferior ()->threads ())
    if (++count > 1)
      return true;

  return false;
}

static void
checkpoint_command (const char *args, int from_tty)
{
  struct objfile *fork_objf;
  struct gdbarch *gdbarch;
  struct target_waitstatus last_target_waitstatus;
  ptid_t last_target_ptid;
  struct value *fork_fn = NULL, *ret;
  pid_t retpid;

  if (!target_has_execution ())
    error (_("The program is not being run."));

  /* Ensure that the inferior is not multithreaded.  */
  update_thread_list ();
  if (inf_has_multiple_threads ())
    error (_("checkpoint: can't checkpoint multiple threads."));

  /* Make the inferior fork, record its (and gdb's) state.  */

  if (lookup_minimal_symbol (current_program_space, "fork").minsym != nullptr)
    fork_fn = find_function_in_inferior ("fork", &fork_objf);
  if (!fork_fn)
    if (lookup_minimal_symbol (current_program_space, "_fork").minsym
	!= nullptr)
      fork_fn = find_function_in_inferior ("fork", &fork_objf);
  if (!fork_fn)
    error (_("checkpoint: can't find fork function in inferior."));

  gdbarch = fork_objf->arch ();
  ret = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);

  /* Tell linux-nat.c that we're checkpointing this inferior.  */
  {
    scoped_restore save_pid
      = make_scoped_restore (&checkpointing_pid, inferior_ptid.pid ());

    ret = call_function_by_hand (fork_fn, NULL, {});
  }

  if (!ret)	/* Probably can't happen.  */
    error (_("checkpoint: call_function_by_hand returned null."));

  retpid = value_as_long (ret);
  get_last_target_status (nullptr, &last_target_ptid, &last_target_waitstatus);

  auto [fp, inf] = find_fork_pid (retpid);

  if (!fp)
    error (_("Failed to find new fork"));

  if (from_tty)
    {
      int parent_pid;

      gdb_printf (_("Checkpoint %s: fork returned pid %ld.\n"),
		  ((number_of_inferiors () > 1)
		   ? string_printf ("%d.%d", inf->num, fp->num).c_str ()
		   : string_printf ("%d", fp->num).c_str ()),
		 (long) retpid);

      if (info_verbose)
	{
	  parent_pid = last_target_ptid.lwp ();
	  if (parent_pid == 0)
	    parent_pid = last_target_ptid.pid ();
	  gdb_printf (_("   gdb says parent = %ld.\n"),
		      (long) parent_pid);
	}
    }

  if (one_fork_p (inf))
    {
      /* Special case -- if this is the first fork in the list (the
	 list was hitherto empty), then add inferior_ptid as a special
	 zeroeth fork id.  */
      fork_list (inf).emplace_front (inferior_ptid.pid (), 0);
    }

  fork_save_infrun_state (fp);
  fp->parent_ptid = last_target_ptid;
}

static void
linux_fork_context (struct fork_info *newfp, int from_tty, inferior *newinf)
{
  bool inferior_changed = false;

  /* Now we attempt to switch processes.  */
  gdb_assert (newfp != NULL);

  if (newinf != current_inferior ())
    {
      thread_info *tp = any_thread_of_inferior (newinf);
      switch_to_thread (tp);
      inferior_changed = true;
    }

  auto oldfp = find_fork_ptid (inferior_ptid).first;
  gdb_assert (oldfp != NULL);

  if (oldfp != newfp)
    {
      fork_save_infrun_state (oldfp);
      remove_breakpoints ();
      fork_load_infrun_state (newfp);
      insert_breakpoints ();
      if (!inferior_changed)
	gdb_printf (_("Switching to %s\n"),
		    target_pid_to_str (proc_ptid (inferior_ptid)).c_str ());
    }

  notify_user_selected_context_changed
    (inferior_changed ? (USER_SELECTED_INFERIOR | USER_SELECTED_FRAME)
		      : USER_SELECTED_FRAME);
}

/* Switch inferior process (checkpoint) context, by checkpoint id.  */

static void
restart_command (const char *args, int from_tty)
{
  if (!args || !*args)
    error (_("Requires argument (checkpoint id to restart)"));

  auto [fp, inf] = parse_checkpoint_id (args);

  /* Don't allow switching from a thread/fork that's running.  */
  inferior *curinf = current_inferior ();
  if (curinf->pid != 0
      && any_thread_of_inferior (curinf)->state == THREAD_RUNNING)
    error (_("Cannot execute this command while "
	     "the selected thread is running."));

  linux_fork_context (fp, from_tty, inf);
}

INIT_GDB_FILE (linux_fork)
{
  /* Checkpoint command: create a fork of the inferior process
     and set it aside for later debugging.  */

  add_com ("checkpoint", class_obscure, checkpoint_command, _("\
Fork a duplicate process (experimental)."));

  /* Restart command: restore the context of a specified checkpoint
     process.  */

  add_com ("restart", class_obscure, restart_command, _("\
Restore program context from a checkpoint.\n\
Usage: restart N\n\
Argument N is checkpoint ID, as displayed by 'info checkpoints'."));

  /* Delete checkpoint command: kill the process and remove it from
     the fork list.  */

  add_cmd ("checkpoint", class_obscure, delete_checkpoint_command, _("\
Delete a checkpoint (experimental)."),
	   &deletelist);

  /* Detach checkpoint command: release the process to run independently,
     and remove it from the fork list.  */

  add_cmd ("checkpoint", class_obscure, detach_checkpoint_command, _("\
Detach from a checkpoint (experimental)."),
	   &detachlist);

  /* Info checkpoints command: list all forks/checkpoints
     currently under gdb's control.  */

  add_info ("checkpoints", info_checkpoints_command,
	    _("IDs of currently known checkpoints."));
}
