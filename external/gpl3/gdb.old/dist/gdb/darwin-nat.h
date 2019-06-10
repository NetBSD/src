/* Common things used by the various darwin files
   Copyright (C) 1995-2017 Free Software Foundation, Inc.

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

#ifndef __DARWIN_NAT_H__
#define __DARWIN_NAT_H__

#include <mach/mach.h>

/* Describe the mach exception handling state for a task.  This state is saved
   before being changed and restored when a process is detached.
   For more information on these fields see task_get_exception_ports manual
   page.  */
struct darwin_exception_info
{
  /* Exceptions handled by the port.  */
  exception_mask_t masks[EXC_TYPES_COUNT];

  /* Ports receiving exception messages.  */
  mach_port_t ports[EXC_TYPES_COUNT];

  /* Type of messages sent.  */
  exception_behavior_t behaviors[EXC_TYPES_COUNT];

  /* Type of state to be sent.  */
  thread_state_flavor_t flavors[EXC_TYPES_COUNT];

  /* Number of elements set.  */
  mach_msg_type_number_t count;
};
typedef struct darwin_exception_info darwin_exception_info;

struct darwin_exception_msg
{
  mach_msg_header_t header;

  /* Thread and task taking the exception.  */
  mach_port_t thread_port;
  mach_port_t task_port;

  /* Type of the exception.  */
  exception_type_t ex_type;

  /* Machine dependent details.  */
  mach_msg_type_number_t data_count;
  integer_t ex_data[2];
};

enum darwin_msg_state
{
  /* The thread is running.  */
  DARWIN_RUNNING,

  /* The thread is stopped.  */
  DARWIN_STOPPED,

  /* The thread has sent a message and waits for a reply.  */
  DARWIN_MESSAGE
};

struct private_thread_info
{
  /* The thread port from a GDB point of view.  */
  thread_t gdb_port;

  /* The thread port from the inferior point of view.  Not to be used inside
     gdb except for get_ada_task_ptid.  */
  thread_t inf_port;

  /* Current message state.
     If the kernel has sent a message it expects a reply and the inferior
     can't be killed before.  */
  enum darwin_msg_state msg_state;

  /* True if this thread is single-stepped.  */
  unsigned char single_step;

  /* True if a signal was manually sent to the thread.  */
  unsigned char signaled;

  /* The last exception received.  */
  struct darwin_exception_msg event;
};
typedef struct private_thread_info darwin_thread_t;

/* Define the threads vector type.  */
DEF_VEC_O (darwin_thread_t);


/* Describe an inferior.  */
struct private_inferior
{
  /* Corresponding task port.  */
  task_t task;

  /* Port which will receive the dead-name notification for the task port.
     This is used to detect the death of the task.  */
  mach_port_t notify_port;

  /* Initial exception handling.  */
  darwin_exception_info exception_info;

  /* Number of messages that have been received but not yet replied.  */
  unsigned int pending_messages;

  /* Set if inferior is not controlled by ptrace(2) but through Mach.  */
  unsigned char no_ptrace;

  /* True if this task is suspended.  */
  unsigned char suspended;

  /* Sorted vector of known threads.  */
  VEC(darwin_thread_t) *threads;
};
typedef struct private_inferior darwin_inferior;

/* Exception port.  */
extern mach_port_t darwin_ex_port;

/* Port set.  */
extern mach_port_t darwin_port_set;

/* A copy of mach_host_self ().  */
extern mach_port_t darwin_host_self;

/* FUNCTION_NAME is defined in common-utils.h (or not).  */
#ifdef FUNCTION_NAME
#define MACH_CHECK_ERROR(ret) \
  mach_check_error (ret, __FILE__, __LINE__, FUNCTION_NAME)
#else
#define MACH_CHECK_ERROR(ret) \
  mach_check_error (ret, __FILE__, __LINE__, "??")
#endif

extern void mach_check_error (kern_return_t ret, const char *file,
			      unsigned int line, const char *func);

void darwin_set_sstep (thread_t thread, int enable);

/* This one is called in darwin-nat.c, but needs to be provided by the
   platform specific nat code.  It allows each platform to add platform specific
   stuff to the darwin_ops.  */
extern void darwin_complete_target (struct target_ops *target);

void darwin_check_osabi (darwin_inferior *inf, thread_t thread);

#endif /* __DARWIN_NAT_H__ */
