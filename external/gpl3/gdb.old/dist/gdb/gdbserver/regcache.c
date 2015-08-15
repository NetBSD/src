/* Register support routines for the remote server for GDB.
   Copyright (C) 2001-2014 Free Software Foundation, Inc.

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
#include "regdef.h"
#include "gdbthread.h"
#include "tdesc.h"

#include <stdlib.h>
#include <string.h>

#ifndef IN_PROCESS_AGENT

struct regcache *
get_thread_regcache (struct thread_info *thread, int fetch)
{
  struct regcache *regcache;

  regcache = (struct regcache *) inferior_regcache_data (thread);

  /* Threads' regcaches are created lazily, because biarch targets add
     the main thread/lwp before seeing it stop for the first time, and
     it is only after the target sees the thread stop for the first
     time that the target has a chance of determining the process's
     architecture.  IOW, when we first add the process's main thread
     we don't know which architecture/tdesc its regcache should
     have.  */
  if (regcache == NULL)
    {
      struct process_info *proc = get_thread_process (thread);

      if (proc->tdesc == NULL)
	fatal ("no target description");

      regcache = new_register_cache (proc->tdesc);
      set_inferior_regcache_data (thread, regcache);
    }

  if (fetch && regcache->registers_valid == 0)
    {
      struct thread_info *saved_inferior = current_inferior;

      current_inferior = thread;
      fetch_inferior_registers (regcache, -1);
      current_inferior = saved_inferior;
      regcache->registers_valid = 1;
    }

  return regcache;
}

void
regcache_invalidate_thread (struct thread_info *thread)
{
  struct regcache *regcache;

  regcache = (struct regcache *) inferior_regcache_data (thread);

  if (regcache == NULL)
    return;

  if (regcache->registers_valid)
    {
      struct thread_info *saved_inferior = current_inferior;

      current_inferior = thread;
      store_inferior_registers (regcache, -1);
      current_inferior = saved_inferior;
    }

  regcache->registers_valid = 0;
}

static int
regcache_invalidate_one (struct inferior_list_entry *entry,
			 void *pid_p)
{
  struct thread_info *thread = (struct thread_info *) entry;
  int pid = *(int *) pid_p;

  /* Only invalidate the regcaches of threads of this process.  */
  if (ptid_get_pid (entry->id) == pid)
    regcache_invalidate_thread (thread);

  return 0;
}

void
regcache_invalidate (void)
{
  /* Only update the threads of the current process.  */
  int pid = ptid_get_pid (current_inferior->entry.id);

  find_inferior (&all_threads, regcache_invalidate_one, &pid);
}

#endif

struct regcache *
init_register_cache (struct regcache *regcache,
		     const struct target_desc *tdesc,
		     unsigned char *regbuf)
{
#ifndef IN_PROCESS_AGENT
  if (regbuf == NULL)
    {
      /* Make sure to zero-initialize the register cache when it is
	 created, in case there are registers the target never
	 fetches.  This way they'll read as zero instead of
	 garbage.  */
      regcache->tdesc = tdesc;
      regcache->registers = xcalloc (1, tdesc->registers_size);
      regcache->registers_owned = 1;
      regcache->register_status = xcalloc (1, tdesc->num_registers);
      gdb_assert (REG_UNAVAILABLE == 0);
    }
  else
#else
  if (regbuf == NULL)
    fatal ("init_register_cache: can't allocate memory from the heap");
  else
#endif
    {
      regcache->tdesc = tdesc;
      regcache->registers = regbuf;
      regcache->registers_owned = 0;
#ifndef IN_PROCESS_AGENT
      regcache->register_status = NULL;
#endif
    }

  regcache->registers_valid = 0;

  return regcache;
}

#ifndef IN_PROCESS_AGENT

struct regcache *
new_register_cache (const struct target_desc *tdesc)
{
  struct regcache *regcache;

  gdb_assert (tdesc->registers_size != 0);

  regcache = xmalloc (sizeof (*regcache));
  return init_register_cache (regcache, tdesc, NULL);
}

void
free_register_cache (struct regcache *regcache)
{
  if (regcache)
    {
      if (regcache->registers_owned)
	free (regcache->registers);
      free (regcache->register_status);
      free (regcache);
    }
}

#endif

void
regcache_cpy (struct regcache *dst, struct regcache *src)
{
  gdb_assert (src != NULL && dst != NULL);
  gdb_assert (src->tdesc == dst->tdesc);
  gdb_assert (src != dst);

  memcpy (dst->registers, src->registers, src->tdesc->registers_size);
#ifndef IN_PROCESS_AGENT
  if (dst->register_status != NULL && src->register_status != NULL)
    memcpy (dst->register_status, src->register_status,
	    src->tdesc->num_registers);
#endif
  dst->registers_valid = src->registers_valid;
}


#ifndef IN_PROCESS_AGENT

void
registers_to_string (struct regcache *regcache, char *buf)
{
  unsigned char *registers = regcache->registers;
  const struct target_desc *tdesc = regcache->tdesc;
  int i;

  for (i = 0; i < tdesc->num_registers; i++)
    {
      if (regcache->register_status[i] == REG_VALID)
	{
	  convert_int_to_ascii (registers, buf,
				register_size (tdesc, i));
	  buf += register_size (tdesc, i) * 2;
	}
      else
	{
	  memset (buf, 'x', register_size (tdesc, i) * 2);
	  buf += register_size (tdesc, i) * 2;
	}
      registers += register_size (tdesc, i);
    }
  *buf = '\0';
}

void
registers_from_string (struct regcache *regcache, char *buf)
{
  int len = strlen (buf);
  unsigned char *registers = regcache->registers;
  const struct target_desc *tdesc = regcache->tdesc;

  if (len != tdesc->registers_size * 2)
    {
      warning ("Wrong sized register packet (expected %d bytes, got %d)",
	       2 * tdesc->registers_size, len);
      if (len > tdesc->registers_size * 2)
	len = tdesc->registers_size * 2;
    }
  convert_ascii_to_int (buf, registers, len / 2);
}

struct reg *
find_register_by_name (const struct target_desc *tdesc, const char *name)
{
  int i;

  for (i = 0; i < tdesc->num_registers; i++)
    if (strcmp (name, tdesc->reg_defs[i].name) == 0)
      return &tdesc->reg_defs[i];
  fatal ("Unknown register %s requested", name);
  return 0;
}

int
find_regno (const struct target_desc *tdesc, const char *name)
{
  int i;

  for (i = 0; i < tdesc->num_registers; i++)
    if (strcmp (name, tdesc->reg_defs[i].name) == 0)
      return i;
  fatal ("Unknown register %s requested", name);
  return -1;
}

struct reg *
find_register_by_number (const struct target_desc *tdesc, int n)
{
  return &tdesc->reg_defs[n];
}

#endif

#ifndef IN_PROCESS_AGENT
static void
free_register_cache_thread (struct thread_info *thread)
{
  struct regcache *regcache
    = (struct regcache *) inferior_regcache_data (thread);

  if (regcache != NULL)
    {
      regcache_invalidate_thread (thread);
      free_register_cache (regcache);
      set_inferior_regcache_data (thread, NULL);
    }
}

static void
free_register_cache_thread_one (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;

  free_register_cache_thread (thread);
}

void
regcache_release (void)
{
  /* Flush and release all pre-existing register caches.  */
  for_each_inferior (&all_threads, free_register_cache_thread_one);
}
#endif

int
register_cache_size (const struct target_desc *tdesc)
{
  return tdesc->registers_size;
}

int
register_size (const struct target_desc *tdesc, int n)
{
  return tdesc->reg_defs[n].size / 8;
}

static unsigned char *
register_data (struct regcache *regcache, int n, int fetch)
{
  return regcache->registers + regcache->tdesc->reg_defs[n].offset / 8;
}

/* Supply register N, whose contents are stored in BUF, to REGCACHE.
   If BUF is NULL, the register's value is recorded as
   unavailable.  */

void
supply_register (struct regcache *regcache, int n, const void *buf)
{
  if (buf)
    {
      memcpy (register_data (regcache, n, 0), buf,
	      register_size (regcache->tdesc, n));
#ifndef IN_PROCESS_AGENT
      if (regcache->register_status != NULL)
	regcache->register_status[n] = REG_VALID;
#endif
    }
  else
    {
      memset (register_data (regcache, n, 0), 0,
	      register_size (regcache->tdesc, n));
#ifndef IN_PROCESS_AGENT
      if (regcache->register_status != NULL)
	regcache->register_status[n] = REG_UNAVAILABLE;
#endif
    }
}

/* Supply register N with value zero to REGCACHE.  */

void
supply_register_zeroed (struct regcache *regcache, int n)
{
  memset (register_data (regcache, n, 0), 0,
	  register_size (regcache->tdesc, n));
#ifndef IN_PROCESS_AGENT
  if (regcache->register_status != NULL)
    regcache->register_status[n] = REG_VALID;
#endif
}

/* Supply the whole register set whose contents are stored in BUF, to
   REGCACHE.  If BUF is NULL, all the registers' values are recorded
   as unavailable.  */

void
supply_regblock (struct regcache *regcache, const void *buf)
{
  if (buf)
    {
      const struct target_desc *tdesc = regcache->tdesc;

      memcpy (regcache->registers, buf, tdesc->registers_size);
#ifndef IN_PROCESS_AGENT
      {
	int i;

	for (i = 0; i < tdesc->num_registers; i++)
	  regcache->register_status[i] = REG_VALID;
      }
#endif
    }
  else
    {
      const struct target_desc *tdesc = regcache->tdesc;

      memset (regcache->registers, 0, tdesc->registers_size);
#ifndef IN_PROCESS_AGENT
      {
	int i;

	for (i = 0; i < tdesc->num_registers; i++)
	  regcache->register_status[i] = REG_UNAVAILABLE;
      }
#endif
    }
}

#ifndef IN_PROCESS_AGENT

void
supply_register_by_name (struct regcache *regcache,
			 const char *name, const void *buf)
{
  supply_register (regcache, find_regno (regcache->tdesc, name), buf);
}

#endif

void
collect_register (struct regcache *regcache, int n, void *buf)
{
  memcpy (buf, register_data (regcache, n, 1),
	  register_size (regcache->tdesc, n));
}

#ifndef IN_PROCESS_AGENT

void
collect_register_as_string (struct regcache *regcache, int n, char *buf)
{
  convert_int_to_ascii (register_data (regcache, n, 1), buf,
			register_size (regcache->tdesc, n));
}

void
collect_register_by_name (struct regcache *regcache,
			  const char *name, void *buf)
{
  collect_register (regcache, find_regno (regcache->tdesc, name), buf);
}

/* Special handling for register PC.  */

CORE_ADDR
regcache_read_pc (struct regcache *regcache)
{
  CORE_ADDR pc_val;

  if (the_target->read_pc)
    pc_val = the_target->read_pc (regcache);
  else
    internal_error (__FILE__, __LINE__,
		    "regcache_read_pc: Unable to find PC");

  return pc_val;
}

void
regcache_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  if (the_target->write_pc)
    the_target->write_pc (regcache, pc);
  else
    internal_error (__FILE__, __LINE__,
		    "regcache_write_pc: Unable to update PC");
}

#endif
