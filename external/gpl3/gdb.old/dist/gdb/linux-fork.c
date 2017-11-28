/* GNU/Linux native-dependent code for debugging multiple forks.

   Copyright (C) 2005-2016 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "infrun.h"
#include "regcache.h"
#include "gdbcmd.h"
#include "infcall.h"
#include "objfiles.h"
#include "linux-fork.h"
#include "linux-nat.h"
#include "gdbthread.h"
#include "source.h"

#include "nat/gdb_ptrace.h"
#include "gdb_wait.h"
#include <dirent.h>
#include <ctype.h>

struct fork_info *fork_list;
static int highest_fork_num;

/* Prevent warning from -Wmissing-prototypes.  */
extern void _initialize_linux_fork (void);

/* Fork list data structure:  */
struct fork_info
{
  struct fork_info *next;
  ptid_t ptid;
  ptid_t parent_ptid;
  int num;			/* Convenient handle (GDB fork id).  */
  struct regcache *savedregs;	/* Convenient for info fork, saves
				   having to actually switch contexts.  */
  int clobber_regs;		/* True if we should restore saved regs.  */
  off_t *filepos;		/* Set of open file descriptors' offsets.  */
  int maxfd;
};

/* Fork list methods:  */

int
forks_exist_p (void)
{
  return (fork_list != NULL);
}

/* Return the last fork in the list.  */

static struct fork_info *
find_last_fork (void)
{
  struct fork_info *last;

  if (fork_list == NULL)
    return NULL;

  for (last = fork_list; last->next != NULL; last = last->next)
    ;
  return last;
}

/* Add a fork to the internal fork list.  */

struct fork_info *
add_fork (pid_t pid)
{
  struct fork_info *fp;

  if (fork_list == NULL && pid != ptid_get_pid (inferior_ptid))
    {
      /* Special case -- if this is the first fork in the list
	 (the list is hitherto empty), and if this new fork is
	 NOT the current inferior_ptid, then add inferior_ptid
	 first, as a special zeroeth fork id.  */
      highest_fork_num = -1;
      add_fork (ptid_get_pid (inferior_ptid));	/* safe recursion */
    }

  fp = XCNEW (struct fork_info);
  fp->ptid = ptid_build (pid, pid, 0);
  fp->num = ++highest_fork_num;

  if (fork_list == NULL)
    fork_list = fp;
  else
    {
      struct fork_info *last = find_last_fork ();

      last->next = fp;
    }

  return fp;
}

static void
free_fork (struct fork_info *fp)
{
  /* Notes on step-resume breakpoints: since this is a concern for
     threads, let's convince ourselves that it's not a concern for
     forks.  There are two ways for a fork_info to be created.  First,
     by the checkpoint command, in which case we're at a gdb prompt
     and there can't be any step-resume breakpoint.  Second, by a fork
     in the user program, in which case we *may* have stepped into the
     fork call, but regardless of whether we follow the parent or the
     child, we will return to the same place and the step-resume
     breakpoint, if any, will take care of itself as usual.  And
     unlike threads, we do not save a private copy of the step-resume
     breakpoint -- so we're OK.  */

  if (fp)
    {
      if (fp->savedregs)
	regcache_xfree (fp->savedregs);
      if (fp->filepos)
	xfree (fp->filepos);
      xfree (fp);
    }
}

static void
delete_fork (ptid_t ptid)
{
  struct fork_info *fp, *fpprev;

  fpprev = NULL;

  linux_nat_forget_process (ptid_get_pid (ptid));

  for (fp = fork_list; fp; fpprev = fp, fp = fp->next)
    if (ptid_equal (fp->ptid, ptid))
      break;

  if (!fp)
    return;

  if (fpprev)
    fpprev->next = fp->next;
  else
    fork_list = fp->next;

  free_fork (fp);

  /* Special case: if there is now only one process in the list,
     and if it is (hopefully!) the current inferior_ptid, then
     remove it, leaving the list empty -- we're now down to the
     default case of debugging a single process.  */
  if (fork_list != NULL && fork_list->next == NULL &&
      ptid_equal (fork_list->ptid, inferior_ptid))
    {
      /* Last fork -- delete from list and handle as solo process
	 (should be a safe recursion).  */
      delete_fork (inferior_ptid);
    }
}

/* Find a fork_info by matching PTID.  */
static struct fork_info *
find_fork_ptid (ptid_t ptid)
{
  struct fork_info *fp;

  for (fp = fork_list; fp; fp = fp->next)
    if (ptid_equal (fp->ptid, ptid))
      return fp;

  return NULL;
}

/* Find a fork_info by matching ID.  */
static struct fork_info *
find_fork_id (int num)
{
  struct fork_info *fp;

  for (fp = fork_list; fp; fp = fp->next)
    if (fp->num == num)
      return fp;

  return NULL;
}

/* Find a fork_info by matching pid.  */
extern struct fork_info *
find_fork_pid (pid_t pid)
{
  struct fork_info *fp;

  for (fp = fork_list; fp; fp = fp->next)
    if (pid == ptid_get_pid (fp->ptid))
      return fp;

  return NULL;
}

static ptid_t
fork_id_to_ptid (int num)
{
  struct fork_info *fork = find_fork_id (num);
  if (fork)
    return fork->ptid;
  else
    return pid_to_ptid (-1);
}

static void
init_fork_list (void)
{
  struct fork_info *fp, *fpnext;

  if (!fork_list)
    return;

  for (fp = fork_list; fp; fp = fpnext)
    {
      fpnext = fp->next;
      free_fork (fp);
    }

  fork_list = NULL;
}

/* Fork list <-> gdb interface.  */

/* Utility function for fork_load/fork_save.
   Calls lseek in the (current) inferior process.  */

static off_t
call_lseek (int fd, off_t offset, int whence)
{
  char exp[80];

  snprintf (&exp[0], sizeof (exp), "lseek (%d, %ld, %d)",
	    fd, (long) offset, whence);
  return (off_t) parse_and_eval_long (&exp[0]);
}

/* Load infrun state for the fork PTID.  */

static void
fork_load_infrun_state (struct fork_info *fp)
{
  extern void nullify_last_target_wait_ptid ();
  int i;

  linux_nat_switch_fork (fp->ptid);

  if (fp->savedregs && fp->clobber_regs)
    regcache_cpy (get_current_regcache (), fp->savedregs);

  registers_changed ();
  reinit_frame_cache ();

  stop_pc = regcache_read_pc (get_current_regcache ());
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

/* Save infrun state for the fork PTID.
   Exported for use by linux child_follow_fork.  */

static void
fork_save_infrun_state (struct fork_info *fp, int clobber_regs)
{
  char path[PATH_MAX];
  struct dirent *de;
  DIR *d;

  if (fp->savedregs)
    regcache_xfree (fp->savedregs);

  fp->savedregs = regcache_dup (get_current_regcache ());
  fp->clobber_regs = clobber_regs;

  if (clobber_regs)
    {
      /* Now save the 'state' (file position) of all open file descriptors.
	 Unfortunately fork does not take care of that for us...  */
      snprintf (path, PATH_MAX, "/proc/%ld/fd",
		(long) ptid_get_pid (fp->ptid));
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
}

/* Kill 'em all, let God sort 'em out...  */

void
linux_fork_killall (void)
{
  /* Walk list and kill every pid.  No need to treat the
     current inferior_ptid as special (we do not return a
     status for it) -- however any process may be a child
     or a parent, so may get a SIGCHLD from a previously
     killed child.  Wait them all out.  */
  struct fork_info *fp;
  pid_t pid, ret;
  int status;

  for (fp = fork_list; fp; fp = fp->next)
    {
      pid = ptid_get_pid (fp->ptid);
      do {
	/* Use SIGKILL instead of PTRACE_KILL because the former works even
	   if the thread is running, while the later doesn't.  */
	kill (pid, SIGKILL);
	ret = waitpid (pid, &status, 0);
	/* We might get a SIGCHLD instead of an exit status.  This is
	 aggravated by the first kill above - a child has just
	 died.  MVS comment cut-and-pasted from linux-nat.  */
      } while (ret == pid && WIFSTOPPED (status));
    }
  init_fork_list ();	/* Clear list, prepare to start fresh.  */
}

/* The current inferior_ptid has exited, but there are other viable
   forks to debug.  Delete the exiting one and context-switch to the
   first available.  */

void
linux_fork_mourn_inferior (void)
{
  struct fork_info *last;
  int status;

  /* Wait just one more time to collect the inferior's exit status.
     Do not check whether this succeeds though, since we may be
     dealing with a process that we attached to.  Such a process will
     only report its exit status to its original parent.  */
  waitpid (ptid_get_pid (inferior_ptid), &status, 0);

  /* OK, presumably inferior_ptid is the one who has exited.
     We need to delete that one from the fork_list, and switch
     to the next available fork.  */
  delete_fork (inferior_ptid);

  /* There should still be a fork - if there's only one left,
     delete_fork won't remove it, because we haven't updated
     inferior_ptid yet.  */
  gdb_assert (fork_list);

  last = find_last_fork ();
  fork_load_infrun_state (last);
  printf_filtered (_("[Switching to %s]\n"),
		   target_pid_to_str (inferior_ptid));

  /* If there's only one fork, switch back to non-fork mode.  */
  if (fork_list->next == NULL)
    delete_fork (inferior_ptid);
}

/* The current inferior_ptid is being detached, but there are other
   viable forks to debug.  Detach and delete it and context-switch to
   the first available.  */

void
linux_fork_detach (const char *args, int from_tty)
{
  /* OK, inferior_ptid is the one we are detaching from.  We need to
     delete it from the fork_list, and switch to the next available
     fork.  */

  if (ptrace (PTRACE_DETACH, ptid_get_pid (inferior_ptid), 0, 0))
    error (_("Unable to detach %s"), target_pid_to_str (inferior_ptid));

  delete_fork (inferior_ptid);

  /* There should still be a fork - if there's only one left,
     delete_fork won't remove it, because we haven't updated
     inferior_ptid yet.  */
  gdb_assert (fork_list);

  fork_load_infrun_state (fork_list);

  if (from_tty)
    printf_filtered (_("[Switching to %s]\n"),
		     target_pid_to_str (inferior_ptid));

  /* If there's only one fork, switch back to non-fork mode.  */
  if (fork_list->next == NULL)
    delete_fork (inferior_ptid);
}

static void
inferior_call_waitpid_cleanup (void *fp)
{
  struct fork_info *oldfp = (struct fork_info *) fp;

  if (oldfp)
    {
      /* Switch back to inferior_ptid.  */
      remove_breakpoints ();
      fork_load_infrun_state (oldfp);
      insert_breakpoints ();
    }
}

static int
inferior_call_waitpid (ptid_t pptid, int pid)
{
  struct objfile *waitpid_objf;
  struct value *waitpid_fn = NULL;
  struct value *argv[4], *retv;
  struct gdbarch *gdbarch = get_current_arch ();
  struct fork_info *oldfp = NULL, *newfp = NULL;
  struct cleanup *old_cleanup;
  int ret = -1;

  if (!ptid_equal (pptid, inferior_ptid))
    {
      /* Switch to pptid.  */
      oldfp = find_fork_ptid (inferior_ptid);
      gdb_assert (oldfp != NULL);
      newfp = find_fork_ptid (pptid);
      gdb_assert (newfp != NULL);
      fork_save_infrun_state (oldfp, 1);
      remove_breakpoints ();
      fork_load_infrun_state (newfp);
      insert_breakpoints ();
    }

  old_cleanup = make_cleanup (inferior_call_waitpid_cleanup, oldfp);

  /* Get the waitpid_fn.  */
  if (lookup_minimal_symbol ("waitpid", NULL, NULL).minsym != NULL)
    waitpid_fn = find_function_in_inferior ("waitpid", &waitpid_objf);
  if (!waitpid_fn
      && lookup_minimal_symbol ("_waitpid", NULL, NULL).minsym != NULL)
    waitpid_fn = find_function_in_inferior ("_waitpid", &waitpid_objf);
  if (!waitpid_fn)
    goto out;

  /* Get the argv.  */
  argv[0] = value_from_longest (builtin_type (gdbarch)->builtin_int, pid);
  argv[1] = value_from_pointer (builtin_type (gdbarch)->builtin_data_ptr, 0);
  argv[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);
  argv[3] = 0;

  retv = call_function_by_hand (waitpid_fn, 3, argv);
  if (value_as_long (retv) < 0)
    goto out;

  ret = 0;

out:
  do_cleanups (old_cleanup);
  return ret;
}

/* Fork list <-> user interface.  */

static void
delete_checkpoint_command (char *args, int from_tty)
{
  ptid_t ptid, pptid;
  struct fork_info *fi;

  if (!args || !*args)
    error (_("Requires argument (checkpoint id to delete)"));

  ptid = fork_id_to_ptid (parse_and_eval_long (args));
  if (ptid_equal (ptid, minus_one_ptid))
    error (_("No such checkpoint id, %s"), args);

  if (ptid_equal (ptid, inferior_ptid))
    error (_("\
Please switch to another checkpoint before deleting the current one"));

  if (ptrace (PTRACE_KILL, ptid_get_pid (ptid), 0, 0))
    error (_("Unable to kill pid %s"), target_pid_to_str (ptid));

  fi = find_fork_ptid (ptid);
  gdb_assert (fi);
  pptid = fi->parent_ptid;

  if (from_tty)
    printf_filtered (_("Killed %s\n"), target_pid_to_str (ptid));

  delete_fork (ptid);

  /* If fi->parent_ptid is not a part of lwp but it's a part of checkpoint
     list, waitpid the ptid.
     If fi->parent_ptid is a part of lwp and it is stoped, waitpid the
     ptid.  */
  if ((!find_thread_ptid (pptid) && find_fork_ptid (pptid))
      || (find_thread_ptid (pptid) && is_stopped (pptid)))
    {
      if (inferior_call_waitpid (pptid, ptid_get_pid (ptid)))
        warning (_("Unable to wait pid %s"), target_pid_to_str (ptid));
    }
}

static void
detach_checkpoint_command (char *args, int from_tty)
{
  ptid_t ptid;

  if (!args || !*args)
    error (_("Requires argument (checkpoint id to detach)"));

  ptid = fork_id_to_ptid (parse_and_eval_long (args));
  if (ptid_equal (ptid, minus_one_ptid))
    error (_("No such checkpoint id, %s"), args);

  if (ptid_equal (ptid, inferior_ptid))
    error (_("\
Please switch to another checkpoint before detaching the current one"));

  if (ptrace (PTRACE_DETACH, ptid_get_pid (ptid), 0, 0))
    error (_("Unable to detach %s"), target_pid_to_str (ptid));

  if (from_tty)
    printf_filtered (_("Detached %s\n"), target_pid_to_str (ptid));

  delete_fork (ptid);
}

/* Print information about currently known checkpoints.  */

static void
info_checkpoints_command (char *arg, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct symtab_and_line sal;
  struct fork_info *fp;
  ULONGEST pc;
  int requested = -1;
  struct fork_info *printed = NULL;

  if (arg && *arg)
    requested = (int) parse_and_eval_long (arg);

  for (fp = fork_list; fp; fp = fp->next)
    {
      if (requested > 0 && fp->num != requested)
	continue;

      printed = fp;
      if (ptid_equal (fp->ptid, inferior_ptid))
	{
	  printf_filtered ("* ");
	  pc = regcache_read_pc (get_current_regcache ());
	}
      else
	{
	  printf_filtered ("  ");
	  pc = regcache_read_pc (fp->savedregs);
	}
      printf_filtered ("%d %s", fp->num, target_pid_to_str (fp->ptid));
      if (fp->num == 0)
	printf_filtered (_(" (main process)"));
      printf_filtered (_(" at "));
      fputs_filtered (paddress (gdbarch, pc), gdb_stdout);

      sal = find_pc_line (pc, 0);
      if (sal.symtab)
	printf_filtered (_(", file %s"),
			 symtab_to_filename_for_display (sal.symtab));
      if (sal.line)
	printf_filtered (_(", line %d"), sal.line);
      if (!sal.symtab && !sal.line)
	{
	  struct bound_minimal_symbol msym;

	  msym = lookup_minimal_symbol_by_pc (pc);
	  if (msym.minsym)
	    printf_filtered (", <%s>", MSYMBOL_LINKAGE_NAME (msym.minsym));
	}

      putchar_filtered ('\n');
    }
  if (printed == NULL)
    {
      if (requested > 0)
	printf_filtered (_("No checkpoint number %d.\n"), requested);
      else
	printf_filtered (_("No checkpoints.\n"));
    }
}

/* The PID of the process we're checkpointing.  */
static int checkpointing_pid = 0;

int
linux_fork_checkpointing_p (int pid)
{
  return (checkpointing_pid == pid);
}

/* Callback for iterate over threads.  Used to check whether
   the current inferior is multi-threaded.  Returns true as soon
   as it sees the second thread of the current inferior.  */

static int
inf_has_multiple_thread_cb (struct thread_info *tp, void *data)
{
  int *count_p = (int *) data;
  
  if (current_inferior ()->pid == ptid_get_pid (tp->ptid))
    (*count_p)++;
  
  /* Stop the iteration if multiple threads have been detected.  */
  return *count_p > 1;
}

/* Return true if the current inferior is multi-threaded.  */

static int
inf_has_multiple_threads (void)
{
  int count = 0;

  iterate_over_threads (inf_has_multiple_thread_cb, &count);
  return (count > 1);
}

static void
checkpoint_command (char *args, int from_tty)
{
  struct objfile *fork_objf;
  struct gdbarch *gdbarch;
  struct target_waitstatus last_target_waitstatus;
  ptid_t last_target_ptid;
  struct value *fork_fn = NULL, *ret;
  struct fork_info *fp;
  pid_t retpid;
  struct cleanup *old_chain;

  if (!target_has_execution) 
    error (_("The program is not being run."));

  /* Ensure that the inferior is not multithreaded.  */
  update_thread_list ();
  if (inf_has_multiple_threads ())
    error (_("checkpoint: can't checkpoint multiple threads."));
  
  /* Make the inferior fork, record its (and gdb's) state.  */

  if (lookup_minimal_symbol ("fork", NULL, NULL).minsym != NULL)
    fork_fn = find_function_in_inferior ("fork", &fork_objf);
  if (!fork_fn)
    if (lookup_minimal_symbol ("_fork", NULL, NULL).minsym != NULL)
      fork_fn = find_function_in_inferior ("fork", &fork_objf);
  if (!fork_fn)
    error (_("checkpoint: can't find fork function in inferior."));

  gdbarch = get_objfile_arch (fork_objf);
  ret = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);

  /* Tell linux-nat.c that we're checkpointing this inferior.  */
  old_chain = make_cleanup_restore_integer (&checkpointing_pid);
  checkpointing_pid = ptid_get_pid (inferior_ptid);

  ret = call_function_by_hand (fork_fn, 0, &ret);
  do_cleanups (old_chain);
  if (!ret)	/* Probably can't happen.  */
    error (_("checkpoint: call_function_by_hand returned null."));

  retpid = value_as_long (ret);
  get_last_target_status (&last_target_ptid, &last_target_waitstatus);

  fp = find_fork_pid (retpid);

  if (from_tty)
    {
      int parent_pid;

      printf_filtered (_("checkpoint %d: fork returned pid %ld.\n"),
		       fp != NULL ? fp->num : -1, (long) retpid);
      if (info_verbose)
	{
	  parent_pid = ptid_get_lwp (last_target_ptid);
	  if (parent_pid == 0)
	    parent_pid = ptid_get_pid (last_target_ptid);
	  printf_filtered (_("   gdb says parent = %ld.\n"),
			   (long) parent_pid);
	}
    }

  if (!fp)
    error (_("Failed to find new fork"));
  fork_save_infrun_state (fp, 1);
  fp->parent_ptid = last_target_ptid;
}

static void
linux_fork_context (struct fork_info *newfp, int from_tty)
{
  /* Now we attempt to switch processes.  */
  struct fork_info *oldfp;

  gdb_assert (newfp != NULL);

  oldfp = find_fork_ptid (inferior_ptid);
  gdb_assert (oldfp != NULL);

  fork_save_infrun_state (oldfp, 1);
  remove_breakpoints ();
  fork_load_infrun_state (newfp);
  insert_breakpoints ();

  printf_filtered (_("Switching to %s\n"),
		   target_pid_to_str (inferior_ptid));

  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* Switch inferior process (checkpoint) context, by checkpoint id.  */
static void
restart_command (char *args, int from_tty)
{
  struct fork_info *fp;

  if (!args || !*args)
    error (_("Requires argument (checkpoint id to restart)"));

  if ((fp = find_fork_id (parse_and_eval_long (args))) == NULL)
    error (_("Not found: checkpoint id %s"), args);

  linux_fork_context (fp, from_tty);
}

void
_initialize_linux_fork (void)
{
  init_fork_list ();

  /* Checkpoint command: create a fork of the inferior process
     and set it aside for later debugging.  */

  add_com ("checkpoint", class_obscure, checkpoint_command, _("\
Fork a duplicate process (experimental)."));

  /* Restart command: restore the context of a specified checkpoint
     process.  */

  add_com ("restart", class_obscure, restart_command, _("\
restart <n>: restore program context from a checkpoint.\n\
Argument 'n' is checkpoint ID, as displayed by 'info checkpoints'."));

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
