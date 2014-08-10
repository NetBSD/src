/* Native support for the SGI Iris running IRIX version 5, for GDB.

   Copyright (C) 1988-2014 Free Software Foundation, Inc.

   Contributed by Alessandro Forin(af@cs.cmu.edu) at CMU
   and by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin.
   Implemented for Irix 4.x by Garrett A. Wollman.
   Modified for Irix 5.x by Ian Lance Taylor.

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
#include "gdbcore.h"
#include "target.h"
#include "regcache.h"
#include "procfs.h"

#include <string.h>
#include <sys/time.h>
#include <sys/procfs.h>
#include <setjmp.h>		/* For JB_XXX.  */

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"
#include "mips-tdep.h"

static void fetch_core_registers (struct regcache *, char *,
				  unsigned int, int, CORE_ADDR);


/*
 * See the comment in m68k-tdep.c regarding the utility of these functions.
 *
 * These definitions are from the MIPS SVR4 ABI, so they may work for
 * any MIPS SVR4 target.
 */

void
supply_gregset (struct regcache *regcache, const gregset_t *gregsetp)
{
  int regi;
  const greg_t *regp = &(*gregsetp)[0];
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int gregoff = sizeof (greg_t) - mips_isa_regsize (gdbarch);
  static char zerobuf[32] = {0};

  for (regi = 0; regi <= CTX_RA; regi++)
    regcache_raw_supply (regcache, regi,
			 (const char *) (regp + regi) + gregoff);

  regcache_raw_supply (regcache, mips_regnum (gdbarch)->pc,
		       (const char *) (regp + CTX_EPC) + gregoff);
  regcache_raw_supply (regcache, mips_regnum (gdbarch)->hi,
		       (const char *) (regp + CTX_MDHI) + gregoff);
  regcache_raw_supply (regcache, mips_regnum (gdbarch)->lo,
		       (const char *) (regp + CTX_MDLO) + gregoff);
  regcache_raw_supply (regcache, mips_regnum (gdbarch)->cause,
		       (const char *) (regp + CTX_CAUSE) + gregoff);

  /* Fill inaccessible registers with zero.  */
  regcache_raw_supply (regcache, mips_regnum (gdbarch)->badvaddr, zerobuf);
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregsetp, int regno)
{
  int regi, size;
  greg_t *regp = &(*gregsetp)[0];
  gdb_byte buf[MAX_REGISTER_SIZE];
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Under Irix6, if GDB is built with N32 ABI and is debugging an O32
     executable, we have to sign extend the registers to 64 bits before
     filling in the gregset structure.  */

  for (regi = 0; regi <= CTX_RA; regi++)
    if ((regno == -1) || (regno == regi))
      {
	size = register_size (gdbarch, regi);
	regcache_raw_collect (regcache, regi, buf);
	*(regp + regi) = extract_signed_integer (buf, size, byte_order);
      }

  if ((regno == -1) || (regno == mips_regnum (gdbarch)->pc))
    {
      regi = mips_regnum (gdbarch)->pc;
      size = register_size (gdbarch, regi);
      regcache_raw_collect (regcache, regi, buf);
      *(regp + CTX_EPC) = extract_signed_integer (buf, size, byte_order);
    }

  if ((regno == -1) || (regno == mips_regnum (gdbarch)->cause))
    {
      regi = mips_regnum (gdbarch)->cause;
      size = register_size (gdbarch, regi);
      regcache_raw_collect (regcache, regi, buf);
      *(regp + CTX_CAUSE) = extract_signed_integer (buf, size, byte_order);
    }

  if ((regno == -1) || (regno == mips_regnum (gdbarch)->hi))
    {
      regi = mips_regnum (gdbarch)->hi;
      size = register_size (gdbarch, regi);
      regcache_raw_collect (regcache, regi, buf);
      *(regp + CTX_MDHI) = extract_signed_integer (buf, size, byte_order);
    }

  if ((regno == -1) || (regno == mips_regnum (gdbarch)->lo))
    {
      regi = mips_regnum (gdbarch)->lo;
      size = register_size (gdbarch, regi);
      regcache_raw_collect (regcache, regi, buf);
      *(regp + CTX_MDLO) = extract_signed_integer (buf, size, byte_order);
    }
}

/*
 * Now we do the same thing for floating-point registers.
 * We don't bother to condition on gdbarch_fp0_regnum since any
 * reasonable MIPS configuration has an R3010 in it.
 *
 * Again, see the comments in m68k-tdep.c.
 */

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregsetp)
{
  int regi;
  static char zerobuf[32] = {0};
  char fsrbuf[8];
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* FIXME, this is wrong for the N32 ABI which has 64 bit FP regs.  */

  for (regi = 0; regi < 32; regi++)
    regcache_raw_supply (regcache, gdbarch_fp0_regnum (gdbarch) + regi,
			 (const char *) &fpregsetp->__fp_r.__fp_regs[regi]);

  /* We can't supply the FSR register directly to the regcache,
     because there is a size issue: On one hand, fpregsetp->fp_csr
     is 32bits long, while the regcache expects a 64bits long value.
     So we use a buffer of the correct size and copy into it the register
     value at the proper location.  */
  memset (fsrbuf, 0, 4);
  memcpy (fsrbuf + 4, &fpregsetp->__fp_csr, 4);

  regcache_raw_supply (regcache,
		       mips_regnum (gdbarch)->fp_control_status, fsrbuf);

  /* FIXME: how can we supply FCRIR?  SGI doesn't tell us.  */
  regcache_raw_supply (regcache,
		       mips_regnum (gdbarch)->fp_implementation_revision,
		       zerobuf);
}

void
fill_fpregset (const struct regcache *regcache,
	       fpregset_t *fpregsetp, int regno)
{
  int regi;
  char *from, *to;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* FIXME, this is wrong for the N32 ABI which has 64 bit FP regs.  */

  for (regi = gdbarch_fp0_regnum (gdbarch);
       regi < gdbarch_fp0_regnum (gdbarch) + 32; regi++)
    {
      if ((regno == -1) || (regno == regi))
	{
	  const int fp0_regnum = gdbarch_fp0_regnum (gdbarch);

	  to = (char *) &(fpregsetp->__fp_r.__fp_regs[regi - fp0_regnum]);
          regcache_raw_collect (regcache, regi, to);
	}
    }

  if (regno == -1
      || regno == mips_regnum (gdbarch)->fp_control_status)
    {
      char fsrbuf[8];

      /* We can't fill the FSR register directly from the regcache,
         because there is a size issue: On one hand, fpregsetp->fp_csr
         is 32bits long, while the regcache expects a 64bits long buffer.
         So we use a buffer of the correct size and copy the register
         value from that buffer.  */
      regcache_raw_collect (regcache,
			    mips_regnum (gdbarch)->fp_control_status, fsrbuf);

      memcpy (&fpregsetp->__fp_csr, fsrbuf + 4, 4);
    }
}


/* Provide registers to GDB from a core file.

   CORE_REG_SECT points to an array of bytes, which were obtained from
   a core file which BFD thinks might contain register contents. 
   CORE_REG_SIZE is its size.

   Normally, WHICH says which register set corelow suspects this is:
     0 --- the general-purpose register set
     2 --- the floating-point register set
   However, for Irix 5, WHICH isn't used.

   REG_ADDR is also unused.  */

static void
fetch_core_registers (struct regcache *regcache,
		      char *core_reg_sect, unsigned core_reg_size,
		      int which, CORE_ADDR reg_addr)
{
  char *srcp = core_reg_sect;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int regsize = mips_isa_regsize (gdbarch);
  int regno;

  /* If regsize is 8, this is a N32 or N64 core file.
     If regsize is 4, this is an O32 core file.  */
  if (core_reg_size != regsize * gdbarch_num_regs (gdbarch))
    {
      warning (_("wrong size gregset struct in core file"));
      return;
    }

  for (regno = 0; regno < gdbarch_num_regs (gdbarch); regno++)
    {
      regcache_raw_supply (regcache, regno, srcp);
      srcp += regsize;
    }
}

/* Register that we are able to handle irix5 core file formats.
   This really is bfd_target_unknown_flavour.  */

static struct core_fns irix5_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_irix5_nat;

void
_initialize_irix5_nat (void)
{
  struct target_ops *t;

  t = procfs_target ();
  procfs_use_watchpoints (t);
  add_target (t);

  deprecated_add_core_fns (&irix5_core_fns);
}
