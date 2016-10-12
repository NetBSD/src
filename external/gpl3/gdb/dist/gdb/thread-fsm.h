/* Thread command's finish-state machine, for GDB, the GNU debugger.
   Copyright (C) 2015-2016 Free Software Foundation, Inc.

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

#ifndef THREAD_FSM_H
#define THREAD_FSM_H

#include "mi/mi-common.h" /* For enum async_reply_reason.  */

struct return_value_info;
struct thread_fsm_ops;

/* A thread finite-state machine structure contains the necessary info
   and callbacks to manage the state machine protocol of a thread's
   execution command.  */

struct thread_fsm
{
  /* Pointer of the virtual table of methods.  */
  struct thread_fsm_ops *ops;

  /* Whether the FSM is done successfully.  */
  int finished;

  /* The interpreter that issued the execution command that caused
     this thread to resume.  If the top level interpreter is MI/async,
     and the execution command was a CLI command (next/step/etc.),
     we'll want to print stop event output to the MI console channel
     (the stepped-to line, etc.), as if the user entered the execution
     command on a real GDB console.  */
  struct interp *command_interp;
};

/* The virtual table of a thread_fsm.  */

struct thread_fsm_ops
{
  /* The destructor.  This should simply free heap allocated data
     structures.  Cleaning up target resources (like, e.g.,
     breakpoints) should be done in the clean_up method.  */
  void (*dtor) (struct thread_fsm *self);

  /* Called to clean up target resources after the FSM.  E.g., if the
     FSM created internal breakpoints, this is where they should be
     deleted.  */
  void (*clean_up) (struct thread_fsm *self, struct thread_info *thread);

  /* Called after handle_inferior_event decides the target is done
     (that is, after stop_waiting).  The FSM is given a chance to
     decide whether the command is done and thus the target should
     stop, or whether there's still more to do and thus the thread
     should be re-resumed.  This is a good place to cache target data
     too.  For example, the "finish" command saves the just-finished
     function's return value here.  */
  int (*should_stop) (struct thread_fsm *self, struct thread_info *thread);

  /* If this FSM saved a function's return value, you can use this
     method to retrieve it.  Otherwise, this returns NULL.  */
  struct return_value_info *(*return_value) (struct thread_fsm *self);

  /* The async_reply_reason that is broadcast to MI clients if this
     FSM finishes successfully.  */
  enum async_reply_reason (*async_reply_reason) (struct thread_fsm *self);

  /* Whether the stop should be notified to the user/frontend.  */
  int (*should_notify_stop) (struct thread_fsm *self);
};
/* Initialize FSM.  */
extern void thread_fsm_ctor (struct thread_fsm *self,
			     struct thread_fsm_ops *ops,
			     struct interp *cmd_interp);

/* Calls the FSM's dtor method, and then frees FSM.  */
extern void thread_fsm_delete (struct thread_fsm *fsm);

/* Calls the FSM's clean_up method.  */
extern void thread_fsm_clean_up (struct thread_fsm *fsm,
				 struct thread_info *thread);

/* Calls the FSM's should_stop method.  */
extern int thread_fsm_should_stop (struct thread_fsm *fsm,
				   struct thread_info *thread);

/* Calls the FSM's return_value method.  */
extern struct return_value_info *
  thread_fsm_return_value (struct thread_fsm *fsm);

/* Marks the FSM as completed successfully.  */
extern void thread_fsm_set_finished (struct thread_fsm *fsm);

/* Returns true if the FSM completed successfully.  */
extern int thread_fsm_finished_p (struct thread_fsm *fsm);

/* Calls the FSM's reply_reason method.  */
extern enum async_reply_reason
  thread_fsm_async_reply_reason (struct thread_fsm *fsm);

/* Calls the FSM's should_notify_stop method.  */
extern int thread_fsm_should_notify_stop (struct thread_fsm *self);

#endif /* THREAD_FSM_H */
