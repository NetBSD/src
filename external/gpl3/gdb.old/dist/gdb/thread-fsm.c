/* Thread command's finish-state machine, for GDB, the GNU debugger.
   Copyright (C) 2015-2017 Free Software Foundation, Inc.

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
#include "thread-fsm.h"

/* See thread-fsm.h.  */

void
thread_fsm_ctor (struct thread_fsm *self, struct thread_fsm_ops *ops,
		 struct interp *cmd_interp)
{
  self->command_interp = cmd_interp;
  self->finished = 0;
  self->ops = ops;
}

/* See thread-fsm.h.  */

void
thread_fsm_delete (struct thread_fsm *self)
{
  if (self != NULL)
    {
      if (self->ops->dtor != NULL)
	self->ops->dtor (self);
      xfree (self);
    }
}

/* See thread-fsm.h.  */

void
thread_fsm_clean_up (struct thread_fsm *self, struct thread_info *thread)
{
  if (self->ops->clean_up != NULL)
    self->ops->clean_up (self, thread);
}

/* See thread-fsm.h.  */

int
thread_fsm_should_stop (struct thread_fsm *self, struct thread_info *thread)
{
  return self->ops->should_stop (self, thread);
}

/* See thread-fsm.h.  */

struct return_value_info *
thread_fsm_return_value (struct thread_fsm *self)
{
  if (self->ops->return_value != NULL)
    return self->ops->return_value (self);
  return NULL;
}

/* See thread-fsm.h.  */

void
thread_fsm_set_finished (struct thread_fsm *self)
{
  self->finished = 1;
}

/* See thread-fsm.h.  */

int
thread_fsm_finished_p (struct thread_fsm *self)
{
  return self->finished;
}

/* See thread-fsm.h.  */

enum async_reply_reason
thread_fsm_async_reply_reason (struct thread_fsm *self)
{
  /* If we didn't finish, then the stop reason must come from
     elsewhere.  E.g., a breakpoint hit or a signal intercepted.  */
  gdb_assert (thread_fsm_finished_p (self));

  return self->ops->async_reply_reason (self);
}

/* See thread-fsm.h.  */

int
thread_fsm_should_notify_stop (struct thread_fsm *self)
{
  if (self->ops->should_notify_stop != NULL)
    return self->ops->should_notify_stop (self);
  return 1;
}
