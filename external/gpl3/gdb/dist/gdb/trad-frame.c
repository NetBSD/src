/* Traditional frame unwind support, for GDB the GNU Debugger.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "frame.h"
#include "trad-frame.h"
#include "regcache.h"
#include "frame-unwind.h"
#include "target.h"
#include "value.h"
#include "gdbarch.h"

struct trad_frame_cache
{
  struct frame_info *this_frame;
  CORE_ADDR this_base;
  struct trad_frame_saved_reg *prev_regs;
  struct frame_id this_id;
};

struct trad_frame_cache *
trad_frame_cache_zalloc (struct frame_info *this_frame)
{
  struct trad_frame_cache *this_trad_cache;

  this_trad_cache = FRAME_OBSTACK_ZALLOC (struct trad_frame_cache);
  this_trad_cache->prev_regs = trad_frame_alloc_saved_regs (this_frame);
  this_trad_cache->this_frame = this_frame;
  return this_trad_cache;
}

/* See trad-frame.h.  */

void
trad_frame_reset_saved_regs (struct gdbarch *gdbarch,
			     struct trad_frame_saved_reg *regs)
{
  int numregs = gdbarch_num_cooked_regs (gdbarch);
  for (int regnum = 0; regnum < numregs; regnum++)
    {
      regs[regnum].realreg = regnum;
      regs[regnum].addr = -1;
    }
}

struct trad_frame_saved_reg *
trad_frame_alloc_saved_regs (struct gdbarch *gdbarch)
{
  int numregs = gdbarch_num_cooked_regs (gdbarch);
  struct trad_frame_saved_reg *this_saved_regs
    = FRAME_OBSTACK_CALLOC (numregs, struct trad_frame_saved_reg);

  trad_frame_reset_saved_regs (gdbarch, this_saved_regs);
  return this_saved_regs;
}

/* A traditional frame is unwound by analysing the function prologue
   and using the information gathered to track registers.  For
   non-optimized frames, the technique is reliable (just need to check
   for all potential instruction sequences).  */

struct trad_frame_saved_reg *
trad_frame_alloc_saved_regs (struct frame_info *this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  return trad_frame_alloc_saved_regs (gdbarch);
}

enum { TF_REG_VALUE = -1, TF_REG_UNKNOWN = -2 };

int
trad_frame_value_p (struct trad_frame_saved_reg this_saved_regs[], int regnum)
{
  return (this_saved_regs[regnum].realreg == TF_REG_VALUE);
}

int
trad_frame_addr_p (struct trad_frame_saved_reg this_saved_regs[], int regnum)
{
  return (this_saved_regs[regnum].realreg >= 0
	  && this_saved_regs[regnum].addr != -1);
}

int
trad_frame_realreg_p (struct trad_frame_saved_reg this_saved_regs[],
		      int regnum)
{
  return (this_saved_regs[regnum].realreg >= 0
	  && this_saved_regs[regnum].addr == -1);
}

void
trad_frame_set_value (struct trad_frame_saved_reg this_saved_regs[],
		      int regnum, LONGEST val)
{
  /* Make the REALREG invalid, indicating that the ADDR contains the
     register's value.  */
  this_saved_regs[regnum].realreg = TF_REG_VALUE;
  this_saved_regs[regnum].addr = val;
}

/* See trad-frame.h.  */

void
trad_frame_set_realreg (struct trad_frame_saved_reg this_saved_regs[],
			int regnum, int realreg)
{
  this_saved_regs[regnum].realreg = realreg;
  this_saved_regs[regnum].addr = -1;
}

/* See trad-frame.h.  */

void
trad_frame_set_addr (struct trad_frame_saved_reg this_saved_regs[],
		     int regnum, CORE_ADDR addr)
{
  this_saved_regs[regnum].realreg = regnum;
  this_saved_regs[regnum].addr = addr;
}

void
trad_frame_set_reg_value (struct trad_frame_cache *this_trad_cache,
			  int regnum, LONGEST val)
{
  /* External interface for users of trad_frame_cache
     (who cannot access the prev_regs object directly).  */
  trad_frame_set_value (this_trad_cache->prev_regs, regnum, val);
}

void
trad_frame_set_reg_realreg (struct trad_frame_cache *this_trad_cache,
			    int regnum, int realreg)
{
  trad_frame_set_realreg (this_trad_cache->prev_regs, regnum, realreg);
}

void
trad_frame_set_reg_addr (struct trad_frame_cache *this_trad_cache,
			 int regnum, CORE_ADDR addr)
{
  trad_frame_set_addr (this_trad_cache->prev_regs, regnum, addr);
}

void
trad_frame_set_reg_regmap (struct trad_frame_cache *this_trad_cache,
			   const struct regcache_map_entry *regmap,
			   CORE_ADDR addr, size_t size)
{
  struct gdbarch *gdbarch = get_frame_arch (this_trad_cache->this_frame);
  int offs = 0, count;

  for (; (count = regmap->count) != 0; regmap++)
    {
      int regno = regmap->regno;
      int slot_size = regmap->size;

      if (slot_size == 0 && regno != REGCACHE_MAP_SKIP)
	slot_size = register_size (gdbarch, regno);

      if (offs + slot_size > size)
	break;

      if (regno == REGCACHE_MAP_SKIP)
	offs += count * slot_size;
      else
	for (; count--; regno++, offs += slot_size)
	  {
	    /* Mimic the semantics of regcache::transfer_regset if a
	       register slot's size does not match the size of a
	       register.

	       If a register slot is larger than a register, assume
	       the register's value is stored in the first N bytes of
	       the slot and ignore the remaining bytes.

	       If the register slot is smaller than the register,
	       assume that the slot contains the low N bytes of the
	       register's value.  Since trad_frame assumes that
	       registers stored by address are sized according to the
	       register, read the low N bytes and zero-extend them to
	       generate a register value.  */
	    if (slot_size >= register_size (gdbarch, regno))
	      trad_frame_set_reg_addr (this_trad_cache, regno, addr + offs);
	    else
	      {
		enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		gdb_byte buf[slot_size];

		if (target_read_memory (addr + offs, buf, sizeof buf) == 0)
		  {
		    LONGEST val
		      = extract_unsigned_integer (buf, sizeof buf, byte_order);
		    trad_frame_set_reg_value (this_trad_cache, regno, val);
		  }
	      }
	  }
    }
}

void
trad_frame_set_unknown (struct trad_frame_saved_reg this_saved_regs[],
			int regnum)
{
  /* Make the REALREG invalid, indicating that the value is not known.  */
  this_saved_regs[regnum].realreg = TF_REG_UNKNOWN;
  this_saved_regs[regnum].addr = -1;
}

struct value *
trad_frame_get_prev_register (struct frame_info *this_frame,
			      struct trad_frame_saved_reg this_saved_regs[],
			      int regnum)
{
  if (trad_frame_addr_p (this_saved_regs, regnum))
    /* The register was saved in memory.  */
    return frame_unwind_got_memory (this_frame, regnum,
				    this_saved_regs[regnum].addr);
  else if (trad_frame_realreg_p (this_saved_regs, regnum))
    return frame_unwind_got_register (this_frame, regnum,
				      this_saved_regs[regnum].realreg);
  else if (trad_frame_value_p (this_saved_regs, regnum))
    /* The register's value is available.  */
    return frame_unwind_got_constant (this_frame, regnum,
				      this_saved_regs[regnum].addr);
  else
    return frame_unwind_got_optimized (this_frame, regnum);
}

struct value *
trad_frame_get_register (struct trad_frame_cache *this_trad_cache,
			 struct frame_info *this_frame,
			 int regnum)
{
  return trad_frame_get_prev_register (this_frame, this_trad_cache->prev_regs,
				       regnum);
}

void
trad_frame_set_id (struct trad_frame_cache *this_trad_cache,
		   struct frame_id this_id)
{
  this_trad_cache->this_id = this_id;
}

void
trad_frame_get_id (struct trad_frame_cache *this_trad_cache,
		   struct frame_id *this_id)
{
  (*this_id) = this_trad_cache->this_id;
}

void
trad_frame_set_this_base (struct trad_frame_cache *this_trad_cache,
			  CORE_ADDR this_base)
{
  this_trad_cache->this_base = this_base;
}

CORE_ADDR
trad_frame_get_this_base (struct trad_frame_cache *this_trad_cache)
{
  return this_trad_cache->this_base;
}
