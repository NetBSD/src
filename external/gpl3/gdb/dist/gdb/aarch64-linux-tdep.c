/* Target-dependent code for GNU/Linux AArch64.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "gdbarch.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "aarch64-tdep.h"
#include "aarch64-linux-tdep.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "target/target.h"

#include "regcache.h"
#include "regset.h"

#include "stap-probe.h"
#include "parser-defs.h"
#include "user-regs.h"
#include "xml-syscall.h"
#include <ctype.h>

#include "record-full.h"
#include "linux-record.h"

/* Signal frame handling.

      +------------+  ^
      | saved lr   |  |
   +->| saved fp   |--+
   |  |            |
   |  |            |
   |  +------------+
   |  | saved lr   |
   +--| saved fp   |
   ^  |            |
   |  |            |
   |  +------------+
   ^  |            |
   |  | signal     |
   |  |            |        SIGTRAMP_FRAME (struct rt_sigframe)
   |  | saved regs |
   +--| saved sp   |--> interrupted_sp
   |  | saved pc   |--> interrupted_pc
   |  |            |
   |  +------------+
   |  | saved lr   |--> default_restorer (movz x8, NR_sys_rt_sigreturn; svc 0)
   +--| saved fp   |<- FP
      |            |         NORMAL_FRAME
      |            |<- SP
      +------------+

  On signal delivery, the kernel will create a signal handler stack
  frame and setup the return address in LR to point at restorer stub.
  The signal stack frame is defined by:

  struct rt_sigframe
  {
    siginfo_t info;
    struct ucontext uc;
  };

  The ucontext has the following form:
  struct ucontext
  {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    sigset_t uc_sigmask;
    struct sigcontext uc_mcontext;
  };

  struct sigcontext
  {
    unsigned long fault_address;
    unsigned long regs[31];
    unsigned long sp;		/ * 31 * /
    unsigned long pc;		/ * 32 * /
    unsigned long pstate;	/ * 33 * /
    __u8 __reserved[4096]
  };

  The reserved space in sigcontext contains additional structures, each starting
  with a aarch64_ctx, which specifies a unique identifier and the total size of
  the structure.  The final structure in reserved will start will a null
  aarch64_ctx.  The penultimate entry in reserved may be a extra_context which
  then points to a further block of reserved space.

  struct aarch64_ctx {
	u32 magic;
	u32 size;
  };

  The restorer stub will always have the form:

  d28015a8        movz    x8, #0xad
  d4000001        svc     #0x0

  This is a system call sys_rt_sigreturn.

  We detect signal frames by snooping the return code for the restorer
  instruction sequence.

  The handler then needs to recover the saved register set from
  ucontext.uc_mcontext.  */

/* These magic numbers need to reflect the layout of the kernel
   defined struct rt_sigframe and ucontext.  */
#define AARCH64_SIGCONTEXT_REG_SIZE             8
#define AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET     128
#define AARCH64_UCONTEXT_SIGCONTEXT_OFFSET      176
#define AARCH64_SIGCONTEXT_XO_OFFSET            8
#define AARCH64_SIGCONTEXT_RESERVED_OFFSET      288

#define AARCH64_SIGCONTEXT_RESERVED_SIZE	4096

/* Unique identifiers that may be used for aarch64_ctx.magic.  */
#define AARCH64_EXTRA_MAGIC			0x45585401
#define AARCH64_FPSIMD_MAGIC			0x46508001
#define AARCH64_SVE_MAGIC			0x53564501

/* Defines for the extra_context that follows an AARCH64_EXTRA_MAGIC.  */
#define AARCH64_EXTRA_DATAP_OFFSET		8

/* Defines for the fpsimd that follows an AARCH64_FPSIMD_MAGIC.  */
#define AARCH64_FPSIMD_FPSR_OFFSET		8
#define AARCH64_FPSIMD_FPCR_OFFSET		12
#define AARCH64_FPSIMD_V0_OFFSET		16
#define AARCH64_FPSIMD_VREG_SIZE		16

/* Defines for the sve structure that follows an AARCH64_SVE_MAGIC.  */
#define AARCH64_SVE_CONTEXT_VL_OFFSET		8
#define AARCH64_SVE_CONTEXT_REGS_OFFSET		16
#define AARCH64_SVE_CONTEXT_P_REGS_OFFSET(vq) (32 * vq * 16)
#define AARCH64_SVE_CONTEXT_FFR_OFFSET(vq) \
  (AARCH64_SVE_CONTEXT_P_REGS_OFFSET (vq) + (16 * vq * 2))
#define AARCH64_SVE_CONTEXT_SIZE(vq) \
  (AARCH64_SVE_CONTEXT_FFR_OFFSET (vq) + (vq * 2))


/* Read an aarch64_ctx, returning the magic value, and setting *SIZE to the
   size, or return 0 on error.  */

static uint32_t
read_aarch64_ctx (CORE_ADDR ctx_addr, enum bfd_endian byte_order,
		  uint32_t *size)
{
  uint32_t magic = 0;
  gdb_byte buf[4];

  if (target_read_memory (ctx_addr, buf, 4) != 0)
    return 0;
  magic = extract_unsigned_integer (buf, 4, byte_order);

  if (target_read_memory (ctx_addr + 4, buf, 4) != 0)
    return 0;
  *size = extract_unsigned_integer (buf, 4, byte_order);

  return magic;
}

/* Implement the "init" method of struct tramp_frame.  */

static void
aarch64_linux_sigframe_init (const struct tramp_frame *self,
			     struct frame_info *this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);
  CORE_ADDR sigcontext_addr = (sp + AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET
			       + AARCH64_UCONTEXT_SIGCONTEXT_OFFSET );
  CORE_ADDR section = sigcontext_addr + AARCH64_SIGCONTEXT_RESERVED_OFFSET;
  CORE_ADDR section_end = section + AARCH64_SIGCONTEXT_RESERVED_SIZE;
  CORE_ADDR fpsimd = 0;
  CORE_ADDR sve_regs = 0;
  uint32_t size, magic;
  bool extra_found = false;
  int num_regs = gdbarch_num_regs (gdbarch);

  /* Read in the integer registers.  */

  for (int i = 0; i < 31; i++)
    {
      trad_frame_set_reg_addr (this_cache,
			       AARCH64_X0_REGNUM + i,
			       sigcontext_addr + AARCH64_SIGCONTEXT_XO_OFFSET
				 + i * AARCH64_SIGCONTEXT_REG_SIZE);
    }
  trad_frame_set_reg_addr (this_cache, AARCH64_SP_REGNUM,
			   sigcontext_addr + AARCH64_SIGCONTEXT_XO_OFFSET
			     + 31 * AARCH64_SIGCONTEXT_REG_SIZE);
  trad_frame_set_reg_addr (this_cache, AARCH64_PC_REGNUM,
			   sigcontext_addr + AARCH64_SIGCONTEXT_XO_OFFSET
			     + 32 * AARCH64_SIGCONTEXT_REG_SIZE);

  /* Search for the FP and SVE sections, stopping at null.  */
  while ((magic = read_aarch64_ctx (section, byte_order, &size)) != 0
	 && size != 0)
    {
      switch (magic)
	{
	case AARCH64_FPSIMD_MAGIC:
	  fpsimd = section;
	  section += size;
	  break;

	case AARCH64_SVE_MAGIC:
	  {
	    /* Check if the section is followed by a full SVE dump, and set
	       sve_regs if it is.  */
	    gdb_byte buf[4];
	    uint16_t vq;

	    if (!tdep->has_sve ())
	      break;

	    if (target_read_memory (section + AARCH64_SVE_CONTEXT_VL_OFFSET,
				    buf, 2) != 0)
	      {
		section += size;
		break;
	      }
	    vq = sve_vq_from_vl (extract_unsigned_integer (buf, 2, byte_order));

	    if (vq != tdep->vq)
	      error (_("Invalid vector length in signal frame %d vs %s."), vq,
		     pulongest (tdep->vq));

	    if (size >= AARCH64_SVE_CONTEXT_SIZE (vq))
	      sve_regs = section + AARCH64_SVE_CONTEXT_REGS_OFFSET;

	    section += size;
	    break;
	  }

	case AARCH64_EXTRA_MAGIC:
	  {
	    /* Extra is always the last valid section in reserved and points to
	       an additional block of memory filled with more sections. Reset
	       the address to the extra section and continue looking for more
	       structures.  */
	    gdb_byte buf[8];

	    if (target_read_memory (section + AARCH64_EXTRA_DATAP_OFFSET,
				    buf, 8) != 0)
	      {
		section += size;
		break;
	      }

	    section = extract_unsigned_integer (buf, 8, byte_order);
	    extra_found = true;
	    break;
	  }

	default:
	  section += size;
	  break;
	}

      /* Prevent searching past the end of the reserved section.  The extra
	 section does not have a hard coded limit - we have to rely on it ending
	 with nulls.  */
      if (!extra_found && section > section_end)
	break;
    }

  if (sve_regs != 0)
    {
      CORE_ADDR offset;

      for (int i = 0; i < 32; i++)
	{
	  offset = sve_regs + (i * tdep->vq * 16);
	  trad_frame_set_reg_addr (this_cache, AARCH64_SVE_Z0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache,
				   num_regs + AARCH64_SVE_V0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_Q0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_D0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_S0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_H0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_B0_REGNUM + i,
				   offset);
	}

      offset = sve_regs + AARCH64_SVE_CONTEXT_P_REGS_OFFSET (tdep->vq);
      for (int i = 0; i < 16; i++)
	trad_frame_set_reg_addr (this_cache, AARCH64_SVE_P0_REGNUM + i,
				 offset + (i * tdep->vq * 2));

      offset = sve_regs + AARCH64_SVE_CONTEXT_FFR_OFFSET (tdep->vq);
      trad_frame_set_reg_addr (this_cache, AARCH64_SVE_FFR_REGNUM, offset);
    }

  if (fpsimd != 0)
    {
      trad_frame_set_reg_addr (this_cache, AARCH64_FPSR_REGNUM,
			       fpsimd + AARCH64_FPSIMD_FPSR_OFFSET);
      trad_frame_set_reg_addr (this_cache, AARCH64_FPCR_REGNUM,
			       fpsimd + AARCH64_FPSIMD_FPCR_OFFSET);

      /* If there was no SVE section then set up the V registers.  */
      if (sve_regs == 0)
	for (int i = 0; i < 32; i++)
	  {
	    CORE_ADDR offset = (fpsimd + AARCH64_FPSIMD_V0_OFFSET
				  + (i * AARCH64_FPSIMD_VREG_SIZE));

	    trad_frame_set_reg_addr (this_cache, AARCH64_V0_REGNUM + i, offset);
	    trad_frame_set_reg_addr (this_cache,
				     num_regs + AARCH64_Q0_REGNUM + i, offset);
	    trad_frame_set_reg_addr (this_cache,
				     num_regs + AARCH64_D0_REGNUM + i, offset);
	    trad_frame_set_reg_addr (this_cache,
				     num_regs + AARCH64_S0_REGNUM + i, offset);
	    trad_frame_set_reg_addr (this_cache,
				     num_regs + AARCH64_H0_REGNUM + i, offset);
	    trad_frame_set_reg_addr (this_cache,
				     num_regs + AARCH64_B0_REGNUM + i, offset);
	    if (tdep->has_sve ())
	      trad_frame_set_reg_addr (this_cache,
				       num_regs + AARCH64_SVE_V0_REGNUM + i,
				       offset);
	  }
    }

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame aarch64_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    /* movz x8, 0x8b (S=1,o=10,h=0,i=0x8b,r=8)
       Soo1 0010 1hhi iiii iiii iiii iiir rrrr  */
    {0xd2801168, ULONGEST_MAX},

    /* svc  0x0      (o=0, l=1)
       1101 0100 oooi iiii iiii iiii iii0 00ll  */
    {0xd4000001, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  aarch64_linux_sigframe_init
};

/* Register maps.  */

static const struct regcache_map_entry aarch64_linux_gregmap[] =
  {
    { 31, AARCH64_X0_REGNUM, 8 }, /* x0 ... x30 */
    { 1, AARCH64_SP_REGNUM, 8 },
    { 1, AARCH64_PC_REGNUM, 8 },
    { 1, AARCH64_CPSR_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry aarch64_linux_fpregmap[] =
  {
    { 32, AARCH64_V0_REGNUM, 16 }, /* v0 ... v31 */
    { 1, AARCH64_FPSR_REGNUM, 4 },
    { 1, AARCH64_FPCR_REGNUM, 4 },
    { 0 }
  };

/* Register set definitions.  */

const struct regset aarch64_linux_gregset =
  {
    aarch64_linux_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset aarch64_linux_fpregset =
  {
    aarch64_linux_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* The fields in an SVE header at the start of a SVE regset.  */

#define SVE_HEADER_SIZE_LENGTH		4
#define SVE_HEADER_MAX_SIZE_LENGTH	4
#define SVE_HEADER_VL_LENGTH		2
#define SVE_HEADER_MAX_VL_LENGTH	2
#define SVE_HEADER_FLAGS_LENGTH		2
#define SVE_HEADER_RESERVED_LENGTH	2

#define SVE_HEADER_SIZE_OFFSET		0
#define SVE_HEADER_MAX_SIZE_OFFSET	\
  (SVE_HEADER_SIZE_OFFSET + SVE_HEADER_SIZE_LENGTH)
#define SVE_HEADER_VL_OFFSET		\
  (SVE_HEADER_MAX_SIZE_OFFSET + SVE_HEADER_MAX_SIZE_LENGTH)
#define SVE_HEADER_MAX_VL_OFFSET	\
  (SVE_HEADER_VL_OFFSET + SVE_HEADER_VL_LENGTH)
#define SVE_HEADER_FLAGS_OFFSET		\
  (SVE_HEADER_MAX_VL_OFFSET + SVE_HEADER_MAX_VL_LENGTH)
#define SVE_HEADER_RESERVED_OFFSET	\
  (SVE_HEADER_FLAGS_OFFSET + SVE_HEADER_FLAGS_LENGTH)
#define SVE_HEADER_SIZE			\
  (SVE_HEADER_RESERVED_OFFSET + SVE_HEADER_RESERVED_LENGTH)

#define SVE_HEADER_FLAG_SVE		1

/* Get VQ value from SVE section in the core dump.  */

static uint64_t
aarch64_linux_core_read_vq (struct gdbarch *gdbarch, bfd *abfd)
{
  gdb_byte header[SVE_HEADER_SIZE];
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  asection *sve_section = bfd_get_section_by_name (abfd, ".reg-aarch-sve");

  if (sve_section == nullptr)
    {
      /* No SVE state.  */
      return 0;
    }

  size_t size = bfd_section_size (sve_section);

  /* Check extended state size.  */
  if (size < SVE_HEADER_SIZE)
    {
      warning (_("'.reg-aarch-sve' section in core file too small."));
      return 0;
    }

  if (!bfd_get_section_contents (abfd, sve_section, header, 0, SVE_HEADER_SIZE))
    {
      warning (_("Couldn't read sve header from "
		 "'.reg-aarch-sve' section in core file."));
      return 0;
    }

  uint64_t vl = extract_unsigned_integer (header + SVE_HEADER_VL_OFFSET,
					  SVE_HEADER_VL_LENGTH, byte_order);
  uint64_t vq = sve_vq_from_vl (vl);

  if (vq > AARCH64_MAX_SVE_VQ)
    {
      warning (_("SVE Vector length in core file not supported by this version"
		 " of GDB.  (VQ=%s)"), pulongest (vq));
      return 0;
    }
  else if (vq == 0)
    {
      warning (_("SVE Vector length in core file is invalid. (VQ=%s"),
	       pulongest (vq));
      return 0;
    }

  return vq;
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the registers to "unavailable" status.  */

static void
aarch64_linux_supply_sve_regset (const struct regset *regset,
				 struct regcache *regcache,
				 int regnum, const void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (buf == nullptr)
    return regcache->supply_regset (regset, regnum, nullptr, size);
  gdb_assert (size > SVE_HEADER_SIZE);

  /* BUF contains an SVE header followed by a register dump of either the
     passed in SVE regset or a NEON fpregset.  */

  /* Extract required fields from the header.  */
  ULONGEST vl = extract_unsigned_integer (header + SVE_HEADER_VL_OFFSET,
					  SVE_HEADER_VL_LENGTH, byte_order);
  uint16_t flags = extract_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
					     SVE_HEADER_FLAGS_LENGTH,
					     byte_order);

  if (regnum == -1 || regnum == AARCH64_SVE_VG_REGNUM)
    {
      gdb_byte vg_target[8];
      store_integer ((gdb_byte *)&vg_target, sizeof (uint64_t), byte_order,
		     sve_vg_from_vl (vl));
      regcache->raw_supply (AARCH64_SVE_VG_REGNUM, &vg_target);
    }

  if (flags & SVE_HEADER_FLAG_SVE)
    {
      /* Register dump is a SVE structure.  */
      regcache->supply_regset (regset, regnum,
			       (gdb_byte *) buf + SVE_HEADER_SIZE,
			       size - SVE_HEADER_SIZE);
    }
  else
    {
      /* Register dump is a fpsimd structure.  First clear the SVE
	 registers.  */
      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	regcache->raw_supply_zeroed (AARCH64_SVE_Z0_REGNUM + i);
      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	regcache->raw_supply_zeroed (AARCH64_SVE_P0_REGNUM + i);
      regcache->raw_supply_zeroed (AARCH64_SVE_FFR_REGNUM);

      /* Then supply the fpsimd registers.  */
      regcache->supply_regset (&aarch64_linux_fpregset, regnum,
			       (gdb_byte *) buf + SVE_HEADER_SIZE,
			       size - SVE_HEADER_SIZE);
    }
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
aarch64_linux_collect_sve_regset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  uint64_t vq = gdbarch_tdep (gdbarch)->vq;

  gdb_assert (buf != NULL);
  gdb_assert (size > SVE_HEADER_SIZE);

  /* BUF starts with a SVE header prior to the register dump.  */

  store_unsigned_integer (header + SVE_HEADER_SIZE_OFFSET,
			  SVE_HEADER_SIZE_LENGTH, byte_order, size);
  store_unsigned_integer (header + SVE_HEADER_MAX_SIZE_OFFSET,
			  SVE_HEADER_MAX_SIZE_LENGTH, byte_order, size);
  store_unsigned_integer (header + SVE_HEADER_VL_OFFSET, SVE_HEADER_VL_LENGTH,
			  byte_order, sve_vl_from_vq (vq));
  store_unsigned_integer (header + SVE_HEADER_MAX_VL_OFFSET,
			  SVE_HEADER_MAX_VL_LENGTH, byte_order,
			  sve_vl_from_vq (vq));
  store_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
			  SVE_HEADER_FLAGS_LENGTH, byte_order,
			  SVE_HEADER_FLAG_SVE);
  store_unsigned_integer (header + SVE_HEADER_RESERVED_OFFSET,
			  SVE_HEADER_RESERVED_LENGTH, byte_order, 0);

  /* The SVE register dump follows.  */
  regcache->collect_regset (regset, regnum, (gdb_byte *) buf + SVE_HEADER_SIZE,
			    size - SVE_HEADER_SIZE);
}

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
aarch64_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					    iterate_over_regset_sections_cb *cb,
					    void *cb_data,
					    const struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  cb (".reg", AARCH64_LINUX_SIZEOF_GREGSET, AARCH64_LINUX_SIZEOF_GREGSET,
      &aarch64_linux_gregset, NULL, cb_data);

  if (tdep->has_sve ())
    {
      /* Create this on the fly in order to handle vector register sizes.  */
      const struct regcache_map_entry sve_regmap[] =
	{
	  { 32, AARCH64_SVE_Z0_REGNUM, (int) (tdep->vq * 16) },
	  { 16, AARCH64_SVE_P0_REGNUM, (int) (tdep->vq * 16 / 8) },
	  { 1, AARCH64_SVE_FFR_REGNUM, 4 },
	  { 1, AARCH64_FPSR_REGNUM, 4 },
	  { 1, AARCH64_FPCR_REGNUM, 4 },
	  { 0 }
	};

      const struct regset aarch64_linux_sve_regset =
	{
	  sve_regmap,
	  aarch64_linux_supply_sve_regset, aarch64_linux_collect_sve_regset,
	  REGSET_VARIABLE_SIZE
	};

      cb (".reg-aarch-sve",
	  SVE_HEADER_SIZE + regcache_map_entry_size (aarch64_linux_fpregmap),
	  SVE_HEADER_SIZE + regcache_map_entry_size (sve_regmap),
	  &aarch64_linux_sve_regset, "SVE registers", cb_data);
    }
  else
    cb (".reg2", AARCH64_LINUX_SIZEOF_FPREGSET, AARCH64_LINUX_SIZEOF_FPREGSET,
	&aarch64_linux_fpregset, NULL, cb_data);


  if (tdep->has_pauth ())
    {
      /* Create this on the fly in order to handle the variable location.  */
      const struct regcache_map_entry pauth_regmap[] =
	{
	  { 2, AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base), 8},
	  { 0 }
	};

      const struct regset aarch64_linux_pauth_regset =
	{
	  pauth_regmap, regcache_supply_regset, regcache_collect_regset
	};

      cb (".reg-aarch-pauth", AARCH64_LINUX_SIZEOF_PAUTH,
	  AARCH64_LINUX_SIZEOF_PAUTH, &aarch64_linux_pauth_regset,
	  "pauth registers", cb_data);
    }
}

/* Implement the "core_read_description" gdbarch method.  */

static const struct target_desc *
aarch64_linux_core_read_description (struct gdbarch *gdbarch,
				     struct target_ops *target, bfd *abfd)
{
  CORE_ADDR hwcap = linux_get_hwcap (target);

  return aarch64_read_description (aarch64_linux_core_read_vq (gdbarch, abfd),
				   hwcap & AARCH64_HWCAP_PACA);
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
aarch64_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '#' || isdigit (*s) /* Literal number.  */
	  || *s == '[' /* Register indirection.  */
	  || isalpha (*s)); /* Register value.  */
}

/* This routine is used to parse a special token in AArch64's assembly.

   The special tokens parsed by it are:

      - Register displacement (e.g, [fp, #-8])

   It returns one if the special token has been parsed successfully,
   or zero if the current token is not considered special.  */

static int
aarch64_stap_parse_special_token (struct gdbarch *gdbarch,
				  struct stap_parse_info *p)
{
  if (*p->arg == '[')
    {
      /* Temporary holder for lookahead.  */
      const char *tmp = p->arg;
      char *endp;
      /* Used to save the register name.  */
      const char *start;
      char *regname;
      int len;
      int got_minus = 0;
      long displacement;
      struct stoken str;

      ++tmp;
      start = tmp;

      /* Register name.  */
      while (isalnum (*tmp))
	++tmp;

      if (*tmp != ',')
	return 0;

      len = tmp - start;
      regname = (char *) alloca (len + 2);

      strncpy (regname, start, len);
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      ++tmp;
      tmp = skip_spaces (tmp);
      /* Now we expect a number.  It can begin with '#' or simply
	 a digit.  */
      if (*tmp == '#')
	++tmp;

      if (*tmp == '-')
	{
	  ++tmp;
	  got_minus = 1;
	}
      else if (*tmp == '+')
	++tmp;

      if (!isdigit (*tmp))
	return 0;

      displacement = strtol (tmp, &endp, 10);
      tmp = endp;

      /* Skipping last `]'.  */
      if (*tmp++ != ']')
	return 0;

      /* The displacement.  */
      write_exp_elt_opcode (&p->pstate, OP_LONG);
      write_exp_elt_type (&p->pstate, builtin_type (gdbarch)->builtin_long);
      write_exp_elt_longcst (&p->pstate, displacement);
      write_exp_elt_opcode (&p->pstate, OP_LONG);
      if (got_minus)
	write_exp_elt_opcode (&p->pstate, UNOP_NEG);

      /* The register name.  */
      write_exp_elt_opcode (&p->pstate, OP_REGISTER);
      str.ptr = regname;
      str.length = len;
      write_exp_string (&p->pstate, str);
      write_exp_elt_opcode (&p->pstate, OP_REGISTER);

      write_exp_elt_opcode (&p->pstate, BINOP_ADD);

      /* Casting to the expected type.  */
      write_exp_elt_opcode (&p->pstate, UNOP_CAST);
      write_exp_elt_type (&p->pstate, lookup_pointer_type (p->arg_type));
      write_exp_elt_opcode (&p->pstate, UNOP_CAST);

      write_exp_elt_opcode (&p->pstate, UNOP_IND);

      p->arg = tmp;
    }
  else
    return 0;

  return 1;
}

/* AArch64 process record-replay constructs: syscall, signal etc.  */

struct linux_record_tdep aarch64_linux_record_tdep;

/* Enum that defines the AArch64 linux specific syscall identifiers used for
   process record/replay.  */

enum aarch64_syscall {
  aarch64_sys_io_setup = 0,
  aarch64_sys_io_destroy = 1,
  aarch64_sys_io_submit = 2,
  aarch64_sys_io_cancel = 3,
  aarch64_sys_io_getevents = 4,
  aarch64_sys_setxattr = 5,
  aarch64_sys_lsetxattr = 6,
  aarch64_sys_fsetxattr = 7,
  aarch64_sys_getxattr = 8,
  aarch64_sys_lgetxattr = 9,
  aarch64_sys_fgetxattr = 10,
  aarch64_sys_listxattr = 11,
  aarch64_sys_llistxattr = 12,
  aarch64_sys_flistxattr = 13,
  aarch64_sys_removexattr = 14,
  aarch64_sys_lremovexattr = 15,
  aarch64_sys_fremovexattr = 16,
  aarch64_sys_getcwd = 17,
  aarch64_sys_lookup_dcookie = 18,
  aarch64_sys_eventfd2 = 19,
  aarch64_sys_epoll_create1 = 20,
  aarch64_sys_epoll_ctl = 21,
  aarch64_sys_epoll_pwait = 22,
  aarch64_sys_dup = 23,
  aarch64_sys_dup3 = 24,
  aarch64_sys_fcntl = 25,
  aarch64_sys_inotify_init1 = 26,
  aarch64_sys_inotify_add_watch = 27,
  aarch64_sys_inotify_rm_watch = 28,
  aarch64_sys_ioctl = 29,
  aarch64_sys_ioprio_set = 30,
  aarch64_sys_ioprio_get = 31,
  aarch64_sys_flock = 32,
  aarch64_sys_mknodat = 33,
  aarch64_sys_mkdirat = 34,
  aarch64_sys_unlinkat = 35,
  aarch64_sys_symlinkat = 36,
  aarch64_sys_linkat = 37,
  aarch64_sys_renameat = 38,
  aarch64_sys_umount2 = 39,
  aarch64_sys_mount = 40,
  aarch64_sys_pivot_root = 41,
  aarch64_sys_nfsservctl = 42,
  aarch64_sys_statfs = 43,
  aarch64_sys_fstatfs = 44,
  aarch64_sys_truncate = 45,
  aarch64_sys_ftruncate = 46,
  aarch64_sys_fallocate = 47,
  aarch64_sys_faccessat = 48,
  aarch64_sys_chdir = 49,
  aarch64_sys_fchdir = 50,
  aarch64_sys_chroot = 51,
  aarch64_sys_fchmod = 52,
  aarch64_sys_fchmodat = 53,
  aarch64_sys_fchownat = 54,
  aarch64_sys_fchown = 55,
  aarch64_sys_openat = 56,
  aarch64_sys_close = 57,
  aarch64_sys_vhangup = 58,
  aarch64_sys_pipe2 = 59,
  aarch64_sys_quotactl = 60,
  aarch64_sys_getdents64 = 61,
  aarch64_sys_lseek = 62,
  aarch64_sys_read = 63,
  aarch64_sys_write = 64,
  aarch64_sys_readv = 65,
  aarch64_sys_writev = 66,
  aarch64_sys_pread64 = 67,
  aarch64_sys_pwrite64 = 68,
  aarch64_sys_preadv = 69,
  aarch64_sys_pwritev = 70,
  aarch64_sys_sendfile = 71,
  aarch64_sys_pselect6 = 72,
  aarch64_sys_ppoll = 73,
  aarch64_sys_signalfd4 = 74,
  aarch64_sys_vmsplice = 75,
  aarch64_sys_splice = 76,
  aarch64_sys_tee = 77,
  aarch64_sys_readlinkat = 78,
  aarch64_sys_newfstatat = 79,
  aarch64_sys_fstat = 80,
  aarch64_sys_sync = 81,
  aarch64_sys_fsync = 82,
  aarch64_sys_fdatasync = 83,
  aarch64_sys_sync_file_range2 = 84,
  aarch64_sys_sync_file_range = 84,
  aarch64_sys_timerfd_create = 85,
  aarch64_sys_timerfd_settime = 86,
  aarch64_sys_timerfd_gettime = 87,
  aarch64_sys_utimensat = 88,
  aarch64_sys_acct = 89,
  aarch64_sys_capget = 90,
  aarch64_sys_capset = 91,
  aarch64_sys_personality = 92,
  aarch64_sys_exit = 93,
  aarch64_sys_exit_group = 94,
  aarch64_sys_waitid = 95,
  aarch64_sys_set_tid_address = 96,
  aarch64_sys_unshare = 97,
  aarch64_sys_futex = 98,
  aarch64_sys_set_robust_list = 99,
  aarch64_sys_get_robust_list = 100,
  aarch64_sys_nanosleep = 101,
  aarch64_sys_getitimer = 102,
  aarch64_sys_setitimer = 103,
  aarch64_sys_kexec_load = 104,
  aarch64_sys_init_module = 105,
  aarch64_sys_delete_module = 106,
  aarch64_sys_timer_create = 107,
  aarch64_sys_timer_gettime = 108,
  aarch64_sys_timer_getoverrun = 109,
  aarch64_sys_timer_settime = 110,
  aarch64_sys_timer_delete = 111,
  aarch64_sys_clock_settime = 112,
  aarch64_sys_clock_gettime = 113,
  aarch64_sys_clock_getres = 114,
  aarch64_sys_clock_nanosleep = 115,
  aarch64_sys_syslog = 116,
  aarch64_sys_ptrace = 117,
  aarch64_sys_sched_setparam = 118,
  aarch64_sys_sched_setscheduler = 119,
  aarch64_sys_sched_getscheduler = 120,
  aarch64_sys_sched_getparam = 121,
  aarch64_sys_sched_setaffinity = 122,
  aarch64_sys_sched_getaffinity = 123,
  aarch64_sys_sched_yield = 124,
  aarch64_sys_sched_get_priority_max = 125,
  aarch64_sys_sched_get_priority_min = 126,
  aarch64_sys_sched_rr_get_interval = 127,
  aarch64_sys_kill = 129,
  aarch64_sys_tkill = 130,
  aarch64_sys_tgkill = 131,
  aarch64_sys_sigaltstack = 132,
  aarch64_sys_rt_sigsuspend = 133,
  aarch64_sys_rt_sigaction = 134,
  aarch64_sys_rt_sigprocmask = 135,
  aarch64_sys_rt_sigpending = 136,
  aarch64_sys_rt_sigtimedwait = 137,
  aarch64_sys_rt_sigqueueinfo = 138,
  aarch64_sys_rt_sigreturn = 139,
  aarch64_sys_setpriority = 140,
  aarch64_sys_getpriority = 141,
  aarch64_sys_reboot = 142,
  aarch64_sys_setregid = 143,
  aarch64_sys_setgid = 144,
  aarch64_sys_setreuid = 145,
  aarch64_sys_setuid = 146,
  aarch64_sys_setresuid = 147,
  aarch64_sys_getresuid = 148,
  aarch64_sys_setresgid = 149,
  aarch64_sys_getresgid = 150,
  aarch64_sys_setfsuid = 151,
  aarch64_sys_setfsgid = 152,
  aarch64_sys_times = 153,
  aarch64_sys_setpgid = 154,
  aarch64_sys_getpgid = 155,
  aarch64_sys_getsid = 156,
  aarch64_sys_setsid = 157,
  aarch64_sys_getgroups = 158,
  aarch64_sys_setgroups = 159,
  aarch64_sys_uname = 160,
  aarch64_sys_sethostname = 161,
  aarch64_sys_setdomainname = 162,
  aarch64_sys_getrlimit = 163,
  aarch64_sys_setrlimit = 164,
  aarch64_sys_getrusage = 165,
  aarch64_sys_umask = 166,
  aarch64_sys_prctl = 167,
  aarch64_sys_getcpu = 168,
  aarch64_sys_gettimeofday = 169,
  aarch64_sys_settimeofday = 170,
  aarch64_sys_adjtimex = 171,
  aarch64_sys_getpid = 172,
  aarch64_sys_getppid = 173,
  aarch64_sys_getuid = 174,
  aarch64_sys_geteuid = 175,
  aarch64_sys_getgid = 176,
  aarch64_sys_getegid = 177,
  aarch64_sys_gettid = 178,
  aarch64_sys_sysinfo = 179,
  aarch64_sys_mq_open = 180,
  aarch64_sys_mq_unlink = 181,
  aarch64_sys_mq_timedsend = 182,
  aarch64_sys_mq_timedreceive = 183,
  aarch64_sys_mq_notify = 184,
  aarch64_sys_mq_getsetattr = 185,
  aarch64_sys_msgget = 186,
  aarch64_sys_msgctl = 187,
  aarch64_sys_msgrcv = 188,
  aarch64_sys_msgsnd = 189,
  aarch64_sys_semget = 190,
  aarch64_sys_semctl = 191,
  aarch64_sys_semtimedop = 192,
  aarch64_sys_semop = 193,
  aarch64_sys_shmget = 194,
  aarch64_sys_shmctl = 195,
  aarch64_sys_shmat = 196,
  aarch64_sys_shmdt = 197,
  aarch64_sys_socket = 198,
  aarch64_sys_socketpair = 199,
  aarch64_sys_bind = 200,
  aarch64_sys_listen = 201,
  aarch64_sys_accept = 202,
  aarch64_sys_connect = 203,
  aarch64_sys_getsockname = 204,
  aarch64_sys_getpeername = 205,
  aarch64_sys_sendto = 206,
  aarch64_sys_recvfrom = 207,
  aarch64_sys_setsockopt = 208,
  aarch64_sys_getsockopt = 209,
  aarch64_sys_shutdown = 210,
  aarch64_sys_sendmsg = 211,
  aarch64_sys_recvmsg = 212,
  aarch64_sys_readahead = 213,
  aarch64_sys_brk = 214,
  aarch64_sys_munmap = 215,
  aarch64_sys_mremap = 216,
  aarch64_sys_add_key = 217,
  aarch64_sys_request_key = 218,
  aarch64_sys_keyctl = 219,
  aarch64_sys_clone = 220,
  aarch64_sys_execve = 221,
  aarch64_sys_mmap = 222,
  aarch64_sys_fadvise64 = 223,
  aarch64_sys_swapon = 224,
  aarch64_sys_swapoff = 225,
  aarch64_sys_mprotect = 226,
  aarch64_sys_msync = 227,
  aarch64_sys_mlock = 228,
  aarch64_sys_munlock = 229,
  aarch64_sys_mlockall = 230,
  aarch64_sys_munlockall = 231,
  aarch64_sys_mincore = 232,
  aarch64_sys_madvise = 233,
  aarch64_sys_remap_file_pages = 234,
  aarch64_sys_mbind = 235,
  aarch64_sys_get_mempolicy = 236,
  aarch64_sys_set_mempolicy = 237,
  aarch64_sys_migrate_pages = 238,
  aarch64_sys_move_pages = 239,
  aarch64_sys_rt_tgsigqueueinfo = 240,
  aarch64_sys_perf_event_open = 241,
  aarch64_sys_accept4 = 242,
  aarch64_sys_recvmmsg = 243,
  aarch64_sys_wait4 = 260,
  aarch64_sys_prlimit64 = 261,
  aarch64_sys_fanotify_init = 262,
  aarch64_sys_fanotify_mark = 263,
  aarch64_sys_name_to_handle_at = 264,
  aarch64_sys_open_by_handle_at = 265,
  aarch64_sys_clock_adjtime = 266,
  aarch64_sys_syncfs = 267,
  aarch64_sys_setns = 268,
  aarch64_sys_sendmmsg = 269,
  aarch64_sys_process_vm_readv = 270,
  aarch64_sys_process_vm_writev = 271,
  aarch64_sys_kcmp = 272,
  aarch64_sys_finit_module = 273,
  aarch64_sys_sched_setattr = 274,
  aarch64_sys_sched_getattr = 275,
};

/* aarch64_canonicalize_syscall maps syscall ids from the native AArch64
   linux set of syscall ids into a canonical set of syscall ids used by
   process record.  */

static enum gdb_syscall
aarch64_canonicalize_syscall (enum aarch64_syscall syscall_number)
{
#define SYSCALL_MAP(SYSCALL) case aarch64_sys_##SYSCALL: \
  return gdb_sys_##SYSCALL

#define UNSUPPORTED_SYSCALL_MAP(SYSCALL) case aarch64_sys_##SYSCALL: \
  return gdb_sys_no_syscall

  switch (syscall_number)
    {
      SYSCALL_MAP (io_setup);
      SYSCALL_MAP (io_destroy);
      SYSCALL_MAP (io_submit);
      SYSCALL_MAP (io_cancel);
      SYSCALL_MAP (io_getevents);

      SYSCALL_MAP (setxattr);
      SYSCALL_MAP (lsetxattr);
      SYSCALL_MAP (fsetxattr);
      SYSCALL_MAP (getxattr);
      SYSCALL_MAP (lgetxattr);
      SYSCALL_MAP (fgetxattr);
      SYSCALL_MAP (listxattr);
      SYSCALL_MAP (llistxattr);
      SYSCALL_MAP (flistxattr);
      SYSCALL_MAP (removexattr);
      SYSCALL_MAP (lremovexattr);
      SYSCALL_MAP (fremovexattr);
      SYSCALL_MAP (getcwd);
      SYSCALL_MAP (lookup_dcookie);
      SYSCALL_MAP (eventfd2);
      SYSCALL_MAP (epoll_create1);
      SYSCALL_MAP (epoll_ctl);
      SYSCALL_MAP (epoll_pwait);
      SYSCALL_MAP (dup);
      SYSCALL_MAP (dup3);
      SYSCALL_MAP (fcntl);
      SYSCALL_MAP (inotify_init1);
      SYSCALL_MAP (inotify_add_watch);
      SYSCALL_MAP (inotify_rm_watch);
      SYSCALL_MAP (ioctl);
      SYSCALL_MAP (ioprio_set);
      SYSCALL_MAP (ioprio_get);
      SYSCALL_MAP (flock);
      SYSCALL_MAP (mknodat);
      SYSCALL_MAP (mkdirat);
      SYSCALL_MAP (unlinkat);
      SYSCALL_MAP (symlinkat);
      SYSCALL_MAP (linkat);
      SYSCALL_MAP (renameat);
      UNSUPPORTED_SYSCALL_MAP (umount2);
      SYSCALL_MAP (mount);
      SYSCALL_MAP (pivot_root);
      SYSCALL_MAP (nfsservctl);
      SYSCALL_MAP (statfs);
      SYSCALL_MAP (truncate);
      SYSCALL_MAP (ftruncate);
      SYSCALL_MAP (fallocate);
      SYSCALL_MAP (faccessat);
      SYSCALL_MAP (fchdir);
      SYSCALL_MAP (chroot);
      SYSCALL_MAP (fchmod);
      SYSCALL_MAP (fchmodat);
      SYSCALL_MAP (fchownat);
      SYSCALL_MAP (fchown);
      SYSCALL_MAP (openat);
      SYSCALL_MAP (close);
      SYSCALL_MAP (vhangup);
      SYSCALL_MAP (pipe2);
      SYSCALL_MAP (quotactl);
      SYSCALL_MAP (getdents64);
      SYSCALL_MAP (lseek);
      SYSCALL_MAP (read);
      SYSCALL_MAP (write);
      SYSCALL_MAP (readv);
      SYSCALL_MAP (writev);
      SYSCALL_MAP (pread64);
      SYSCALL_MAP (pwrite64);
      UNSUPPORTED_SYSCALL_MAP (preadv);
      UNSUPPORTED_SYSCALL_MAP (pwritev);
      SYSCALL_MAP (sendfile);
      SYSCALL_MAP (pselect6);
      SYSCALL_MAP (ppoll);
      UNSUPPORTED_SYSCALL_MAP (signalfd4);
      SYSCALL_MAP (vmsplice);
      SYSCALL_MAP (splice);
      SYSCALL_MAP (tee);
      SYSCALL_MAP (readlinkat);
      SYSCALL_MAP (newfstatat);

      SYSCALL_MAP (fstat);
      SYSCALL_MAP (sync);
      SYSCALL_MAP (fsync);
      SYSCALL_MAP (fdatasync);
      SYSCALL_MAP (sync_file_range);
      UNSUPPORTED_SYSCALL_MAP (timerfd_create);
      UNSUPPORTED_SYSCALL_MAP (timerfd_settime);
      UNSUPPORTED_SYSCALL_MAP (timerfd_gettime);
      UNSUPPORTED_SYSCALL_MAP (utimensat);
      SYSCALL_MAP (acct);
      SYSCALL_MAP (capget);
      SYSCALL_MAP (capset);
      SYSCALL_MAP (personality);
      SYSCALL_MAP (exit);
      SYSCALL_MAP (exit_group);
      SYSCALL_MAP (waitid);
      SYSCALL_MAP (set_tid_address);
      SYSCALL_MAP (unshare);
      SYSCALL_MAP (futex);
      SYSCALL_MAP (set_robust_list);
      SYSCALL_MAP (get_robust_list);
      SYSCALL_MAP (nanosleep);

      SYSCALL_MAP (getitimer);
      SYSCALL_MAP (setitimer);
      SYSCALL_MAP (kexec_load);
      SYSCALL_MAP (init_module);
      SYSCALL_MAP (delete_module);
      SYSCALL_MAP (timer_create);
      SYSCALL_MAP (timer_settime);
      SYSCALL_MAP (timer_gettime);
      SYSCALL_MAP (timer_getoverrun);
      SYSCALL_MAP (timer_delete);
      SYSCALL_MAP (clock_settime);
      SYSCALL_MAP (clock_gettime);
      SYSCALL_MAP (clock_getres);
      SYSCALL_MAP (clock_nanosleep);
      SYSCALL_MAP (syslog);
      SYSCALL_MAP (ptrace);
      SYSCALL_MAP (sched_setparam);
      SYSCALL_MAP (sched_setscheduler);
      SYSCALL_MAP (sched_getscheduler);
      SYSCALL_MAP (sched_getparam);
      SYSCALL_MAP (sched_setaffinity);
      SYSCALL_MAP (sched_getaffinity);
      SYSCALL_MAP (sched_yield);
      SYSCALL_MAP (sched_get_priority_max);
      SYSCALL_MAP (sched_get_priority_min);
      SYSCALL_MAP (sched_rr_get_interval);
      SYSCALL_MAP (kill);
      SYSCALL_MAP (tkill);
      SYSCALL_MAP (tgkill);
      SYSCALL_MAP (sigaltstack);
      SYSCALL_MAP (rt_sigsuspend);
      SYSCALL_MAP (rt_sigaction);
      SYSCALL_MAP (rt_sigprocmask);
      SYSCALL_MAP (rt_sigpending);
      SYSCALL_MAP (rt_sigtimedwait);
      SYSCALL_MAP (rt_sigqueueinfo);
      SYSCALL_MAP (rt_sigreturn);
      SYSCALL_MAP (setpriority);
      SYSCALL_MAP (getpriority);
      SYSCALL_MAP (reboot);
      SYSCALL_MAP (setregid);
      SYSCALL_MAP (setgid);
      SYSCALL_MAP (setreuid);
      SYSCALL_MAP (setuid);
      SYSCALL_MAP (setresuid);
      SYSCALL_MAP (getresuid);
      SYSCALL_MAP (setresgid);
      SYSCALL_MAP (getresgid);
      SYSCALL_MAP (setfsuid);
      SYSCALL_MAP (setfsgid);
      SYSCALL_MAP (times);
      SYSCALL_MAP (setpgid);
      SYSCALL_MAP (getpgid);
      SYSCALL_MAP (getsid);
      SYSCALL_MAP (setsid);
      SYSCALL_MAP (getgroups);
      SYSCALL_MAP (setgroups);
      SYSCALL_MAP (uname);
      SYSCALL_MAP (sethostname);
      SYSCALL_MAP (setdomainname);
      SYSCALL_MAP (getrlimit);
      SYSCALL_MAP (setrlimit);
      SYSCALL_MAP (getrusage);
      SYSCALL_MAP (umask);
      SYSCALL_MAP (prctl);
      SYSCALL_MAP (getcpu);
      SYSCALL_MAP (gettimeofday);
      SYSCALL_MAP (settimeofday);
      SYSCALL_MAP (adjtimex);
      SYSCALL_MAP (getpid);
      SYSCALL_MAP (getppid);
      SYSCALL_MAP (getuid);
      SYSCALL_MAP (geteuid);
      SYSCALL_MAP (getgid);
      SYSCALL_MAP (getegid);
      SYSCALL_MAP (gettid);
      SYSCALL_MAP (sysinfo);
      SYSCALL_MAP (mq_open);
      SYSCALL_MAP (mq_unlink);
      SYSCALL_MAP (mq_timedsend);
      SYSCALL_MAP (mq_timedreceive);
      SYSCALL_MAP (mq_notify);
      SYSCALL_MAP (mq_getsetattr);
      SYSCALL_MAP (msgget);
      SYSCALL_MAP (msgctl);
      SYSCALL_MAP (msgrcv);
      SYSCALL_MAP (msgsnd);
      SYSCALL_MAP (semget);
      SYSCALL_MAP (semctl);
      SYSCALL_MAP (semtimedop);
      SYSCALL_MAP (semop);
      SYSCALL_MAP (shmget);
      SYSCALL_MAP (shmctl);
      SYSCALL_MAP (shmat);
      SYSCALL_MAP (shmdt);
      SYSCALL_MAP (socket);
      SYSCALL_MAP (socketpair);
      SYSCALL_MAP (bind);
      SYSCALL_MAP (listen);
      SYSCALL_MAP (accept);
      SYSCALL_MAP (connect);
      SYSCALL_MAP (getsockname);
      SYSCALL_MAP (getpeername);
      SYSCALL_MAP (sendto);
      SYSCALL_MAP (recvfrom);
      SYSCALL_MAP (setsockopt);
      SYSCALL_MAP (getsockopt);
      SYSCALL_MAP (shutdown);
      SYSCALL_MAP (sendmsg);
      SYSCALL_MAP (recvmsg);
      SYSCALL_MAP (readahead);
      SYSCALL_MAP (brk);
      SYSCALL_MAP (munmap);
      SYSCALL_MAP (mremap);
      SYSCALL_MAP (add_key);
      SYSCALL_MAP (request_key);
      SYSCALL_MAP (keyctl);
      SYSCALL_MAP (clone);
      SYSCALL_MAP (execve);

    case aarch64_sys_mmap:
      return gdb_sys_mmap2;

      SYSCALL_MAP (fadvise64);
      SYSCALL_MAP (swapon);
      SYSCALL_MAP (swapoff);
      SYSCALL_MAP (mprotect);
      SYSCALL_MAP (msync);
      SYSCALL_MAP (mlock);
      SYSCALL_MAP (munlock);
      SYSCALL_MAP (mlockall);
      SYSCALL_MAP (munlockall);
      SYSCALL_MAP (mincore);
      SYSCALL_MAP (madvise);
      SYSCALL_MAP (remap_file_pages);
      SYSCALL_MAP (mbind);
      SYSCALL_MAP (get_mempolicy);
      SYSCALL_MAP (set_mempolicy);
      SYSCALL_MAP (migrate_pages);
      SYSCALL_MAP (move_pages);
      UNSUPPORTED_SYSCALL_MAP (rt_tgsigqueueinfo);
      UNSUPPORTED_SYSCALL_MAP (perf_event_open);
      UNSUPPORTED_SYSCALL_MAP (accept4);
      UNSUPPORTED_SYSCALL_MAP (recvmmsg);

      SYSCALL_MAP (wait4);

      UNSUPPORTED_SYSCALL_MAP (prlimit64);
      UNSUPPORTED_SYSCALL_MAP (fanotify_init);
      UNSUPPORTED_SYSCALL_MAP (fanotify_mark);
      UNSUPPORTED_SYSCALL_MAP (name_to_handle_at);
      UNSUPPORTED_SYSCALL_MAP (open_by_handle_at);
      UNSUPPORTED_SYSCALL_MAP (clock_adjtime);
      UNSUPPORTED_SYSCALL_MAP (syncfs);
      UNSUPPORTED_SYSCALL_MAP (setns);
      UNSUPPORTED_SYSCALL_MAP (sendmmsg);
      UNSUPPORTED_SYSCALL_MAP (process_vm_readv);
      UNSUPPORTED_SYSCALL_MAP (process_vm_writev);
      UNSUPPORTED_SYSCALL_MAP (kcmp);
      UNSUPPORTED_SYSCALL_MAP (finit_module);
      UNSUPPORTED_SYSCALL_MAP (sched_setattr);
      UNSUPPORTED_SYSCALL_MAP (sched_getattr);
  default:
    return gdb_sys_no_syscall;
  }
}

/* Retrieve the syscall number at a ptrace syscall-stop, either on syscall entry
   or exit.  Return -1 upon error.  */

static LONGEST
aarch64_linux_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread)
{
  struct regcache *regs = get_thread_regcache (thread);
  LONGEST ret;

  /* Get the system call number from register x8.  */
  regs->cooked_read (AARCH64_X0_REGNUM + 8, &ret);

  /* On exit from a successful execve, we will be in a new process and all the
     registers will be cleared - x0 to x30 will be 0, except for a 1 in x7.
     This function will only ever get called when stopped at the entry or exit
     of a syscall, so by checking for 0 in x0 (arg0/retval), x1 (arg1), x8
     (syscall), x29 (FP) and x30 (LR) we can infer:
     1) Either inferior is at exit from successful execve.
     2) Or inferior is at entry to a call to io_setup with invalid arguments and
	a corrupted FP and LR.
     It should be safe enough to assume case 1.  */
  if (ret == 0)
    {
      LONGEST x1 = -1, fp = -1, lr = -1;
      regs->cooked_read (AARCH64_X0_REGNUM + 1, &x1);
      regs->cooked_read (AARCH64_FP_REGNUM, &fp);
      regs->cooked_read (AARCH64_LR_REGNUM, &lr);
      if (x1 == 0 && fp ==0 && lr == 0)
	return aarch64_sys_execve;
    }

  return ret;
}

/* Record all registers but PC register for process-record.  */

static int
aarch64_all_but_pc_registers_record (struct regcache *regcache)
{
  int i;

  for (i = AARCH64_X0_REGNUM; i < AARCH64_PC_REGNUM; i++)
    if (record_full_arch_list_add_reg (regcache, i))
      return -1;

  if (record_full_arch_list_add_reg (regcache, AARCH64_CPSR_REGNUM))
    return -1;

  return 0;
}

/* Handler for aarch64 system call instruction recording.  */

static int
aarch64_linux_syscall_record (struct regcache *regcache,
			      unsigned long svc_number)
{
  int ret = 0;
  enum gdb_syscall syscall_gdb;

  syscall_gdb =
    aarch64_canonicalize_syscall ((enum aarch64_syscall) svc_number);

  if (syscall_gdb < 0)
    {
      printf_unfiltered (_("Process record and replay target doesn't "
			   "support syscall number %s\n"),
			 plongest (svc_number));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     if (aarch64_all_but_pc_registers_record (regcache))
       return -1;
     return 0;
   }

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &aarch64_linux_record_tdep);
  if (ret != 0)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_X0_REGNUM))
    return -1;
  /* Record LR.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_LR_REGNUM))
    return -1;
  /* Record CPSR.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_CPSR_REGNUM))
    return -1;

  return 0;
}

/* Implement the "gcc_target_options" gdbarch method.  */

static std::string
aarch64_linux_gcc_target_options (struct gdbarch *gdbarch)
{
  /* GCC doesn't know "-m64".  */
  return {};
}

static void
aarch64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  static const char *const stap_integer_prefixes[] = { "#", "", NULL };
  static const char *const stap_register_prefixes[] = { "", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->lowest_pc = 0x8000;

  linux_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 svr4_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  tramp_frame_prepend_unwinder (gdbarch, &aarch64_linux_rt_sigframe);

  /* Enable longjmp.  */
  tdep->jb_pc = 11;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, aarch64_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description
    (gdbarch, aarch64_linux_core_read_description);

  /* SystemTap related.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					    stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					    stap_register_indirection_suffixes);
  set_gdbarch_stap_is_single_operand (gdbarch, aarch64_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					aarch64_stap_parse_special_token);

  /* Reversible debugging, process record.  */
  set_gdbarch_process_record (gdbarch, aarch64_process_record);
  /* Syscall record.  */
  tdep->aarch64_syscall_record = aarch64_linux_syscall_record;

  /* The top byte of a user space address known as the "tag",
     is ignored by the kernel and can be regarded as additional
     data associated with the address.  */
  set_gdbarch_significant_addr_bit (gdbarch, 56);

  /* Initialize the aarch64_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size__old_kernel_stat = 32;
  aarch64_linux_record_tdep.size_tms = 32;
  aarch64_linux_record_tdep.size_loff_t = 8;
  aarch64_linux_record_tdep.size_flock = 32;
  aarch64_linux_record_tdep.size_oldold_utsname = 45;
  aarch64_linux_record_tdep.size_ustat = 32;
  aarch64_linux_record_tdep.size_old_sigaction = 32;
  aarch64_linux_record_tdep.size_old_sigset_t = 8;
  aarch64_linux_record_tdep.size_rlimit = 16;
  aarch64_linux_record_tdep.size_rusage = 144;
  aarch64_linux_record_tdep.size_timeval = 16;
  aarch64_linux_record_tdep.size_timezone = 8;
  aarch64_linux_record_tdep.size_old_gid_t = 2;
  aarch64_linux_record_tdep.size_old_uid_t = 2;
  aarch64_linux_record_tdep.size_fd_set = 128;
  aarch64_linux_record_tdep.size_old_dirent = 280;
  aarch64_linux_record_tdep.size_statfs = 120;
  aarch64_linux_record_tdep.size_statfs64 = 120;
  aarch64_linux_record_tdep.size_sockaddr = 16;
  aarch64_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_msghdr = 56;
  aarch64_linux_record_tdep.size_itimerval = 32;
  aarch64_linux_record_tdep.size_stat = 144;
  aarch64_linux_record_tdep.size_old_utsname = 325;
  aarch64_linux_record_tdep.size_sysinfo = 112;
  aarch64_linux_record_tdep.size_msqid_ds = 120;
  aarch64_linux_record_tdep.size_shmid_ds = 112;
  aarch64_linux_record_tdep.size_new_utsname = 390;
  aarch64_linux_record_tdep.size_timex = 208;
  aarch64_linux_record_tdep.size_mem_dqinfo = 24;
  aarch64_linux_record_tdep.size_if_dqblk = 72;
  aarch64_linux_record_tdep.size_fs_quota_stat = 80;
  aarch64_linux_record_tdep.size_timespec = 16;
  aarch64_linux_record_tdep.size_pollfd = 8;
  aarch64_linux_record_tdep.size_NFS_FHSIZE = 32;
  aarch64_linux_record_tdep.size_knfsd_fh = 132;
  aarch64_linux_record_tdep.size_TASK_COMM_LEN = 16;
  aarch64_linux_record_tdep.size_sigaction = 32;
  aarch64_linux_record_tdep.size_sigset_t = 8;
  aarch64_linux_record_tdep.size_siginfo_t = 128;
  aarch64_linux_record_tdep.size_cap_user_data_t = 8;
  aarch64_linux_record_tdep.size_stack_t = 24;
  aarch64_linux_record_tdep.size_off_t = 8;
  aarch64_linux_record_tdep.size_stat64 = 144;
  aarch64_linux_record_tdep.size_gid_t = 4;
  aarch64_linux_record_tdep.size_uid_t = 4;
  aarch64_linux_record_tdep.size_PAGE_SIZE = 4096;
  aarch64_linux_record_tdep.size_flock64 = 32;
  aarch64_linux_record_tdep.size_user_desc = 16;
  aarch64_linux_record_tdep.size_io_event = 32;
  aarch64_linux_record_tdep.size_iocb = 64;
  aarch64_linux_record_tdep.size_epoll_event = 12;
  aarch64_linux_record_tdep.size_itimerspec = 32;
  aarch64_linux_record_tdep.size_mq_attr = 64;
  aarch64_linux_record_tdep.size_termios = 36;
  aarch64_linux_record_tdep.size_termios2 = 44;
  aarch64_linux_record_tdep.size_pid_t = 4;
  aarch64_linux_record_tdep.size_winsize = 8;
  aarch64_linux_record_tdep.size_serial_struct = 72;
  aarch64_linux_record_tdep.size_serial_icounter_struct = 80;
  aarch64_linux_record_tdep.size_hayes_esp_config = 12;
  aarch64_linux_record_tdep.size_size_t = 8;
  aarch64_linux_record_tdep.size_iovec = 16;
  aarch64_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.ioctl_TCGETS = 0x5401;
  aarch64_linux_record_tdep.ioctl_TCSETS = 0x5402;
  aarch64_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  aarch64_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  aarch64_linux_record_tdep.ioctl_TCGETA = 0x5405;
  aarch64_linux_record_tdep.ioctl_TCSETA = 0x5406;
  aarch64_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  aarch64_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  aarch64_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  aarch64_linux_record_tdep.ioctl_TCXONC = 0x540a;
  aarch64_linux_record_tdep.ioctl_TCFLSH = 0x540b;
  aarch64_linux_record_tdep.ioctl_TIOCEXCL = 0x540c;
  aarch64_linux_record_tdep.ioctl_TIOCNXCL = 0x540d;
  aarch64_linux_record_tdep.ioctl_TIOCSCTTY = 0x540e;
  aarch64_linux_record_tdep.ioctl_TIOCGPGRP = 0x540f;
  aarch64_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  aarch64_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  aarch64_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  aarch64_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  aarch64_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  aarch64_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  aarch64_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  aarch64_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  aarch64_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  aarch64_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  aarch64_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541a;
  aarch64_linux_record_tdep.ioctl_FIONREAD = 0x541b;
  aarch64_linux_record_tdep.ioctl_TIOCINQ = 0x541b;
  aarch64_linux_record_tdep.ioctl_TIOCLINUX = 0x541c;
  aarch64_linux_record_tdep.ioctl_TIOCCONS = 0x541d;
  aarch64_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541e;
  aarch64_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541f;
  aarch64_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  aarch64_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  aarch64_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  aarch64_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  aarch64_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  aarch64_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  aarch64_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  aarch64_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  aarch64_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  aarch64_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  aarch64_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  aarch64_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  aarch64_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  aarch64_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  aarch64_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  aarch64_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  aarch64_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  aarch64_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  aarch64_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  aarch64_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  aarch64_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  aarch64_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  aarch64_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  aarch64_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  aarch64_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  aarch64_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  aarch64_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545a;
  aarch64_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545b;
  aarch64_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545c;
  aarch64_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545d;
  aarch64_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545e;
  aarch64_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545f;
  aarch64_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.fcntl_F_GETLK = 5;
  aarch64_linux_record_tdep.fcntl_F_GETLK64 = 12;
  aarch64_linux_record_tdep.fcntl_F_SETLK64 = 13;
  aarch64_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  /* The AArch64 syscall calling convention: reg x0-x6 for arguments,
     reg x8 for syscall number and return value in reg x0.  */
  aarch64_linux_record_tdep.arg1 = AARCH64_X0_REGNUM + 0;
  aarch64_linux_record_tdep.arg2 = AARCH64_X0_REGNUM + 1;
  aarch64_linux_record_tdep.arg3 = AARCH64_X0_REGNUM + 2;
  aarch64_linux_record_tdep.arg4 = AARCH64_X0_REGNUM + 3;
  aarch64_linux_record_tdep.arg5 = AARCH64_X0_REGNUM + 4;
  aarch64_linux_record_tdep.arg6 = AARCH64_X0_REGNUM + 5;
  aarch64_linux_record_tdep.arg7 = AARCH64_X0_REGNUM + 6;

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/aarch64-linux.xml");
  set_gdbarch_get_syscall_number (gdbarch, aarch64_linux_get_syscall_number);

  /* Displaced stepping.  */
  set_gdbarch_max_insn_length (gdbarch, 4 * AARCH64_DISPLACED_MODIFIED_INSNS);
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					aarch64_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, aarch64_displaced_step_fixup);
  set_gdbarch_displaced_step_location (gdbarch, linux_displaced_step_location);
  set_gdbarch_displaced_step_hw_singlestep (gdbarch,
					    aarch64_displaced_step_hw_singlestep);

  set_gdbarch_gcc_target_options (gdbarch, aarch64_linux_gcc_target_options);
}

void _initialize_aarch64_linux_tdep ();
void
_initialize_aarch64_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_LINUX,
			  aarch64_linux_init_abi);
}
