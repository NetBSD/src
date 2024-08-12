/* Register support routines for the remote server for GDB.
   Copyright (C) 2001-2023 Free Software Foundation, Inc.

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
#include "gdbsupport/rsp-low.h"
#ifndef IN_PROCESS_AGENT

struct regcache *
get_thread_regcache (struct thread_info *thread, int fetch)
{
  struct regcache *regcache;

  regcache = thread_regcache_data (thread);

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

      gdb_assert (proc->tdesc != NULL);

      regcache = new_register_cache (proc->tdesc);
      set_thread_regcache_data (thread, regcache);
    }

  if (fetch && regcache->registers_valid == 0)
    {
      scoped_restore_current_thread restore_thread;

      switch_to_thread (thread);
      /* Invalidate all registers, to prevent stale left-overs.  */
      memset (regcache->register_status, REG_UNAVAILABLE,
	      regcache->tdesc->reg_defs.size ());
      fetch_inferior_registers (regcache, -1);
      regcache->registers_valid = 1;
    }

  return regcache;
}

/* See gdbsupport/common-regcache.h.  */

struct regcache *
get_thread_regcache_for_ptid (ptid_t ptid)
{
  return get_thread_regcache (find_thread_ptid (ptid), 1);
}

void
regcache_invalidate_thread (struct thread_info *thread)
{
  struct regcache *regcache;

  regcache = thread_regcache_data (thread);

  if (regcache == NULL)
    return;

  if (regcache->registers_valid)
    {
      scoped_restore_current_thread restore_thread;

      switch_to_thread (thread);
      store_inferior_registers (regcache, -1);
    }

  regcache->registers_valid = 0;
}

/* See regcache.h.  */

void
regcache_invalidate_pid (int pid)
{
  /* Only invalidate the regcaches of threads of this process.  */
  for_each_thread (pid, regcache_invalidate_thread);
}

/* See regcache.h.  */

void
regcache_invalidate (void)
{
  /* Only update the threads of the current process.  */
  int pid = current_thread->id.pid ();

  regcache_invalidate_pid (pid);
}

#endif

struct regcache *
init_register_cache (struct regcache *regcache,
		     const struct target_desc *tdesc,
		     unsigned char *regbuf)
{
  if (regbuf == NULL)
    {
#ifndef IN_PROCESS_AGENT
      /* Make sure to zero-initialize the register cache when it is
	 created, in case there are registers the target never
	 fetches.  This way they'll read as zero instead of
	 garbage.  */
      regcache->tdesc = tdesc;
      regcache->registers
	= (unsigned char *) xcalloc (1, tdesc->registers_size);
      regcache->registers_owned = 1;
      regcache->register_status
	= (unsigned char *) xmalloc (tdesc->reg_defs.size ());
      memset ((void *) regcache->register_status, REG_UNAVAILABLE,
	      tdesc->reg_defs.size ());
#else
      gdb_assert_not_reached ("can't allocate memory from the heap");
#endif
    }
  else
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
  struct regcache *regcache = new struct regcache;

  gdb_assert (tdesc->registers_size != 0);

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
      delete regcache;
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
	    src->tdesc->reg_defs.size ());
#endif
  dst->registers_valid = src->registers_valid;
}

/* Return a reference to the description of register N.  */

static const struct gdb::reg &
find_register_by_number (const struct target_desc *tdesc, int n)
{
  return tdesc->reg_defs[n];
}

#ifndef IN_PROCESS_AGENT

void
registers_to_string (struct regcache *regcache, char *buf)
{
  unsigned char *registers = regcache->registers;
  const struct target_desc *tdesc = regcache->tdesc;

  for (int i = 0; i < tdesc->reg_defs.size (); ++i)
    {
      if (regcache->register_status[i] == REG_VALID)
	{
	  bin2hex (registers, buf, register_size (tdesc, i));
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
  hex2bin (buf, registers, len / 2);
}

/* See regcache.h */

gdb::optional<int>
find_regno_no_throw (const struct target_desc *tdesc, const char *name)
{
  for (int i = 0; i < tdesc->reg_defs.size (); ++i)
    {
      if (strcmp (name, find_register_by_number (tdesc, i).name) == 0)
	return i;
    }
  return {};
}

int
find_regno (const struct target_desc *tdesc, const char *name)
{
  gdb::optional<int> regnum = find_regno_no_throw (tdesc, name);

  if (regnum.has_value ())
    return *regnum;

  internal_error ("Unknown register %s requested", name);
}

static void
free_register_cache_thread (struct thread_info *thread)
{
  struct regcache *regcache = thread_regcache_data (thread);

  if (regcache != NULL)
    {
      regcache_invalidate_thread (thread);
      free_register_cache (regcache);
      set_thread_regcache_data (thread, NULL);
    }
}

void
regcache_release (void)
{
  /* Flush and release all pre-existing register caches.  */
  for_each_thread (free_register_cache_thread);
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
  return find_register_by_number (tdesc, n).size / 8;
}

/* See gdbsupport/common-regcache.h.  */

int
regcache_register_size (const struct regcache *regcache, int n)
{
  return register_size (regcache->tdesc, n);
}

static unsigned char *
register_data (const struct regcache *regcache, int n)
{
  return (regcache->registers
	  + find_register_by_number (regcache->tdesc, n).offset / 8);
}

void
supply_register (struct regcache *regcache, int n, const void *buf)
{
  return regcache->raw_supply (n, buf);
}

/* See gdbsupport/common-regcache.h.  */

void
regcache::raw_supply (int n, const void *buf)
{
  if (buf)
    {
      memcpy (register_data (this, n), buf, register_size (tdesc, n));
#ifndef IN_PROCESS_AGENT
      if (register_status != NULL)
	register_status[n] = REG_VALID;
#endif
    }
  else
    {
      memset (register_data (this, n), 0, register_size (tdesc, n));
#ifndef IN_PROCESS_AGENT
      if (register_status != NULL)
	register_status[n] = REG_UNAVAILABLE;
#endif
    }
}

/* Supply register N with value zero to REGCACHE.  */

void
supply_register_zeroed (struct regcache *regcache, int n)
{
  memset (register_data (regcache, n), 0,
	  register_size (regcache->tdesc, n));
#ifndef IN_PROCESS_AGENT
  if (regcache->register_status != NULL)
    regcache->register_status[n] = REG_VALID;
#endif
}

#ifndef IN_PROCESS_AGENT

/* Supply register called NAME with value zero to REGCACHE.  */

void
supply_register_by_name_zeroed (struct regcache *regcache,
				const char *name)
{
  supply_register_zeroed (regcache, find_regno (regcache->tdesc, name));
}

#endif

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

	for (i = 0; i < tdesc->reg_defs.size (); i++)
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

	for (i = 0; i < tdesc->reg_defs.size (); i++)
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
  regcache->raw_collect (n, buf);
}

/* See gdbsupport/common-regcache.h.  */

void
regcache::raw_collect (int n, void *buf) const
{
  memcpy (buf, register_data (this, n), register_size (tdesc, n));
}

enum register_status
regcache_raw_read_unsigned (struct regcache *regcache, int regnum,
			    ULONGEST *val)
{
  int size;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0
	      && regnum < regcache->tdesc->reg_defs.size ());

  size = register_size (regcache->tdesc, regnum);

  if (size > (int) sizeof (ULONGEST))
    error (_("That operation is not available on integers of more than"
	    "%d bytes."),
	  (int) sizeof (ULONGEST));

  *val = 0;
  collect_register (regcache, regnum, val);

  return REG_VALID;
}

#ifndef IN_PROCESS_AGENT

/* See regcache.h.  */

ULONGEST
regcache_raw_get_unsigned_by_name (struct regcache *regcache,
				   const char *name)
{
  return regcache_raw_get_unsigned (regcache,
				    find_regno (regcache->tdesc, name));
}

void
collect_register_as_string (struct regcache *regcache, int n, char *buf)
{
  bin2hex (register_data (regcache, n), buf,
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
  return the_target->read_pc (regcache);
}

void
regcache_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  the_target->write_pc (regcache, pc);
}

#endif

/* See gdbsupport/common-regcache.h.  */

enum register_status
regcache::get_register_status (int regnum) const
{
#ifndef IN_PROCESS_AGENT
  gdb_assert (regnum >= 0 && regnum < tdesc->reg_defs.size ());
  return (enum register_status) (register_status[regnum]);
#else
  return REG_VALID;
#endif
}

/* See gdbsupport/common-regcache.h.  */

bool
regcache::raw_compare (int regnum, const void *buf, int offset) const
{
  gdb_assert (buf != NULL);

  const unsigned char *regbuf = register_data (this, regnum);
  int size = register_size (tdesc, regnum);
  gdb_assert (size >= offset);

  return (memcmp (buf, regbuf + offset, size - offset) == 0);
}
