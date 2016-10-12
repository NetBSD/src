/* Machine independent support for QNX Neutrino /proc (process file system)
   for GDB.  Written by Colin Burgess at QNX Software Systems Limited.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

   Contributed by QNX Software Systems Ltd.

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

#include <fcntl.h>
#include <spawn.h>
#include <sys/debug.h>
#include <sys/procfs.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <dirent.h>
#include <sys/netmgr.h>
#include <sys/auxv.h>

#include "gdbcore.h"
#include "inferior.h"
#include "target.h"
#include "objfiles.h"
#include "gdbthread.h"
#include "nto-tdep.h"
#include "command.h"
#include "regcache.h"
#include "solib.h"
#include "inf-child.h"
#include "common/filestuff.h"

#define NULL_PID		0
#define _DEBUG_FLAG_TRACE	(_DEBUG_FLAG_TRACE_EXEC|_DEBUG_FLAG_TRACE_RD|\
		_DEBUG_FLAG_TRACE_WR|_DEBUG_FLAG_TRACE_MODIFY)

int ctl_fd;

static sighandler_t ofunc;

static procfs_run run;

static ptid_t do_attach (ptid_t ptid);

static int procfs_can_use_hw_breakpoint (struct target_ops *self,
					 enum bptype, int, int);

static int procfs_insert_hw_watchpoint (struct target_ops *self,
					CORE_ADDR addr, int len,
					enum target_hw_bp_type type,
					struct expression *cond);

static int procfs_remove_hw_watchpoint (struct target_ops *self,
					CORE_ADDR addr, int len,
					enum target_hw_bp_type type,
					struct expression *cond);

static int procfs_stopped_by_watchpoint (struct target_ops *ops);

/* These two globals are only ever set in procfs_open_1, but are
   referenced elsewhere.  'nto_procfs_node' is a flag used to say
   whether we are local, or we should get the current node descriptor
   for the remote QNX node.  */
static char *nodestr;
static unsigned nto_procfs_node = ND_LOCAL_NODE;

/* Return the current QNX Node, or error out.  This is a simple
   wrapper for the netmgr_strtond() function.  The reason this
   is required is because QNX node descriptors are transient so
   we have to re-acquire them every time.  */
static unsigned
nto_node (void)
{
  unsigned node;

  if (ND_NODE_CMP (nto_procfs_node, ND_LOCAL_NODE) == 0
      || nodestr == NULL)
    return ND_LOCAL_NODE;

  node = netmgr_strtond (nodestr, 0);
  if (node == -1)
    error (_("Lost the QNX node.  Debug session probably over."));

  return (node);
}

static enum gdb_osabi
procfs_is_nto_target (bfd *abfd)
{
  return GDB_OSABI_QNXNTO;
}

/* This is called when we call 'target native' or 'target procfs
   <arg>' from the (gdb) prompt.  For QNX6 (nto), the only valid arg
   will be a QNX node string, eg: "/net/some_node".  If arg is not a
   valid QNX node, we will default to local.  */
static void
procfs_open_1 (struct target_ops *ops, const char *arg, int from_tty)
{
  char *endstr;
  char buffer[50];
  int fd, total_size;
  procfs_sysinfo *sysinfo;
  struct cleanup *cleanups;
  char nto_procfs_path[PATH_MAX];

  /* Offer to kill previous inferiors before opening this target.  */
  target_preopen (from_tty);

  nto_is_nto_target = procfs_is_nto_target;

  /* Set the default node used for spawning to this one,
     and only override it if there is a valid arg.  */

  xfree (nodestr);
  nodestr = NULL;

  nto_procfs_node = ND_LOCAL_NODE;
  nodestr = (arg != NULL) ? xstrdup (arg) : NULL;

  init_thread_list ();

  if (nodestr)
    {
      nto_procfs_node = netmgr_strtond (nodestr, &endstr);
      if (nto_procfs_node == -1)
	{
	  if (errno == ENOTSUP)
	    printf_filtered ("QNX Net Manager not found.\n");
	  printf_filtered ("Invalid QNX node %s: error %d (%s).\n", nodestr,
			   errno, safe_strerror (errno));
	  xfree (nodestr);
	  nodestr = NULL;
	  nto_procfs_node = ND_LOCAL_NODE;
	}
      else if (*endstr)
	{
	  if (*(endstr - 1) == '/')
	    *(endstr - 1) = 0;
	  else
	    *endstr = 0;
	}
    }
  snprintf (nto_procfs_path, PATH_MAX - 1, "%s%s",
	    (nodestr != NULL) ? nodestr : "", "/proc");

  fd = open (nto_procfs_path, O_RDONLY);
  if (fd == -1)
    {
      printf_filtered ("Error opening %s : %d (%s)\n", nto_procfs_path, errno,
		       safe_strerror (errno));
      error (_("Invalid procfs arg"));
    }
  cleanups = make_cleanup_close (fd);

  sysinfo = (void *) buffer;
  if (devctl (fd, DCMD_PROC_SYSINFO, sysinfo, sizeof buffer, 0) != EOK)
    {
      printf_filtered ("Error getting size: %d (%s)\n", errno,
		       safe_strerror (errno));
      error (_("Devctl failed."));
    }
  else
    {
      total_size = sysinfo->total_size;
      sysinfo = alloca (total_size);
      if (sysinfo == NULL)
	{
	  printf_filtered ("Memory error: %d (%s)\n", errno,
			   safe_strerror (errno));
	  error (_("alloca failed."));
	}
      else
	{
	  if (devctl (fd, DCMD_PROC_SYSINFO, sysinfo, total_size, 0) != EOK)
	    {
	      printf_filtered ("Error getting sysinfo: %d (%s)\n", errno,
			       safe_strerror (errno));
	      error (_("Devctl failed."));
	    }
	  else
	    {
	      if (sysinfo->type !=
		  nto_map_arch_to_cputype (gdbarch_bfd_arch_info
					   (target_gdbarch ())->arch_name))
		error (_("Invalid target CPU."));
	    }
	}
    }
  do_cleanups (cleanups);

  inf_child_open_target (ops, arg, from_tty);
  printf_filtered ("Debugging using %s\n", nto_procfs_path);
}

static void
procfs_set_thread (ptid_t ptid)
{
  pid_t tid;

  tid = ptid_get_tid (ptid);
  devctl (ctl_fd, DCMD_PROC_CURTHREAD, &tid, sizeof (tid), 0);
}

/*  Return nonzero if the thread TH is still alive.  */
static int
procfs_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  pid_t tid;
  pid_t pid;
  procfs_status status;
  int err;

  tid = ptid_get_tid (ptid);
  pid = ptid_get_pid (ptid);

  if (kill (pid, 0) == -1)
    return 0;

  status.tid = tid;
  if ((err = devctl (ctl_fd, DCMD_PROC_TIDSTATUS,
		     &status, sizeof (status), 0)) != EOK)
    return 0;

  /* Thread is alive or dead but not yet joined,
     or dead and there is an alive (or dead unjoined) thread with
     higher tid.

     If the tid is not the same as requested, requested tid is dead.  */
  return (status.tid == tid) && (status.state != STATE_DEAD);
}

static void
update_thread_private_data_name (struct thread_info *new_thread,
				 const char *newname)
{
  int newnamelen;
  struct private_thread_info *pti;

  gdb_assert (newname != NULL);
  gdb_assert (new_thread != NULL);
  newnamelen = strlen (newname);
  if (!new_thread->priv)
    {
      new_thread->priv = xmalloc (offsetof (struct private_thread_info,
					       name)
				     + newnamelen + 1);
      memcpy (new_thread->priv->name, newname, newnamelen + 1);
    }
  else if (strcmp (newname, new_thread->priv->name) != 0)
    {
      /* Reallocate if neccessary.  */
      int oldnamelen = strlen (new_thread->priv->name);

      if (oldnamelen < newnamelen)
	new_thread->priv = xrealloc (new_thread->priv,
					offsetof (struct private_thread_info,
						  name)
					+ newnamelen + 1);
      memcpy (new_thread->priv->name, newname, newnamelen + 1);
    }
}

static void 
update_thread_private_data (struct thread_info *new_thread, 
			    pthread_t tid, int state, int flags)
{
  struct private_thread_info *pti;
  procfs_info pidinfo;
  struct _thread_name *tn;
  procfs_threadctl tctl;

#if _NTO_VERSION > 630
  gdb_assert (new_thread != NULL);

  if (devctl (ctl_fd, DCMD_PROC_INFO, &pidinfo,
	      sizeof(pidinfo), 0) != EOK)
    return;

  memset (&tctl, 0, sizeof (tctl));
  tctl.cmd = _NTO_TCTL_NAME;
  tn = (struct _thread_name *) (&tctl.data);

  /* Fetch name for the given thread.  */
  tctl.tid = tid;
  tn->name_buf_len = sizeof (tctl.data) - sizeof (*tn);
  tn->new_name_len = -1; /* Getting, not setting.  */
  if (devctl (ctl_fd, DCMD_PROC_THREADCTL, &tctl, sizeof (tctl), NULL) != EOK)
    tn->name_buf[0] = '\0';

  tn->name_buf[_NTO_THREAD_NAME_MAX] = '\0';

  update_thread_private_data_name (new_thread, tn->name_buf);

  pti = (struct private_thread_info *) new_thread->priv;
  pti->tid = tid;
  pti->state = state;
  pti->flags = flags;
#endif /* _NTO_VERSION */
}

static void
procfs_update_thread_list (struct target_ops *ops)
{
  procfs_status status;
  pid_t pid;
  ptid_t ptid;
  pthread_t tid;
  struct thread_info *new_thread;

  if (ctl_fd == -1)
    return;

  prune_threads ();

  pid = ptid_get_pid (inferior_ptid);

  status.tid = 1;

  for (tid = 1;; ++tid)
    {
      if (status.tid == tid 
	  && (devctl (ctl_fd, DCMD_PROC_TIDSTATUS, &status, sizeof (status), 0)
	      != EOK))
	break;
      if (status.tid != tid)
	/* The reason why this would not be equal is that devctl might have 
	   returned different tid, meaning the requested tid no longer exists
	   (e.g. thread exited).  */
	continue;
      ptid = ptid_build (pid, 0, tid);
      new_thread = find_thread_ptid (ptid);
      if (!new_thread)
	new_thread = add_thread (ptid);
      update_thread_private_data (new_thread, tid, status.state, 0);
      status.tid++;
    }
  return;
}

static void
do_closedir_cleanup (void *dir)
{
  closedir (dir);
}

static void
procfs_pidlist (char *args, int from_tty)
{
  DIR *dp = NULL;
  struct dirent *dirp = NULL;
  char buf[PATH_MAX];
  procfs_info *pidinfo = NULL;
  procfs_debuginfo *info = NULL;
  procfs_status *status = NULL;
  pid_t num_threads = 0;
  pid_t pid;
  char name[512];
  struct cleanup *cleanups;
  char procfs_dir[PATH_MAX];

  snprintf (procfs_dir, sizeof (procfs_dir), "%s%s",
	    (nodestr != NULL) ? nodestr : "", "/proc");

  dp = opendir (procfs_dir);
  if (dp == NULL)
    {
      fprintf_unfiltered (gdb_stderr, "failed to opendir \"%s\" - %d (%s)",
			  procfs_dir, errno, safe_strerror (errno));
      return;
    }

  cleanups = make_cleanup (do_closedir_cleanup, dp);

  /* Start scan at first pid.  */
  rewinddir (dp);

  do
    {
      int fd;
      struct cleanup *inner_cleanup;

      /* Get the right pid and procfs path for the pid.  */
      do
	{
	  dirp = readdir (dp);
	  if (dirp == NULL)
	    {
	      do_cleanups (cleanups);
	      return;
	    }
	  snprintf (buf, sizeof (buf), "%s%s/%s/as",
		    (nodestr != NULL) ? nodestr : "",
		    "/proc", dirp->d_name);
	  pid = atoi (dirp->d_name);
	}
      while (pid == 0);

      /* Open the procfs path.  */
      fd = open (buf, O_RDONLY);
      if (fd == -1)
	{
	  fprintf_unfiltered (gdb_stderr, "failed to open %s - %d (%s)\n",
			      buf, errno, safe_strerror (errno));
	  continue;
	}
      inner_cleanup = make_cleanup_close (fd);

      pidinfo = (procfs_info *) buf;
      if (devctl (fd, DCMD_PROC_INFO, pidinfo, sizeof (buf), 0) != EOK)
	{
	  fprintf_unfiltered (gdb_stderr,
			      "devctl DCMD_PROC_INFO failed - %d (%s)\n",
			      errno, safe_strerror (errno));
	  break;
	}
      num_threads = pidinfo->num_threads;

      info = (procfs_debuginfo *) buf;
      if (devctl (fd, DCMD_PROC_MAPDEBUG_BASE, info, sizeof (buf), 0) != EOK)
	strcpy (name, "unavailable");
      else
	strcpy (name, info->path);

      /* Collect state info on all the threads.  */
      status = (procfs_status *) buf;
      for (status->tid = 1; status->tid <= num_threads; status->tid++)
	{
	  const int err
	    = devctl (fd, DCMD_PROC_TIDSTATUS, status, sizeof (buf), 0);
	  printf_filtered ("%s - %d", name, pid);
	  if (err == EOK && status->tid != 0)
	    printf_filtered ("/%d\n", status->tid);
	  else
	    {
	      printf_filtered ("\n");
	      break;
	    }
	}

      do_cleanups (inner_cleanup);
    }
  while (dirp != NULL);

  do_cleanups (cleanups);
  return;
}

static void
procfs_meminfo (char *args, int from_tty)
{
  procfs_mapinfo *mapinfos = NULL;
  static int num_mapinfos = 0;
  procfs_mapinfo *mapinfo_p, *mapinfo_p2;
  int flags = ~0, err, num, i, j;

  struct
  {
    procfs_debuginfo info;
    char buff[_POSIX_PATH_MAX];
  } map;

  struct info
  {
    unsigned addr;
    unsigned size;
    unsigned flags;
    unsigned debug_vaddr;
    unsigned long long offset;
  };

  struct printinfo
  {
    unsigned long long ino;
    unsigned dev;
    struct info text;
    struct info data;
    char name[256];
  } printme;

  /* Get the number of map entrys.  */
  err = devctl (ctl_fd, DCMD_PROC_MAPINFO, NULL, 0, &num);
  if (err != EOK)
    {
      printf ("failed devctl num mapinfos - %d (%s)\n", err,
	      safe_strerror (err));
      return;
    }

  mapinfos = XNEWVEC (procfs_mapinfo, num);

  num_mapinfos = num;
  mapinfo_p = mapinfos;

  /* Fill the map entrys.  */
  err = devctl (ctl_fd, DCMD_PROC_MAPINFO, mapinfo_p, num
		* sizeof (procfs_mapinfo), &num);
  if (err != EOK)
    {
      printf ("failed devctl mapinfos - %d (%s)\n", err, safe_strerror (err));
      xfree (mapinfos);
      return;
    }

  num = min (num, num_mapinfos);

  /* Run through the list of mapinfos, and store the data and text info
     so we can print it at the bottom of the loop.  */
  for (mapinfo_p = mapinfos, i = 0; i < num; i++, mapinfo_p++)
    {
      if (!(mapinfo_p->flags & flags))
	mapinfo_p->ino = 0;

      if (mapinfo_p->ino == 0)	/* Already visited.  */
	continue;

      map.info.vaddr = mapinfo_p->vaddr;

      err = devctl (ctl_fd, DCMD_PROC_MAPDEBUG, &map, sizeof (map), 0);
      if (err != EOK)
	continue;

      memset (&printme, 0, sizeof printme);
      printme.dev = mapinfo_p->dev;
      printme.ino = mapinfo_p->ino;
      printme.text.addr = mapinfo_p->vaddr;
      printme.text.size = mapinfo_p->size;
      printme.text.flags = mapinfo_p->flags;
      printme.text.offset = mapinfo_p->offset;
      printme.text.debug_vaddr = map.info.vaddr;
      strcpy (printme.name, map.info.path);

      /* Check for matching data.  */
      for (mapinfo_p2 = mapinfos, j = 0; j < num; j++, mapinfo_p2++)
	{
	  if (mapinfo_p2->vaddr != mapinfo_p->vaddr
	      && mapinfo_p2->ino == mapinfo_p->ino
	      && mapinfo_p2->dev == mapinfo_p->dev)
	    {
	      map.info.vaddr = mapinfo_p2->vaddr;
	      err =
		devctl (ctl_fd, DCMD_PROC_MAPDEBUG, &map, sizeof (map), 0);
	      if (err != EOK)
		continue;

	      if (strcmp (map.info.path, printme.name))
		continue;

	      /* Lower debug_vaddr is always text, if nessessary, swap.  */
	      if ((int) map.info.vaddr < (int) printme.text.debug_vaddr)
		{
		  memcpy (&(printme.data), &(printme.text),
			  sizeof (printme.data));
		  printme.text.addr = mapinfo_p2->vaddr;
		  printme.text.size = mapinfo_p2->size;
		  printme.text.flags = mapinfo_p2->flags;
		  printme.text.offset = mapinfo_p2->offset;
		  printme.text.debug_vaddr = map.info.vaddr;
		}
	      else
		{
		  printme.data.addr = mapinfo_p2->vaddr;
		  printme.data.size = mapinfo_p2->size;
		  printme.data.flags = mapinfo_p2->flags;
		  printme.data.offset = mapinfo_p2->offset;
		  printme.data.debug_vaddr = map.info.vaddr;
		}
	      mapinfo_p2->ino = 0;
	    }
	}
      mapinfo_p->ino = 0;

      printf_filtered ("%s\n", printme.name);
      printf_filtered ("\ttext=%08x bytes @ 0x%08x\n", printme.text.size,
		       printme.text.addr);
      printf_filtered ("\t\tflags=%08x\n", printme.text.flags);
      printf_filtered ("\t\tdebug=%08x\n", printme.text.debug_vaddr);
      printf_filtered ("\t\toffset=%s\n", phex (printme.text.offset, 8));
      if (printme.data.size)
	{
	  printf_filtered ("\tdata=%08x bytes @ 0x%08x\n", printme.data.size,
			   printme.data.addr);
	  printf_filtered ("\t\tflags=%08x\n", printme.data.flags);
	  printf_filtered ("\t\tdebug=%08x\n", printme.data.debug_vaddr);
	  printf_filtered ("\t\toffset=%s\n", phex (printme.data.offset, 8));
	}
      printf_filtered ("\tdev=0x%x\n", printme.dev);
      printf_filtered ("\tino=0x%x\n", (unsigned int) printme.ino);
    }
  xfree (mapinfos);
  return;
}

/* Print status information about what we're accessing.  */
static void
procfs_files_info (struct target_ops *ignore)
{
  struct inferior *inf = current_inferior ();

  printf_unfiltered ("\tUsing the running image of %s %s via %s.\n",
		     inf->attach_flag ? "attached" : "child",
		     target_pid_to_str (inferior_ptid),
		     (nodestr != NULL) ? nodestr : "local node");
}

/* Target to_pid_to_exec_file implementation.  */

static char *
procfs_pid_to_exec_file (struct target_ops *ops, const int pid)
{
  int proc_fd;
  static char proc_path[PATH_MAX];
  ssize_t rd;

  /* Read exe file name.  */
  snprintf (proc_path, sizeof (proc_path), "%s/proc/%d/exefile",
	    (nodestr != NULL) ? nodestr : "", pid);
  proc_fd = open (proc_path, O_RDONLY);
  if (proc_fd == -1)
    return NULL;

  rd = read (proc_fd, proc_path, sizeof (proc_path) - 1);
  close (proc_fd);
  if (rd <= 0)
    {
      proc_path[0] = '\0';
      return NULL;
    }
  proc_path[rd] = '\0';
  return proc_path;
}

/* Attach to process PID, then initialize for debugging it.  */
static void
procfs_attach (struct target_ops *ops, const char *args, int from_tty)
{
  char *exec_file;
  int pid;
  struct inferior *inf;

  pid = parse_pid_to_attach (args);

  if (pid == getpid ())
    error (_("Attaching GDB to itself is not a good idea..."));

  if (from_tty)
    {
      exec_file = (char *) get_exec_file (0);

      if (exec_file)
	printf_unfiltered ("Attaching to program `%s', %s\n", exec_file,
			   target_pid_to_str (pid_to_ptid (pid)));
      else
	printf_unfiltered ("Attaching to %s\n",
			   target_pid_to_str (pid_to_ptid (pid)));

      gdb_flush (gdb_stdout);
    }
  inferior_ptid = do_attach (pid_to_ptid (pid));
  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = 1;

  if (!target_is_pushed (ops))
    push_target (ops);

  procfs_update_thread_list (ops);
}

static void
procfs_post_attach (struct target_ops *self, pid_t pid)
{
  if (exec_bfd)
    solib_create_inferior_hook (0);
}

static ptid_t
do_attach (ptid_t ptid)
{
  procfs_status status;
  struct sigevent event;
  char path[PATH_MAX];

  snprintf (path, PATH_MAX - 1, "%s%s/%d/as",
	    (nodestr != NULL) ? nodestr : "", "/proc", ptid_get_pid (ptid));
  ctl_fd = open (path, O_RDWR);
  if (ctl_fd == -1)
    error (_("Couldn't open proc file %s, error %d (%s)"), path, errno,
	   safe_strerror (errno));
  if (devctl (ctl_fd, DCMD_PROC_STOP, &status, sizeof (status), 0) != EOK)
    error (_("Couldn't stop process"));

  /* Define a sigevent for process stopped notification.  */
  event.sigev_notify = SIGEV_SIGNAL_THREAD;
  event.sigev_signo = SIGUSR1;
  event.sigev_code = 0;
  event.sigev_value.sival_ptr = NULL;
  event.sigev_priority = -1;
  devctl (ctl_fd, DCMD_PROC_EVENT, &event, sizeof (event), 0);

  if (devctl (ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status), 0) == EOK
      && status.flags & _DEBUG_FLAG_STOPPED)
    SignalKill (nto_node (), ptid_get_pid (ptid), 0, SIGCONT, 0, 0);
  nto_init_solib_absolute_prefix ();
  return ptid_build (ptid_get_pid (ptid), 0, status.tid);
}

/* Ask the user what to do when an interrupt is received.  */
static void
interrupt_query (void)
{
  if (query (_("Interrupted while waiting for the program.\n\
Give up (and stop debugging it)? ")))
    {
      target_mourn_inferior ();
      quit ();
    }
}

/* The user typed ^C twice.  */
static void
nto_handle_sigint_twice (int signo)
{
  signal (signo, ofunc);
  interrupt_query ();
  signal (signo, nto_handle_sigint_twice);
}

static void
nto_handle_sigint (int signo)
{
  /* If this doesn't work, try more severe steps.  */
  signal (signo, nto_handle_sigint_twice);

  target_interrupt (inferior_ptid);
}

static ptid_t
procfs_wait (struct target_ops *ops,
	     ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  sigset_t set;
  siginfo_t info;
  procfs_status status;
  static int exit_signo = 0;	/* To track signals that cause termination.  */

  ourstatus->kind = TARGET_WAITKIND_SPURIOUS;

  if (ptid_equal (inferior_ptid, null_ptid))
    {
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_0;
      exit_signo = 0;
      return null_ptid;
    }

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);

  devctl (ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status), 0);
  while (!(status.flags & _DEBUG_FLAG_ISTOP))
    {
      ofunc = signal (SIGINT, nto_handle_sigint);
      sigwaitinfo (&set, &info);
      signal (SIGINT, ofunc);
      devctl (ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status), 0);
    }

  nto_inferior_data (NULL)->stopped_flags = status.flags;
  nto_inferior_data (NULL)->stopped_pc = status.ip;

  if (status.flags & _DEBUG_FLAG_SSTEP)
    {
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
    }
  /* Was it a breakpoint?  */
  else if (status.flags & _DEBUG_FLAG_TRACE)
    {
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = GDB_SIGNAL_TRAP;
    }
  else if (status.flags & _DEBUG_FLAG_ISTOP)
    {
      switch (status.why)
	{
	case _DEBUG_WHY_SIGNALLED:
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  ourstatus->value.sig =
	    gdb_signal_from_host (status.info.si_signo);
	  exit_signo = 0;
	  break;
	case _DEBUG_WHY_FAULTED:
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  if (status.info.si_signo == SIGTRAP)
	    {
	      ourstatus->value.sig = 0;
	      exit_signo = 0;
	    }
	  else
	    {
	      ourstatus->value.sig =
		gdb_signal_from_host (status.info.si_signo);
	      exit_signo = ourstatus->value.sig;
	    }
	  break;

	case _DEBUG_WHY_TERMINATED:
	  {
	    int waitval = 0;

	    waitpid (ptid_get_pid (inferior_ptid), &waitval, WNOHANG);
	    if (exit_signo)
	      {
		/* Abnormal death.  */
		ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
		ourstatus->value.sig = exit_signo;
	      }
	    else
	      {
		/* Normal death.  */
		ourstatus->kind = TARGET_WAITKIND_EXITED;
		ourstatus->value.integer = WEXITSTATUS (waitval);
	      }
	    exit_signo = 0;
	    break;
	  }

	case _DEBUG_WHY_REQUESTED:
	  /* We are assuming a requested stop is due to a SIGINT.  */
	  ourstatus->kind = TARGET_WAITKIND_STOPPED;
	  ourstatus->value.sig = GDB_SIGNAL_INT;
	  exit_signo = 0;
	  break;
	}
    }

  return ptid_build (status.pid, 0, status.tid);
}

/* Read the current values of the inferior's registers, both the
   general register set and floating point registers (if supported)
   and update gdb's idea of their current values.  */
static void
procfs_fetch_registers (struct target_ops *ops,
			struct regcache *regcache, int regno)
{
  union
  {
    procfs_greg greg;
    procfs_fpreg fpreg;
    procfs_altreg altreg;
  }
  reg;
  int regsize;

  procfs_set_thread (inferior_ptid);
  if (devctl (ctl_fd, DCMD_PROC_GETGREG, &reg, sizeof (reg), &regsize) == EOK)
    nto_supply_gregset (regcache, (char *) &reg.greg);
  if (devctl (ctl_fd, DCMD_PROC_GETFPREG, &reg, sizeof (reg), &regsize)
      == EOK)
    nto_supply_fpregset (regcache, (char *) &reg.fpreg);
  if (devctl (ctl_fd, DCMD_PROC_GETALTREG, &reg, sizeof (reg), &regsize)
      == EOK)
    nto_supply_altregset (regcache, (char *) &reg.altreg);
}

/* Helper for procfs_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
procfs_xfer_memory (gdb_byte *readbuf, const gdb_byte *writebuf,
		    ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  int nbytes;

  if (lseek (ctl_fd, (off_t) memaddr, SEEK_SET) != (off_t) memaddr)
    return TARGET_XFER_E_IO;

  if (writebuf != NULL)
    nbytes = write (ctl_fd, writebuf, len);
  else
    nbytes = read (ctl_fd, readbuf, len);
  if (nbytes <= 0)
    return TARGET_XFER_E_IO;
  *xfered_len = nbytes;
  return TARGET_XFER_OK;
}

/* Target to_xfer_partial implementation.  */

static enum target_xfer_status
procfs_xfer_partial (struct target_ops *ops, enum target_object object,
		     const char *annex, gdb_byte *readbuf,
		     const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		     ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return procfs_xfer_memory (readbuf, writebuf, offset, len, xfered_len);
    case TARGET_OBJECT_AUXV:
      if (readbuf != NULL)
	{
	  int err;
	  CORE_ADDR initial_stack;
	  debug_process_t procinfo;
	  /* For 32-bit architecture, size of auxv_t is 8 bytes.  */
	  const unsigned int sizeof_auxv_t = sizeof (auxv_t);
	  const unsigned int sizeof_tempbuf = 20 * sizeof_auxv_t;
	  int tempread;
	  gdb_byte *const tempbuf = alloca (sizeof_tempbuf);

	  if (tempbuf == NULL)
	    return TARGET_XFER_E_IO;

	  err = devctl (ctl_fd, DCMD_PROC_INFO, &procinfo,
		        sizeof procinfo, 0);
	  if (err != EOK)
	    return TARGET_XFER_E_IO;

	  initial_stack = procinfo.initial_stack;

	  /* procfs is always 'self-hosted', no byte-order manipulation.  */
	  tempread = nto_read_auxv_from_initial_stack (initial_stack, tempbuf,
						       sizeof_tempbuf,
						       sizeof (auxv_t));
	  tempread = min (tempread, len) - offset;
	  memcpy (readbuf, tempbuf + offset, tempread);
	  *xfered_len = tempread;
	  return tempread ? TARGET_XFER_OK : TARGET_XFER_EOF;
	}
	/* Fallthru */
    default:
      return ops->beneath->to_xfer_partial (ops->beneath, object, annex,
					    readbuf, writebuf, offset, len,
					    xfered_len);
    }
}

/* Take a program previously attached to and detaches it.
   The program resumes execution and will no longer stop
   on signals, etc.  We'd better not have left any breakpoints
   in the program or it'll die when it hits one.  */
static void
procfs_detach (struct target_ops *ops, const char *args, int from_tty)
{
  int siggnal = 0;
  int pid;

  target_announce_detach ();

  if (args)
    siggnal = atoi (args);

  if (siggnal)
    SignalKill (nto_node (), ptid_get_pid (inferior_ptid), 0, siggnal, 0, 0);

  close (ctl_fd);
  ctl_fd = -1;

  pid = ptid_get_pid (inferior_ptid);
  inferior_ptid = null_ptid;
  detach_inferior (pid);
  init_thread_list ();
  inf_child_maybe_unpush_target (ops);
}

static int
procfs_breakpoint (CORE_ADDR addr, int type, int size)
{
  procfs_break brk;

  brk.type = type;
  brk.addr = addr;
  brk.size = size;
  errno = devctl (ctl_fd, DCMD_PROC_BREAK, &brk, sizeof (brk), 0);
  if (errno != EOK)
    return 1;
  return 0;
}

static int
procfs_insert_breakpoint (struct target_ops *ops, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  bp_tgt->placed_address = bp_tgt->reqstd_address;
  return procfs_breakpoint (bp_tgt->placed_address, _DEBUG_BREAK_EXEC, 0);
}

static int
procfs_remove_breakpoint (struct target_ops *ops, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt,
			  enum remove_bp_reason reason)
{
  return procfs_breakpoint (bp_tgt->placed_address, _DEBUG_BREAK_EXEC, -1);
}

static int
procfs_insert_hw_breakpoint (struct target_ops *self, struct gdbarch *gdbarch,
			     struct bp_target_info *bp_tgt)
{
  bp_tgt->placed_address = bp_tgt->reqstd_address;
  return procfs_breakpoint (bp_tgt->placed_address,
			    _DEBUG_BREAK_EXEC | _DEBUG_BREAK_HW, 0);
}

static int
procfs_remove_hw_breakpoint (struct target_ops *self,
			     struct gdbarch *gdbarch,
			     struct bp_target_info *bp_tgt)
{
  return procfs_breakpoint (bp_tgt->placed_address,
			    _DEBUG_BREAK_EXEC | _DEBUG_BREAK_HW, -1);
}

static void
procfs_resume (struct target_ops *ops,
	       ptid_t ptid, int step, enum gdb_signal signo)
{
  int signal_to_pass;
  procfs_status status;
  sigset_t *run_fault = (sigset_t *) (void *) &run.fault;

  if (ptid_equal (inferior_ptid, null_ptid))
    return;

  procfs_set_thread (ptid_equal (ptid, minus_one_ptid) ? inferior_ptid :
		     ptid);

  run.flags = _DEBUG_RUN_FAULT | _DEBUG_RUN_TRACE;
  if (step)
    run.flags |= _DEBUG_RUN_STEP;

  sigemptyset (run_fault);
  sigaddset (run_fault, FLTBPT);
  sigaddset (run_fault, FLTTRACE);
  sigaddset (run_fault, FLTILL);
  sigaddset (run_fault, FLTPRIV);
  sigaddset (run_fault, FLTBOUNDS);
  sigaddset (run_fault, FLTIOVF);
  sigaddset (run_fault, FLTIZDIV);
  sigaddset (run_fault, FLTFPE);
  /* Peter V will be changing this at some point.  */
  sigaddset (run_fault, FLTPAGE);

  run.flags |= _DEBUG_RUN_ARM;

  signal_to_pass = gdb_signal_to_host (signo);

  if (signal_to_pass)
    {
      devctl (ctl_fd, DCMD_PROC_STATUS, &status, sizeof (status), 0);
      signal_to_pass = gdb_signal_to_host (signo);
      if (status.why & (_DEBUG_WHY_SIGNALLED | _DEBUG_WHY_FAULTED))
	{
	  if (signal_to_pass != status.info.si_signo)
	    {
	      SignalKill (nto_node (), ptid_get_pid (inferior_ptid), 0,
			  signal_to_pass, 0, 0);
	      run.flags |= _DEBUG_RUN_CLRFLT | _DEBUG_RUN_CLRSIG;
	    }
	  else		/* Let it kill the program without telling us.  */
	    sigdelset (&run.trace, signal_to_pass);
	}
    }
  else
    run.flags |= _DEBUG_RUN_CLRSIG | _DEBUG_RUN_CLRFLT;

  errno = devctl (ctl_fd, DCMD_PROC_RUN, &run, sizeof (run), 0);
  if (errno != EOK)
    {
      perror (_("run error!\n"));
      return;
    }
}

static void
procfs_mourn_inferior (struct target_ops *ops)
{
  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      SignalKill (nto_node (), ptid_get_pid (inferior_ptid), 0, SIGKILL, 0, 0);
      close (ctl_fd);
    }
  inferior_ptid = null_ptid;
  init_thread_list ();
  inf_child_mourn_inferior (ops);
}

/* This function breaks up an argument string into an argument
   vector suitable for passing to execvp().
   E.g., on "run a b c d" this routine would get as input
   the string "a b c d", and as output it would fill in argv with
   the four arguments "a", "b", "c", "d".  The only additional
   functionality is simple quoting.  The gdb command:
  	run a "b c d" f
   will fill in argv with the three args "a", "b c d", "e".  */
static void
breakup_args (char *scratch, char **argv)
{
  char *pp, *cp = scratch;
  char quoting = 0;

  for (;;)
    {
      /* Scan past leading separators.  */
      quoting = 0;
      while (*cp == ' ' || *cp == '\t' || *cp == '\n')
	cp++;

      /* Break if at end of string.  */
      if (*cp == '\0')
	break;

      /* Take an arg.  */
      if (*cp == '"')
	{
	  cp++;
	  quoting = strchr (cp, '"') ? 1 : 0;
	}

      *argv++ = cp;

      /* Scan for next arg separator.  */
      pp = cp;
      if (quoting)
	cp = strchr (pp, '"');
      if ((cp == NULL) || (!quoting))
	cp = strchr (pp, ' ');
      if (cp == NULL)
	cp = strchr (pp, '\t');
      if (cp == NULL)
	cp = strchr (pp, '\n');

      /* No separators => end of string => break.  */
      if (cp == NULL)
	{
	  pp = cp;
	  break;
	}

      /* Replace the separator with a terminator.  */
      *cp++ = '\0';
    }

  /* Execv requires a null-terminated arg vector.  */
  *argv = NULL;
}

static void
procfs_create_inferior (struct target_ops *ops, char *exec_file,
			char *allargs, char **env, int from_tty)
{
  struct inheritance inherit;
  pid_t pid;
  int flags, errn;
  char **argv, *args;
  const char *in = "", *out = "", *err = "";
  int fd, fds[3];
  sigset_t set;
  const char *inferior_io_terminal = get_inferior_io_terminal ();
  struct inferior *inf;

  argv = xmalloc (((strlen (allargs) + 1) / (unsigned) 2 + 2) *
		  sizeof (*argv));
  argv[0] = get_exec_file (1);
  if (!argv[0])
    {
      if (exec_file)
	argv[0] = exec_file;
      else
	return;
    }

  args = xstrdup (allargs);
  breakup_args (args, (exec_file != NULL) ? &argv[1] : &argv[0]);

  argv = nto_parse_redirection (argv, &in, &out, &err);

  fds[0] = STDIN_FILENO;
  fds[1] = STDOUT_FILENO;
  fds[2] = STDERR_FILENO;

  /* If the user specified I/O via gdb's --tty= arg, use it, but only
     if the i/o is not also being specified via redirection.  */
  if (inferior_io_terminal)
    {
      if (!in[0])
	in = inferior_io_terminal;
      if (!out[0])
	out = inferior_io_terminal;
      if (!err[0])
	err = inferior_io_terminal;
    }

  if (in[0])
    {
      fd = open (in, O_RDONLY);
      if (fd == -1)
	perror (in);
      else
	fds[0] = fd;
    }
  if (out[0])
    {
      fd = open (out, O_WRONLY);
      if (fd == -1)
	perror (out);
      else
	fds[1] = fd;
    }
  if (err[0])
    {
      fd = open (err, O_WRONLY);
      if (fd == -1)
	perror (err);
      else
	fds[2] = fd;
    }

  /* Clear any pending SIGUSR1's but keep the behavior the same.  */
  signal (SIGUSR1, signal (SIGUSR1, SIG_IGN));

  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigprocmask (SIG_UNBLOCK, &set, NULL);

  memset (&inherit, 0, sizeof (inherit));

  if (ND_NODE_CMP (nto_procfs_node, ND_LOCAL_NODE) != 0)
    {
      inherit.nd = nto_node ();
      inherit.flags |= SPAWN_SETND;
      inherit.flags &= ~SPAWN_EXEC;
    }
  inherit.flags |= SPAWN_SETGROUP | SPAWN_HOLD;
  inherit.pgroup = SPAWN_NEWPGROUP;
  pid = spawnp (argv[0], 3, fds, &inherit, argv,
		ND_NODE_CMP (nto_procfs_node, ND_LOCAL_NODE) == 0 ? env : 0);
  xfree (args);

  sigprocmask (SIG_BLOCK, &set, NULL);

  if (pid == -1)
    error (_("Error spawning %s: %d (%s)"), argv[0], errno,
	   safe_strerror (errno));

  if (fds[0] != STDIN_FILENO)
    close (fds[0]);
  if (fds[1] != STDOUT_FILENO)
    close (fds[1]);
  if (fds[2] != STDERR_FILENO)
    close (fds[2]);

  inferior_ptid = do_attach (pid_to_ptid (pid));
  procfs_update_thread_list (ops);

  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = 0;

  flags = _DEBUG_FLAG_KLC;	/* Kill-on-Last-Close flag.  */
  errn = devctl (ctl_fd, DCMD_PROC_SET_FLAG, &flags, sizeof (flags), 0);
  if (errn != EOK)
    {
      /* FIXME: expected warning?  */
      /* warning( "Failed to set Kill-on-Last-Close flag: errno = %d(%s)\n",
         errn, strerror(errn) ); */
    }
  if (!target_is_pushed (ops))
    push_target (ops);
  target_terminal_init ();

  if (exec_bfd != NULL
      || (symfile_objfile != NULL && symfile_objfile->obfd != NULL))
    solib_create_inferior_hook (0);
}

static void
procfs_interrupt (struct target_ops *self, ptid_t ptid)
{
  devctl (ctl_fd, DCMD_PROC_STOP, NULL, 0, 0);
}

static void
procfs_kill_inferior (struct target_ops *ops)
{
  target_mourn_inferior ();
}

/* Fill buf with regset and return devctl cmd to do the setting.  Return
   -1 if we fail to get the regset.  Store size of regset in regsize.  */
static int
get_regset (int regset, char *buf, int bufsize, int *regsize)
{
  int dev_get, dev_set;
  switch (regset)
    {
    case NTO_REG_GENERAL:
      dev_get = DCMD_PROC_GETGREG;
      dev_set = DCMD_PROC_SETGREG;
      break;

    case NTO_REG_FLOAT:
      dev_get = DCMD_PROC_GETFPREG;
      dev_set = DCMD_PROC_SETFPREG;
      break;

    case NTO_REG_ALT:
      dev_get = DCMD_PROC_GETALTREG;
      dev_set = DCMD_PROC_SETALTREG;
      break;

    case NTO_REG_SYSTEM:
    default:
      return -1;
    }
  if (devctl (ctl_fd, dev_get, buf, bufsize, regsize) != EOK)
    return -1;

  return dev_set;
}

static void
procfs_store_registers (struct target_ops *ops,
			struct regcache *regcache, int regno)
{
  union
  {
    procfs_greg greg;
    procfs_fpreg fpreg;
    procfs_altreg altreg;
  }
  reg;
  unsigned off;
  int len, regset, regsize, dev_set, err;
  char *data;

  if (ptid_equal (inferior_ptid, null_ptid))
    return;
  procfs_set_thread (inferior_ptid);

  if (regno == -1)
    {
      for (regset = NTO_REG_GENERAL; regset < NTO_REG_END; regset++)
	{
	  dev_set = get_regset (regset, (char *) &reg,
				sizeof (reg), &regsize);
	  if (dev_set == -1)
	    continue;

	  if (nto_regset_fill (regcache, regset, (char *) &reg) == -1)
	    continue;

	  err = devctl (ctl_fd, dev_set, &reg, regsize, 0);
	  if (err != EOK)
	    fprintf_unfiltered (gdb_stderr,
				"Warning unable to write regset %d: %s\n",
				regno, safe_strerror (err));
	}
    }
  else
    {
      regset = nto_regset_id (regno);
      if (regset == -1)
	return;

      dev_set = get_regset (regset, (char *) &reg, sizeof (reg), &regsize);
      if (dev_set == -1)
	return;

      len = nto_register_area (get_regcache_arch (regcache),
			       regno, regset, &off);

      if (len < 1)
	return;

      regcache_raw_collect (regcache, regno, (char *) &reg + off);

      err = devctl (ctl_fd, dev_set, &reg, regsize, 0);
      if (err != EOK)
	fprintf_unfiltered (gdb_stderr,
			    "Warning unable to write regset %d: %s\n", regno,
			    safe_strerror (err));
    }
}

/* Set list of signals to be handled in the target.  */

static void
procfs_pass_signals (struct target_ops *self,
		     int numsigs, unsigned char *pass_signals)
{
  int signo;

  sigfillset (&run.trace);

  for (signo = 1; signo < NSIG; signo++)
    {
      int target_signo = gdb_signal_from_host (signo);
      if (target_signo < numsigs && pass_signals[target_signo])
        sigdelset (&run.trace, signo);
    }
}

static char *
procfs_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[1024];
  int pid, tid, n;
  struct tidinfo *tip;

  pid = ptid_get_pid (ptid);
  tid = ptid_get_tid (ptid);

  n = snprintf (buf, 1023, "process %d", pid);

#if 0				/* NYI */
  tip = procfs_thread_info (pid, tid);
  if (tip != NULL)
    snprintf (&buf[n], 1023, " (state = 0x%02x)", tip->state);
#endif

  return buf;
}

/* to_can_run implementation for "target procfs".  Note this really
  means "can this target be the default run target", which there can
  be only one, and we make it be "target native" like other ports.
  "target procfs <node>" wouldn't make sense as default run target, as
  it needs <node>.  */

static int
procfs_can_run (struct target_ops *self)
{
  return 0;
}

/* "target procfs".  */
static struct target_ops nto_procfs_ops;

/* "target native".  */
static struct target_ops *nto_native_ops;

/* to_open implementation for "target procfs".  */

static void
procfs_open (const char *arg, int from_tty)
{
  procfs_open_1 (&nto_procfs_ops, arg, from_tty);
}

/* to_open implementation for "target native".  */

static void
procfs_native_open (const char *arg, int from_tty)
{
  procfs_open_1 (nto_native_ops, arg, from_tty);
}

/* Create the "native" and "procfs" targets.  */

static void
init_procfs_targets (void)
{
  struct target_ops *t = inf_child_target ();

  /* Leave to_shortname as "native".  */
  t->to_longname = "QNX Neutrino local process";
  t->to_doc = "QNX Neutrino local process (started by the \"run\" command).";
  t->to_open = procfs_native_open;
  t->to_attach = procfs_attach;
  t->to_post_attach = procfs_post_attach;
  t->to_detach = procfs_detach;
  t->to_resume = procfs_resume;
  t->to_wait = procfs_wait;
  t->to_fetch_registers = procfs_fetch_registers;
  t->to_store_registers = procfs_store_registers;
  t->to_xfer_partial = procfs_xfer_partial;
  t->to_files_info = procfs_files_info;
  t->to_insert_breakpoint = procfs_insert_breakpoint;
  t->to_remove_breakpoint = procfs_remove_breakpoint;
  t->to_can_use_hw_breakpoint = procfs_can_use_hw_breakpoint;
  t->to_insert_hw_breakpoint = procfs_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = procfs_remove_hw_breakpoint;
  t->to_insert_watchpoint = procfs_insert_hw_watchpoint;
  t->to_remove_watchpoint = procfs_remove_hw_watchpoint;
  t->to_stopped_by_watchpoint = procfs_stopped_by_watchpoint;
  t->to_kill = procfs_kill_inferior;
  t->to_create_inferior = procfs_create_inferior;
  t->to_mourn_inferior = procfs_mourn_inferior;
  t->to_pass_signals = procfs_pass_signals;
  t->to_thread_alive = procfs_thread_alive;
  t->to_update_thread_list = procfs_update_thread_list;
  t->to_pid_to_str = procfs_pid_to_str;
  t->to_interrupt = procfs_interrupt;
  t->to_have_continuable_watchpoint = 1;
  t->to_extra_thread_info = nto_extra_thread_info;
  t->to_pid_to_exec_file = procfs_pid_to_exec_file;

  nto_native_ops = t;

  /* Register "target native".  This is the default run target.  */
  add_target (t);

  /* Register "target procfs <node>".  */
  nto_procfs_ops = *t;
  nto_procfs_ops.to_shortname = "procfs";
  nto_procfs_ops.to_can_run = procfs_can_run;
  t->to_longname = "QNX Neutrino local or remote process";
  t->to_doc = "QNX Neutrino process.  target procfs <node>";
  t->to_open = procfs_open;

  add_target (&nto_procfs_ops);
}

#define OSTYPE_NTO 1

extern initialize_file_ftype _initialize_procfs;

void
_initialize_procfs (void)
{
  sigset_t set;

  init_procfs_targets ();

  /* We use SIGUSR1 to gain control after we block waiting for a process.
     We use sigwaitevent to wait.  */
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigprocmask (SIG_BLOCK, &set, NULL);

  /* Initially, make sure all signals are reported.  */
  sigfillset (&run.trace);

  /* Stuff some information.  */
  nto_cpuinfo_flags = SYSPAGE_ENTRY (cpuinfo)->flags;
  nto_cpuinfo_valid = 1;

  add_info ("pidlist", procfs_pidlist, _("pidlist"));
  add_info ("meminfo", procfs_meminfo, _("memory information"));

  nto_is_nto_target = procfs_is_nto_target;
}


static int
procfs_hw_watchpoint (int addr, int len, enum target_hw_bp_type type)
{
  procfs_break brk;

  switch (type)
    {
    case hw_read:
      brk.type = _DEBUG_BREAK_RD;
      break;
    case hw_access:
      brk.type = _DEBUG_BREAK_RW;
      break;
    default:			/* Modify.  */
/* FIXME: brk.type = _DEBUG_BREAK_RWM gives EINVAL for some reason.  */
      brk.type = _DEBUG_BREAK_RW;
    }
  brk.type |= _DEBUG_BREAK_HW;	/* Always ask for HW.  */
  brk.addr = addr;
  brk.size = len;

  errno = devctl (ctl_fd, DCMD_PROC_BREAK, &brk, sizeof (brk), 0);
  if (errno != EOK)
    {
      perror (_("Failed to set hardware watchpoint"));
      return -1;
    }
  return 0;
}

static int
procfs_can_use_hw_breakpoint (struct target_ops *self,
			      enum bptype type,
			      int cnt, int othertype)
{
  return 1;
}

static int
procfs_remove_hw_watchpoint (struct target_ops *self,
			     CORE_ADDR addr, int len,
			     enum target_hw_bp_type type,
			     struct expression *cond)
{
  return procfs_hw_watchpoint (addr, -1, type);
}

static int
procfs_insert_hw_watchpoint (struct target_ops *self,
			     CORE_ADDR addr, int len,
			     enum target_hw_bp_type type,
			     struct expression *cond)
{
  return procfs_hw_watchpoint (addr, len, type);
}

static int
procfs_stopped_by_watchpoint (struct target_ops *ops)
{
  /* NOTE: nto_stopped_by_watchpoint will be called ONLY while we are
     stopped due to a SIGTRAP.  This assumes gdb works in 'all-stop' mode;
     future gdb versions will likely run in 'non-stop' mode in which case
     we will have to store/examine statuses per thread in question.
     Until then, this will work fine.  */

  struct inferior *inf = current_inferior ();
  struct nto_inferior_data *inf_data;

  gdb_assert (inf != NULL);

  inf_data = nto_inferior_data (inf);

  return inf_data->stopped_flags
	 & (_DEBUG_FLAG_TRACE_RD
	    | _DEBUG_FLAG_TRACE_WR
	    | _DEBUG_FLAG_TRACE_MODIFY);
}
