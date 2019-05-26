/* Cache and manage the values of registers for GDB, the GNU debugger.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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
#include "gdbthread.h"
#include "target.h"
#include "test-target.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "reggroups.h"
#include "observable.h"
#include "regset.h"
#include <forward_list>

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
  long sizeof_raw_registers;

  /* The cooked register space.  Each cooked register in the range
     [0..NR_RAW_REGISTERS) is direct-mapped onto the corresponding raw
     register.  The remaining [NR_RAW_REGISTERS
     .. NR_COOKED_REGISTERS) (a.k.a. pseudo registers) are mapped onto
     both raw registers and memory by the architecture methods
     gdbarch_pseudo_register_read and gdbarch_pseudo_register_write.  */
  int nr_cooked_registers;
  long sizeof_cooked_registers;

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
  descr->nr_cooked_registers = gdbarch_num_cooked_regs (gdbarch);

  /* Fill in a table of register types.  */
  descr->register_type
    = GDBARCH_OBSTACK_CALLOC (gdbarch, descr->nr_cooked_registers,
			      struct type *);
  for (i = 0; i < descr->nr_cooked_registers; i++)
    descr->register_type[i] = gdbarch_register_type (gdbarch, i);

  /* Construct a strictly RAW register cache.  Don't allow pseudo's
     into the register cache.  */

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
    for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
      {
	descr->sizeof_register[i] = TYPE_LENGTH (descr->register_type[i]);
	descr->register_offset[i] = offset;
	offset += descr->sizeof_register[i];
      }
    /* Set the real size of the raw register cache buffer.  */
    descr->sizeof_raw_registers = offset;

    for (; i < descr->nr_cooked_registers; i++)
      {
	descr->sizeof_register[i] = TYPE_LENGTH (descr->register_type[i]);
	descr->register_offset[i] = offset;
	offset += descr->sizeof_register[i];
      }
    /* Set the real size of the readonly register cache buffer.  */
    descr->sizeof_cooked_registers = offset;
  }

  return descr;
}

static struct regcache_descr *
regcache_descr (struct gdbarch *gdbarch)
{
  return (struct regcache_descr *) gdbarch_data (gdbarch,
						 regcache_descr_handle);
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

  gdb_assert (regnum >= 0 && regnum < gdbarch_num_cooked_regs (gdbarch));
  size = descr->sizeof_register[regnum];
  return size;
}

/* See common/common-regcache.h.  */

int
regcache_register_size (const struct regcache *regcache, int n)
{
  return register_size (regcache->arch (), n);
}

reg_buffer::reg_buffer (gdbarch *gdbarch, bool has_pseudo)
  : m_has_pseudo (has_pseudo)
{
  gdb_assert (gdbarch != NULL);
  m_descr = regcache_descr (gdbarch);

  if (has_pseudo)
    {
      m_registers.reset (new gdb_byte[m_descr->sizeof_cooked_registers] ());
      m_register_status.reset
	(new register_status[m_descr->nr_cooked_registers] ());
    }
  else
    {
      m_registers.reset (new gdb_byte[m_descr->sizeof_raw_registers] ());
      m_register_status.reset
	(new register_status[gdbarch_num_regs (gdbarch)] ());
    }
}

regcache::regcache (gdbarch *gdbarch, const address_space *aspace_)
/* The register buffers.  A read/write register cache can only hold
   [0 .. gdbarch_num_regs).  */
  : detached_regcache (gdbarch, false), m_aspace (aspace_)
{
  m_ptid = minus_one_ptid;
}

readonly_detached_regcache::readonly_detached_regcache (regcache &src)
  : readonly_detached_regcache (src.arch (),
				[&src] (int regnum, gdb_byte *buf)
				  {
				    return src.cooked_read (regnum, buf);
				  })
{
}

gdbarch *
reg_buffer::arch () const
{
  return m_descr->gdbarch;
}

/* Return  a pointer to register REGNUM's buffer cache.  */

gdb_byte *
reg_buffer::register_buffer (int regnum) const
{
  return m_registers.get () + m_descr->register_offset[regnum];
}

void
reg_buffer::save (register_read_ftype cooked_read)
{
  struct gdbarch *gdbarch = m_descr->gdbarch;
  int regnum;

  /* It should have pseudo registers.  */
  gdb_assert (m_has_pseudo);
  /* Clear the dest.  */
  memset (m_registers.get (), 0, m_descr->sizeof_cooked_registers);
  memset (m_register_status.get (), REG_UNKNOWN, m_descr->nr_cooked_registers);
  /* Copy over any registers (identified by their membership in the
     save_reggroup) and mark them as valid.  The full [0 .. gdbarch_num_regs +
     gdbarch_num_pseudo_regs) range is checked since some architectures need
     to save/restore `cooked' registers that live in memory.  */
  for (regnum = 0; regnum < m_descr->nr_cooked_registers; regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, save_reggroup))
	{
	  gdb_byte *dst_buf = register_buffer (regnum);
	  enum register_status status = cooked_read (regnum, dst_buf);

	  gdb_assert (status != REG_UNKNOWN);

	  if (status != REG_VALID)
	    memset (dst_buf, 0, register_size (gdbarch, regnum));

	  m_register_status[regnum] = status;
	}
    }
}

void
regcache::restore (readonly_detached_regcache *src)
{
  struct gdbarch *gdbarch = m_descr->gdbarch;
  int regnum;

  gdb_assert (src != NULL);
  gdb_assert (src->m_has_pseudo);

  gdb_assert (gdbarch == src->arch ());

  /* Copy over any registers, being careful to only restore those that
     were both saved and need to be restored.  The full [0 .. gdbarch_num_regs
     + gdbarch_num_pseudo_regs) range is checked since some architectures need
     to save/restore `cooked' registers that live in memory.  */
  for (regnum = 0; regnum < m_descr->nr_cooked_registers; regnum++)
    {
      if (gdbarch_register_reggroup_p (gdbarch, regnum, restore_reggroup))
	{
	  if (src->m_register_status[regnum] == REG_VALID)
	    cooked_write (regnum, src->register_buffer (regnum));
	}
    }
}

/* See common/common-regcache.h.  */

enum register_status
reg_buffer::get_register_status (int regnum) const
{
  assert_regnum (regnum);

  return m_register_status[regnum];
}

void
reg_buffer::invalidate (int regnum)
{
  assert_regnum (regnum);
  m_register_status[regnum] = REG_UNKNOWN;
}

void
reg_buffer::assert_regnum (int regnum) const
{
  gdb_assert (regnum >= 0);
  if (m_has_pseudo)
    gdb_assert (regnum < m_descr->nr_cooked_registers);
  else
    gdb_assert (regnum < gdbarch_num_regs (arch ()));
}

/* Global structure containing the current regcache.  */

/* NOTE: this is a write-through cache.  There is no "dirty" bit for
   recording if the register values have been changed (eg. by the
   user).  Therefore all registers must be written back to the
   target when appropriate.  */
std::forward_list<regcache *> regcache::current_regcache;

struct regcache *
get_thread_arch_aspace_regcache (ptid_t ptid, struct gdbarch *gdbarch,
				 struct address_space *aspace)
{
  for (const auto &regcache : regcache::current_regcache)
    if (regcache->ptid () == ptid && regcache->arch () == gdbarch)
      return regcache;

  regcache *new_regcache = new regcache (gdbarch, aspace);

  regcache::current_regcache.push_front (new_regcache);
  new_regcache->set_ptid (ptid);

  return new_regcache;
}

struct regcache *
get_thread_arch_regcache (ptid_t ptid, struct gdbarch *gdbarch)
{
  address_space *aspace = target_thread_address_space (ptid);

  return get_thread_arch_aspace_regcache  (ptid, gdbarch, aspace);
}

static ptid_t current_thread_ptid;
static struct gdbarch *current_thread_arch;

struct regcache *
get_thread_regcache (ptid_t ptid)
{
  if (!current_thread_arch || current_thread_ptid != ptid)
    {
      current_thread_ptid = ptid;
      current_thread_arch = target_thread_architecture (ptid);
    }

  return get_thread_arch_regcache (ptid, current_thread_arch);
}

/* See regcache.h.  */

struct regcache *
get_thread_regcache (thread_info *thread)
{
  return get_thread_regcache (thread->ptid);
}

struct regcache *
get_current_regcache (void)
{
  return get_thread_regcache (inferior_thread ());
}

/* See common/common-regcache.h.  */

struct regcache *
get_thread_regcache_for_ptid (ptid_t ptid)
{
  return get_thread_regcache (ptid);
}

/* Observer for the target_changed event.  */

static void
regcache_observer_target_changed (struct target_ops *target)
{
  registers_changed ();
}

/* Update global variables old ptids to hold NEW_PTID if they were
   holding OLD_PTID.  */
void
regcache::regcache_thread_ptid_changed (ptid_t old_ptid, ptid_t new_ptid)
{
  for (auto &regcache : regcache::current_regcache)
    {
      if (regcache->ptid () == old_ptid)
	regcache->set_ptid (new_ptid);
    }
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
  for (auto oit = regcache::current_regcache.before_begin (),
	 it = std::next (oit);
       it != regcache::current_regcache.end ();
       )
    {
      if ((*it)->ptid ().matches (ptid))
	{
	  delete *it;
	  it = regcache::current_regcache.erase_after (oit);
	}
      else
	oit = it++;
    }

  if (current_thread_ptid.matches (ptid))
    {
      current_thread_ptid = null_ptid;
      current_thread_arch = NULL;
    }

  if (inferior_ptid.matches (ptid))
    {
      /* We just deleted the regcache of the current thread.  Need to
	 forget about any frames we have cached, too.  */
      reinit_frame_cache ();
    }
}

/* See regcache.h.  */

void
registers_changed_thread (thread_info *thread)
{
  registers_changed_ptid (thread->ptid);
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

void
regcache::raw_update (int regnum)
{
  assert_regnum (regnum);

  /* Make certain that the register cache is up-to-date with respect
     to the current thread.  This switching shouldn't be necessary
     only there is still only one target side register cache.  Sigh!
     On the bright side, at least there is a regcache object.  */

  if (get_register_status (regnum) == REG_UNKNOWN)
    {
      target_fetch_registers (this, regnum);

      /* A number of targets can't access the whole set of raw
	 registers (because the debug API provides no means to get at
	 them).  */
      if (m_register_status[regnum] == REG_UNKNOWN)
	m_register_status[regnum] = REG_UNAVAILABLE;
    }
}

enum register_status
readable_regcache::raw_read (int regnum, gdb_byte *buf)
{
  gdb_assert (buf != NULL);
  raw_update (regnum);

  if (m_register_status[regnum] != REG_VALID)
    memset (buf, 0, m_descr->sizeof_register[regnum]);
  else
    memcpy (buf, register_buffer (regnum),
	    m_descr->sizeof_register[regnum]);

  return m_register_status[regnum];
}

enum register_status
regcache_raw_read_signed (struct regcache *regcache, int regnum, LONGEST *val)
{
  gdb_assert (regcache != NULL);
  return regcache->raw_read (regnum, val);
}

template<typename T, typename>
enum register_status
readable_regcache::raw_read (int regnum, T *val)
{
  gdb_byte *buf;
  enum register_status status;

  assert_regnum (regnum);
  buf = (gdb_byte *) alloca (m_descr->sizeof_register[regnum]);
  status = raw_read (regnum, buf);
  if (status == REG_VALID)
    *val = extract_integer<T> (buf,
			       m_descr->sizeof_register[regnum],
			       gdbarch_byte_order (m_descr->gdbarch));
  else
    *val = 0;
  return status;
}

enum register_status
regcache_raw_read_unsigned (struct regcache *regcache, int regnum,
			    ULONGEST *val)
{
  gdb_assert (regcache != NULL);
  return regcache->raw_read (regnum, val);
}

void
regcache_raw_write_signed (struct regcache *regcache, int regnum, LONGEST val)
{
  gdb_assert (regcache != NULL);
  regcache->raw_write (regnum, val);
}

template<typename T, typename>
void
regcache::raw_write (int regnum, T val)
{
  gdb_byte *buf;

  assert_regnum (regnum);
  buf = (gdb_byte *) alloca (m_descr->sizeof_register[regnum]);
  store_integer (buf, m_descr->sizeof_register[regnum],
		 gdbarch_byte_order (m_descr->gdbarch), val);
  raw_write (regnum, buf);
}

void
regcache_raw_write_unsigned (struct regcache *regcache, int regnum,
			     ULONGEST val)
{
  gdb_assert (regcache != NULL);
  regcache->raw_write (regnum, val);
}

LONGEST
regcache_raw_get_signed (struct regcache *regcache, int regnum)
{
  LONGEST value;
  enum register_status status;

  status = regcache_raw_read_signed (regcache, regnum, &value);
  if (status == REG_UNAVAILABLE)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("Register %d is not available"), regnum);
  return value;
}

enum register_status
readable_regcache::cooked_read (int regnum, gdb_byte *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < m_descr->nr_cooked_registers);
  if (regnum < num_raw_registers ())
    return raw_read (regnum, buf);
  else if (m_has_pseudo
	   && m_register_status[regnum] != REG_UNKNOWN)
    {
      if (m_register_status[regnum] == REG_VALID)
	memcpy (buf, register_buffer (regnum),
		m_descr->sizeof_register[regnum]);
      else
	memset (buf, 0, m_descr->sizeof_register[regnum]);

      return m_register_status[regnum];
    }
  else if (gdbarch_pseudo_register_read_value_p (m_descr->gdbarch))
    {
      struct value *mark, *computed;
      enum register_status result = REG_VALID;

      mark = value_mark ();

      computed = gdbarch_pseudo_register_read_value (m_descr->gdbarch,
						     this, regnum);
      if (value_entirely_available (computed))
	memcpy (buf, value_contents_raw (computed),
		m_descr->sizeof_register[regnum]);
      else
	{
	  memset (buf, 0, m_descr->sizeof_register[regnum]);
	  result = REG_UNAVAILABLE;
	}

      value_free_to_mark (mark);

      return result;
    }
  else
    return gdbarch_pseudo_register_read (m_descr->gdbarch, this,
					 regnum, buf);
}

struct value *
readable_regcache::cooked_read_value (int regnum)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < m_descr->nr_cooked_registers);

  if (regnum < num_raw_registers ()
      || (m_has_pseudo && m_register_status[regnum] != REG_UNKNOWN)
      || !gdbarch_pseudo_register_read_value_p (m_descr->gdbarch))
    {
      struct value *result;

      result = allocate_value (register_type (m_descr->gdbarch, regnum));
      VALUE_LVAL (result) = lval_register;
      VALUE_REGNUM (result) = regnum;

      /* It is more efficient in general to do this delegation in this
	 direction than in the other one, even though the value-based
	 API is preferred.  */
      if (cooked_read (regnum,
		       value_contents_raw (result)) == REG_UNAVAILABLE)
	mark_value_bytes_unavailable (result, 0,
				      TYPE_LENGTH (value_type (result)));

      return result;
    }
  else
    return gdbarch_pseudo_register_read_value (m_descr->gdbarch,
					       this, regnum);
}

enum register_status
regcache_cooked_read_signed (struct regcache *regcache, int regnum,
			     LONGEST *val)
{
  gdb_assert (regcache != NULL);
  return regcache->cooked_read (regnum, val);
}

template<typename T, typename>
enum register_status
readable_regcache::cooked_read (int regnum, T *val)
{
  enum register_status status;
  gdb_byte *buf;

  gdb_assert (regnum >= 0 && regnum < m_descr->nr_cooked_registers);
  buf = (gdb_byte *) alloca (m_descr->sizeof_register[regnum]);
  status = cooked_read (regnum, buf);
  if (status == REG_VALID)
    *val = extract_integer<T> (buf, m_descr->sizeof_register[regnum],
			       gdbarch_byte_order (m_descr->gdbarch));
  else
    *val = 0;
  return status;
}

enum register_status
regcache_cooked_read_unsigned (struct regcache *regcache, int regnum,
			       ULONGEST *val)
{
  gdb_assert (regcache != NULL);
  return regcache->cooked_read (regnum, val);
}

void
regcache_cooked_write_signed (struct regcache *regcache, int regnum,
			      LONGEST val)
{
  gdb_assert (regcache != NULL);
  regcache->cooked_write (regnum, val);
}

template<typename T, typename>
void
regcache::cooked_write (int regnum, T val)
{
  gdb_byte *buf;

  gdb_assert (regnum >=0 && regnum < m_descr->nr_cooked_registers);
  buf = (gdb_byte *) alloca (m_descr->sizeof_register[regnum]);
  store_integer (buf, m_descr->sizeof_register[regnum],
		 gdbarch_byte_order (m_descr->gdbarch), val);
  cooked_write (regnum, buf);
}

void
regcache_cooked_write_unsigned (struct regcache *regcache, int regnum,
				ULONGEST val)
{
  gdb_assert (regcache != NULL);
  regcache->cooked_write (regnum, val);
}

void
regcache::raw_write (int regnum, const gdb_byte *buf)
{

  gdb_assert (buf != NULL);
  assert_regnum (regnum);

  /* On the sparc, writing %g0 is a no-op, so we don't even want to
     change the registers array if something writes to this register.  */
  if (gdbarch_cannot_store_register (arch (), regnum))
    return;

  /* If we have a valid copy of the register, and new value == old
     value, then don't bother doing the actual store.  */
  if (get_register_status (regnum) == REG_VALID
      && (memcmp (register_buffer (regnum), buf,
		  m_descr->sizeof_register[regnum]) == 0))
    return;

  target_prepare_to_store (this);
  raw_supply (regnum, buf);

  /* Invalidate the register after it is written, in case of a
     failure.  */
  auto invalidator
    = make_scope_exit ([&] { this->invalidate (regnum); });

  target_store_registers (this, regnum);

  /* The target did not throw an error so we can discard invalidating
     the register.  */
  invalidator.release ();
}

void
regcache::cooked_write (int regnum, const gdb_byte *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < m_descr->nr_cooked_registers);
  if (regnum < num_raw_registers ())
    raw_write (regnum, buf);
  else
    gdbarch_pseudo_register_write (m_descr->gdbarch, this,
				   regnum, buf);
}

/* See regcache.h.  */

enum register_status
readable_regcache::read_part (int regnum, int offset, int len,
			      gdb_byte *out, bool is_raw)
{
  int reg_size = register_size (arch (), regnum);

  gdb_assert (out != NULL);
  gdb_assert (offset >= 0 && offset <= reg_size);
  gdb_assert (len >= 0 && offset + len <= reg_size);

  if (offset == 0 && len == 0)
    {
      /* Nothing to do.  */
      return REG_VALID;
    }

  if (offset == 0 && len == reg_size)
    {
      /* Read the full register.  */
      return (is_raw) ? raw_read (regnum, out) : cooked_read (regnum, out);
    }

  enum register_status status;
  gdb_byte *reg = (gdb_byte *) alloca (reg_size);

  /* Read full register to buffer.  */
  status = (is_raw) ? raw_read (regnum, reg) : cooked_read (regnum, reg);
  if (status != REG_VALID)
    return status;

  /* Copy out.  */
  memcpy (out, reg + offset, len);
  return REG_VALID;
}

/* See regcache.h.  */

void
reg_buffer::raw_collect_part (int regnum, int offset, int len,
			      gdb_byte *out) const
{
  int reg_size = register_size (arch (), regnum);

  gdb_assert (out != nullptr);
  gdb_assert (offset >= 0 && offset <= reg_size);
  gdb_assert (len >= 0 && offset + len <= reg_size);

  if (offset == 0 && len == 0)
    {
      /* Nothing to do.  */
      return;
    }

  if (offset == 0 && len == reg_size)
    {
      /* Collect the full register.  */
      return raw_collect (regnum, out);
    }

  /* Read to buffer, then write out.  */
  gdb_byte *reg = (gdb_byte *) alloca (reg_size);
  raw_collect (regnum, reg);
  memcpy (out, reg + offset, len);
}

/* See regcache.h.  */

enum register_status
regcache::write_part (int regnum, int offset, int len,
		      const gdb_byte *in, bool is_raw)
{
  int reg_size = register_size (arch (), regnum);

  gdb_assert (in != NULL);
  gdb_assert (offset >= 0 && offset <= reg_size);
  gdb_assert (len >= 0 && offset + len <= reg_size);

  if (offset == 0 && len == 0)
    {
      /* Nothing to do.  */
      return REG_VALID;
    }

  if (offset == 0 && len == reg_size)
    {
      /* Write the full register.  */
      (is_raw) ? raw_write (regnum, in) : cooked_write (regnum, in);
      return REG_VALID;
    }

  enum register_status status;
  gdb_byte *reg = (gdb_byte *) alloca (reg_size);

  /* Read existing register to buffer.  */
  status = (is_raw) ? raw_read (regnum, reg) : cooked_read (regnum, reg);
  if (status != REG_VALID)
    return status;

  /* Update buffer, then write back to regcache.  */
  memcpy (reg + offset, in, len);
  is_raw ? raw_write (regnum, reg) : cooked_write (regnum, reg);
  return REG_VALID;
}

/* See regcache.h.  */

void
reg_buffer::raw_supply_part (int regnum, int offset, int len,
			     const gdb_byte *in)
{
  int reg_size = register_size (arch (), regnum);

  gdb_assert (in != nullptr);
  gdb_assert (offset >= 0 && offset <= reg_size);
  gdb_assert (len >= 0 && offset + len <= reg_size);

  if (offset == 0 && len == 0)
    {
      /* Nothing to do.  */
      return;
    }

  if (offset == 0 && len == reg_size)
    {
      /* Supply the full register.  */
      return raw_supply (regnum, in);
    }

  gdb_byte *reg = (gdb_byte *) alloca (reg_size);

  /* Read existing value to buffer.  */
  raw_collect (regnum, reg);

  /* Write to buffer, then write out.  */
  memcpy (reg + offset, in, len);
  raw_supply (regnum, reg);
}

enum register_status
readable_regcache::raw_read_part (int regnum, int offset, int len,
				  gdb_byte *buf)
{
  assert_regnum (regnum);
  return read_part (regnum, offset, len, buf, true);
}

/* See regcache.h.  */

void
regcache::raw_write_part (int regnum, int offset, int len,
			  const gdb_byte *buf)
{
  assert_regnum (regnum);
  write_part (regnum, offset, len, buf, true);
}

/* See regcache.h.  */

enum register_status
readable_regcache::cooked_read_part (int regnum, int offset, int len,
				     gdb_byte *buf)
{
  gdb_assert (regnum >= 0 && regnum < m_descr->nr_cooked_registers);
  return read_part (regnum, offset, len, buf, false);
}

/* See regcache.h.  */

void
regcache::cooked_write_part (int regnum, int offset, int len,
			     const gdb_byte *buf)
{
  gdb_assert (regnum >= 0 && regnum < m_descr->nr_cooked_registers);
  write_part (regnum, offset, len, buf, false);
}

/* See common/common-regcache.h.  */

void
reg_buffer::raw_supply (int regnum, const void *buf)
{
  void *regbuf;
  size_t size;

  assert_regnum (regnum);

  regbuf = register_buffer (regnum);
  size = m_descr->sizeof_register[regnum];

  if (buf)
    {
      memcpy (regbuf, buf, size);
      m_register_status[regnum] = REG_VALID;
    }
  else
    {
      /* This memset not strictly necessary, but better than garbage
	 in case the register value manages to escape somewhere (due
	 to a bug, no less).  */
      memset (regbuf, 0, size);
      m_register_status[regnum] = REG_UNAVAILABLE;
    }
}

/* See regcache.h.  */

void
reg_buffer::raw_supply_integer (int regnum, const gdb_byte *addr,
				int addr_len, bool is_signed)
{
  enum bfd_endian byte_order = gdbarch_byte_order (m_descr->gdbarch);
  gdb_byte *regbuf;
  size_t regsize;

  assert_regnum (regnum);

  regbuf = register_buffer (regnum);
  regsize = m_descr->sizeof_register[regnum];

  copy_integer_to_size (regbuf, regsize, addr, addr_len, is_signed,
			byte_order);
  m_register_status[regnum] = REG_VALID;
}

/* See regcache.h.  */

void
reg_buffer::raw_supply_zeroed (int regnum)
{
  void *regbuf;
  size_t size;

  assert_regnum (regnum);

  regbuf = register_buffer (regnum);
  size = m_descr->sizeof_register[regnum];

  memset (regbuf, 0, size);
  m_register_status[regnum] = REG_VALID;
}

/* See common/common-regcache.h.  */

void
reg_buffer::raw_collect (int regnum, void *buf) const
{
  const void *regbuf;
  size_t size;

  gdb_assert (buf != NULL);
  assert_regnum (regnum);

  regbuf = register_buffer (regnum);
  size = m_descr->sizeof_register[regnum];
  memcpy (buf, regbuf, size);
}

/* See regcache.h.  */

void
reg_buffer::raw_collect_integer (int regnum, gdb_byte *addr, int addr_len,
				 bool is_signed) const
{
  enum bfd_endian byte_order = gdbarch_byte_order (m_descr->gdbarch);
  const gdb_byte *regbuf;
  size_t regsize;

  assert_regnum (regnum);

  regbuf = register_buffer (regnum);
  regsize = m_descr->sizeof_register[regnum];

  copy_integer_to_size (addr, addr_len, regbuf, regsize, is_signed,
			byte_order);
}

/* See regcache.h.  */

void
regcache::transfer_regset_register (struct regcache *out_regcache, int regnum,
				    const gdb_byte *in_buf, gdb_byte *out_buf,
				    int slot_size, int offs) const
{
  struct gdbarch *gdbarch = arch ();
  int reg_size = std::min (register_size (gdbarch, regnum), slot_size);

  /* Use part versions and reg_size to prevent possible buffer overflows when
     accessing the regcache.  */

  if (out_buf != nullptr)
    {
      raw_collect_part (regnum, 0, reg_size, out_buf + offs);

      /* Ensure any additional space is cleared.  */
      if (slot_size > reg_size)
	memset (out_buf + offs + reg_size, 0, slot_size - reg_size);
    }
  else if (in_buf != nullptr)
    out_regcache->raw_supply_part (regnum, 0, reg_size, in_buf + offs);
  else
    {
      /* Invalidate the register.  */
      out_regcache->raw_supply (regnum, nullptr);
    }
}

/* See regcache.h.  */

void
regcache::transfer_regset (const struct regset *regset,
			   struct regcache *out_regcache,
			   int regnum, const gdb_byte *in_buf,
			   gdb_byte *out_buf, size_t size) const
{
  const struct regcache_map_entry *map;
  int offs = 0, count;

  for (map = (const struct regcache_map_entry *) regset->regmap;
       (count = map->count) != 0;
       map++)
    {
      int regno = map->regno;
      int slot_size = map->size;

      if (slot_size == 0 && regno != REGCACHE_MAP_SKIP)
	slot_size = m_descr->sizeof_register[regno];

      if (regno == REGCACHE_MAP_SKIP
	  || (regnum != -1
	      && (regnum < regno || regnum >= regno + count)))
	  offs += count * slot_size;

      else if (regnum == -1)
	for (; count--; regno++, offs += slot_size)
	  {
	    if (offs + slot_size > size)
	      break;

	    transfer_regset_register (out_regcache, regno, in_buf, out_buf,
				      slot_size, offs);
	  }
      else
	{
	  /* Transfer a single register and return.  */
	  offs += (regnum - regno) * slot_size;
	  if (offs + slot_size > size)
	    return;

	  transfer_regset_register (out_regcache, regnum, in_buf, out_buf,
				    slot_size, offs);
	  return;
	}
    }
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the register(s) to "unavailable" status. */

void
regcache_supply_regset (const struct regset *regset,
			struct regcache *regcache,
			int regnum, const void *buf, size_t size)
{
  regcache->supply_regset (regset, regnum, (const gdb_byte *) buf, size);
}

void
regcache::supply_regset (const struct regset *regset,
			 int regnum, const void *buf, size_t size)
{
  transfer_regset (regset, this, regnum, (const gdb_byte *) buf, nullptr, size);
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

void
regcache_collect_regset (const struct regset *regset,
			 const struct regcache *regcache,
			 int regnum, void *buf, size_t size)
{
  regcache->collect_regset (regset, regnum, (gdb_byte *) buf, size);
}

void
regcache::collect_regset (const struct regset *regset,
			 int regnum, void *buf, size_t size) const
{
  transfer_regset (regset, nullptr, regnum, nullptr, (gdb_byte *) buf, size);
}

/* See common/common-regcache.h.  */

bool
reg_buffer::raw_compare (int regnum, const void *buf, int offset) const
{
  gdb_assert (buf != NULL);
  assert_regnum (regnum);

  const char *regbuf = (const char *) register_buffer (regnum);
  size_t size = m_descr->sizeof_register[regnum];
  gdb_assert (size >= offset);

  return (memcmp (buf, regbuf + offset, size - offset) == 0);
}

/* Special handling for register PC.  */

CORE_ADDR
regcache_read_pc (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();

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
  struct gdbarch *gdbarch = regcache->arch ();

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

int
reg_buffer::num_raw_registers () const
{
  return gdbarch_num_regs (arch ());
}

void
regcache::debug_print_register (const char *func,  int regno)
{
  struct gdbarch *gdbarch = arch ();

  fprintf_unfiltered (gdb_stdlog, "%s ", func);
  if (regno >= 0 && regno < gdbarch_num_regs (gdbarch)
      && gdbarch_register_name (gdbarch, regno) != NULL
      && gdbarch_register_name (gdbarch, regno)[0] != '\0')
    fprintf_unfiltered (gdb_stdlog, "(%s)",
			gdbarch_register_name (gdbarch, regno));
  else
    fprintf_unfiltered (gdb_stdlog, "(%d)", regno);
  if (regno >= 0 && regno < gdbarch_num_regs (gdbarch))
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int size = register_size (gdbarch, regno);
      gdb_byte *buf = register_buffer (regno);

      fprintf_unfiltered (gdb_stdlog, " = ");
      for (int i = 0; i < size; i++)
	{
	  fprintf_unfiltered (gdb_stdlog, "%02x", buf[i]);
	}
      if (size <= sizeof (LONGEST))
	{
	  ULONGEST val = extract_unsigned_integer (buf, size, byte_order);

	  fprintf_unfiltered (gdb_stdlog, " %s %s",
			      core_addr_to_string_nz (val), plongest (val));
	}
    }
  fprintf_unfiltered (gdb_stdlog, "\n");
}

static void
reg_flush_command (const char *command, int from_tty)
{
  /* Force-flush the register cache.  */
  registers_changed ();
  if (from_tty)
    printf_filtered (_("Register cache flushed.\n"));
}

void
register_dump::dump (ui_file *file)
{
  auto descr = regcache_descr (m_gdbarch);
  int regnum;
  int footnote_nr = 0;
  int footnote_register_offset = 0;
  int footnote_register_type_name_null = 0;
  long register_offset = 0;

  gdb_assert (descr->nr_cooked_registers
	      == gdbarch_num_cooked_regs (m_gdbarch));

  for (regnum = -1; regnum < descr->nr_cooked_registers; regnum++)
    {
      /* Name.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %-10s", "Name");
      else
	{
	  const char *p = gdbarch_register_name (m_gdbarch, regnum);

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
      else if (regnum < gdbarch_num_regs (m_gdbarch))
	fprintf_unfiltered (file, " %4d", regnum);
      else
	fprintf_unfiltered (file, " %4d",
			    (regnum - gdbarch_num_regs (m_gdbarch)));

      /* Offset.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %6s  ", "Offset");
      else
	{
	  fprintf_unfiltered (file, " %6ld",
			      descr->register_offset[regnum]);
	  if (register_offset != descr->register_offset[regnum]
	      || (regnum > 0
		  && (descr->register_offset[regnum]
		      != (descr->register_offset[regnum - 1]
			  + descr->sizeof_register[regnum - 1])))
	      )
	    {
	      if (!footnote_register_offset)
		footnote_register_offset = ++footnote_nr;
	      fprintf_unfiltered (file, "*%d", footnote_register_offset);
	    }
	  else
	    fprintf_unfiltered (file, "  ");
	  register_offset = (descr->register_offset[regnum]
			     + descr->sizeof_register[regnum]);
	}

      /* Size.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %5s ", "Size");
      else
	fprintf_unfiltered (file, " %5ld", descr->sizeof_register[regnum]);

      /* Type.  */
      {
	const char *t;
	std::string name_holder;

	if (regnum < 0)
	  t = "Type";
	else
	  {
	    static const char blt[] = "builtin_type";

	    t = TYPE_NAME (register_type (m_gdbarch, regnum));
	    if (t == NULL)
	      {
		if (!footnote_register_type_name_null)
		  footnote_register_type_name_null = ++footnote_nr;
		name_holder = string_printf ("*%d",
					     footnote_register_type_name_null);
		t = name_holder.c_str ();
	      }
	    /* Chop a leading builtin_type.  */
	    if (startswith (t, blt))
	      t += strlen (blt);
	  }
	fprintf_unfiltered (file, " %-15s", t);
      }

      /* Leading space always present.  */
      fprintf_unfiltered (file, " ");

      dump_reg (file, regnum);

      fprintf_unfiltered (file, "\n");
    }

  if (footnote_register_offset)
    fprintf_unfiltered (file, "*%d: Inconsistent register offsets.\n",
			footnote_register_offset);
  if (footnote_register_type_name_null)
    fprintf_unfiltered (file,
			"*%d: Register type's name NULL.\n",
			footnote_register_type_name_null);
}

#if GDB_SELF_TEST
#include "common/selftest.h"
#include "selftest-arch.h"
#include "target-float.h"

namespace selftests {

class regcache_access : public regcache
{
public:

  /* Return the number of elements in current_regcache.  */

  static size_t
  current_regcache_size ()
  {
    return std::distance (regcache::current_regcache.begin (),
			  regcache::current_regcache.end ());
  }
};

static void
current_regcache_test (void)
{
  /* It is empty at the start.  */
  SELF_CHECK (regcache_access::current_regcache_size () == 0);

  ptid_t ptid1 (1), ptid2 (2), ptid3 (3);

  /* Get regcache from ptid1, a new regcache is added to
     current_regcache.  */
  regcache *regcache = get_thread_arch_aspace_regcache (ptid1,
							target_gdbarch (),
							NULL);

  SELF_CHECK (regcache != NULL);
  SELF_CHECK (regcache->ptid () == ptid1);
  SELF_CHECK (regcache_access::current_regcache_size () == 1);

  /* Get regcache from ptid2, a new regcache is added to
     current_regcache.  */
  regcache = get_thread_arch_aspace_regcache (ptid2,
					      target_gdbarch (),
					      NULL);
  SELF_CHECK (regcache != NULL);
  SELF_CHECK (regcache->ptid () == ptid2);
  SELF_CHECK (regcache_access::current_regcache_size () == 2);

  /* Get regcache from ptid3, a new regcache is added to
     current_regcache.  */
  regcache = get_thread_arch_aspace_regcache (ptid3,
					      target_gdbarch (),
					      NULL);
  SELF_CHECK (regcache != NULL);
  SELF_CHECK (regcache->ptid () == ptid3);
  SELF_CHECK (regcache_access::current_regcache_size () == 3);

  /* Get regcache from ptid2 again, nothing is added to
     current_regcache.  */
  regcache = get_thread_arch_aspace_regcache (ptid2,
					      target_gdbarch (),
					      NULL);
  SELF_CHECK (regcache != NULL);
  SELF_CHECK (regcache->ptid () == ptid2);
  SELF_CHECK (regcache_access::current_regcache_size () == 3);

  /* Mark ptid2 is changed, so regcache of ptid2 should be removed from
     current_regcache.  */
  registers_changed_ptid (ptid2);
  SELF_CHECK (regcache_access::current_regcache_size () == 2);
}

class target_ops_no_register : public test_target_ops
{
public:
  target_ops_no_register ()
    : test_target_ops {}
  {}

  void reset ()
  {
    fetch_registers_called = 0;
    store_registers_called = 0;
    xfer_partial_called = 0;
  }

  void fetch_registers (regcache *regs, int regno) override;
  void store_registers (regcache *regs, int regno) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex, gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  unsigned int fetch_registers_called = 0;
  unsigned int store_registers_called = 0;
  unsigned int xfer_partial_called = 0;
};

void
target_ops_no_register::fetch_registers (regcache *regs, int regno)
{
  /* Mark register available.  */
  regs->raw_supply_zeroed (regno);
  this->fetch_registers_called++;
}

void
target_ops_no_register::store_registers (regcache *regs, int regno)
{
  this->store_registers_called++;
}

enum target_xfer_status
target_ops_no_register::xfer_partial (enum target_object object,
				      const char *annex, gdb_byte *readbuf,
				      const gdb_byte *writebuf,
				      ULONGEST offset, ULONGEST len,
				      ULONGEST *xfered_len)
{
  this->xfer_partial_called++;

  *xfered_len = len;
  return TARGET_XFER_OK;
}

class readwrite_regcache : public regcache
{
public:
  readwrite_regcache (struct gdbarch *gdbarch)
    : regcache (gdbarch, nullptr)
  {}
};

/* Test regcache::cooked_read gets registers from raw registers and
   memory instead of target to_{fetch,store}_registers.  */

static void
cooked_read_test (struct gdbarch *gdbarch)
{
  /* Error out if debugging something, because we're going to push the
     test target, which would pop any existing target.  */
  if (current_top_target ()->stratum () >= process_stratum)
    error (_("target already pushed"));

  /* Create a mock environment.  An inferior with a thread, with a
     process_stratum target pushed.  */

  target_ops_no_register mock_target;
  ptid_t mock_ptid (1, 1);
  inferior mock_inferior (mock_ptid.pid ());
  address_space mock_aspace {};
  mock_inferior.gdbarch = gdbarch;
  mock_inferior.aspace = &mock_aspace;
  thread_info mock_thread (&mock_inferior, mock_ptid);

  /* Add the mock inferior to the inferior list so that look ups by
     target+ptid can find it.  */
  scoped_restore restore_inferior_list
    = make_scoped_restore (&inferior_list);
  inferior_list = &mock_inferior;

  /* Switch to the mock inferior.  */
  scoped_restore_current_inferior restore_current_inferior;
  set_current_inferior (&mock_inferior);

  /* Push the process_stratum target so we can mock accessing
     registers.  */
  push_target (&mock_target);

  /* Pop it again on exit (return/exception).  */
  struct on_exit
  {
    ~on_exit ()
    {
      pop_all_targets_at_and_above (process_stratum);
    }
  } pop_targets;

  /* Switch to the mock thread.  */
  scoped_restore restore_inferior_ptid
    = make_scoped_restore (&inferior_ptid, mock_ptid);

  /* Test that read one raw register from regcache_no_target will go
     to the target layer.  */

  /* Find a raw register which size isn't zero.  */
  int nonzero_regnum;
  for (nonzero_regnum = 0;
       nonzero_regnum < gdbarch_num_regs (gdbarch);
       nonzero_regnum++)
    {
      if (register_size (gdbarch, nonzero_regnum) != 0)
	break;
    }

  readwrite_regcache readwrite (gdbarch);
  gdb::def_vector<gdb_byte> buf (register_size (gdbarch, nonzero_regnum));

  readwrite.raw_read (nonzero_regnum, buf.data ());

  /* raw_read calls target_fetch_registers.  */
  SELF_CHECK (mock_target.fetch_registers_called > 0);
  mock_target.reset ();

  /* Mark all raw registers valid, so the following raw registers
     accesses won't go to target.  */
  for (auto i = 0; i < gdbarch_num_regs (gdbarch); i++)
    readwrite.raw_update (i);

  mock_target.reset ();
  /* Then, read all raw and pseudo registers, and don't expect calling
     to_{fetch,store}_registers.  */
  for (int regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      if (register_size (gdbarch, regnum) == 0)
	continue;

      gdb::def_vector<gdb_byte> inner_buf (register_size (gdbarch, regnum));

      SELF_CHECK (REG_VALID == readwrite.cooked_read (regnum,
						      inner_buf.data ()));

      SELF_CHECK (mock_target.fetch_registers_called == 0);
      SELF_CHECK (mock_target.store_registers_called == 0);

      /* Some SPU pseudo registers are got via TARGET_OBJECT_SPU.  */
      if (gdbarch_bfd_arch_info (gdbarch)->arch != bfd_arch_spu)
	SELF_CHECK (mock_target.xfer_partial_called == 0);

      mock_target.reset ();
    }

  readonly_detached_regcache readonly (readwrite);

  /* GDB may go to target layer to fetch all registers and memory for
     readonly regcache.  */
  mock_target.reset ();

  for (int regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      if (register_size (gdbarch, regnum) == 0)
	continue;

      gdb::def_vector<gdb_byte> inner_buf (register_size (gdbarch, regnum));
      enum register_status status = readonly.cooked_read (regnum,
							  inner_buf.data ());

      if (regnum < gdbarch_num_regs (gdbarch))
	{
	  auto bfd_arch = gdbarch_bfd_arch_info (gdbarch)->arch;

	  if (bfd_arch == bfd_arch_frv || bfd_arch == bfd_arch_h8300
	      || bfd_arch == bfd_arch_m32c || bfd_arch == bfd_arch_sh
	      || bfd_arch == bfd_arch_alpha || bfd_arch == bfd_arch_v850
	      || bfd_arch == bfd_arch_msp430 || bfd_arch == bfd_arch_mep
	      || bfd_arch == bfd_arch_mips || bfd_arch == bfd_arch_v850_rh850
	      || bfd_arch == bfd_arch_tic6x || bfd_arch == bfd_arch_mn10300
	      || bfd_arch == bfd_arch_rl78 || bfd_arch == bfd_arch_score
	      || bfd_arch == bfd_arch_riscv || bfd_arch == bfd_arch_csky)
	    {
	      /* Raw registers.  If raw registers are not in save_reggroup,
		 their status are unknown.  */
	      if (gdbarch_register_reggroup_p (gdbarch, regnum, save_reggroup))
		SELF_CHECK (status == REG_VALID);
	      else
		SELF_CHECK (status == REG_UNKNOWN);
	    }
	  else
	    SELF_CHECK (status == REG_VALID);
	}
      else
	{
	  if (gdbarch_register_reggroup_p (gdbarch, regnum, save_reggroup))
	    SELF_CHECK (status == REG_VALID);
	  else
	    {
	      /* If pseudo registers are not in save_reggroup, some of
		 them can be computed from saved raw registers, but some
		 of them are unknown.  */
	      auto bfd_arch = gdbarch_bfd_arch_info (gdbarch)->arch;

	      if (bfd_arch == bfd_arch_frv
		  || bfd_arch == bfd_arch_m32c
		  || bfd_arch == bfd_arch_mep
		  || bfd_arch == bfd_arch_sh)
		SELF_CHECK (status == REG_VALID || status == REG_UNKNOWN);
	      else if (bfd_arch == bfd_arch_mips
		       || bfd_arch == bfd_arch_h8300)
		SELF_CHECK (status == REG_UNKNOWN);
	      else
		SELF_CHECK (status == REG_VALID);
	    }
	}

      SELF_CHECK (mock_target.fetch_registers_called == 0);
      SELF_CHECK (mock_target.store_registers_called == 0);
      SELF_CHECK (mock_target.xfer_partial_called == 0);

      mock_target.reset ();
    }
}

/* Test regcache::cooked_write by writing some expected contents to
   registers, and checking that contents read from registers and the
   expected contents are the same.  */

static void
cooked_write_test (struct gdbarch *gdbarch)
{
  /* Error out if debugging something, because we're going to push the
     test target, which would pop any existing target.  */
  if (current_top_target ()->stratum () >= process_stratum)
    error (_("target already pushed"));

  /* Create a mock environment.  A process_stratum target pushed.  */

  target_ops_no_register mock_target;

  /* Push the process_stratum target so we can mock accessing
     registers.  */
  push_target (&mock_target);

  /* Pop it again on exit (return/exception).  */
  struct on_exit
  {
    ~on_exit ()
    {
      pop_all_targets_at_and_above (process_stratum);
    }
  } pop_targets;

  readwrite_regcache readwrite (gdbarch);

  const int num_regs = gdbarch_num_cooked_regs (gdbarch);

  for (auto regnum = 0; regnum < num_regs; regnum++)
    {
      if (register_size (gdbarch, regnum) == 0
	  || gdbarch_cannot_store_register (gdbarch, regnum))
	continue;

      auto bfd_arch = gdbarch_bfd_arch_info (gdbarch)->arch;

      if ((bfd_arch == bfd_arch_sparc
	   /* SPARC64_CWP_REGNUM, SPARC64_PSTATE_REGNUM,
	      SPARC64_ASI_REGNUM and SPARC64_CCR_REGNUM are hard to test.  */
	   && gdbarch_ptr_bit (gdbarch) == 64
	   && (regnum >= gdbarch_num_regs (gdbarch)
	       && regnum <= gdbarch_num_regs (gdbarch) + 4))
	  || (bfd_arch == bfd_arch_spu
	      /* SPU pseudo registers except SPU_SP_REGNUM are got by
		 TARGET_OBJECT_SPU.  */
	      && regnum >= gdbarch_num_regs (gdbarch) && regnum != 130))
	continue;

      std::vector<gdb_byte> expected (register_size (gdbarch, regnum), 0);
      std::vector<gdb_byte> buf (register_size (gdbarch, regnum), 0);
      const auto type = register_type (gdbarch, regnum);

      if (TYPE_CODE (type) == TYPE_CODE_FLT
	  || TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
	{
	  /* Generate valid float format.  */
	  target_float_from_string (expected.data (), type, "1.25");
	}
      else if (TYPE_CODE (type) == TYPE_CODE_INT
	       || TYPE_CODE (type) == TYPE_CODE_ARRAY
	       || TYPE_CODE (type) == TYPE_CODE_PTR
	       || TYPE_CODE (type) == TYPE_CODE_UNION
	       || TYPE_CODE (type) == TYPE_CODE_STRUCT)
	{
	  if (bfd_arch == bfd_arch_ia64
	      || (regnum >= gdbarch_num_regs (gdbarch)
		  && (bfd_arch == bfd_arch_xtensa
		      || bfd_arch == bfd_arch_bfin
		      || bfd_arch == bfd_arch_m32c
		      /* m68hc11 pseudo registers are in memory.  */
		      || bfd_arch == bfd_arch_m68hc11
		      || bfd_arch == bfd_arch_m68hc12
		      || bfd_arch == bfd_arch_s390))
	      || (bfd_arch == bfd_arch_frv
		  /* FRV pseudo registers except iacc0.  */
		  && regnum > gdbarch_num_regs (gdbarch)))
	    {
	      /* Skip setting the expected values for some architecture
		 registers.  */
	    }
	  else if (bfd_arch == bfd_arch_rl78 && regnum == 40)
	    {
	      /* RL78_PC_REGNUM */
	      for (auto j = 0; j < register_size (gdbarch, regnum) - 1; j++)
		expected[j] = j;
	    }
	  else
	    {
	      for (auto j = 0; j < register_size (gdbarch, regnum); j++)
		expected[j] = j;
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_FLAGS)
	{
	  /* No idea how to test flags.  */
	  continue;
	}
      else
	{
	  /* If we don't know how to create the expected value for the
	     this type, make it fail.  */
	  SELF_CHECK (0);
	}

      readwrite.cooked_write (regnum, expected.data ());

      SELF_CHECK (readwrite.cooked_read (regnum, buf.data ()) == REG_VALID);
      SELF_CHECK (expected == buf);
    }
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void
_initialize_regcache (void)
{
  regcache_descr_handle
    = gdbarch_data_register_post_init (init_regcache_descr);

  gdb::observers::target_changed.attach (regcache_observer_target_changed);
  gdb::observers::thread_ptid_changed.attach
    (regcache::regcache_thread_ptid_changed);

  add_com ("flushregs", class_maintenance, reg_flush_command,
	   _("Force gdb to flush its register cache (maintainer command)"));

#if GDB_SELF_TEST
  selftests::register_test ("current_regcache", selftests::current_regcache_test);

  selftests::register_test_foreach_arch ("regcache::cooked_read_test",
					 selftests::cooked_read_test);
  selftests::register_test_foreach_arch ("regcache::cooked_write_test",
					 selftests::cooked_write_test);
#endif
}
