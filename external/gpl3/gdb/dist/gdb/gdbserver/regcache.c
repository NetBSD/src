/* Register support routines for the remote server for GDB.
   Copyright (C) 2001-2013 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>

static int register_bytes;

static struct reg *reg_defs;
static int num_registers;

const char **gdbserver_expedite_regs;

#ifndef IN_PROCESS_AGENT

struct regcache *
get_thread_regcache (struct thread_info *thread, int fetch)
{
  struct regcache *regcache;

  regcache = (struct regcache *) inferior_regcache_data (thread);

  if (regcache == NULL)
    fatal ("no register cache");

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
regcache_invalidate_one (struct inferior_list_entry *entry)
{
  struct thread_info *thread = (struct thread_info *) entry;
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

void
regcache_invalidate (void)
{
  for_each_inferior (&all_threads, regcache_invalidate_one);
}

#endif

struct regcache *
init_register_cache (struct regcache *regcache, unsigned char *regbuf)
{
#ifndef IN_PROCESS_AGENT
  if (regbuf == NULL)
    {
      /* Make sure to zero-initialize the register cache when it is
	 created, in case there are registers the target never
	 fetches.  This way they'll read as zero instead of
	 garbage.  */
      regcache->registers = xcalloc (1, register_bytes);
      regcache->registers_owned = 1;
      regcache->register_status = xcalloc (1, num_registers);
      gdb_assert (REG_UNAVAILABLE == 0);
    }
  else
#else
  if (regbuf == NULL)
    fatal ("init_register_cache: can't allocate memory from the heap");
  else
#endif
    {
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
new_register_cache (void)
{
  struct regcache *regcache;

  if (register_bytes == 0)
    return NULL; /* The architecture hasn't been initialized yet.  */

  regcache = xmalloc (sizeof (*regcache));
  return init_register_cache (regcache, NULL);
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
  memcpy (dst->registers, src->registers, register_bytes);
#ifndef IN_PROCESS_AGENT
  if (dst->register_status != NULL && src->register_status != NULL)
    memcpy (dst->register_status, src->register_status, num_registers);
#endif
  dst->registers_valid = src->registers_valid;
}

#ifndef IN_PROCESS_AGENT
static void
realloc_register_cache (struct inferior_list_entry *thread_p)
{
  struct thread_info *thread = (struct thread_info *) thread_p;
  struct regcache *regcache
    = (struct regcache *) inferior_regcache_data (thread);

  if (regcache != NULL)
    regcache_invalidate_one (thread_p);
  free_register_cache (regcache);
  set_inferior_regcache_data (thread, new_register_cache ());
}
#endif

void
set_register_cache (struct reg *regs, int n)
{
  int offset, i;

#ifndef IN_PROCESS_AGENT
  /* Before changing the register cache internal layout, flush the
     contents of valid caches back to the threads.  */
  regcache_invalidate ();
#endif

  reg_defs = regs;
  num_registers = n;

  offset = 0;
  for (i = 0; i < n; i++)
    {
      regs[i].offset = offset;
      offset += regs[i].size;
    }

  register_bytes = offset / 8;

  /* Make sure PBUFSIZ is large enough to hold a full register packet.  */
  if (2 * register_bytes + 32 > PBUFSIZ)
    fatal ("Register packet size exceeds PBUFSIZ.");

#ifndef IN_PROCESS_AGENT
  /* Re-allocate all pre-existing register caches.  */
  for_each_inferior (&all_threads, realloc_register_cache);
#endif
}

int
register_cache_size (void)
{
  return register_bytes;
}

#ifndef IN_PROCESS_AGENT

void
registers_to_string (struct regcache *regcache, char *buf)
{
  unsigned char *registers = regcache->registers;
  int i;

  for (i = 0; i < num_registers; i++)
    {
      if (regcache->register_status[i] == REG_VALID)
	{
	  convert_int_to_ascii (registers, buf, register_size (i));
	  buf += register_size (i) * 2;
	}
      else
	{
	  memset (buf, 'x', register_size (i) * 2);
	  buf += register_size (i) * 2;
	}
      registers += register_size (i);
    }
  *buf = '\0';
}

void
registers_from_string (struct regcache *regcache, char *buf)
{
  int len = strlen (buf);
  unsigned char *registers = regcache->registers;

  if (len != register_bytes * 2)
    {
      warning ("Wrong sized register packet (expected %d bytes, got %d)",
	       2*register_bytes, len);
      if (len > register_bytes * 2)
	len = register_bytes * 2;
    }
  convert_ascii_to_int (buf, registers, len / 2);
}

struct reg *
find_register_by_name (const char *name)
{
  int i;

  for (i = 0; i < num_registers; i++)
    if (!strcmp (name, reg_defs[i].name))
      return &reg_defs[i];
  fatal ("Unknown register %s requested", name);
  return 0;
}

int
find_regno (const char *name)
{
  int i;

  for (i = 0; i < num_registers; i++)
    if (!strcmp (name, reg_defs[i].name))
      return i;
  fatal ("Unknown register %s requested", name);
  return -1;
}

struct reg *
find_register_by_number (int n)
{
  return &reg_defs[n];
}

#endif

int
register_size (int n)
{
  return reg_defs[n].size / 8;
}

static unsigned char *
register_data (struct regcache *regcache, int n, int fetch)
{
  return regcache->registers + (reg_defs[n].offset / 8);
}

/* Supply register N, whose contents are stored in BUF, to REGCACHE.
   If BUF is NULL, the register's value is recorded as
   unavailable.  */

void
supply_register (struct regcache *regcache, int n, const void *buf)
{
  if (buf)
    {
      memcpy (register_data (regcache, n, 0), buf, register_size (n));
#ifndef IN_PROCESS_AGENT
      if (regcache->register_status != NULL)
	regcache->register_status[n] = REG_VALID;
#endif
    }
  else
    {
      memset (register_data (regcache, n, 0), 0, register_size (n));
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
  memset (register_data (regcache, n, 0), 0, register_size (n));
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
      memcpy (regcache->registers, buf, register_bytes);
#ifndef IN_PROCESS_AGENT
      {
	int i;

	for (i = 0; i < num_registers; i++)
	  regcache->register_status[i] = REG_VALID;
      }
#endif
    }
  else
    {
      memset (regcache->registers, 0, register_bytes);
#ifndef IN_PROCESS_AGENT
      {
	int i;

	for (i = 0; i < num_registers; i++)
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
  supply_register (regcache, find_regno (name), buf);
}

#endif

void
collect_register (struct regcache *regcache, int n, void *buf)
{
  memcpy (buf, register_data (regcache, n, 1), register_size (n));
}

#ifndef IN_PROCESS_AGENT

void
collect_register_as_string (struct regcache *regcache, int n, char *buf)
{
  convert_int_to_ascii (register_data (regcache, n, 1),
			buf, register_size (n));
}

void
collect_register_by_name (struct regcache *regcache,
			  const char *name, void *buf)
{
  collect_register (regcache, find_regno (name), buf);
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
