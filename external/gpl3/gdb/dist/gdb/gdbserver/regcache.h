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

#ifndef REGCACHE_H
#define REGCACHE_H

struct inferior_list_entry;
struct thread_info;
struct target_desc;

/* The register exists, it has a value, but we don't know what it is.
   Used when inspecting traceframes.  */
#define REG_UNAVAILABLE 0

/* We know the register's value (and we have it cached).  */
#define REG_VALID 1

/* The data for the register cache.  Note that we have one per
   inferior; this is primarily for simplicity, as the performance
   benefit is minimal.  */

struct regcache
{
  /* The regcache's target description.  */
  const struct target_desc *tdesc;

  /* Whether the REGISTERS buffer's contents are valid.  If false, we
     haven't fetched the registers from the target yet.  Not that this
     register cache is _not_ pass-through, unlike GDB's.  Note that
     "valid" here is unrelated to whether the registers are available
     in a traceframe.  For that, check REGISTER_STATUS below.  */
  int registers_valid;
  int registers_owned;
  unsigned char *registers;
#ifndef IN_PROCESS_AGENT
  /* One of REG_UNAVAILBLE or REG_VALID.  */
  unsigned char *register_status;
#endif
};

struct regcache *init_register_cache (struct regcache *regcache,
				      const struct target_desc *tdesc,
				      unsigned char *regbuf);

void regcache_cpy (struct regcache *dst, struct regcache *src);

/* Create a new register cache for INFERIOR.  */

struct regcache *new_register_cache (const struct target_desc *tdesc);

struct regcache *get_thread_regcache (struct thread_info *thread, int fetch);

/* Release all memory associated with the register cache for INFERIOR.  */

void free_register_cache (struct regcache *regcache);

/* Invalidate cached registers for one thread.  */

void regcache_invalidate_thread (struct thread_info *);

/* Invalidate cached registers for all threads of the current
   process.  */

void regcache_invalidate (void);

/* Invalidate and release the register cache of all threads of the
   current process.  */

void regcache_release (void);

/* Convert all registers to a string in the currently specified remote
   format.  */

void registers_to_string (struct regcache *regcache, char *buf);

/* Convert a string to register values and fill our register cache.  */

void registers_from_string (struct regcache *regcache, char *buf);

CORE_ADDR regcache_read_pc (struct regcache *regcache);

void regcache_write_pc (struct regcache *regcache, CORE_ADDR pc);

/* Return a pointer to the description of register ``n''.  */

struct reg *find_register_by_number (const struct target_desc *tdesc, int n);

int register_cache_size (const struct target_desc *tdesc);

int register_size (const struct target_desc *tdesc, int n);

int find_regno (const struct target_desc *tdesc, const char *name);

void supply_register (struct regcache *regcache, int n, const void *buf);

void supply_register_zeroed (struct regcache *regcache, int n);

void supply_register_by_name (struct regcache *regcache,
			      const char *name, const void *buf);

void supply_regblock (struct regcache *regcache, const void *buf);

void collect_register (struct regcache *regcache, int n, void *buf);

void collect_register_as_string (struct regcache *regcache, int n, char *buf);

void collect_register_by_name (struct regcache *regcache,
			       const char *name, void *buf);

#endif /* REGCACHE_H */
