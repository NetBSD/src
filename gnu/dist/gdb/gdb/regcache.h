/* Cache and manage the values of registers for GDB, the GNU debugger.

   Copyright 1986, 1987, 1989, 1991, 1994, 1995, 1996, 1998, 2000,
   2001, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef REGCACHE_H
#define REGCACHE_H

struct regcache;
struct gdbarch;

extern struct regcache *current_regcache;

void regcache_xfree (struct regcache *regcache);
struct cleanup *make_cleanup_regcache_xfree (struct regcache *regcache);
struct regcache *regcache_xmalloc (struct gdbarch *gdbarch);

/* Transfer a raw register [0..NUM_REGS) between core-gdb and the
   regcache. */

void regcache_raw_read (struct regcache *regcache, int rawnum, void *buf);
void regcache_raw_write (struct regcache *regcache, int rawnum,
			 const void *buf);
extern void regcache_raw_read_signed (struct regcache *regcache,
				      int regnum, LONGEST *val);
extern void regcache_raw_read_unsigned (struct regcache *regcache,
					int regnum, ULONGEST *val);
extern void regcache_raw_write_signed (struct regcache *regcache,
				       int regnum, LONGEST val);
extern void regcache_raw_write_unsigned (struct regcache *regcache,
					 int regnum, ULONGEST val);

/* Partial transfer of a raw registers.  These perform read, modify,
   write style operations.  */

void regcache_raw_read_part (struct regcache *regcache, int regnum,
			     int offset, int len, void *buf);
void regcache_raw_write_part (struct regcache *regcache, int regnum,
			      int offset, int len, const void *buf);

int regcache_valid_p (struct regcache *regcache, int regnum);

/* Transfer a cooked register [0..NUM_REGS+NUM_PSEUDO_REGS).  */
void regcache_cooked_read (struct regcache *regcache, int rawnum, void *buf);
void regcache_cooked_write (struct regcache *regcache, int rawnum,
			    const void *buf);

/* NOTE: cagney/2002-08-13: At present GDB has no reliable mechanism
   for indicating when a ``cooked'' register was constructed from
   invalid or unavailable ``raw'' registers.  One fairly easy way of
   adding such a mechanism would be for the cooked functions to return
   a register valid indication.  Given the possibility of such a
   change, the extract functions below use a reference parameter,
   rather than a function result.  */

/* Read a register as a signed/unsigned quantity.  */
extern void regcache_cooked_read_signed (struct regcache *regcache,
					 int regnum, LONGEST *val);
extern void regcache_cooked_read_unsigned (struct regcache *regcache,
					   int regnum, ULONGEST *val);

/* Partial transfer of a cooked register.  These perform read, modify,
   write style operations.  */

void regcache_cooked_read_part (struct regcache *regcache, int regnum,
				int offset, int len, void *buf);
void regcache_cooked_write_part (struct regcache *regcache, int regnum,
				 int offset, int len, const void *buf);

/* Transfer a raw register [0..NUM_REGS) between the regcache and the
   target.  These functions are called by the target in response to a
   target_fetch_registers() or target_store_registers().  */

extern void supply_register (int regnum, const void *val);
extern void regcache_collect (int regnum, void *buf);


/* The register's ``offset''.

   NOTE: cagney/2002-08-17: The ``struct value'' and expression
   evaluator treat the register cache as a large liner buffer.
   Instead of reading/writing a register using its register number,
   the code read/writes registers by specifying their offset into the
   buffer and a number of bytes.  The code also assumes that these
   byte read/writes can cross register boundaries, adjacent registers
   treated as a contiguous set of bytes.

   The below map that model onto the real register cache.  New code
   should go out of their way to avoid using these interfaces.

   FIXME: cagney/2002-08-17: The ``struct value'' and expression
   evaluator should be fixed.  Instead of using the { offset, length }
   pair to describe a value within one or more registers, the code
   should use a chain of { regnum, offset, len } tripples.  */

extern int register_offset_hack (struct gdbarch *gdbarch, int regnum);
extern void regcache_cooked_read_using_offset_hack (struct regcache *regcache,
						    int offset, int len,
						    void *buf);
extern void regcache_cooked_write_using_offset_hack (struct regcache *regcache,
						     int offset, int len,
						     const void *buf);


/* The type of a register.  This function is slightly more efficient
   then its gdbarch vector counterpart since it returns a precomputed
   value stored in a table.

   NOTE: cagney/2002-08-17: The original macro was called
   REGISTER_VIRTUAL_TYPE.  This was because the register could have
   different raw and cooked (nee virtual) representations.  The
   CONVERTABLE methods being used to convert between the two
   representations.  Current code does not do this.  Instead, the
   first [0..NUM_REGS) registers are 1:1 raw:cooked, and the type
   exactly describes the register's representation.  Consequently, the
   ``virtual'' has been dropped.

   FIXME: cagney/2002-08-17: A number of architectures, including the
   MIPS, are currently broken in this regard.  */

extern struct type *register_type (struct gdbarch *gdbarch, int regnum);


/* Return the size of the largest register.  Used when allocating
   space for an aribtrary register value.  */

extern int max_register_size (struct gdbarch *gdbarch);


/* DEPRECATED: Character array containing an image of the inferior
   programs' registers for the most recently referenced thread. */

extern char *registers;

/* DEPRECATED: Character array containing the current state of each
   register (unavailable<0, invalid=0, valid>0) for the most recently
   referenced thread. */

extern signed char *register_valid;

/* Copy/duplicate the contents of a register cache.  By default, the
   operation is pass-through.  Writes to DST and reads from SRC will
   go through to the target.

   The ``cpy'' functions can not have overlapping SRC and DST buffers.

   ``no passthrough'' versions do not go through to the target.  They
   only transfer values already in the cache.  */

extern struct regcache *regcache_dup (struct regcache *regcache);
extern struct regcache *regcache_dup_no_passthrough (struct regcache *regcache);
extern void regcache_cpy (struct regcache *dest, struct regcache *src);
extern void regcache_cpy_no_passthrough (struct regcache *dest, struct regcache *src);

extern char *deprecated_grub_regcache_for_registers (struct regcache *);
extern char *deprecated_grub_regcache_for_register_valid (struct regcache *);

extern int register_cached (int regnum);

extern void set_register_cached (int regnum, int state);

extern void register_changed (int regnum);

extern void registers_changed (void);

extern void registers_fetched (void);

extern void read_register_bytes (int regbyte, char *myaddr, int len);

extern void read_register_gen (int regnum, char *myaddr);

extern void write_register_gen (int regnum, char *myaddr);

extern void write_register_bytes (int regbyte, char *myaddr, int len);

/* Rename to read_unsigned_register()? */
extern ULONGEST read_register (int regnum);

/* Rename to read_unsigned_register_pid()? */
extern ULONGEST read_register_pid (int regnum, ptid_t ptid);

extern LONGEST read_signed_register (int regnum);

extern LONGEST read_signed_register_pid (int regnum, ptid_t ptid);

extern void write_register (int regnum, LONGEST val);

extern void write_register_pid (int regnum, CORE_ADDR val, ptid_t ptid);

#endif /* REGCACHE_H */
