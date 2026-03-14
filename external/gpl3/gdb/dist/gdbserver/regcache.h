/* Register support routines for the remote server for GDB.
   Copyright (C) 2001-2025 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_REGCACHE_H
#define GDBSERVER_REGCACHE_H

#include "gdbsupport/common-regcache.h"
#include <memory>

struct thread_info;
struct target_desc;

/* The data for the register cache.  Note that we have one per
   inferior; this is primarily for simplicity, as the performance
   benefit is minimal.  */

struct regcache : public reg_buffer_common
{
  /* The regcache's target description.  */
  const struct target_desc *tdesc = nullptr;

  /* Whether the REGISTERS buffer's contents are fetched.  If false,
     we haven't fetched the registers from the target yet.  Note that
     this register cache is _not_ pass-through, unlike GDB's.  Also,
     note that "fetched" here is unrelated to whether the registers
     are available in a traceframe.  For that, check REGISTER_STATUS
     below.  */
  bool registers_fetched = false;
  bool registers_owned = false;
  unsigned char *registers = nullptr;
#ifndef IN_PROCESS_AGENT
  /* Construct a regcache using the register layout described by TDESC.

     The regcache dynamically allocates its register buffer.  */
  regcache (const target_desc *tdesc);

  /* Destructor.  */
  ~regcache ();
#endif

  /* Construct a regcache using the register layout described by TDESC
     and REGBUF as the register buffer.

     The regcache does *not* take ownership of the buffer.  */
  regcache (const target_desc *tdesc, unsigned char *regbuf);

  DISABLE_COPY_AND_ASSIGN (regcache);

  /* Clear the register values to all zeros and set the register
     statuses to STATUS.  */
  void reset (enum register_status status);

  /* See gdbsupport/common-regcache.h.  */
  enum register_status get_register_status (int regnum) const override;

  /* Set the status of register REGNUM to STATUS.  */
  void set_register_status (int regnum, enum register_status status);

  /* See gdbsupport/common-regcache.h.  */
  int register_size (int regnum) const override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_supply (int regnum, gdb::array_view<const gdb_byte> src) override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_supply_part_zeroed (int regnum, int offset, size_t size) override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_collect (int regnum, gdb::array_view<gdb_byte> dst) const override;

  /* See gdbsupport/common-regcache.h.  */
  bool raw_compare (int regnum, const void *buf, int offset) const override;

  /* Copy the contents of SRC into this regcache.  */
  void copy_from (regcache *src);

private:

#ifndef IN_PROCESS_AGENT
  /* See gdbsupport/common-regcache.h.  */
  std::unique_ptr<enum register_status[]> m_register_status;
#endif
};

regcache *get_thread_regcache (thread_info *thread, bool fetch = true);

/* Invalidate cached registers for one thread.  */

void regcache_invalidate_thread (thread_info *);

/* Invalidate cached registers for all threads of the given process.  */

void regcache_invalidate_pid (int pid);

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

/* For regcache_read_pc see gdbsupport/common-regcache.h.  */

void regcache_write_pc (struct regcache *regcache, CORE_ADDR pc);

int register_cache_size (const struct target_desc *tdesc);

int register_size (const struct target_desc *tdesc, int n);

/* No throw version of find_regno.  If NAME is not a known register, return
   an empty value.  */
std::optional<int> find_regno_no_throw (const struct target_desc *tdesc,
					const char *name);

int find_regno (const struct target_desc *tdesc, const char *name);

void supply_register (struct regcache *regcache, int n, const void *buf);

void supply_register_zeroed (struct regcache *regcache, int n);

void supply_register_by_name (struct regcache *regcache,
			      const char *name, const void *buf);

void supply_register_by_name_zeroed (struct regcache *regcache,
				     const char *name);

void supply_regblock (struct regcache *regcache, const void *buf);

void collect_register (struct regcache *regcache, int n, void *buf);

void collect_register_as_string (struct regcache *regcache, int n, char *buf);

void collect_register_by_name (struct regcache *regcache,
			       const char *name, void *buf);

/* Read a raw register as an unsigned integer.  Convenience wrapper
   around regcache_raw_get_unsigned that takes a register name instead
   of a register number.  */

ULONGEST regcache_raw_get_unsigned_by_name (struct regcache *regcache,
					    const char *name);

#endif /* GDBSERVER_REGCACHE_H */
