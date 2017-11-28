/* Inferior process information for the remote server for GDB.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#include "server.h"
#include "gdbthread.h"
#include "dll.h"

struct inferior_list all_processes;
struct inferior_list all_threads;

struct thread_info *current_thread;

#define get_thread(inf) ((struct thread_info *)(inf))

void
add_inferior_to_list (struct inferior_list *list,
		      struct inferior_list_entry *new_inferior)
{
  new_inferior->next = NULL;
  if (list->tail != NULL)
    list->tail->next = new_inferior;
  else
    list->head = new_inferior;
  list->tail = new_inferior;
}

/* Invoke ACTION for each inferior in LIST.  */

void
for_each_inferior (struct inferior_list *list,
		   void (*action) (struct inferior_list_entry *))
{
  struct inferior_list_entry *cur = list->head, *next;

  while (cur != NULL)
    {
      next = cur->next;
      (*action) (cur);
      cur = next;
    }
}

/* Invoke ACTION for each inferior in LIST, passing DATA to ACTION.  */

void
for_each_inferior_with_data (struct inferior_list *list,
			     void (*action) (struct inferior_list_entry *,
					     void *),
			     void *data)
{
  struct inferior_list_entry *cur = list->head, *next;

  while (cur != NULL)
    {
      next = cur->next;
      (*action) (cur, data);
      cur = next;
    }
}

void
remove_inferior (struct inferior_list *list,
		 struct inferior_list_entry *entry)
{
  struct inferior_list_entry **cur;

  if (list->head == entry)
    {
      list->head = entry->next;
      if (list->tail == entry)
	list->tail = list->head;
      return;
    }

  cur = &list->head;
  while (*cur && (*cur)->next != entry)
    cur = &(*cur)->next;

  if (*cur == NULL)
    return;

  (*cur)->next = entry->next;

  if (list->tail == entry)
    list->tail = *cur;
}

struct thread_info *
add_thread (ptid_t thread_id, void *target_data)
{
  struct thread_info *new_thread = XCNEW (struct thread_info);

  new_thread->entry.id = thread_id;
  new_thread->last_resume_kind = resume_continue;
  new_thread->last_status.kind = TARGET_WAITKIND_IGNORE;

  add_inferior_to_list (&all_threads, &new_thread->entry);

  if (current_thread == NULL)
    current_thread = new_thread;

  new_thread->target_data = target_data;

  return new_thread;
}

ptid_t
thread_to_gdb_id (struct thread_info *thread)
{
  return thread->entry.id;
}

/* Wrapper around get_first_inferior to return a struct thread_info *.  */

struct thread_info *
get_first_thread (void)
{
  return (struct thread_info *) get_first_inferior (&all_threads);
}

struct thread_info *
find_thread_ptid (ptid_t ptid)
{
  return (struct thread_info *) find_inferior_id (&all_threads, ptid);
}

/* Predicate function for matching thread entry's pid to the given
   pid value passed by address in ARGS.  */

static int
thread_pid_matches_callback (struct inferior_list_entry *entry, void *args)
{
  return (ptid_get_pid (entry->id) == *(pid_t *)args);
}

/* Find a thread associated with the given PROCESS, or NULL if no
   such thread exists.  */

static struct thread_info *
find_thread_process (const struct process_info *const process)
{
  pid_t pid = ptid_get_pid (ptid_of (process));

  return (struct thread_info *)
    find_inferior (&all_threads, thread_pid_matches_callback, &pid);
}

/* Helper for find_any_thread_of_pid.  Returns true if a thread
   matches a PID.  */

static int
thread_of_pid (struct inferior_list_entry *entry, void *pid_p)
{
  int pid = *(int *) pid_p;

  return (ptid_get_pid (entry->id) == pid);
}

/* See gdbthread.h.  */

struct thread_info *
find_any_thread_of_pid (int pid)
{
  struct inferior_list_entry *entry;

  entry = find_inferior (&all_threads, thread_of_pid, &pid);

  return (struct thread_info *) entry;
}

ptid_t
gdb_id_to_thread_id (ptid_t gdb_id)
{
  struct thread_info *thread = find_thread_ptid (gdb_id);

  return thread ? thread->entry.id : null_ptid;
}

static void
free_one_thread (struct inferior_list_entry *inf)
{
  struct thread_info *thread = get_thread (inf);
  free_register_cache (inferior_regcache_data (thread));
  free (thread);
}

void
remove_thread (struct thread_info *thread)
{
  if (thread->btrace != NULL)
    target_disable_btrace (thread->btrace);

  discard_queued_stop_replies (ptid_of (thread));
  remove_inferior (&all_threads, (struct inferior_list_entry *) thread);
  free_one_thread (&thread->entry);
  if (current_thread == thread)
    current_thread = NULL;
}

/* Return a pointer to the first inferior in LIST, or NULL if there isn't one.
   This is for cases where the caller needs a thread, but doesn't care
   which one.  */

struct inferior_list_entry *
get_first_inferior (struct inferior_list *list)
{
  if (list->head != NULL)
    return list->head;
  return NULL;
}

/* Find the first inferior_list_entry E in LIST for which FUNC (E, ARG)
   returns non-zero.  If no entry is found then return NULL.  */

struct inferior_list_entry *
find_inferior (struct inferior_list *list,
	       int (*func) (struct inferior_list_entry *, void *), void *arg)
{
  struct inferior_list_entry *inf = list->head;

  while (inf != NULL)
    {
      struct inferior_list_entry *next;

      next = inf->next;
      if ((*func) (inf, arg))
	return inf;
      inf = next;
    }

  return NULL;
}

struct inferior_list_entry *
find_inferior_id (struct inferior_list *list, ptid_t id)
{
  struct inferior_list_entry *inf = list->head;

  while (inf != NULL)
    {
      if (ptid_equal (inf->id, id))
	return inf;
      inf = inf->next;
    }

  return NULL;
}

void *
inferior_target_data (struct thread_info *inferior)
{
  return inferior->target_data;
}

void
set_inferior_target_data (struct thread_info *inferior, void *data)
{
  inferior->target_data = data;
}

struct regcache *
inferior_regcache_data (struct thread_info *inferior)
{
  return inferior->regcache_data;
}

void
set_inferior_regcache_data (struct thread_info *inferior, struct regcache *data)
{
  inferior->regcache_data = data;
}

/* Return true if LIST has exactly one entry.  */

int
one_inferior_p (struct inferior_list *list)
{
  return list->head != NULL && list->head == list->tail;
}

/* Reset head,tail of LIST, assuming all entries have already been freed.  */

void
clear_inferior_list (struct inferior_list *list)
{
  list->head = NULL;
  list->tail = NULL;
}

void
clear_inferiors (void)
{
  for_each_inferior (&all_threads, free_one_thread);
  clear_inferior_list (&all_threads);

  clear_dlls ();

  current_thread = NULL;
}

struct process_info *
add_process (int pid, int attached)
{
  struct process_info *process = XCNEW (struct process_info);

  process->entry.id = pid_to_ptid (pid);
  process->attached = attached;

  add_inferior_to_list (&all_processes, &process->entry);

  return process;
}

/* Remove a process from the common process list and free the memory
   allocated for it.
   The caller is responsible for freeing private data first.  */

void
remove_process (struct process_info *process)
{
  clear_symbol_cache (&process->symbol_cache);
  free_all_breakpoints (process);
  gdb_assert (find_thread_process (process) == NULL);
  remove_inferior (&all_processes, &process->entry);
  VEC_free (int, process->syscalls_to_catch);
  free (process);
}

struct process_info *
find_process_pid (int pid)
{
  return (struct process_info *)
    find_inferior_id (&all_processes, pid_to_ptid (pid));
}

/* Wrapper around get_first_inferior to return a struct process_info *.  */

struct process_info *
get_first_process (void)
{
  return (struct process_info *) get_first_inferior (&all_processes);
}

/* Return non-zero if INF, a struct process_info, was started by us,
   i.e. not attached to.  */

static int
started_inferior_callback (struct inferior_list_entry *entry, void *args)
{
  struct process_info *process = (struct process_info *) entry;

  return ! process->attached;
}

/* Return non-zero if there are any inferiors that we have created
   (as opposed to attached-to).  */

int
have_started_inferiors_p (void)
{
  return (find_inferior (&all_processes, started_inferior_callback, NULL)
	  != NULL);
}

/* Return non-zero if INF, a struct process_info, was attached to.  */

static int
attached_inferior_callback (struct inferior_list_entry *entry, void *args)
{
  struct process_info *process = (struct process_info *) entry;

  return process->attached;
}

/* Return non-zero if there are any inferiors that we have attached to.  */

int
have_attached_inferiors_p (void)
{
  return (find_inferior (&all_processes, attached_inferior_callback, NULL)
	  != NULL);
}

struct process_info *
get_thread_process (const struct thread_info *thread)
{
  int pid = ptid_get_pid (thread->entry.id);
  return find_process_pid (pid);
}

struct process_info *
current_process (void)
{
  gdb_assert (current_thread != NULL);
  return get_thread_process (current_thread);
}

static void
do_restore_current_thread_cleanup (void *arg)
{
  current_thread = (struct thread_info *) arg;
}

struct cleanup *
make_cleanup_restore_current_thread (void)
{
  return make_cleanup (do_restore_current_thread_cleanup, current_thread);
}
