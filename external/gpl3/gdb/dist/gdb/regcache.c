/* Cache and manage the values of registers for GDB, the GNU debugger.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "target.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "reggroups.h"
#include "gdb_assert.h"
#include "gdb_string.h"
#include "gdbcmd.h"		/* For maintenanceprintlist.  */
#include "observer.h"
#include "exceptions.h"
#include "remote.h"

/*
 * DATA STRUCTURE
 *
 * Here is the actual register cache.
 */

/* Per-architecture object describing the layout of a register cache.
   Computed once when the architecture is created.  */

struct gdbarch_data *regcache_descr_handle;

struct regcache_descr
{
  /* The architecture this descriptor belongs to.  */
  struct gdbarch *gdbarch;

  /* The raw register cache.  Each raw (or hard) register is supplied
     by the target interface.  The raw cache should not contain
     redundant information - if the PC is constructed from two
     registers then those registers and not the PC lives in the raw
     cache.  */
  int nr_raw_registers;
  long sizeof_raw_registers;
  long sizeof_raw_register_status;

  /* The cooked register space.  Each cooked register in the range
     [0..NR_RAW_REGISTERS) is direct-mapped onto the corresponding raw
     register.  The remaining [NR_RAW_REGISTERS
     .. NR_COOKED_REGISTERS) (a.k.a. pseudo registers) are mapped onto
     both raw registers and memory by the architecture methods
     gdbarch_pseudo_register_read and gdbarch_pseudo_register_write.  */
  int nr_cooked_registers;
  long sizeof_cooked_registers;
  long sizeof_cooked_register_status;

  /* Offset and size (in 8 bit bytes), of each register in the
     register cache.  All registers (including those in the range
     [NR_RAW_REGISTERS .. NR_COOKED_REGISTERS) are given an
     offset.  */
  long *register_offset;
  long *sizeof_register;

  /* Cached table containing the type of each register.  */
  struct type **register_type;
};

static void *
init_regcache_descr (struct gdbarch *gdbarch)
{
  int i;
  struct regcache_descr *descr;
  gdb_assert (gdbarch != NULL);

  /* Create an initial, zero filled, table.  */
  descr = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct regcache_descr);
  descr->gdbarch = gdbarch;

  /* Total size of the register space.  The raw registers are mapped
     directly onto the raw register cache while the pseudo's are
     either mapped onto raw-registers or memory.  */
  descr->nr_cooked_registers = gdbarch_num_regs (gdbarch)
			       + gdbarch_num_pseudo_regs (gdbarch);
  descr->sizeof_cooked_register_status
    = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);

  /* Fill in a table of register types.  */
  descr->register_type
    = GDBARCH_OBSTACK_CALLOC (gdbarch, descr->nr_cooked_registers,
			      struct type *);
  for (i = 0; i < descr->nr_cooked_registers; i++)
    descr->register_type[i] = gdbarch_register_type (gdbarch, i);

  /* Construct a strictly RAW register cache.  Don't allow pseudo's
     into the register cache.  */
  descr->nr_raw_registers = gdbarch_num_regs (gdbarch);
  descr->sizeof_raw_register_status = gdbarch_num_regs (gdbarch);

  /* Lay out the register cache.

     NOTE: cagney/2002-05-22: Only register_type() is used when
     constructing the register cache.  It is assumed that the
     register's raw size, virtual size and type length are all the
     same.  */

  {
    long offset = 0;

    descr->sizeof_register
      = GDBARCH_OBSTACK_CALLOC (gdbarch, descr->nr_cooked_registers, long);
    descr->register_offset
      = GDBARCH_OBSTACK_CALLOC (gdbarch, descr->nr_cooked_registers, long);
    for (i = 0; i < descr->nr_raw_registers; i++)
      {
	descr->sizeof_register[i] = TYPE_LENGTH (descr->register_type[i]);
	descr->register_offset[i] = offset;
	offset += descr->sizeof_register[i];
	gdb_assert (MAX_REGISTER_SIZE >= descr->sizeof_register[i]);
      }
    /* Set the real size of the raw register cache buffer.  */
    descr->sizeof_raw_registers = offset;

    for (; i < descr->nr_cooked_registers; i++)
      {
	descr->sizeof_register[i] = TYPE_LENGTH (descr->register_type[i]);
	descr->register_offset[i] = offset;
	offset += descr->sizeof_register[i];
	gdb_assert (MAX_REGISTER_SIZE >= descr->sizeof_register[i]);
      }
    /* Set the real size of the readonly register cache buffer.  */
    descr->sizeof_cooked_registers = offset;
  }

  return descr;
}

static struct regcache_descr *
regcache_descr (struct gdbarch *gdbarch)
{
  return gdbarch_data (gdbarch, regcache_descr_handle);
}

/* Utility functions returning useful register attributes stored in
   the regcache descr.  */

struct type *
register_type (struct gdbarch *gdbarch, int regnum)
{
  struct regcache_descr *descr = regcache_descr (gdbarch);

  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  return descr->register_type[regnum];
}

/* Utility functions returning useful register attributes stored in
   the regcache descr.  */

int
register_size (struct gdbarch *gdbarch, int regnum)
{
  struct regcache_descr *descr = regcache_descr (gdbarch);
  int size;

  gdb_assert (regnum >= 0
	      && regnum < (gdbarch_num_regs (gdbarch)
			   + gdbarch_num_pseudo_regs (gdbarch)));
  size = descr->sizeof_register[regnum];
  return size;
}

/* The register cache for storing raw register values.  */

struct regcache
{
  struct regcache_descr *descr;

  /* The address space of this register cache (for registers where it
     makes sense, like PC or SP).  */
  struct address_space *aspace;

  /* The register buffers.  A read-only register cache can hold the
     full [0 .. gdbarch_num_regs + gdbarch_num_pseudo_regs) while a read/write
     register cache can only hold [0 .. gdbarch_num_regs).  */
  gdb_byte *registers;
  /* Register cache status.  */
  signed char *register_status;
  /* Is this a read-only cache?  A read-only cache is used for saving
     the target's register state (e.g, across an inferior function
     call or just before forcing a function return).  A read-only
     cache can only be updated via the methods regcache_dup() and
     regcache_cpy().  The actual contents are determined by the
     reggroup_save and reggroup_restore methods.  */
  int readonly_p;
  /* If this is a read-write cache, which thread's registers is
     it connected to?  */
  ptid_t ptid;
};

static struct regcache *
regcache_xmalloc_1 (struct gdbarch *gdbarch, struct address_space *aspace,
		    int readonly_p)
{
  struct regcache_descr *descr;
  struct regcache *regcache;

  gdb_assert (gdbarch != NULL);
  descr = regcache_descr (gdbarch);
  regcache = XMALLOC (struct regcache);
  regcache->descr = descr;
  regcache->readonly_p = readonly_p;
  if (readonly_p)
    {
      regcache->registers
	= XCALLOC (descr->sizeof_cooked_registers, gdb_byte);
      regcache->register_status
	= XCALLOC (descr->sizeof_cooked_register_status, signed char);
    }
  else
    {
      regcache->registers
	= XCALLOC (descr->sizeof_raw_registers, gdb_byte);
      regcache->register_status
	= XCALLOC (descr->sizeof_raw_register_status, signed char);
    }
  regcache->aspace = aspace;
  regcache->ptid = minus_one_ptid;
  return regcache;
}

struct regcache *
regcache_xmalloc (struct gdbarch *gdbarch, struct address_space *aspace)
{
  return regcache_xmalloc_1 (gdbarch, aspace, 1);
}

void
regcache_xfree (struct regcache *regcache)
{
  if (regcache == NULL)
    return;
  xfree (regcache->registers);
  xfree (regcache->register_status);
  xfree (regcache);
}

static void
do_regcache_xfree (void *data)
{
  regcache_xfree (data);
}

struct cleanup *
make_cleanup_regcache_xfree (struct regcache *regcache)
{
  return make_cleanup (do_regcache_xfree, regcache);
}

/* Return REGCACHE's architecture.  */

struct gdbarch *
get_regcache_arch (const struct regcache *regcache)
{
  return regcache->descr->gdbarch;
}

struct address_space *
get_regcache_aspace (const struct regcache *regcache)
{
  return regcache->aspace;
}

/* Return  a pointer to register REGNUM's buffer cache.  */

static gdb_byte *
register_buffer (const struct regcache *regcache, int regnum)
{
  return regcache->registers + regcache->descr->register_offset[regnum];
}

void
regcache_save (struct regcache *dst, regcache_cooked_read_ftype *cooked_read,
	       void *src)
{
  struct gdbarch *gdbarch = dst->descr->gdbarch;
  gdb_byte buf[MAX_REGISTER_SIZE];
  int regnum;

  /* The DST should be `read-only', if it wasn't then the save would
     end up trying to write the register values back out to the
     target.  */
  gdb_assert (dst->readonly_p);
  /* Clear the dest.  */
  memset (dst->registers, 0, dst->descr->sizeof_cooked_registers);
  memset (dst->register_status, 0,
	  dst->descr->sizeof_cooked_register_status);
  /* Copy over any registers (identified by their membership in the
     save_reggroup) and mark them as valid.  The full [0 .. gdbarch_num_regs +
     gdbarch_num_pseudo_regs) range is checked since some architectures need
     to save/restore `cooked' registers that live in memory.  */
  for (regnum = 0; regnum < dst->descr->nr_cooked_registers; regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, save_reggroup))
	{
	  enum register_status status = cooked_read (src, regnum, buf);

	  if (status == REG_VALID)
	    memcpy (register_buffer (dst, regnum), buf,
		    register_size (gdbarch, regnum));
	  else
	    {
	      gdb_assert (status != REG_UNKNOWN);

	      memset (register_buffer (dst, regnum), 0,
		      register_size (gdbarch, regnum));
	    }
	  dst->register_status[regnum] = status;
	}
    }
}

static void
regcache_restore (struct regcache *dst,
		  regcache_cooked_read_ftype *cooked_read,
		  void *cooked_read_context)
{
  struct gdbarch *gdbarch = dst->descr->gdbarch;
  gdb_byte buf[MAX_REGISTER_SIZE];
  int regnum;

  /* The dst had better not be read-only.  If it is, the `restore'
     doesn't make much sense.  */
  gdb_assert (!dst->readonly_p);
  /* Copy over any registers, being careful to only restore those that
     were both saved and need to be restored.  The full [0 .. gdbarch_num_regs
     + gdbarch_num_pseudo_regs) range is checked since some architectures need
     to save/restore `cooked' registers that live in memory.  */
  for (regnum = 0; regnum < dst->descr->nr_cooked_registers; regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, restore_reggroup))
	{
	  enum register_status status;

	  status = cooked_read (cooked_read_context, regnum, buf);
	  if (status == REG_VALID)
	    regcache_cooked_write (dst, regnum, buf);
	}
    }
}

static enum register_status
do_cooked_read (void *src, int regnum, gdb_byte *buf)
{
  struct regcache *regcache = src;

  return regcache_cooked_read (regcache, regnum, buf);
}

void
regcache_cpy (struct regcache *dst, struct regcache *src)
{
  gdb_assert (src != NULL && dst != NULL);
  gdb_assert (src->descr->gdbarch == dst->descr->gdbarch);
  gdb_assert (src != dst);
  gdb_assert (src->readonly_p || dst->readonly_p);

  if (!src->readonly_p)
    regcache_save (dst, do_cooked_read, src);
  else if (!dst->readonly_p)
    regcache_restore (dst, do_cooked_read, src);
  else
    regcache_cpy_no_passthrough (dst, src);
}

void
regcache_cpy_no_passthrough (struct regcache *dst, struct regcache *src)
{
  gdb_assert (src != NULL && dst != NULL);
  gdb_assert (src->descr->gdbarch == dst->descr->gdbarch);
  /* NOTE: cagney/2002-05-17: Don't let the caller do a no-passthrough
     move of data into a thread's regcache.  Doing this would be silly
     - it would mean that regcache->register_status would be
     completely invalid.  */
  gdb_assert (dst->readonly_p && src->readonly_p);

  memcpy (dst->registers, src->registers,
	  dst->descr->sizeof_cooked_registers);
  memcpy (dst->register_status, src->register_status,
	  dst->descr->sizeof_cooked_register_status);
}

struct regcache *
regcache_dup (struct regcache *src)
{
  struct regcache *newbuf;

  newbuf = regcache_xmalloc (src->descr->gdbarch, get_regcache_aspace (src));
  regcache_cpy (newbuf, src);
  return newbuf;
}

enum register_status
regcache_register_status (const struct regcache *regcache, int regnum)
{
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0);
  if (regcache->readonly_p)
    gdb_assert (regnum < regcache->descr->nr_cooked_registers);
  else
    gdb_assert (regnum < regcache->descr->nr_raw_registers);

  return regcache->register_status[regnum];
}

void
regcache_invalidate (struct regcache *regcache, int regnum)
{
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0);
  gdb_assert (!regcache->readonly_p);
  gdb_assert (regnum < regcache->descr->nr_raw_registers);
  regcache->register_status[regnum] = REG_UNKNOWN;
}


/* Global structure containing the current regcache.  */

/* NOTE: this is a write-through cache.  There is no "dirty" bit for
   recording if the register values have been changed (eg. by the
   user).  Therefore all registers must be written back to the
   target when appropriate.  */

struct regcache_list
{
  struct regcache *regcache;
  struct regcache_list *next;
};

static struct regcache_list *current_regcache;

struct regcache *
get_thread_arch_aspace_regcache (ptid_t ptid, struct gdbarch *gdbarch,
				 struct address_space *aspace)
{
  struct regcache_list *list;
  struct regcache *new_regcache;

  for (list = current_regcache; list; list = list->next)
    if (ptid_equal (list->regcache->ptid, ptid)
	&& get_regcache_arch (list->regcache) == gdbarch)
      return list->regcache;

  new_regcache = regcache_xmalloc_1 (gdbarch, aspace, 0);
  new_regcache->ptid = ptid;

  list = xmalloc (sizeof (struct regcache_list));
  list->regcache = new_regcache;
  list->next = current_regcache;
  current_regcache = list;

  return new_regcache;
}

struct regcache *
get_thread_arch_regcache (ptid_t ptid, struct gdbarch *gdbarch)
{
  struct address_space *aspace;

  /* For the benefit of "maint print registers" & co when debugging an
     executable, allow dumping the regcache even when there is no
     thread selected (target_thread_address_space internal-errors if
     no address space is found).  Note that normal user commands will
     fail higher up on the call stack due to no
     target_has_registers.  */
  aspace = (ptid_equal (null_ptid, ptid)
	    ? NULL
	    : target_thread_address_space (ptid));

  return get_thread_arch_aspace_regcache  (ptid, gdbarch, aspace);
}

static ptid_t current_thread_ptid;
static struct gdbarch *current_thread_arch;

struct regcache *
get_thread_regcache (ptid_t ptid)
{
  if (!current_thread_arch || !ptid_equal (current_thread_ptid, ptid))
    {
      current_thread_ptid = ptid;
      current_thread_arch = target_thread_architecture (ptid);
    }

  return get_thread_arch_regcache (ptid, current_thread_arch);
}

struct regcache *
get_current_regcache (void)
{
  return get_thread_regcache (inferior_ptid);
}


/* Observer for the target_changed event.  */

static void
regcache_observer_target_changed (struct target_ops *target)
{
  registers_changed ();
}

/* Update global variables old ptids to hold NEW_PTID if they were
   holding OLD_PTID.  */
static void
regcache_thread_ptid_changed (ptid_t old_ptid, ptid_t new_ptid)
{
  struct regcache_list *list;

  for (list = current_regcache; list; list = list->next)
    if (ptid_equal (list->regcache->ptid, old_ptid))
      list->regcache->ptid = new_ptid;
}

/* Low level examining and depositing of registers.

   The caller is responsible for making sure that the inferior is
   stopped before calling the fetching routines, or it will get
   garbage.  (a change from GDB version 3, in which the caller got the
   value from the last stop).  */

/* REGISTERS_CHANGED ()

   Indicate that registers may have changed, so invalidate the cache.  */

void
registers_changed_ptid (ptid_t ptid)
{
  struct regcache_list *list, **list_link;

  list = current_regcache;
  list_link = &current_regcache;
  while (list)
    {
      if (ptid_match (list->regcache->ptid, ptid))
	{
	  struct regcache_list *dead = list;

	  *list_link = list->next;
	  regcache_xfree (list->regcache);
	  list = *list_link;
	  xfree (dead);
	  continue;
	}

      list_link = &list->next;
      list = *list_link;
    }

  if (ptid_match (current_thread_ptid, ptid))
    {
      current_thread_ptid = null_ptid;
      current_thread_arch = NULL;
    }

  if (ptid_match (inferior_ptid, ptid))
    {
      /* We just deleted the regcache of the current thread.  Need to
	 forget about any frames we have cached, too.  */
      reinit_frame_cache ();
    }
}

void
registers_changed (void)
{
  registers_changed_ptid (minus_one_ptid);

  /* Force cleanup of any alloca areas if using C alloca instead of
     a builtin alloca.  This particular call is used to clean up
     areas allocated by low level target code which may build up
     during lengthy interactions between gdb and the target before
     gdb gives control to the user (ie watchpoints).  */
  alloca (0);
}

enum register_status
regcache_raw_read (struct regcache *regcache, int regnum, gdb_byte *buf)
{
  gdb_assert (regcache != NULL && buf != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  /* Make certain that the register cache is up-to-date with respect
     to the current thread.  This switching shouldn't be necessary
     only there is still only one target side register cache.  Sigh!
     On the bright side, at least there is a regcache object.  */
  if (!regcache->readonly_p
      && regcache_register_status (regcache, regnum) == REG_UNKNOWN)
    {
      struct cleanup *old_chain = save_inferior_ptid ();

      inferior_ptid = regcache->ptid;
      target_fetch_registers (regcache, regnum);
      do_cleanups (old_chain);

      /* A number of targets can't access the whole set of raw
	 registers (because the debug API provides no means to get at
	 them).  */
      if (regcache->register_status[regnum] == REG_UNKNOWN)
	regcache->register_status[regnum] = REG_UNAVAILABLE;
    }

  if (regcache->register_status[regnum] != REG_VALID)
    memset (buf, 0, regcache->descr->sizeof_register[regnum]);
  else
    memcpy (buf, register_buffer (regcache, regnum),
	    regcache->descr->sizeof_register[regnum]);

  return regcache->register_status[regnum];
}

enum register_status
regcache_raw_read_signed (struct regcache *regcache, int regnum, LONGEST *val)
{
  gdb_byte *buf;
  enum register_status status;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  status = regcache_raw_read (regcache, regnum, buf);
  if (status == REG_VALID)
    *val = extract_signed_integer
      (buf, regcache->descr->sizeof_register[regnum],
       gdbarch_byte_order (regcache->descr->gdbarch));
  else
    *val = 0;
  return status;
}

enum register_status
regcache_raw_read_unsigned (struct regcache *regcache, int regnum,
			    ULONGEST *val)
{
  gdb_byte *buf;
  enum register_status status;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  status = regcache_raw_read (regcache, regnum, buf);
  if (status == REG_VALID)
    *val = extract_unsigned_integer
      (buf, regcache->descr->sizeof_register[regnum],
       gdbarch_byte_order (regcache->descr->gdbarch));
  else
    *val = 0;
  return status;
}

void
regcache_raw_write_signed (struct regcache *regcache, int regnum, LONGEST val)
{
  void *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >=0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  store_signed_integer (buf, regcache->descr->sizeof_register[regnum],
			gdbarch_byte_order (regcache->descr->gdbarch), val);
  regcache_raw_write (regcache, regnum, buf);
}

void
regcache_raw_write_unsigned (struct regcache *regcache, int regnum,
			     ULONGEST val)
{
  void *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >=0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  store_unsigned_integer (buf, regcache->descr->sizeof_register[regnum],
			  gdbarch_byte_order (regcache->descr->gdbarch), val);
  regcache_raw_write (regcache, regnum, buf);
}

enum register_status
regcache_cooked_read (struct regcache *regcache, int regnum, gdb_byte *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < regcache->descr->nr_cooked_registers);
  if (regnum < regcache->descr->nr_raw_registers)
    return regcache_raw_read (regcache, regnum, buf);
  else if (regcache->readonly_p
	   && regcache->register_status[regnum] != REG_UNKNOWN)
    {
      /* Read-only register cache, perhaps the cooked value was
	 cached?  */
      if (regcache->register_status[regnum] == REG_VALID)
	memcpy (buf, register_buffer (regcache, regnum),
		regcache->descr->sizeof_register[regnum]);
      else
	memset (buf, 0, regcache->descr->sizeof_register[regnum]);

      return regcache->register_status[regnum];
    }
  else if (gdbarch_pseudo_register_read_value_p (regcache->descr->gdbarch))
    {
      struct value *mark, *computed;
      enum register_status result = REG_VALID;

      mark = value_mark ();

      computed = gdbarch_pseudo_register_read_value (regcache->descr->gdbarch,
						     regcache, regnum);
      if (value_entirely_available (computed))
	memcpy (buf, value_contents_raw (computed),
		regcache->descr->sizeof_register[regnum]);
      else
	{
	  memset (buf, 0, regcache->descr->sizeof_register[regnum]);
	  result = REG_UNAVAILABLE;
	}

      value_free_to_mark (mark);

      return result;
    }
  else
    return gdbarch_pseudo_register_read (regcache->descr->gdbarch, regcache,
					 regnum, buf);
}

struct value *
regcache_cooked_read_value (struct regcache *regcache, int regnum)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < regcache->descr->nr_cooked_registers);

  if (regnum < regcache->descr->nr_raw_registers
      || (regcache->readonly_p
	  && regcache->register_status[regnum] != REG_UNKNOWN)
      || !gdbarch_pseudo_register_read_value_p (regcache->descr->gdbarch))
    {
      struct value *result;

      result = allocate_value (register_type (regcache->descr->gdbarch,
					      regnum));
      VALUE_LVAL (result) = lval_register;
      VALUE_REGNUM (result) = regnum;

      /* It is more efficient in general to do this delegation in this
	 direction than in the other one, even though the value-based
	 API is preferred.  */
      if (regcache_cooked_read (regcache, regnum,
				value_contents_raw (result)) == REG_UNAVAILABLE)
	mark_value_bytes_unavailable (result, 0,
				      TYPE_LENGTH (value_type (result)));

      return result;
    }
  else
    return gdbarch_pseudo_register_read_value (regcache->descr->gdbarch,
					       regcache, regnum);
}

enum register_status
regcache_cooked_read_signed (struct regcache *regcache, int regnum,
			     LONGEST *val)
{
  enum register_status status;
  gdb_byte *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_cooked_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  status = regcache_cooked_read (regcache, regnum, buf);
  if (status == REG_VALID)
    *val = extract_signed_integer
      (buf, regcache->descr->sizeof_register[regnum],
       gdbarch_byte_order (regcache->descr->gdbarch));
  else
    *val = 0;
  return status;
}

enum register_status
regcache_cooked_read_unsigned (struct regcache *regcache, int regnum,
			       ULONGEST *val)
{
  enum register_status status;
  gdb_byte *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_cooked_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  status = regcache_cooked_read (regcache, regnum, buf);
  if (status == REG_VALID)
    *val = extract_unsigned_integer
      (buf, regcache->descr->sizeof_register[regnum],
       gdbarch_byte_order (regcache->descr->gdbarch));
  else
    *val = 0;
  return status;
}

void
regcache_cooked_write_signed (struct regcache *regcache, int regnum,
			      LONGEST val)
{
  void *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >=0 && regnum < regcache->descr->nr_cooked_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  store_signed_integer (buf, regcache->descr->sizeof_register[regnum],
			gdbarch_byte_order (regcache->descr->gdbarch), val);
  regcache_cooked_write (regcache, regnum, buf);
}

void
regcache_cooked_write_unsigned (struct regcache *regcache, int regnum,
				ULONGEST val)
{
  void *buf;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >=0 && regnum < regcache->descr->nr_cooked_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  store_unsigned_integer (buf, regcache->descr->sizeof_register[regnum],
			  gdbarch_byte_order (regcache->descr->gdbarch), val);
  regcache_cooked_write (regcache, regnum, buf);
}

void
regcache_raw_write (struct regcache *regcache, int regnum,
		    const gdb_byte *buf)
{
  struct cleanup *old_chain;

  gdb_assert (regcache != NULL && buf != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  gdb_assert (!regcache->readonly_p);

  /* On the sparc, writing %g0 is a no-op, so we don't even want to
     change the registers array if something writes to this register.  */
  if (gdbarch_cannot_store_register (get_regcache_arch (regcache), regnum))
    return;

  /* If we have a valid copy of the register, and new value == old
     value, then don't bother doing the actual store.  */
  if (regcache_register_status (regcache, regnum) == REG_VALID
      && (memcmp (register_buffer (regcache, regnum), buf,
		  regcache->descr->sizeof_register[regnum]) == 0))
    return;

  old_chain = save_inferior_ptid ();
  inferior_ptid = regcache->ptid;

  target_prepare_to_store (regcache);
  memcpy (register_buffer (regcache, regnum), buf,
	  regcache->descr->sizeof_register[regnum]);
  regcache->register_status[regnum] = REG_VALID;
  target_store_registers (regcache, regnum);

  do_cleanups (old_chain);
}

void
regcache_cooked_write (struct regcache *regcache, int regnum,
		       const gdb_byte *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < regcache->descr->nr_cooked_registers);
  if (regnum < regcache->descr->nr_raw_registers)
    regcache_raw_write (regcache, regnum, buf);
  else
    gdbarch_pseudo_register_write (regcache->descr->gdbarch, regcache,
				   regnum, buf);
}

/* Perform a partial register transfer using a read, modify, write
   operation.  */

typedef void (regcache_read_ftype) (struct regcache *regcache, int regnum,
				    void *buf);
typedef void (regcache_write_ftype) (struct regcache *regcache, int regnum,
				     const void *buf);

static enum register_status
regcache_xfer_part (struct regcache *regcache, int regnum,
		    int offset, int len, void *in, const void *out,
		    enum register_status (*xread) (struct regcache *regcache,
						  int regnum,
						  gdb_byte *buf),
		    void (*write) (struct regcache *regcache, int regnum,
				   const gdb_byte *buf))
{
  struct regcache_descr *descr = regcache->descr;
  gdb_byte reg[MAX_REGISTER_SIZE];

  gdb_assert (offset >= 0 && offset <= descr->sizeof_register[regnum]);
  gdb_assert (len >= 0 && offset + len <= descr->sizeof_register[regnum]);
  /* Something to do?  */
  if (offset + len == 0)
    return REG_VALID;
  /* Read (when needed) ...  */
  if (in != NULL
      || offset > 0
      || offset + len < descr->sizeof_register[regnum])
    {
      enum register_status status;

      gdb_assert (xread != NULL);
      status = xread (regcache, regnum, reg);
      if (status != REG_VALID)
	return status;
    }
  /* ... modify ...  */
  if (in != NULL)
    memcpy (in, reg + offset, len);
  if (out != NULL)
    memcpy (reg + offset, out, len);
  /* ... write (when needed).  */
  if (out != NULL)
    {
      gdb_assert (write != NULL);
      write (regcache, regnum, reg);
    }

  return REG_VALID;
}

enum register_status
regcache_raw_read_part (struct regcache *regcache, int regnum,
			int offset, int len, gdb_byte *buf)
{
  struct regcache_descr *descr = regcache->descr;

  gdb_assert (regnum >= 0 && regnum < descr->nr_raw_registers);
  return regcache_xfer_part (regcache, regnum, offset, len, buf, NULL,
			     regcache_raw_read, regcache_raw_write);
}

void
regcache_raw_write_part (struct regcache *regcache, int regnum,
			 int offset, int len, const gdb_byte *buf)
{
  struct regcache_descr *descr = regcache->descr;

  gdb_assert (regnum >= 0 && regnum < descr->nr_raw_registers);
  regcache_xfer_part (regcache, regnum, offset, len, NULL, buf,
		      regcache_raw_read, regcache_raw_write);
}

enum register_status
regcache_cooked_read_part (struct regcache *regcache, int regnum,
			   int offset, int len, gdb_byte *buf)
{
  struct regcache_descr *descr = regcache->descr;

  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  return regcache_xfer_part (regcache, regnum, offset, len, buf, NULL,
			     regcache_cooked_read, regcache_cooked_write);
}

void
regcache_cooked_write_part (struct regcache *regcache, int regnum,
			    int offset, int len, const gdb_byte *buf)
{
  struct regcache_descr *descr = regcache->descr;

  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  regcache_xfer_part (regcache, regnum, offset, len, NULL, buf,
		      regcache_cooked_read, regcache_cooked_write);
}

/* Supply register REGNUM, whose contents are stored in BUF, to REGCACHE.  */

void
regcache_raw_supply (struct regcache *regcache, int regnum, const void *buf)
{
  void *regbuf;
  size_t size;

  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  gdb_assert (!regcache->readonly_p);

  regbuf = register_buffer (regcache, regnum);
  size = regcache->descr->sizeof_register[regnum];

  if (buf)
    {
      memcpy (regbuf, buf, size);
      regcache->register_status[regnum] = REG_VALID;
    }
  else
    {
      /* This memset not strictly necessary, but better than garbage
	 in case the register value manages to escape somewhere (due
	 to a bug, no less).  */
      memset (regbuf, 0, size);
      regcache->register_status[regnum] = REG_UNAVAILABLE;
    }
}

/* Collect register REGNUM from REGCACHE and store its contents in BUF.  */

void
regcache_raw_collect (const struct regcache *regcache, int regnum, void *buf)
{
  const void *regbuf;
  size_t size;

  gdb_assert (regcache != NULL && buf != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);

  regbuf = register_buffer (regcache, regnum);
  size = regcache->descr->sizeof_register[regnum];
  memcpy (buf, regbuf, size);
}


/* Special handling for register PC.  */

CORE_ADDR
regcache_read_pc (struct regcache *regcache)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  CORE_ADDR pc_val;

  if (gdbarch_read_pc_p (gdbarch))
    pc_val = gdbarch_read_pc (gdbarch, regcache);
  /* Else use per-frame method on get_current_frame.  */
  else if (gdbarch_pc_regnum (gdbarch) >= 0)
    {
      ULONGEST raw_val;

      if (regcache_cooked_read_unsigned (regcache,
					 gdbarch_pc_regnum (gdbarch),
					 &raw_val) == REG_UNAVAILABLE)
	throw_error (NOT_AVAILABLE_ERROR, _("PC register is not available"));

      pc_val = gdbarch_addr_bits_remove (gdbarch, raw_val);
    }
  else
    internal_error (__FILE__, __LINE__,
		    _("regcache_read_pc: Unable to find PC"));
  return pc_val;
}

void
regcache_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (gdbarch_write_pc_p (gdbarch))
    gdbarch_write_pc (gdbarch, regcache, pc);
  else if (gdbarch_pc_regnum (gdbarch) >= 0)
    regcache_cooked_write_unsigned (regcache,
				    gdbarch_pc_regnum (gdbarch), pc);
  else
    internal_error (__FILE__, __LINE__,
		    _("regcache_write_pc: Unable to update PC"));

  /* Writing the PC (for instance, from "load") invalidates the
     current frame.  */
  reinit_frame_cache ();
}


static void
reg_flush_command (char *command, int from_tty)
{
  /* Force-flush the register cache.  */
  registers_changed ();
  if (from_tty)
    printf_filtered (_("Register cache flushed.\n"));
}

static void
dump_endian_bytes (struct ui_file *file, enum bfd_endian endian,
		   const gdb_byte *buf, long len)
{
  int i;

  switch (endian)
    {
    case BFD_ENDIAN_BIG:
      for (i = 0; i < len; i++)
	fprintf_unfiltered (file, "%02x", buf[i]);
      break;
    case BFD_ENDIAN_LITTLE:
      for (i = len - 1; i >= 0; i--)
	fprintf_unfiltered (file, "%02x", buf[i]);
      break;
    default:
      internal_error (__FILE__, __LINE__, _("Bad switch"));
    }
}

enum regcache_dump_what
{
  regcache_dump_none, regcache_dump_raw,
  regcache_dump_cooked, regcache_dump_groups,
  regcache_dump_remote
};

static void
regcache_dump (struct regcache *regcache, struct ui_file *file,
	       enum regcache_dump_what what_to_dump)
{
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);
  struct gdbarch *gdbarch = regcache->descr->gdbarch;
  int regnum;
  int footnote_nr = 0;
  int footnote_register_size = 0;
  int footnote_register_offset = 0;
  int footnote_register_type_name_null = 0;
  long register_offset = 0;
  gdb_byte buf[MAX_REGISTER_SIZE];

#if 0
  fprintf_unfiltered (file, "nr_raw_registers %d\n",
		      regcache->descr->nr_raw_registers);
  fprintf_unfiltered (file, "nr_cooked_registers %d\n",
		      regcache->descr->nr_cooked_registers);
  fprintf_unfiltered (file, "sizeof_raw_registers %ld\n",
		      regcache->descr->sizeof_raw_registers);
  fprintf_unfiltered (file, "sizeof_raw_register_status %ld\n",
		      regcache->descr->sizeof_raw_register_status);
  fprintf_unfiltered (file, "gdbarch_num_regs %d\n", 
		      gdbarch_num_regs (gdbarch));
  fprintf_unfiltered (file, "gdbarch_num_pseudo_regs %d\n",
		      gdbarch_num_pseudo_regs (gdbarch));
#endif

  gdb_assert (regcache->descr->nr_cooked_registers
	      == (gdbarch_num_regs (gdbarch)
		  + gdbarch_num_pseudo_regs (gdbarch)));

  for (regnum = -1; regnum < regcache->descr->nr_cooked_registers; regnum++)
    {
      /* Name.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %-10s", "Name");
      else
	{
	  const char *p = gdbarch_register_name (gdbarch, regnum);

	  if (p == NULL)
	    p = "";
	  else if (p[0] == '\0')
	    p = "''";
	  fprintf_unfiltered (file, " %-10s", p);
	}

      /* Number.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %4s", "Nr");
      else
	fprintf_unfiltered (file, " %4d", regnum);

      /* Relative number.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %4s", "Rel");
      else if (regnum < gdbarch_num_regs (gdbarch))
	fprintf_unfiltered (file, " %4d", regnum);
      else
	fprintf_unfiltered (file, " %4d",
			    (regnum - gdbarch_num_regs (gdbarch)));

      /* Offset.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %6s  ", "Offset");
      else
	{
	  fprintf_unfiltered (file, " %6ld",
			      regcache->descr->register_offset[regnum]);
	  if (register_offset != regcache->descr->register_offset[regnum]
	      || (regnum > 0
		  && (regcache->descr->register_offset[regnum]
		      != (regcache->descr->register_offset[regnum - 1]
			  + regcache->descr->sizeof_register[regnum - 1])))
	      )
	    {
	      if (!footnote_register_offset)
		footnote_register_offset = ++footnote_nr;
	      fprintf_unfiltered (file, "*%d", footnote_register_offset);
	    }
	  else
	    fprintf_unfiltered (file, "  ");
	  register_offset = (regcache->descr->register_offset[regnum]
			     + regcache->descr->sizeof_register[regnum]);
	}

      /* Size.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %5s ", "Size");
      else
	fprintf_unfiltered (file, " %5ld",
			    regcache->descr->sizeof_register[regnum]);

      /* Type.  */
      {
	const char *t;

	if (regnum < 0)
	  t = "Type";
	else
	  {
	    static const char blt[] = "builtin_type";

	    t = TYPE_NAME (register_type (regcache->descr->gdbarch, regnum));
	    if (t == NULL)
	      {
		char *n;

		if (!footnote_register_type_name_null)
		  footnote_register_type_name_null = ++footnote_nr;
		n = xstrprintf ("*%d", footnote_register_type_name_null);
		make_cleanup (xfree, n);
		t = n;
	      }
	    /* Chop a leading builtin_type.  */
	    if (strncmp (t, blt, strlen (blt)) == 0)
	      t += strlen (blt);
	  }
	fprintf_unfiltered (file, " %-15s", t);
      }

      /* Leading space always present.  */
      fprintf_unfiltered (file, " ");

      /* Value, raw.  */
      if (what_to_dump == regcache_dump_raw)
	{
	  if (regnum < 0)
	    fprintf_unfiltered (file, "Raw value");
	  else if (regnum >= regcache->descr->nr_raw_registers)
	    fprintf_unfiltered (file, "<cooked>");
	  else if (regcache_register_status (regcache, regnum) == REG_UNKNOWN)
	    fprintf_unfiltered (file, "<invalid>");
	  else if (regcache_register_status (regcache, regnum) == REG_UNAVAILABLE)
	    fprintf_unfiltered (file, "<unavailable>");
	  else
	    {
	      regcache_raw_read (regcache, regnum, buf);
	      fprintf_unfiltered (file, "0x");
	      dump_endian_bytes (file,
				 gdbarch_byte_order (gdbarch), buf,
				 regcache->descr->sizeof_register[regnum]);
	    }
	}

      /* Value, cooked.  */
      if (what_to_dump == regcache_dump_cooked)
	{
	  if (regnum < 0)
	    fprintf_unfiltered (file, "Cooked value");
	  else
	    {
	      enum register_status status;

	      status = regcache_cooked_read (regcache, regnum, buf);
	      if (status == REG_UNKNOWN)
		fprintf_unfiltered (file, "<invalid>");
	      else if (status == REG_UNAVAILABLE)
		fprintf_unfiltered (file, "<unavailable>");
	      else
		{
		  fprintf_unfiltered (file, "0x");
		  dump_endian_bytes (file,
				     gdbarch_byte_order (gdbarch), buf,
				     regcache->descr->sizeof_register[regnum]);
		}
	    }
	}

      /* Group members.  */
      if (what_to_dump == regcache_dump_groups)
	{
	  if (regnum < 0)
	    fprintf_unfiltered (file, "Groups");
	  else
	    {
	      const char *sep = "";
	      struct reggroup *group;

	      for (group = reggroup_next (gdbarch, NULL);
		   group != NULL;
		   group = reggroup_next (gdbarch, group))
		{
		  if (gdbarch_register_reggroup_p (gdbarch, regnum, group))
		    {
		      fprintf_unfiltered (file,
					  "%s%s", sep, reggroup_name (group));
		      sep = ",";
		    }
		}
	    }
	}

      /* Remote packet configuration.  */
      if (what_to_dump == regcache_dump_remote)
	{
	  if (regnum < 0)
	    {
	      fprintf_unfiltered (file, "Rmt Nr  g/G Offset");
	    }
	  else if (regnum < regcache->descr->nr_raw_registers)
	    {
	      int pnum, poffset;

	      if (remote_register_number_and_offset (get_regcache_arch (regcache), regnum,
						     &pnum, &poffset))
		fprintf_unfiltered (file, "%7d %11d", pnum, poffset);
	    }
	}

      fprintf_unfiltered (file, "\n");
    }

  if (footnote_register_size)
    fprintf_unfiltered (file, "*%d: Inconsistent register sizes.\n",
			footnote_register_size);
  if (footnote_register_offset)
    fprintf_unfiltered (file, "*%d: Inconsistent register offsets.\n",
			footnote_register_offset);
  if (footnote_register_type_name_null)
    fprintf_unfiltered (file, 
			"*%d: Register type's name NULL.\n",
			footnote_register_type_name_null);
  do_cleanups (cleanups);
}

static void
regcache_print (char *args, enum regcache_dump_what what_to_dump)
{
  if (args == NULL)
    regcache_dump (get_current_regcache (), gdb_stdout, what_to_dump);
  else
    {
      struct cleanup *cleanups;
      struct ui_file *file = gdb_fopen (args, "w");

      if (file == NULL)
	perror_with_name (_("maintenance print architecture"));
      cleanups = make_cleanup_ui_file_delete (file);
      regcache_dump (get_current_regcache (), file, what_to_dump);
      do_cleanups (cleanups);
    }
}

static void
maintenance_print_registers (char *args, int from_tty)
{
  regcache_print (args, regcache_dump_none);
}

static void
maintenance_print_raw_registers (char *args, int from_tty)
{
  regcache_print (args, regcache_dump_raw);
}

static void
maintenance_print_cooked_registers (char *args, int from_tty)
{
  regcache_print (args, regcache_dump_cooked);
}

static void
maintenance_print_register_groups (char *args, int from_tty)
{
  regcache_print (args, regcache_dump_groups);
}

static void
maintenance_print_remote_registers (char *args, int from_tty)
{
  regcache_print (args, regcache_dump_remote);
}

extern initialize_file_ftype _initialize_regcache; /* -Wmissing-prototype */

void
_initialize_regcache (void)
{
  regcache_descr_handle
    = gdbarch_data_register_post_init (init_regcache_descr);

  observer_attach_target_changed (regcache_observer_target_changed);
  observer_attach_thread_ptid_changed (regcache_thread_ptid_changed);

  add_com ("flushregs", class_maintenance, reg_flush_command,
	   _("Force gdb to flush its register cache (maintainer command)"));

  add_cmd ("registers", class_maintenance, maintenance_print_registers,
	   _("Print the internal register configuration.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("raw-registers", class_maintenance,
	   maintenance_print_raw_registers,
	   _("Print the internal register configuration "
	     "including raw values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("cooked-registers", class_maintenance,
	   maintenance_print_cooked_registers,
	   _("Print the internal register configuration "
	     "including cooked values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("register-groups", class_maintenance,
	   maintenance_print_register_groups,
	   _("Print the internal register configuration "
	     "including each register's group.\n"
	     "Takes an optional file parameter."),
	   &maintenanceprintlist);
  add_cmd ("remote-registers", class_maintenance,
	   maintenance_print_remote_registers, _("\
Print the internal register configuration including each register's\n\
remote register number and buffer offset in the g/G packets.\n\
Takes an optional file parameter."),
	   &maintenanceprintlist);

}
