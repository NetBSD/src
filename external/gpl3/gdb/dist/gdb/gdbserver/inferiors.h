/* Inferior process information for the remote server for GDB.
   Copyright (C) 1993-2014 Free Software Foundation, Inc.

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

#ifndef INFERIORS_H
#define INFERIORS_H

/* Generic information for tracking a list of ``inferiors'' - threads,
   processes, etc.  */
struct inferior_list
{
  struct inferior_list_entry *head;
  struct inferior_list_entry *tail;
};
struct inferior_list_entry
{
  ptid_t id;
  struct inferior_list_entry *next;
};

struct thread_info;
struct target_desc;
struct sym_cache;
struct breakpoint;
struct raw_breakpoint;
struct fast_tracepoint_jump;
struct process_info_private;

struct process_info
{
  struct inferior_list_entry head;

  /* Nonzero if this child process was attached rather than
     spawned.  */
  int attached;

  /* True if GDB asked us to detach from this process, but we remained
     attached anyway.  */
  int gdb_detached;

  /* The symbol cache.  */
  struct sym_cache *symbol_cache;

  /* The list of memory breakpoints.  */
  struct breakpoint *breakpoints;

  /* The list of raw memory breakpoints.  */
  struct raw_breakpoint *raw_breakpoints;

  /* The list of installed fast tracepoints.  */
  struct fast_tracepoint_jump *fast_tracepoint_jumps;

  const struct target_desc *tdesc;

  /* Private target data.  */
  struct process_info_private *private;
};

/* Return a pointer to the process that corresponds to the current
   thread (current_inferior).  It is an error to call this if there is
   no current thread selected.  */

struct process_info *current_process (void);
struct process_info *get_thread_process (struct thread_info *);

extern struct inferior_list all_processes;

void add_inferior_to_list (struct inferior_list *list,
			   struct inferior_list_entry *new_inferior);
void for_each_inferior (struct inferior_list *list,
			void (*action) (struct inferior_list_entry *));

extern struct thread_info *current_inferior;
void remove_inferior (struct inferior_list *list,
		      struct inferior_list_entry *entry);

struct process_info *add_process (int pid, int attached);
void remove_process (struct process_info *process);
struct process_info *find_process_pid (int pid);
int have_started_inferiors_p (void);
int have_attached_inferiors_p (void);

ptid_t thread_id_to_gdb_id (ptid_t);
ptid_t thread_to_gdb_id (struct thread_info *);
ptid_t gdb_id_to_thread_id (ptid_t);

void clear_inferiors (void);
struct inferior_list_entry *find_inferior
     (struct inferior_list *,
      int (*func) (struct inferior_list_entry *,
		   void *),
      void *arg);
struct inferior_list_entry *find_inferior_id (struct inferior_list *list,
					      ptid_t id);
void *inferior_target_data (struct thread_info *);
void set_inferior_target_data (struct thread_info *, void *);
void *inferior_regcache_data (struct thread_info *);
void set_inferior_regcache_data (struct thread_info *, void *);

#endif /* INFERIORS_H */
