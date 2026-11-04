/* Linux-specific PROCFS manipulation routines.
   Copyright (C) 2011-2025 Free Software Foundation, Inc.

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

#ifndef GDB_NAT_LINUX_PROCFS_H
#define GDB_NAT_LINUX_PROCFS_H

#include <unistd.h>

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  Failure to open the /proc file results in a warning.  */

extern int linux_proc_get_tgid (pid_t lwpid);

/* Return the TracerPid of LWPID from /proc/pid/status.  Returns -1 if
   not found.  Does not warn on failure to open the /proc file.  */

extern pid_t linux_proc_get_tracerpid_nowarn (pid_t lwpid);

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

extern int linux_proc_pid_is_stopped (pid_t pid);

extern int linux_proc_pid_is_trace_stopped_nowarn (pid_t pid);

/* Return non-zero if PID is a zombie.  Failure to open the
   /proc/pid/status file results in a warning.  */

extern int linux_proc_pid_is_zombie (pid_t pid);

/* Return non-zero if PID is a zombie.  Does not warn on failure to
   open the /proc file.  */

extern int linux_proc_pid_is_zombie_nowarn (pid_t pid);

/* Return non-zero if /proc/PID/status indicates that PID is gone
   (i.e., in Z/Zombie or X/Dead state).  Failure to open the /proc
   file is assumed to indicate the thread is gone.  */

extern int linux_proc_pid_is_gone (pid_t pid);

/* Index of fields of interest in /proc/PID/stat, from procfs(5) man page.  */
#define LINUX_PROC_STAT_STATE 3
#define LINUX_PROC_STAT_STARTTIME 22
#define LINUX_PROC_STAT_PROCESSOR 39

/* Returns FIELD (as numbered in procfs(5) man page) of
   /proc/PID/task/LWP/stat file.  */

extern std::optional<std::string> linux_proc_get_stat_field (ptid_t ptid,
							     int field);

/* Return a string giving the thread's name or NULL if the
   information is unavailable.  The returned value points to a statically
   allocated buffer.  The value therefore becomes invalid at the next
   linux_proc_tid_get_name call.  */

extern const char *linux_proc_tid_get_name (ptid_t ptid);

/* Callback function for linux_proc_attach_tgid_threads.  If the PTID
   thread is not yet known, try to attach to it and return true,
   otherwise return false.  */
typedef int (*linux_proc_attach_lwp_func) (ptid_t ptid);

/* If PID is a tgid, scan the /proc/PID/task/ directory for existing
   threads, and call FUNC for each thread found.  */
extern void linux_proc_attach_tgid_threads (pid_t pid,
					    linux_proc_attach_lwp_func func);

/* Return true if the /proc/PID/task/ directory exists.  */
extern int linux_proc_task_list_dir_exists (pid_t pid);

/* Return the full absolute name of the executable file that was run
   to create the process PID.  The returned value persists until this
   function is next called.

   LOCAL_FS should be true if the file returned from the function will
   be searched for in the same filesystem as GDB itself is running.
   In practice, this means LOCAL_FS should be true if PID and GDB are
   running in the same MNT namespace and GDB's sysroot is either the
   empty string, or is 'target:'.

   When used from gdbserver, where there is no sysroot, the only check
   that matters is that PID and gdbserver are running in the same MNT
   namespace.  */

extern const char *linux_proc_pid_to_exec_file (int pid, bool local_fs);

/* Display possible problems on this system.  Display them only once
   per GDB execution.  */

extern void linux_proc_init_warnings ();

#endif /* GDB_NAT_LINUX_PROCFS_H */
