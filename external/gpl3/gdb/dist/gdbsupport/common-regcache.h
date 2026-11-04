/* Cache and manage the values of registers

   Copyright (C) 2014-2025 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_COMMON_REGCACHE_H
#define GDBSUPPORT_COMMON_REGCACHE_H

struct reg_buffer_common;

/* This header is a stopgap until we have an independent regcache.  */

enum register_status : signed char
  {
    /* The register value is not in the cache, and we don't know yet
       whether it's available in the target (or traceframe).  */
    REG_UNKNOWN = 0,

    /* The register value is valid and cached.  */
    REG_VALID = 1,

    /* The register value is unavailable.  E.g., we're inspecting a
       traceframe, and this register wasn't collected.  Note that this
       "unavailable" is different from saying the register does not
       exist in the target's architecture --- in that case, the target
       should have given us a target description that does not include
       the register in the first place.  */
    REG_UNAVAILABLE = -1
  };

/* Return a pointer to the register cache associated with the
   thread specified by PTID.  This function must be provided by
   the client.  */

extern reg_buffer_common *get_thread_regcache_for_ptid (ptid_t ptid);

/* Read the PC register.  This function must be provided by the
   client.  */

extern CORE_ADDR regcache_read_pc (reg_buffer_common *regcache);

/* Read the PC register.  If PC cannot be read, return 0.
   This is a wrapper around 'regcache_read_pc'.  */

extern CORE_ADDR regcache_read_pc_protected (reg_buffer_common *regcache);

/* Read a raw register into a unsigned integer.  */
extern enum register_status
regcache_raw_read_unsigned (reg_buffer_common *regcache, int regnum,
			    ULONGEST *val);

ULONGEST regcache_raw_get_unsigned (reg_buffer_common *regcache, int regnum);

struct reg_buffer_common
{
  virtual ~reg_buffer_common () = default;

  /* Get the availability status of the value of register REGNUM in this
     buffer.  */
  virtual register_status get_register_status (int regnum) const = 0;

  /* Return the size of register numbered REGNUM in this buffer.  */
  virtual int register_size (int regnum) const = 0;

  /* Supply register REGNUM, whose contents are stored in SRC, to this register
     buffer.  */
  virtual void raw_supply (int regnum, gdb::array_view<const gdb_byte> src)
    = 0;

  void raw_supply (int regnum, const uint64_t *src)
  {
    raw_supply (regnum,
		gdb::make_array_view ((const gdb_byte *) src, sizeof (*src)));
  }

  void raw_supply (int regnum, const gdb_byte *src)
  { raw_supply (regnum, gdb::make_array_view (src, register_size (regnum))); }

  /* Supply part of register REGNUM with zeroed value.  Start at OFFSET in
     the register, with size SIZE.  The rest of the register is left
     untouched.  */
  virtual void raw_supply_part_zeroed (int regnum, int offset, size_t size)
    = 0;

  /* Collect register REGNUM from this register buffer and store its contents in
     DST.  */
  virtual void raw_collect (int regnum, gdb::array_view<gdb_byte> dst) const
    = 0;

  void raw_collect (int regnum, uint64_t *dst) const
  {
    raw_collect (regnum,
		 gdb::make_array_view ((gdb_byte *) dst, sizeof (*dst)));
  };

  void raw_collect (int regnum, gdb_byte *dst)
  { raw_collect (regnum, gdb::make_array_view (dst, register_size (regnum))); }

  /* Compare the contents of the register stored in the regcache (ignoring the
     first OFFSET bytes) to the contents of BUF (without any offset).  Returns
     true if the same.  */
  virtual bool raw_compare (int regnum, const void *buf, int offset) const = 0;
};

#endif /* GDBSUPPORT_COMMON_REGCACHE_H */
