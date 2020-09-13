/* Cell SPU GNU/Linux multi-architecture debugging support.
   Copyright (C) 2009-2019 Free Software Foundation, Inc.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.

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
#include "gdbcore.h"
#include "gdbcmd.h"
#include "arch-utils.h"
#include "observable.h"
#include "inferior.h"
#include "regcache.h"
#include "symfile.h"
#include "objfiles.h"
#include "solib.h"
#include "solist.h"

#include "ppc-tdep.h"
#include "ppc-linux-tdep.h"
#include "spu-tdep.h"

/* The SPU multi-architecture support target.  */

static const target_info spu_multiarch_target_info = {
  "spu",
  N_("SPU multi-architecture support."),
  N_("SPU multi-architecture support.")
};

struct spu_multiarch_target final : public target_ops
{
  const target_info &info () const override
  { return spu_multiarch_target_info; }

  strata stratum () const override { return arch_stratum; }

  void mourn_inferior () override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  int search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
		     const gdb_byte *pattern, ULONGEST pattern_len,
		     CORE_ADDR *found_addrp) override;

  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;

  struct gdbarch *thread_architecture (ptid_t) override;
};

static spu_multiarch_target spu_ops;

/* Number of SPE objects loaded into the current inferior.  */
static int spu_nr_solib;

/* Stand-alone SPE executable?  */
#define spu_standalone_p() \
  (symfile_objfile && symfile_objfile->obfd \
   && bfd_get_arch (symfile_objfile->obfd) == bfd_arch_spu)

/* PPU side system calls.  */
#define INSTR_SC	0x44000002
#define NR_spu_run	0x0116

/* If the PPU thread is currently stopped on a spu_run system call,
   return to FD and ADDR the file handle and NPC parameter address
   used with the system call.  Return non-zero if successful.  */
static int
parse_spufs_run (ptid_t ptid, int *fd, CORE_ADDR *addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  struct gdbarch_tdep *tdep;
  struct regcache *regcache;
  gdb_byte buf[4];
  ULONGEST regval;

  /* If we're not on PPU, there's nothing to detect.  */
  if (gdbarch_bfd_arch_info (target_gdbarch ())->arch != bfd_arch_powerpc)
    return 0;

  /* If we're called too early (e.g. after fork), we cannot
     access the inferior yet.  */
  if (find_inferior_ptid (ptid) == NULL)
    return 0;

  /* Get PPU-side registers.  */
  regcache = get_thread_arch_regcache (ptid, target_gdbarch ());
  tdep = gdbarch_tdep (target_gdbarch ());

  /* Fetch instruction preceding current NIP.  */
  {
    scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);
    inferior_ptid = ptid;
    regval = target_read_memory (regcache_read_pc (regcache) - 4, buf, 4);
  }
  if (regval != 0)
    return 0;
  /* It should be a "sc" instruction.  */
  if (extract_unsigned_integer (buf, 4, byte_order) != INSTR_SC)
    return 0;
  /* System call number should be NR_spu_run.  */
  regcache_cooked_read_unsigned (regcache, tdep->ppc_gp0_regnum, &regval);
  if (regval != NR_spu_run)
    return 0;

  /* Register 3 contains fd, register 4 the NPC param pointer.  */
  regcache_cooked_read_unsigned (regcache, PPC_ORIG_R3_REGNUM, &regval);
  *fd = (int) regval;
  regcache_cooked_read_unsigned (regcache, tdep->ppc_gp0_regnum + 4, &regval);
  *addr = (CORE_ADDR) regval;
  return 1;
}

/* Find gdbarch for SPU context SPUFS_FD.  */
static struct gdbarch *
spu_gdbarch (int spufs_fd)
{
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  info.bfd_arch_info = bfd_lookup_arch (bfd_arch_spu, bfd_mach_spu);
  info.byte_order = BFD_ENDIAN_BIG;
  info.osabi = GDB_OSABI_LINUX;
  info.id = &spufs_fd;
  return gdbarch_find_by_info (info);
}

/* Override the to_thread_architecture routine.  */
struct gdbarch *
spu_multiarch_target::thread_architecture (ptid_t ptid)
{
  int spufs_fd;
  CORE_ADDR spufs_addr;

  if (parse_spufs_run (ptid, &spufs_fd, &spufs_addr))
    return spu_gdbarch (spufs_fd);

  return beneath ()->thread_architecture (ptid);
}

/* Override the to_region_ok_for_hw_watchpoint routine.  */

int
spu_multiarch_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  /* We cannot watch SPU local store.  */
  if (SPUADDR_SPU (addr) != -1)
    return 0;

  return beneath ()->region_ok_for_hw_watchpoint (addr, len);
}

/* Override the to_fetch_registers routine.  */

void
spu_multiarch_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int spufs_fd;
  CORE_ADDR spufs_addr;

  /* Since we use functions that rely on inferior_ptid, we need to set and
     restore it.  */
  scoped_restore save_ptid
    = make_scoped_restore (&inferior_ptid, regcache->ptid ());

  /* This version applies only if we're currently in spu_run.  */
  if (gdbarch_bfd_arch_info (gdbarch)->arch != bfd_arch_spu)
    {
      beneath ()->fetch_registers (regcache, regno);
      return;
    }

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (inferior_ptid, &spufs_fd, &spufs_addr))
    return;

  /* The ID register holds the spufs file handle.  */
  if (regno == -1 || regno == SPU_ID_REGNUM)
    {
      gdb_byte buf[4];
      store_unsigned_integer (buf, 4, byte_order, spufs_fd);
      regcache->raw_supply (SPU_ID_REGNUM, buf);
    }

  /* The NPC register is found in PPC memory at SPUFS_ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      gdb_byte buf[4];

      if (target_read (beneath (), TARGET_OBJECT_MEMORY, NULL,
		       buf, spufs_addr, sizeof buf) == sizeof buf)
	regcache->raw_supply (SPU_PC_REGNUM, buf);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_GPRS))
    {
      gdb_byte buf[16 * SPU_NUM_GPRS];
      char annex[32];
      int i;

      xsnprintf (annex, sizeof annex, "%d/regs", spufs_fd);
      if (target_read (beneath (), TARGET_OBJECT_SPU, annex,
		       buf, 0, sizeof buf) == sizeof buf)
	for (i = 0; i < SPU_NUM_GPRS; i++)
	  regcache->raw_supply (i, buf + i*16);
    }
}

/* Override the to_store_registers routine.  */

void
spu_multiarch_target::store_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int spufs_fd;
  CORE_ADDR spufs_addr;

  /* Since we use functions that rely on inferior_ptid, we need to set and
     restore it.  */
  scoped_restore save_ptid
    = make_scoped_restore (&inferior_ptid, regcache->ptid ());

  /* This version applies only if we're currently in spu_run.  */
  if (gdbarch_bfd_arch_info (gdbarch)->arch != bfd_arch_spu)
    {
      beneath ()->store_registers (regcache, regno);
      return;
    }

  /* We must be stopped on a spu_run system call.  */
  if (!parse_spufs_run (inferior_ptid, &spufs_fd, &spufs_addr))
    return;

  /* The NPC register is found in PPC memory at SPUFS_ADDR.  */
  if (regno == -1 || regno == SPU_PC_REGNUM)
    {
      gdb_byte buf[4];
      regcache->raw_collect (SPU_PC_REGNUM, buf);

      target_write (beneath (), TARGET_OBJECT_MEMORY, NULL,
		    buf, spufs_addr, sizeof buf);
    }

  /* The GPRs are found in the "regs" spufs file.  */
  if (regno == -1 || (regno >= 0 && regno < SPU_NUM_GPRS))
    {
      gdb_byte buf[16 * SPU_NUM_GPRS];
      char annex[32];
      int i;

      for (i = 0; i < SPU_NUM_GPRS; i++)
	regcache->raw_collect (i, buf + i*16);

      xsnprintf (annex, sizeof annex, "%d/regs", spufs_fd);
      target_write (beneath (), TARGET_OBJECT_SPU, annex,
		    buf, 0, sizeof buf);
    }
}

/* Override the to_xfer_partial routine.  */

enum target_xfer_status
spu_multiarch_target::xfer_partial (enum target_object object,
				    const char *annex, gdb_byte *readbuf,
				    const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
				    ULONGEST *xfered_len)
{
  struct target_ops *ops_beneath = this->beneath ();

  /* Use the "mem" spufs file to access SPU local store.  */
  if (object == TARGET_OBJECT_MEMORY)
    {
      int fd = SPUADDR_SPU (offset);
      CORE_ADDR addr = SPUADDR_ADDR (offset);
      char mem_annex[32], lslr_annex[32];
      gdb_byte buf[32];
      ULONGEST lslr;
      enum target_xfer_status ret;

      if (fd >= 0)
	{
	  xsnprintf (mem_annex, sizeof mem_annex, "%d/mem", fd);
	  ret = ops_beneath->xfer_partial (TARGET_OBJECT_SPU,
					   mem_annex, readbuf, writebuf,
					   addr, len, xfered_len);
	  if (ret == TARGET_XFER_OK)
	    return ret;

	  /* SPU local store access wraps the address around at the
	     local store limit.  We emulate this here.  To avoid needing
	     an extra access to retrieve the LSLR, we only do that after
	     trying the original address first, and getting end-of-file.  */
	  xsnprintf (lslr_annex, sizeof lslr_annex, "%d/lslr", fd);
	  memset (buf, 0, sizeof buf);
	  if (ops_beneath->xfer_partial (TARGET_OBJECT_SPU,
					 lslr_annex, buf, NULL,
					 0, sizeof buf, xfered_len)
	      != TARGET_XFER_OK)
	    return ret;

	  lslr = strtoulst ((char *) buf, NULL, 16);
	  return ops_beneath->xfer_partial (TARGET_OBJECT_SPU,
					    mem_annex, readbuf, writebuf,
					    addr & lslr, len, xfered_len);
	}
    }

  return ops_beneath->xfer_partial (object, annex,
				    readbuf, writebuf, offset, len, xfered_len);
}

/* Override the to_search_memory routine.  */
int
spu_multiarch_target::search_memory (CORE_ADDR start_addr, ULONGEST search_space_len,
				     const gdb_byte *pattern, ULONGEST pattern_len,
				     CORE_ADDR *found_addrp)
{
  /* For SPU local store, always fall back to the simple method.  */
  if (SPUADDR_SPU (start_addr) >= 0)
    return simple_search_memory (this, start_addr, search_space_len,
				 pattern, pattern_len, found_addrp);

  return beneath ()->search_memory (start_addr, search_space_len,
				    pattern, pattern_len, found_addrp);
}


/* Push and pop the SPU multi-architecture support target.  */

static void
spu_multiarch_activate (void)
{
  /* If GDB was configured without SPU architecture support,
     we cannot install SPU multi-architecture support either.  */
  if (spu_gdbarch (-1) == NULL)
    return;

  push_target (&spu_ops);

  /* Make sure the thread architecture is re-evaluated.  */
  registers_changed ();
}

static void
spu_multiarch_deactivate (void)
{
  unpush_target (&spu_ops);

  /* Make sure the thread architecture is re-evaluated.  */
  registers_changed ();
}

static void
spu_multiarch_inferior_created (struct target_ops *ops, int from_tty)
{
  if (spu_standalone_p ())
    spu_multiarch_activate ();
}

static void
spu_multiarch_solib_loaded (struct so_list *so)
{
  if (!spu_standalone_p ())
    if (so->abfd && bfd_get_arch (so->abfd) == bfd_arch_spu)
      if (spu_nr_solib++ == 0)
	spu_multiarch_activate ();
}

static void
spu_multiarch_solib_unloaded (struct so_list *so)
{
  if (!spu_standalone_p ())
    if (so->abfd && bfd_get_arch (so->abfd) == bfd_arch_spu)
      if (--spu_nr_solib == 0)
	spu_multiarch_deactivate ();
}

void
spu_multiarch_target::mourn_inferior ()
{
  beneath ()->mourn_inferior ();
  spu_multiarch_deactivate ();
}

void
_initialize_spu_multiarch (void)
{
  /* Install observers to watch for SPU objects.  */
  gdb::observers::inferior_created.attach (spu_multiarch_inferior_created);
  gdb::observers::solib_loaded.attach (spu_multiarch_solib_loaded);
  gdb::observers::solib_unloaded.attach (spu_multiarch_solib_unloaded);
}

