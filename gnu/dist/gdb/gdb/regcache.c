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

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "gdb_assert.h"
#include "gdb_string.h"
#include "gdbcmd.h"		/* For maintenanceprintlist.  */

/*
 * DATA STRUCTURE
 *
 * Here is the actual register cache.
 */

/* Per-architecture object describing the layout of a register cache.
   Computed once when the architecture is created */

struct gdbarch_data *regcache_descr_handle;

struct regcache_descr
{
  /* The architecture this descriptor belongs to.  */
  struct gdbarch *gdbarch;

  /* Is this a ``legacy'' register cache?  Such caches reserve space
     for raw and pseudo registers and allow access to both.  */
  int legacy_p;

  /* The raw register cache.  This should contain just [0
     .. NUM_RAW_REGISTERS).  However, for older targets, it contains
     space for the full [0 .. NUM_RAW_REGISTERS +
     NUM_PSEUDO_REGISTERS).  */
  int nr_raw_registers;
  long sizeof_raw_registers;
  long sizeof_raw_register_valid_p;

  /* The cooked register space.  Each cooked register in the range
     [0..NR_RAW_REGISTERS) is direct-mapped onto the corresponding raw
     register.  The remaining [NR_RAW_REGISTERS
     .. NR_COOKED_REGISTERS) (a.k.a. pseudo regiters) are mapped onto
     both raw registers and memory by the architecture methods
     gdbarch_register_read and gdbarch_register_write.  */
  int nr_cooked_registers;

  /* Offset and size (in 8 bit bytes), of reach register in the
     register cache.  All registers (including those in the range
     [NR_RAW_REGISTERS .. NR_COOKED_REGISTERS) are given an offset.
     Assigning all registers an offset makes it possible to keep
     legacy code, such as that found in read_register_bytes() and
     write_register_bytes() working.  */
  long *register_offset;
  long *sizeof_register;

  /* Useful constant.  Largest of all the registers.  */
  long max_register_size;

  /* Cached table containing the type of each register.  */
  struct type **register_type;
};

void
init_legacy_regcache_descr (struct gdbarch *gdbarch,
			    struct regcache_descr *descr)
{
  int i;
  /* FIXME: cagney/2002-05-11: gdbarch_data() should take that
     ``gdbarch'' as a parameter.  */
  gdb_assert (gdbarch != NULL);

  /* FIXME: cagney/2002-05-11: Shouldn't be including pseudo-registers
     in the register buffer.  Unfortunatly some architectures do.  */
  descr->nr_raw_registers = descr->nr_cooked_registers;
  descr->sizeof_raw_register_valid_p = descr->nr_cooked_registers;

  /* FIXME: cagney/2002-05-11: Instead of using REGISTER_BYTE() this
     code should compute the offets et.al. at runtime.  This currently
     isn't possible because some targets overlap register locations -
     see the mess in read_register_bytes() and write_register_bytes()
     registers.  */
  descr->sizeof_register = XCALLOC (descr->nr_cooked_registers, long);
  descr->register_offset = XCALLOC (descr->nr_cooked_registers, long);
  descr->max_register_size = 0;
  for (i = 0; i < descr->nr_cooked_registers; i++)
    {
      descr->register_offset[i] = REGISTER_BYTE (i);
      descr->sizeof_register[i] = REGISTER_RAW_SIZE (i);
      if (descr->max_register_size < REGISTER_RAW_SIZE (i))
	descr->max_register_size = REGISTER_RAW_SIZE (i);
      if (descr->max_register_size < REGISTER_VIRTUAL_SIZE (i))
	descr->max_register_size = REGISTER_VIRTUAL_SIZE (i);
    }

  /* Come up with the real size of the registers buffer.  */
  descr->sizeof_raw_registers = REGISTER_BYTES; /* OK use.  */
  for (i = 0; i < descr->nr_cooked_registers; i++)
    {
      long regend;
      /* Keep extending the buffer so that there is always enough
         space for all registers.  The comparison is necessary since
         legacy code is free to put registers in random places in the
         buffer separated by holes.  Once REGISTER_BYTE() is killed
         this can be greatly simplified.  */
      /* FIXME: cagney/2001-12-04: This code shouldn't need to use
         REGISTER_BYTE().  Unfortunatly, legacy code likes to lay the
         buffer out so that certain registers just happen to overlap.
         Ulgh!  New targets use gdbarch's register read/write and
         entirely avoid this uglyness.  */
      regend = descr->register_offset[i] + descr->sizeof_register[i];
      if (descr->sizeof_raw_registers < regend)
	descr->sizeof_raw_registers = regend;
    }
}

static void *
init_regcache_descr (struct gdbarch *gdbarch)
{
  int i;
  struct regcache_descr *descr;
  gdb_assert (gdbarch != NULL);

  /* Create an initial, zero filled, table.  */
  descr = XCALLOC (1, struct regcache_descr);
  descr->gdbarch = gdbarch;

  /* Total size of the register space.  The raw registers are mapped
     directly onto the raw register cache while the pseudo's are
     either mapped onto raw-registers or memory.  */
  descr->nr_cooked_registers = NUM_REGS + NUM_PSEUDO_REGS;

  /* Fill in a table of register types.  */
  descr->register_type = XCALLOC (descr->nr_cooked_registers,
				  struct type *);
  for (i = 0; i < descr->nr_cooked_registers; i++)
    {
      descr->register_type[i] = REGISTER_VIRTUAL_TYPE (i);
    }

  /* If an old style architecture, fill in the remainder of the
     register cache descriptor using the register macros.  */
  if (!gdbarch_pseudo_register_read_p (gdbarch)
      && !gdbarch_pseudo_register_write_p (gdbarch))
    {
      descr->legacy_p = 1;
      init_legacy_regcache_descr (gdbarch, descr);
      return descr;
    }

  /* Construct a strictly RAW register cache.  Don't allow pseudo's
     into the register cache.  */
  descr->nr_raw_registers = NUM_REGS;

  /* FIXME: cagney/2002-08-13: Overallocate the register_valid_p
     array.  This pretects GDB from erant code that accesses elements
     of the global register_valid_p[] array in the range [NUM_REGS
     .. NUM_REGS + NUM_PSEUDO_REGS).  */
  descr->sizeof_raw_register_valid_p = NUM_REGS + NUM_PSEUDO_REGS;

  /* Lay out the register cache.  The pseud-registers are included in
     the layout even though their value isn't stored in the register
     cache.  Some code, via read_register_bytes() access a register
     using an offset/length rather than a register number.

     NOTE: cagney/2002-05-22: Only register_type() is used when
     constructing the register cache.  It is assumed that the
     register's raw size, virtual size and type length are all the
     same.  */

  {
    long offset = 0;
    descr->sizeof_register = XCALLOC (descr->nr_cooked_registers, long);
    descr->register_offset = XCALLOC (descr->nr_cooked_registers, long);
    descr->max_register_size = 0;
    for (i = 0; i < descr->nr_cooked_registers; i++)
      {
	descr->sizeof_register[i] = TYPE_LENGTH (descr->register_type[i]);
	descr->register_offset[i] = offset;
	offset += descr->sizeof_register[i];
	if (descr->max_register_size < descr->sizeof_register[i])
	  descr->max_register_size = descr->sizeof_register[i];
      }
    /* Set the real size of the register cache buffer.  */
    /* FIXME: cagney/2002-05-22: Should only need to allocate space
       for the raw registers.  Unfortunatly some code still accesses
       the register array directly using the global registers[].
       Until that code has been purged, play safe and over allocating
       the register buffer.  Ulgh!  */
    descr->sizeof_raw_registers = offset;
    /* = descr->register_offset[descr->nr_raw_registers]; */
  }

#if 0
  /* Sanity check.  Confirm that the assumptions about gdbarch are
     true.  The REGCACHE_DESCR_HANDLE is set before doing the checks
     so that targets using the generic methods supplied by regcache
     don't go into infinite recursion trying to, again, create the
     regcache.  */
  set_gdbarch_data (gdbarch, regcache_descr_handle, descr);
  for (i = 0; i < descr->nr_cooked_registers; i++)
    {
      gdb_assert (descr->sizeof_register[i] == REGISTER_RAW_SIZE (i));
      gdb_assert (descr->sizeof_register[i] == REGISTER_VIRTUAL_SIZE (i));
      gdb_assert (descr->register_offset[i] == REGISTER_BYTE (i));
    }
  /* gdb_assert (descr->sizeof_raw_registers == REGISTER_BYTES (i));  */
#endif

  return descr;
}

static struct regcache_descr *
regcache_descr (struct gdbarch *gdbarch)
{
  return gdbarch_data (gdbarch, regcache_descr_handle);
}

static void
xfree_regcache_descr (struct gdbarch *gdbarch, void *ptr)
{
  struct regcache_descr *descr = ptr;
  if (descr == NULL)
    return;
  xfree (descr->register_offset);
  xfree (descr->sizeof_register);
  descr->register_offset = NULL;
  descr->sizeof_register = NULL;
  xfree (descr);
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
max_register_size (struct gdbarch *gdbarch)
{
  struct regcache_descr *descr = regcache_descr (gdbarch);
  return descr->max_register_size;
}

/* The register cache for storing raw register values.  */

struct regcache
{
  struct regcache_descr *descr;
  char *raw_registers;
  char *raw_register_valid_p;
  /* If a value isn't in the cache should the corresponding target be
     queried for a value.  */
  int passthrough_p;
};

struct regcache *
regcache_xmalloc (struct gdbarch *gdbarch)
{
  struct regcache_descr *descr;
  struct regcache *regcache;
  gdb_assert (gdbarch != NULL);
  descr = regcache_descr (gdbarch);
  regcache = XMALLOC (struct regcache);
  regcache->descr = descr;
  regcache->raw_registers
    = XCALLOC (descr->sizeof_raw_registers, char);
  regcache->raw_register_valid_p
    = XCALLOC (descr->sizeof_raw_register_valid_p, char);
  regcache->passthrough_p = 0;
  return regcache;
}

void
regcache_xfree (struct regcache *regcache)
{
  if (regcache == NULL)
    return;
  xfree (regcache->raw_registers);
  xfree (regcache->raw_register_valid_p);
  xfree (regcache);
}

void
do_regcache_xfree (void *data)
{
  regcache_xfree (data);
}

struct cleanup *
make_cleanup_regcache_xfree (struct regcache *regcache)
{
  return make_cleanup (do_regcache_xfree, regcache);
}

void
regcache_cpy (struct regcache *dst, struct regcache *src)
{
  int i;
  char *buf;
  gdb_assert (src != NULL && dst != NULL);
  gdb_assert (src->descr->gdbarch == dst->descr->gdbarch);
  gdb_assert (src != dst);
  /* FIXME: cagney/2002-05-17: To say this bit is bad is being polite.
     It keeps the existing code working where things rely on going
     through to the register cache.  */
  if (src == current_regcache && src->descr->legacy_p)
    {
      /* ULGH!!!!  Old way.  Use REGISTER bytes and let code below
	 untangle fetch.  */
      read_register_bytes (0, dst->raw_registers, REGISTER_BYTES);
      return;
    }
  /* FIXME: cagney/2002-05-17: To say this bit is bad is being polite.
     It keeps the existing code working where things rely on going
     through to the register cache.  */
  if (dst == current_regcache && dst->descr->legacy_p)
    {
      /* ULGH!!!!  Old way.  Use REGISTER bytes and let code below
	 untangle fetch.  */
      write_register_bytes (0, src->raw_registers, REGISTER_BYTES);
      return;
    }
  buf = alloca (src->descr->max_register_size);
  for (i = 0; i < src->descr->nr_raw_registers; i++)
    {
      /* Should we worry about the valid bit here?  */
      regcache_raw_read (src, i, buf);
      regcache_raw_write (dst, i, buf);
    }
}

void
regcache_cpy_no_passthrough (struct regcache *dst, struct regcache *src)
{
  int i;
  gdb_assert (src != NULL && dst != NULL);
  gdb_assert (src->descr->gdbarch == dst->descr->gdbarch);
  /* NOTE: cagney/2002-05-17: Don't let the caller do a no-passthrough
     move of data into the current_regcache().  Doing this would be
     silly - it would mean that valid_p would be completly invalid.  */
  gdb_assert (dst != current_regcache);
  memcpy (dst->raw_registers, src->raw_registers,
	  dst->descr->sizeof_raw_registers);
  memcpy (dst->raw_register_valid_p, src->raw_register_valid_p,
	  dst->descr->sizeof_raw_register_valid_p);
}

struct regcache *
regcache_dup (struct regcache *src)
{
  struct regcache *newbuf;
  gdb_assert (current_regcache != NULL);
  newbuf = regcache_xmalloc (src->descr->gdbarch);
  regcache_cpy (newbuf, src);
  return newbuf;
}

struct regcache *
regcache_dup_no_passthrough (struct regcache *src)
{
  struct regcache *newbuf;
  gdb_assert (current_regcache != NULL);
  newbuf = regcache_xmalloc (src->descr->gdbarch);
  regcache_cpy_no_passthrough (newbuf, src);
  return newbuf;
}

int
regcache_valid_p (struct regcache *regcache, int regnum)
{
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  return regcache->raw_register_valid_p[regnum];
}

char *
deprecated_grub_regcache_for_registers (struct regcache *regcache)
{
  return regcache->raw_registers;
}

char *
deprecated_grub_regcache_for_register_valid (struct regcache *regcache)
{
  return regcache->raw_register_valid_p;
}

/* Global structure containing the current regcache.  */
/* FIXME: cagney/2002-05-11: The two global arrays registers[] and
   register_valid[] currently point into this structure.  */
struct regcache *current_regcache;

/* NOTE: this is a write-through cache.  There is no "dirty" bit for
   recording if the register values have been changed (eg. by the
   user).  Therefore all registers must be written back to the
   target when appropriate.  */

/* REGISTERS contains the cached register values (in target byte order). */

char *registers;

/* REGISTER_VALID is 0 if the register needs to be fetched,
                     1 if it has been fetched, and
		    -1 if the register value was not available.  

   "Not available" indicates that the target is not not able to supply
   the register at this state.  The register may become available at a
   later time (after the next resume).  This often occures when GDB is
   manipulating a target that contains only a snapshot of the entire
   system being debugged - some of the registers in such a system may
   not have been saved.  */

signed char *register_valid;

/* The thread/process associated with the current set of registers. */

static ptid_t registers_ptid;

/*
 * FUNCTIONS:
 */

/* REGISTER_CACHED()

   Returns 0 if the value is not in the cache (needs fetch).
          >0 if the value is in the cache.
	  <0 if the value is permanently unavailable (don't ask again).  */

int
register_cached (int regnum)
{
  return register_valid[regnum];
}

/* Record that REGNUM's value is cached if STATE is >0, uncached but
   fetchable if STATE is 0, and uncached and unfetchable if STATE is <0.  */

void
set_register_cached (int regnum, int state)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < current_regcache->descr->nr_raw_registers);
  current_regcache->raw_register_valid_p[regnum] = state;
}

/* REGISTER_CHANGED

   invalidate a single register REGNUM in the cache */
void
register_changed (int regnum)
{
  set_register_cached (regnum, 0);
}

/* If REGNUM >= 0, return a pointer to register REGNUM's cache buffer area,
   else return a pointer to the start of the cache buffer.  */

static char *
register_buffer (struct regcache *regcache, int regnum)
{
  return regcache->raw_registers + regcache->descr->register_offset[regnum];
}

/* Return whether register REGNUM is a real register.  */

static int
real_register (int regnum)
{
  return regnum >= 0 && regnum < NUM_REGS;
}

/* Low level examining and depositing of registers.

   The caller is responsible for making sure that the inferior is
   stopped before calling the fetching routines, or it will get
   garbage.  (a change from GDB version 3, in which the caller got the
   value from the last stop).  */

/* REGISTERS_CHANGED ()

   Indicate that registers may have changed, so invalidate the cache.  */

void
registers_changed (void)
{
  int i;

  registers_ptid = pid_to_ptid (-1);

  /* Force cleanup of any alloca areas if using C alloca instead of
     a builtin alloca.  This particular call is used to clean up
     areas allocated by low level target code which may build up
     during lengthy interactions between gdb and the target before
     gdb gives control to the user (ie watchpoints).  */
  alloca (0);

  for (i = 0; i < current_regcache->descr->nr_raw_registers; i++)
    set_register_cached (i, 0);

  if (registers_changed_hook)
    registers_changed_hook ();
}

/* REGISTERS_FETCHED ()

   Indicate that all registers have been fetched, so mark them all valid.  */

/* NOTE: cagney/2001-12-04: This function does not set valid on the
   pseudo-register range since pseudo registers are always supplied
   using supply_register().  */
/* FIXME: cagney/2001-12-04: This function is DEPRECATED.  The target
   code was blatting the registers[] array and then calling this.
   Since targets should only be using supply_register() the need for
   this function/hack is eliminated.  */

void
registers_fetched (void)
{
  int i;

  for (i = 0; i < NUM_REGS; i++)
    set_register_cached (i, 1);
  /* Do not assume that the pseudo-regs have also been fetched.
     Fetching all real regs NEVER accounts for pseudo-regs.  */
}

/* read_register_bytes and write_register_bytes are generally a *BAD*
   idea.  They are inefficient because they need to check for partial
   updates, which can only be done by scanning through all of the
   registers and seeing if the bytes that are being read/written fall
   inside of an invalid register.  [The main reason this is necessary
   is that register sizes can vary, so a simple index won't suffice.]
   It is far better to call read_register_gen and write_register_gen
   if you want to get at the raw register contents, as it only takes a
   regnum as an argument, and therefore can't do a partial register
   update.

   Prior to the recent fixes to check for partial updates, both read
   and write_register_bytes always checked to see if any registers
   were stale, and then called target_fetch_registers (-1) to update
   the whole set.  This caused really slowed things down for remote
   targets.  */

/* Copy INLEN bytes of consecutive data from registers
   starting with the INREGBYTE'th byte of register data
   into memory at MYADDR.  */

void
read_register_bytes (int in_start, char *in_buf, int in_len)
{
  int in_end = in_start + in_len;
  int regnum;
  char *reg_buf = alloca (MAX_REGISTER_RAW_SIZE);

  /* See if we are trying to read bytes from out-of-date registers.  If so,
     update just those registers.  */

  for (regnum = 0; regnum < NUM_REGS + NUM_PSEUDO_REGS; regnum++)
    {
      int reg_start;
      int reg_end;
      int reg_len;
      int start;
      int end;
      int byte;

      reg_start = REGISTER_BYTE (regnum);
      reg_len = REGISTER_RAW_SIZE (regnum);
      reg_end = reg_start + reg_len;

      if (reg_end <= in_start || in_end <= reg_start)
	/* The range the user wants to read doesn't overlap with regnum.  */
	continue;

      if (REGISTER_NAME (regnum) != NULL && *REGISTER_NAME (regnum) != '\0')
	/* Force the cache to fetch the entire register.  */
	read_register_gen (regnum, reg_buf);
      else
	/* Legacy note: even though this register is ``invalid'' we
           still need to return something.  It would appear that some
           code relies on apparent gaps in the register array also
           being returned.  */
	/* FIXME: cagney/2001-08-18: This is just silly.  It defeats
           the entire register read/write flow of control.  Must
           resist temptation to return 0xdeadbeef.  */
	memcpy (reg_buf, registers + reg_start, reg_len);

      /* Legacy note: This function, for some reason, allows a NULL
         input buffer.  If the buffer is NULL, the registers are still
         fetched, just the final transfer is skipped. */
      if (in_buf == NULL)
	continue;

      /* start = max (reg_start, in_start) */
      if (reg_start > in_start)
	start = reg_start;
      else
	start = in_start;

      /* end = min (reg_end, in_end) */
      if (reg_end < in_end)
	end = reg_end;
      else
	end = in_end;

      /* Transfer just the bytes common to both IN_BUF and REG_BUF */
      for (byte = start; byte < end; byte++)
	{
	  in_buf[byte - in_start] = reg_buf[byte - reg_start];
	}
    }
}

/* Read register REGNUM into memory at MYADDR, which must be large
   enough for REGISTER_RAW_BYTES (REGNUM).  Target byte-order.  If the
   register is known to be the size of a CORE_ADDR or smaller,
   read_register can be used instead.  */

static void
legacy_read_register_gen (int regnum, char *myaddr)
{
  gdb_assert (regnum >= 0 && regnum < (NUM_REGS + NUM_PSEUDO_REGS));
  if (! ptid_equal (registers_ptid, inferior_ptid))
    {
      registers_changed ();
      registers_ptid = inferior_ptid;
    }

  if (!register_cached (regnum))
    target_fetch_registers (regnum);

  memcpy (myaddr, register_buffer (current_regcache, regnum),
	  REGISTER_RAW_SIZE (regnum));
}

void
regcache_raw_read (struct regcache *regcache, int regnum, void *buf)
{
  gdb_assert (regcache != NULL && buf != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  if (regcache->descr->legacy_p
      && regcache->passthrough_p)
    {
      gdb_assert (regcache == current_regcache);
      /* For moment, just use underlying legacy code.  Ulgh!!! This
	 silently and very indirectly updates the regcache's regcache
	 via the global register_valid[].  */
      legacy_read_register_gen (regnum, buf);
      return;
    }
  /* Make certain that the register cache is up-to-date with respect
     to the current thread.  This switching shouldn't be necessary
     only there is still only one target side register cache.  Sigh!
     On the bright side, at least there is a regcache object.  */
  if (regcache->passthrough_p)
    {
      gdb_assert (regcache == current_regcache);
      if (! ptid_equal (registers_ptid, inferior_ptid))
	{
	  registers_changed ();
	  registers_ptid = inferior_ptid;
	}
      if (!register_cached (regnum))
	target_fetch_registers (regnum);
    }
  /* Copy the value directly into the register cache.  */
  memcpy (buf, (regcache->raw_registers
		+ regcache->descr->register_offset[regnum]),
	  regcache->descr->sizeof_register[regnum]);
}

void
regcache_raw_read_signed (struct regcache *regcache, int regnum, LONGEST *val)
{
  char *buf;
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  regcache_raw_read (regcache, regnum, buf);
  (*val) = extract_signed_integer (buf,
				   regcache->descr->sizeof_register[regnum]);
}

void
regcache_raw_read_unsigned (struct regcache *regcache, int regnum,
			    ULONGEST *val)
{
  char *buf;
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  regcache_raw_read (regcache, regnum, buf);
  (*val) = extract_unsigned_integer (buf,
				     regcache->descr->sizeof_register[regnum]);
}

void
regcache_raw_write_signed (struct regcache *regcache, int regnum, LONGEST val)
{
  void *buf;
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >=0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  store_signed_integer (buf, regcache->descr->sizeof_register[regnum], val);
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
  store_unsigned_integer (buf, regcache->descr->sizeof_register[regnum], val);
  regcache_raw_write (regcache, regnum, buf);
}

void
read_register_gen (int regnum, char *buf)
{
  gdb_assert (current_regcache != NULL);
  gdb_assert (current_regcache->descr->gdbarch == current_gdbarch);
  if (current_regcache->descr->legacy_p)
    {
      legacy_read_register_gen (regnum, buf);
      return;
    }
  regcache_cooked_read (current_regcache, regnum, buf);
}

void
regcache_cooked_read (struct regcache *regcache, int regnum, void *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < regcache->descr->nr_cooked_registers);
  if (regnum < regcache->descr->nr_raw_registers)
    regcache_raw_read (regcache, regnum, buf);
  else
    gdbarch_pseudo_register_read (regcache->descr->gdbarch, regcache,
				  regnum, buf);
}

void
regcache_cooked_read_signed (struct regcache *regcache, int regnum,
			     LONGEST *val)
{
  char *buf;
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  regcache_cooked_read (regcache, regnum, buf);
  (*val) = extract_signed_integer (buf,
				   regcache->descr->sizeof_register[regnum]);
}

void
regcache_cooked_read_unsigned (struct regcache *regcache, int regnum,
			       ULONGEST *val)
{
  char *buf;
  gdb_assert (regcache != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);
  buf = alloca (regcache->descr->sizeof_register[regnum]);
  regcache_cooked_read (regcache, regnum, buf);
  (*val) = extract_unsigned_integer (buf,
				     regcache->descr->sizeof_register[regnum]);
}

/* Write register REGNUM at MYADDR to the target.  MYADDR points at
   REGISTER_RAW_BYTES(REGNUM), which must be in target byte-order.  */

static void
legacy_write_register_gen (int regnum, const void *myaddr)
{
  int size;
  gdb_assert (regnum >= 0 && regnum < (NUM_REGS + NUM_PSEUDO_REGS));

  /* On the sparc, writing %g0 is a no-op, so we don't even want to
     change the registers array if something writes to this register.  */
  if (CANNOT_STORE_REGISTER (regnum))
    return;

  if (! ptid_equal (registers_ptid, inferior_ptid))
    {
      registers_changed ();
      registers_ptid = inferior_ptid;
    }

  size = REGISTER_RAW_SIZE (regnum);

  if (real_register (regnum))
    {
      /* If we have a valid copy of the register, and new value == old
	 value, then don't bother doing the actual store. */
      if (register_cached (regnum)
	  && (memcmp (register_buffer (current_regcache, regnum), myaddr, size)
	      == 0))
	return;
      else
	target_prepare_to_store ();
    }

  memcpy (register_buffer (current_regcache, regnum), myaddr, size);

  set_register_cached (regnum, 1);
  target_store_registers (regnum);
}

void
regcache_raw_write (struct regcache *regcache, int regnum, const void *buf)
{
  gdb_assert (regcache != NULL && buf != NULL);
  gdb_assert (regnum >= 0 && regnum < regcache->descr->nr_raw_registers);

  if (regcache->passthrough_p
      && regcache->descr->legacy_p)
    {
      /* For moment, just use underlying legacy code.  Ulgh!!! This
	 silently and very indirectly updates the regcache's buffers
	 via the globals register_valid[] and registers[].  */
      gdb_assert (regcache == current_regcache);
      legacy_write_register_gen (regnum, buf);
      return;
    }

  /* On the sparc, writing %g0 is a no-op, so we don't even want to
     change the registers array if something writes to this register.  */
  if (CANNOT_STORE_REGISTER (regnum))
    return;

  /* Handle the simple case first -> not write through so just store
     value in cache.  */
  if (!regcache->passthrough_p)
    {
      memcpy ((regcache->raw_registers
	       + regcache->descr->register_offset[regnum]), buf,
	      regcache->descr->sizeof_register[regnum]);
      regcache->raw_register_valid_p[regnum] = 1;
      return;
    }

  /* Make certain that the correct cache is selected.  */
  gdb_assert (regcache == current_regcache);
  if (! ptid_equal (registers_ptid, inferior_ptid))
    {
      registers_changed ();
      registers_ptid = inferior_ptid;
    }

  /* If we have a valid copy of the register, and new value == old
     value, then don't bother doing the actual store. */
  if (regcache_valid_p (regcache, regnum)
      && (memcmp (register_buffer (regcache, regnum), buf,
		  regcache->descr->sizeof_register[regnum]) == 0))
    return;

  target_prepare_to_store ();
  memcpy (register_buffer (regcache, regnum), buf,
	  regcache->descr->sizeof_register[regnum]);
  regcache->raw_register_valid_p[regnum] = 1;
  target_store_registers (regnum);
}

void
write_register_gen (int regnum, char *buf)
{
  gdb_assert (current_regcache != NULL);
  gdb_assert (current_regcache->descr->gdbarch == current_gdbarch);
  if (current_regcache->descr->legacy_p)
    {
      legacy_write_register_gen (regnum, buf);
      return;
    }
  regcache_cooked_write (current_regcache, regnum, buf);
}

void
regcache_cooked_write (struct regcache *regcache, int regnum, const void *buf)
{
  gdb_assert (regnum >= 0);
  gdb_assert (regnum < regcache->descr->nr_cooked_registers);
  if (regnum < regcache->descr->nr_raw_registers)
    regcache_raw_write (regcache, regnum, buf);
  else
    gdbarch_pseudo_register_write (regcache->descr->gdbarch, regcache,
				   regnum, buf);
}

/* Copy INLEN bytes of consecutive data from memory at MYADDR
   into registers starting with the MYREGSTART'th byte of register data.  */

void
write_register_bytes (int myregstart, char *myaddr, int inlen)
{
  int myregend = myregstart + inlen;
  int regnum;

  target_prepare_to_store ();

  /* Scan through the registers updating any that are covered by the
     range myregstart<=>myregend using write_register_gen, which does
     nice things like handling threads, and avoiding updates when the
     new and old contents are the same.  */

  for (regnum = 0; regnum < NUM_REGS + NUM_PSEUDO_REGS; regnum++)
    {
      int regstart, regend;

      regstart = REGISTER_BYTE (regnum);
      regend = regstart + REGISTER_RAW_SIZE (regnum);

      /* Is this register completely outside the range the user is writing?  */
      if (myregend <= regstart || regend <= myregstart)
	/* do nothing */ ;		

      /* Is this register completely within the range the user is writing?  */
      else if (myregstart <= regstart && regend <= myregend)
	write_register_gen (regnum, myaddr + (regstart - myregstart));

      /* The register partially overlaps the range being written.  */
      else
	{
	  char *regbuf = (char*) alloca (MAX_REGISTER_RAW_SIZE);
	  /* What's the overlap between this register's bytes and
             those the caller wants to write?  */
	  int overlapstart = max (regstart, myregstart);
	  int overlapend   = min (regend,   myregend);

	  /* We may be doing a partial update of an invalid register.
	     Update it from the target before scribbling on it.  */
	  read_register_gen (regnum, regbuf);

	  memcpy (registers + overlapstart,
		  myaddr + (overlapstart - myregstart),
		  overlapend - overlapstart);

	  target_store_registers (regnum);
	}
    }
}

/* Perform a partial register transfer using a read, modify, write
   operation.  */

typedef void (regcache_read_ftype) (struct regcache *regcache, int regnum,
				    void *buf);
typedef void (regcache_write_ftype) (struct regcache *regcache, int regnum,
				     const void *buf);

void
regcache_xfer_part (struct regcache *regcache, int regnum,
		    int offset, int len, void *in, const void *out,
		    regcache_read_ftype *read, regcache_write_ftype *write)
{
  struct regcache_descr *descr = regcache->descr;
  bfd_byte *reg = alloca (descr->max_register_size);
  gdb_assert (offset >= 0 && offset <= descr->sizeof_register[regnum]);
  gdb_assert (len >= 0 && offset + len <= descr->sizeof_register[regnum]);
  /* Something to do?  */
  if (offset + len == 0)
    return;
  /* Read (when needed) ... */
  if (in != NULL
      || offset > 0
      || offset + len < descr->sizeof_register[regnum])
    {
      gdb_assert (read != NULL);
      read (regcache, regnum, reg);
    }
  /* ... modify ... */
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
}

void
regcache_raw_read_part (struct regcache *regcache, int regnum,
			int offset, int len, void *buf)
{
  struct regcache_descr *descr = regcache->descr;
  gdb_assert (regnum >= 0 && regnum < descr->nr_raw_registers);
  regcache_xfer_part (regcache, regnum, offset, len, buf, NULL,
		      regcache_raw_read, regcache_raw_write);
}

void
regcache_raw_write_part (struct regcache *regcache, int regnum,
			 int offset, int len, const void *buf)
{
  struct regcache_descr *descr = regcache->descr;
  gdb_assert (regnum >= 0 && regnum < descr->nr_raw_registers);
  regcache_xfer_part (regcache, regnum, offset, len, NULL, buf,
		      regcache_raw_read, regcache_raw_write);
}

void
regcache_cooked_read_part (struct regcache *regcache, int regnum,
			   int offset, int len, void *buf)
{
  struct regcache_descr *descr = regcache->descr;
  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  regcache_xfer_part (regcache, regnum, offset, len, buf, NULL,
		      regcache_cooked_read, regcache_cooked_write);
}

void
regcache_cooked_write_part (struct regcache *regcache, int regnum,
			    int offset, int len, const void *buf)
{
  struct regcache_descr *descr = regcache->descr;
  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  regcache_xfer_part (regcache, regnum, offset, len, NULL, buf,
		      regcache_cooked_read, regcache_cooked_write);
}

/* Hack to keep code that view the register buffer as raw bytes
   working.  */

int
register_offset_hack (struct gdbarch *gdbarch, int regnum)
{
  struct regcache_descr *descr = regcache_descr (gdbarch);
  gdb_assert (regnum >= 0 && regnum < descr->nr_cooked_registers);
  return descr->register_offset[regnum];
}

static void
cooked_xfer_using_offset_hack (struct regcache *regcache,
			       int buf_start, int buf_len, void *in_b,
			       const void *out_b)
{
  struct regcache_descr *descr = regcache->descr;
  struct gdbarch *gdbarch = descr->gdbarch;
  bfd_byte *in_buf = in_b;
  const bfd_byte *out_buf = out_b;
  int buf_end = buf_start + buf_len;
  int regnum;
  char *reg_buf = alloca (descr->max_register_size);

  /* NOTE: cagney/2002-08-17: This code assumes that the register
     offsets are strictly increasing and do not overlap.  If this
     isn't the case then the bug is in the target architecture and NOT
     this code.  */

  /* NOTE: cagney/2002-08-17: This code assumes that only the
     registers covered by BUF_START:BUF_LEN should be transfered.  If,
     for some reason, there is a gap between two registers, then that
     gap isn't transfered.  (The gap shouldn't be there but that is
     another story.)  */

  /* Iterate through all registers looking for those that lie within
     BUF_START:BUF_LEN.  */

  for (regnum = 0; regnum < descr->nr_cooked_registers; regnum++)
    {
      /* The register's location.  */
      int reg_start = descr->register_offset[regnum];
      int reg_len = descr->sizeof_register[regnum];
      int reg_end = reg_start + reg_len;

      /* The START, END and LEN that falls within the current
         register.  */
      int xfer_start;
      int xfer_end;
      int xfer_len;

      /* start = max (reg_start, buf_start) */
      if (reg_start > buf_start)
	xfer_start = reg_start;
      else
	xfer_start = buf_start;
      
      /* end = min (reg_end, buf_end) */
      if (reg_end < buf_end)
	xfer_end = reg_end;
      else
	xfer_end = buf_end;
      
      /* The number of bytes to transfer.  If there isn't anything to
         transfer (the end is before the start) this will be -ve.  */
      xfer_len = xfer_end - xfer_start;

      if (xfer_len > 0)
	regcache_xfer_part (regcache, regnum, xfer_start - reg_start,
			    xfer_len, in_b, out_b, regcache_cooked_read,
			    regcache_cooked_write);
    }
}

void
regcache_cooked_read_using_offset_hack (struct regcache *regcache,
					int buf_start, int buf_len, void *b)
{
  cooked_xfer_using_offset_hack (regcache, buf_start, buf_len, b, NULL);
}

void
regcache_cooked_write_using_offset_hack (struct regcache *regcache,
					 int buf_start, int buf_len,
					 const void *b)
{
  cooked_xfer_using_offset_hack (regcache, buf_start, buf_len, NULL, b);
}

/* Return the contents of register REGNUM as an unsigned integer.  */

ULONGEST
read_register (int regnum)
{
  char *buf = alloca (REGISTER_RAW_SIZE (regnum));
  read_register_gen (regnum, buf);
  return (extract_unsigned_integer (buf, REGISTER_RAW_SIZE (regnum)));
}

ULONGEST
read_register_pid (int regnum, ptid_t ptid)
{
  ptid_t save_ptid;
  int save_pid;
  CORE_ADDR retval;

  if (ptid_equal (ptid, inferior_ptid))
    return read_register (regnum);

  save_ptid = inferior_ptid;

  inferior_ptid = ptid;

  retval = read_register (regnum);

  inferior_ptid = save_ptid;

  return retval;
}

/* Return the contents of register REGNUM as a signed integer.  */

LONGEST
read_signed_register (int regnum)
{
  void *buf = alloca (REGISTER_RAW_SIZE (regnum));
  read_register_gen (regnum, buf);
  return (extract_signed_integer (buf, REGISTER_RAW_SIZE (regnum)));
}

LONGEST
read_signed_register_pid (int regnum, ptid_t ptid)
{
  ptid_t save_ptid;
  LONGEST retval;

  if (ptid_equal (ptid, inferior_ptid))
    return read_signed_register (regnum);

  save_ptid = inferior_ptid;

  inferior_ptid = ptid;

  retval = read_signed_register (regnum);

  inferior_ptid = save_ptid;

  return retval;
}

/* Store VALUE into the raw contents of register number REGNUM.  */

void
write_register (int regnum, LONGEST val)
{
  void *buf;
  int size;
  size = REGISTER_RAW_SIZE (regnum);
  buf = alloca (size);
  store_signed_integer (buf, size, (LONGEST) val);
  write_register_gen (regnum, buf);
}

void
write_register_pid (int regnum, CORE_ADDR val, ptid_t ptid)
{
  ptid_t save_ptid;

  if (ptid_equal (ptid, inferior_ptid))
    {
      write_register (regnum, val);
      return;
    }

  save_ptid = inferior_ptid;

  inferior_ptid = ptid;

  write_register (regnum, val);

  inferior_ptid = save_ptid;
}

/* SUPPLY_REGISTER()

   Record that register REGNUM contains VAL.  This is used when the
   value is obtained from the inferior or core dump, so there is no
   need to store the value there.

   If VAL is a NULL pointer, then it's probably an unsupported register.
   We just set its value to all zeros.  We might want to record this
   fact, and report it to the users of read_register and friends.  */

void
supply_register (int regnum, const void *val)
{
#if 1
  if (! ptid_equal (registers_ptid, inferior_ptid))
    {
      registers_changed ();
      registers_ptid = inferior_ptid;
    }
#endif

  set_register_cached (regnum, 1);
  if (val)
    memcpy (register_buffer (current_regcache, regnum), val, 
	    REGISTER_RAW_SIZE (regnum));
  else
    memset (register_buffer (current_regcache, regnum), '\000', 
	    REGISTER_RAW_SIZE (regnum));

  /* On some architectures, e.g. HPPA, there are a few stray bits in
     some registers, that the rest of the code would like to ignore.  */

  /* NOTE: cagney/2001-03-16: The macro CLEAN_UP_REGISTER_VALUE is
     going to be deprecated.  Instead architectures will leave the raw
     register value as is and instead clean things up as they pass
     through the method gdbarch_pseudo_register_read() clean up the
     values. */

#ifdef DEPRECATED_CLEAN_UP_REGISTER_VALUE
  DEPRECATED_CLEAN_UP_REGISTER_VALUE \
    (regnum, register_buffer (current_regcache, regnum));
#endif
}

void
regcache_collect (int regnum, void *buf)
{
  memcpy (buf, register_buffer (current_regcache, regnum),
	  REGISTER_RAW_SIZE (regnum));
}


/* read_pc, write_pc, read_sp, write_sp, read_fp, etc.  Special
   handling for registers PC, SP, and FP.  */

/* NOTE: cagney/2001-02-18: The functions generic_target_read_pc(),
   read_pc_pid(), read_pc(), generic_target_write_pc(),
   write_pc_pid(), write_pc(), generic_target_read_sp(), read_sp(),
   generic_target_write_sp(), write_sp(), generic_target_read_fp() and
   read_fp(), will eventually be moved out of the reg-cache into
   either frame.[hc] or to the multi-arch framework.  The are not part
   of the raw register cache.  */

/* This routine is getting awfully cluttered with #if's.  It's probably
   time to turn this into READ_PC and define it in the tm.h file.
   Ditto for write_pc.

   1999-06-08: The following were re-written so that it assumes the
   existence of a TARGET_READ_PC et.al. macro.  A default generic
   version of that macro is made available where needed.

   Since the ``TARGET_READ_PC'' et.al. macro is going to be controlled
   by the multi-arch framework, it will eventually be possible to
   eliminate the intermediate read_pc_pid().  The client would call
   TARGET_READ_PC directly. (cagney). */

CORE_ADDR
generic_target_read_pc (ptid_t ptid)
{
#ifdef PC_REGNUM
  if (PC_REGNUM >= 0)
    {
      CORE_ADDR pc_val = ADDR_BITS_REMOVE ((CORE_ADDR) read_register_pid (PC_REGNUM, ptid));
      return pc_val;
    }
#endif
  internal_error (__FILE__, __LINE__,
		  "generic_target_read_pc");
  return 0;
}

CORE_ADDR
read_pc_pid (ptid_t ptid)
{
  ptid_t saved_inferior_ptid;
  CORE_ADDR pc_val;

  /* In case ptid != inferior_ptid. */
  saved_inferior_ptid = inferior_ptid;
  inferior_ptid = ptid;

  pc_val = TARGET_READ_PC (ptid);

  inferior_ptid = saved_inferior_ptid;
  return pc_val;
}

CORE_ADDR
read_pc (void)
{
  return read_pc_pid (inferior_ptid);
}

void
generic_target_write_pc (CORE_ADDR pc, ptid_t ptid)
{
#ifdef PC_REGNUM
  if (PC_REGNUM >= 0)
    write_register_pid (PC_REGNUM, pc, ptid);
  if (NPC_REGNUM >= 0)
    write_register_pid (NPC_REGNUM, pc + 4, ptid);
#else
  internal_error (__FILE__, __LINE__,
		  "generic_target_write_pc");
#endif
}

void
write_pc_pid (CORE_ADDR pc, ptid_t ptid)
{
  ptid_t saved_inferior_ptid;

  /* In case ptid != inferior_ptid. */
  saved_inferior_ptid = inferior_ptid;
  inferior_ptid = ptid;

  TARGET_WRITE_PC (pc, ptid);

  inferior_ptid = saved_inferior_ptid;
}

void
write_pc (CORE_ADDR pc)
{
  write_pc_pid (pc, inferior_ptid);
}

/* Cope with strage ways of getting to the stack and frame pointers */

CORE_ADDR
generic_target_read_sp (void)
{
#ifdef SP_REGNUM
  if (SP_REGNUM >= 0)
    return read_register (SP_REGNUM);
#endif
  internal_error (__FILE__, __LINE__,
		  "generic_target_read_sp");
}

CORE_ADDR
read_sp (void)
{
  return TARGET_READ_SP ();
}

void
generic_target_write_sp (CORE_ADDR val)
{
#ifdef SP_REGNUM
  if (SP_REGNUM >= 0)
    {
      write_register (SP_REGNUM, val);
      return;
    }
#endif
  internal_error (__FILE__, __LINE__,
		  "generic_target_write_sp");
}

void
write_sp (CORE_ADDR val)
{
  TARGET_WRITE_SP (val);
}

CORE_ADDR
generic_target_read_fp (void)
{
#ifdef FP_REGNUM
  if (FP_REGNUM >= 0)
    return read_register (FP_REGNUM);
#endif
  internal_error (__FILE__, __LINE__,
		  "generic_target_read_fp");
}

CORE_ADDR
read_fp (void)
{
  return TARGET_READ_FP ();
}

/* ARGSUSED */
static void
reg_flush_command (char *command, int from_tty)
{
  /* Force-flush the register cache.  */
  registers_changed ();
  if (from_tty)
    printf_filtered ("Register cache flushed.\n");
}

static void
build_regcache (void)
{
  current_regcache = regcache_xmalloc (current_gdbarch);
  current_regcache->passthrough_p = 1;
  registers = deprecated_grub_regcache_for_registers (current_regcache);
  register_valid = deprecated_grub_regcache_for_register_valid (current_regcache);
}

static void
dump_endian_bytes (struct ui_file *file, enum bfd_endian endian,
		   const unsigned char *buf, long len)
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
      internal_error (__FILE__, __LINE__, "Bad switch");
    }
}

enum regcache_dump_what
{
  regcache_dump_none, regcache_dump_raw, regcache_dump_cooked
};

static void
regcache_dump (struct regcache *regcache, struct ui_file *file,
	       enum regcache_dump_what what_to_dump)
{
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);
  int regnum;
  int footnote_nr = 0;
  int footnote_register_size = 0;
  int footnote_register_offset = 0;
  int footnote_register_type_name_null = 0;
  long register_offset = 0;
  unsigned char *buf = alloca (regcache->descr->max_register_size);

#if 0
  fprintf_unfiltered (file, "legacy_p %d\n", regcache->descr->legacy_p);
  fprintf_unfiltered (file, "nr_raw_registers %d\n",
		      regcache->descr->nr_raw_registers);
  fprintf_unfiltered (file, "nr_cooked_registers %d\n",
		      regcache->descr->nr_cooked_registers);
  fprintf_unfiltered (file, "sizeof_raw_registers %ld\n",
		      regcache->descr->sizeof_raw_registers);
  fprintf_unfiltered (file, "sizeof_raw_register_valid_p %ld\n",
		      regcache->descr->sizeof_raw_register_valid_p);
  fprintf_unfiltered (file, "max_register_size %ld\n",
		      regcache->descr->max_register_size);
  fprintf_unfiltered (file, "NUM_REGS %d\n", NUM_REGS);
  fprintf_unfiltered (file, "NUM_PSEUDO_REGS %d\n", NUM_PSEUDO_REGS);
#endif

  gdb_assert (regcache->descr->nr_cooked_registers
	      == (NUM_REGS + NUM_PSEUDO_REGS));

  for (regnum = -1; regnum < regcache->descr->nr_cooked_registers; regnum++)
    {
      /* Name.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %-10s", "Name");
      else
	{
	  const char *p = REGISTER_NAME (regnum);
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
      else if (regnum < NUM_REGS)
	fprintf_unfiltered (file, " %4d", regnum);
      else
	fprintf_unfiltered (file, " %4d", (regnum - NUM_REGS));

      /* Offset.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %6s  ", "Offset");
      else
	{
	  fprintf_unfiltered (file, " %6ld",
			      regcache->descr->register_offset[regnum]);
	  if (register_offset != regcache->descr->register_offset[regnum]
	      || register_offset != REGISTER_BYTE (regnum)
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
	{
	  fprintf_unfiltered (file, " %5ld",
			      regcache->descr->sizeof_register[regnum]);
	  if ((regcache->descr->sizeof_register[regnum]
	       != REGISTER_RAW_SIZE (regnum))
	      || (regcache->descr->sizeof_register[regnum]
		  != REGISTER_VIRTUAL_SIZE (regnum))
	      || (regcache->descr->sizeof_register[regnum]
		  != TYPE_LENGTH (register_type (regcache->descr->gdbarch,
						 regnum)))
	      )
	    {
	      if (!footnote_register_size)
		footnote_register_size = ++footnote_nr;
	      fprintf_unfiltered (file, "*%d", footnote_register_size);
	    }
	  else
	    fprintf_unfiltered (file, " ");
	}

      /* Type.  */
      if (regnum < 0)
	fprintf_unfiltered (file, " %-20s", "Type");
      else
	{
	  static const char blt[] = "builtin_type";
	  const char *t = TYPE_NAME (register_type (regcache->descr->gdbarch,
						    regnum));
	  if (t == NULL)
	    {
	      char *n;
	      if (!footnote_register_type_name_null)
		footnote_register_type_name_null = ++footnote_nr;
	      xasprintf (&n, "*%d", footnote_register_type_name_null);
	      make_cleanup (xfree, n);
	      t = n;
	    }
	  /* Chop a leading builtin_type.  */
	  if (strncmp (t, blt, strlen (blt)) == 0)
	    t += strlen (blt);
	  fprintf_unfiltered (file, " %-20s", t);
	}

      /* Value, raw.  */
      if (what_to_dump == regcache_dump_raw)
	{
	  if (regnum < 0)
	    fprintf_unfiltered (file, "Raw value");
	  else if (regnum >= regcache->descr->nr_raw_registers)
	    fprintf_unfiltered (file, "<cooked>");
	  else if (!regcache_valid_p (regcache, regnum))
	    fprintf_unfiltered (file, "<invalid>");
	  else
	    {
	      regcache_raw_read (regcache, regnum, buf);
	      fprintf_unfiltered (file, "0x");
	      dump_endian_bytes (file, TARGET_BYTE_ORDER, buf,
				 REGISTER_RAW_SIZE (regnum));
	    }
	}

      /* Value, cooked.  */
      if (what_to_dump == regcache_dump_cooked)
	{
	  if (regnum < 0)
	    fprintf_unfiltered (file, "Cooked value");
	  else
	    {
	      regcache_cooked_read (regcache, regnum, buf);
	      fprintf_unfiltered (file, "0x");
	      dump_endian_bytes (file, TARGET_BYTE_ORDER, buf,
				 REGISTER_VIRTUAL_SIZE (regnum));
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
    regcache_dump (current_regcache, gdb_stdout, what_to_dump);
  else
    {
      struct ui_file *file = gdb_fopen (args, "w");
      if (file == NULL)
	perror_with_name ("maintenance print architecture");
      regcache_dump (current_regcache, file, what_to_dump);    
      ui_file_delete (file);
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

void
_initialize_regcache (void)
{
  regcache_descr_handle = register_gdbarch_data (init_regcache_descr,
						 xfree_regcache_descr);
  REGISTER_GDBARCH_SWAP (current_regcache);
  register_gdbarch_swap (&registers, sizeof (registers), NULL);
  register_gdbarch_swap (&register_valid, sizeof (register_valid), NULL);
  register_gdbarch_swap (NULL, 0, build_regcache);

  add_com ("flushregs", class_maintenance, reg_flush_command,
	   "Force gdb to flush its register cache (maintainer command)");

   /* Initialize the thread/process associated with the current set of
      registers.  For now, -1 is special, and means `no current process'.  */
  registers_ptid = pid_to_ptid (-1);

  add_cmd ("registers", class_maintenance,
	   maintenance_print_registers,
	   "Print the internal register configuration.\
Takes an optional file parameter.",
	   &maintenanceprintlist);
  add_cmd ("raw-registers", class_maintenance,
	   maintenance_print_raw_registers,
	   "Print the internal register configuration including raw values.\
Takes an optional file parameter.",
	   &maintenanceprintlist);
  add_cmd ("cooked-registers", class_maintenance,
	   maintenance_print_cooked_registers,
	   "Print the internal register configuration including cooked values.\
Takes an optional file parameter.",
	   &maintenanceprintlist);

}
