/* Common target dependent for AArch64 systems.

   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#include <sys/utsname.h>
#include <sys/uio.h>
#include "gdbsupport/common-defs.h"
#include "elf/external.h"
#include "elf/common.h"
#include "aarch64-sve-linux-ptrace.h"
#include "arch/aarch64.h"
#include "gdbsupport/common-regcache.h"
#include "gdbsupport/byte-vector.h"

/* See nat/aarch64-sve-linux-ptrace.h.  */

uint64_t
aarch64_sve_get_vq (int tid)
{
  struct iovec iovec;
  struct user_sve_header header;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  /* Ptrace gives the vector length in bytes.  Convert it to VQ, the number of
     128bit chunks in a Z register.  We use VQ because 128bits is the minimum
     a Z register can increase in size.  */

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    {
      /* SVE is not supported.  */
      return 0;
    }

  uint64_t vq = sve_vq_from_vl (header.vl);

  if (!sve_vl_valid (header.vl))
    {
      warning (_("Invalid SVE state from kernel; SVE disabled."));
      return 0;
    }

  return vq;
}

/* See nat/aarch64-sve-linux-ptrace.h.  */

bool
aarch64_sve_set_vq (int tid, uint64_t vq)
{
  struct iovec iovec;
  struct user_sve_header header;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    {
      /* SVE is not supported.  */
      return false;
    }

  header.vl = sve_vl_from_vq (vq);

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    {
      /* Vector length change failed.  */
      return false;
    }

  return true;
}

/* See nat/aarch64-sve-linux-ptrace.h.  */

bool
aarch64_sve_set_vq (int tid, struct reg_buffer_common *reg_buf)
{
  uint64_t reg_vg = 0;

  /* The VG register may not be valid if we've not collected any value yet.
     This can happen, for example,  if we're restoring the regcache after an
     inferior function call, and the VG register comes after the Z
     registers.  */
  if (reg_buf->get_register_status (AARCH64_SVE_VG_REGNUM) != REG_VALID)
  {
    /* If vg is not available yet, fetch it from ptrace.  The VG value from
       ptrace is likely the correct one.  */
    uint64_t vq = aarch64_sve_get_vq (tid);

    /* If something went wrong, just bail out.  */
    if (vq == 0)
      return false;

    reg_vg = sve_vg_from_vq (vq);
  }
  else
    reg_buf->raw_collect (AARCH64_SVE_VG_REGNUM, &reg_vg);

  return aarch64_sve_set_vq (tid, sve_vq_from_vg (reg_vg));
}

/* See nat/aarch64-sve-linux-ptrace.h.  */

std::unique_ptr<gdb_byte[]>
aarch64_sve_get_sveregs (int tid)
{
  struct iovec iovec;
  uint64_t vq = aarch64_sve_get_vq (tid);

  if (vq == 0)
    perror_with_name (_("Unable to fetch SVE register header"));

  /* A ptrace call with NT_ARM_SVE will return a header followed by either a
     dump of all the SVE and FP registers, or an fpsimd structure (identical to
     the one returned by NT_FPREGSET) if the kernel has not yet executed any
     SVE code.  Make sure we allocate enough space for a full SVE dump.  */

  iovec.iov_len = SVE_PT_SIZE (vq, SVE_PT_REGS_SVE);
  std::unique_ptr<gdb_byte[]> buf (new gdb_byte[iovec.iov_len]);
  iovec.iov_base = buf.get ();

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    perror_with_name (_("Unable to fetch SVE registers"));

  return buf;
}

/* See nat/aarch64-sve-linux-ptrace.h.  */

void
aarch64_sve_regs_copy_to_reg_buf (struct reg_buffer_common *reg_buf,
				  const void *buf)
{
  char *base = (char *) buf;
  struct user_sve_header *header = (struct user_sve_header *) buf;

  uint64_t vq = sve_vq_from_vl (header->vl);
  uint64_t vg = sve_vg_from_vl (header->vl);

  /* Sanity check the data in the header.  */
  if (!sve_vl_valid (header->vl)
      || SVE_PT_SIZE (vq, header->flags) != header->size)
    error (_("Invalid SVE header from kernel."));

  /* Update VG.  Note, the registers in the regcache will already be of the
     correct length.  */
  reg_buf->raw_supply (AARCH64_SVE_VG_REGNUM, &vg);

  if (HAS_SVE_STATE (*header))
    {
      /* The register dump contains a set of SVE registers.  */

      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_Z0_REGNUM + i,
			     base + SVE_PT_SVE_ZREG_OFFSET (vq, i));

      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_P0_REGNUM + i,
			     base + SVE_PT_SVE_PREG_OFFSET (vq, i));

      reg_buf->raw_supply (AARCH64_SVE_FFR_REGNUM,
			   base + SVE_PT_SVE_FFR_OFFSET (vq));
      reg_buf->raw_supply (AARCH64_FPSR_REGNUM,
			   base + SVE_PT_SVE_FPSR_OFFSET (vq));
      reg_buf->raw_supply (AARCH64_FPCR_REGNUM,
			   base + SVE_PT_SVE_FPCR_OFFSET (vq));
    }
  else
    {
      /* There is no SVE state yet - the register dump contains a fpsimd
	 structure instead.  These registers still exist in the hardware, but
	 the kernel has not yet initialised them, and so they will be null.  */

      char *zero_reg = (char *) alloca (SVE_PT_SVE_ZREG_SIZE (vq));
      struct user_fpsimd_state *fpsimd
	= (struct user_fpsimd_state *)(base + SVE_PT_FPSIMD_OFFSET);

      /* Copy across the V registers from fpsimd structure to the Z registers,
	 ensuring the non overlapping state is set to null.  */

      memset (zero_reg, 0, SVE_PT_SVE_ZREG_SIZE (vq));

      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	{
	  memcpy (zero_reg, &fpsimd->vregs[i], sizeof (__int128_t));
	  reg_buf->raw_supply (AARCH64_SVE_Z0_REGNUM + i, zero_reg);
	}

      reg_buf->raw_supply (AARCH64_FPSR_REGNUM, &fpsimd->fpsr);
      reg_buf->raw_supply (AARCH64_FPCR_REGNUM, &fpsimd->fpcr);

      /* Clear the SVE only registers.  */

      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_P0_REGNUM + i, zero_reg);

      reg_buf->raw_supply (AARCH64_SVE_FFR_REGNUM, zero_reg);
    }
}

/* See nat/aarch64-sve-linux-ptrace.h.  */

void
aarch64_sve_regs_copy_from_reg_buf (const struct reg_buffer_common *reg_buf,
				    void *buf)
{
  struct user_sve_header *header = (struct user_sve_header *) buf;
  char *base = (char *) buf;
  uint64_t vq = sve_vq_from_vl (header->vl);

  /* Sanity check the data in the header.  */
  if (!sve_vl_valid (header->vl)
      || SVE_PT_SIZE (vq, header->flags) != header->size)
    error (_("Invalid SVE header from kernel."));

  if (!HAS_SVE_STATE (*header))
    {
      /* There is no SVE state yet - the register dump contains a fpsimd
	 structure instead.  Where possible we want to write the reg_buf data
	 back to the kernel using the fpsimd structure.  However, if we cannot
	 then we'll need to reformat the fpsimd into a full SVE structure,
	 resulting in the initialization of SVE state written back to the
	 kernel, which is why we try to avoid it.  */

      bool has_sve_state = false;
      char *zero_reg = (char *) alloca (SVE_PT_SVE_ZREG_SIZE (vq));
      struct user_fpsimd_state *fpsimd
	= (struct user_fpsimd_state *)(base + SVE_PT_FPSIMD_OFFSET);

      memset (zero_reg, 0, SVE_PT_SVE_ZREG_SIZE (vq));

      /* Check in the reg_buf if any of the Z registers are set after the
	 first 128 bits, or if any of the other SVE registers are set.  */

      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	{
	  has_sve_state |= reg_buf->raw_compare (AARCH64_SVE_Z0_REGNUM + i,
						 zero_reg, sizeof (__int128_t));
	  if (has_sve_state)
	    break;
	}

      if (!has_sve_state)
	for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	  {
	    has_sve_state |= reg_buf->raw_compare (AARCH64_SVE_P0_REGNUM + i,
						   zero_reg, 0);
	    if (has_sve_state)
	      break;
	  }

      if (!has_sve_state)
	  has_sve_state |= reg_buf->raw_compare (AARCH64_SVE_FFR_REGNUM,
						 zero_reg, 0);

      /* If no SVE state exists, then use the existing fpsimd structure to
	 write out state and return.  */
      if (!has_sve_state)
	{
	  /* The collects of the Z registers will overflow the size of a vreg.
	     There is enough space in the structure to allow for this, but we
	     cannot overflow into the next register as we might not be
	     collecting every register.  */

	  for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	    {
	      if (REG_VALID
		  == reg_buf->get_register_status (AARCH64_SVE_Z0_REGNUM + i))
		{
		  reg_buf->raw_collect (AARCH64_SVE_Z0_REGNUM + i, zero_reg);
		  memcpy (&fpsimd->vregs[i], zero_reg, sizeof (__int128_t));
		}
	    }

	  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPSR_REGNUM))
	    reg_buf->raw_collect (AARCH64_FPSR_REGNUM, &fpsimd->fpsr);
	  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPCR_REGNUM))
	    reg_buf->raw_collect (AARCH64_FPCR_REGNUM, &fpsimd->fpcr);

	  return;
	}

      /* Otherwise, reformat the fpsimd structure into a full SVE set, by
	 expanding the V registers (working backwards so we don't splat
	 registers before they are copied) and using null for everything else.
	 Note that enough space for a full SVE dump was originally allocated
	 for base.  */

      header->flags |= SVE_PT_REGS_SVE;
      header->size = SVE_PT_SIZE (vq, SVE_PT_REGS_SVE);

      memcpy (base + SVE_PT_SVE_FPSR_OFFSET (vq), &fpsimd->fpsr,
	      sizeof (uint32_t));
      memcpy (base + SVE_PT_SVE_FPCR_OFFSET (vq), &fpsimd->fpcr,
	      sizeof (uint32_t));

      for (int i = AARCH64_SVE_Z_REGS_NUM; i >= 0 ; i--)
	{
	  memcpy (base + SVE_PT_SVE_ZREG_OFFSET (vq, i), &fpsimd->vregs[i],
		  sizeof (__int128_t));
	}
    }

  /* Replace the kernel values with those from reg_buf.  */

  for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
    if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_Z0_REGNUM + i))
      reg_buf->raw_collect (AARCH64_SVE_Z0_REGNUM + i,
			    base + SVE_PT_SVE_ZREG_OFFSET (vq, i));

  for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
    if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_P0_REGNUM + i))
      reg_buf->raw_collect (AARCH64_SVE_P0_REGNUM + i,
			    base + SVE_PT_SVE_PREG_OFFSET (vq, i));

  if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_FFR_REGNUM))
    reg_buf->raw_collect (AARCH64_SVE_FFR_REGNUM,
			  base + SVE_PT_SVE_FFR_OFFSET (vq));
  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPSR_REGNUM))
    reg_buf->raw_collect (AARCH64_FPSR_REGNUM,
			  base + SVE_PT_SVE_FPSR_OFFSET (vq));
  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPCR_REGNUM))
    reg_buf->raw_collect (AARCH64_FPCR_REGNUM,
			  base + SVE_PT_SVE_FPCR_OFFSET (vq));

}
