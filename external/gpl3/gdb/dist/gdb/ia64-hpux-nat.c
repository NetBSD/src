/* Copyright (C) 2010-2013 Free Software Foundation, Inc.

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
#include "ia64-tdep.h"
#include "inferior.h"
#include "inf-ttrace.h"
#include "regcache.h"
#include "solib-ia64-hpux.h"

#include <ia64/sys/uregs.h>
#include <sys/ttrace.h>

/* The offsets used with ttrace to read the value of the raw registers.  */

static int u_offsets[] =
{ /* Static General Registers.  */
  -1,     __r1,   __r2,   __r3,   __r4,   __r5,   __r6,   __r7,
  __r8,   __r9,   __r10,  __r11,  __r12,  __r13,  __r14,  __r15,
  __r16,  __r17,  __r18,  __r19,  __r20,  __r21,  __r22,  __r23,
  __r24,  __r25,  __r26,  __r27,  __r28,  __r29,  __r30,  __r31,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,

  /* Static Floating-Point Registers.  */
    -1,     -1,   __f2,   __f3,   __f4,   __f5,   __f6,   __f7,
  __f8,   __f9,   __f10,  __f11,  __f12,  __f13,  __f14,  __f15,
  __f16,  __f17,  __f18,  __f19,  __f20,  __f21,  __f22,  __f23,
  __f24,  __f25,  __f26,  __f27,  __f28,  __f29,  __f30,  __f31,
  __f32,  __f33,  __f34,  __f35,  __f36,  __f37,  __f38,  __f39,
  __f40,  __f41,  __f42,  __f43,  __f44,  __f45,  __f46,  __f47,
  __f48,  __f49,  __f50,  __f51,  __f52,  __f53,  __f54,  __f55,
  __f56,  __f57,  __f58,  __f59,  __f60,  __f61,  __f62,  __f63,
  __f64,  __f65,  __f66,  __f67,  __f68,  __f69,  __f70,  __f71,
  __f72,  __f73,  __f74,  __f75,  __f76,  __f77,  __f78,  __f79,
  __f80,  __f81,  __f82,  __f83,  __f84,  __f85,  __f86,  __f87,
  __f88,  __f89,  __f90,  __f91,  __f92,  __f93,  __f94,  __f95,
  __f96,  __f97,  __f98,  __f99,  __f100, __f101, __f102, __f103,
  __f104, __f105, __f106, __f107, __f108, __f109, __f110, __f111,
  __f112, __f113, __f114, __f115, __f116, __f117, __f118, __f119,
  __f120, __f121, __f122, __f123, __f124, __f125, __f126, __f127,

  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
  -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,

  /* Branch Registers.  */
  __b0,   __b1,   __b2,   __b3,   __b4,   __b5,   __b6,   __b7,

  /* Virtual frame pointer and virtual return address pointer.  */
  -1, -1,

  /* Other registers.  */
  __pr, __ip, __cr_ipsr, __cfm,

  /* Kernel registers.  */
  -1,   -1,   -1,   -1,
  -1,   -1,   -1,   -1,

  -1, -1, -1, -1, -1, -1, -1, -1,

  /* Some application registers.  */
  __ar_rsc, __ar_bsp, __ar_bspstore, __ar_rnat,

  -1,
  -1,  /* Not available: FCR, IA32 floating control register.  */
  -1, -1,

  -1,         /* Not available: EFLAG.  */
  -1,         /* Not available: CSD.  */
  -1,         /* Not available: SSD.  */
  -1,         /* Not available: CFLG.  */
  -1,         /* Not available: FSR.  */
  -1,         /* Not available: FIR.  */
  -1,         /* Not available: FDR.  */
  -1,
  __ar_ccv, -1, -1, -1, __ar_unat, -1, -1, -1,
  __ar_fpsr, -1, -1, -1,
  -1,         /* Not available: ITC.  */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  __ar_pfs, __ar_lc, __ar_ec,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1
  /* All following registers, starting with nat0, are handled as
     pseudo registers, and hence are handled separately.  */
};

/* Some register have a fixed value and can not be modified.
   Store their value in static constant buffers that can be used
   later to fill the register cache.  */
static const char r0_value[8] = {0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00};
static const char f0_value[16] = {0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00};
static const char f1_value[16] = {0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0xff, 0xff,
                                  0x80, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00};

/* The "to_wait" routine from the "inf-ttrace" layer.  */

static ptid_t (*super_to_wait) (struct target_ops *, ptid_t,
				struct target_waitstatus *, int);

/* The "to_wait" target_ops routine routine for ia64-hpux.  */

static ptid_t
ia64_hpux_wait (struct target_ops *ops, ptid_t ptid,
		struct target_waitstatus *ourstatus, int options)
{
  ptid_t new_ptid;

  new_ptid = super_to_wait (ops, ptid, ourstatus, options);

  /* If this is a DLD event (hard-coded breakpoint instruction
     that was activated by the solib-ia64-hpux module), we need to
     process it, and then resume the execution as if the event did
     not happen.  */
  if (ourstatus->kind == TARGET_WAITKIND_STOPPED
      && ourstatus->value.sig == GDB_SIGNAL_TRAP
      && ia64_hpux_at_dld_breakpoint_p (new_ptid))
    {
      ia64_hpux_handle_dld_breakpoint (new_ptid);

      target_resume (new_ptid, 0, GDB_SIGNAL_0);
      ourstatus->kind = TARGET_WAITKIND_IGNORE;
    }

  return new_ptid;
}

/* Fetch the RNAT register and supply it to the REGCACHE.  */

static void
ia64_hpux_fetch_rnat_register (struct regcache *regcache)
{
  CORE_ADDR addr;
  gdb_byte buf[8];
  int status;

  /* The value of RNAT is stored at bsp|0x1f8, and must be read using
     TT_LWP_RDRSEBS.  */

  regcache_raw_read_unsigned (regcache, IA64_BSP_REGNUM, &addr);
  addr |= 0x1f8;

  status = ttrace (TT_LWP_RDRSEBS, ptid_get_pid (inferior_ptid),
		   ptid_get_lwp (inferior_ptid), addr, sizeof (buf),
		   (uintptr_t) buf);
  if (status < 0)
    error (_("failed to read RNAT register at %s"),
	   paddress (get_regcache_arch(regcache), addr));

  regcache_raw_supply (regcache, IA64_RNAT_REGNUM, buf);
}

/* Read the value of the register saved at OFFSET in the save_state_t
   structure, and store its value in BUF.  LEN is the size of the register
   to be read.  */

static int
ia64_hpux_read_register_from_save_state_t (int offset, gdb_byte *buf, int len)
{
  int status;

 status = ttrace (TT_LWP_RUREGS, ptid_get_pid (inferior_ptid),
		  ptid_get_lwp (inferior_ptid), offset, len, (uintptr_t) buf);

  return status;
}

/* Fetch register REGNUM from the inferior.  */

static void
ia64_hpux_fetch_register (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int offset, len, status;
  gdb_byte *buf;

  if (regnum == IA64_GR0_REGNUM)
    {
      /* r0 is always 0.  */
      regcache_raw_supply (regcache, regnum, r0_value);
      return;
    }

  if (regnum == IA64_FR0_REGNUM)
    {
      /* f0 is always 0.0.  */
      regcache_raw_supply (regcache, regnum, f0_value);
      return;
    }

  if (regnum == IA64_FR1_REGNUM)
    {
      /* f1 is always 1.0.  */
      regcache_raw_supply (regcache, regnum, f1_value);
      return;
    }

  if (regnum == IA64_RNAT_REGNUM)
    {
      ia64_hpux_fetch_rnat_register (regcache);
      return;
    }

  /* Get the register location. If the register can not be fetched,
     then return now.  */
  offset = u_offsets[regnum];
  if (offset == -1)
    return;

  len = register_size (gdbarch, regnum);
  buf = alloca (len * sizeof (gdb_byte));
  status = ia64_hpux_read_register_from_save_state_t (offset, buf, len);
  if (status < 0)
    warning (_("Failed to read register value for %s."),
             gdbarch_register_name (gdbarch, regnum));

  regcache_raw_supply (regcache, regnum, buf);
}

/* The "to_fetch_registers" target_ops routine for ia64-hpux.  */

static void
ia64_hpux_fetch_registers (struct target_ops *ops,
			   struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (get_regcache_arch (regcache));
	 regnum++)
      ia64_hpux_fetch_register (regcache, regnum);
  else
    ia64_hpux_fetch_register (regcache, regnum);
}

/* Save register REGNUM (stored in BUF) in the save_state_t structure.
   LEN is the size of the register in bytes.

   Return the value from the corresponding ttrace call (a negative value
   means that the operation failed).  */

static int
ia64_hpux_write_register_to_saved_state_t (int offset, gdb_byte *buf, int len)
{
  return ttrace (TT_LWP_WUREGS, ptid_get_pid (inferior_ptid),
		 ptid_get_lwp (inferior_ptid), offset, len, (uintptr_t) buf);
}

/* Store register REGNUM into the inferior.  */

static void
ia64_hpux_store_register (const struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int offset = u_offsets[regnum];
  gdb_byte *buf;
  int len, status;

  /* If the register can not be stored, then return now.  */
  if (offset == -1)
    return;

  /* I don't know how to store that register for now.  So just ignore any
     request to store it, to avoid an internal error.  */
  if (regnum == IA64_PSR_REGNUM)
    return;

  len = register_size (gdbarch, regnum);
  buf = alloca (len * sizeof (gdb_byte));
  regcache_raw_collect (regcache, regnum, buf);

  status = ia64_hpux_write_register_to_saved_state_t (offset, buf, len);

  if (status < 0)
    error (_("failed to write register value for %s."),
           gdbarch_register_name (gdbarch, regnum));
}

/* The "to_store_registers" target_ops routine for ia64-hpux.  */

static void
ia64_hpux_store_registers (struct target_ops *ops,
			   struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (get_regcache_arch (regcache));
	 regnum++)
      ia64_hpux_store_register (regcache, regnum);
  else
    ia64_hpux_store_register (regcache, regnum);
}

/* The "xfer_partial" routine from the "inf-ttrace" target layer.
   Ideally, we would like to use this routine for all transfer
   requests, but this platforms has a lot of special cases that
   need to be handled manually.  So we override this routine and
   delegate back if we detect that we are not in a special case.  */

static LONGEST (*super_xfer_partial) (struct target_ops *, enum target_object,
				      const char *, gdb_byte *,
				      const gdb_byte *, ULONGEST, LONGEST);

/* The "xfer_partial" routine for a memory region that is completely
   outside of the backing-store region.  */

static LONGEST
ia64_hpux_xfer_memory_no_bs (struct target_ops *ops, const char *annex,
			     gdb_byte *readbuf, const gdb_byte *writebuf,
			     CORE_ADDR addr, LONGEST len)
{
  /* Memory writes need to be aligned on 16byte boundaries, at least
     when writing in the text section.  On the other hand, the size
     of the buffer does not need to be a multiple of 16bytes.

     No such restriction when performing memory reads.  */

  if (writebuf && addr & 0x0f)
    {
      const CORE_ADDR aligned_addr = addr & ~0x0f;
      const int aligned_len = len + (addr - aligned_addr);
      gdb_byte *aligned_buf = alloca (aligned_len * sizeof (gdb_byte));
      LONGEST status;

      /* Read the portion of memory between ALIGNED_ADDR and ADDR, so
         that we can write it back during our aligned memory write.  */
      status = super_xfer_partial (ops, TARGET_OBJECT_MEMORY, annex,
				   aligned_buf /* read */,
				   NULL /* write */,
				   aligned_addr, addr - aligned_addr);
      if (status <= 0)
	return 0;
      memcpy (aligned_buf + (addr - aligned_addr), writebuf, len);

      return super_xfer_partial (ops, TARGET_OBJECT_MEMORY, annex,
				 NULL /* read */, aligned_buf /* write */,
				 aligned_addr, aligned_len);
    }
  else
    /* Memory read or properly aligned memory write.  */
    return super_xfer_partial (ops, TARGET_OBJECT_MEMORY, annex, readbuf,
			       writebuf, addr, len);
}

/* Read LEN bytes at ADDR from memory, and store it in BUF.  This memory
   region is assumed to be inside the backing store.

   Return zero if the operation failed.  */

static int
ia64_hpux_read_memory_bs (gdb_byte *buf, CORE_ADDR addr, int len)
{
  gdb_byte tmp_buf[8];
  CORE_ADDR tmp_addr = addr & ~0x7;

  while (tmp_addr < addr + len)
    {
      int status;
      int skip_lo = 0;
      int skip_hi = 0;

      status = ttrace (TT_LWP_RDRSEBS, ptid_get_pid (inferior_ptid),
		       ptid_get_lwp (inferior_ptid), tmp_addr,
		       sizeof (tmp_buf), (uintptr_t) tmp_buf);
      if (status < 0)
        return 0;

      if (tmp_addr < addr)
        skip_lo = addr - tmp_addr;

      if (tmp_addr + sizeof (tmp_buf) > addr + len)
        skip_hi = (tmp_addr + sizeof (tmp_buf)) - (addr + len);

      memcpy (buf + (tmp_addr + skip_lo - addr),
              tmp_buf + skip_lo,
              sizeof (tmp_buf) - skip_lo - skip_hi);

      tmp_addr += sizeof (tmp_buf);
    }

  return 1;
}

/* Write LEN bytes from BUF in memory at ADDR.  This memory region is assumed
   to be inside the backing store.

   Return zero if the operation failed.  */

static int
ia64_hpux_write_memory_bs (const gdb_byte *buf, CORE_ADDR addr, int len)
{
  gdb_byte tmp_buf[8];
  CORE_ADDR tmp_addr = addr & ~0x7;

  while (tmp_addr < addr + len)
    {
      int status;
      int lo = 0;
      int hi = 7;

      if (tmp_addr < addr || tmp_addr + sizeof (tmp_buf) > addr + len)
	/* Part of the 8byte region pointed by tmp_addr needs to be preserved.
	   So read it in before we copy the data that needs to be changed.  */
	if (!ia64_hpux_read_memory_bs (tmp_buf, tmp_addr, sizeof (tmp_buf)))
	  return 0;

      if (tmp_addr < addr)
        lo = addr - tmp_addr;

      if (tmp_addr + sizeof (tmp_buf) > addr + len)
        hi = addr - tmp_addr + len - 1;

      memcpy (tmp_buf + lo, buf + tmp_addr - addr  + lo, hi - lo + 1);

      status = ttrace (TT_LWP_WRRSEBS, ptid_get_pid (inferior_ptid),
		       ptid_get_lwp (inferior_ptid), tmp_addr,
		       sizeof (tmp_buf), (uintptr_t) tmp_buf);
      if (status < 0)
        return 0;

      tmp_addr += sizeof (tmp_buf);
    }

  return 1;
}

/* The "xfer_partial" routine for a memory region that is completely
   inside of the backing-store region.  */

static LONGEST
ia64_hpux_xfer_memory_bs (struct target_ops *ops, const char *annex,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  CORE_ADDR addr, LONGEST len)
{
  int success;

  if (readbuf)
    success = ia64_hpux_read_memory_bs (readbuf, addr, len);
  else
    success = ia64_hpux_write_memory_bs (writebuf, addr, len);

  if (success)
    return len;
  else
    return 0;
}

/* Get a register value as a unsigned value directly from the system,
   instead of going through the regcache.

   This function is meant to be used when inferior_ptid is not
   a thread/process known to GDB.  */

static ULONGEST
ia64_hpux_get_register_from_save_state_t (int regnum, int reg_size)
{
  gdb_byte *buf = alloca (reg_size);
  int offset = u_offsets[regnum];
  int status;

  /* The register is assumed to be available for fetching.  */
  gdb_assert (offset != -1);

  status = ia64_hpux_read_register_from_save_state_t (offset, buf, reg_size);
  if (status < 0)
    {
      /* This really should not happen.  If it does, emit a warning
	 and pretend the register value is zero.  Not exactly the best
	 error recovery mechanism, but better than nothing.  We will
	 try to do better if we can demonstrate that this can happen
	 under normal circumstances.  */
      warning (_("Failed to read value of register number %d."), regnum);
      return 0;
    }

  return extract_unsigned_integer (buf, reg_size, BFD_ENDIAN_BIG);
}

/* The "xfer_partial" target_ops routine for ia64-hpux, in the case
   where the requested object is TARGET_OBJECT_MEMORY.  */

static LONGEST
ia64_hpux_xfer_memory (struct target_ops *ops, const char *annex,
		       gdb_byte *readbuf, const gdb_byte *writebuf,
		       CORE_ADDR addr, LONGEST len)
{
  CORE_ADDR bsp, bspstore;
  CORE_ADDR start_addr, short_len;
  int status = 0;

  /* The back-store region cannot be read/written by the standard memory
     read/write operations.  So we handle the memory region piecemeal:
       (1) and (2) The regions before and after the backing-store region,
           which can be treated as normal memory;
       (3) The region inside the backing-store, which needs to be
           read/written specially.  */

  if (in_inferior_list (ptid_get_pid (inferior_ptid)))
    {
      struct regcache *regcache = get_current_regcache ();

      regcache_raw_read_unsigned (regcache, IA64_BSP_REGNUM, &bsp);
      regcache_raw_read_unsigned (regcache, IA64_BSPSTORE_REGNUM, &bspstore);
    }
  else
    {
      /* This is probably a child of our inferior created by a fork.
	 Because this process has not been added to our inferior list
	 (we are probably in the process of handling that child
	 process), we do not have a regcache to read the registers
	 from.  So get those values directly from the kernel.  */
      bsp = ia64_hpux_get_register_from_save_state_t (IA64_BSP_REGNUM, 8);
      bspstore =
	ia64_hpux_get_register_from_save_state_t (IA64_BSPSTORE_REGNUM, 8);
    }

  /* 1. Memory region before BSPSTORE.  */

  if (addr < bspstore)
    {
      short_len = len;
      if (addr + len > bspstore)
        short_len = bspstore - addr;

      status = ia64_hpux_xfer_memory_no_bs (ops, annex, readbuf, writebuf,
					    addr, short_len);
      if (status <= 0)
        return 0;
    }

  /* 2. Memory region after BSP.  */

  if (addr + len > bsp)
    {
      start_addr = addr;
      if (start_addr < bsp)
        start_addr = bsp;
      short_len = len + addr - start_addr;

      status = ia64_hpux_xfer_memory_no_bs
		(ops, annex,
		 readbuf ? readbuf + (start_addr - addr) : NULL,
		 writebuf ? writebuf + (start_addr - addr) : NULL,
		 start_addr, short_len);
      if (status <= 0)
	return 0;
    }

  /* 3. Memory region between BSPSTORE and BSP.  */

  if (bspstore != bsp
      && ((addr < bspstore && addr + len > bspstore)
	  || (addr + len <= bsp && addr + len > bsp)))
    {
      start_addr = addr;
      if (addr < bspstore)
        start_addr = bspstore;
      short_len = len + addr - start_addr;

      if (start_addr + short_len > bsp)
        short_len = bsp - start_addr;

      gdb_assert (short_len > 0);

      status = ia64_hpux_xfer_memory_bs
		 (ops, annex,
		  readbuf ? readbuf + (start_addr - addr) : NULL,
		  writebuf ? writebuf + (start_addr - addr) : NULL,
		  start_addr, short_len);
      if (status < 0)
	return 0;
    }

  return len;
}

/* Handle the transfer of TARGET_OBJECT_HPUX_UREGS objects on ia64-hpux.
   ANNEX is currently ignored.

   The current implementation does not support write transfers (because
   we do not currently do not need these transfers), and will raise
   a failed assertion if WRITEBUF is not NULL.  */

static LONGEST
ia64_hpux_xfer_uregs (struct target_ops *ops, const char *annex,
		      gdb_byte *readbuf, const gdb_byte *writebuf,
		      ULONGEST offset, LONGEST len)
{
  int status;

  gdb_assert (writebuf == NULL);

  status = ia64_hpux_read_register_from_save_state_t (offset, readbuf, len);
  if (status < 0)
    return -1;
  return len;
}

/* Handle the transfer of TARGET_OBJECT_HPUX_SOLIB_GOT objects on ia64-hpux.

   The current implementation does not support write transfers (because
   we do not currently do not need these transfers), and will raise
   a failed assertion if WRITEBUF is not NULL.  */

static LONGEST
ia64_hpux_xfer_solib_got (struct target_ops *ops, const char *annex,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST offset, LONGEST len)
{
  CORE_ADDR fun_addr;
  /* The linkage pointer.  We use a uint64_t to make sure that the size
     of the object we are returning is always 64 bits long, as explained
     in the description of the TARGET_OBJECT_HPUX_SOLIB_GOT object.
     This is probably paranoia, but we do not use a CORE_ADDR because
     it could conceivably be larger than uint64_t.  */
  uint64_t got;

  gdb_assert (writebuf == NULL);

  if (offset > sizeof (got))
    return 0;

  fun_addr = string_to_core_addr (annex);
  got = ia64_hpux_get_solib_linkage_addr (fun_addr);

  if (len > sizeof (got) - offset)
    len = sizeof (got) - offset;
  memcpy (readbuf, &got + offset, len);

  return len;
}

/* The "to_xfer_partial" target_ops routine for ia64-hpux.  */

static LONGEST
ia64_hpux_xfer_partial (struct target_ops *ops, enum target_object object,
			const char *annex, gdb_byte *readbuf,
			const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
  LONGEST val;

  if (object == TARGET_OBJECT_MEMORY)
    val = ia64_hpux_xfer_memory (ops, annex, readbuf, writebuf, offset, len);
  else if (object == TARGET_OBJECT_HPUX_UREGS)
    val = ia64_hpux_xfer_uregs (ops, annex, readbuf, writebuf, offset, len);
  else if (object == TARGET_OBJECT_HPUX_SOLIB_GOT)
    val = ia64_hpux_xfer_solib_got (ops, annex, readbuf, writebuf, offset,
				    len);
  else
    val = super_xfer_partial (ops, object, annex, readbuf, writebuf, offset,
			      len);

  return val;
}

/* The "to_can_use_hw_breakpoint" target_ops routine for ia64-hpux.  */

static int
ia64_hpux_can_use_hw_breakpoint (int type, int cnt, int othertype)
{
  /* No hardware watchpoint/breakpoint support yet.  */
  return 0;
}

/* The "to_mourn_inferior" routine from the "inf-ttrace" target_ops layer.  */

static void (*super_mourn_inferior) (struct target_ops *);

/* The "to_mourn_inferior" target_ops routine for ia64-hpux.  */

static void
ia64_hpux_mourn_inferior (struct target_ops *ops)
{
  const int pid = ptid_get_pid (inferior_ptid);
  int status;

  super_mourn_inferior (ops);

  /* On this platform, the process still exists even after we received
     an exit event.  Detaching from the process isn't sufficient either,
     as it only turns the process into a zombie.  So the only solution
     we found is to kill it.  */
  ttrace (TT_PROC_EXIT, pid, 0, 0, 0, 0);
  wait (&status);
}

/* Prevent warning from -Wmissing-prototypes.  */
void _initialize_ia64_hpux_nat (void);

void
_initialize_ia64_hpux_nat (void)
{
  struct target_ops *t;

  t = inf_ttrace_target ();
  super_to_wait = t->to_wait;
  super_xfer_partial = t->to_xfer_partial;
  super_mourn_inferior = t->to_mourn_inferior;

  t->to_wait = ia64_hpux_wait;
  t->to_fetch_registers = ia64_hpux_fetch_registers;
  t->to_store_registers = ia64_hpux_store_registers;
  t->to_xfer_partial = ia64_hpux_xfer_partial;
  t->to_can_use_hw_breakpoint = ia64_hpux_can_use_hw_breakpoint;
  t->to_mourn_inferior = ia64_hpux_mourn_inferior;
  t->to_attach_no_wait = 1;

  add_target (t);
}
