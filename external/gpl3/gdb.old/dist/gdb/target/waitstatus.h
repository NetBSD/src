/* Target waitstatus definitions and prototypes.

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

#ifndef WAITSTATUS_H
#define WAITSTATUS_H

#include "common-utils.h"
#include "ptid.h"
#include "gdb_signals.h"

/* Stuff for target_wait.  */

/* Generally, what has the program done?  */
enum target_waitkind
{
  /* The program has exited.  The exit status is in value.integer.  */
  TARGET_WAITKIND_EXITED,

  /* The program has stopped with a signal.  Which signal is in
     value.sig.  */
  TARGET_WAITKIND_STOPPED,

  /* The program has terminated with a signal.  Which signal is in
     value.sig.  */
  TARGET_WAITKIND_SIGNALLED,

  /* The program is letting us know that it dynamically loaded
     something (e.g. it called load(2) on AIX).  */
  TARGET_WAITKIND_LOADED,

  /* The program has forked.  A "related" process' PTID is in
     value.related_pid.  I.e., if the child forks, value.related_pid
     is the parent's ID.  */
  TARGET_WAITKIND_FORKED,
 
  /* The program has vforked.  A "related" process's PTID is in
     value.related_pid.  */
  TARGET_WAITKIND_VFORKED,
 
  /* The program has exec'ed a new executable file.  The new file's
     pathname is pointed to by value.execd_pathname.  */
  TARGET_WAITKIND_EXECD,
  
  /* The program had previously vforked, and now the child is done
     with the shared memory region, because it exec'ed or exited.
     Note that the event is reported to the vfork parent.  This is
     only used if GDB did not stay attached to the vfork child,
     otherwise, a TARGET_WAITKIND_EXECD or
     TARGET_WAITKIND_EXIT|SIGNALLED event associated with the child
     has the same effect.  */
  TARGET_WAITKIND_VFORK_DONE,

  /* The program has entered or returned from a system call.  On
     HP-UX, this is used in the hardware watchpoint implementation.
     The syscall's unique integer ID number is in
     value.syscall_id.  */
  TARGET_WAITKIND_SYSCALL_ENTRY,
  TARGET_WAITKIND_SYSCALL_RETURN,

  /* Nothing happened, but we stopped anyway.  This perhaps should
     be handled within target_wait, but I'm not sure target_wait
     should be resuming the inferior.  */
  TARGET_WAITKIND_SPURIOUS,

  /* An event has occured, but we should wait again.
     Remote_async_wait() returns this when there is an event
     on the inferior, but the rest of the world is not interested in
     it.  The inferior has not stopped, but has just sent some output
     to the console, for instance.  In this case, we want to go back
     to the event loop and wait there for another event from the
     inferior, rather than being stuck in the remote_async_wait()
     function.  This way the event loop is responsive to other events,
     like for instance the user typing.  */
  TARGET_WAITKIND_IGNORE,
 
  /* The target has run out of history information,
     and cannot run backward any further.  */
  TARGET_WAITKIND_NO_HISTORY,
 
  /* There are no resumed children left in the program.  */
  TARGET_WAITKIND_NO_RESUMED
};

struct target_waitstatus
{
  enum target_waitkind kind;

  /* Additional information about the event.  */
  union
    {
      /* Exit status */
      int integer;
      /* Signal number */
      enum gdb_signal sig;
      /* Forked child pid */
      ptid_t related_pid;
      /* execd pathname */
      char *execd_pathname;
      /* Syscall number */
      int syscall_number;
    } value;
};

/* Prototypes */

/* Return a pretty printed form of target_waitstatus.
   Space for the result is malloc'd, caller must free.  */
extern char *target_waitstatus_to_string (const struct target_waitstatus *);

#endif /* WAITSTATUS_H */
