/* Target-dependent code for GNU/Linux on LoongArch processors.

   Copyright (C) 2022-2025 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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

#include "arch/loongarch-syscall.h"
#include "extract-store-integer.h"
#include "gdbarch.h"
#include "glibc-tdep.h"
#include "inferior.h"
#include "linux-record.h"
#include "linux-tdep.h"
#include "solib-svr4-linux.h"
#include "loongarch-tdep.h"
#include "record-full.h"
#include "regset.h"
#include "solib-svr4.h"
#include "target-descriptions.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "value.h"
#include "xml-syscall.h"

/* The syscall's XML filename for LoongArch.  */
#define XML_SYSCALL_FILENAME_LOONGARCH "syscalls/loongarch-linux.xml"

/* Unpack an elf_gregset_t into GDB's register cache.  */

static void
loongarch_supply_gregset (const struct regset *regset,
			  struct regcache *regcache, int regnum,
			  const void *gprs, size_t len)
{
  int regsize = register_size (regcache->arch (), 0);
  const gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      regcache->raw_supply_zeroed (0);

      for (int i = 1; i < 32; i++)
	{
	  buf = (const gdb_byte*) gprs + regsize * i;
	  regcache->raw_supply (i, (const void *) buf);
	}

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_ORIG_A0_REGNUM;
      regcache->raw_supply (LOONGARCH_ORIG_A0_REGNUM, (const void *) buf);

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_PC_REGNUM;
      regcache->raw_supply (LOONGARCH_PC_REGNUM, (const void *) buf);

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_BADV_REGNUM;
      regcache->raw_supply (LOONGARCH_BADV_REGNUM, (const void *) buf);
    }
  else if (regnum == 0)
    regcache->raw_supply_zeroed (0);
  else if ((regnum > 0 && regnum < 32)
	   || regnum == LOONGARCH_ORIG_A0_REGNUM
	   || regnum == LOONGARCH_PC_REGNUM
	   || regnum == LOONGARCH_BADV_REGNUM)
    {
      buf = (const gdb_byte*) gprs + regsize * regnum;
      regcache->raw_supply (regnum, (const void *) buf);
    }
}

/* Pack the GDB's register cache value into an elf_gregset_t.  */

static void
loongarch_fill_gregset (const struct regset *regset,
			const struct regcache *regcache, int regnum,
			void *gprs, size_t len)
{
  int regsize = register_size (regcache->arch (), 0);
  gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < 32; i++)
	{
	  buf = (gdb_byte *) gprs + regsize * i;
	  regcache->raw_collect (i, (void *) buf);
	}

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_ORIG_A0_REGNUM;
      regcache->raw_collect (LOONGARCH_ORIG_A0_REGNUM, (void *) buf);

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_PC_REGNUM;
      regcache->raw_collect (LOONGARCH_PC_REGNUM, (void *) buf);

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_BADV_REGNUM;
      regcache->raw_collect (LOONGARCH_BADV_REGNUM, (void *) buf);
    }
  else if ((regnum >= 0 && regnum < 32)
	   || regnum == LOONGARCH_ORIG_A0_REGNUM
	   || regnum == LOONGARCH_PC_REGNUM
	   || regnum == LOONGARCH_BADV_REGNUM)
    {
      buf = (gdb_byte *) gprs + regsize * regnum;
      regcache->raw_collect (regnum, (void *) buf);
    }
}

/* Define the general register regset.  */

const struct regset loongarch_gregset =
{
  nullptr,
  loongarch_supply_gregset,
  loongarch_fill_gregset,
};

/* Unpack an elf_fpregset_t into GDB's register cache.  */
static void
loongarch_supply_fpregset (const struct regset *r,
			   struct regcache *regcache, int regnum,
			   const void *fprs, size_t len)
{
  const gdb_byte *buf = nullptr;
  int fprsize = register_size (regcache->arch (), LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->arch (), LOONGARCH_FIRST_FCC_REGNUM);

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
	{
	  buf = (const gdb_byte *)fprs + fprsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_FP_REGNUM + i, (const void *)buf);
	}
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
	{
	  buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	    fccsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_FCC_REGNUM + i, (const void *)buf);
	}
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_supply (LOONGARCH_FCSR_REGNUM, (const void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum < LOONGARCH_FIRST_FCC_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * (regnum - LOONGARCH_FIRST_FP_REGNUM);
      regcache->raw_supply (regnum, (const void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FCC_REGNUM && regnum < LOONGARCH_FCSR_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * (regnum - LOONGARCH_FIRST_FCC_REGNUM);
      regcache->raw_supply (regnum, (const void *)buf);
    }
  else if (regnum == LOONGARCH_FCSR_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_supply (regnum, (const void *)buf);
    }
}

/* Pack the GDB's register cache value into an elf_fpregset_t.  */
static void
loongarch_fill_fpregset (const struct regset *r,
			 const struct regcache *regcache, int regnum,
			 void *fprs, size_t len)
{
  gdb_byte *buf = nullptr;
  int fprsize = register_size (regcache->arch (), LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->arch (), LOONGARCH_FIRST_FCC_REGNUM);

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
	{
	  buf = (gdb_byte *)fprs + fprsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_FP_REGNUM + i, (void *)buf);
	}
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
	{
	  buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	    fccsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_FCC_REGNUM + i, (void *)buf);
	}
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_collect (LOONGARCH_FCSR_REGNUM, (void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum < LOONGARCH_FIRST_FCC_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * (regnum - LOONGARCH_FIRST_FP_REGNUM);
      regcache->raw_collect (regnum, (void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FCC_REGNUM && regnum < LOONGARCH_FCSR_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * (regnum - LOONGARCH_FIRST_FCC_REGNUM);
      regcache->raw_collect (regnum, (void *)buf);
    }
  else if (regnum == LOONGARCH_FCSR_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_collect (regnum, (void *)buf);
    }
}

/* Define the FP register regset.  */
const struct regset loongarch_fpregset =
{
  nullptr,
  loongarch_supply_fpregset,
  loongarch_fill_fpregset,
};

/* Unpack elf_lsxregset_t into GDB's register cache.  */

static void
loongarch_supply_lsxregset (const struct regset *regset,
			    struct regcache *regcache, int regnum,
			    const void *lsxrs, size_t len)
{
  int lsxrsize = register_size (regcache->arch (), LOONGARCH_FIRST_LSX_REGNUM);
  const gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_LSXREGSET; i++)
	{
	  buf = (const gdb_byte*) lsxrs + lsxrsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_LSX_REGNUM + i, (const void *) buf);
	}

    }
  else if (regnum >= LOONGARCH_FIRST_LSX_REGNUM && regnum < LOONGARCH_FIRST_LASX_REGNUM)
    {
      buf = (const gdb_byte*) lsxrs + lsxrsize * (regnum - LOONGARCH_FIRST_LSX_REGNUM);
      regcache->raw_supply (regnum, (const void *) buf);
    }
}

/* Pack the GDB's register cache value into an elf_lsxregset_t.  */

static void
loongarch_fill_lsxregset (const struct regset *regset,
			  const struct regcache *regcache, int regnum,
			  void *lsxrs, size_t len)
{
  int lsxrsize = register_size (regcache->arch (), LOONGARCH_FIRST_LSX_REGNUM);
  gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_LSXREGSET; i++)
	{
	  buf = (gdb_byte *) lsxrs + lsxrsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_LSX_REGNUM + i, (void *) buf);
	}
    }
  else if (regnum >= LOONGARCH_FIRST_LSX_REGNUM && regnum < LOONGARCH_FIRST_LASX_REGNUM)
    {
      buf = (gdb_byte *) lsxrs + lsxrsize * (regnum - LOONGARCH_FIRST_LSX_REGNUM);
      regcache->raw_collect (regnum, (void *) buf);
    }
}

/* Define the Loongson SIMD Extension register regset.  */

const struct regset loongarch_lsxregset =
{
  nullptr,
  loongarch_supply_lsxregset,
  loongarch_fill_lsxregset,
};

/* Unpack elf_lasxregset_t into GDB's register cache.  */

static void
loongarch_supply_lasxregset (const struct regset *regset,
			    struct regcache *regcache, int regnum,
			    const void *lasxrs, size_t len)
{
  int lasxrsize = register_size (regcache->arch (), LOONGARCH_FIRST_LASX_REGNUM);
  const gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_LASXREGSET; i++)
	{
	  buf = (const gdb_byte*) lasxrs + lasxrsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_LASX_REGNUM + i, (const void *) buf);
	}

    }
  else if (regnum >= LOONGARCH_FIRST_LASX_REGNUM
	   && regnum < LOONGARCH_FIRST_LASX_REGNUM + LOONGARCH_LINUX_NUM_LASXREGSET)
    {
      buf = (const gdb_byte*) lasxrs + lasxrsize * (regnum - LOONGARCH_FIRST_LASX_REGNUM);
      regcache->raw_supply (regnum, (const void *) buf);
    }
}

/* Pack the GDB's register cache value into an elf_lasxregset_t.  */

static void
loongarch_fill_lasxregset (const struct regset *regset,
			  const struct regcache *regcache, int regnum,
			  void *lasxrs, size_t len)
{
  int lasxrsize = register_size (regcache->arch (), LOONGARCH_FIRST_LASX_REGNUM);
  gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_LASXREGSET; i++)
	{
	  buf = (gdb_byte *) lasxrs + lasxrsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_LASX_REGNUM + i, (void *) buf);
	}
    }
  else if (regnum >= LOONGARCH_FIRST_LASX_REGNUM
	   && regnum < LOONGARCH_FIRST_LASX_REGNUM + LOONGARCH_LINUX_NUM_LASXREGSET)

    {
      buf = (gdb_byte *) lasxrs + lasxrsize * (regnum - LOONGARCH_FIRST_LASX_REGNUM);
      regcache->raw_collect (regnum, (void *) buf);
    }
}

/* Define the Loongson Advanced SIMD Extension register regset.  */

const struct regset loongarch_lasxregset =
{
  nullptr,
  loongarch_supply_lasxregset,
  loongarch_fill_lasxregset,
};

/* Unpack an lbt regset into GDB's register cache.  */

static void
loongarch_supply_lbtregset (const struct regset *regset,
			    struct regcache *regcache, int regnum,
			    const void *regs, size_t len)
{
  int scrsize = register_size (regcache->arch (), LOONGARCH_FIRST_SCR_REGNUM);
  int eflagssize = register_size (regcache->arch (), LOONGARCH_EFLAGS_REGNUM);
  const gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_SCR; i++)
	{
	  buf = (const gdb_byte *) regs + scrsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_SCR_REGNUM + i,
				(const void *) buf);
	}

      buf = (const gdb_byte*) regs + scrsize * LOONGARCH_LINUX_NUM_SCR;
      regcache->raw_supply (LOONGARCH_EFLAGS_REGNUM, (const void *) buf);

      buf = (const gdb_byte*) regs
	    + scrsize * LOONGARCH_LINUX_NUM_SCR
	    + eflagssize;
      regcache->raw_supply (LOONGARCH_FTOP_REGNUM, (const void *) buf);
    }
  else if (regnum >= LOONGARCH_FIRST_SCR_REGNUM
	   && regnum <= LOONGARCH_LAST_SCR_REGNUM)
    {
      buf = (const gdb_byte*) regs
	    + scrsize * (regnum - LOONGARCH_FIRST_SCR_REGNUM);
      regcache->raw_supply (regnum, (const void *) buf);
    }
  else if (regnum == LOONGARCH_EFLAGS_REGNUM)
    {
      buf = (const gdb_byte*) regs + scrsize * LOONGARCH_LINUX_NUM_SCR;
      regcache->raw_supply (regnum, (const void *) buf);
    }
  else if (regnum == LOONGARCH_FTOP_REGNUM)
    {
      buf = (const gdb_byte*) regs
	    + scrsize * LOONGARCH_LINUX_NUM_SCR
	    + eflagssize;
      regcache->raw_supply (regnum, (const void *) buf);
    }
}

/* Pack the GDB's register cache value into an lbt regset.  */

static void
loongarch_fill_lbtregset (const struct regset *regset,
			  const struct regcache *regcache, int regnum,
			  void *regs, size_t len)
{
  int scrsize = register_size (regcache->arch (), LOONGARCH_FIRST_SCR_REGNUM);
  int eflagssize = register_size (regcache->arch (), LOONGARCH_EFLAGS_REGNUM);
  gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_SCR; i++)
	{
	  buf = (gdb_byte *) regs + scrsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_SCR_REGNUM + i, (void *) buf);
	}

      buf = (gdb_byte *) regs + scrsize * LOONGARCH_LINUX_NUM_SCR;
      regcache->raw_collect (LOONGARCH_EFLAGS_REGNUM, (void *) buf);

      buf = (gdb_byte *) regs + scrsize * LOONGARCH_LINUX_NUM_SCR + eflagssize;
      regcache->raw_collect (LOONGARCH_FTOP_REGNUM, (void *) buf);
    }
  else if (regnum >= LOONGARCH_FIRST_SCR_REGNUM
	   && regnum <= LOONGARCH_LAST_SCR_REGNUM)
    {
      buf = (gdb_byte *) regs + scrsize * (regnum - LOONGARCH_FIRST_SCR_REGNUM);
      regcache->raw_collect (regnum, (void *) buf);
    }
  else if (regnum == LOONGARCH_EFLAGS_REGNUM)
    {
      buf = (gdb_byte *) regs + scrsize * LOONGARCH_LINUX_NUM_SCR;
      regcache->raw_collect (regnum, (void *) buf);
    }
  else if (regnum == LOONGARCH_FTOP_REGNUM)
    {
      buf = (gdb_byte *) regs + scrsize * LOONGARCH_LINUX_NUM_SCR + eflagssize;
      regcache->raw_collect (regnum, (void *) buf);
    }
}

/* Define the lbt register regset.  */

const struct regset loongarch_lbtregset =
{
  nullptr,
  loongarch_supply_lbtregset,
  loongarch_fill_lbtregset,
};

/* Implement the "init" method of struct tramp_frame.  */

#define LOONGARCH_RT_SIGFRAME_UCONTEXT_OFFSET	128
#define LOONGARCH_UCONTEXT_SIGCONTEXT_OFFSET	176

static void
loongarch_linux_rt_sigframe_init (const struct tramp_frame *self,
				  const frame_info_ptr &this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR sigcontext_base = (frame_sp + LOONGARCH_RT_SIGFRAME_UCONTEXT_OFFSET
			       + LOONGARCH_UCONTEXT_SIGCONTEXT_OFFSET);

  trad_frame_set_reg_addr (this_cache, LOONGARCH_PC_REGNUM, sigcontext_base);
  for (int i = 0; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, i, sigcontext_base + 8 + i * 8);

  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* li.w    a7, __NR_rt_sigreturn  */
#define LOONGARCH_INST_LIW_A7_RT_SIGRETURN	0x03822c0b
/* syscall 0  */
#define LOONGARCH_INST_SYSCALL			0x002b0000

static const struct tramp_frame loongarch_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { LOONGARCH_INST_LIW_A7_RT_SIGRETURN, ULONGEST_MAX },
    { LOONGARCH_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  loongarch_linux_rt_sigframe_init,
  nullptr
};

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
loongarch_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  int gprsize = register_size (gdbarch, 0);
  int gpsize = gprsize * LOONGARCH_LINUX_NUM_GREGSET;
  int fprsize = register_size (gdbarch, LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (gdbarch, LOONGARCH_FIRST_FCC_REGNUM);
  int fcsrsize = register_size (gdbarch, LOONGARCH_FCSR_REGNUM);
  int fpsize = fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	       fccsize * LOONGARCH_LINUX_NUM_FCC + fcsrsize;
  int lsxrsize = register_size (gdbarch, LOONGARCH_FIRST_LSX_REGNUM);
  int lsxsize = lsxrsize * LOONGARCH_LINUX_NUM_LSXREGSET;
  int lasxrsize = register_size (gdbarch, LOONGARCH_FIRST_LASX_REGNUM);
  int lasxsize = lasxrsize * LOONGARCH_LINUX_NUM_LASXREGSET;
  int scrsize = register_size (gdbarch, LOONGARCH_FIRST_SCR_REGNUM);
  int eflagssize = register_size (gdbarch, LOONGARCH_EFLAGS_REGNUM);
  int ftopsize = register_size (gdbarch, LOONGARCH_FTOP_REGNUM);
  int lbtsize = scrsize * LOONGARCH_LINUX_NUM_SCR + eflagssize + ftopsize;

  cb (".reg", gpsize, gpsize,
      &loongarch_gregset, nullptr, cb_data);
  cb (".reg2", fpsize, fpsize,
      &loongarch_fpregset, nullptr, cb_data);
  cb (".reg-loongarch-lsx", lsxsize, lsxsize,
      &loongarch_lsxregset, nullptr, cb_data);
  cb (".reg-loongarch-lasx", lasxsize, lasxsize,
      &loongarch_lasxregset, nullptr, cb_data);
  cb (".reg-loongarch-lbt", lbtsize, lbtsize,
      &loongarch_lbtregset, nullptr, cb_data);
}

/* The following value is derived from __NR_rt_sigreturn in
   <include/uapi/asm-generic/unistd.h> from the Linux source tree.  */

#define LOONGARCH_NR_rt_sigreturn	139

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
loongarch_linux_syscall_next_pc (const frame_info_ptr &frame)
{
  const CORE_ADDR pc = get_frame_pc (frame);
  ULONGEST a7 = get_frame_register_unsigned (frame, LOONGARCH_A7_REGNUM);

  /* If we are about to make a sigreturn syscall, use the unwinder to
     decode the signal frame.  */
  if (a7 == LOONGARCH_NR_rt_sigreturn)
    return frame_unwind_caller_pc (frame);

  return pc + 4;
}

/* Implement the "get_syscall_number" gdbarch method.  */

static LONGEST
loongarch_linux_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int regsize = register_size (gdbarch, LOONGARCH_A7_REGNUM);
  /* The content of a register.  */
  gdb_byte buf[8];
  /* The result.  */
  LONGEST ret;

  gdb_assert (regsize <= sizeof (buf));

  /* Getting the system call number from the register.
     When dealing with the LoongArch architecture, this information
     is stored at the a7 register.  */
  regcache->cooked_read (LOONGARCH_A7_REGNUM, buf);

  ret = extract_signed_integer (buf, regsize, byte_order);

  return ret;
}

static linux_record_tdep loongarch_linux_record_tdep;

/* loongarch_canonicalize_syscall maps syscall ids from the native LoongArch
   linux set of syscall ids into a canonical set of syscall ids used by
   process record. */

static enum gdb_syscall
loongarch_canonicalize_syscall (enum loongarch_syscall syscall_number)
{
#define SYSCALL_MAP(SYSCALL)			\
  case loongarch_sys_ ## SYSCALL:		\
    return gdb_sys_ ## SYSCALL

#define SYSCALL_MAP_RENAME(SYSCALL, GDB_SYSCALL)	\
  case loongarch_sys_ ## SYSCALL:			\
    return GDB_SYSCALL;

#define UNSUPPORTED_SYSCALL_MAP(SYSCALL)	\
  case loongarch_sys_ ## SYSCALL:		\
    return gdb_sys_no_syscall

  switch(syscall_number)
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

      SYSCALL_MAP_RENAME (mmap, gdb_sys_old_mmap);

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
      SYSCALL_MAP (accept4);
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
      UNSUPPORTED_SYSCALL_MAP (renameat2);
      UNSUPPORTED_SYSCALL_MAP (seccomp);
      SYSCALL_MAP (getrandom);
      UNSUPPORTED_SYSCALL_MAP (memfd_create);
      UNSUPPORTED_SYSCALL_MAP (bpf);
      UNSUPPORTED_SYSCALL_MAP (execveat);
      UNSUPPORTED_SYSCALL_MAP (userfaultfd);
      UNSUPPORTED_SYSCALL_MAP (membarrier);
      UNSUPPORTED_SYSCALL_MAP (mlock2);
      UNSUPPORTED_SYSCALL_MAP (copy_file_range);
      UNSUPPORTED_SYSCALL_MAP (preadv2);
      UNSUPPORTED_SYSCALL_MAP (pwritev2);
      UNSUPPORTED_SYSCALL_MAP (pkey_mprotect);
      UNSUPPORTED_SYSCALL_MAP (pkey_alloc);
      UNSUPPORTED_SYSCALL_MAP (pkey_free);
      SYSCALL_MAP (statx);
      UNSUPPORTED_SYSCALL_MAP (io_pgetevents);
      UNSUPPORTED_SYSCALL_MAP (rseq);
      UNSUPPORTED_SYSCALL_MAP (kexec_file_load);
      UNSUPPORTED_SYSCALL_MAP (pidfd_send_signal);
      UNSUPPORTED_SYSCALL_MAP (io_uring_setup);
      UNSUPPORTED_SYSCALL_MAP (io_uring_enter);
      UNSUPPORTED_SYSCALL_MAP (io_uring_register);
      UNSUPPORTED_SYSCALL_MAP (open_tree);
      UNSUPPORTED_SYSCALL_MAP (move_mount);
      UNSUPPORTED_SYSCALL_MAP (fsopen);
      UNSUPPORTED_SYSCALL_MAP (fsconfig);
      UNSUPPORTED_SYSCALL_MAP (fsmount);
      UNSUPPORTED_SYSCALL_MAP (fspick);
      UNSUPPORTED_SYSCALL_MAP (pidfd_open);
      UNSUPPORTED_SYSCALL_MAP (clone3);
      UNSUPPORTED_SYSCALL_MAP (close_range);
      UNSUPPORTED_SYSCALL_MAP (openat2);
      UNSUPPORTED_SYSCALL_MAP (pidfd_getfd);
      UNSUPPORTED_SYSCALL_MAP (faccessat2);
      UNSUPPORTED_SYSCALL_MAP (process_madvise);
      UNSUPPORTED_SYSCALL_MAP (epoll_pwait2);
      UNSUPPORTED_SYSCALL_MAP (mount_setattr);
      UNSUPPORTED_SYSCALL_MAP (quotactl_fd);
      UNSUPPORTED_SYSCALL_MAP (landlock_create_ruleset);
      UNSUPPORTED_SYSCALL_MAP (landlock_add_rule);
      UNSUPPORTED_SYSCALL_MAP (landlock_restrict_self);
      UNSUPPORTED_SYSCALL_MAP (process_mrelease);
      UNSUPPORTED_SYSCALL_MAP (futex_waitv);
      UNSUPPORTED_SYSCALL_MAP (set_mempolicy_home_node);
      UNSUPPORTED_SYSCALL_MAP (cachestat);
      UNSUPPORTED_SYSCALL_MAP (fchmodat2);
      UNSUPPORTED_SYSCALL_MAP (map_shadow_stack);
      UNSUPPORTED_SYSCALL_MAP (futex_wake);
      UNSUPPORTED_SYSCALL_MAP (futex_wait);
      UNSUPPORTED_SYSCALL_MAP (futex_requeue);
      UNSUPPORTED_SYSCALL_MAP (statmount);
      UNSUPPORTED_SYSCALL_MAP (listmount);
      UNSUPPORTED_SYSCALL_MAP (lsm_get_self_attr);
      UNSUPPORTED_SYSCALL_MAP (lsm_set_self_attr);
      UNSUPPORTED_SYSCALL_MAP (lsm_list_modules);
      UNSUPPORTED_SYSCALL_MAP (mseal);
      UNSUPPORTED_SYSCALL_MAP (syscalls);
    default:
      return gdb_sys_no_syscall;
    }

#undef SYSCALL_MAP
#undef SYSCALL_MAP_RENAME
#undef UNSUPPORTED_SYSCALL_MAP
}

static int
loongarch_record_all_but_pc_registers (struct regcache *regcache)
{

  /* Record General purpose Registers. */
  for (int i = 0; i < 32; ++i)
    if (record_full_arch_list_add_reg (regcache, i))
      return -1;

  /* Record orig_a0 */
  if (record_full_arch_list_add_reg (regcache, LOONGARCH_ORIG_A0_REGNUM))
    return -1;

  /* Record badvaddr */
  if (record_full_arch_list_add_reg (regcache, LOONGARCH_BADV_REGNUM))
    return -1;

  return 0;
}

/* Handler for LoongArch architecture system call instruction recording.  */

static int
loongarch_linux_syscall_record (struct regcache *regcache,
				unsigned long syscall_number)
{
  int ret = 0;
  enum gdb_syscall syscall_gdb;

  syscall_gdb =
    loongarch_canonicalize_syscall ((enum loongarch_syscall) syscall_number);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		  "support syscall number %s\n"), plongest (syscall_number));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn || syscall_gdb == gdb_sys_rt_sigreturn)
    return loongarch_record_all_but_pc_registers (regcache);

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &loongarch_linux_record_tdep);

  if (ret != 0)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, LOONGARCH_A0_REGNUM))
    return -1;

  return 0;
}

/* Initialize the loongarch_linux_record_tdep. These values are the size
   of the type that will be used in a system call. They are obtained from
   Linux Kernel source.  */

static void
init_loongarch_linux_record_tdep (struct gdbarch *gdbarch)
{
  loongarch_linux_record_tdep.size_pointer
	= gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  loongarch_linux_record_tdep.size_tms = 32;
  loongarch_linux_record_tdep.size_loff_t = 8;
  loongarch_linux_record_tdep.size_flock = 32;
  loongarch_linux_record_tdep.size_oldold_utsname = 45;
  loongarch_linux_record_tdep.size_ustat = 32;
  loongarch_linux_record_tdep.size_old_sigaction = 32;
  loongarch_linux_record_tdep.size_old_sigset_t = 8;
  loongarch_linux_record_tdep.size_rlimit = 16;
  loongarch_linux_record_tdep.size_rusage = 144;
  loongarch_linux_record_tdep.size_timeval = 16;
  loongarch_linux_record_tdep.size_timezone = 8;
  loongarch_linux_record_tdep.size_old_gid_t = 4;
  loongarch_linux_record_tdep.size_old_uid_t = 4;
  loongarch_linux_record_tdep.size_fd_set = 128;
  loongarch_linux_record_tdep.size_old_dirent = 280;
  loongarch_linux_record_tdep.size_statfs = 120;
  loongarch_linux_record_tdep.size_statfs64 = 120;
  loongarch_linux_record_tdep.size_sockaddr = 16;
  loongarch_linux_record_tdep.size_int
	    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  loongarch_linux_record_tdep.size_long
	    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  loongarch_linux_record_tdep.size_ulong
	    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  loongarch_linux_record_tdep.size_msghdr = 56;
  loongarch_linux_record_tdep.size_itimerval = 32;
  loongarch_linux_record_tdep.size_stat = 144;
  loongarch_linux_record_tdep.size_old_utsname = 325;
  loongarch_linux_record_tdep.size_sysinfo = 112;
  loongarch_linux_record_tdep.size_msqid_ds = 120;
  loongarch_linux_record_tdep.size_shmid_ds = 112;
  loongarch_linux_record_tdep.size_new_utsname = 390;
  loongarch_linux_record_tdep.size_timex = 208;
  loongarch_linux_record_tdep.size_mem_dqinfo = 72;
  loongarch_linux_record_tdep.size_if_dqblk = 72;
  loongarch_linux_record_tdep.size_fs_quota_stat = 80;
  loongarch_linux_record_tdep.size_timespec = 16;
  loongarch_linux_record_tdep.size_pollfd = 8;
  loongarch_linux_record_tdep.size_NFS_FHSIZE = 32;
  loongarch_linux_record_tdep.size_knfsd_fh = 132;
  loongarch_linux_record_tdep.size_TASK_COMM_LEN = 16;
  loongarch_linux_record_tdep.size_sigaction = 24;
  loongarch_linux_record_tdep.size_sigset_t = 8;
  loongarch_linux_record_tdep.size_siginfo_t = 128;
  loongarch_linux_record_tdep.size_cap_user_data_t = 8;
  loongarch_linux_record_tdep.size_stack_t = 24;
  loongarch_linux_record_tdep.size_off_t = 8;
  loongarch_linux_record_tdep.size_stat64 = 144;
  loongarch_linux_record_tdep.size_gid_t = 4;
  loongarch_linux_record_tdep.size_uid_t = 4;
  loongarch_linux_record_tdep.size_PAGE_SIZE = 0x4000;
  loongarch_linux_record_tdep.size_flock64 = 32;
  loongarch_linux_record_tdep.size_user_desc = 16;
  loongarch_linux_record_tdep.size_io_event = 32;
  loongarch_linux_record_tdep.size_iocb = 64;
  loongarch_linux_record_tdep.size_epoll_event = 12;
  loongarch_linux_record_tdep.size_itimerspec = 32;
  loongarch_linux_record_tdep.size_mq_attr = 64;
  loongarch_linux_record_tdep.size_termios = 36;
  loongarch_linux_record_tdep.size_termios2 = 44;
  loongarch_linux_record_tdep.size_pid_t = 4;
  loongarch_linux_record_tdep.size_winsize = 8;
  loongarch_linux_record_tdep.size_serial_struct = 72;
  loongarch_linux_record_tdep.size_serial_icounter_struct = 80;
  loongarch_linux_record_tdep.size_hayes_esp_config = 12;
  loongarch_linux_record_tdep.size_size_t = 8;
  loongarch_linux_record_tdep.size_iovec = 16;
  loongarch_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  loongarch_linux_record_tdep.ioctl_TCGETS = 0x5401;
  loongarch_linux_record_tdep.ioctl_TCSETS = 0x5402;
  loongarch_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  loongarch_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  loongarch_linux_record_tdep.ioctl_TCGETA = 0x5405;
  loongarch_linux_record_tdep.ioctl_TCSETA = 0x5406;
  loongarch_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  loongarch_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  loongarch_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  loongarch_linux_record_tdep.ioctl_TCXONC = 0x540a;
  loongarch_linux_record_tdep.ioctl_TCFLSH = 0x540b;
  loongarch_linux_record_tdep.ioctl_TIOCEXCL = 0x540c;
  loongarch_linux_record_tdep.ioctl_TIOCNXCL = 0x540d;
  loongarch_linux_record_tdep.ioctl_TIOCSCTTY = 0x540e;
  loongarch_linux_record_tdep.ioctl_TIOCGPGRP = 0x540f;
  loongarch_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  loongarch_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  loongarch_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  loongarch_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  loongarch_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  loongarch_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  loongarch_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  loongarch_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  loongarch_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  loongarch_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  loongarch_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541a;
  loongarch_linux_record_tdep.ioctl_FIONREAD = 0x541b;
  loongarch_linux_record_tdep.ioctl_TIOCINQ = 0x541b;
  loongarch_linux_record_tdep.ioctl_TIOCLINUX = 0x541c;
  loongarch_linux_record_tdep.ioctl_TIOCCONS = 0x541d;
  loongarch_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541e;
  loongarch_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541f;
  loongarch_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  loongarch_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  loongarch_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  loongarch_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  loongarch_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  loongarch_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  loongarch_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  loongarch_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  loongarch_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  loongarch_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  loongarch_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  loongarch_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  loongarch_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  loongarch_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  loongarch_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  loongarch_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  loongarch_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  loongarch_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  loongarch_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  loongarch_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  loongarch_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  loongarch_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  loongarch_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  loongarch_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  loongarch_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  loongarch_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  loongarch_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545a;
  loongarch_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545b;
  loongarch_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545c;
  loongarch_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545d;
  loongarch_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545e;
  loongarch_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545f;
  loongarch_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source. */
  loongarch_linux_record_tdep.fcntl_F_GETLK = 5;
  loongarch_linux_record_tdep.fcntl_F_GETLK64 = 12;
  loongarch_linux_record_tdep.fcntl_F_SETLK64 = 13;
  loongarch_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  loongarch_linux_record_tdep.arg1 = LOONGARCH_A0_REGNUM + 0;
  loongarch_linux_record_tdep.arg2 = LOONGARCH_A0_REGNUM + 1;
  loongarch_linux_record_tdep.arg3 = LOONGARCH_A0_REGNUM + 2;
  loongarch_linux_record_tdep.arg4 = LOONGARCH_A0_REGNUM + 3;
  loongarch_linux_record_tdep.arg5 = LOONGARCH_A0_REGNUM + 4;
  loongarch_linux_record_tdep.arg6 = LOONGARCH_A0_REGNUM + 5;
  loongarch_linux_record_tdep.arg7 = LOONGARCH_A0_REGNUM + 6;
}

/* Initialize LoongArch Linux ABI info.  */

static void
loongarch_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  loongarch_gdbarch_tdep *tdep = gdbarch_tdep<loongarch_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  set_solib_svr4_ops (gdbarch, (info.bfd_arch_info->bits_per_address == 32
				? make_linux_ilp32_svr4_solib_ops
				: make_linux_lp64_svr4_solib_ops));

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch, svr4_fetch_objfile_link_map);

  /* Prepend tramp frame unwinder for signal.  */
  tramp_frame_prepend_unwinder (gdbarch, &loongarch_linux_rt_sigframe);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections (gdbarch, loongarch_iterate_over_regset_sections);

  tdep->syscall_next_pc = loongarch_linux_syscall_next_pc;

  /* Set the correct XML syscall filename.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_LOONGARCH);

  /* Get the syscall number from the arch's register.  */
  set_gdbarch_get_syscall_number (gdbarch, loongarch_linux_get_syscall_number);

  /* Reversible debugging, process record.  */
  set_gdbarch_process_record (gdbarch, loongarch_process_record);

  /* Syscall record.  */
  tdep->loongarch_syscall_record = loongarch_linux_syscall_record;
  init_loongarch_linux_record_tdep (gdbarch);
}

/* Initialize LoongArch Linux target support.  */

INIT_GDB_FILE (loongarch_linux_tdep)
{
  gdbarch_register_osabi (bfd_arch_loongarch, bfd_mach_loongarch32,
			  GDB_OSABI_LINUX, loongarch_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_loongarch, bfd_mach_loongarch64,
			  GDB_OSABI_LINUX, loongarch_linux_init_abi);
}
