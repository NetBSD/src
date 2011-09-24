/* Inferior process information for the remote server for GDB.
   Copyright (C) 2002, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

struct inferior_list all_processes;
struct inferior_list all_threads;
struct inferior_list all_dlls;
int dlls_changed;

struct thread_info *current_inferior;


/* Oft used ptids */
ptid_t null_ptid;
ptid_t minus_one_ptid;

/* Create a ptid given the necessary PID, LWP, and TID components.  */

ptid_t
ptid_build (int pid, long lwp, long tid)
{
  ptid_t ptid;

  ptid.pid = pid;
  ptid.lwp = lwp;
  ptid.tid = tid;
  return ptid;
}

/* Create a ptid from just a pid.  */

ptid_t
pid_to_ptid (int pid)
{
  return ptid_build (pid, 0, 0);
}

/* Fetch the pid (process id) component from a ptid.  */

int
ptid_get_pid (ptid_t ptid)
{
  return ptid.pid;
}

/* Fetch the lwp (lightweight process) component from a ptid.  */

long
ptid_get_lwp (ptid_t ptid)
{
  return ptid.lwp;
}

/* Fetch the tid (thread id) component from a ptid.  */

long
ptid_get_tid (ptid_t ptid)
{
  return ptid.tid;
}

/* ptid_equal() is used to test equality of two ptids.  */

int
ptid_equal (ptid_t ptid1, ptid_t ptid2)
{
  return (ptid1.pid == ptid2.pid
	  && ptid1.lwp == ptid2.lwp
	  && ptid1.tid == ptid2.tid);
}

/* Return true if this ptid represents a process.  */

int
ptid_is_pid (ptid_t ptid)
{
  if (ptid_equal (minus_one_ptid, ptid))
    return 0;
  if (ptid_equal (null_ptid, ptid))
    return 0;

  return (ptid_get_pid (ptid) != 0
	  && ptid_get_lwp (ptid) == 0
	  && ptid_get_tid (ptid) == 0);
}

#define get_thread(inf) ((struct thread_info *)(inf))
#define get_dll(inf) ((struct dll_info *)(inf))

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

static void
free_one_dll (struct inferior_list_entry *inf)
{
  struct dll_info *dll = get_dll (inf);
  if (dll->name != NULL)
    free (dll->name);
  free (dll);
}

/* Find a DLL with the same name and/or base address.  A NULL name in
   the key is ignored; so is an all-ones base address.  */

static int
match_dll (struct inferior_list_entry *inf, void *arg)
{
  struct dll_info *iter = (void *) inf;
  struct dll_info *key = arg;

  if (key->base_addr != ~(CORE_ADDR) 0
      && iter->base_addr == key->base_addr)
    return 1;
  else if (key->name != NULL
	   && iter->name != NULL
	   && strcmp (key->name, iter->name) == 0)
    return 1;

  return 0;
}

/* Record a newly loaded DLL at BASE_ADDR.  */

void
loaded_dll (const char *name, CORE_ADDR base_addr)
{
  struct dll_info *new_dll = xmalloc (sizeof (*new_dll));
  memset (new_dll, 0, sizeof (*new_dll));

  new_dll->entry.id = minus_one_ptid;

  new_dll->name = xstrdup (name);
  new_dll->base_addr = base_addr;

  add_inferior_to_list (&all_dlls, &new_dll->entry);
  dlls_changed = 1;
}

/* Record that the DLL with NAME and BASE_ADDR has been unloaded.  */

void
unloaded_dll (const char *name, CORE_ADDR base_addr)
{
  struct dll_info *dll;
  struct dll_info key_dll;

  /* Be careful not to put the key DLL in any list.  */
  key_dll.name = (char *) name;
  key_dll.base_addr = base_addr;

  dll = (void *) find_inferior (&all_dlls, match_dll, &key_dll);

  if (dll == NULL)
    /* For some inferiors we might get unloaded_dll events without having
       a corresponding loaded_dll.  In that case, the dll cannot be found
       in ALL_DLL, and there is nothing further for us to do.

       This has been observed when running 32bit executables on Windows64
       (i.e. through WOW64, the interface between the 32bits and 64bits
       worlds).  In that case, the inferior always does some strange
       unloading of unnamed dll.  */
    return;
  else
    {
      /* DLL has been found so remove the entry and free associated
         resources.  */
      remove_inferior (&all_dlls, &dll->entry);
      free_one_dll (&dll->entry);
      dlls_changed = 1;
    }
}

#define clear_list(LIST) \
  do { (LIST)->head = (LIST)->tail = NULL; } while (0)

void
clear_inferiors (void)
{
  for_each_inferior (&all_threads, free_one_thread);
  for_each_inferior (&all_dlls, free_one_dll);

  clear_list (&all_threads);
  clear_list (&all_dlls);

  current_inferior = NULL;
}

/* Two utility functions for a truly degenerate inferior_list: a simple
   PID listing.  */

void
add_pid_to_list (struct inferior_list *list, unsigned long pid)
{
  struct inferior_list_entry *new_entry;

  new_entry = xmalloc (sizeof (struct inferior_list_entry));
  new_entry->id = pid_to_ptid (pid);
  add_inferior_to_list (list, new_entry);
}

int
pull_pid_from_list (struct inferior_list *list, unsigned long pid)
{
  struct inferior_list_entry *new_entry;

  new_entry = find_inferior_id (list, pid_to_ptid (pid));
  if (new_entry == NULL)
    return 0;
  else
    {
      remove_inferior (list, new_entry);
      free (new_entry);
      return 1;
    }
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

void
initialize_inferiors (void)
{
  null_ptid = ptid_build (0, 0, 0);
  minus_one_ptid = ptid_build (-1, 0, 0);
}
