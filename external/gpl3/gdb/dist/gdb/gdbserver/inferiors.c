/* Inferior process information for the remote server for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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

#include <stdlib.h>

#include "server.h"
#include "gdbthread.h"

struct inferior_list all_processes;
struct inferior_list all_threads;

struct thread_info *current_inferior;

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

void
add_thread (ptid_t thread_id, void *target_data)
{
  struct thread_info *new_thread = xmalloc (sizeof (*new_thread));

  memset (new_thread, 0, sizeof (*new_thread));

  new_thread->entry.id = thread_id;
  new_thread->last_resume_kind = resume_continue;
  new_thread->last_status.kind = TARGET_WAITKIND_IGNORE;

  add_inferior_to_list (&all_threads, & new_thread->entry);

  if (current_inferior == NULL)
    current_inferior = new_thread;

  new_thread->target_data = target_data;
  set_inferior_regcache_data (new_thread, new_register_cache ());
}

ptid_t
thread_id_to_gdb_id (ptid_t thread_id)
{
  struct inferior_list_entry *inf = all_threads.head;

  while (inf != NULL)
    {
      if (ptid_equal (inf->id, thread_id))
	return thread_id;
      inf = inf->next;
    }

  return null_ptid;
}

ptid_t
thread_to_gdb_id (struct thread_info *thread)
{
  return thread->entry.id;
}

struct thread_info *
find_thread_ptid (ptid_t ptid)
{
  struct inferior_list_entry *inf = all_threads.head;

  while (inf != NULL)
    {
      struct thread_info *thread = get_thread (inf);
      if (ptid_equal (thread->entry.id, ptid))
	return thread;
      inf = inf->next;
    }

  return NULL;
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

  remove_inferior (&all_threads, (struct inferior_list_entry *) thread);
  free_one_thread (&thread->entry);
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

void *
inferior_regcache_data (struct thread_info *inferior)
{
  return inferior->regcache_data;
}

void
set_inferior_regcache_data (struct thread_info *inferior, void *data)
{
  inferior->regcache_data = data;
}

#define clear_list(LIST) \
  do { (LIST)->head = (LIST)->tail = NULL; } while (0)

void
clear_inferiors (void)
{
  for_each_inferior (&all_threads, free_one_thread);
  clear_list (&all_threads);

  clear_dlls ();

  current_inferior = NULL;
}

struct process_info *
add_process (int pid, int attached)
{
  struct process_info *process;

  process = xcalloc (1, sizeof (*process));

  process->head.id = pid_to_ptid (pid);
  process->attached = attached;

  add_inferior_to_list (&all_processes, &process->head);

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
  remove_inferior (&all_processes, &process->head);
  free (process);
}

struct process_info *
find_process_pid (int pid)
{
  return (struct process_info *)
    find_inferior_id (&all_processes, pid_to_ptid (pid));
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
get_thread_process (struct thread_info *thread)
{
  int pid = ptid_get_pid (thread->entry.id);
  return find_process_pid (pid);
}

struct process_info *
current_process (void)
{
  if (current_inferior == NULL)
    fatal ("Current inferior requested, but current_inferior is NULL\n");

  return get_thread_process (current_inferior);
}
